#include "dpcm.h"

const int zigzag[64] = {
    0,  1,  8,  16, 9,  2,  3,  10,
    17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

void zigzag_to_block(Block *block, int *out) 
{
    for (int i = 0; i < 64; i++) 
    {
        int row = zigzag[i] / 8;
        int col = zigzag[i] % 8;
        out[i] = (int)round(block->coeff[row][col]);
    }
}

void inverse_zigzag(int *in, Block *block) 
{
    for (int i = 0; i < 64; i++) 
    {
        int row = zigzag[i] / 8;
        int col = zigzag[i] % 8;
        block->coeff[row][col] = (double)in[i];
    }
}

void dpcm_encode_block(int *block, int *prev_dc) 
{
    int diff = block[0] - *prev_dc;
    *prev_dc = block[0];
    block[0] = diff;
}

void dpcm_decode_block(int *block, int *prev_dc) 
{
    block[0] += *prev_dc;
    *prev_dc = block[0];
}

int computeCategory(int val) 
{
    int absval = val> 0? val: -val;
    int cat = 0;
    if (absval == 0) return 0;
    while (absval > 0) 
    {
        absval >>= 1;
        cat++;
    }
    return cat;
}

int extrdiff(int val, int cat) 
{
    if (cat == 0) return 0;
    int half = 1 << (cat - 1);
    if (val >= half) return val;
    else return val + (1 << cat) - 1; 
}

int revextrdiff(int val, int cat) 
{
    if (cat == 0) return 0;
    int half = 1 << (cat - 1);
    if (val >= half) return val;
    else return val - (1 << cat) + 1; 
}
