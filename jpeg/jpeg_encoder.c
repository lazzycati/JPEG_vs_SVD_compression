#include "jpeg_encoder.h"

static void write_uint16(BitStream *stream, uint16_t val) 
{
    if (stream->size + 2 > stream->capacity) 
    {
        stream->capacity *= 2;
        stream->data = (uint8_t*)realloc(stream->data, stream->capacity);
    }
    stream->data[stream->size++] = (val >> 8) & 0xFF;
    stream->data[stream->size++] = val & 0xFF;
}

static void write_uint8(BitStream *stream, uint8_t val) 
{
    if (stream->size + 1 > stream->capacity) 
    {
        stream->capacity *= 2;
        stream->data = (uint8_t*)realloc(stream->data, stream->capacity);
    }
    stream->data[stream->size++] = val;
}

static void write_marker(BitStream *stream, uint16_t marker) 
{
    write_uint16(stream, marker);
}

JPEGEncoder* create_jpeg_encoder() 
{
    init_tables();
    JPEGEncoder *encoder = (JPEGEncoder*)malloc(sizeof(JPEGEncoder));
    encoder->stream = NULL;
    encoder->dcctx.prev_dcY = 0;
    encoder->dcctx.prev_dcCb = 0;
    encoder->dcctx.prev_dcCr = 0;
    return encoder;
}

void destroy_jpeg_encoder(JPEGEncoder *encoder) 
{
    if (encoder) 
    {
        if (encoder->stream) 
        {
            free(encoder->stream->data);
            free(encoder->stream);
        }
        free(encoder);
    }
}

static void init_bitstream(JPEGEncoder *encoder) 
{
    if (encoder->stream) 
    {
        free(encoder->stream->data);
        free(encoder->stream);
    }
    encoder->stream = (BitStream*)malloc(sizeof(BitStream));
    encoder->stream->capacity = 8192;
    encoder->stream->data = (uint8_t*)malloc(encoder->stream->capacity);
    encoder->stream->size = 0;
    encoder->stream->buffer = 0;
    encoder->stream->bitscount = 0;
}

static void write_jfif_header(BitStream *stream) 
{
    write_marker(stream, APP0);
    write_uint16(stream, 16);  // длина
    write_uint8(stream, 'J');
    write_uint8(stream, 'F');
    write_uint8(stream, 'I');
    write_uint8(stream, 'F');
    write_uint8(stream, 0);    // Null-terminator
    write_uint8(stream, 1);    // Основная версия
    write_uint8(stream, 2);    // Доп. версия
    write_uint8(stream, 1);    // Единицы измерения
    write_uint16(stream, 96);  // Плотность пикселей (96*96)
    write_uint16(stream, 96);  
    write_uint8(stream, 0);    // Информация о миниатюре (отсутствует)
    write_uint8(stream, 0);    
}

static void write_dqt(BitStream *stream, int quality) 
{
    int qt[8][8];
    write_marker(stream, DQT);
    write_uint16(stream, 67);  // Длина = 2 + 1 + 64
    write_uint8(stream, 0x00); // таблица 0, 8-bit
    scale_quantization_table(qt, luminance_qt, quality);
    for (int i = 0; i < 64; i++) 
    {
        int row = zigzag[i] / 8;
        int col = zigzag[i] % 8;
        int val = qt[row][col];
        if (val < 1) val = 1;
        if (val > 255) val = 255;
        write_uint8(stream, val);
    }
    write_marker(stream, DQT);
    write_uint16(stream, 67);
    write_uint8(stream, 0x01); // таблица 1, 8-bit
    scale_quantization_table(qt, chrominance_qt, quality);
    for (int i = 0; i < 64; i++) 
    {
        int row = zigzag[i] / 8;
        int col = zigzag[i] % 8;
        int val = qt[row][col];
        if (val < 1) val = 1;
        if (val > 255) val = 255;
        write_uint8(stream, val);
    }
}

//Инфа об изобр.
static void write_sof0(BitStream *stream, JPEGImage *image) 
{
    write_marker(stream, SOF0);
    write_uint16(stream, 17);  // Длина = 8 + 3 * 3
    write_uint8(stream, 24);    // Точность
    write_uint16(stream, image->height);
    write_uint16(stream, image->width);
    write_uint8(stream, 3);    // Число каналов
    // Y 
    write_uint8(stream, 1);    // ID
    write_uint8(stream, 0x22); // 4:2:0 субдискр.
    write_uint8(stream, 0);    // № QT table (0)
    // Cb 
    write_uint8(stream, 2);    
    write_uint8(stream, 0x11); 
    write_uint8(stream, 1);    
    // Cr 
    write_uint8(stream, 3);    
    write_uint8(stream, 0x11); 
    write_uint8(stream, 1);    
}

