#include "renderer.h"
#include "window.h"
#include <math.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include "logger.h"



#define FOV 200

typedef enum _surface_type { NONE = 0, FLOOR = 1, CEIL = 2 } surface_type_t;

static SDL_Renderer *renderer = NULL;
static SDL_Texture *screen_texture = NULL;
static uint32_t *screen_buffer = NULL;
static uint32_t screen_buffer_size = 0;
static uint16_t scrnw = 0, scrnh = 0;
static vec3f_t camera_pos;
static float camera_angle_cos, camera_angle_sin;
static texture_t *textures_arr = NULL;
static vec2i_t *surface_buffer = NULL;
static surface_type_t surface_type = NONE;
static vec2i_t surface_x_range = {0, 0};

static bool r_init_screen(uint16_t scrn_w, uint16_t scrn_h)
{
    screen_buffer_size = scrn_w * scrn_h * sizeof(uint32_t);
    screen_buffer = malloc(screen_buffer_size);

    if (screen_buffer != NULL)
    {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
        screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, scrn_w, scrn_h);
        memset(screen_buffer, 0, screen_buffer_size);
        
        if (screen_texture != NULL)
            return true;

        free(screen_buffer);
    }

    DOOM_LOG_ERROR("Nao foi possivel iniciar o buffer de renderizacao");
    return false;
}

// world space -> camera space (translate and rotate)
static vec2f_t r_world_pos_to_camera(vec2f_t point) 
{
    const vec2f_t u = { point.x - camera_pos.x, point.y - camera_pos.y };
    return (vec2f_t) {
        .x = u.x * camera_angle_cos - u.y * camera_angle_sin,
        .y = u.x * camera_angle_sin + u.y * camera_angle_cos,
    };
}

static void r_clip_behind_player(vec2f_t *a, float *z0, vec2f_t b, float z1)
{
    float distance_plane_a = a->y;
    float distance_plane_b = b.y;
    float d = distance_plane_a - distance_plane_b;
    if (d == 0) d = 1;
    float s = distance_plane_a / d;
    a->x = a->x + s * (b.x - a->x);
    a->y = a->y + s * (b.y - a->y);
    // Esses valores são especulados mas basicamente sem eles Y seria tão pequeno
    // Que as divisões por ele se tornariam muito absurdas causando bugs visuais
    if (a->y < 0.01f && a->y > -0.01f) a->y = 1; 

    *z0 = *z0 + s * (z1 - (*z0));
}

