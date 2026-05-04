#include "huffman.h"

BitStream* create_bitstream() 
{
    BitStream *bs = (BitStream*)malloc(sizeof(BitStream));
    if (!bs) return NULL;
    bs->capacity = 1024;
    bs->data = (uint8_t*)malloc(bs->capacity);
    if (!bs->data) 
    {
        free(bs);
        return NULL;
    }
    bs->size = 0;
    bs->buffer = 0;
    bs->bitscount = 0;
    return bs;
}

void write_bits(BitStream *bs, uint32_t bits, int nums) 
{
    if (nums == 0 || !bs) return;
    bs->buffer = (bs->buffer << nums) | bits;
    bs->bitscount += nums;
    while (bs->bitscount >= 8) 
    {
        bs->bitscount -= 8;
        uint8_t byte = (bs->buffer >> bs->bitscount) & 0xFF;
        if (byte == 0xFF) 
        {
            // В JPEG после 0xFF нужно вставить 0x00
            if (bs->size + 2 >= bs->capacity) 
            {
                bs->capacity *= 2;
                bs->data = (uint8_t*)realloc(bs->data, bs->capacity);
            }
            bs->data[bs->size++] = 0xFF;
            bs->data[bs->size++] = 0x00;
        } 
        else 
        {
            if (bs->size + 1 >= bs->capacity) 
            {
                bs->capacity *= 2;
                bs->data = (uint8_t*)realloc(bs->data, bs->capacity);
            }
            bs->data[bs->size++] = byte;
        }
    }
}

void flush_bits(BitStream *bs) 
{
    if (bs->bitscount > 0) 
    {
        bs->buffer <<= (8 - bs->bitscount);
        uint8_t byte = bs->buffer & 0xFF;
        if (byte == 0xFF) 
        {
            bs->data[bs->size++] = 0xFF;
            bs->data[bs->size++] = 0x00;
        } 
        else bs->data[bs->size++] = byte;
        bs->bitscount = 0;
        bs->buffer = 0;
    }
}

void free_bitstream(BitStream *bs) 
{
    if (bs) 
    {
        if (bs->data) free(bs->data);
        free(bs);
    }
}

void huffman_encodeDC(BitStream *bs, int diff, int is_lum) 
{
    int bytes = getbits(diff);
    const HuffmanCode *table = is_lum ? dc_lumtable : dc_chromtable;
    HuffmanCode huff = table[bytes];
    write_bits(bs, huff.code, huff.length);
    if (bytes != 0) 
    {
        int encval = encode_diff(diff, bytes);
        write_bits(bs, encval, bytes);
    }
}

uint16_t AChuffman_code(int rle_symbol, int is_lum) 
{
    if (rle_symbol < 0 || rle_symbol > 255) return 0;
    if (is_lum) return AC_lumtable[rle_symbol].code;
    else return AC_chromtable[rle_symbol].code;
}

int AChuffman_length(int rle_symbol, int is_lum) 
{
    if (rle_symbol < 0 || rle_symbol > 255) return 0;
    if (is_lum) return AC_lumtable[rle_symbol].length;
    else return AC_chromtable[rle_symbol].length;
}
