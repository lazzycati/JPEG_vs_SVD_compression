#pragma once
#include "jpegtypes.h"
#include "dpcm.h"
#include "huffman.h"
#include "quantization.h"
#include "subdiscretization.h"
#include "jpeg.h"
#include "dct.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M_SOI  0xD8
#define M_EOI  0xD9
#define M_APP0 0xE0
#define M_DQT  0xDB
#define M_SOF0 0xC0
#define M_DHT  0xC4
#define M_SOS  0xDA

typedef struct {
    int width, height;
    int channels;
    int indch[4];
    int sampling[4];
    int qt_ind[4];
    int qt_tables[4][8][8];
    int has_qt[4];
    HuffmanTable DC_tables[4];
    HuffmanTable AC_tables[4];
    int has_DCtable[4];
    int has_ACtable[4];
    BitStream *stream;
    DCContext dcctx; 
    int current_dc[3];
} JPEGDecoder;

typedef struct {
    uint8_t *data;
    int size;
} JPEGFileData;

JPEGDecoder* create_jpeg_decoder();
void destroy_jpeg_decoder(JPEGDecoder *decoder);
JPEGFileData* jpeg_readfile(char *filename);
void free_datajpeg(JPEGFileData *file_data);
int jpeg_parse_headers(JPEGDecoder *decoder, JPEGFileData *file_data);
YCbCrImage* jpeg_decode_image(JPEGDecoder *decoder, char *filename, int quality);
int jpeg_decode_file(char *inputfile, char *outputfile, int quality);