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

void PSNR_channels(YCbCrImage *img1, YCbCrImage *img2, double psnr[3]);
// PSNR среднее по всем каналам
double PSNR(YCbCrImage *img1, YCbCrImage *img2);
//Для каждого канала отдельно
void SSIM_channels(YCbCrImage *img1, YCbCrImage *img2, double ssim[3]);
// SSIM среднее по всем каналам
double SSIM(YCbCrImage *img1, YCbCrImage *img2);