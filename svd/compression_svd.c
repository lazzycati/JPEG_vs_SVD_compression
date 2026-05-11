#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "compression_svd.h"
#include "loadbmp.h"
#include "subdiscretization.h"

void free_ycbcr_svd(YCbCrSVD *svd) 
{
    if (!svd) return;
    if (svd->Y) free_svdstate(svd->Y);
    if (svd->Cb) free_svdstate(svd->Cb);
    if (svd->Cr) free_svdstate(svd->Cr);
    free(svd);
}

void save_svd_compressed(char *filename, YCbCrSVD *svd, int rY, int rCb, int rCr) 
{
    if (!filename || !svd || !svd->Y || !svd->Cb || !svd->Cr) return;
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    fwrite(&svd->yrows, sizeof(int), 1, f);
    fwrite(&svd->ycols, sizeof(int), 1, f);
    fwrite(&svd->crows, sizeof(int), 1, f);
    fwrite(&svd->ccols, sizeof(int), 1, f);
    fwrite(&rY, sizeof(int), 1, f);
    fwrite(&rCb, sizeof(int), 1, f);
    fwrite(&rCr, sizeof(int), 1, f);
    fwrite(svd->Y->S, sizeof(double), rY, f);
    for (int i = 0; i < svd->yrows; i++) fwrite(svd->Y->U->data[i], sizeof(double), rY, f);
    for (int i = 0; i < svd->ycols; i++) fwrite(svd->Y->V->data[i], sizeof(double), rY, f);
    fwrite(svd->Cb->S, sizeof(double), rCb, f);
    for (int i = 0; i < svd->crows; i++) fwrite(svd->Cb->U->data[i], sizeof(double), rCb, f);
    for (int i = 0; i < svd->ccols; i++) fwrite(svd->Cb->V->data[i], sizeof(double), rCb, f);
    fwrite(svd->Cr->S, sizeof(double), rCr, f);
    for (int i = 0; i < svd->crows; i++) fwrite(svd->Cr->U->data[i], sizeof(double), rCr, f);
    for (int i = 0; i < svd->ccols; i++) fwrite(svd->Cr->V->data[i], sizeof(double), rCr, f);
    fclose(f);
    printf("Сохранено YCbCr SVD: %s (Y: %dx%d, k=%d; C: %dx%d, rCb=%d, rCr=%d)\n", filename, svd->yrows, svd->ycols, rY, svd->crows, svd->ccols, rCb, rCr);
}

YCbCrSVD* load_svd_compressed(char *filename, int *rY, int *rCb, int *rCr) 
{
    if (!filename || !rY || !rCb || !rCr) return NULL;
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    YCbCrSVD *svd = (YCbCrSVD*)malloc(sizeof(YCbCrSVD));
    if (!svd) 
    {
        fclose(f);
        return NULL;
    }
    fread(&svd->yrows, sizeof(int), 1, f);
    fread(&svd->ycols, sizeof(int), 1, f);
    fread(&svd->crows, sizeof(int), 1, f);
    fread(&svd->ccols, sizeof(int), 1, f);
    fread(rY, sizeof(int), 1, f);
    fread(rCb, sizeof(int), 1, f);
    fread(rCr, sizeof(int), 1, f);
    svd->Y = (SVD*)malloc(sizeof(SVD));
    if (!svd->Y) 
    {
        free(svd);
        fclose(f);
        return NULL;
    }
    svd->Y->S = (double*)malloc(*rY * sizeof(double));
    svd->Y->U = (Matrix*)malloc(sizeof(Matrix));
    svd->Y->V = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(svd->Y->U, svd->yrows, *rY);
    createMatrix(svd->Y->V, svd->ycols, *rY);
    fread(svd->Y->S, sizeof(double), *rY, f);
    for (int i = 0; i < svd->yrows; i++) fread(svd->Y->U->data[i], sizeof(double), *rY, f);
    for (int i = 0; i < svd->ycols; i++) fread(svd->Y->V->data[i], sizeof(double), *rY, f);
    svd->Cb = (SVD*)malloc(sizeof(SVD));
    if (!svd->Cb)
    {
        free_ycbcr_svd(svd);
        fclose(f);
        return NULL;
    }
    svd->Cb->S = (double*)malloc(*rCb * sizeof(double));
    svd->Cb->U = (Matrix*)malloc(sizeof(Matrix));
    svd->Cb->V = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(svd->Cb->U, svd->crows, *rCb);
    createMatrix(svd->Cb->V, svd->ccols, *rCb);
    fread(svd->Cb->S, sizeof(double), *rCb, f);
    for (int i = 0; i < svd->crows; i++) fread(svd->Cb->U->data[i], sizeof(double), *rCb, f);
    for (int i = 0; i < svd->ccols; i++) fread(svd->Cb->V->data[i], sizeof(double), *rCb, f);
    svd->Cr = (SVD*)malloc(sizeof(SVD));
    if (!svd->Cr) 
    {
        free_ycbcr_svd(svd);
        fclose(f);
        return NULL;
    }
    svd->Cr->S = (double*)malloc(*rCr * sizeof(double));
    svd->Cr->U = (Matrix*)malloc(sizeof(Matrix));
    svd->Cr->V = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(svd->Cr->U, svd->crows, *rCr);
    createMatrix(svd->Cr->V, svd->ccols, *rCr);
    fread(svd->Cr->S, sizeof(double), *rCr, f);
    for (int i = 0; i < svd->crows; i++) fread(svd->Cr->U->data[i], sizeof(double), *rCr, f);
    for (int i = 0; i < svd->ccols; i++) fread(svd->Cr->V->data[i], sizeof(double), *rCr, f);
    fclose(f);
    svd->rY = *rY;
    svd->rCb = *rCb;
    svd->rCr = *rCr;
    printf("Загружено YCbCr SVD: %s (Y: %dx%d, k=%d; C: %dx%d, rCb=%d, rCr=%d)\n", filename, svd->yrows, svd->ycols, *rY, svd->crows, svd->ccols, *rCb, *rCr);
    return svd;
}

