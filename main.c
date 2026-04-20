#include <stdio.h>
#include "loadbmp.h"
#include "basicmatrix.h"
#include "svd.h"
#include <stdlib.h>
#include "compression_svd.h"

typedef struct Matrix {
    double **data;
    int rows;
    int cols;
} Matrix;

int main() 
{
    Image *img = load_bmp("ava.bmp");
    if (!img) 
    {
        printf("Ошибка загрузки\n");
        return 1;
    }
    int m = img->height;
    int n = img->width;
    printf("Изображение: %dx%d\n", m, n);
    Matrix *A = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(A, m, n);
    image_to_matrix(A, img);
    printf("Выполнение SVD:\n");
    SVD *svd = svd_double_sided_jacobi(A, 1e-10, 100);
    int k = 30;
    save_svd_compressed_float("image_compressed_float.svd", svd, k, m, n);
    int l_m, l_n, l_k;
    SVD *l_svd = load_svd_compressed_float("image_compressed_float.svd", &l_m, &l_n, &l_k);
    save_reconstructedmatrix_to_bmp(l_svd, l_k, l_m, l_n, "reconstructed_from_float.bmp");
    print_compression_stats_float("image_compressed_float.svd", "enot.bmp", m, n, k);
    free_svdstate(l_svd);
    return 0;
}