static void r_draw_wall(sector_t *sector, wall_t *wall)
{
    // Transformando as posições do mundo em relação ao player
    vec2f_t op0 = r_world_pos_to_camera(v2i_to_v2f(wall->a));
    vec2f_t op1 = r_world_pos_to_camera(v2i_to_v2f(wall->b));
    
    vec2f_t wall_dir = { op1.x - op0.x, op1.y - op0.y };
    vec2f_t normal = { -wall_dir.y, wall_dir.x };
    // Produto escalar da normal com a posição da câmera
    float dot = normal.x * op0.x + normal.y * op0.y;
    if (dot < 0)
    {
        if (surface_type == NONE) return;
        vec2f_t tmp = op0;
        op0 = op1;
        op1 = tmp;
    }

    vec2f_t op2 = op0;
    vec2f_t op3 = op1;
    float z0 = sector->z_floor + camera_pos.z;
    float z1 = sector->z_floor + camera_pos.z;
    float z2 = z0 - sector->z_ceil;
    float z3 = z1 - sector->z_ceil;

    if (op0.y <= 0 && op1.y <= 0) 
        return;

    if (op0.y <= 0)
    {
        r_clip_behind_player(&op0, &z0, op1, z1);
        r_clip_behind_player(&op2, &z2, op3, z3);
    }

    if (op1.y <= 0)
    {
        r_clip_behind_player(&op1, &z1, op0, z0);
        r_clip_behind_player(&op3, &z3, op2, z2);
    }

    op0 = (vec2f_t){op0.x * FOV/op0.y + scrnw/2, z0*FOV/op0.y + scrnh/2};
    op1 = (vec2f_t){op1.x * FOV/op1.y + scrnw/2, z1*FOV/op1.y + scrnh/2};
    op2 = (vec2f_t){op2.x * FOV/op2.y + scrnw/2, z2*FOV/op2.y + scrnh/2};
    op3 = (vec2f_t){op3.x * FOV/op3.y + scrnw/2, z3*FOV/op3.y + scrnh/2};

    int dy_bottom = op1.y - op0.y;
    int dy_top = op3.y - op2.y;
    int dx = op1.x - op0.x;
    if (dx == 0) dx = 1;

    int xs = op0.x;

    float ht = 0, ht_step = (float)textures_arr[wall->texture_id].width/(float)(op1.x - op0.x);
    if (op0.x < 0) { ht -= ht_step * op0.x; op0.x = 0; }
    if (op1.x < 0) op1.x = 0;
    if (op0.x > scrnw) op0.x = scrnw;
    if (op1.x > scrnw) op1.x = scrnw;
    
    if (textures_arr[wall->texture_id].height == 1 && textures_arr[wall->texture_id].width == 1)
    {
        if (surface_type != NONE && op0.x < surface_x_range.x)
            surface_x_range.x = op0.x;
        if (surface_type != NONE && op1.x > surface_x_range.y)
            surface_x_range.y = op1.x;

        for (int x = op0.x; x < op1.x; x++)
        {
            int y1 = dy_bottom * (x - xs + 0.5) / dx + op0.y;
            int y2 = dy_top * (x - xs + 0.5) / dx + op2.y;
            
            if (y1 < 0) y1 = 0;
            if (y2 < 0) y2 = 0;
            if (y1 > scrnh) y1 = scrnh;
            if (y2 > scrnh) y2 = scrnh;
            
            if (dot < 0)
            {
                if (surface_type == FLOOR)  { surface_buffer[x].x = y1; continue; }
                if (surface_type == CEIL)   { surface_buffer[x].x = y2; continue; }
            }

            if (surface_type == FLOOR)  { surface_buffer[x].y = y1; }
            if (surface_type == CEIL)   { surface_buffer[x].y = y2; }
            
            for (int y = y2; y < y1; y++)
                screen_buffer[scrnw * y + x] = textures_arr[wall->texture_id].data.color;
        }
    }
    else
    {
        if (surface_type != NONE && op0.x < surface_x_range.x)
        surface_x_range.x = op0.x;
        if (surface_type != NONE && op1.x > surface_x_range.y)
            surface_x_range.y = op1.x;

        for (int x = op0.x; x < op1.x; x++)
        {
            int y1 = dy_bottom * (x - xs + 0.5) / dx + op0.y;
            int y2 = dy_top * (x - xs + 0.5) / dx + op2.y;
            
            float vt = 0, vt_step = (float)textures_arr[wall->texture_id].height/(float)(y1 - y2);

            if (y1 < 0) { vt -= vt_step * y1; y1 = 0; }
            if (y2 < 0) y2 = 0;
            if (y1 > scrnh) y1 = scrnh;
            if (y2 > scrnh) y2 = scrnh;
            
            if (dot < 0)
            {
                if (surface_type == FLOOR)  { surface_buffer[x].x = y1; continue; }
                if (surface_type == CEIL)   { surface_buffer[x].x = y2; continue; }
            }

            if (surface_type == FLOOR)  { surface_buffer[x].y = y1; }
            if (surface_type == CEIL)   { surface_buffer[x].y = y2; }

            for (int y = y2; y < y1; y++)
            {
                uint32_t pixel = (int)(textures_arr[wall->texture_id].height - vt - 1) * textures_arr[wall->texture_id].width + (int)ht;
                uint32_t color = textures_arr[wall->texture_id].data.buffer[pixel];
                screen_buffer[scrnw * y + x] = color;
                vt += vt_step;
            }
            ht += ht_step;
        }
    }
}

void draw_surface(surface_type_t type, uint8_t surface_texture_id)
{
    if (type == FLOOR)
    {
        for(int x = surface_x_range.x; x < surface_x_range.y; x++)
        {
            // Para o chão a componente Y é menor que a X
            for (int y = surface_buffer[x].y; y < surface_buffer[x].x; y++)
            {
                if (y < 0 || y > scrnh || x < 0 || x > scrnw) 
                    return;
                screen_buffer[scrnw * y + x] = textures_arr[surface_texture_id].data.color;
            }
        }
    }
    else if (type == CEIL)
    {
        for(int x = surface_x_range.x; x < surface_x_range.y; x++)
        {
            // Para o teto a componente X é menor que a Y
            for (int y = surface_buffer[x].x; y < surface_buffer[x].y; y++)
            {
                if (y < 0 || y > scrnh || x < 0 || x > scrnw) 
                    return;
                screen_buffer[scrnw * y + x] = textures_arr[surface_texture_id].data.color;
            }
        }
    }
}

