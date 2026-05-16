#pragma once 
#include "jpegtypes.h"
extern const int luminance_qt[8][8];
extern const int chrominance_qt[8][8];

void scale_quantization_table(int out[8][8], const int in[8][8], int quality);
void quantize_jpeg(JPEGImage *jpeg, int quality);
void dequantize_jpeg(JPEGImage *jpeg, int quality);
