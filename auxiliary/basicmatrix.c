#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "basicmatrix.h"
#include <math.h>

void createMatrix(Matrix *matrix, int rows, int cols) 
{
    matrix->rows = rows;
    matrix->cols = cols;
    matrix->data = (double**)malloc(rows * sizeof(double*));
    if (!matrix->data) 
    {
        free(matrix);
        return;
    }
    for (int i = 0; i < rows; i++) 
    {
        matrix->data[i] = (double*)calloc(cols, sizeof(double));
        if (!matrix->data[i]) 
        {
            for (int j = 0; j < i; j++) free(matrix->data[j]);
            free(matrix->data);
            free(matrix);
            return;
        }
    }
}

void freeMatrix(Matrix *matrix) 
{
    if (!matrix) return;
    for (int i = 0; i < matrix->rows; i++) free(matrix->data[i]);
    free(matrix->data);
    free(matrix);
}

void channel_to_matrix(Matrix *matrix, int width, int height, uint8_t *channel) 
{
    for (int i = 0; i < height; i++) 
    {
        for (int j = 0; j < width; j++) 
        {
            matrix->data[i][j] = (double)channel[i * width + j];
        }
    }
}

void matrix_to_channel(Matrix *matrix, int *width, int *height, uint8_t *channel) 
{
    *width = matrix->cols;
    *height = matrix->rows;
    int tpixels = matrix->rows * matrix->cols;
    channel = (uint8_t*)malloc(tpixels * sizeof(uint8_t));
    if (!channel) return;
    for (int i = 0; i < matrix->rows; i++) 
    {
        for (int j = 0; j < matrix->cols; j++) 
        {
            double val = matrix->data[i][j];
            if (val < 0) val = 0;
            if (val > 255) val = 255;
            channel[i * matrix->cols + j] = (uint8_t)val;
        }
    }
}

void identityMatrix(Matrix *matrix, int N) 
{
    for (int i = 0; i < N; i++) matrix->data[i][i] = 1.0;
}

void copyMatrix(Matrix *src, Matrix *dst) 
{
    for (int i = 0; i < src->rows; i++) 
    {
        for (int j = 0; j < src->cols; j++) 
        {
            dst->data[i][j] = src->data[i][j];
        }
    }
}
