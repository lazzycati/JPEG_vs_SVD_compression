#pragma once 
#include <stdint.h>
#include "loadbmp.h"
typedef struct {
    int width;
    int height;
    uint8_t *Y;      // полное разрешение, размер width × height
    uint8_t *Cb;     // субдискретизированный, размер (width + 1)/2 × (height + 1)/2
    uint8_t *Cr;     // субдискретизированный
} YCbCrImage420;

YCbCrImage420* convert_to_sub420(YCbCrImage *img);
YCbCrImage* convert_from_sub420(YCbCrImage420 *img420);
void free_ycbcr420(YCbCrImage420 *img);