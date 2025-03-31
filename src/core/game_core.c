#include "game_core.h"
#include "window.h"
#include "renderer/renderer.h"
#include "player.h"
#include "bsp/bsp.h"
#include "logger.h"
#include "wad/wad_reader.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_keyboard.h"
#include <string.h>
#include "utils.h"
#include "assets/asset.h"
#include "assets/image.h"
#include "assets/animation.h"
#include "timer.h"

#define PLAYER_ACCEL 10
#define PLAYER_MAX_SPEED 230
#define PLAYER_HEIGHT 43
#define FORCE_UP 20
#define GRAVITY 35
#define MAX_STEP 24
#define TERMINAL_VELOCITY 300
#define TICK 1/35.f

#define CAMERA_BOB_SPEED 10.f
#define CAMERA_BOB_RANGE 5.f


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
        .position = (vec3f_t){0, 0, -5},
        .angle = 0,
        .sector_id = 0,
        .health = 100,
        .armor = 0,
        .bullet_count = {1, 50, 2, 0},
        .killed_enemies_count = 0,
        .weapon_index = 2,
        .weapon_type = {FIST, PISTOL, SHOTGUN, NONE}, // TODO: substituir SHOTGUN por NONE
        .velocity = (vec3f_t){ 0, 0, 0},
    };
    
    if (!w_init(scrn_w, scrn_h))
        return false;

    if (!r_init(scrn_w / 4, scrn_h / 4))
        return false;

    return true;
}

