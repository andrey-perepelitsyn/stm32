/*
 * lcd_nokia.h
 *
 *  Created on: Oct 31, 2015
 *      Author: Andrey Perepelitsyn
 *
 *  Lib for using monochrome Nokia cellphone LCD display
 *  tested on 1110i, but 1110 and 1202 should work without code changes
 *
 *  Features:
 *  fast hardware 9-bit SPI, terminal output with scrolling, 6x8 charcell,
 *  Windows-1251 or UTF-8 encoding for cyrillic chars,
 *  framebuffer for pixel graphics implementation
 *
 *  TODO:
 *  enhanced display support -- power management, orientation, contrast, etc
 *  big fonts, proportional fonts, graphics primitives?
 */

#ifndef LCD_NOKIA_H_
#define LCD_NOKIA_H_

#include <stm32f0xx_conf.h>
/*
 * choose from SPI1 and SPI2 ports
 */
#undef  LCD_USE_SPI2
//#define LCD_USE_SPI2

#ifdef USE_SPI2

#define LCD_PORT           GPIOB
#define LCD_SCK_PIN_NUM    ((uint8_t)13)
#define LCD_RESET_PIN_NUM  ((uint8_t)14)
#define LCD_MOSI_PIN_NUM   ((uint8_t)15)
#define LCD_SPI            SPI2
#define LCD_SPI_CLK        RCC_APB1Periph_SPI2
#define LCD_GPIO_CLK       RCC_AHBPeriph_GPIOB
#define LCD_SPI_IRQn       SPI2_IRQn
#define LCD_SPI_IRQHandler SPI2_IRQHandler

#else

#define LCD_PORT           GPIOA
#define LCD_SCK_PIN_NUM    ((uint8_t)5)
#define LCD_RESET_PIN_NUM  ((uint8_t)6)
#define LCD_MOSI_PIN_NUM   ((uint8_t)7)
#define LCD_SPI            SPI1
#define LCD_SPI_CLK        RCC_APB2Periph_SPI1
#define LCD_GPIO_CLK       RCC_AHBPeriph_GPIOA
#define LCD_SPI_IRQn       SPI1_IRQn
#define LCD_SPI_IRQHandler SPI1_IRQHandler

#define LCD_SCK_PIN        ((uint16_t)(1 << LCD_SCK_PIN_NUM))
#define LCD_RESET_PIN      ((uint16_t)(1 << LCD_RESET_PIN_NUM))
#define LCD_MOSI_PIN       ((uint16_t)(1 << LCD_MOSI_PIN_NUM))

#endif // USE_SPI2

#define LCD_FLAG_SCROLL    1       // scrolling is on
#define LCD_FLAG_UTF8CYR   2       // use UTF-8 for cyrillic strings, not Windows-1251

typedef void (*lcd_wait_func_t)(uint32_t);

/*
 * \brief Nokia 1100, 1110, 1110i, 1202... LCD framebuffer presentation
 */
typedef uint8_t lcd_framebuffer_t[9][96];

typedef struct {
	lcd_framebuffer_t *framebuffer;
	lcd_wait_func_t    wait_func;
	uint16_t           flags;          // scrolling, encoding, etc
	uint8_t            current_line;   // text line, [0..7] (8th is not used)
	uint8_t            current_column; // graphics column, not text! [0..95]
} lcd_state_t;

/*
 * \brief Initialize display
 * \return 0 if successful. At the time, there is no checks for success, so 0 returned always.
 *         TODO: Maybe sometime read mode will be implemented to obtain init status.
 * \param framebuffer pointer to 96*9 bytes memory area to store framebuffer.
 *        Using FB makes possible pixel operations, scrolling, etc.
 *        If NULL, LCD library will be started in dumb mode.
 * \param delay_func function to do delays in init process, takes number of milliseconds
 *        as an argument. If NULL, dumb loop delay will be used.
 */
uint16_t lcd_init(framebuffer_t *framebuffer, lcd_wait_func_t wait_func);

/*
 * dumb loop wait function, used in display init, if no user wait function provided
 * \param msec milliseconds to wait
 */
void lcd_dumb_wait(uint32_t msec);

/*
 * \brief Copy framebuffer (if exists) to display.
 */
void lcd_fb_show(void);

/*
 * \brief Set text position, where next character will be printed.
 *        Note, that horizontal position is set in pixels, not in characters!
 * \param line [0..7]
 * \param column [0..95]
 */
void lcd_set_cursor(uint8_t line, uint8_t column);

/*
 * \brief Clear display
 */
void lcd_clear(void);

/*
 * \brief Write raw data to display. Data is sent byte by byte, with bit 8 set to 1 (data)
 * \param data source buffer
 * \length size of data buffer
 */
void lcd_write_raw(const uint8_t *data, int length);

/*
 * \brief Write data to display and duplicate to framebuffer, if exists
 * \param data source buffer
 * \length size of data buffer
 */
void lcd_fb_write_raw(const uint8_t *data, int length);

/*
 * \brief Write command byte to display
 * \param command byte
 */
void lcd_write_command(uint8_t command);

/*
 * \brief Scroll display 8 pixels up
 */
void lcd_scroll(void);

/*
 * \brief Print single char and advance cursor.
 *        Windows-1251 cyrillic letters are supported.
 *        Scroll display after printing to bottom right most position.
 *        Cyrillic UTF-8 chars are supported.
 * \param c ascii code
 */
void lcd_putc(int c);

/*
 * \brief Print string of chars. UTF-8 cyrillic letters are supported.
 * \param s zero-terminated string buffer
 */
void lcd_puts(const unsigned char *s);

/*
 * \brief Set flags: strings encoding, screen scrolling, etc
 * \param flags See lcd_state_t definition for details.
 * \sa lcd_state_t
 */
void lcd_set_flags(uint16_t flags);

#endif /* LCD_NOKIA_H_ */




