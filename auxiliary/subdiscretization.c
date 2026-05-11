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
