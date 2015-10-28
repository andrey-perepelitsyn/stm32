/*
 * dht22.c
 *
 *  Created on: Oct 24, 2015
 *      Author: Andrey Perepelitsyn
 *
 *  DHT22 manipulation logic. Asynchronus version, using GPIO interrupts
 *
 *  Warning! Uses SysTick for time measuring.
 *  TODO: make alternative way of time measuring for using with RTOSes
 *
 *  TODO: translate comments:)
 * 1. настроить ногу как выход по схеме "open drain" и выставить "0"
 * 2. настроить обработчик прерывания от таймера и выставить таймер как минимум на 1мс
 * 3. в обработчике от таймера:
 * 3.1 настроить ногу как вход
 * 3.2 запустить таймер с обработчиком для фиксации таймаута и измерения длительности импульсов
 * 3.3 настроить обработчик прерываний по фронтам на ноге
 * 3.4 если вызван по таймауту, снять обработчики, записать ошибку в выходные данные, вызвать done_cb
 * 4. в обработчике от событий на ноге:
 * 4.1 засечь длительность импульса, сохранить бит, перезапустить таймер таймаута
 * 4.2 если импульсов набралось достаточное количество, снять обработчики прерываний, посчитать результат и вызвать done_cb
 *
 */

#include "dht22.h"

#ifdef DHT22_ASYNC

dht22_t *dht22_data;

GPIO_InitTypeDef GPIO_initStruct;
EXTI_InitTypeDef EXTI_initStruct;
NVIC_InitTypeDef NVIC_initStruct;

dht22_result_t dht22_start_metering(dht22_t *data)
{
	// 1
	RCC_AHBPeriphClockCmd(DHT22_AHBPERIPH, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	GPIO_initStruct.GPIO_Pin = DHT22_PIN;
	GPIO_initStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_initStruct.GPIO_Speed = GPIO_Speed_Level_3;
	GPIO_initStruct.GPIO_OType = GPIO_OType_OD;
	GPIO_initStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(DHT22_PORT, &GPIO_initStruct);
	GPIO_ResetBits(DHT22_PORT, DHT22_PIN);
	// 2
	dht22_data = data;
	dht22_data->metering_stage = DHT22_sendStrobe0;
	dht22_data->result = DHT22_ERR_METERING;
	dht22_data->timeout = SystemCoreClock / (1000000 / DHT22_TIMEOUT);
	dht22_data->max_zero_duration = SystemCoreClock / (1000000 / DHT22_MAX_ZERO_LEN);
	dht22_data->current_byte = 0;
	dht22_data->bits_left = 10; // учитывая два начальных импульса
	dht22_data->checksum = 0;

	SysTick_Config(SystemCoreClock / 500); // 2 ms

	return DHT22_OK;
}

void SysTick_Handler(void)
{
	if(dht22_data->metering_stage == DHT22_done)
		return;
	if(dht22_data->metering_stage == DHT22_sendStrobe0) {
		// 3.1
		GPIO_initStruct.GPIO_Pin = DHT22_PIN;
		GPIO_initStruct.GPIO_Mode = GPIO_Mode_IN;
		GPIO_Init(DHT22_PORT, &GPIO_initStruct);
		// 3.2
		dht22_data->metering_stage = DHT22_waitRisingEdge;
		SysTick_Config(dht22_data->timeout);
		// 3.3
		SYSCFG_EXTILineConfig(DHT22_EXTI_PORT, DHT22_PIN_NUM);
		EXTI_initStruct.EXTI_Line = DHT22_EXTI_LINE;
		EXTI_initStruct.EXTI_Mode = EXTI_Mode_Interrupt;
		EXTI_initStruct.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
		EXTI_initStruct.EXTI_LineCmd = ENABLE;
		EXTI_Init(&EXTI_initStruct);

		NVIC_initStruct.NVIC_IRQChannel = EXTI4_15_IRQn;
		NVIC_initStruct.NVIC_IRQChannelPriority = 0x00;
		NVIC_initStruct.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_initStruct);
	}
	else {
		// 3.4
		SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);
		NVIC_initStruct.NVIC_IRQChannel = EXTI4_15_IRQn;
		NVIC_initStruct.NVIC_IRQChannelPriority = 0x00;
		NVIC_initStruct.NVIC_IRQChannelCmd = DISABLE;
		NVIC_Init(&NVIC_initStruct);
		dht22_data->result = DHT22_ERR_TIMEOUT;
		dht22_data->done_cb(dht22_data);
	}
}

void EXTI4_15_IRQHandler(void)
{
	if(EXTI_GetITStatus(DHT22_EXTI_LINE) != RESET) {
		// 4.1
		if(!(DHT22_PORT->IDR & DHT22_PIN)) {
			// кончился импульс "1", значит можно вычислить бит
			dht22_data->metering_stage = DHT22_waitRisingEdge;
			// вдвинем нулевой битик в данные
			dht22_data->raw_data[dht22_data->current_byte] <<= 1;
			// сколько длился импульс?
			if((dht22_data->timeout - SysTick->VAL) > dht22_data->max_zero_duration) {
				// это единица, добавим её в нулевую позицию
				dht22_data->raw_data[dht22_data->current_byte]++;
			}
			if(--(dht22_data->bits_left) == 0) {
				// байт заполнен, идём к следующему
				dht22_data->bits_left = 8;
				if(++(dht22_data->current_byte) == 5) {
					// все байты прочитаны, пора закругляться
					// 4.2
					// запретим прерывания SysTick
					SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);
					// запретим прерывания от _нашей_ ноги
					EXTI_initStruct.EXTI_LineCmd = DISABLE;
					EXTI_Init(&EXTI_initStruct);
					// вообще-то следует запретить все прерывания, которые обрабатывает наш обработчик:)
					// измерение ведь закончено
					NVIC_initStruct.NVIC_IRQChannelCmd = DISABLE;
					NVIC_Init(&NVIC_initStruct);
					// наконец-то, оформляем результат
					dht22_data->humidity = (dht22_data->raw_data[0] << 8) + dht22_data->raw_data[1];
					if(dht22_data->raw_data[2] & 128)
						dht22_data->temperature = -(((dht22_data->raw_data[2] & 127) << 8) + dht22_data->raw_data[3]);
					else
						dht22_data->temperature = (dht22_data->raw_data[2] << 8) + dht22_data->raw_data[3];
					// проверяем контрольную сумму
					dht22_data->result = (dht22_data->raw_data[4] == dht22_data->checksum) ? DHT22_OK : DHT22_ERR_CHECKSUM;
					// change status of metering
					dht22_data->metering_stage = DHT22_done;
					// вызываем колбэк с результатом
					dht22_data->done_cb(dht22_data);
				}
				else {
					// это был любой байт, кроме последнего. добавляем его к контрольной сумме!
					dht22_data->checksum += dht22_data->raw_data[dht22_data->current_byte - 1];
				}
			}
		}
		else {
			dht22_data->metering_stage = DHT22_waitFallingEdge;
		}
		// перезапустим таймер
		SysTick_Config(dht22_data->timeout);
		EXTI_ClearITPendingBit(DHT22_EXTI_LINE);
	}
}

#endif // DHT22_ASYNC
