#include "game_core.h"
#include "window.h"
#include "renderer/renderer.h"
#include "player.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_keyboard.h"

#define PLAYER_SPEED 5

typedef struct _game_core
{
    bool is_running, is_paused;
    uint16_t scrnw, scrnh;
    player_t player;
} game_core_t;

static game_core_t game_manager = {0};

bool g_init(uint16_t scrn_w, uint16_t scrn_h)
{
    game_manager.scrnw = scrn_w;
    game_manager.scrnh = scrn_h;

    game_manager.is_running = true;
    game_manager.is_paused = false;
    game_manager.player = (player_t)
    {
        .position = (vec3f_t){14, -22, 0},
        .angle = 0,
        .angle_cos = 1,
        .angle_sin = 0,
        .sector_id = 0,
        .velocity = (vec3f_t){ 0, 0, 0},
        .yaw = 0
    };
    if (!w_init(scrn_w, scrn_h))
        return false;

    if (!r_init(256, 256))
        return false;

    return true;
}

void g_run()
{
    wall_t walls[] = { { .a = {2, 8}, .b = {58, 8}, .is_portal = false } };
    sector_t sectors[] = {
        { 
            .first_wall_id = 0,       // Índice da primeira parede
            .num_walls = 1,   // Número de paredes
            .z_floor = 0.0f,
            .z_ceil = 3.0f
        }
    };
    queue_sector_t queue = {
        .arr = { 0 },  // Adiciona o ID do setor
        .size = 1,                    // Apenas 1 elemento na fila
        .front = 0,
    };

    const uint8_t* keystate = SDL_GetKeyboardState(NULL);
    uint64_t last = SDL_GetPerformanceCounter(), now = 0;
    int last_mouse_x = 0;
    while (game_manager.is_running && w_handle_events()) 
    {
        now = SDL_GetPerformanceCounter();
        double deltaTime = (double)((now - last) * 1000 / (double)SDL_GetPerformanceFrequency()) * 0.001;
        last = now;

        int mouse_x = 0;
        SDL_GetMouseState(&mouse_x, NULL);
        float mouse_dx = mouse_x - last_mouse_x;
        last_mouse_x = mouse_x;
        game_manager.player.angle += mouse_dx * deltaTime;
        game_manager.player.angle = fmod(game_manager.player.angle, 2 * PI);
    
        // Corrige valores negativos
        if (game_manager.player.angle < 0)
            game_manager.player.angle += 2 * PI;

        game_manager.player.angle_cos = cos(game_manager.player.angle);
        game_manager.player.angle_sin = sin(game_manager.player.angle);
        
        float dx = game_manager.player.angle_sin * PLAYER_SPEED;
        float dy = game_manager.player.angle_cos * PLAYER_SPEED;
        vec3f_t current_pos = game_manager.player.position;
        printf("%.2f, %.2f - %.2f\n", current_pos.x, current_pos.y, game_manager.player.angle);
        vec2f_t dir = {0};
        if(keystate[SDL_SCANCODE_W]) dir = (vec2f_t){dx, -dy};
        if(keystate[SDL_SCANCODE_S]) dir = (vec2f_t){-dx, dy};
        if(keystate[SDL_SCANCODE_A]) dir = (vec2f_t){dy, -dx};
        if(keystate[SDL_SCANCODE_D]) dir = (vec2f_t){-dy, dx};

        game_manager.player.position = (vec3f_t) { current_pos.x + dir.x * deltaTime, current_pos.y + dir.y * deltaTime, current_pos.z };

        r_begin_draw(&game_manager.player);

        r_draw_sectors(sectors, walls, &queue);

        r_end_draw();
    }
}

void g_shutdown()
{
    r_shutdown();
    w_shutdown();
}
