#ifndef _MICROFONT_H
#define _MICROFONT_H

#include <stdint.h>

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

#define MF_MAX_FONT_W    32
#define MF_MAX_FONT_H    64
#define MF_FIRST_CHAR    32
#define MF_LAST_CHAR     255
#define MF_FONT_CHARS    (MF_LAST_CHAR - MF_FIRST_CHAR + 1)
#define MF_MAX_FONT_DATA 8192
#define MF_EMPTY_CHAR    65535

typedef struct {
	uint16_t magic;                // 0xface
	uint8_t  height;               // высота шрифта
	uint8_t  widthbits;            // количество бит, отведенных под ширину
	uint16_t index[MF_FONT_CHARS]; // массив смещений в битах
	uint32_t bits[];               // битовый массив
} microfont_t;

#endif // _MICROFONT_H