YCbCrImage420* reconstruct_from_svd(YCbCrSVD *svd, int rY, int rCb, int rCr) 
{
    if (!svd || !svd->Y || !svd->Cb || !svd->Cr) return NULL;
    Matrix *Yrec = svd_reconstruct(svd->Y, rY);
    Matrix *Cbrec = svd_reconstruct(svd->Cb, rCb);
    Matrix *Crrec = svd_reconstruct(svd->Cr, rCr);
    if (!Yrec || !Cbrec || !Crrec) 
    {
        printf("Ошибка восстановления матриц\n");
        if (Yrec) freeMatrix(Yrec);
        if (Cbrec) freeMatrix(Cbrec);
        if (Crrec) freeMatrix(Crrec);
        return NULL;
    }
    YCbCrImage420 *img420 = (YCbCrImage420*)malloc(sizeof(YCbCrImage420));
    if (!img420) 
    {
        printf("Ошибка выделения памяти для YCbCrImage420\n");
        freeMatrix(Yrec);
        freeMatrix(Cbrec);
        freeMatrix(Crrec);
        return NULL;
    }
    img420->width = svd->ycols;
    img420->height = svd->yrows;
    img420->Y = (uint8_t*)malloc(svd->yrows * svd->ycols);
    img420->Cb = (uint8_t*)malloc(svd->crows * svd->ccols);
    img420->Cr = (uint8_t*)malloc(svd->crows * svd->ccols);
    if (!img420->Y || !img420->Cb || !img420->Cr) 
    {
        printf("Ошибка выделения памяти для каналов\n");
        free_ycbcr420(img420);
        freeMatrix(Yrec);
        freeMatrix(Cbrec);
        freeMatrix(Crrec);
        return NULL;
    }
    for (int i = 0; i < svd->yrows; i++) 
    {
        for (int j = 0; j < svd->ycols; j++) 
        {
            double val = Yrec->data[i][j];
            val = fmax(0.0, fmin(255.0, val));
            img420->Y[i * svd->ycols + j] = (uint8_t)val;
        }
    }
    for (int i = 0; i < svd->crows; i++) 
    {
        for (int j = 0; j < svd->ccols; j++) 
        {
            double cbval = Cbrec->data[i][j];
            double crval = Crrec->data[i][j];
            cbval = fmax(0.0, fmin(255.0, cbval));
            crval = fmax(0.0, fmin(255.0, crval));
            img420->Cb[i * svd->ccols + j] = (uint8_t)cbval;
            img420->Cr[i * svd->ccols + j] = (uint8_t)crval;
        }
    }
    freeMatrix(Yrec);
    freeMatrix(Cbrec);
    freeMatrix(Crrec);
    printf("Восстановлено YCbCr420 изображение из SVD (Y: k=%d, Cb: k=%d, Cr: k=%d)\n", rY, rCb, rCr);
    return img420;
}

void print_stats(char *svd_filename, char *bmp_filename, int m, int n, int k) 
{
    if (!svd_filename || !bmp_filename) return;
    FILE *fsvd = fopen(svd_filename, "rb");
    if (!fsvd) return;
    int svdbytes = ftell(fsvd);
    fseek(fsvd, 0, SEEK_END);
    fclose(fsvd);
    FILE *fbmp = fopen(bmp_filename, "rb");
    if (!fbmp) return;
    fseek(fbmp, 0, SEEK_END);
    int bmpbytes = ftell(fbmp);
    fclose(fbmp);
    double cr = (double)bmpbytes / (double)svdbytes;
    printf("Исходный BMP файл: %10ld байт\n", bmpbytes);
    printf("Сжатый SVD файл: %10ld байт\n", svdbytes);
    printf("Степень сжатия: %10.2f:1\n", cr);
    if (cr > 1.0) 
    {
        printf("Достигнуто сжатие в %.2f раз\n", cr);
    }
    else if (cr < 1.0) 
    {
        printf("Произошло расширение файла (размер увеличился в %.2f раз)\n", 1.0 / cr);
    } 
    else 
    {
        printf("Размер не изменился\n");
    }
    double percent = (1.0 - 1.0 / cr) * 100;
    if (percent > 0) printf("Экономия места: %.1f%%\n", percent);
    printf("\n");
}
