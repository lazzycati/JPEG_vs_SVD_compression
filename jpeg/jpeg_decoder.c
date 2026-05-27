#include "jpeg_decoder.h"

JPEGDecoder* create_jpeg_decoder() 
{
    JPEGDecoder *decoder = (JPEGDecoder*)calloc(1, sizeof(JPEGDecoder));
    if (!decoder) return NULL;
    decoder->dcctx.prev_dcY = 0;
    decoder->dcctx.prev_dcCb = 0;
    decoder->dcctx.prev_dcCr = 0;
    decoder->current_dc[0] = 0;
    decoder->current_dc[1] = 0;
    decoder->current_dc[2] = 0;
    return decoder;
}

void destroy_jpeg_decoder(JPEGDecoder *decoder) 
{
    if (decoder) 
    {
        if (decoder->stream) 
        {
            free(decoder->stream);
        }
        free(decoder);
    }
}

JPEGFileData* jpeg_readfile(char *filename) 
{
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size <= 0) 
    {
        fclose(file);
        return NULL;
    }
    JPEGFileData *fdata = (JPEGFileData*)malloc(sizeof(JPEGFileData));
    fdata->data = (uint8_t*)malloc(size);
    fdata->size = size;
    int read = fread(fdata->data, 1, size, file);
    fclose(file);
    if (read != size) 
    {
        free(fdata->data);
        free(fdata);
        return NULL;
    }
    return fdata;
}

void free_datajpeg(JPEGFileData *fdata) 
{
    if (fdata) 
    {
        free(fdata->data);
        free(fdata);
    }
}

static uint16_t read_uint16(uint8_t *data) 
{
    return (data[0] << 8) | data[1];
}

static void parse_dqt(JPEGDecoder *decoder, uint8_t *data, int len) 
{
    int pos = 0;
    while (pos < len) 
    {
        uint8_t info = data[pos++];
        int table_ind = info & 0x0F;
        int precision = (info >> 4) & 0x0F;
        if (table_ind >= 4) 
        {
            if (precision == 0) pos += 64;
            else pos += 128;
            continue;
        }
        decoder->has_qt[table_ind] = 1;
        if (precision == 0) 
        {
            for (int i = 0; i < 64 && pos < len; i++) 
            {
                int row = zigzag[i] / 8;
                int col = zigzag[i] % 8;
                decoder->qt_tables[table_ind][row][col] = data[pos++];
            }
        } 
        else 
        {
            for (int i = 0; i < 64 && pos + 1 < len; i++) 
            {
                int row = zigzag[i] / 8;
                int col = zigzag[i] % 8;
                decoder->qt_tables[table_ind][row][col] = read_uint16(&data[pos]);
                pos += 2;
            }
        }
    }
}

static void parse_dht(JPEGDecoder *decoder, uint8_t *data, int len) 
{
    int pos = 0;
    while (pos < len) 
    {
        uint8_t info = data[pos++];
        int table_ind = info & 0x0F;
        int is_AC = (info >> 4) & 0x01;
        if (table_ind >= 4) break;
        HuffmanTable *table = is_AC ? &decoder->AC_tables[table_ind] : &decoder->DC_tables[table_ind];
        if (is_AC) decoder->has_ACtable[table_ind] = 1;
        else decoder->has_DCtable[table_ind] = 1;
        memset(table, 0, sizeof(HuffmanTable));
        int vals = 0;
        for (int i = 1; i <= 16 && pos < len; i++) 
        {
            table->bits[i] = data[pos++];
            vals += table->bits[i];
        }
        if (vals > 256) vals = 256;
        for (int i = 0; i < vals && pos < len; i++) table->huffval[i] = data[pos++];
        table->lastk = vals;
        generate_codes(table);
    }
}

static int find_next_marker(JPEGFileData *fdata, int start) 
{
    for (int i = start; i < fdata->size - 1; i++) 
    {
        if (fdata->data[i] == 0xFF) 
        {
            if (fdata->data[i + 1] == 0x00) 
            {
                i++;
                continue;
            }
            if (fdata->data[i + 1] == 0xFF) continue;
            return i;
        }
    }
    return 1;
}


