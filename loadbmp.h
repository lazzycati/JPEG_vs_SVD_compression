#pragma once

#include <stdint.h>

typedef struct {
    int width;
    int height;
    uint8_t *data;  
} Image;

Image* load_bmp(char *filename);
int store_to_bmp(char *filename, Image *img);
void free_image(Image *img);
