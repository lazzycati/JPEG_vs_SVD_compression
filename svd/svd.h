#pragma once
#include "basicmatrix.h"

typedef struct SVD SVD;
//svd-разложение двусторонним якоби
SVD* svd_double_sided_jacobi(Matrix *matrix, double epsilon, int maxiter);
//восстановление матрицы из k-ых компонент svd-разложения
Matrix* svd_reconstruct(SVD *svd, int k);
//освобождение svd-структуры
void free_svdstate(SVD *result);
