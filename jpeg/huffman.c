#include "huffman.h"

HuffmanTable DC_lum;
HuffmanTable DC_chrom;
HuffmanTable AC_lum;
HuffmanTable AC_chrom;

void init_tables() 
{
    uint8_t DC_luma_bits[17] = {0, 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0};
    uint8_t DC_luma_vals[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    memcpy(DC_lum.bits, DC_luma_bits, sizeof(DC_luma_bits));
    memcpy(DC_lum.huffval, DC_luma_vals, sizeof(DC_luma_vals));
    DC_lum.lastk = 12;
    generate_codes(&DC_lum);
    uint8_t DC_chroma_bits[17] = {0, 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
    uint8_t DC_chroma_vals[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    memcpy(DC_chrom.bits, DC_chroma_bits, sizeof(DC_chroma_bits));
    memcpy(DC_chrom.huffval, DC_chroma_vals, sizeof(DC_chroma_vals));
    DC_chrom.lastk = 12;
    generate_codes(&DC_chrom);
    uint8_t AC_luma_bits[17] = {0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125};
    uint8_t AC_luma_vals[] = {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
        0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
        0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
        0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
        0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
        0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
        0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
        0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
        0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
        0xF9, 0xFA
    };
    memcpy(AC_lum.bits, AC_luma_bits, sizeof(AC_luma_bits));
    memcpy(AC_lum.huffval, AC_luma_vals, sizeof(AC_luma_vals));
    AC_lum.lastk = 162;
    generate_codes(&AC_lum);
    uint8_t AC_chroma_bits[17] = {0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119};
    uint8_t AC_chroma_vals[] = {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
        0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34,
        0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
        0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
        0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
        0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
        0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
        0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
        0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
        0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
        0xF9, 0xFA
    };
    memcpy(AC_chrom.bits, AC_chroma_bits, sizeof(AC_chroma_bits));
    memcpy(AC_chrom.huffval, AC_chroma_vals, sizeof(AC_chroma_vals));
    AC_chrom.lastk = 162;
    generate_codes(&AC_chrom);
}

void generate_codes(HuffmanTable *table) 
{
    int code = 0;
    int si = 0;
    for (int i = 1; i <= 16; i++) 
    {
        for (int j = 0; j < table->bits[i]; j++) 
        {
            table->huffcode[si] = code;  
            table->huffsize[si] = i;     
            si++;
            code++;  
        }
        code <<= 1; 
    }
}

void add_bit(BitStream *stream, int bit) 
{
    stream->buffer = (stream->buffer << 1) | (bit & 1);
    stream->bitscount++;
    if (stream->bitscount == 8) 
    {
        if (stream->size + 1 > stream->capacity) 
        {
            stream->capacity *= 2;
            stream->data = (uint8_t*)realloc(stream->data, stream->capacity);
        }
        stream->data[stream->size++] = (uint8_t)(stream->buffer & 0xFF);
        // Если записан байт 0xFF (маркер JPEG), следующий байт должен быть 0x00, чтобы отличить данные от маркеров.
        if (stream->data[stream->size - 1] == 0xFF) 
        {
            if (stream->size + 1 > stream->capacity) 
            {
                stream->capacity *= 2;
                stream->data = (uint8_t*)realloc(stream->data, stream->capacity);
            }
            stream->data[stream->size++] = 0x00;
        }
        stream->buffer = 0;
        stream->bitscount = 0;
    }
}

void write_bits(BitStream *stream, int val, int numbits) 
{
    for (int i = numbits - 1; i >= 0; i--) add_bit(stream, (val >> i) & 1);
}

void flush(BitStream *stream) 
{
    if (stream->bitscount > 0) 
    {
        while (stream->bitscount < 8) 
        {
            stream->buffer = (stream->buffer << 1) | 1;
            stream->bitscount++;
        }
        if (stream->size + 1 > stream->capacity) 
        {
            stream->capacity *= 2;
            stream->data = (uint8_t*)realloc(stream->data, stream->capacity);
        }
        stream->data[stream->size++] = (uint8_t)(stream->buffer & 0xFF);
        if (stream->data[stream->size - 1] == 0xFF) 
        {
            if (stream->size + 1 > stream->capacity) 
            {
                stream->capacity *= 2;
                stream->data = (uint8_t*)realloc(stream->data, stream->capacity);
            }
            stream->data[stream->size++] = 0x00;
        }
        stream->buffer = 0;
        stream->bitscount = 0;
    }
}

int read_bit(BitStream *stream) 
{
    if (stream->bitscount == 0) 
    {
        if (stream->size == 0) return -1;
        uint8_t byte = stream->data[0];
        stream->data++;
        stream->size--;
        if (byte == 0xFF && stream->size > 0 && stream->data[0] == 0x00) 
        {
            stream->data++;
            stream->size--;
        }
        stream->buffer = byte;
        stream->bitscount = 8;
    }
    int bit = (stream->buffer >> 7) & 1;
    stream->buffer <<= 1;
    stream->bitscount--;
    return bit;
}

static int read_bits(BitStream *stream, int numbits) 
{
    int val = 0;
    for (int i = 0; i < numbits; i++) 
    {
        int bit = read_bit(stream);
        if (bit < 0) return -1;
        val = (val << 1) | bit;
    }
    return val;
}

int huffman_encode_simb(BitStream *stream, int s, HuffmanTable *table) 
{
    // Ищем код в таблице:
    int ind = -1;
    for (int i = 0; i < table->lastk; i++) 
    {
        if (table->huffval[i] == s) 
        {
            ind = i;
            break;
        }
    }
    if (ind == -1) return -1;
    uint16_t code = table->huffcode[ind];
    int size = table->huffsize[ind];
    //Записываем код в поток от старшего бита к младшему
    for (int i = size - 1; i >= 0; i--) add_bit(stream, (code >> i) & 1);
    return size;
}

int huffman_decode_simb(BitStream *stream, HuffmanTable *table, int *s) 
{
    uint16_t code = 0;
    for (int i = 1; i <= 16; i++) 
    {
        //Читаем бит с потока
        int bit = read_bit(stream);
        if (bit < 0) return -1;
        code = (code << 1) | bit;
        //проверяем совпад. в таблице кодов:
        for (int j = 0; j < table->lastk; j++) 
        {
            if (table->huffsize[j] == i && table->huffcode[j] == code) 
            {
                *s = table->huffval[j];
                return 0;
            }
        }
    }
    return -1; 
}