int jpeg_parse_headers(JPEGDecoder *decoder, JPEGFileData *fdata) 
{
    if (!decoder || !fdata || fdata->size < 2) return -1;
    if (fdata->data[0] != 0xFF || fdata->data[1] != M_SOI) return 1;
    int pos = 2;
    while (pos < fdata->size - 1) 
    {
        if (fdata->data[pos] != 0xFF) pos++;
        uint8_t marker = fdata->data[pos + 1];
        if (marker == 0x00 || marker == M_SOI) pos += 2;
        if (marker == 0xFF) pos++;
        if (fdata->data[pos] != 0xFF || marker == 0x00 || marker == 0xFF) continue;
        else if (marker == M_EOI) break;
        else if (marker == M_APP0) 
        {
            int len = read_uint16(&fdata->data[pos + 2]);
            pos += len + 2;
        }
        else if (marker == M_DQT) 
        {
            int len = read_uint16(&fdata->data[pos + 2]);
            parse_dqt(decoder, &fdata->data[pos + 4], len - 2);
            pos += len + 2;
        }
        else if (marker == M_SOF0) 
        {
            int len = read_uint16(&fdata->data[pos + 2]);
            decoder->height = read_uint16(&fdata->data[pos + 5]);
            decoder->width = read_uint16(&fdata->data[pos + 7]);
            decoder->channels = fdata->data[pos + 9];
            for (int i = 0; i < decoder->channels && i < 3; i++) 
            {
                decoder->indch[i] = fdata->data[pos + 10 + i * 3];
                decoder->sampling[i] = fdata->data[pos + 11 + i * 3];
                decoder->qt_ind[i] = fdata->data[pos + 12 + i * 3];
            }
            pos += len + 2;
        }
        else if (marker == M_DHT) 
        {
            int len = read_uint16(&fdata->data[pos + 2]);
            parse_dht(decoder, &fdata->data[pos + 4], len - 2);
            pos += len + 2;
        }
        else if (marker == M_SOS) 
        {
            int headerl = read_uint16(&fdata->data[pos + 2]);
            int start = pos + headerl + 2;
            int end = fdata->size - 2;
            int eoipos = find_next_marker(fdata, start);
            while (eoipos >= 0) 
            {
                if (fdata->data[eoipos + 1] == M_EOI) 
                {
                    end = eoipos;
                    break;
                }
                break;
            }
            int dsize = end - start;
            if (dsize <= 0) return 1;
            decoder->stream = (BitStream*)malloc(sizeof(BitStream));
            decoder->stream->data = (uint8_t*)malloc(dsize);
            memcpy(decoder->stream->data, &fdata->data[start], dsize);
            decoder->stream->size = dsize;
            decoder->stream->capacity = dsize;
            decoder->stream->buffer = 0;
            decoder->stream->bitscount = 0;
            break;
        }
        else 
        {
            if (marker >= 0xC0 && marker <= 0xFE) 
            {
                int len = read_uint16(&fdata->data[pos + 2]);
                pos += len + 2;
            } 
            else pos += 2;
        }
    }
    return 0;
}

static void decode_block(JPEGDecoder *decoder, int *block, int channel, HuffmanTable *dc_table, HuffmanTable *ac_table) 
{
    memset(block, 0, 64 * sizeof(int));
    int category;
    if (huffman_decode_simb(decoder->stream, dc_table, &category) < 0) return;
    int diff = 0;
    if (category > 0) 
    {
        int val = read_bits(decoder->stream, category);
        if (val < 0) return;
        diff = revextrdiff(val, category);
    }
    decoder->current_dc[channel] += diff;
    block[0] = decoder->current_dc[channel];
    int pos = 1;
    while (pos < 64) 
    {
        int s;
        if (huffman_decode_simb(decoder->stream, ac_table, &s) < 0) break;
        if (s == 0x00) break; 
        int run = (s >> 4) & 0x0F;
        int size = s & 0x0F;
        if (run == 15 && size == 0) pos += 16;  
        else 
        {
            pos += run;
            if (pos < 64 && size > 0) 
            {
                int val = read_bits(decoder->stream, size);
                if (val < 0) break;
                block[pos] = revextrdiff(val, size);
                pos++;
            }
        }
    }
}

