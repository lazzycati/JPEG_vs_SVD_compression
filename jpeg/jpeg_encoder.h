#pragma once
#include "jpegtypes.h"
#include "dpcm.h"
#include "huffman.h"
#include "quantization.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// JPEG маркеры
#define SOI 0xFFD8  // Start of Image (начало изображения)
#define EOI 0xFFD9   // End of Image (конец изображения)
#define APP0 0xFFE0  // Application segment (JFIF заголовок)
#define DQT 0xFFDB   // Define Quantization Table
#define SOF0 0xFFC0   // Start of Frame (информация об изображении)
#define DHT 0xFFC4    // Define Huffman Table
#define SOS 0xFFDA    // Start of Scan (начало данных)

typedef struct {
    BitStream *stream;
    DCContext dcctx;
} JPEGEncoder;

JPEGEncoder* create_jpeg_encoder();
void destroy_jpeg_encoder(JPEGEncoder *encoder);
void jpeg_encode_image(JPEGEncoder *encoder, JPEGImage *image, int quality);
int jpeg_write_file(JPEGEncoder *encoder, JPEGImage *image, char *filename, int quality);