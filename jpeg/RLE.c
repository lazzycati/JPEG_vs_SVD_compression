#include "RLE.h"

void zigzag(int block[8][8], int output[64])
{
    for (int i = 0; i < 64; i++) 
    {
        int pos = zigzagtable[i];
        int row = pos / 8;
        int col = pos % 8;
        output[i] = block[row][col];
    }
}

void encode_ACblock(BitStream *bs, int block[8][8], int is_lum) 
{
    int zvalues[64];
    zigzag(block, zvalues);
    int zerocount = 0;
    for (int i = 1; i < 64; i++) 
    {
        int val = zvalues[i];
        if (val == 0) zerocount++;  
        else 
        {
            // Если накопилось 16 или больше нулей - маркер ZRL
            while (zerocount >= 16) 
            {
                // ZRL = (15,0) = 0xF0, записываем хаффман-код ZRL:
                write_bits(bs, AChuffman_code(0xF0, is_lum), AChuffman_length(0xF0, is_lum));
                zerocount -= 16;
            }
            // Формируем RLE символ: (сколько нулей перед значением << 4) (старший ниббл) | категория (младший ниббл)
            int category = getbits(val);
            int rle_symbol = (zerocount << 4) | category;
            // Пишем Хаффман код для символа
            write_bits(bs, AChuffman_code(rle_symbol, is_lum), AChuffman_length(rle_symbol, is_lum));
            // Пишем само значение (category бит)
            int encval = encode_diff(val, category);
            write_bits(bs, encval, category);
            zerocount = 0;  
        }
    }
    //В конце блока - EOB (End of Block)
    write_bits(bs, AChuffman_code(0x00, is_lum), AChuffman_length(0x00, is_lum));
}

void encode_ACcomponents(JPEGImage *jpeg, BitStream *bs) 
{
    printf("\n=== Кодирование AC компонент ===\n");
    int block[8][8];
    printf("Кодирование AC для Y...\n");
    for (int by = 0; by < jpeg->yblocks; by++) 
    {
        for (int bx = 0; bx < jpeg->xblocks; bx++) 
        {
            for (int i = 0; i < 8; i++) 
            {
                for (int j = 0; j < 8; j++) 
                {
                    block[i][j] = (int)jpeg->Yblocks[by][bx].coeff[i][j];
                }
            }
            encode_ACblock(bs, block, 1);  
        }
    }
    printf("Кодирование AC для Cb и Cr...\n");
    for (int by = 0; by < jpeg->cbyblocks; by++)
     {
        for (int bx = 0; bx < jpeg->cbxblocks; bx++) 
        {
            for (int i = 0; i < 8; i++) 
            {
                for (int j = 0; j < 8; j++) 
                {
                    block[i][j] = (int)jpeg->Cbblocks[by][bx].coeff[i][j];
                }
            }
            encode_ACblock(bs, block, 0);  
            for (int i = 0; i < 8; i++) 
            {
                for (int j = 0; j < 8; j++) 
                {
                    block[i][j] = (int)jpeg->Crblocks[by][bx].coeff[i][j];
                }
            }
            encode_ACblock(bs, block, 0);
        }
    }
}