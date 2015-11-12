#include <stdio.h>
#include <stdint.h>
#include "bitstream.h"

int main(int argc, char *argv[])
{
	bitstream_t bs;
	uint32_t data[40];
	uint32_t i, j;
	
	for(i = 0; i < 40; i++)
		data[i] = 0;
	bs_init(&bs, data);
	for(i = 1; i < 32; i++)
	{
		bs_write_bits(&bs, (1 << i) - 1, i);
		bs_write_bits(&bs, 0, i);
	}
	for(i = 0; i < 40; i++)
		for(j = 0; j < 32; j++)
		{
			putchar(data[i] & 1 ? '#' : '.');
			data[i] >>= 1;
		}
	return 0;
}
