#include "game_core.h"
#include "window.h"
#include "renderer/renderer.h"
#include "player.h"
#include "logger.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_keyboard.h"
#include <string.h>

#define PLAYER_SPEED 5

typedef struct _game_core
{
    bool is_running, is_paused;
    uint16_t scrnw, scrnh;
    player_t player;
} game_core_t;

static game_core_t game_manager = {0};

static void g_merge(queue_sector_t *queue, float *depth, int low, int high)
{
    float tmp[queue->size];
    uint32_t tmp_id[queue->size];
    memcpy(tmp, depth, queue->size * sizeof(float));
    memcpy(tmp_id, queue->arr, queue->size * sizeof(uint32_t));

    int pivot = (low + high) / 2;
    int i1 = low, i2 = pivot + 1;
    for (int curr = low; curr <= high; curr++)
    {
        if (i2 > high || tmp[i1] > tmp[i2])
        {
            depth[curr] = tmp[i1];
            queue->arr[curr] = tmp_id[i1];
            i1++;
        }
        else
        {
            depth[curr] = tmp[i2];
            queue->arr[curr] = tmp_id[i2];
            i2++;
        }
    }
}

static void g_merge_sort_aux(queue_sector_t *queue, float *depth, int low, int high)
{
    if (low < high)
    {
        int pivot = (low + high) / 2;

        g_merge_sort_aux(queue, depth, low, pivot);
        g_merge_sort_aux(queue, depth, pivot + 1, high);
        g_merge(queue, depth, low, high);
    }
}

static void g_merge_sort(queue_sector_t *queue, float *depth)
{
    g_merge_sort_aux(queue, depth, 0, queue->size - 1);
}

static void g_sort_sectors(sector_t *sectors, queue_sector_t *queue)
{
    float depth[queue->size];

    for (uint32_t i = 0; i < queue->size; i++)
    {
        uint32_t id = queue->arr[i];
        // Transformando as posições do mundo em relação ao player
        vec2f_t u = { sectors[id].center.x - game_manager.player.position.x, sectors[id].center.y - game_manager.player.position.y };
        u = (vec2f_t) {
            .x = u.x * game_manager.player.angle_cos - u.y * game_manager.player.angle_sin,
            .y = u.x * game_manager.player.angle_sin + u.y * game_manager.player.angle_cos,
        };

        depth[i] = sqrt(u.x * u.x + u.y * u.y);
    }

    g_merge_sort(queue, depth); 
}

bool g_init(uint16_t scrn_w, uint16_t scrn_h)
{
    game_manager.scrnw = scrn_w;
    game_manager.scrnh = scrn_h;

    game_manager.is_running = true;
    game_manager.is_paused = false;
    game_manager.player = (player_t)
    {
        .position = (vec3f_t){0, 0, 0},
        .angle = 0,
        .angle_cos = 1,
        .angle_sin = 0,
        .sector_id = 0,
        .velocity = (vec3f_t){ 0, 0, 0},
        .yaw = 0
    };
    
    if (!w_init(scrn_w, scrn_h))
        return false;

    if (!r_init(scrn_w / 4, scrn_h / 4))
        return false;

    return true;
}

