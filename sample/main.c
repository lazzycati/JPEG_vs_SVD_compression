#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "compression_svd.h"
#include "metrics.h"
#include "jpeg_decoder.h"

void WriteHeader(char *filename) 
{
    FILE *csv = fopen(filename, "w");
    if (!csv) 
    {
        printf("Ошибка создания CSV файла\n");
        return;
    }
    fprintf(csv, "method,param_name,param_val,compr_ratio,psnr,ssim,time\n");
    fclose(csv);
}

void AddstrCsv(char *filename, char *method, char* param_name, double param_val, 
    double compr_ratio, double psnr, double ssim, double time) 
{
    FILE* csv = fopen(filename, "a");
    if (!csv) 
    {
        printf("Ошибка открытия CSV для записи\n");
        return;
    }
    fprintf(csv, "%s,%s,%.2f,%.4f,%.2f,%.6f,%.2f\n",  
        method, param_name, param_val, compr_ratio, psnr, ssim, time);
    fclose(csv);
}

int get_filesize(char *filename) 
{
    FILE *file = fopen(filename, "rb");
    if (!file) return 1;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    return size;
}

void SVDTest(char *inputbmp, char *csvfile) 
{
    YCbCrImage *img1 = load_bmp_to_ycbcr(inputbmp);
    if (!img1) 
    {
        printf("Ошибка загрузки %s\n", inputbmp);
        return;
    }
    YCbCrImage420 *img420 = convert_to_sub420(img1);
    if (!img420) 
    {
        printf("Ошибка конвертации в 4:2:0\n");
        free_ycbcr_image(img1);
        return;
    }
    int yrows = img420->height;
    int ycols = img420->width;
    int crows = (img420->height + 1) / 2;
    int ccols = (img420->width + 1) / 2;
    printf("Выполнение SVD разложения для Y канала\n");
    Matrix *Ymat = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(Ymat, yrows, ycols);
    channel_to_matrix(Ymat, ycols, yrows, img420->Y);
    SVD *svdY = svd_double_sided_jacobi(Ymat, 1e-10, 1000);
    freeMatrix(Ymat);
    printf("Выполнение SVD разложения для Cb канала\n");
    Matrix *Cbmat = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(Cbmat, crows, ccols);
    channel_to_matrix(Cbmat, ccols, crows, img420->Cb);
    SVD *svdCb = svd_double_sided_jacobi(Cbmat, 1e-10, 1000);
    freeMatrix(Cbmat);
    printf("Выполнение SVD разложения для Cr канала\n");
    Matrix *Crmat = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(Crmat, crows, ccols);
    channel_to_matrix(Crmat, ccols, crows, img420->Cr);
    SVD *svdCr = svd_double_sided_jacobi(Crmat, 1e-10, 1000);
    freeMatrix(Crmat);
    int k_values[] = {5, 10, 20, 30, 50, 80, 100, 150, 200};
    int num_k = sizeof(k_values) / sizeof(k_values[0]);
    int orig_size = get_filesize(inputbmp);
    for (int i = 0; i < num_k; i++) 
    {
        int kY = k_values[i];
        int kCb = (kY / 5);
        int kCr = (kY / 5);
        printf("\n SVD с kY=%d, kCb=%d, kCr=%d \n", kY, kCb, kCr);
        clock_t start = clock();
        YCbCrSVD *svd = (YCbCrSVD*)malloc(sizeof(YCbCrSVD));
        svd->yrows = yrows;
        svd->ycols = ycols;
        svd->crows = crows;
        svd->ccols = ccols;
        svd->Y = svdY;
        svd->Cb = svdCb;
        svd->Cr = svdCr;
        YCbCrImage420 *rec420 = reconstruct_from_svd(svd, kY, kCb, kCr);
        YCbCrImage *recimg = convert_from_sub420(rec420);
        clock_t end = clock();
        double time = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        char outbmp[256];
        sprintf(outbmp, "temp_svd_k%d.bmp", kY);
        store_ycbcr_to_bmp(outbmp, recimg);
        save_svd_compressed("temp_svd.svd", svd, kY, kCb, kCr);
        int compr_size = get_filesize("temp_svd.svd");
        double psnr = PSNR(img1, recimg);
        double ssim = SSIM(img1, recimg);
        double compr_ratio = (double)orig_size / compr_size;
        printf("  Коэффициент сжатия: %.2f:1\n", compr_ratio);
        printf("  PSNR: %.2f dB\n", psnr);
        printf("  SSIM: %.4f\n", ssim);
        printf("  Время: %.2f ms\n", time);
        AddstrCsv(csvfile, "SVD", "k", kY, compr_ratio, psnr, ssim, time);
        remove("temp_svd.svd");
        remove(outbmp);
        free_ycbcr420(rec420);
        free_ycbcr_image(recimg);
        free(svd);
    }
    free_ycbcr420(img420);
    free_ycbcr_image(img1);
}

