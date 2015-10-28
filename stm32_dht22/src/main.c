/*
 *  Example program for demonstration of stm32_dht22 library
 */

#include "diag/Trace.h"
#include <cmsis_device.h>
#include <stm32f0xx_conf.h>

#include "dht22.h"

static void metering_done(dht22_t *data)
{
	if(data->result == DHT22_OK)
		trace_printf("temperature: %d, humidity: %d\n", data->temperature, data->humidity);
	else
		trace_printf("got error %d\n", data->result);
}

int main(int argc, char* argv[])
{
	dht22_t dht22;

	// At this stage the system clock should have already been configured
	// at high speed.
	trace_printf("Let the show begin! (at %d Hz)\n", SystemCoreClock);

#ifdef DHT22_ASYNC

	// using async version:
	dht22.done_cb = metering_done;
	dht22_start_metering(&dht22);

	while(1);

#else

	// using sync version:
	while (1)
	{
		int i;
		// Add your code here.
		if(dht22_dumb_read_sensor(&dht22) == DHT22_OK)
			trace_printf("temperature: %d, humidity: %d\n", dht22.temperature, dht22.humidity);
		else
			trace_printf("got error %d\n", dht22.result);
		for(i = 0; i < 3000; i++)
			dht22_wait(SystemCoreClock / 1000);
	}
#endif // DHT22_ASYNC
}

