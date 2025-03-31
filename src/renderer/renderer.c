#include "renderer.h"
#include "window.h"
#include <math.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include "logger.h"

#define MAX_SCALE 64.f
#define MIN_SCALE 0.00390625f

typedef struct _renderer
{
    SDL_Renderer *handler;
    SDL_Texture *screen_texture;
    uint32_t *screen_buffer;
    uint32_t screen_buffer_size;
    uint16_t resolution_width, resolution_height;
    vec3f_t camera_pos;
    float camera_angle;
    float screen_dist;
    float *x_to_angle;
    int16_t *upper_clip;
    int16_t *lower_clip;
    float *depth_buffer; // TODO: Criar um vetor de flags tamanho 1 byte para indicar se é uma solid wall, upper, lower ou up_low
} rederer_t;

static rederer_t renderer = {
    .handler = NULL,
    .screen_texture = NULL,
    .screen_buffer = NULL,
    .screen_buffer_size = 0,
    .resolution_width = 0,
    .resolution_height = 0,
    .camera_pos = (vec3f_t){0, 0, 0},
    .screen_dist = 0,
    .x_to_angle = NULL,
    .upper_clip = NULL,
    .lower_clip = NULL,
    .depth_buffer = NULL,
};

float max_depth = 0.f;
float min_depth = 0.f;

static bool initialized = false;

#define WIDTH renderer.resolution_width
#define HEIGHT renderer.resolution_height
#define H_WIDTH (WIDTH / 2.f)
#define H_HEIGHT (HEIGHT / 2.f)

static bool r_init_screen()
{
    renderer.screen_buffer_size = WIDTH * HEIGHT * sizeof(uint32_t);
    renderer.screen_buffer = malloc(renderer.screen_buffer_size);

    if (renderer.screen_buffer != NULL)
    {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
        renderer.screen_texture = SDL_CreateTexture(renderer.handler, 
                                                    SDL_PIXELFORMAT_RGBA32, 
                                                    SDL_TEXTUREACCESS_STREAMING, 
                                                    WIDTH, 
                                                    HEIGHT);
        memset(renderer.screen_buffer, 0, renderer.screen_buffer_size);
        
        if (renderer.screen_texture != NULL)
            return true;

        free(renderer.screen_buffer);
    }

    DOOM_LOG_ERROR("Nao foi possivel iniciar o buffer de renderizacao");
    return false;
}

static bool r_create_tables()
{
    renderer.x_to_angle = (float*)malloc((WIDTH + 1) * sizeof(float));

    if (renderer.x_to_angle == NULL)
        return false;

    renderer.upper_clip = (int16_t*)malloc(WIDTH * sizeof(int16_t));

    if (renderer.upper_clip == NULL)
    {
        free(renderer.x_to_angle);
        return false;
    }

    renderer.lower_clip = (int16_t*)malloc(WIDTH * sizeof(int16_t));

    if (renderer.lower_clip == NULL)
    {
        free(renderer.x_to_angle);
        free(renderer.upper_clip);
        return false;
    }

    renderer.depth_buffer = (float*)malloc(WIDTH * HEIGHT * sizeof(float));

    if (renderer.depth_buffer == NULL)
    {
        free(renderer.x_to_angle);
        free(renderer.upper_clip);
        free(renderer.lower_clip);
        return false;
    }

    for (uint32_t i = 0; i <= WIDTH; i++)
        renderer.x_to_angle[i] = atanf(((H_WIDTH) - i) / renderer.screen_dist);

    return true;
}

bool r_init(uint16_t scrn_w, uint16_t scrn_h)
{
    SDL_Window *win = (SDL_Window*)w_get_handler();
    renderer.resolution_width = scrn_w;
    renderer.resolution_height = scrn_h;
    renderer.screen_dist = (float)(H_WIDTH) / tanf(H_FOV);
    
    if (!r_create_tables()) return false;

    renderer.handler = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

    if (renderer.handler == NULL)
    {
        DOOM_LOG_ERROR("Nao foi possivel criar o renderer SDL");
        return false;
    }

    if (!r_init_screen())
    {
        SDL_DestroyRenderer(renderer.handler);
        return false;
    }

    SDL_RenderSetLogicalSize(renderer.handler, WIDTH, HEIGHT);
    initialized = true;
    return true;
}

