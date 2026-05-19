#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "compression_svd.h"
#include "metrics.h"
#include "jpeg_decoder.h"

int get_filesize(char *filename) 
{
    FILE *file = fopen(filename, "rb");
    if (!file) return 1;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    return size;
}

int main() 
{
    YCbCrImage *img1 = load_bmp_to_ycbcr("ava.bmp");
    if (!img1) 
    {
        printf("Ошибка загрузки bird.bmp\n");
        return 1;
    }
    int m = img1->height;
    int n = img1->width;
    printf("Изображение: %dx%d\n", m, n);
    YCbCrImage420 *img420 = convert_to_sub420(img1);
    if (!img420) 
    {
        printf("Ошибка конвертации в 4:2:0\n");
        free_ycbcr_image(img1);
        return 1;
    }
    YCbCrSVD *svd = (YCbCrSVD*)malloc(sizeof(YCbCrSVD));
    svd->yrows = img420->height;
    svd->ycols = img420->width;
    svd->crows = (img420->height + 1) / 2;
    svd->ccols = (img420->width + 1) / 2;
    printf("Выполнение SVD для Y канала (%dx%d)...\n", svd->yrows, svd->ycols);
    Matrix *Ymat = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(Ymat, svd->yrows, svd->ycols);
    channel_to_matrix(Ymat, svd->ycols, svd->yrows, img420->Y);
    svd->Y = svd_double_sided_jacobi(Ymat, 1e-10, 1000);
    freeMatrix(Ymat);
    printf("Выполнение SVD для Cb канала (%dx%d)...\n", svd->crows, svd->ccols);
    Matrix *Cbmat = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(Cbmat, svd->crows, svd->ccols);
    channel_to_matrix(Cbmat, svd->ccols, svd->crows, img420->Cb);
    svd->Cb = svd_double_sided_jacobi(Cbmat, 1e-10, 1000);
    freeMatrix(Cbmat);
    printf("Выполнение SVD для Cr канала (%dx%d)...\n", svd->crows, svd->ccols);
    Matrix *Crmat = (Matrix*)malloc(sizeof(Matrix));
    createMatrix(Crmat, svd->crows, svd->ccols);
    channel_to_matrix(Crmat, svd->ccols, svd->crows, img420->Cr);
    svd->Cr = svd_double_sided_jacobi(Crmat, 1e-10, 1000);
    freeMatrix(Crmat);
    int rY = 50;
    int rCb = 10;
    int rCr = 10;
    printf("Сохранение сжатого SVD (kY=%d, kCb=%d, kCr=%d)...\n", rY, rCb, rCr);
    save_svd_compressed("image_compressed.svd", svd, rY, rCb, rCr);
    int loaded_kY, loaded_kCb, loaded_kCr;
    YCbCrSVD *loaded_svd = load_svd_compressed("image_compressed.svd", &loaded_kY, &loaded_kCb, &loaded_kCr);
    if (!loaded_svd) 
    {
        printf("Ошибка загрузки SVD\n");
        free_ycbcr420(img420);
        free_ycbcr_svd(svd);
        free_ycbcr_image(img1);
        return 1;
    }
    printf("Восстановление изображения\n");
    YCbCrImage420 *reconstructed420 = reconstruct_from_svd(loaded_svd, loaded_kY, loaded_kCb, loaded_kCr);
    YCbCrImage *img2 = convert_from_sub420(reconstructed420);
    store_ycbcr_to_bmp("ava1.bmp", img2);
    print_stats("image_compressed.svd", "bird.bmp", m, n, rY);
    JPEGImage *jpeg_img = prep_for_dct(img420);
    if (!jpeg_img) 
    {
        printf("Ошибка разбиения на блоки\n");
        free_ycbcr420(img420);
        free_ycbcr_image(img1);
        return 1;
    }
    DCT(jpeg_img);
    JPEGEncoder *encoder = create_jpeg_encoder();
    if (!encoder) 
    {
        printf("Ошибка создания кодера\n");
        free_jpeg(jpeg_img);
        free_ycbcr420(img420);
        free_ycbcr_image(img1);
        return 1;
    }
    int jpeg_size = jpeg_write_file(encoder, jpeg_img, "output.jpg", 100);
    destroy_jpeg_encoder(encoder);
    if (jpeg_size > 0) 
    {
        printf("Урааа!! output.jpg создан (%d bytes)\n\n", jpeg_size);
    } 
    else 
    {
        printf("Ошибка: output.jpg не создан\n\n");
    }
    if (jpeg_decode_file("output.jpg", "ava2.bmp") == 0) 
    {
        printf("Ураа!!: ava2.bmp - восстановленное изобр.\n\n");
    } 
    else 
    {
        printf("Ошибки декодирования((\n\n");
        return 1;
    }
    YCbCrImage *img3 = load_bmp_to_ycbcr("ava2.bmp");
    int orig_size = get_filesize("ava.bmp");
    int compr_size = get_filesize("output.jpg");
    printf("Сжатие: %.2f:1\n", (double)orig_size / compr_size);
    double psnr1 = PSNR(img1, img2);
    double ssim1 = SSIM(img1, img2);
    double psnr2 = PSNR(img1, img3);
    double ssim2 = SSIM(img1, img3);
    printf("PSNR svd: %.2f dB\n", psnr1);
    printf("SSIM svd: %.4f\n", ssim1);
    printf("PSNR jpeg: %.2f dB\n", psnr2);
    printf("SSIM jpeg: %.4f\n", ssim2);
    free_ycbcr420(img420);
    free_ycbcr420(reconstructed420);
    free_ycbcr_image(img1);
    free_ycbcr_image(img2);
    free_ycbcr_svd(svd);
    free_ycbcr_svd(loaded_svd);
    printf("\nГотово!\n");
    return 0;
}
