#pragma once
#include "loadbmp.h"
#define MAX_PIXEL 255.0
#define K1 0.01
#define K2 0.03
#define L 255.0
#define C1 (K1 * K1 * L * L)  
#define C2 (K2 * K2 * L * L)  
#define C3 (C2 / 2.0)        
#define WSIZE 11

double PSNR(Image *img1, Image *img2);
double SSIM(Image *img1, Image *img2);