void r_begin_draw(const player_t *player)
{
    for (uint32_t i = 0; i < WIDTH * HEIGHT; i++)
    {
        renderer.screen_buffer[i] = 0;
        renderer.depth_buffer[i] = FLT_MAX;
    }

    min_depth = FLT_MAX;
    max_depth = 0.f;
    
    for (uint16_t i = 0; i < WIDTH; i++) 
    {
        renderer.upper_clip[i] = -1;
        renderer.lower_clip[i] = HEIGHT;
    }

    renderer.camera_pos = (vec3f_t){ player->position.x, player->position.y, player->position.z };
    renderer.camera_angle = player->angle;
}

void r_draw_pixel(int x, int y, uint32_t color)
{
    if (x < 0 || x > WIDTH || y < 0 || y > HEIGHT || (color & 0xFF000000) == 0) return;
    renderer.screen_buffer[WIDTH * y + x] = color;
}

void r_draw_vertical_line(int16_t x, int16_t y1, int16_t y2, const char *wall_texture, int16_t light_level, uint32_t color)
{
    if (y1 < 0) y1 = 0;
    if (y2 > HEIGHT) y2 = HEIGHT;
    if (x < 0 || x > WIDTH || y1 > HEIGHT || y2 < 0) return;

    for (uint32_t i = y1; i <= y2; i++)
        renderer.screen_buffer[WIDTH * i + x] = color; 
}

void r_draw_wall_col(const char *texture_name, float texture_column, int16_t x, int16_t y1, int16_t y2, float texture_alt, float inv_scale, int16_t light_level, float depth)
{
    image_t *texture = a_get_texture_by_name(texture_name);
    if (texture != NULL && y1 < y2)
    {
        int16_t col = (int16_t)(texture_column) % texture->width;
        float tex_y = texture_alt + ((float)y1 - H_HEIGHT) * inv_scale;

        for (uint16_t y = y1; y <= y2; y++)
        {
            uint32_t i = WIDTH * y + x;
            if (depth < renderer.depth_buffer[i])
                renderer.depth_buffer[i] = depth;


            uint32_t color = texture->data[((int16_t)tex_y % texture->height) * texture->width + col];
            renderer.screen_buffer[i] = color;
            tex_y += inv_scale;
        }
    }
}

void r_draw_flat(const char *texture_name, int16_t x, int16_t y1, int16_t y2, float world_z, int16_t light_level)
{
    if (strcmp(texture_name, "F_SKY1") == 0)
    {
        float tex_column = 2.2f * (renderer.camera_angle + renderer.x_to_angle[x]);
        r_draw_wall_col("SKY1", tex_column, x, y1, y2, 100.f, 1.5f, 1, FLT_MAX);
        return;
    }

    image_t *texture = a_get_flat_by_name(texture_name);
    if (texture == NULL) return;

    float player_dir_x = cosf(renderer.camera_angle);
    float player_dir_y = sinf(renderer.camera_angle);

    for (uint16_t y = y1; y <= y2; y++)
    {
        float z = H_WIDTH * world_z / (H_HEIGHT - y);
        float px = player_dir_x * z + renderer.camera_pos.x;
        float py = player_dir_y * z + renderer.camera_pos.y;

        float left_x = -player_dir_y * z + px;
        float left_y = player_dir_x * z + py;
        float right_x = player_dir_y * z + px;
        float right_y = -player_dir_x * z + py;

        float dx = (right_x - left_x) / WIDTH;
        float dy = (right_y - left_y) / WIDTH;

        int16_t tx = (int16_t)(left_x + dx * x) & 63;
        int16_t ty = (int16_t)(left_y + dy * x) & 63;

        uint32_t color = texture->data[ty * texture->width + tx];
        renderer.screen_buffer[y * WIDTH + x] = color;
    }
}

