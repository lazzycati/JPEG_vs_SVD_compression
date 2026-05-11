#pragma once
#include "basicmatrix.h"
#include "subdiscretization.h"
#define EPS 1e-10
#define MAX_ITER 1000
typedef struct SVD {
    Matrix *U;
    double *S;
    Matrix *V;
} SVD;

typedef struct YCbCrSVD {
    SVD *Y;
    SVD *Cb;
    SVD *Cr;
    int yrows, ycols;
    int crows, ccols;
    int rY, rCb, rCr;
} YCbCrSVD;

typedef struct Matrix Matrix;

//svd-разложение двусторонним якоби
SVD* svd_double_sided_jacobi(Matrix *matrix, double epsilon, int maxiter);
//восстановление матрицы из k-ых компонент svd-разложения
Matrix* svd_reconstruct(SVD *svd, int k);
//Применяем svd к 3 каналам
void YCbCr_SVD(YCbCrImage420 *img, int k);
//освобождение svd-структуры
void free_svdstate(SVD *result);
