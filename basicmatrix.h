#include "loadbmp.h"
#pragma once 

typedef struct {
    double **data;
    int rows;
    int cols;
} Matrix;

typedef struct {
    Matrix *U;      // левые сингулярные векторы 
    double *S;      // сингулярные числа 
    Matrix *V;      // правые сингулярные векторы 
} SVD;

void createMatrix(Matrix *matrix, int rows, int cols);
void freeMatrix(Matrix *matrix);
void image_to_matrix(Matrix *matrix, Image *img);
void matrix_to_image(Matrix *matrix, Image *img);
void identity_matrix(Matrix *matrix, int N);
void copy_matrix(Matrix *src, Matrix *dst);