void g_run()
{
    wall_t walls[8] = { 
        { .a = {5, 5}, .b = {-5, 5}, .is_portal = false, .texture_id = 0 }, 
        { .a = {-5, -5}, .b = {5, -5}, .is_portal = false, .texture_id = 0 },
        { .a = {5, -5}, .b = {5, 5}, .is_portal = false, .texture_id = 0 },
        { .a = {-5, 5}, .b = {-5, -5}, .is_portal = false, .texture_id = 0 },
    };

    for (uint32_t i = 1; i < 2; i++)
    {
        uint32_t j = 4 * i;
        walls[j] = (wall_t){.a = {5, 5}, .b = {-5, 5}, .is_portal = false, .texture_id = i};
        walls[j + 1] = (wall_t){.a = {-5, -5}, .b = {5, -5}, .is_portal = false, .texture_id = i};
        walls[j + 2] = (wall_t){.a = {5, -5}, .b = {5, 5}, .is_portal = false, .texture_id = i};
        walls[j + 3] = (wall_t){.a = {-5, 5}, .b = {-5, -5}, .is_portal = false, .texture_id = i};

        for (uint32_t k = 0; k < 4; k++)
        {
            walls[j + k].a.x += 15 * i;
            walls[j + k].a.y += 15 * i;
            walls[j + k].b.x += 15 * i;
            walls[j + k].b.y += 15 * i;
        }
    }

    sector_t sectors[] = {
        { 
            .first_wall_id = 0,       // Índice da primeira parede
            .num_walls = 4,   // Número de paredes
            .z_floor = 0.0f,
            .z_ceil = 5.0f
        },
        { 
            .first_wall_id = 4,       // Índice da primeira parede
            .num_walls = 4,   // Número de paredes
            .z_floor = 0.0f,
            .z_ceil = 5.0f
        },
    };


    for (uint32_t i = 0; i < 2; i++)
    {
        for (uint32_t j = 0; j < sectors[i].num_walls; j++)
        {
            const wall_t *wall = &walls[sectors[i].first_wall_id + j];
            sectors[i].center.x += (wall->a.x + wall->b.x) / 2.f;
            sectors[i].center.y += (wall->a.y + wall->b.y) / 2.f;
        }

        sectors[i].center.x /= sectors[i].num_walls;
        sectors[i].center.y /= sectors[i].num_walls;
    }

    queue_sector_t queue = {
        .arr = { 0, 1 },  // Adiciona o ID do setor
        .size = 2,                    // Apenas 1 elemento na fila
        .front = 0,
    };

    texture_t textures[] = {
        { .width = 1, .height = 1, .data.color = 0xFF0000FF},
        { .width = 1, .height = 1, .data.color = 0xFF00FF00}
    };

    const uint8_t* keystate = SDL_GetKeyboardState(NULL);
    uint64_t last = SDL_GetPerformanceCounter(), now = 0;
    float sense = 0.2f;

    bool esc_pressed = false;
    while (game_manager.is_running && w_handle_events()) 
    {
        now = SDL_GetPerformanceCounter();
        double deltaTime = (double)((now - last) * 1000 / (double)SDL_GetPerformanceFrequency()) * 0.001;
        last = now;
        
        if(keystate[SDL_SCANCODE_ESCAPE]) 
        {
            if (!esc_pressed)
                game_manager.is_paused = !game_manager.is_paused;
            esc_pressed = true;
        }
        else
            esc_pressed = false;

        if (!game_manager.is_paused)
        {
            int mouse_x = 0;
            SDL_GetMouseState(&mouse_x, NULL);
            SDL_WarpMouseInWindow((SDL_Window*)w_get_handler(), game_manager.scrnw / 2, game_manager.scrnh / 2);
            float mouse_dx = mouse_x - game_manager.scrnw / 2;
            game_manager.player.angle += mouse_dx * deltaTime * sense;
                
            // Corrige valores negativos
            if (game_manager.player.angle < 0)
                game_manager.player.angle += 2 * PI;
            else if (game_manager.player.angle >= 2 * PI)
                game_manager.player.angle -= 2 * PI;
    
            game_manager.player.angle_cos = cos(game_manager.player.angle);
            game_manager.player.angle_sin = sin(game_manager.player.angle);
            
            float dx = game_manager.player.angle_sin * PLAYER_SPEED;
            float dy = game_manager.player.angle_cos * PLAYER_SPEED;
            vec3f_t current_pos = game_manager.player.position;
            vec2f_t dir = {0};
            if(keystate[SDL_SCANCODE_W]) dir = (vec2f_t){dx, dy};
            if(keystate[SDL_SCANCODE_S]) dir = (vec2f_t){-dx, -dy};
            if(keystate[SDL_SCANCODE_A]) dir = (vec2f_t){-dy, dx};
            if(keystate[SDL_SCANCODE_D]) dir = (vec2f_t){dy, -dx};
            if(keystate[SDL_SCANCODE_Z]) current_pos.z += PLAYER_SPEED * deltaTime;
            if(keystate[SDL_SCANCODE_C]) current_pos.z -= PLAYER_SPEED * deltaTime;
            
            game_manager.player.position = (vec3f_t) { current_pos.x + dir.x * deltaTime, current_pos.y + dir.y * deltaTime, current_pos.z };
    
            g_sort_sectors(sectors, &queue);
    
            r_begin_draw(&game_manager.player, textures);
    
            r_draw_sectors(sectors, walls, &queue);
    
            r_end_draw();
        }
    }
}

void g_shutdown()
{
    r_shutdown();
    w_shutdown();
}
