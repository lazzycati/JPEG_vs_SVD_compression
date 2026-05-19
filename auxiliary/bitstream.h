#pragma once
#include "jpegtypes.h"
#include <stdint.h>
#include <stdlib.h>
//добавляет бит во временный буфер и по необходимости вып. flush, учитывает байт-стаффинг
void add_bit(BitStream *stream, int bit);
void write_bits(BitStream *stream, int value, int num_bits);
//Дописывает недостающие биты (единицами) до полного байта и записывает последний байт.
void flush(BitStream *stream);
//Читает бит с потока
int read_bit(BitStream *stream);
int read_bits(BitStream *stream, int numbits);