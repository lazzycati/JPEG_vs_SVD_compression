#include <math.h>
#include "metrics.h"
#include <stdio.h>
//метрика, измеряющая отношение максимально возможной мощности сигнала к мощности искажающего шума
//насколько сильно отличается каждое пиксельное значение одного изображения от другого, основан на MSE
//PSNR = 10 * log₁₀(MAX² / MSE) (логарифм для приближения к человеческому восприятию)
double PSNR(Image *img1, Image *img2)
{
    int w = img1->width;
    int h = img1->height;
    if (!img1 || !img2 || !img1->data || !img2->data || w <= 0 || h <= 0) return -1.0;
    int cpixels = w * h;
    double mse = 0.0;
    for (int i = 0; i < cpixels; i++) 
    {
        int diff = img1->data[i] - img2->data[i];
        mse += (double)(diff * diff);
    }
    mse /= cpixels;
    if (mse == 0.0) 
    {
        printf("Изображения идентичны, PSNR = INF \n");
        return INFINITY;  
    }
    double psnr = 10.0 * log10((MAX_PIXEL * MAX_PIXEL) / mse);
    return psnr;
}
//Проблема: PSNR не учитывает струетуру изображения (ведь человек ориентируентся на восприятие а не какие-то цифры), это делает SSIM


// Вычисляет SSIM для одного окна (патча)
static double one_window_ssim(Image *img1, Image *img2, int x, int y, int wsize)
{
    int w = img1->width;
    int h = img1->height;
    int mid = wsize / 2;
    int x1 = (x - mid < 0) ? 0 : x - mid;
    int x2 = (x + mid >= w) ? w - 1 : x + mid;
    int y1 = (y - mid < 0) ? 0 : y - mid;
    int y2 = (y + mid >= h) ? h - 1 : y + mid;
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    int n = w * h;
    //Вычисляем средние (μx, μy)
    double sumx = 0.0, sumy = 0.0;
    for (int i = y1; i <= y2; i++) 
    {
        for (int j = x1; j <= x2; j++) 
        {
            sumx += img1->data[i * w + j];
            sumy += img2->data[i * w + j];
        }
    }
    double mux = sumx / n;
    double muy = sumy / n;
    //Вычисляем дисперсии (σx², σy²) и ковариацию (σxy)
    double varx = 0.0, vary = 0.0, covxy = 0.0;
    for (int i = y1; i <= y2; i++) 
    {
        for (int j = x1; j <= x2; j++) 
        {
            double dx = img1->data[i * w + j] - mux;
            double dy = img2->data[i * w + j] - muy;
            varx += dx * dx; // σx² = Σ(dx²)
            vary += dy * dy;  // σy² = Σ(dy²)
            covxy += dx * dy;  // σxy = Σ(dx·dy)
        }
    }
    varx /= n;
    vary /= n;
    covxy /= n;
    double sigmax = sqrt(varx);
    double sigmay = sqrt(vary);
    //Вычисляем компоненты SSIM
    double luminance = (2.0 * mux * muy + C1) / (mux * mux + muy * muy + C1);
    double contrast = (2.0 * sigmax * sigmay + C2) / (varx + vary + C2);
    double structure = (covxy + C3) / (sigmax * sigmay + C3);
    //Итоговый SSIM
    return luminance * contrast * structure;
}

//SSIM оценивает структурное сходство изображений. Сравнивает целые патчи (окрестности). рассматривает такие компоненты чел. восприятия, как:
//Яркость (luminance) — средний уровень освещенности, контраст (contrast) — разброс значений относительно среднего, 
//структура (structure) — взаимосвязи между соседними пикселями (ковариация)
//SSIM(x,y) = [luminance(x,y)]^α * [contrast(x,y)]^β * [structure(x,y)]^γ, обычно α = β = γ = 1, поэтому: 
//SSIM(x,y) = luminance * contrast * structure. (так как компоненты восприятия независимы)
//1. luminance: l(x,y) = (2μx·μy + C₁) / (μx² + μy² + C₁), где μx, μy — средние значения в окне, 
//C₁ = (K₁·L)² — стабилизирующая константа (K₁=0.01, L=255), чтобы избежать деления на 0 при μx, μy = 0
//2. contrast: c(x,y) = (2σx·σy + C₂) / (σx² + σy² + C₂), где σx, σy — стандартные отклонения, C₂ = (K₂·L)² (K₂=0.03)
//3. structure: s(x,y) = (σxy + C₃) / (σx·σy + C₃), где σxy — ковариация между x и y, C₃ = C₂/2
// Вычисляет средний SSIM по всему изображению (скользящим окном)
double SSIM(Image *img1, Image *img2)
{
    int w = img1->width;
    int h = img1->height;
    if (!img1 || !img2 || !img1->data || !img2->data || w <= 0 || h <= 0) return -1.0;
    double ssim = 0.0;
    int count = 0;
    // Проходим по всем пикселям с шагом 1 (или можно шаг = window_size/2)
    for (int y = 0; y < h; y++) 
    {
        for (int x = 0; x < w; x++) 
        {
            ssim += one_window_ssim(img1, img2, x, y, WSIZE);
            count++;
        }
    }
    return ssim / count;
}