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

void main(void)
{
	dht22_t dht22;
	RCC_ClocksTypeDef rcc_clocks;
	int i;

	uint32_t fb[96*9/4];
	unsigned char buf[40];

	// At this stage the system clock should have already been configured
	// at high speed.
	trace_printf("Let the show begin! (at %d Hz)\n", SystemCoreClock);

	RCC_GetClocksFreq(&rcc_clocks);
	trace_printf(
			"SYSCLK_Frequency:    %8u\n"
			"HCLK_Frequency:      %8u\n"
			"PCLK_Frequency:      %8u\n"
			"ADCCLK_Frequency:    %8u\n"
			"CECCLK_Frequency:    %8u\n"
			"I2C1CLK_Frequency:   %8u\n"
			"USART1CLK_Frequency: %8u\n",
			rcc_clocks.SYSCLK_Frequency, rcc_clocks.HCLK_Frequency, rcc_clocks.PCLK_Frequency, rcc_clocks.ADCCLK_Frequency,
			rcc_clocks.CECCLK_Frequency, rcc_clocks.I2C1CLK_Frequency, rcc_clocks.USART1CLK_Frequency);

	lcd_init(fb);
	lcd_clear();
	lcd_set_flags(LCD_FLAG_SCROLL);
	lcd_puts("Hello, world!\n");

#ifdef DHT22_ASYNC

	// using async version:
	dht22.done_cb = metering_done;
	dht22_start_metering(&dht22);

	while(1);

#else

	// using sync version:
	while (1)
	{
		if(dht22_dumb_read_sensor(&dht22) == DHT22_OK)
			sprintf(buf, "\nt:%d.%d, h:%d.%d", dht22.temperature / 10, dht22.temperature % 10, dht22.humidity / 10, dht22.humidity % 10);
		else
			sprintf(buf, "\ngot error %d", dht22.result);
		lcd_puts(buf);
		for(i = 0; i < 3000; i++)
			dht22_wait(SystemCoreClock / 1000);
	}
#endif // DHT22_ASYNC
}