YCbCrImage* jpeg_decode_image(JPEGDecoder *decoder, char *filename, int quality) 
{
    if (!decoder || !decoder->stream) return NULL;
    int xblocks = (decoder->width + 7) / 8;
    int yblocks = (decoder->height + 7) / 8;
    int cbxblocks = (xblocks + 1) / 2;
    int cbyblocks = (yblocks + 1) / 2;
    printf("Decoding: %dx%d blocks (Y: %dx%d, CbCr: %dx%d)\n", decoder->width, decoder->height, xblocks, yblocks, cbxblocks, cbyblocks);
    JPEGImage *image = (JPEGImage*)malloc(sizeof(JPEGImage));
    image->width = decoder->width;
    image->height = decoder->height;
    image->xblocks = xblocks;
    image->yblocks = yblocks;
    image->cbxblocks = cbxblocks;
    image->cbyblocks = cbyblocks;
    image->Yblocks = (Block**)malloc(yblocks * sizeof(Block*));
    for (int i = 0; i < yblocks; i++) image->Yblocks[i] = (Block*)calloc(xblocks, sizeof(Block));
    image->Cbblocks = (Block**)malloc(cbyblocks * sizeof(Block*));
    for (int i = 0; i < cbyblocks; i++) image->Cbblocks[i] = (Block*)calloc(cbxblocks, sizeof(Block));
    image->Crblocks = (Block**)malloc(cbyblocks * sizeof(Block*));
    for (int i = 0; i < cbyblocks; i++) image->Crblocks[i] = (Block*)calloc(cbxblocks, sizeof(Block));
    HuffmanTable *dc_luma = &decoder->DC_tables[0];
    HuffmanTable *ac_luma = &decoder->AC_tables[0];
    HuffmanTable *dc_chroma = &decoder->DC_tables[1];
    HuffmanTable *ac_chroma = &decoder->AC_tables[1];
    int block[64];
    printf("Decoding Y blocks...\n");
    for (int y = 0; y < yblocks; y++) 
    {
        for (int x = 0; x < xblocks; x++) 
        {
            decode_block(decoder, block, 0, dc_luma, ac_luma);
            inverse_zigzag(block, &image->Yblocks[y][x]);
        }
    }
    printf("Decoding Cb blocks...\n");
    for (int y = 0; y < cbyblocks; y++) 
    {
        for (int x = 0; x < cbxblocks; x++) 
        {
            decode_block(decoder, block, 1, dc_chroma, ac_chroma);
            inverse_zigzag(block, &image->Cbblocks[y][x]);
        }
    }
    printf("Decoding Cr blocks...\n");
    for (int y = 0; y < cbyblocks; y++) 
    {
        for (int x = 0; x < cbxblocks; x++) 
        {
            decode_block(decoder, block, 2, dc_chroma, ac_chroma);
            inverse_zigzag(block, &image->Crblocks[y][x]);
        }
    }
    printf("Dequantizing...\n");
    dequantize_jpeg(image, quality);
    printf("Applying inverse DCT...\n");
    inverse_DCT(image);
    YCbCrImage420 *img420 = restore_from_blocks(image);
    free_jpeg(image);
    YCbCrImage *img = convert_from_sub420(img420);
    free_ycbcr420(img420);
    if (filename && img) store_ycbcr_to_bmp(filename, img);
    return img;
}

int jpeg_decode_file(char *input_file, char *output_file, int quality) 
{
    printf("Reading JPEG file: %s\n", input_file);
    JPEGFileData *fdata = jpeg_readfile(input_file);
    if (!fdata) return 1;
    JPEGDecoder *decoder = create_jpeg_decoder();
    if (!decoder) 
    {
        free_datajpeg(fdata);
        return 1;
    }
    if (jpeg_parse_headers(decoder, fdata) < 0) 
    {
        printf("Failed to parse JPEG headers\n");
        destroy_jpeg_decoder(decoder);
        free_datajpeg(fdata);
        return 1;
    }
    printf("Image: %dx%d\n", decoder->width, decoder->height);
    YCbCrImage *img = jpeg_decode_image(decoder, output_file, quality);
    destroy_jpeg_decoder(decoder);
    free_datajpeg(fdata);
    if (img) 
    {
        free_ycbcr_image(img);
        printf("Decoded successfully: %s\n", output_file);
        return 0;
    }
    return 1;
}