bool r_init(uint16_t scrn_w, uint16_t scrn_h)
{
    SDL_Window *win = (SDL_Window*)w_get_handler();
    scrnw = scrn_w;
    scrnh = scrn_h;
    surface_buffer = (vec2i_t*)malloc(scrn_w * sizeof(vec2i_t));

    if (surface_buffer == NULL)
    {
        DOOM_LOG_ERROR("Nao foi possivel alocar memoria para o buffer de superficie");
        return false;
    }

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

    if (renderer == NULL)
    {
        DOOM_LOG_ERROR("Nao foi possivel criar o renderer SDL");
        return false;
    }

    if (!r_init_screen(scrn_w, scrn_h))
    {
        SDL_DestroyRenderer(renderer);
        return false;
    }

    SDL_RenderSetLogicalSize(renderer, scrn_w, scrn_h);
    return true;
}

void r_begin_draw(const player_t *player, texture_t *textures)
{
    memset(screen_buffer, 0, screen_buffer_size);

    camera_pos = (vec3f_t){ player->position.x, player->position.y, player->position.z };
    camera_angle_cos = player->angle_cos;
    camera_angle_sin = player->angle_sin;

    textures_arr = textures;
}

void r_draw_sectors(sector_t *sectors, wall_t *walls, queue_sector_t *queue)
{
    for (uint32_t i = 0; i < queue->size; i++) 
    {
        uint32_t id = queue->arr[(queue->front + i) % MAX_QUEUE];

        if (camera_pos.z < sectors[id].z_floor) 
        { 
            surface_type = FLOOR;

            for (uint16_t i = 0; i < scrnw; i++)
                surface_buffer[i] = (vec2i_t){scrnh - 1, 0};
        }
        else if (camera_pos.z > sectors[id].z_ceil) 
        { 
            surface_type = CEIL; 

            for (uint16_t i = 0; i < scrnw; i++)
                surface_buffer[i] = (vec2i_t){0, scrnh - 1};
        }
        else surface_type = NONE;

        surface_x_range.x = scrnw;
        surface_x_range.y = 0;

        for (uint32_t j = 0; j < sectors[id].num_walls; j++)
            r_draw_wall(&sectors[id], &walls[sectors[id].first_wall_id + j]);

        draw_surface(surface_type, surface_type == FLOOR ? sectors[id].floor_texture_id : sectors[id].ceil_texture_id);
    }
}

void r_end_draw()
{
    SDL_UpdateTexture(screen_texture, NULL, screen_buffer, scrnw * sizeof(uint32_t));
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void r_draw_pixel(int x, int y, uint32_t color)
{
    screen_buffer[scrnw * y + x] = color;
}

void r_draw_floor()
{
    int x0 = scrnw/2;
    int y0 = scrnh/2;
    float move_up_down = camera_pos.z; if (move_up_down == 0) { move_up_down = 0.001; }

    for (int y = 0; y < y0; y++)
    {
        for (int x = -x0; x < x0; x++)
        {
            float fx = x / (float) y * move_up_down;
            float fy = FOV / (float) y * move_up_down;
            vec2f_t r = (vec2f_t){fx, fy};
            r = (vec2f_t) {
                .x = r.x * camera_angle_sin - r.y * camera_angle_cos - (camera_pos.y),
                .y = r.x * camera_angle_cos + r.y * camera_angle_sin + (camera_pos.x),
            };

            if (r.x < 0) r.x = -r.x + 1;
            if (r.y < 0) r.y = -r.y + 1;
            if (r.x <= 0 || r.y <= 0 || r.x >= 5 || r.y >= 5) continue;
            if ((int)r.x%2 == (int)r.y%2) r_draw_pixel(x + x0, y + y0, 0xFF00FF00);
            else r_draw_pixel(x + x0, y + y0, 0xFF0000FF);
        }
    }
}

texture_t r_create_texture(const char *filename, uint8_t width, uint8_t height)
{
    texture_t texture = { .width = width, .height = height, .data.buffer = NULL };
    FILE *file = fopen(filename, "rb");

    if (file != NULL)
    {
        fseek(file, 54, SEEK_SET);

        uint32_t size = width * height * sizeof(uint32_t);
        texture.data.buffer = (uint32_t*)malloc(size);

        if (texture.data.buffer != NULL)
            fread(texture.data.buffer, sizeof(uint32_t), size, file);

        fclose(file);
    }

    return texture;
}

void r_shutdown()
{
    SDL_DestroyTexture(screen_texture);
    free(screen_buffer);
    free(surface_buffer);
    SDL_DestroyRenderer(renderer);
}
