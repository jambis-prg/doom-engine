#ifndef WAD_READER_H_INCLUDED
#define WAD_READER_H_INCLUDED

#include "typedefs.h"
#include "file_hash.h"
#include <stdio.h>

typedef enum _lump_indices
{
    ZERO_INDEX,
    ENTITIES_INDEX,
    LINEDEFS_INDEX,
    SIDEDEFS_INDEX,
    VERTEXES_INDEX,
    SEGS_INDEX,
    SUBSECTORS_INDEX,
    NODES_INDEX,
    SECTORS_INDEX,
    REJECT_INDEX,
    BLOCKMAP_INDEX,
} lump_indices_t;

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
void wdr_get_lump_header(const wad_reader_t *wad_reader, void *dst, uint32_t lump_index, uint32_t header_len);
texture_map_t *wdr_get_texture_map(const wad_reader_t *wad_reader, const texture_header_t *header, uint32_t lump_index);
void wdr_delete_texture_map(texture_map_t *texture_maps, uint32_t size);
patch_colum_t *wdr_get_patch_columns(const wad_reader_t *wad_reader, const patch_header_t *header, uint32_t lump_index, uint32_t *size_out);
void wdr_delete_patch_columns(patch_colum_t *patch_columns, uint32_t size);
void wdr_close(wad_reader_t *wad_reader);

#endif