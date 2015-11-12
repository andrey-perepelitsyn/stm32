/*
 * fontconv.c
 *
 *  Created on: Nov 10, 2015
 *      Author: Andrey Perepelitsyn
 *
 *  tool for converting BDF font format produced by "Fony" to lcd_display font
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "bitstream.h"
#include "microfont.h"

typedef struct {
	int index, height, width;
	char bits[MF_MAX_FONT_H][MF_MAX_FONT_W];
} char_data_t;

int bdf_read_char(FILE *f, char_data_t *c)
{
	int i, h, w, j, k, code;
	char buf[20];
	uint32_t line;

	while((code = fscanf(f, "STARTCHAR %d", &i)) != 1)
	{
		if(code == EOF)
			return -1;
		fgetc(f);
	}
	while((code = fscanf(f, "BBX %d %d %d %d", &w, &h, &j, &k)) != 4)
	{
		if(code == EOF)
			return -1;
		fgetc(f);
	}
	do {
		while((code = fscanf(f, "%19s", buf)) != 1)
			if(code == EOF)
				return -1;
	} while(strcmp(buf, "BITMAP") != 0);
	if(w > MF_MAX_FONT_W || h > MF_MAX_FONT_H)
		return -1;
	c->height = h;
	c->width = w;
	c->index = i;
	if(h && w)
		for(i = 0; i < h; i++)
		{
			fscanf(f, "%8s", buf);
			for(j = 0; j < strlen(buf); j++)
			{
				int quad = buf[j];
				if(quad >= 'A')
					quad = quad - 'A' + 10;
				else
					quad -= '0';
				for(k = 0; k < 4; k++)
					c->bits[i][j*4 + k] = (quad >> (3 - k)) & 1;
			}
		}

	return 0;
}


int make_microfont(const char_data_t *chars, microfont_t *font)
{
	int i, j, n;
	bitstream_t bs;
	
	font->magic = 0xface;
	font->height = 1;
	font->widthbits = 1;
	for(i = 0; i < MF_FONT_CHARS; i++)
	{
		if(chars[i].index == 0 || chars[i].width == 0 || chars[i].height == 0)
			continue;
//		fprintf(stderr, "i: %02x, w: %d, h: %d\n", i, chars[i].width, chars[i].height);
		if(chars[i].height > font->height)
			font->height = chars[i].height;
		j = 0;
		for(n = chars[i].width - 1; n != 0; n >>= 1)
			j++;
		if(j > font->widthbits)
			font->widthbits = j;
	}
	fprintf(stderr, "widthbits: %d, height: %d\n", font->widthbits, font->height);
	
	bs_init(&bs, font->bits);
	for(i = 0; i < MF_FONT_CHARS; i++)
	{
		if(chars[i].index == 0 || chars[i].width == 0 || chars[i].height == 0) {
			font->index[i] = MF_EMPTY_CHAR;
			continue;
		}
		if(bs_tell(&bs) >= MF_EMPTY_CHAR) {
			fprintf(stderr, "char %02x won't fit!\n", i + MF_FIRST_CHAR);
			return -1;
		}
		font->index[i] = bs_tell(&bs);
		bs_write_bits(&bs, chars[i].width, font->widthbits);
		fprintf(stderr, "%02x '%c', w:%2d, offset: %d\n",
			i + MF_FIRST_CHAR, i + MF_FIRST_CHAR, chars[i].width, bs_tell(&bs));
		
		// TODO: let choose bitstream direction, now only "up-down" implemented
		for(j = 0; j < chars[i].width; j++)
			for(n = 0; n < font->height; n++)
				bs_write_bit(&bs, chars[i].bits[n][j]);
	}

	return sizeof(microfont_t) + (((bs_tell(&bs) + 31) >> 5) << 2);
}

void die(const char *reason, const char *arg)
{
	fprintf(stderr, reason, arg);
	fputs(": ", stderr);
	perror(NULL);
	exit(1);
}

int main(int argc, char *argv[])
{
	int i, j, k, c, fontSize = 0, mfBytes;
	FILE *src, *dst;
	char *srcFileName = NULL, *dstFileName = NULL;
	char_data_t char_data[MF_FONT_CHARS];
	microfont_t *mf;

	opterr = 0;

	while((c = getopt(argc, argv, "hs:i:o:")) != -1)
		switch(c)
		{
		case 'i':
			if(strcmp(optarg, "-")) {
				srcFileName = optarg;
				src = fopen(optarg, "rb");
				if(src == NULL)
					die("unable to open source file '%s'", optarg);
			}
			break;
		case 'o':
			if(strcmp(optarg, "-")) {
				dstFileName = optarg;
				dst = fopen(optarg, "wb+");
				if(dst == NULL)
					die("unable to open destination file '%s'", optarg);
			}
			break;
		case 'h':
			fputs(	"Tool for converting BDF font format to lcd_display font\n"
					"\nUsage:\n\tfontconv -s <size> [-i <source>] [-o <destination>]\n\n", stderr);
			return 1;
		case 's':
			fontSize = atoi(optarg);
			break;
		case '?':
			if(optopt == 's')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else
				fprintf(stderr, "Unknown option -%c.", optopt);
			return 1;
		default:
			abort();
		}

	fprintf(stderr, "source: '%s', destination: '%s'\n", srcFileName, dstFileName);
//	if(fontSize == 0) {
//		fputs("font size is not defined!\n", stderr);
//		return 1;
//	}
	if(srcFileName == NULL)
		src = stdin;
	if(dstFileName == NULL)
		dst = stdout;

	memset(char_data, 0, sizeof(char_data));
	while(!feof(src))
	{
		char_data_t ch;
		if(bdf_read_char(src, &ch))
			break;
		if(ch.index < 32 || ch.index > 255) {
			fprintf(stderr, "char #%02x out of range, skipping.\n", ch.index);
			continue;
		}
//		fprintf(stderr, "char #%02x read ok.\n", ch.index);
		char_data[ch.index - 32] = ch;
	}

/*	//visual test:)
 	for(i = 0; i < MF_FONT_CHARS; i++)
	{
		if(char_data[i].index) {
			fprintf(stderr, "i: 0x%02x '%c', h: %d, w: %d\n",
					char_data[i].index, char_data[i].index, char_data[i].height, char_data[i].width);
			for(j = 0; j < char_data[i].height; j++)
			{
				for(k = 0; k < char_data[i].width; k++)
					fputc(char_data[i].bits[j][k] ? '#' : '.', stderr);
				fputc('\n', stderr);
			}
		}
	}
 */	
	
	mf = (microfont_t *)malloc(sizeof(microfont_t) + MF_MAX_FONT_DATA);
	mfBytes = make_microfont(char_data, mf);
	fprintf(stderr, "microfont size: %d\n", mfBytes);
	
	if(fwrite(mf, mfBytes, 1, dst) != 1) {
		perror("unable to write to destination");
		return 1;
	}

	return 0;
}
