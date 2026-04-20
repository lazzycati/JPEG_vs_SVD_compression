#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "loadbmp.h"

#pragma pack(push, 1) 
typedef struct {
    uint16_t bfType;            // "BM" (0x4D42)
    uint32_t bfSize;
    uint16_t bfReserved1;       // const = 0
    uint16_t bfReserved2;       // const = 0
    uint32_t bfOffBits;         // смещение до пикс. данных 
} BMPFileHeader;

typedef struct {
    uint32_t biSize;            // struct size
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;          // const = 1
    uint16_t biBitCount;        // Бит/пиксель (True color: 24; 8-bit: требует палитры)
    uint32_t biCompression;     // 0 (без сжатия)
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;   // Разрешение по X
    int32_t  biYPelsPerMeter;   // Разрешение по Y
    uint32_t biClrUsed;         // Используемые цвета (0 для True color)
    uint32_t biClrImportant;    // Важные цвета (0 = все)
} BMPInfoHeader;
#pragma pack(pop)

Image* load_bmp(char *filename) 
{
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;
    BMPFileHeader fheader;
    BMPInfoHeader infoheader;
    fread(&fheader, sizeof(BMPFileHeader), 1, file);
    fread(&infoheader, sizeof(BMPInfoHeader), 1, file);
    if (fheader.bfType != 0x4D42) 
    {
        printf("Файл не BMP\n");
        fclose(file);
        return NULL;
    }
    if (infoheader.biBitCount != 24) 
    {
        printf("Работа только с 24-битными BMP. Текущий формат: %d бит\n", infoheader.biBitCount);
        fclose(file);
        return NULL;
    }
    if (infoheader.biCompression != 0) 
    {
        printf("Необходим не сжатый BMP\n");
        fclose(file);
        return NULL;
    }
    Image *img = (Image*)malloc(sizeof(Image));
    if (!img) 
    {
        fclose(file);
        return NULL;
    }
    int width = infoheader.biWidth;
    int height = abs(infoheader.biHeight);  
    img->width = width;
    img->height = height;
    img->data = (uint8_t*)malloc(width * height);
    if (!img->data) 
    {
        free(img);
        fclose(file);
        return NULL;
    }
    //upside_down - влаг, первернуто ли img
    int upside_down = (infoheader.biHeight < 0);  
    // для bmp каждая строка пикселей выравнена и кратна 4 
    int rowSize = ((width * 3 + 3) & ~3);
    uint8_t *row = (uint8_t*)malloc(rowSize);
    if (!row) 
    {
        fclose(file);
        return NULL;
    }
    fseek(file, fheader.bfOffBits, SEEK_SET);
    for (int i = 0; i < height; i++) 
    {
        fread(row, 1, rowSize, file);
        int dest;
        if (upside_down) dest = height - 1 - i;
        else dest = i;
        for (int j = 0; j < width; j++) 
        {
            // BGR порядок
            uint8_t b = row[j * 3 + 0];
            uint8_t g = row[j * 3 + 1];
            uint8_t r = row[j * 3 + 2];
            // Формула яркости (ITU-R BT.601)
            // Y = 0.299*R + 0.587*G + 0.114*B
            uint8_t gray = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
            img->data[dest * width + j] = gray;
        }
    }
    free(row);
    fclose(file);
    return img;
}

int store_to_bmp(char *filename, Image *img) 
{
    FILE* file = fopen(filename, "wb");
    if (!file) 
    {
        printf("Не удалось создать файл %s\n", filename);
        return 0;
    }
    int width = img->width;
    int height = img->height;
    int rowSize = ((width * 3 + 3) & ~3);
    int imageSize = rowSize * height;
    BMPFileHeader fheader = {0};
    fheader.bfType = 0x4D42; 
    fheader.bfSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + imageSize;
    fheader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
    BMPInfoHeader infoheader = {0};
    infoheader.biSize = sizeof(BMPInfoHeader);
    infoheader.biWidth = width;
    infoheader.biHeight = height;  
    infoheader.biPlanes = 1;
    infoheader.biBitCount = 24;
    infoheader.biCompression = 0;  
    infoheader.biSizeImage = imageSize;
    fwrite(&fheader, sizeof(BMPFileHeader), 1, file);
    fwrite(&infoheader, sizeof(BMPInfoHeader), 1, file);
    uint8_t *row = (uint8_t*)calloc(rowSize, 1);  
    if (!row) 
    {
        fclose(file);
        return 0;
    }
    if (infoheader.biHeight < 0)
    {
        for (int i = height - 1; i >= 0; i--) 
        {
            for (int j = 0; j < width; j++) 
            {
                uint8_t gray = img->data[i * width + j];
                // BGR:
                row[j * 3 + 0] = gray;  
                row[j * 3 + 1] = gray;  
                row[j * 3 + 2] = gray;  
            }
            fwrite(row, 1, rowSize, file);
        }
    }
    else
    {
        for (int i = 0; i < height; i++) 
        {
            for (int j = 0; j < width; j++) 
            {
                uint8_t gray = img->data[i * width + j];
                row[j * 3 + 0] = gray;  
                row[j * 3 + 1] = gray;  
                row[j * 3 + 2] = gray;  
            }
            fwrite(row, 1, rowSize, file);
        }
    }
    free(row);
    fclose(file);
    printf("Изображение сохранено как %s\n", filename);
    return 1;
}

void free_image(Image *img) 
{
    if (img) 
    {
        if (img->data) free(img->data);
        free(img);
    }
}
