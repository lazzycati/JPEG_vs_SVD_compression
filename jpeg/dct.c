#include "dct.h"

Block** split_into_blocks(uint8_t *channel, int width, int height, int *numxblocks, int *numyblocks) 
{
    if (!channel || width <= 0 || height <= 0 || !numxblocks || !numyblocks) return NULL;
    *numxblocks = (width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    *numyblocks = (height + BLOCK_SIZE - 1) / BLOCK_SIZE;
    Block **blocks = (Block**)malloc(*numyblocks * sizeof(Block*));
    if (!blocks) return NULL;
    for (int i = 0; i < *numyblocks; i++) 
    {
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
                for (int x = 0; x < BLOCK_SIZE; x++) 
                {
                    int px = bx * BLOCK_SIZE + x;
                    if (py < height && px < width) 
                    {
                        //Нормировка - центрирование вокруг нуля (улучшает сжатие - т.к. сами косинусоиды на которые будем раскладыв.
                        //колеблются вокруг нуля - разложение будет эффективнее - энергия сконцентир. в меньшем объеме коэф.)
                        block->coeff[y][x] = (double)channel[py * width + px] - 128.0;
                    } 
                    else block->coeff[y][x] = 0.0;
                }
            }
            if (by == *numyblocks - 1 || bx == *numxblocks - 1) 
            {
                for (int y = 0; y < BLOCK_SIZE; y++) 
                {
                    //Симметричное отражение (без обрезания на границах)
                    int py = by * BLOCK_SIZE + y;
                    if (py >= height) py = 2 * height - py - 1;  
                    for (int x = 0; x < BLOCK_SIZE; x++) 
                    {
                        int px = bx * BLOCK_SIZE + x;
                        if (px >= width) px = 2 * width - px - 1;  
                        if (py < height && px < width) block->coeff[y][x] = (double)channel[py * width + px] - 128.0;
                    }
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
    memset(jpeg, 0, sizeof(JPEGImage));
    jpeg->width = img->width;    
    jpeg->height = img->height; 
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
            // Нормировка для корректного обратного Dct и сохранения ортонормированности для сохранения энергии сигнала
            //Сумма квадратов пикселей = Сумма квадратов коэффициентов - чтобы ошибка в частотной области не влекла ошибку в про-нной
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
//Дискретное косинусное преобразование (DCT) — способ представить блок 8×8 как сумму 64 косинусоид разной частоты.
//F(u,v) = 0.5 * C(u) * C(v) * Σ(x=0..7) Σ(y=0..7) f(x,y) * cos((2x+1)*u*π/16) * cos((2y+1)*v*π/16)
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

//f(x,y) = 0.25 * Σ(u=0..7) Σ(v=0..7) C(u) C(v) F(u,v) * cos((2x+1)uπ/16) * cos((2y+1)vπ/16)
void idct8(double in[8][8], double out[8][8]) 
{
    for (int x = 0; x < 8; x++) 
    {
        for (int y = 0; y < 8; y++) 
        {
            double sum = 0.0;
            for (int u = 0; u < 8; u++) 
            {
                for (int v = 0; v < 8; v++) 
                {
                    double cu = (u == 0) ? 1.0 / sqrt(2.0) : 1.0;
                    double cv = (v == 0) ? 1.0 / sqrt(2.0) : 1.0;
                    sum += cu * cv * in[u][v] * cos((2 * x + 1) * u * M_PI / 16.0) * cos((2 * y + 1) * v * M_PI / 16.0);
                }
            }
            out[x][y] = sum / 4.0;
        }
    }
}

void inverse_DCT(JPEGImage *jpeg) 
{
    printf("Применение обратного DCT\n");
    for (int by = 0; by < jpeg->yblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->xblocks; bx++) 
        {
            Block *block = &jpeg->Yblocks[by][bx];
            double out[8][8];
            idct8(block->coeff, out);
            memcpy(block->coeff, out, sizeof(double) * 64);
        }
    }
    for (int by = 0; by < jpeg->cbyblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->cbxblocks; bx++) 
        {
            Block *block = &jpeg->Cbblocks[by][bx];
            double out[8][8];
            idct8(block->coeff, out);
            memcpy(block->coeff, out, sizeof(double) * 64);
            block = &jpeg->Crblocks[by][bx];
            idct8(block->coeff, out);
            memcpy(block->coeff, out, sizeof(double) * 64);
        }
    }
    printf("Обратное DCT завершено\n");
}

YCbCrImage420* restore_from_blocks(JPEGImage *jpeg) 
{
    if (!jpeg) return NULL;
    YCbCrImage420 *img = (YCbCrImage420*)malloc(sizeof(YCbCrImage420));
    if (!img) return NULL;
    img->width = jpeg->width;
    img->height = jpeg->height;
    img->Y = (uint8_t*)malloc(img->width * img->height);
    int w = (img->width + 1) / 2;
    int h = (img->height + 1) / 2;
    img->Cb = (uint8_t*)malloc(w * h);
    img->Cr = (uint8_t*)malloc(w * h);
    if (!img->Y || !img->Cb || !img->Cr) 
    {
        free_jpeg(jpeg);
        free(img);
        return NULL;
    }
    // Восстановление Y канала
    for (int by = 0; by < jpeg->yblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->xblocks; bx++) 
        {
            Block *block = &jpeg->Yblocks[by][bx];
            for (int y = 0; y < BLOCK_SIZE; y++) 
            {
                int py = by * BLOCK_SIZE + y;
                if (py >= img->height) continue;
                for (int x = 0; x < BLOCK_SIZE; x++) 
                {
                    int px = bx * BLOCK_SIZE + x;
                    if (px >= img->width) continue;
                    int val = (int)round(block->coeff[y][x] + 128.0);
                    //Клиппируем
                    if (val < 0) val = 0;
                    if (val > 255) val = 255;
                    img->Y[py * img->width + px] = (uint8_t)val;
                }
            }
        }
    }
    // Восстановление Cb и Cr каналов
    for (int by = 0; by < jpeg->cbyblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->cbxblocks; bx++) 
        {
            Block *blockCb = &jpeg->Cbblocks[by][bx];
            Block *blockCr = &jpeg->Crblocks[by][bx];
            for (int y = 0; y < BLOCK_SIZE; y++) 
            {
                int py = by * BLOCK_SIZE + y;
                if (py >= h) continue;
                for (int x = 0; x < BLOCK_SIZE; x++) 
                {
                    int px = bx * BLOCK_SIZE + x;
                    if (px >= w) continue;
                    int valCb = (int)round(blockCb->coeff[y][x] + 128.0);
                    int valCr = (int)round(blockCr->coeff[y][x] + 128.0);
                    if (valCr < 0) valCr = 0;
                    if (valCr > 255) valCr = 255;
                    img->Cb[py * w + px] = (uint8_t)valCb;
                    img->Cr[py * w + px] = (uint8_t)valCr;
                }
            }
        }
    }
    return img;
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