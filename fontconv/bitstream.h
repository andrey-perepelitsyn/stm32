#ifndef _BITSTREAM_H
#define _BITSTREAM_H

#include <stdint.h>

typedef struct {
	uint32_t *data;
	uint32_t  cur_word;
	uint32_t  mask;
	int       cur_bit;
} bitstream_t;

void bs_seek(bitstream_t *bs, uint32_t offset)
{
	bs->cur_word = offset >> 5;
	bs->cur_bit = offset & 0x1F;
	bs->mask = 1 << bs->cur_bit;
}

uint32_t bs_tell(bitstream_t *bs)
{
	return (bs->cur_word << 5) + bs->cur_bit;
}

void bs_init(bitstream_t *bs, uint32_t *data)
{
	bs->data = data;
	bs_seek(bs, 0);
}

void bs_write_bit(bitstream_t *bs, int bit)
{
	bs->data[bs->cur_word] &= ~bs->mask;
	if(bit)
		bs->data[bs->cur_word] |= bs->mask;
	bs->cur_bit++;
	bs->mask <<= 1;
	if(bs->mask == 0) {
		bs->cur_bit = 0;
		bs->mask = 1;
		bs->cur_word++;
	}
}

void bs_write_bits(bitstream_t *bs, uint32_t chunk, int size)
{
	int size2 = bs->cur_bit + size - 32;
	int size1 = 32 - bs->cur_bit;
	
	if(size2 > 0) {
		// write to next word first
		uint32_t mask2 = (1 << size2) - 1;
		uint32_t chunk2 = (chunk >> size1) & mask2;
		bs->data[bs->cur_word + 1] &= ~mask2;
		bs->data[bs->cur_word + 1] |= chunk2;
	}
	else // fix first chunk size
		size1 = size;
	// write to current word
	uint32_t mask = ((1 << size1) - 1) << bs->cur_bit;
	chunk <<= bs->cur_bit;
	chunk &= mask;
	bs->data[bs->cur_word] &= ~mask;
	bs->data[bs->cur_word] |= chunk;
	
	bs->cur_bit += size;
	if(bs->cur_bit >= 32) {
		bs->cur_bit &= 0x1F;
		bs->cur_word++;
	}
	bs->mask = 1 << bs->cur_bit;
}

#endif // _BITSTREAM_H
