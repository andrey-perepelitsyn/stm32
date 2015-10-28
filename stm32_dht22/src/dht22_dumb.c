/*
 * dht22_dumb.c
 *
 *  Created on: Oct 25, 2015
 *      Author: Andrey Perepelitsyn
 *
 *  DHT22 manipulation logic. Dumb synchronous version, w/o GPIO interrupts use
 *
 */

#include "dht22.h"

#ifndef DHT22_ASYNC

dht22_t *dht22_data;

GPIO_InitTypeDef GPIO_initStruct;
EXTI_InitTypeDef EXTI_initStruct;
NVIC_InitTypeDef NVIC_initStruct;
volatile uint8_t timer_is_going;


void dht22_wait(uint32_t ticks)
{
	timer_is_going = 1;
	SysTick_Config(ticks);
	while(timer_is_going);
}

static uint32_t dht22_wait_for_0(uint32_t ticks)
{
	timer_is_going = 1;
	SysTick_Config(ticks);
	while(timer_is_going)
		if(!(DHT22_PORT->IDR & DHT22_PIN))
			return (ticks - SysTick->VAL);
	return 0;
}

static uint32_t dht22_wait_for_1(uint32_t ticks)
{
	timer_is_going = 1;
	SysTick_Config(ticks);
	while(timer_is_going)
		if(DHT22_PORT->IDR & DHT22_PIN)
			return (ticks - SysTick->VAL);
	return 0;
}

dht22_result_t dht22_dumb_read_sensor(dht22_t *data)
{
	uint32_t timeout = SystemCoreClock / 500;
	uint32_t max_zero_duration = SystemCoreClock / (1000000 / 40);
	uint8_t i, j;
	uint8_t raw_data[5];

	// выставляем "0"
	RCC_AHBPeriphClockCmd(DHT22_AHBPERIPH, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	GPIO_initStruct.GPIO_Pin = DHT22_PIN;
	GPIO_initStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_initStruct.GPIO_Speed = GPIO_Speed_Level_3;
	GPIO_initStruct.GPIO_OType = GPIO_OType_OD;
	GPIO_initStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(DHT22_PORT, &GPIO_initStruct);
	GPIO_ResetBits(DHT22_PORT, DHT22_PIN);
	// ждем 2 мс
	dht22_wait(timeout);
	// переключаем ногу на вход
	GPIO_initStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(DHT22_PORT, &GPIO_initStruct);
	// начинаем считать импульсы
	data->result = DHT22_ERR_TIMEOUT;
	// первые два импульса пропускаем
	if(!dht22_wait_for_0(timeout))
		return data->result;
	if(!dht22_wait_for_1(timeout))
		return data->result;
	if(!dht22_wait_for_0(timeout))
		return data->result;
	// читаем 40 бит, распихивая их по байтам
	for(i = 0; i < 5; i++)
	{
		for(j = 0; j < 8; j++)
		{
			uint32_t duration;
			// ждем единицы. если слишком долго -- возвращаем ошибку
			if(!dht22_wait_for_1(timeout))
				return data->result;
			// ждем нуля. длительность ожидания записываем. если слишком долго -- возвращаем ошибку
			if(!(duration = dht22_wait_for_0(timeout)))
				return data->result;
			raw_data[i] <<= 1;
			if(duration > max_zero_duration)
				 raw_data[i]++;
		}
	}
	// проверяем контрольную сумму
	if(((raw_data[0] + raw_data[1] + raw_data[2] + raw_data[3]) & 255) != raw_data[4]) {
		data->result = DHT22_ERR_CHECKSUM;
		return data->result;
	}
	// наконец-то, возвращаем результат
	data->humidity = (raw_data[0] << 8) + raw_data[1];
	if(raw_data[2] & 128)
		data->temperature = -(((raw_data[2] & 127) << 8) + raw_data[3]);
	else
		data->temperature = (raw_data[2] << 8) + raw_data[3];
	data->result = DHT22_OK;
	return data->result;
}

void SysTick_Handler(void)
{
	timer_is_going = 0;
}

#endif // DHT22_ASYNC
