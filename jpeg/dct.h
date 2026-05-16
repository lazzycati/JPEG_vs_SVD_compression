#pragma once
#include "jpegtypes.h"
#include "subdiscretization.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Block** split_into_blocks(uint8_t *channel, int width, int height, int *numxblocks, int *numyblocks);
// Основная функция подготовки к DCT
JPEGImage* prep_for_dct(YCbCrImage420 *img);
void DCT(JPEGImage *jpeg);
void inverse_DCT(JPEGImage *jpeg);
void dct8(double blockin[8][8], double blockout[8][8]);
void idct8(double blockin[8][8], double blockout[8][8]);
//Восстановление изображения из блоков
YCbCrImage420* restore_from_blocks(JPEGImage *jpeg);
void free_jpeg(JPEGImage *jpeg);