void g_run()
{
    wad_reader_t wad_reader = wdr_open("resources/DOOM1.WAD");
    bsp_t bsp = bsp_create(&wad_reader, "E1M1");

    typedef struct _mus
    {
        char ID[4];
        uint16_t len_song;
        uint16_t offset_song;
        uint16_t num_primary_channels;
        uint16_t num_secondary_channels;
        uint16_t num_instruments;
        uint16_t reserved; // unused
        uint16_t *instrument_patch_list;
    } mus_t;
    mus_t mus;
    uint32_t mus_index;
    if (fh_get_value(&wad_reader.file_hash, "D_E1M1", &mus_index))
    {
        wdr_get_lump_header(&wad_reader, &mus, mus_index, sizeof(mus_t) - sizeof(mus.instrument_patch_list));

        DOOM_LOG_DEBUG("%.4s, %d, %d, %d, %d, %d", mus.ID, mus.len_song, mus.offset_song, mus.num_primary_channels, mus.num_secondary_channels,
        mus.num_instruments);
    }

    a_init(&wad_reader);
    
    wdr_close(&wad_reader);

    vec3f_t spawn = bsp_get_player_spawn(&bsp);
    game_manager.player.position.x = spawn.x;
    game_manager.player.position.y = spawn.y;
    game_manager.player.position.z = spawn.z + PLAYER_HEIGHT;
    game_manager.player.angle = (bsp.entities[0].angle * PI) / 180.f;

    const uint8_t* keystate = SDL_GetKeyboardState(NULL);
    float sense = 0.2f;

    bool esc_pressed = false;
    animation_t pistol_anim = anm_create_animation(SHOTGUN_INDEX, SHOTGUN_COUNT, false, SHOTGUN);
    int16_t last_ground_height = 0;

    t_start();
    while (game_manager.is_running && w_handle_events()) 
    {
        t_update();
        double delta_time = t_get_delta_time();

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
            game_manager.player.angle -= mouse_dx * delta_time * sense;
                
            // Corrige valores negativos
            if (game_manager.player.angle < 0)
                game_manager.player.angle += 2 * PI;
            else if (game_manager.player.angle >= 2 * PI)
                game_manager.player.angle -= 2 * PI;

            vec2f_t dir = {0};
            if (keystate[SDL_SCANCODE_W]) dir = u_rotate_vec2f((vec2f_t){1, 0}, game_manager.player.angle);
            if (keystate[SDL_SCANCODE_S]) dir = u_rotate_vec2f((vec2f_t){-1, 0}, game_manager.player.angle);
            if (keystate[SDL_SCANCODE_A]) dir = u_rotate_vec2f((vec2f_t){0, 1}, game_manager.player.angle);
            if (keystate[SDL_SCANCODE_D]) dir = u_rotate_vec2f((vec2f_t){0, -1}, game_manager.player.angle);
            if (!pistol_anim.is_playing) // Troca a arma selecionada
            {
                if (keystate[SDL_SCANCODE_1])
                {
                    game_manager.player.weapon_index = 0;
                    pistol_anim = anm_create_animation(FIST_INDEX, FIST_COUNT, false, FIST);
                }
                if (keystate[SDL_SCANCODE_2] && game_manager.player.weapon_type[1] != NONE && game_manager.player.bullet_count[1] != 0)
                {
                    game_manager.player.weapon_index = 1;
                    pistol_anim = anm_create_animation(PISTOL_INDEX, PISTOL_COUNT, false, PISTOL);
                }
                if (keystate[SDL_SCANCODE_3] && game_manager.player.weapon_type[2] != NONE && game_manager.player.bullet_count[2] != 0)
                {
                    game_manager.player.weapon_index = 2;
                    pistol_anim = anm_create_animation(SHOTGUN_INDEX, SHOTGUN_COUNT, false, SHOTGUN);
                }
                if (keystate[SDL_SCANCODE_4] && game_manager.player.weapon_type[3] != NONE && game_manager.player.bullet_count[3] != 0)
                {
                    // game_manager.player.weapon_index = 3;
                    // pistol_anim = anm_create_animation([], [], false, game_manager.player.weapon_type[3]); Trocar o colchete pela função que associa as armas
                }
            }
            if (!pistol_anim.is_playing && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0 && game_manager.player.bullet_count[game_manager.player.weapon_index] > 0)
            {
                pistol_anim.is_playing = true;
                if (game_manager.player.weapon_index != 0)
                    game_manager.player.bullet_count[game_manager.player.weapon_index]--; // Decrementa a bala
            }

            vec2f_t desired_velocity = { dir.x * PLAYER_MAX_SPEED, dir.y * PLAYER_MAX_SPEED };
            game_manager.player.velocity.x += (desired_velocity.x - game_manager.player.velocity.x) * delta_time * PLAYER_ACCEL;
            game_manager.player.velocity.y += (desired_velocity.y - game_manager.player.velocity.y) * delta_time * PLAYER_ACCEL;

            vec2f_t dir_2 = u_normalize_vec2f((vec2f_t){game_manager.player.velocity.x, game_manager.player.velocity.y });
            
            if (u_magnitude_vec(game_manager.player.velocity.x, game_manager.player.velocity.y, 0) >= PLAYER_MAX_SPEED)
            {
                game_manager.player.velocity.x = dir_2.x * PLAYER_MAX_SPEED;
                game_manager.player.velocity.y = dir_2.y * PLAYER_MAX_SPEED;
            }

            game_manager.player.position.x += game_manager.player.velocity.x * delta_time;
            game_manager.player.position.y += game_manager.player.velocity.y * delta_time;
    
            // Atualiza para obter a altura do subsetor
            bsp_update(&bsp, game_manager.player.position, game_manager.player.angle);
            int16_t new_ground_height = bsp_get_sub_sector_height(&bsp);
            float delta_z = new_ground_height + PLAYER_HEIGHT - game_manager.player.position.z;
            if (delta_z > 0)
            {
                if (new_ground_height - last_ground_height <= MAX_STEP)
                {
                    game_manager.player.position.z += delta_z > 0.1 ? delta_z * delta_time * FORCE_UP
                                                                                :
                                                                              delta_z;
                    last_ground_height = new_ground_height;
                }
            }
            else if(delta_z < -0.1f)
            {
                game_manager.player.velocity.z += GRAVITY * delta_time;
                if (game_manager.player.velocity.z < TERMINAL_VELOCITY)
                    game_manager.player.velocity.z = TERMINAL_VELOCITY;
                
                game_manager.player.position.z -= game_manager.player.velocity.z * delta_time;
                last_ground_height = new_ground_height;
            }
            else
            {
                game_manager.player.position.z -= delta_z;
                game_manager.player.velocity.z = 0;
                last_ground_height = new_ground_height;
            }
            
            anm_update(&pistol_anim);
            
            r_begin_draw(&game_manager.player);

            float normalized_velocity = (u_magnitude_vec(game_manager.player.velocity.x, game_manager.player.velocity.y, 0)) / PLAYER_MAX_SPEED;
            float bob_y = (cos(t_get_time() * CAMERA_BOB_SPEED)) * CAMERA_BOB_RANGE * normalized_velocity;

            vec3f_t tmp = game_manager.player.position;
            tmp.z += bob_y;
            // Atualiza novamente para renderizar com a nova altura + bob_y
            bsp_update(&bsp, tmp, game_manager.player.angle);

            bsp_render(&bsp);
            bsp_render_sprites(&bsp);
            anm_render(&pistol_anim, normalized_velocity);

            // Troca para a mão quando acaba a bala
            if (game_manager.player.weapon_index != 0 && game_manager.player.bullet_count[game_manager.player.weapon_index] == 0 && !pistol_anim.is_playing)
            {
                game_manager.player.weapon_index = 0;
                pistol_anim = anm_create_animation(FIST_INDEX, FIST_COUNT, false, FIST);
            }

            r_end_draw();
        }
    }

    bsp_delete(&bsp);
    a_shutdown();
}

void g_shutdown()
{
    r_shutdown();
    w_shutdown();
}
