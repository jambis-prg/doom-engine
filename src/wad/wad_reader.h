#ifndef WAD_READER_H_INCLUDED
#define WAD_READER_H_INCLUDED

#include "typedefs.h"
#include "file_hash.h"
#include <stdio.h>


typedef struct _directory
{
    uint32_t filepos, size;
    char name[8];
} directory_t;


typedef struct _wad_reader
{
    char type[4];
    uint32_t lump_count, init_offset;
    directory_t *directories;
    file_hash_t file_hash;
    FILE *file;
} wad_reader_t;

wad_reader_t wdr_open(const char* filename);
void* wdr_get_lump_data(const wad_reader_t *wad_reader, uint32_t lump_index, uint32_t header_len, uint32_t *size);
void wdr_close(wad_reader_t *wad_reader);

#endif