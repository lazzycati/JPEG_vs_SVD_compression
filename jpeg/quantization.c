#include "quantization.h"
#include <stdio.h>
#include <math.h>

const int luminance_qt[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
};

const int chrominance_qt[8][8] = {
    {17, 18, 24, 47, 99, 99, 99, 99},
    {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
    {47, 66, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}
};
// Масштабирование таблицы под нужное качество
void scale_quantization_table(int out[8][8], const int in[8][8], int quality) 
{
    int scale;
    if (quality < 50) scale = 5000 / quality;
    else scale = 200 - 2 * quality;
    for (int i = 0; i < 8; i++) 
    {
        for (int j = 0; j < 8; j++) 
        {
            int val = (in[i][j] * scale + 50) / 100;
            // Ограничиваем диапазон [1, 255]
            if (val < 1) val = 1;
            if (val > 255) val = 255;
            out[i][j] = val;
        }
    }
}

// Применение квантования ко всем блокам JPEG изображения
void quantize_jpeg(JPEGImage *jpeg, int quality) 
{
    // Создаем таблицы квантования под нужное качество
    int lum_qt[8][8], chrom_qt[8][8];
    scale_quantization_table(lum_qt, luminance_qt, quality);
    scale_quantization_table(chrom_qt, chrominance_qt, quality);
    printf("Квантование (качество = %d)...\n", quality);
    // Квантуем Y блоки (luminance)
    for (int by = 0; by < jpeg->yblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->xblocks; bx++) 
        {
            Block *block = &jpeg->Yblocks[by][bx];
            for (int i = 0; i < 8; i++) 
            {
                for (int j = 0; j < 8; j++) 
                {
                    //Cохраняем как int для последующего сжатия
                    int quantized = (int)floor(block->coeff[i][j] / lum_qt[i][j]);
                    block->coeff[i][j] = quantized;  
                }
            }
        }
    }
    // Квантуем Cb и Cr блоки (chrominance)
    for (int by = 0; by < jpeg->cbyblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->cbxblocks; bx++) 
        {
            Block *block1 = &jpeg->Cbblocks[by][bx];
            for (int i = 0; i < 8; i++) 
            {
                for (int j = 0; j < 8; j++) 
                {
                    int quantized = (int)floor(block1->coeff[i][j] / chrom_qt[i][j]);
                    block1->coeff[i][j] = quantized;
                }
            }
            Block *block2 = &jpeg->Crblocks[by][bx];
            for (int i = 0; i < 8; i++) 
            {
                for (int j = 0; j < 8; j++) 
                {
                    int quantized = (int)floor(block2->coeff[i][j] / chrom_qt[i][j]);
                    block2->coeff[i][j] = quantized;
                }
            }
        }
    }
    printf("Квантование завершено\n");
}

void dequantize_jpeg(JPEGImage *jpeg, int quality) 
{
    int lum_qt[8][8], chrom_qt[8][8];
    scale_quantization_table(lum_qt, luminance_qt, quality);
    scale_quantization_table(chrom_qt, chrominance_qt, quality);
    printf("Обратное квантование\n");
    for (int by = 0; by < jpeg->yblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->xblocks; bx++) 
        {
            Block *block = &jpeg->Yblocks[by][bx];
            for (int i = 0; i < 8; i++) 
            {
                for (int j = 0; j < 8; j++) 
                {
                    block->coeff[i][j] = block->coeff[i][j] * lum_qt[i][j];
                }
            }
        }
    }
    for (int by = 0; by < jpeg->cbyblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->cbxblocks; bx++) 
        {
            Block *block = &jpeg->Cbblocks[by][bx];
            for (int i = 0; i < 8; i++) 
            {
                for (int j = 0; j < 8; j++) 
                {
                    block->coeff[i][j] = block->coeff[i][j] * chrom_qt[i][j];
                }
            }
            block = &jpeg->Crblocks[by][bx];
            for (int i = 0; i < 8; i++) 
            {
                for (int j = 0; j < 8; j++) 
                {
                    block->coeff[i][j] = block->coeff[i][j] * chrom_qt[i][j];
                }
            }
        }
    }
    printf("Обратное квантование завершено\n");
}