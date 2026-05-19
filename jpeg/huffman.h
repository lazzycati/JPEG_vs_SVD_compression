#pragma once
#include <stdint.h>
#include "jpegtypes.h"
#include "bitstream.h"
typedef struct {
    uint8_t bits[17]; // Количество кодов каждой длины 
    uint8_t huffval[256];   // Значения кодов в порядке увеличения длины кода
    uint16_t huffcode[256];   // Сгенерированные коды Хаффмана
    int huffsize[256];          // Длины кодов 
    int lastk;                  // Количество кодов в таблице
} HuffmanTable;

extern HuffmanTable DC_lum;
extern HuffmanTable DC_chrom;
extern HuffmanTable AC_lum;
extern HuffmanTable AC_chrom;

void init_tables();
void generate_codes(HuffmanTable *table);
int huffman_encode_simb(BitStream *stream, int s, HuffmanTable *table);
int huffman_decode_simb(BitStream *stream, HuffmanTable *table, int *s);
