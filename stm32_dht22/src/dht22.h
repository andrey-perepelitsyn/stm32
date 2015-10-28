/*
 * dht22.h
 *
 *  Created on: Oct 23, 2015
 *      Author: Andrey Perepelitsyn
 *
 *  definitions for DHT22 manipulation
 */

#ifndef DHT22_H_
#define DHT22_H_

#include <stdint.h>
#include <cmsis_device.h>
#include <stm32f0xx_conf.h>

/*
 * choose your port and pin
 * to keep things simple, only pins 4..15 are allowed
 */
#define DHT22_PORT      GPIOB
#define DHT22_PIN_NUM   ((uint8_t)9)
#define DHT22_AHBPERIPH RCC_AHBPeriph_GPIOB
#define DHT22_EXTI_PORT EXTI_PortSourceGPIOB

/*
 *  sync and async versions both use SysTick interrupt, so you must choose only on of them.
 *  as an option, just exclude dht22.c or dht22_dumb.c from build.
 */
#undef DHT22_ASYNC
//#define DHT22_ASYNC

/*
 * some timings, in usec
 */
#define DHT22_MAX_ZERO_LEN     40    // according to datasheet, "0" pulse is 26~28, "1" pulse is 70
#define DHT22_START_STROBE_LEN 1000  // in usec. according to datasheet, reasonable value is 1~10 msec
#define DHT22_TIMEOUT          2000  // how long to wait, before consider reading failed. must be greater than "1" length

#define DHT22_PIN       ((uint16_t)(1 << DHT22_PIN_NUM))
#define DHT22_EXTI_LINE ((uint32_t)(1 << DHT22_PIN_NUM))

typedef enum {
	DHT22_done = 0,
	DHT22_sendStrobe0 = 1,
	DHT22_sendStrobe1 = 2,
	DHT22_waitFallingEdge = 3,
	DHT22_waitRisingEdge = 4
} dht22_metering_stage_t;

typedef enum {
	DHT22_OK = 0,
	DHT22_ERROR = -1,
	DHT22_ERR_TIMEOUT = -2,
	DHT22_ERR_METERING = -3,
	DHT22_ERR_CHECKSUM = -4
} dht22_result_t;

typedef struct dht22_struct {
	// "public" members
	int16_t  temperature;
	uint16_t humidity;
	dht22_result_t result;
	// "private" members
	dht22_metering_stage_t metering_stage;
	uint32_t timeout;
	uint32_t max_zero_duration;
	void (*done_cb)(struct dht22_struct *);
	uint8_t raw_data[5];
	uint8_t bits_left;
	uint8_t current_byte;
	uint8_t checksum;
} dht22_t;

/*
 * \brief async version of function
 *        start metering process, call \param data->done_cb when done
 * \param data points to data struct, where obtained data should be stored
 * \return immediately, DHT22_OK if metering process started successfully
 */
dht22_result_t dht22_start_metering(dht22_t *data);

/*
 * \brief sync version, not using external interrupt
 * \param data points to data stuct, where obtained data should be stored
 * \return when metering done, DHT22_OK if metering process started successfully
 */
dht22_result_t dht22_dumb_read_sensor(dht22_t *data);

/*
 * \brief external interrupt handler for async version
 */
void EXTI4_15_IRQHandler(void);

/*
 * \brief SysTick interrupt for time measuring
 */
void SysTick_Handler(void);

/*
 * \brief empty-loop-style wait function
 * \param ticks time in SysTick ticks. Limited by 2^24!
 */
void dht22_wait(uint32_t ticks);

#endif /* DHT22_H_ */
