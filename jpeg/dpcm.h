#pragma once 
#include "jpegtypes.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

extern const int zigzag[64];

void zigzag_to_block(Block *block, int *out);
void inverse_zigzag(int *in, Block *block);
void dpcm_encode_block(int *block, int *prev_dc);
void dpcm_decode_block(int *block, int *prev_dc);
//определяем, сколько бит нужно для хранения значения
int computeCategory(int val);
//Возвращает разницу между DC-компонентами, с учетом знака
int extrdiff(int val, int cat);
