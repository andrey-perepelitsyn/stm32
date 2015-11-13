/*
 * lcd_nokia.c
 *
 *  Created on: Oct 31, 2015
 *      Author: Andrey Perepelitsyn
 */

#include "diag/Trace.h"
#include "lcd_nokia.h"
#include "lcd_chars.h"
#include "dht22.h"

const uint8_t lcd_init_sequence[] = {
	0xEB, // Thermal comp. on
	0x2F, // Supply mode
	0xA1, // Horisontal reverse: Reverse - 0xA9, Normal - 0xA1
//	0xC9, // Vertical reverse (comment if no need)
	0xA6, // Positive - A7, Negative - A6
	0x90, // Contrast 0x90...0x9F
	0xEC, // Set refresh rate to 80 Hz
	0xAF, // Enable LCD
	0xA4  // Display all points normal
};

lcd_state_t lcd_state;

GPIO_InitTypeDef lcd_gpio_config;
SPI_InitTypeDef  lcd_spi_config;

void lcd_dumb_wait(uint32_t msec)
{
	uint32_t i, j, k = SystemCoreClock / 11000;
	for(i = 0; i < msec; i++)
	{
		for(j = 0; j < k; j++)
			__NOP();
	}
}

uint16_t lcd_init(framebuffer_t *framebuffer, lcd_wait_func_t wait_func)
{
	uint16_t i;

	// use framebuffer
	lcd_state.framebuffer = framebuffer;
	// scrolling is on by default, if we have a framebuffer
	// UTF-8 is on by default, because I use Eclipse
	lcd_state.flags = framebuffer != NULL ? LCD_FLAG_SCROLL|LCD_FLAG_UTF8CYR : LCD_FLAG_UTF8CYR;
	// use user :) wait function or default one
	lcd_state.wait_func = wait_func == NULL ? lcd_dumb_wait : wait_func;

	// init clocks
	RCC_APB2PeriphClockCmd(LCD_SPI_CLK, ENABLE);
	RCC_AHBPeriphClockCmd(LCD_GPIO_CLK, ENABLE);
	/*
	 * init GPIOs.
	 * since there be no reading, MISO is not used,
	 * and pin is configured as output GPIO for RESET signal
	 */
	// switch SCK and MOSI pins to alternate function
	GPIO_PinAFConfig(LCD_PORT, LCD_SCK_PIN_NUM, GPIO_AF_0);
	GPIO_PinAFConfig(LCD_PORT, LCD_MOSI_PIN_NUM, GPIO_AF_0);

	// set common parameters
	lcd_gpio_config.GPIO_Mode = GPIO_Mode_AF;
	lcd_gpio_config.GPIO_OType = GPIO_OType_PP;
	lcd_gpio_config.GPIO_PuPd = GPIO_PuPd_NOPULL;
	lcd_gpio_config.GPIO_Speed = GPIO_Speed_50MHz;

	// config SCK pin
	lcd_gpio_config.GPIO_Pin = LCD_SCK_PIN;
	GPIO_Init(LCD_PORT, &lcd_gpio_config);

	// config MOSI pin
	lcd_gpio_config.GPIO_Pin = LCD_MOSI_PIN;
	GPIO_Init(LCD_PORT, &lcd_gpio_config);

	// config MISO pin
	lcd_gpio_config.GPIO_Mode = GPIO_Mode_OUT;
	lcd_gpio_config.GPIO_Pin = LCD_RESET_PIN;
	GPIO_Init(LCD_PORT, &lcd_gpio_config);
	/*
	 * init SPI
	 * hell bunch of configurations :-\
	 */
	// just in case
	SPI_I2S_DeInit(LCD_SPI);

	// config SPI
	lcd_spi_config.SPI_Direction = SPI_Direction_1Line_Tx;
	lcd_spi_config.SPI_Mode = SPI_Mode_Master;
	lcd_spi_config.SPI_DataSize = SPI_DataSize_9b;
	lcd_spi_config.SPI_CPOL = SPI_CPOL_Low;
	lcd_spi_config.SPI_CPHA = SPI_CPHA_1Edge;
//	lcd_spi_config.SPI_CPOL = SPI_CPOL_High;
//	lcd_spi_config.SPI_CPHA = SPI_CPHA_2Edge;
	lcd_spi_config.SPI_NSS = SPI_NSS_Soft;
	lcd_spi_config.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	lcd_spi_config.SPI_FirstBit = SPI_FirstBit_MSB;
	lcd_spi_config.SPI_CRCPolynomial = 7; // ?
	SPI_Init(LCD_SPI, &lcd_spi_config);

	// enable NSS
	SPI_NSSInternalSoftwareConfig(LCD_SPI, SPI_NSSInternalSoft_Set);

	// enable SPI
	SPI_Cmd(LCD_SPI, ENABLE);

	// finally, send RESET signal
	GPIO_ResetBits(LCD_PORT, LCD_RESET_PIN);

	lcd_state.wait_func(5); // let RESET last 5 msec!
	GPIO_SetBits(LCD_PORT, LCD_RESET_PIN);

//	// send Reading Mode command
//	lcd_write_command(0xdb);
//	// switch SPI to read
//	SPI_BiDirectionalLineConfig(LCD_SPI, SPI_Direction_Rx);
//	while(SPI_GetReceptionFIFOStatus(LCD_SPI) < SPI_ReceptionFIFOStatus_HalfFull);
//	data = SPI_I2S_ReceiveData16(LCD_SPI);
//	// switch SPI back to write
//	SPI_BiDirectionalLineConfig(LCD_SPI, SPI_Direction_Tx);

	// send command of internal display reset
	lcd_write_command(0xE2);
	// wait...
	lcd_state.wait_func(20);
	// send init commands to display
	for(i = 0; i < sizeof(lcd_init_sequence); i++)
		lcd_write_command(lcd_init_sequence[i]);

	return 0;
}

