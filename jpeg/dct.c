#include "dct.h"
#include <math.h>
#include <stdint.h>

Block** split_into_blocks(uint8_t *channel, int width, int height, int *numxblocks, int *numyblocks) 
{
    if (!channel || width <= 0 || height <= 0 || !numxblocks || !numyblocks) return NULL;
    *numxblocks = (width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    *numyblocks = (height + BLOCK_SIZE - 1) / BLOCK_SIZE;
    Block **blocks = (Block**)malloc(*numyblocks * sizeof(Block*));
    if (!blocks) return NULL;
    for (int i = 0; i < *numyblocks; i++) {
        blocks[i] = (Block*)malloc(*numxblocks * sizeof(Block));
        if (!blocks[i]) 
        {
            for (int j = 0; j < i; j++) free(blocks[j]);
            free(blocks);
            return NULL;
        }
    }
    for (int by = 0; by < *numyblocks; by++) 
    {
        for (int bx = 0; bx < *numxblocks; bx++) 
        {
            Block *block = &blocks[by][bx];
            for (int y = 0; y < BLOCK_SIZE; y++) 
            {
                int py = by * BLOCK_SIZE + y;
                if (py >= height) py = height - 1;
                for (int x = 0; x < BLOCK_SIZE; x++) 
                {
                    int px = bx * BLOCK_SIZE + x;
                    if (px >= width) px = width - 1;
                    // DC-сдвиг диапазона: [0, 255] -> [-128, 127]
                    block->coeff[y][x] = (double)channel[py * width + px] - 128.0;
                }
            }
        }
    }
    return blocks;
}

JPEGImage* prep_for_dct(YCbCrImage420 *img) 
{
    if (!img) return NULL;
    JPEGImage *jpeg = (JPEGImage*)malloc(sizeof(JPEGImage));
    if (!jpeg) return NULL;
    // Разбиваем Y канал (полное разрешение)
    jpeg->Yblocks = split_into_blocks(img->Y, img->width, img->height, &jpeg->xblocks, &jpeg->yblocks);
    if (!jpeg->Yblocks) 
    {
        free(jpeg);
        return NULL;
    }
    // Разбиваем Cb, Cr каналы (уменьшенные)
    int w = (img->width + 1) / 2;
    int h = (img->height + 1) / 2;
    jpeg->Cbblocks = split_into_blocks(img->Cb, w, h, &jpeg->cbxblocks, &jpeg->cbyblocks);
    if (!jpeg->Cbblocks) 
    {
        for (int i = 0; i < jpeg->yblocks; i++) free(jpeg->Yblocks[i]);
        free(jpeg->Yblocks);
        free(jpeg);
        return NULL;
    }
    jpeg->Crblocks = split_into_blocks(img->Cr, w, h, &jpeg->cbxblocks, &jpeg->cbyblocks);
    if (!jpeg->Crblocks) 
    {
        for (int i = 0; i < jpeg->yblocks; i++) free(jpeg->Yblocks[i]);
        free(jpeg->Yblocks);
        for (int i = 0; i < jpeg->cbyblocks; i++) free(jpeg->Cbblocks[i]);
        free(jpeg->Cbblocks);
        free(jpeg);
        return NULL;
    }
    printf("Подготовлено блоков: Y: %dx%d, Cb: %dx%d, Cr: %dx%d\n", jpeg->xblocks, jpeg->yblocks, jpeg->cbxblocks, jpeg->cbyblocks, jpeg->cbxblocks, jpeg->cbyblocks);
    return jpeg;
}

//Сложность: вместо O(N⁴) -> O(2·N³) 
void dct8(double in[8][8], double out[8][8]) 
{
    double t[8][8];
    // 1D DCT горизонтальное преобразование
    for (int i = 0; i < 8; i++) 
    {
        for (int v = 0; v < 8; v++) 
        {
            // Нормировка для корректного обратного Dct и сохранения ортонормированности 
            double cv = (v == 0) ? 1.0 / sqrt(2.0) : 1.0;
            double sum = 0.0;
            for (int j = 0; j < 8; j++) sum += in[i][j] * cos((2 * j + 1) * v * M_PI / 16.0);
            t[i][v] = sum * cv * 0.5;
        }
    }
    // 1D DCT вертикальное
    //Для каждой частоты v (уже обработанной по горизонтали) применяем вертикальное преобразование с частотой u
    for (int u = 0; u < 8; u++) 
    {
        for (int v = 0; v < 8; v++) 
        {
            double cu = (u == 0) ? 1.0 / sqrt(2.0) : 1.0;
            double sum = 0.0;
            for (int i = 0; i < 8; i++) 
            {
                sum += t[i][v] * cos((2 * i + 1) * u * M_PI / 16.0);
            }
            out[u][v] = sum * cu * 0.5;
        }
    }
}

void DCT(JPEGImage *jpeg) 
{
    printf("Применение DCT к субдискретизированному изображению...\n");
    for (int by = 0; by < jpeg->yblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->xblocks; bx++) 
        {
            Block *block = &jpeg->Yblocks[by][bx];
            double out[8][8];
            dct8(block->coeff, out);
            memcpy(block->coeff, out, sizeof(double) * 64);
        }
    }
    for (int by = 0; by < jpeg->cbyblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->cbxblocks; bx++) 
        {
            Block *block = &jpeg->Cbblocks[by][bx];
            double out1[8][8];
            dct8(block->coeff, out1);
            memcpy(block->coeff, out1, sizeof(double) * 64);
            block = &jpeg->Crblocks[by][bx];
            double out2[8][8];
            dct8(block->coeff, out2);
            memcpy(block->coeff, out2, sizeof(double) * 64);
        }
    }
    printf("DCT завершено\n");
}

void free_jpeg(JPEGImage *jpeg) 
{
    if (!jpeg) return;
    if (jpeg->Yblocks) 
    {
        for (int i = 0; i < jpeg->yblocks; i++) if (jpeg->Yblocks[i]) free(jpeg->Yblocks[i]);
        free(jpeg->Yblocks);
    }
    if (jpeg->Cbblocks) 
    {
        for (int i = 0; i < jpeg->cbyblocks; i++) if (jpeg->Cbblocks[i]) free(jpeg->Cbblocks[i]);
        free(jpeg->Cbblocks);
    }
    if (jpeg->Crblocks) 
    {
        for (int i = 0; i < jpeg->cbyblocks; i++) if (jpeg->Crblocks[i]) free(jpeg->Crblocks[i]);
        free(jpeg->Crblocks);
    }
    free(jpeg);
}