#include <stdint.h>
#include "subdiscretization.h"
#include <stdlib.h>
#include <string.h>
//Сжатие в 2 раза только за счет субдискретизации цветности
YCbCrImage420* convert_to_sub420(YCbCrImage *img)
{
    YCbCrImage420 *img420 = (YCbCrImage420*)malloc(sizeof(YCbCrImage420));
    img420->width = img->width;
    img420->height = img->height;
    // Y канал без изменений
    img420->Y = (uint8_t*)malloc(img->width * img->height);
    memcpy(img420->Y, img->Y, img->width * img->height);
    int w = (img->width + 1) / 2;
    int h = (img->height + 1) / 2;
    img420->Cb = (uint8_t*)malloc(w * h);
    img420->Cr = (uint8_t*)malloc(w * h);
    // Усреднение блоков 2x2
    for (int i = 0; i < img->height; i += 2) 
    {
        for (int j = 0; j < img->width; j += 2) 
        {
            int sumCb = 0, sumCr = 0;
            int count = 0;
            for (int di = 0; di < 2 && (i + di) < img->height; di++) 
            {
                for (int dj = 0; dj < 2 && (j + dj) < img->width; dj++) 
                {
                    //Суммируем значения цветности всех пикселей в текущем блоке
                    sumCb += img->Cb[(i + di) * img->width + (j + dj)];
                    sumCr += img->Cr[(i + di) * img->width + (j + dj)];
                    count++;
                }
            }
            //Вычисляем среднее арифметическое цветности в блоке, сохраняем результат в новое, уменьшенное изображение
            int subi = i / 2;
            int subj = j / 2;
            img420->Cb[subi * w + subj] = sumCb / count;
            img420->Cr[subi * w + subj] = sumCr / count;
        }
    }
    return img420;
}

//Для Cb и Cr работает биилинейная интерполяция - каждый новый пискель - среднее взвешенное 4 соседних
YCbCrImage* convert_from_sub420(YCbCrImage420 *img420)
{
    if (!img420) return NULL;
    YCbCrImage *img = (YCbCrImage*)malloc(sizeof(YCbCrImage));
    if (!img) return NULL;
    img->width = img420->width;
    img->height = img420->height;
    img->Y = (uint8_t*)malloc(img->width * img->height);
    img->Cb = (uint8_t*)malloc(img->width * img->height);
    img->Cr = (uint8_t*)malloc(img->width * img->height);
    if (!img->Y || !img->Cb || !img->Cr) 
    {
        if (img->Y) free(img->Y);
        if (img->Cb) free(img->Cb);
        if (img->Cr) free(img->Cr);
        free(img);
        return NULL;
    }
    memcpy(img->Y, img420->Y, img->width * img->height);
    int w_sub = (img->width + 1) / 2;
    int h_sub = (img->height + 1) / 2;
    // Восстанавливаем Cb и Cr каналы с билинейной интерполяцией
    for (int y = 0; y < img->height; y++) 
    {
        for (int x = 0; x < img->width; x++) 
        {
            // Координаты в субдискретизированном изображении
            double fx = (double)x / 2.0f;
            double fy = (double)y / 2.0f;
            int x0 = (int)fx;
            int y0 = (int)fy;
            int x1 = (x0 + 1 < w_sub) ? x0 + 1 : x0;
            int y1 = (y0 + 1 < h_sub) ? y0 + 1 : y0;
            // Веса для интерполяции
            double dx = fx - x0;
            double dy = fy - y0;
            double wx0 = 1.0f - dx;
            double wx1 = dx;
            double wy0 = 1.0f - dy;
            double wy1 = dy;
            // Билинейная интерполяция для Cb
            int idx00 = y0 * w_sub + x0;
            int idx10 = y0 * w_sub + x1;
            int idx01 = y1 * w_sub + x0;
            int idx11 = y1 * w_sub + x1;
            double cb = wy0 * (wx0 * img420->Cb[idx00] + wx1 * img420->Cb[idx10]) + wy1 * (wx0 * img420->Cb[idx01] + wx1 * img420->Cb[idx11]);
            double cr = wy0 * (wx0 * img420->Cr[idx00] + wx1 * img420->Cr[idx10]) + wy1 * (wx0 * img420->Cr[idx01] + wx1 * img420->Cr[idx11]);
            img->Cb[y * img->width + x] = (uint8_t)cb;
            img->Cr[y * img->width + x] = (uint8_t)cr;
        }
    }
    return img;
}

void free_ycbcr420(YCbCrImage420 *img)
{
    if (img) 
    {
        if (img->Y) free(img->Y);
        if (img->Cb) free(img->Cb);
        if (img->Cr) free(img->Cr);
        free(img);
    }
}
