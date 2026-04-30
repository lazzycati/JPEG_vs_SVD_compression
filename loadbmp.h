#pragma once

#include <stdint.h>

typedef struct {
    int width;
    int height;
    uint8_t *data;  
} Image;

typedef struct {
    int width;
    int height;
    uint8_t *Y;      // Luminance
    uint8_t *Cb;     // Chroma Blue
    uint8_t *Cr;     // Chroma Red
} YCbCrImage;

Image* load_bmp(char *filename);
int store_to_bmp(char *filename, Image *img);
void free_image(Image *img);
YCbCrImage* load_bmp_to_ycbcr(char *filename);
int store_ycbcr_to_bmp(char *filename, YCbCrImage *img);
void free_ycbcr_image(YCbCrImage *img);
