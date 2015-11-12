#ifndef _MICROFONT_H
#define _MICROFONT_H

#define <stdint.h>

/*
 *	Формат для хранения шрифта:
 *	* Ограничения
 *	Первый знак -- всегда 0x20 (пробел)
 *	Последний знак -- всегда 0xFF ("я")
 *  Кодировка -- всегда Windows-1251
 *	Ширина -- до 32 пикселов
 *	Высота -- до 64 пикселов
 *	Размер данных -- до 8кбайт (64 кбит, адресуются uint32_t)
 */

#define MAX_FONT_W 32
#define MAX_FONT_H 64
#define FONT_CHARS 224

typedef struct {
	uint16_t magic;        // 0xface
	uint8_t  height;       // высота шрифта
	uint8_t  widthbits;    // количество бит, отведенных под ширину
} microfont_t;

#endif // _MICROFONT_H