void r_draw_portal_wall_range(portal_wall_desc_t *portal_wall_desc)
{
    float rw_scale = r_scale_from_global_angle(portal_wall_desc->x1, portal_wall_desc->rw_normal_angle, portal_wall_desc->rw_distance);
    float rw_scale_step = 0;
    
    if (portal_wall_desc->x1 < portal_wall_desc->x2)
    {
        float scale2 = r_scale_from_global_angle(portal_wall_desc->x2, portal_wall_desc->rw_normal_angle, portal_wall_desc->rw_distance);
        rw_scale_step = (scale2 - rw_scale) / (portal_wall_desc->x2 - portal_wall_desc->x1);
    }

    float wall_y1 = H_HEIGHT - portal_wall_desc->world_front_z1 * rw_scale;
    float wall_y1_step = -rw_scale_step * portal_wall_desc->world_front_z1;

    float wall_y2 = H_HEIGHT - portal_wall_desc->world_front_z2 * rw_scale;
    float wall_y2_step = -rw_scale_step * portal_wall_desc->world_front_z2;
    
    float portal_y1 = wall_y2;
    float portal_y1_step = wall_y2_step;

    float portal_y2 = wall_y1;
    float portal_y2_step = wall_y1_step;

    if (portal_wall_desc->draw_upper_wall && portal_wall_desc->world_back_z1 > portal_wall_desc->world_front_z2)
    {
        portal_y1 = H_HEIGHT - portal_wall_desc->world_back_z1 * rw_scale;
        portal_y1_step = -rw_scale_step * portal_wall_desc->world_back_z1;
    }

    if (portal_wall_desc->draw_lower_wall && portal_wall_desc->world_back_z2 < portal_wall_desc->world_front_z1)
    {
        portal_y2 = H_HEIGHT - portal_wall_desc->world_back_z2 * rw_scale;
        portal_y2_step = -rw_scale_step * portal_wall_desc->world_back_z2;
    }

    float angle, texture_column, inv_scale;
    for (int16_t x = portal_wall_desc->x1; x < portal_wall_desc->x2; x++)
    {
        float depth = portal_wall_desc->rw_distance / cosf(portal_wall_desc->rw_normal_angle - renderer.x_to_angle[x] - renderer.camera_angle);

        float draw_wall_y1 = wall_y1 - 1;

        if (portal_wall_desc->draw_upper_wall || portal_wall_desc->draw_lower_wall)
        {
            angle = portal_wall_desc->rw_center_angle - renderer.x_to_angle[x];
            texture_column = portal_wall_desc->rw_distance * tanf(angle) - portal_wall_desc->rw_offset;
            inv_scale = 1.0f / rw_scale;
        }

        if (portal_wall_desc->draw_upper_wall)
        {
            float draw_upper_wall_y1 = wall_y1 - 1;

            if (portal_wall_desc->draw_ceil)
            {
                int16_t cy1 = renderer.upper_clip[x] + 1;
                int16_t cy2 = (int16_t)(fmin(draw_wall_y1 - 1, renderer.lower_clip[x] - 1));
                r_draw_flat(portal_wall_desc->ceil_texture, x, cy1, cy2, portal_wall_desc->world_front_z1, portal_wall_desc->light_level);
            }

            int16_t wy1 = (int16_t)(fmax(draw_upper_wall_y1, renderer.upper_clip[x] + 1));
            int16_t wy2 = (int16_t)(fmin(portal_y1, renderer.lower_clip[x] - 1));
            r_draw_wall_col(portal_wall_desc->upper_wall_texture, texture_column, x, wy1, wy2, portal_wall_desc->upper_tex_alt, inv_scale, portal_wall_desc->light_level, depth);

            if (renderer.upper_clip[x] < wy2)
                renderer.upper_clip[x] = wy2;

            portal_y1 += portal_y1_step;
        }

        if (portal_wall_desc->draw_ceil)
        {
            int16_t cy1 = renderer.upper_clip[x] + 1;
            int16_t cy2 = (int16_t)(fmin(draw_wall_y1 - 1, renderer.lower_clip[x] - 1));
            r_draw_flat(portal_wall_desc->ceil_texture, x, cy1, cy2, portal_wall_desc->world_front_z1, portal_wall_desc->light_level);

            if (renderer.upper_clip[x] < cy2)
                renderer.upper_clip[x] = cy2;
        }

        if (portal_wall_desc->draw_lower_wall)
        {
            if (portal_wall_desc->draw_floor)
            {
                int16_t fy1 = (int16_t)(fmax(wall_y2 + 1, renderer.upper_clip[x] + 1));
                int16_t fy2 = renderer.lower_clip[x] - 1;

                r_draw_flat(portal_wall_desc->floor_texture, x, fy1, fy2, portal_wall_desc->world_front_z2, portal_wall_desc->light_level);
            }

            float draw_lower_wall_y1 = portal_y2 - 1;

            int16_t wy1 = (int16_t)(fmax(draw_lower_wall_y1, renderer.upper_clip[x] + 1));
            int16_t wy2 = (int16_t)(fmin(wall_y2, renderer.lower_clip[x] - 1));
            r_draw_wall_col(portal_wall_desc->lower_wall_texture, texture_column, x, wy1, wy2, portal_wall_desc->lower_tex_alt, inv_scale, portal_wall_desc->light_level, depth);
            
            if (renderer.lower_clip[x] > wy1)
                renderer.lower_clip[x] = wy1;
            
            portal_y2 += portal_y2_step;
        }

        if (portal_wall_desc->draw_floor)
        {
            int16_t fy1 = (int16_t)(fmax(wall_y2 + 1, renderer.upper_clip[x] + 1));
            int16_t fy2 = renderer.lower_clip[x] - 1;
            r_draw_flat(portal_wall_desc->floor_texture, x, fy1, fy2, portal_wall_desc->world_front_z2, portal_wall_desc->light_level);

            if (renderer.lower_clip[x] > wall_y2 + 1)
                renderer.lower_clip[x] = fy1;
        }

        rw_scale += rw_scale_step;
        wall_y1 += wall_y1_step;
        wall_y2 += wall_y2_step;
    }
}

