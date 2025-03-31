#include "wad_reader.h"
#include <stddef.h>

wad_reader_t wdr_open(const char* filename)
{
    wad_reader_t wad_reader = {0};

    wad_reader.file = fopen(filename, "rb");

    if (wad_reader.file != NULL)
    {
        fread(&wad_reader.type, sizeof(wad_reader.type), 1, wad_reader.file);
        fread(&wad_reader.lump_count, sizeof(uint32_t), 1, wad_reader.file);
        fread(&wad_reader.init_offset, sizeof(uint32_t), 1, wad_reader.file);

        wad_reader.directories = (directory_t*)malloc(sizeof(directory_t) * wad_reader.lump_count);

        if (wad_reader.directories != NULL)
        {
            wad_reader.file_hash = fh_create_hash(wad_reader.lump_count * 2);
            uint32_t dir_size = sizeof(directory_t);
            for (uint32_t i = 0; i < wad_reader.lump_count; i++)
            {
                uint32_t offset = wad_reader.init_offset + i * dir_size;

                fseek(wad_reader.file, offset, SEEK_SET);
                fread(&wad_reader.directories[i].filepos, sizeof(uint32_t), 1, wad_reader.file);
                fread(&wad_reader.directories[i].size, sizeof(uint32_t), 1, wad_reader.file);
                fread(wad_reader.directories[i].name, sizeof(wad_reader.directories[i].name), 1, wad_reader.file);
                fh_insert_hash(&wad_reader.file_hash, wad_reader.directories[i].name, i);
            }
        }
        else
        {
            fclose(wad_reader.file);
            wad_reader.file = NULL;
        }
    }

    return wad_reader;
}

void *wdr_get_lump_data(const wad_reader_t *wad_reader, uint32_t lump_index, uint32_t header_len, uint32_t *size)
{
    directory_t *lump = &wad_reader->directories[lump_index];
    void *buffer = malloc(lump->size - header_len);

    if (buffer != NULL)
    {
        uint32_t offset = lump->filepos + header_len;

        fseek(wad_reader->file, offset, SEEK_SET);
        fread(buffer, (lump->size - header_len), 1, wad_reader->file);
        if (size != NULL)
            *size = (lump->size - header_len);
    }

    return buffer;
}

void wdr_get_lump_header(const wad_reader_t *wad_reader, void *dst, uint32_t lump_index, uint32_t header_len)
{
    directory_t *lump = &wad_reader->directories[lump_index];

    fseek(wad_reader->file, lump->filepos, SEEK_SET);
    fread(dst, header_len, 1, wad_reader->file);
}

texture_map_t *wdr_get_texture_map(const wad_reader_t *wad_reader, const texture_header_t *header, uint32_t lump_index)
{
    uint32_t texture_map_offset = wad_reader->directories[lump_index].filepos;
    texture_map_t *texture_maps = (texture_map_t *)malloc(header->texture_count * sizeof(texture_map_t));

    if (texture_maps != NULL)
    {
        for (uint32_t i = 0; i < header->texture_count; i++)
        {
            uint32_t offset = texture_map_offset + header->texture_data_offset[i];
            fseek(wad_reader->file, offset, SEEK_SET);
            fread(&texture_maps[i].name, sizeof(texture_maps[0].name), 1, wad_reader->file);
            fread(&texture_maps[i].flags, sizeof(texture_maps[0].flags), 1, wad_reader->file);
            fread(&texture_maps[i].width, sizeof(texture_maps[0].width), 1, wad_reader->file);
            fread(&texture_maps[i].height, sizeof(texture_maps[0].height), 1, wad_reader->file);
            fread(&texture_maps[i].column_dir, sizeof(texture_maps[0].column_dir), 1, wad_reader->file);
            fread(&texture_maps[i].patch_count, sizeof(texture_maps[0].patch_count), 1, wad_reader->file);
            
            texture_maps[i].patch_maps = (patch_map_t*)malloc(texture_maps[i].patch_count * sizeof(patch_map_t));
            
            if (texture_maps[i].patch_maps == NULL)
            {
                wdr_delete_texture_map(texture_maps, header->texture_count);
                return NULL;
            }

            fread(texture_maps[i].patch_maps, sizeof(patch_map_t), texture_maps[i].patch_count, wad_reader->file);
        }
    }

    return texture_maps;
}

void wdr_delete_texture_map(texture_map_t *texture_maps, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
        if (texture_maps[i].patch_maps != NULL)
            free(texture_maps[i].patch_maps);
    free(texture_maps);
}

patch_colum_t *wdr_get_patch_columns(const wad_reader_t *wad_reader, const patch_header_t *header, uint32_t lump_index, uint32_t *size_out)
{
    uint32_t patch_offset = wad_reader->directories[lump_index].filepos;
    uint32_t capacity = 2;
    uint32_t size = 0;
    patch_colum_t *patch_columns = (patch_colum_t *)malloc(capacity * sizeof(patch_colum_t));

    for (uint32_t i = 0; i < header->width; i++)
    {
        uint32_t offset = patch_offset + header->column_offset[i];
        while (true)
        {
            if (size >= capacity)
            {
                capacity *= 2;
                patch_colum_t *tmp = (patch_colum_t *)realloc(patch_columns, capacity * sizeof(patch_colum_t));

                if (tmp == NULL)
                {
                    wdr_delete_patch_columns(patch_columns, size);
                    return NULL;
                }

                patch_columns = tmp;
            }

            patch_colum_t *column = &patch_columns[size];
            fseek(wad_reader->file, offset, SEEK_SET);
            fread(&column->top_delta, sizeof(uint8_t), 1, wad_reader->file);
    
            if (column->top_delta != 0xFF)
            {
                // Lendo o length e o padding pre de uma vez só
                fread(&column->length, sizeof(uint8_t), 2, wad_reader->file);

                column->data = (uint8_t*)malloc(column->length);

                if (column->data == NULL)
                {
                    wdr_delete_patch_columns(patch_columns, size);
                    return NULL;
                }

                fread(column->data, sizeof(uint8_t), column->length, wad_reader->file);
                fread(&column->padding_post, sizeof(uint8_t), 1, wad_reader->file);
                offset += 4 + column->length;
                size++;
            }
            else
            {
                column->data = NULL;
                offset++;
                size++;
                break;
            }
        }
    }

    patch_colum_t *tmp = realloc(patch_columns, size * sizeof(patch_colum_t));
    if (tmp == NULL)
    {
        wdr_delete_patch_columns(patch_columns, size);
        patch_columns = NULL;
        size = 0;
    }
    else
        patch_columns = tmp;

    *size_out = size;
    return patch_columns;
}

void wdr_delete_patch_columns(patch_colum_t *patch_columns, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
        if (patch_columns[i].data != NULL)
            free(patch_columns[i].data);
    free(patch_columns);
}

void wdr_close(wad_reader_t *wad_reader)
{
    if (wad_reader->file != NULL)
        fclose(wad_reader->file);

    if (wad_reader->directories != NULL)
        free(wad_reader->directories);

    fh_delete_hash(&wad_reader->file_hash);
}
