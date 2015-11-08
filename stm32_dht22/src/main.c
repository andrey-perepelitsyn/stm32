/*
 *  Example program for demonstration of stm32_dht22 library
 */

#include "diag/Trace.h"
#include <cmsis_device.h>
#include <stm32f0xx_conf.h>
#include <stdio.h>

#include "dht22.h"
#include "lcd_nokia.h"

static void metering_done(dht22_t *data)
{
	if(data->result == DHT22_OK)
		trace_printf("temperature: %d, humidity: %d\n", data->temperature, data->humidity);
	else
		trace_printf("got error %d\n", data->result);
}

static void my_wait(uint32_t msec)
{
	dht22_wait(SystemCoreClock / 1000 * msec);
}

void main(void)
{
	dht22_t dht22;
	RCC_ClocksTypeDef rcc_clocks;
	int i;

	uint32_t fb[96*9/4];
	unsigned char buf[200];

	lcd_init(fb, NULL);
	lcd_clear();
	lcd_set_flags(LCD_FLAG_SCROLL);
	RCC_GetClocksFreq(&rcc_clocks);
	SysTick_Config(10000000L);
	lcd_fb_show();
	sprintf(buf,
			"SYS:   %8lu\n"
			"AHB:   %8lu\n"
			"APB:   %8lu\n"
			"ADC:   %8lu\n"
			"CEC:   %8lu\n"
			"I2C1:  %8lu\n"
			"USART1:%8lu\n"
			"fb_show():%lu",
			rcc_clocks.SYSCLK_Frequency, rcc_clocks.HCLK_Frequency, rcc_clocks.PCLK_Frequency, rcc_clocks.ADCCLK_Frequency,
			rcc_clocks.CECCLK_Frequency, rcc_clocks.I2C1CLK_Frequency, rcc_clocks.USART1CLK_Frequency,
			10000000UL - SysTick->VAL);
	lcd_puts(buf);

#ifdef DHT22_ASYNC

	// using async version:
	dht22.done_cb = metering_done;
	dht22_start_metering(&dht22);

	while(1);

#else

	// using sync version:
	while (1)
	{
		SysTick_Config(10000000);
		lcd_dumb_wait(10);
		sprintf(buf, "\n%lu", 10000000 - SysTick->VAL);
		lcd_puts(buf);
		for(i = 0; i < 3000; i++)
			dht22_wait(SystemCoreClock / 1000);
		if(dht22_dumb_read_sensor(&dht22) == DHT22_OK)
			sprintf(buf, "\nt:%d.%d, h:%d.%d", dht22.temperature / 10, dht22.temperature % 10, dht22.humidity / 10, dht22.humidity % 10);
		else
			sprintf(buf, "\ngot error %d", dht22.result);
		lcd_puts(buf);
	}
#endif // DHT22_ASYNC
}