void r_draw_solid_wall_range(solid_wall_desc_t *solid_wall_desc)
{
    float rw_scale = r_scale_from_global_angle(solid_wall_desc->x1, solid_wall_desc->rw_normal_angle, solid_wall_desc->rw_distance);
    float rw_scale_step = 0;
    
    if (solid_wall_desc->x1 < solid_wall_desc->x2)
    {
        float scale2 = r_scale_from_global_angle(solid_wall_desc->x2, solid_wall_desc->rw_normal_angle, solid_wall_desc->rw_distance);
        rw_scale_step = (scale2 - rw_scale) / (solid_wall_desc->x2 - solid_wall_desc->x1);
    }

    float wall_y1 = H_HEIGHT - solid_wall_desc->world_front_z1 * rw_scale;
    float wall_y1_step = -rw_scale_step * solid_wall_desc->world_front_z1;

    float wall_y2 = H_HEIGHT - solid_wall_desc->world_front_z2 * rw_scale;
    float wall_y2_step = -rw_scale_step * solid_wall_desc->world_front_z2;
    
    for (int16_t x = solid_wall_desc->x1; x <= solid_wall_desc->x2; x++)
    {
        float depth = solid_wall_desc->rw_distance / cosf(solid_wall_desc->rw_normal_angle - renderer.x_to_angle[x] - renderer.camera_angle);

        float draw_wall_y1 = wall_y1 - 1;

        if (solid_wall_desc->draw_ceil)
        {
            int16_t cy1 = renderer.upper_clip[x] + 1;
            int16_t cy2 = (int16_t)(fmin(draw_wall_y1 - 1, renderer.lower_clip[x] - 1));
            r_draw_flat(solid_wall_desc->ceil_texture, x, cy1, cy2, solid_wall_desc->world_front_z1, solid_wall_desc->light_level);
        }

        if (solid_wall_desc->draw_wall && x < solid_wall_desc->x2)
        {
            int16_t wy1 = (int16_t)(fmax(draw_wall_y1, renderer.upper_clip[x] + 1));
            int16_t wy2 = (int16_t)(fmin(wall_y2, renderer.lower_clip[x] - 1));

            if (wy1 < wy2)
            {
                float angle = solid_wall_desc->rw_center_angle - renderer.x_to_angle[x];
                float texture_column = solid_wall_desc->rw_distance * tanf(angle) - solid_wall_desc->rw_offset;
                float inv_scale = 1.0f / rw_scale;
                r_draw_wall_col(solid_wall_desc->wall_texture, 
                    texture_column, 
                    x, wy1, wy2, 
                    solid_wall_desc->middle_texture_alt, 
                    inv_scale,
                    solid_wall_desc->light_level,
                    depth
                );
            }
        }

        if (solid_wall_desc->draw_floor)
        {
            int16_t fy1 = (int16_t)(fmax(wall_y2 + 1, renderer.upper_clip[x] + 1));
            int16_t fy2 = renderer.lower_clip[x] - 1;
            r_draw_flat(solid_wall_desc->floor_texture, x, fy1, fy2, solid_wall_desc->world_front_z2, solid_wall_desc->light_level);
        }

        rw_scale += rw_scale_step;
        wall_y1 += wall_y1_step;
        wall_y2 += wall_y2_step;
    }
}

