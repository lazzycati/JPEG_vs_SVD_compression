#include "bitstream.h"

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

int read_bits(BitStream *stream, int numbits) 
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
