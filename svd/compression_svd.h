#pragma once
#include "basicmatrix.h"
#include "svd.h"

void save_reconstructedmatrix_to_bmp(SVD *svd, int k, int m, int n, char *filename);
void save_svd_compressed(char *filename, SVD *svd, int k, int m, int n);
SVD* load_svd_compressed(char *filename, int *m, int *n, int *k);
void print_compression_stats(char *svd_filename, char *bmp_filename, int m, int n, int k);
void print_compression_stats_float(char *svd_filename, char *bmp_filename, int m, int n, int k);
SVD* load_svd_compressed_float(char *filename, int *m, int *n, int *k);
void save_svd_compressed_float(char *filename, SVD *svd, int k, int m, int n);