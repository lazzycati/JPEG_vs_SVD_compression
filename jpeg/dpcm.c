#include "dpcm.h"

int getbits(int diff) 
{
    int d = (diff < 0) ? -diff : diff;
    if (d == 0) return 0;
    if (d == 1) return 1;
    if (d <= 3) return 2;
    if (d <= 7) return 3;
    if (d <= 15) return 4;
    if (d <= 31) return 5;
    if (d <= 63) return 6;
    if (d <= 127) return 7;
    if (d <= 255) return 8;
    if (d <= 511) return 9;
    if (d <= 1023) return 10;
    return 11;
}

int encode_diff(int diff, int bits) 
{
    if (bits == 0) return 0;  
    if (diff > 0) return diff;
    else return (-diff) - 1;
}

void encode_DCcomponents(JPEGImage *jpeg, DCContext *ctx, BitStream *bs, FILE *output) 
{
    printf("Кодирование DC для Y компонента...\n");
    for (int y = 0; y < jpeg->yblocks; y++) 
    {
        for (int x = 0; x < jpeg->xblocks; x++) 
        {
            Block *curblock = &jpeg->Yblocks[y][x];
            // DC-коэффициент - самый первый элемент в блоке (0,0)
            int cur_dc = (int)curblock->coeff[0][0];
            // Кодируем DC-коэффициент
            int diff = cur_dc - ctx->prev_dcY;
            ctx->prev_dcY = cur_dc;
            huffman_encodeDC(bs, diff, 1);
        }
    }
    printf("\nКодирование DC для Cb и Cr компонент...\n");
    for (int y = 0; y < jpeg->cbyblocks; y++) 
    {
        for (int x = 0; x < jpeg->cbxblocks; x++) 
        {
            Block *curblock1 = &jpeg->Cbblocks[y][x];
            int cur_dc = (int)curblock1->coeff[0][0];
            int diff = cur_dc - ctx->prev_dcCb;
            ctx->prev_dcCb = cur_dc;
            huffman_encodeDC(bs, diff, 0);
            Block *curblock2 = &jpeg->Crblocks[y][x];
            int cur_dc = (int)curblock2->coeff[0][0];
            int diff = cur_dc - ctx->prev_dcCr;
            ctx->prev_dcCr = cur_dc;
            huffman_encodeDC(bs, diff, 0);
        }
    }
}