void JPEGTest(char *inputbmp, char *csvfile) 
{
    YCbCrImage *img1 = load_bmp_to_ycbcr(inputbmp);
    if (!img1) 
    {
        printf("Ошибка загрузки %s\n", inputbmp);
        return;
    }
    YCbCrImage420 *img420 = convert_to_sub420(img1);
    if (!img420) 
    {
        printf("Ошибка конвертации в 4:2:0\n");
        free_ycbcr_image(img1);
        return;
    }
    int q_values[] = {10, 30, 50, 70, 80, 90, 95, 100};
    int num_q = sizeof(q_values) / sizeof(q_values[0]);
    int orig_size = get_filesize(inputbmp);
    for (int i = 0; i < num_q; i++) 
    {
        int q = q_values[i];
        printf("\nJPEG с качеством %d \n", q);
        JPEGImage *jpegimg = prep_for_dct(img420);
        if (!jpegimg) 
        {
            printf("Ошибка разбиения на блоки для качества %d\n", q);
            continue;
        }
        DCT(jpegimg);
        JPEGEncoder *encoder = create_jpeg_encoder();
        if (!encoder) 
        {
            printf("Ошибка создания кодера\n");
            free_jpeg(jpegimg);
            continue;
        }
        clock_t start = clock();
        char jpeg_file[256];
        sprintf(jpeg_file, "temp_q%d.jpg", q);
        jpeg_write_file(encoder, jpegimg, jpeg_file, q);
        int jpeg_size = get_filesize(jpeg_file);
        clock_t end = clock();
        double time = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        destroy_jpeg_encoder(encoder);
        if (jpeg_size <= 0) 
        {
            printf("Ошибка: JPEG не создан для качества %d\n", q);
            free_jpeg(jpegimg);
            continue;
        }
        char decoded_bmp[256];
        sprintf(decoded_bmp, "temp_jpeg_q%d.bmp", q);
        if (jpeg_decode_file(jpeg_file, decoded_bmp, q) != 0) 
        {
            printf("Ошибка декодирования JPEG для качества %d\n", q);
            free_jpeg(jpegimg);
            remove(jpeg_file);
            continue;
        }
        YCbCrImage *recimg = load_bmp_to_ycbcr(decoded_bmp);
        if (!recimg) 
        {
            printf("Ошибка загрузки восстановленного BMP для качества %d\n", q);
            free_jpeg(jpegimg);
            remove(jpeg_file);
            remove(decoded_bmp);
            continue;
        }
        double psnr = PSNR(img1, recimg);
        double ssim = SSIM(img1, recimg);
        double compr_ratio = (double)orig_size / jpeg_size;
        printf("  Коэффициент сжатия: %.2f:1\n", compr_ratio);
        printf("  PSNR: %.2f dB\n", psnr);
        printf("  SSIM: %.4f\n", ssim);
        printf("  Время: %.2f ms\n", time);
        printf("  Размер: %d -> %d байт\n", orig_size, jpeg_size);
        AddstrCsv(csvfile, "JPEG", "quality", q, compr_ratio, psnr, ssim, time);
        free_ycbcr_image(recimg);
        free_jpeg(jpegimg);
        remove(jpeg_file);
        remove(decoded_bmp);
    }
    free_ycbcr420(img420);
    free_ycbcr_image(img1);
}

int main() 
{
    char *csvfile = "compr_results.csv";
    char *origbmp = "ava.bmp";
    WriteHeader(csvfile);
    printf("Сравнение сжатия: SVD vs JPEG\n");
    SVDTest(origbmp, csvfile);
    JPEGTest(origbmp, csvfile);
    return 0;
}