static void write_huffman_table(BitStream *stream, HuffmanTable *table, int is_AC, int tablenum) 
{
    int size = 2 + 1 + 16;  // длина + table info + 16 бит
    for (int i = 1; i <= 16; i++) size += table->bits[i];
    write_marker(stream, DHT);
    write_uint16(stream, size);
    uint8_t table_info = (is_AC ? 0x10 : 0x00) | (tablenum & 0x0F);
    write_uint8(stream, table_info);
    for (int i = 1; i <= 16; i++) write_uint8(stream, table->bits[i]);
    for (int i = 0; i < table->lastk; i++) write_uint8(stream, table->huffval[i]);
}

static void write_dht(BitStream *stream) 
{
    write_huffman_table(stream, &DC_lum, 0, 0);
    write_huffman_table(stream, &AC_lum, 1, 0);
    write_huffman_table(stream, &DC_chrom, 0, 1);
    write_huffman_table(stream, &AC_chrom, 1, 1);
}

static void write_sos(BitStream *stream) 
{
    write_marker(stream, SOS);
    write_uint16(stream, 12);  // Длина
    write_uint8(stream, 3);    // Кол-во каналов
    // Y
    write_uint8(stream, 1);
    write_uint8(stream, 0x00);  // DC table 0, AC table 0
    // Cb
    write_uint8(stream, 2);
    write_uint8(stream, 0x11);  // DC table 1, AC table 1
    // Cr
    write_uint8(stream, 3);
    write_uint8(stream, 0x11);  // DC table 1, AC table 1
    //спектральный диапазон
    write_uint8(stream, 0);   // Ss
    write_uint8(stream, 63);  // Se
    write_uint8(stream, 0);   // Ah/Al
}

static void encode_block(BitStream *stream, int *block, int *prev_dc, HuffmanTable *DCtable, HuffmanTable *ACtable) 
{
    dpcm_encode_block(block, prev_dc);
    int category = computeCategory(block[0]);
    huffman_encode_simb(stream, category, DCtable);
    if (category > 0) 
    {
        int encodedDC = extrdiff(block[0], category);
        write_bits(stream, encodedDC, category);
    }
    //RLE-кодирование
    int runz = 0;
    for (int i = 1; i < 64; i++) 
    {
        if (block[i] == 0) 
        {
            runz++;
            if (i == 63) huffman_encode_simb(stream, 0x00, ACtable);  
        } 
        else 
        {
            while (runz >= 16) 
            {
                huffman_encode_simb(stream, 0xF0, ACtable);  
                runz -= 16;
            }
            int ACcategory = computeCategory(block[i]);
            int symbol = (runz << 4) | ACcategory;
            huffman_encode_simb(stream, symbol, ACtable);
            if (ACcategory > 0) 
            {
                int val = extrdiff(block[i], ACcategory);
                write_bits(stream, val, ACcategory);
            }
            runz = 0;
        }
    }
}

void jpeg_encode_image(JPEGEncoder *encoder, JPEGImage *image, int quality) 
{
    if (!encoder || !image) return;
    init_bitstream(encoder);
    write_marker(encoder->stream, SOI);
    write_jfif_header(encoder->stream);
    write_dqt(encoder->stream, quality);
    write_sof0(encoder->stream, image);
    write_dht(encoder->stream);
    write_sos(encoder->stream);
    quantize_jpeg(image, quality);
    encoder->dcctx.prev_dcY = 0;
    encoder->dcctx.prev_dcCb = 0;
    encoder->dcctx.prev_dcCr = 0;
    int block[64];
    for (int y = 0; y < image->yblocks; y++) 
    {
        for (int x = 0; x < image->xblocks; x++) 
        {
            zigzag_to_block(&image->Yblocks[y][x], block);
            encode_block(encoder->stream, block, &encoder->dcctx.prev_dcY, &DC_lum, &AC_lum);
        }
    }
    for (int y = 0; y < image->cbyblocks; y++) 
    {
        for (int x = 0; x < image->cbxblocks; x++) 
        {
            zigzag_to_block(&image->Cbblocks[y][x], block);
            encode_block(encoder->stream, block, &encoder->dcctx.prev_dcCb, &DC_chrom, &AC_chrom);
        }
    }
    for (int y = 0; y < image->cbyblocks; y++) 
    {
        for (int x = 0; x < image->cbxblocks; x++) 
        {
            zigzag_to_block(&image->Crblocks[y][x], block);
            encode_block(encoder->stream, block, &encoder->dcctx.prev_dcCr, &DC_chrom, &AC_chrom);
        }
    }
    flush(encoder->stream);
    write_marker(encoder->stream, EOI);
}

int jpeg_write_file(JPEGEncoder *encoder, JPEGImage *image, char *filename, int quality) 
{
    if (!encoder || !image || !filename) return -1;
    jpeg_encode_image(encoder, image, quality);
    FILE *file = fopen(filename, "wb");
    if (!file) return 1;
    size_t written = fwrite(encoder->stream->data, 1, encoder->stream->size, file);
    fclose(file);
    if (written != (size_t)encoder->stream->size) return 1;
    printf("JPEG записан в: %s (занимает %d байт)\n", filename, encoder->stream->size);
    return encoder->stream->size;
}