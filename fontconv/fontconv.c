/*
 * fontconv.c
 *
 *  Created on: Nov 10, 2015
 *      Author: Andrey Perepelitsyn
 *
 *  tool converting *.bin font format produced by "Fony" to lcd_display font
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FONT_SIZE 20

void die(const char *reason, const char *arg)
{
	fprintf(stderr, reason, arg);
	fputs(": ", stderr);
	perror(NULL);
	exit(1);
}

int main(int argc, char *argv[])
{
	int c, fontSize = 0;
	FILE *src, *dst;
	char *srcFileName = NULL, *dstFileName = NULL;

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
			fputs("\nUsage:\n\tfontconv -s <size> [-i <source>] [-o <destination>]\n\n", stderr);
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
	if(fontSize == 0) {
		fputs("font size is not defined!\n", stderr);
		return 1;
	}
	if(srcFileName == NULL)
		src = stdin;
	if(dstFileName == NULL)
		dst = stdout;

	c = ' ';
	while(!feof(src))
	{
		char bitmap[MAX_FONT_SIZE];
		int i, j;
		if(fread(bitmap, fontSize, 1, src) != 1)
			break;
		fprintf(dst, "# '%c' 0x%02X\n", c, c);
		for(i = 0; i < fontSize; i++)
		{
			for(j = 7; j >= 0; j--)
				fputc(((bitmap[i] >> j) & 1) + '0', dst);
			fputs("b\n", dst);
		}
		c++;
	}

	return 0;
}
