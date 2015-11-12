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
	char bits[MAX_FONT_H][MAX_FONT_W];
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
	if(w > MAX_FONT_W || h > MAX_FONT_H)
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

void die(const char *reason, const char *arg)
{
	fprintf(stderr, reason, arg);
	fputs(": ", stderr);
	perror(NULL);
	exit(1);
}

int main(int argc, char *argv[])
{
	int i, j, k, c, fontSize = 0;
	FILE *src, *dst;
	char *srcFileName = NULL, *dstFileName = NULL;
	char_data_t char_data[FONT_CHARS];

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
				dst = fopen(optarg, "w+");
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
		fprintf(stderr, "char #%02x read ok.\n", ch.index);
		char_data[ch.index - 32] = ch;
	}

	for(i = 0; i < FONT_CHARS; i++)
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
	
	return 0;
}
