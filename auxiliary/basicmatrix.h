#pragma once 
#include "loadbmp.h"
typedef struct Matrix {
    double **data;
    int rows;
    int cols;
} Matrix;

void createMatrix(Matrix *matrix, int rows, int cols);
void freeMatrix(Matrix *matrix);
void channel_to_matrix(Matrix *matrix, int width, int height, uint8_t *channel);
void matrix_to_channel(Matrix *matrix, int *width, int *height, uint8_t *channel);
void identityMatrix(Matrix *matrix, int N);
void copyMatrix(Matrix *src, Matrix *dst);