void lcd_set_cursor(uint8_t line, uint8_t column)
{
	line &= 7;
	column &= 127;
	if(column > 95) column -= 96;
	lcd_state.current_line = line;
	lcd_state.current_column = column;
	lcd_write_command(0xB0 | line);
	lcd_write_command(0x10 | (column >> 4));
	lcd_write_command(column & 0x0F);
}

void lcd_clear(void)
{
	int i;
	uint8_t spaceholder = 0, *p = (uint8_t *)lcd_state.framebuffer;

	if(lcd_state.framebuffer != NULL) {
		for(i = 0; i < sizeof(lcd_state.framebuffer); i++)
			*p++ = spaceholder;
		lcd_write_raw(lcd_state.framebuffer, sizeof(lcd_state.framebuffer));
	}
	else
		for(i = 0; i < sizeof(lcd_state.framebuffer); i++)
			lcd_write_raw(&spaceholder, 1);

	lcd_set_cursor(0, 0);
}

void lcd_write_raw(const uint8_t *data, int length)
{
	int i;
	for(i = 0; i < length; i++)
	{
		while(SPI_GetTransmissionFIFOStatus(LCD_SPI) >= SPI_TransmissionFIFOStatus_HalfFull);
		SPI_I2S_SendData16(LCD_SPI, data[i] | 0x100);

	}
}

void lcd_fb_write_raw(const uint8_t *data, int length)
{
	int i;
	uint8_t *dest;
	lcd_write_raw(data, length);
	if(lcd_state.framebuffer != NULL) {
		dest = &(lcd_state.framebuffer[lcd_state.current_line][lcd_state.current_column]);
		for(i = 0; i < length; i++)
			*dest++ = *data++;
	}
}

void lcd_write_command(uint8_t command)
{
	while(SPI_GetTransmissionFIFOStatus(LCD_SPI) >= SPI_TransmissionFIFOStatus_HalfFull);
	SPI_I2S_SendData16(LCD_SPI, command);
}

void lcd_fb_show(void)
{
	if(lcd_state.framebuffer != NULL) {
		// set pointer to video-RAM beginning
		lcd_write_command(0xB0);
		lcd_write_command(0x10);
		lcd_write_command(0x00);
		// write data
		lcd_write_raw(lcd_state.framebuffer, sizeof(lcd_state.framebuffer));
		lcd_set_cursor(lcd_state.current_line, lcd_state.current_column);
	}
}

void lcd_scroll(void)
{
	int i, j;
	if(lcd_state.framebuffer != NULL) {
		for(i = 0; i < 8; i++)
			for(j = 0; j < 96; j++)
				lcd_state.framebuffer[i][j] = lcd_state.framebuffer[i+1][j];
		lcd_fb_show();
	}
}

void lcd_putc(int c)
{
	// let's process ASCII as fast as possible
	if(c < 127 && c >= 32) {
		lcd_fb_write_raw(&(lcd_chars6x8[c - 32][0]), 6);
	}
	else {
		// cyrillic is a bit slower
		if(c >= 0xC0 && c <= 0xFF)
			// cyrillic starts in font array at index 95
			lcd_fb_write_raw(&(lcd_chars6x8[c - 0xC0 + 95][0]), 6);
		else
			// handle control characters
			switch(c)
			{
			case '\n':
				// new line: new line, then do carriage return
				if(++lcd_state.current_line >= 8)
					// scroll or wrap, depending if we have a framebuffer and scrolling state
					if(lcd_state.framebuffer != NULL && (lcd_state.flags & LCD_FLAG_SCROLL)) {
						lcd_state.current_line = 7;
						lcd_scroll();
					}
			case '\r':
				// carriage return: move cursor to beginning of current line
				lcd_set_cursor(lcd_state.current_line, 0);
				return;
			case '\f':
				// form feed: clear screen
				lcd_clear();
				return;
			default:
				// non-printable character. skip it
				return;
			}
	}
	// advance cursor/scroll
	if((lcd_state.current_column += 6) >= 96) {
		// new line
		lcd_state.current_column = 0;
		if(++lcd_state.current_line >= 8) {
			// scroll or wrap, depending if we have a framebuffer and scrolling state
			if(lcd_state.framebuffer != NULL && (lcd_state.flags & LCD_FLAG_SCROLL)) {
				lcd_state.current_line = 7;
				lcd_scroll();
			}
			else
				lcd_set_cursor(0, 0);
		}
	}
}

void lcd_puts(const unsigned char *s)
{
	if(lcd_state.flags & LCD_FLAG_UTF8CYR) {
		// interprete UTF-8 cyrillic chars
		while(*s)
		{
			if(*s == 0xD0) {
				// А..Я,а..п: UD090..UD0BF
				s++;
				if(!*s) // unexpected end
					return;
				if(*s >= 0x90 && *s < 0xC0)
					lcd_putc(*s + (0xC0 - 0x90)); // 'A' in Win1251 -- 0xC0
				s++;
				continue;
			}
			if(*s == 0xD1) {
				// р..я:      UD180..UD18F
				s++;
				if(!*s) // unexpected end
					return;
				if(*s >= 0x80 && *s < 0x90)
					lcd_putc(*s + (0xF0 - 0x80)); // 'р' in Win1251 -- 0xF0
				s++;
				continue;
			}
			// ascii char
			lcd_putc(*s++);
		}
	}
	else
		// 1-byte encoding handling is trivial
		while(*s)
			lcd_putc(*s++);
}

void lcd_set_flags(uint16_t flags)
{
	lcd_state.flags = flags;
}


