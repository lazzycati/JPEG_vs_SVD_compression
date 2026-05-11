#pragma once
#include "basicmatrix.h"
#include "svd.h"
#include "subdiscretization.h"
#include "loadbmp.h"

void save_svd_compressed(char *filename, YCbCrSVD *svd, int rY, int rCb, int rCr);
YCbCrSVD* load_svd_compressed(char *filename, int *rY, int *rCb, int *rCr);
YCbCrImage420* reconstruct_from_svd(YCbCrSVD *svd, int kY, int kCb, int kCr);
void print_stats(char *svd_filename, char *bmp_filename, int m, int n, int k);
void free_ycbcr_svd(YCbCrSVD *svd);