#include "loadbmp.h"
#pragma once 

typedef struct Matrix Matrix;

void createMatrix(Matrix *matrix, int rows, int cols);
void freeMatrix(Matrix *matrix);
void image_to_matrix(Matrix *matrix, Image *img);
void matrix_to_image(Matrix *matrix, Image *img);
void identityMatrix(Matrix *matrix, int N);
void copyMatrix(Matrix *src, Matrix *dst);