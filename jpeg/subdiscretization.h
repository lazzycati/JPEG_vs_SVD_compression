#include <stdint.h>
#include "loadbmp.h"
#pragma once 
//глаз видит яркость в 3D (X, Y, T), но цвет воспринимает "размытым" в 3-4 раза.
//4:2:0 — цветность в 2 раза меньше по горизонтали и вертикали (наиболее распространен)
typedef struct {
    int width;
    int height;
    uint8_t *Y;      // полное разрешение, размер width × height
    uint8_t *Cb;     // субдискретизированный, размер (width + 1)/2 × (height + 1)/2
    uint8_t *Cr;     // субдискретизированный
} YCbCrImage420;

YCbCrImage420* convert_to_sub420(YCbCrImage *img);
void free_ycbcr420(YCbCrImage420 *img);