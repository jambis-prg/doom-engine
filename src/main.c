#include "core/game_core.h"
#include "wad/wad_reader.h"
#include "logger.h"

int main()
{
    if (g_init(1280, 960))
    {
        wad_reader_t wad_reader = wdr_open("resources/DOOM1.WAD");

        uint32_t value = 0;
        if (fh_get_value(&wad_reader.file_hash, "E1M1", &value))
        {
            uint32_t size = 0;
            linedef_t *buffer = wdr_get_lump_data(&wad_reader, value + 2, 0, &size);
            if (buffer != NULL)
            {
                size /= sizeof(linedef_t);

                for (uint32_t i = 0; i < size; i++)
                    DOOM_LOG_DEBUG("[%d, %d, %d, %d, %d, %d, %d]", buffer[i].start_vertex, buffer[i].end_vertex, buffer[i].flags, buffer[i].line_type, buffer[i].sector_tag, buffer[i].front_sidedef_id, buffer[i].back_sidedef_id);

                free(buffer);
            }
        }
        
        wdr_close(&wad_reader);

        g_run();

        g_shutdown();
    }
    return 0;
}