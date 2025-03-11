#include "renderer.h"
#include "window.h"
#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include "logger.h"

static SDL_Renderer *renderer = NULL;
static SDL_Texture *screen_texture = NULL;
static uint32_t *screen_buffer = NULL;
static uint32_t screen_buffer_size = 0;
static uint16_t scrnw = 0, scrnh = 0;
static vec3f_t camera_pos;
static float camera_angle_cos, camera_angle_sin;
static texture_t *textures_arr = NULL;

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
    if (a->y == 0) a->y = 1;

    *z0 = *z0 + s * (z1 - (*z0));
}

static void r_draw_wall(sector_t *sector, wall_t *wall, bool back_face)
{
    // Transformando as posições do mundo em relação ao player
    vec2f_t op0 = r_world_pos_to_camera(v2i_to_v2f(wall->a));
    vec2f_t op1 = r_world_pos_to_camera(v2i_to_v2f(wall->b));

    if (back_face)
    {
        vec2f_t tmp = op0;
        op0 = op1;
        op1 = tmp;
    }

    vec2f_t op2 = op0;
    vec2f_t op3 = op1;
    float z0 = sector->z_floor - camera_pos.z;
    float z1 = sector->z_floor - camera_pos.z;
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

    op0 = (vec2f_t){op0.x * 200/op0.y + scrnw/2, z0*200/op0.y + scrnh/2};
    op1 = (vec2f_t){op1.x * 200/op1.y + scrnw/2, z1*200/op1.y + scrnh/2};
    op2 = (vec2f_t){op2.x * 200/op2.y + scrnw/2, z2*200/op2.y + scrnh/2};
    op3 = (vec2f_t){op3.x * 200/op3.y + scrnw/2, z3*200/op3.y + scrnh/2};

    int dy_bottom = op1.y - op0.y;
    int dy_top = op3.y - op2.y;
    int dx = op1.x - op0.x;
    if (dx == 0) dx = 1;

    int xs = op0.x;

    if (op0.x < 1) op0.x = 1;
    if (op1.x < 1) op1.x = 1;
    if (op0.x > scrnw - 1) op0.x = scrnw - 1;
    if (op1.x > scrnw - 1) op1.x = scrnw - 1;
    
    if (textures_arr[wall->texture_id].height == 1 && textures_arr[wall->texture_id].width == 1)
    {
        for (int x = op0.x; x < op1.x; x++)
        {
            int y1 = dy_bottom * (x - xs + 0.5) / dx + op0.y;
            int y2 = dy_top * (x - xs + 0.5) / dx + op2.y;
            
            if (y1 < 1) y1 = 1;
            if (y2 < 1) y2 = 1;
            if (y1 > scrnh - 1) y1 = scrnh - 1;
            if (y2 > scrnh - 1) y2 = scrnh - 1;
            
            //screen_buffer[scrnw * y1 + x] = 0xFF0000FF;
            //screen_buffer[scrnw * y2 + x] = 0xFF0000FF;
            for (int y = y2; y < y1; y++)
                screen_buffer[scrnw * y + x] = textures_arr[wall->texture_id].data.color;
        }
    }
    else
    {
        for (int x = op0.x; x < op1.x; x++)
        {
            int y1 = dy_bottom * (x - xs + 0.5) / dx + op0.y;
            int y2 = dy_top * (x - xs + 0.5) / dx + op2.y;
            
            if (y1 < 1) y1 = 1;
            if (y2 < 1) y2 = 1;
            if (y1 > scrnh - 1) y1 = scrnh - 1;
            if (y2 > scrnh - 1) y2 = scrnh - 1;
            
            for (int y = y2; y < y1; y++)
            {
                uint32_t pixel = (textures_arr[wall->texture_id].height - y - 1) * textures_arr[wall->texture_id].width + x;
                uint32_t color = textures_arr[wall->texture_id].data.buffer[pixel];
                screen_buffer[scrnw * y + x] = color;
            }
        }
    }
}

bool r_init(uint16_t scrn_w, uint16_t scrn_h)
{
    SDL_Window *win = (SDL_Window*)w_get_handler();
    scrnw = scrn_w;
    scrnh = scrn_h;

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

        for (uint8_t k = 0; k < 2; k++)
        {
            for (uint32_t j = 0; j < sectors[id].num_walls; j++) 
                r_draw_wall(&sectors[id], &walls[sectors[id].first_wall_id + j], k);
        }
    }
}

void r_end_draw()
{
    SDL_UpdateTexture(screen_texture, NULL, screen_buffer, scrnw * sizeof(uint32_t));
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void r_shutdown()
{
    SDL_DestroyTexture(screen_texture);
    free(screen_buffer);
    SDL_DestroyRenderer(renderer);
}
