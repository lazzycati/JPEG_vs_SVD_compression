#include <stdio.h>
#include "compression_svd.h"
#include <stdlib.h>

typedef struct Matrix {
    double **data;
    int rows;
    int cols;
} Mattrix;

typedef struct SVD {
    Matrix *U;      
    double *S;      
    Matrix *V;      
} SVD;

//сохраняем восстановленное сжатое изображение в bmp-формате
void save_reconstructedmatrix_to_bmp(SVD *svd, int k, int m, int n, char *filename) 
{
    Matrix *rec = svd_reconstruct(svd, k);
    Image *img = (Image*)malloc(sizeof(Image));
    matrix_to_image(rec, img);
    store_to_bmp(filename, img);
    free_image(img);
    freeMatrix(rec);
    printf("Сохранено восстановленное изображение: %s (k=%d)\n", filename, k);
}
//сохраняем svd-разложение для быстрого доступа в бинарный файл
void save_svd_compressed(char *filename, SVD *svd, int k, int m, int n) 
{
    FILE *f = fopen(filename, "wb");
    if (!f) 
    {
        printf("Не удалось создать файл %s\n", filename);
        return;
    }
    fwrite(&m, sizeof(int), 1, f);
    fwrite(&n, sizeof(int), 1, f);
    fwrite(&k, sizeof(int), 1, f);
    fwrite(svd->S, sizeof(double), k, f);
    for (int i = 0; i < m; i++) fwrite(&svd->U->data[i][0], sizeof(double), k, f);
    for (int i = 0; i < n; i++) fwrite(&svd->V->data[i][0], sizeof(double), k, f);
}
//Читаем svd-разложение из бинарного файла
SVD* load_svd_compressed(char *filename, int *m, int *n, int *k) 
{
    FILE *f = fopen(filename, "rb");
    if (!f) 
    {
        printf("Не удалось открыть файл %s\n", filename);
        return NULL;
    }
    fread(m, sizeof(int), 1, f);
    fread(n, sizeof(int), 1, f);
    fread(k, sizeof(int), 1, f);
    SVD *svd = (SVD*)malloc(sizeof(SVD));
    svd->S = (double*)malloc(*k * sizeof(double));
    svd->U = (Matrix*)malloc(sizeof(Matrix));
    svd->V = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(svd->U, *m, *k);
    createMatrix(svd->V, *n, *k);
    fread(svd->S, sizeof(double), *k, f);
    for (int i = 0; i < *m; i++) fread(&svd->U->data[i][0], sizeof(double), *k, f);
    for (int i = 0; i < *n; i++) fread(&svd->V->data[i][0], sizeof(double), *k, f);
    fclose(f);
    printf("Загружено сжатое представление: %s (m=%d, n=%d, k=%d)\n", filename, *m, *n, *k);
    return svd;
}
//сохраняем svd в бинарный файл с приведением трех матриц к типу float для сравнения сжатия
void save_svd_compressed_float(char *filename, SVD *svd, int k, int m, int n) 
{
    FILE *f = fopen(filename, "wb");
    if (!f) 
    {
        printf("Не удалось создать файл %s\n", filename);
        return;
    }
    fwrite(&m, sizeof(int), 1, f);
    fwrite(&n, sizeof(int), 1, f);
    fwrite(&k, sizeof(int), 1, f);
    float *S_float = (float*)malloc(k * sizeof(float));
    for (int i = 0; i < k; i++) S_float[i] = (float)svd->S[i];
    fwrite(S_float, sizeof(float), k, f);
    free(S_float);
    float *U_row = (float*)malloc(k * sizeof(float));
    for (int i = 0; i < m; i++) 
    {
        for (int j = 0; j < k; j++) U_row[j] = (float)svd->U->data[i][j];
        fwrite(U_row, sizeof(float), k, f);
    }
    free(U_row);
    float *V_row = (float*)malloc(k * sizeof(float));
    for (int i = 0; i < n; i++) 
    {
        for (int j = 0; j < k; j++) V_row[j] = (float)svd->V->data[i][j];
        fwrite(V_row, sizeof(float), k, f);
    }
    free(V_row);
    fclose(f);
    int svd_bytes = sizeof(int) * 3 + k * sizeof(float) + m * k * sizeof(float) + n * k * sizeof(float);
    printf("Сжатое представление (float) сохранено: %s\n", filename);
    printf("Размер файла: ~%.1f KB (k=%d)\n", k, svd_bytes / 1024.0);
}
//Читаем float-svd-разложение из бинарного файла
SVD* load_svd_compressed_float(char *filename, int *m, int *n, int *k) 
{
    FILE *f = fopen(filename, "rb");
    if (!f) 
    {
        printf("Не удалось открыть файл %s\n", filename);
        return NULL;
    }
    fread(m, sizeof(int), 1, f);
    fread(n, sizeof(int), 1, f);
    fread(k, sizeof(int), 1, f);
    SVD *svd = (SVD*)malloc(sizeof(SVD));
    svd->S = (double*)malloc(*k * sizeof(double));
    svd->U = (Matrix*)malloc(sizeof(Matrix));
    svd->V = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(svd->U, *m, *k);
    createMatrix(svd->V, *n, *k);
    float *S_float = (float*)malloc(*k * sizeof(float));
    fread(S_float, sizeof(float), *k, f);
    for (int i = 0; i < *k; i++) svd->S[i] = (double)S_float[i];
    free(S_float);
    float *U_row = (float*)malloc(*k * sizeof(float));
    for (int i = 0; i < *m; i++) 
    {
        fread(U_row, sizeof(float), *k, f);
        for (int j = 0; j < *k; j++) svd->U->data[i][j] = (double)U_row[j];
    }
    free(U_row);
    float *V_row = (float*)malloc(*k * sizeof(float));
    for (int i = 0; i < *n; i++) 
    {
        fread(V_row, sizeof(float), *k, f);
        for (int j = 0; j < *k; j++) svd->V->data[i][j] = (double)V_row[j];
    }
    free(V_row);
    fclose(f);
    printf("Загружено сжатое представление (float): %s (m=%d, n=%d, k=%d)\n", filename, *m, *n, *k);
    return svd;
}
//Выводим информацию о сжатии
void print_compression_stats(char *svd_filename, char *bmp_filename, int m, int n, int k) 
{
    FILE *f_svd = fopen(svd_filename, "rb");
    if (!f_svd) 
    {
        printf("Не удалось открыть %s\n", svd_filename);
        return;
    }
    fseek(f_svd, 0, SEEK_END);
    int svd_bytes = ftell(f_svd);
    fclose(f_svd);
    FILE *f_bmp = fopen(bmp_filename, "rb");
    if (!f_bmp) 
    {
        printf("Не удалось открыть %s\n", bmp_filename);
        return;
    }
    fseek(f_bmp, 0, SEEK_END);
    int bmp_bytes = ftell(f_bmp);
    fclose(f_bmp);
    int svd_calc = sizeof(int) * 3 + k * sizeof(double) + m * k * sizeof(double) + n * k * sizeof(double);
    double ratio = ((double)bmp_bytes) / (svd_bytes * 3);
    printf("\n=== Статистика сжатия ===\n");
    printf("Параметры: m=%d, n=%d, k=%d\n", m, n, k);
    printf("Исходный BMP (grayscale): %d байт\n", bmp_bytes);
    printf("Сжатый SVD-файл:          %d байт\n", svd_bytes);
    printf("Расчетный размер SVD:     %d байт\n", svd_calc);
    printf("Степень сжатия:           %.2f:1 %s\n", ratio, (ratio >= 1.0) ? "(сжатие)" : "(расширение)");
}
//Выводим информацию о float-сжатии
void print_compression_stats_float(char *svd_filename, char *bmp_filename, int m, int n, int k) 
{
    FILE *f_svd = fopen(svd_filename, "rb");
    if (!f_svd) 
    {
        printf("Не удалось открыть %s\n", svd_filename);
        return;
    }
    fseek(f_svd, 0, SEEK_END);
    int svd_bytes = ftell(f_svd);
    fclose(f_svd);
    FILE *f_bmp = fopen(bmp_filename, "rb");
    if (!f_bmp) 
    {
        printf("Не удалось открыть %s\n", bmp_filename);
        return;
    }
    fseek(f_bmp, 0, SEEK_END);
    int bmp_bytes = ftell(f_bmp);
    fclose(f_bmp);
    int svd_calc_float = sizeof(int) * 3 + k * sizeof(float) + m * k * sizeof(float) + n * k * sizeof(float);
    int svd_calc_double = sizeof(int)*3 + k * sizeof(double) + m * k * sizeof(double) + n * k * sizeof(double);
    double ratio = (double)bmp_bytes / (svd_bytes * 3);
    printf("\n=== Статистика сжатия (float) ===\n");
    printf("Параметры: m=%d, n=%d, k=%d\n", m, n, k);
    printf("Исходный BMP (RGB):       %d байт\n", bmp_bytes);
    printf("Сжатый SVD-файл (float):  %d байт\n", svd_bytes);
    printf("Расчетный размер (float): %d байт\n", svd_calc_float);
    printf("Расчетный размер (double):%d байт\n", svd_calc_double);
    printf("Степень сжатия:           %.2f:1 %s\n", ratio, (ratio >= 1.0) ? "(сжатие)" : "(расширение)");
}