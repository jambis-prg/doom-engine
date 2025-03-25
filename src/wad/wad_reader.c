#include "wad_reader.h"

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
    void *buffer = malloc(lump->size);

    if (buffer != NULL)
    {
        uint32_t offset = lump->filepos + header_len;

        fseek(wad_reader->file, offset, SEEK_SET);
        fread(buffer, lump->size, 1, wad_reader->file);
        *size = lump->size;
    }

    return buffer;
}

void wdr_close(wad_reader_t *wad_reader)
{
    if (wad_reader->file != NULL)
        fclose(wad_reader->file);

    if (wad_reader->directories != NULL)
        free(wad_reader->directories);

    fh_delete_hash(&wad_reader->file_hash);
}