void r_draw_sprite(int16_t x, int16_t z, image_t *sprite, float rw_scale, float rw_distance)
{
    float sprite_screen_height = sprite->height * rw_scale;
    float sprite_screen_width = sprite->width * rw_scale;
    float y_offset = H_HEIGHT - (z + sprite->top_offset) * rw_scale;
    float x_offset = x - sprite_screen_width / 2;

    if (x_offset < 0 || x_offset + sprite_screen_width > WIDTH || y_offset < 0 || y_offset + sprite_screen_height > HEIGHT || sprite->data == NULL) return;

    for (int16_t x = 0; x < sprite_screen_width; x++) 
    {
        for (int16_t y = 0; y < sprite_screen_height; y++)
        {
            uint32_t i = (uint32_t)(y_offset + y) * WIDTH + (uint32_t)(x_offset + x);
            if (rw_distance > renderer.depth_buffer[i])
                continue;

            // Mapeia para o espaço do sprite original
            int16_t tex_x = (x * sprite->width) / sprite_screen_width;
            int16_t tex_y = (y * sprite->height) / sprite_screen_height;

            uint32_t color = sprite->data[tex_y * sprite->width + tex_x];
            if (color != 0x00)
                renderer.screen_buffer[i] = color;
        }
    }
}

void r_end_draw()
{
    SDL_UpdateTexture(renderer.screen_texture, NULL, renderer.screen_buffer, WIDTH * sizeof(uint32_t));
    SDL_RenderCopy(renderer.handler, renderer.screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer.handler);
}

int16_t r_angle_to_x(float angle)
{
    return (angle > 0) ? (int16_t)(renderer.screen_dist - tanf(angle) * H_WIDTH) 
                                        : 
                        (int16_t)(-tanf(angle) * H_WIDTH + renderer.screen_dist);
}

float r_scale_from_global_angle(int16_t x, float normal_angle, float distance)
{
    float x_angle = renderer.x_to_angle[x];
    float num = renderer.screen_dist * cosf(normal_angle - x_angle - renderer.camera_angle);
    float den = distance * cosf(x_angle);

    float scale = num / den;
    scale = fmin(MAX_SCALE, fmax(MIN_SCALE, scale));
    return scale;
}

uint16_t r_get_width()
{
    return WIDTH;
}

uint16_t r_get_height()
{
    return HEIGHT;
}

void r_shutdown()
{
    if (initialized)
    {
        SDL_DestroyTexture(renderer.screen_texture);
        free(renderer.screen_buffer);
        free(renderer.x_to_angle);
        free(renderer.upper_clip);
        free(renderer.lower_clip);
        free(renderer.depth_buffer);
        SDL_DestroyRenderer(renderer.handler);
    }
}