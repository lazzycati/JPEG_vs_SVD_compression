#include <math.h>
#include "metrics.h"
#include <stdio.h>

static double psnr_channel(uint8_t *channel1, uint8_t *channel2, int size)
{
    double mse = 0.0;
    for (int i = 0; i < size; i++) 
    {
        int diff = (int)channel1[i] - (int)channel2[i];
        mse += (double)(diff * diff);
    }
    mse /= size;
    if (mse == 0.0) return INFINITY;
    return 10.0 * log10((MAX_PIXEL * MAX_PIXEL) / mse);
}

void PSNR_channels(YCbCrImage *img1, YCbCrImage *img2, double psnr[3])
{
    if (!img1 || !img2) 
    {
        psnr[0] = psnr[1] = psnr[2] = -1.0;
        return;
    }
    int size = img1->width * img1->height;
    psnr[0] = psnr_channel(img1->Y, img2->Y, size);
    psnr[1] = psnr_channel(img1->Cb, img2->Cb, size);
    psnr[2] = psnr_channel(img1->Cr, img2->Cr, size);
}

double PSNR(YCbCrImage *img1, YCbCrImage *img2)
{
    double psnr[3];
    PSNR_channels(img1, img2, psnr);
    return (psnr[0] + psnr[1] + psnr[2]) / 3.0;
}

static double ssim_compare_channels(uint8_t *channel1, uint8_t *channel2, int width, int height)
{
    double ssim_sum = 0.0;
    int count = 0;
    for (int y = 0; y < height; y++) 
    {
        for (int x = 0; x < width; x++) 
        {
            int mid = WSIZE / 2;
            int x1 = (x - mid < 0) ? 0 : x - mid;
            int x2 = (x + mid >= width) ? width - 1 : x + mid;
            int y1 = (y - mid < 0) ? 0 : y - mid;
            int y2 = (y + mid >= height) ? height - 1 : y + mid;
            int wind_w = x2 - x1 + 1;
            int wind_h = y2 - y1 + 1;
            int n = wind_w * wind_h;
            double sum1 = 0.0, sum2 = 0.0;
            for (int i = y1; i <= y2; i++) 
            {
                for (int j = x1; j <= x2; j++) 
                {
                    sum1 += channel1[i * width + j];
                    sum2 += channel2[i * width + j];
                }
            }
            double mu1 = sum1 / n;
            double mu2 = sum2 / n;
            double var1 = 0.0, var2 = 0.0, cov12 = 0.0;
            for (int i = y1; i <= y2; i++) 
            {
                for (int j = x1; j <= x2; j++) 
                {
                    double d1 = channel1[i * width + j] - mu1;
                    double d2 = channel2[i * width + j] - mu2;
                    var1 += d1 * d1;
                    var2 += d2 * d2;
                    cov12 += d1 * d2;
                }
            }
            var1 /= n;
            var2 /= n;
            cov12 /= n;
            double sigma1 = sqrt(var1);
            double sigma2 = sqrt(var2);
            double luminance = (2.0 * mu1 * mu2 + C1) / (mu1 * mu1 + mu2 * mu2 + C1);
            double contrast = (2.0 * sigma1 * sigma2 + C2) / (var1 + var2 + C2);
            double structure = (cov12 + C3) / (sigma1 * sigma2 + C3);
            ssim_sum += luminance * contrast * structure;
            count++;
        }
    }
    return ssim_sum / count;
}

void SSIM_channels(YCbCrImage *img1, YCbCrImage *img2, double ssim[3])
{
    if (!img1 || !img2) 
    {
        ssim[0] = ssim[1] = ssim[2] = -1.0;
        return;
    }
    int width = img1->width;
    int height = img1->height;
    ssim[0] = ssim_compare_channels(img1->Y, img2->Y, width, height);
    ssim[1] = ssim_compare_channels(img1->Cb, img2->Cb, width, height);
    ssim[2] = ssim_compare_channels(img1->Cr, img2->Cr, width, height);
}

double SSIM(YCbCrImage *img1, YCbCrImage *img2)
{
    double ssim[3];
    SSIM_channels(img1, img2, ssim);
    return (ssim[0] + ssim[1] + ssim[2]) / 3.0;
}