#pragma once
#include <stdint.h>
#define BLOCK_SIZE 8

typedef struct {
    double coeff[BLOCK_SIZE][BLOCK_SIZE];
} Block;

typedef struct JPEGImage {
    int width;
    int height;
    int cbxblocks;   
    int cbyblocks;
    int xblocks;
    int yblocks;
    Block **Yblocks;
    Block **Cbblocks;
    Block **Crblocks;
} JPEGImage;

typedef struct {
    int prev_dcY;   
    int prev_dcCb;  
    int prev_dcCr; 
} DCContext;

typedef struct {
    uint8_t *data;      
    int size;        
    int capacity;    
    uint32_t buffer;    
    int bitscount; 
} BitStream;