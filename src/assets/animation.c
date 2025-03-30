#include "animation.h"
#include "renderer/renderer.h"
#include "asset.h"
#include "core/timer.h"
#include <math.h>

#define TOP_OFFSET_ALLIGN 72

/*
 * Função que retorna o índice do sprite da animação da arma.
 * Cada arma possui uma sequência finita de frames:
 *
 * FIST:        [0]                       (1 frame)
 * CHAINSAW:    [0, 1, 2, 1]              (4 frames)
 * PISTOL:      [0, 1, 2, 3, 0]           (5 frames)
 * SHOTGUN:     [0, 0, 0, 1, 2, 3, 2, 1, 0] (9 frames)
 * CHAINGUN:    [0, 1, 2, 3, 4, 5]        (6 frames)
 * ROCKET:      [0, 1, 2, 3, 4, 5]        (6 frames)
 * PLASMA:      [0, 1, 2, 3]              (4 frames)
 * BFG:         [0, 1, 2, 3, 4, 5, 6, 7]   (8 frames)
 * SUPER_SHT:   [0, 1, 2, 3, 2, 1, 0]      (7 frames)
 *
 * Se current_frame for maior ou igual ao tamanho da sequência, retorna -1.
 */
static int16_t anm_get_gun_frame_idx(uint16_t current_frame, gun_type_t type)
{
    switch(type)
    {
        case FIST:
            switch(current_frame)
            {
                case 0: return 0;
                default: return -1;
            }
        case CHAINSAW:
            switch(current_frame)
            {
                case 0: return 0;
                case 1: return 1;
                case 2: return 2;
                case 3: return 1;
                default: return -1;
            }
        case PISTOL:
            switch(current_frame)
            {
                case 0: return 0;
                case 1: return 1;
                case 2: return 2;
                case 3: return 3;
                default: return -1;
            }
        case SHOTGUN:
            switch(current_frame)
            {
                case 0: 
                case 1: 
                case 2: return 0;  // frames 0, 1 e 2 retornam sprite 0
                case 3: return 1;
                case 4: return 2;
                case 5: return 3;
                case 6: return 2;
                case 7: return 1;
                default: return -1;
            }
        case CHAINGUN:
            switch(current_frame)
            {
                case 0: return 0;
                case 1: return 1;
                case 2: return 2;
                case 3: return 3;
                case 4: return 4;
                case 5: return 5;
                default: return -1;
            }
        case ROCKET:
            switch(current_frame)
            {
                case 0: return 0;
                case 1: return 1;
                case 2: return 2;
                case 3: return 3;
                case 4: return 4;
                case 5: return 5;
                default: return -1;
            }
        case PLASMA:
            switch(current_frame)
            {
                case 0: return 0;
                case 1: return 1;
                case 2: return 2;
                case 3: return 3;
                default: return -1;
            }
        case BFG:
            switch(current_frame)
            {
                case 0: return 0;
                case 1: return 1;
                case 2: return 2;
                case 3: return 3;
                case 4: return 4;
                case 5: return 5;
                case 6: return 6;
                case 7: return 7;
                default: return -1;
            }
        case SUPER_SHT:
            switch(current_frame)
            {
                case 0: return 0;
                case 1: return 1;
                case 2: return 2;
                case 3: return 3;
                case 4: return 2;
                case 5: return 1;
                case 6: return 0;
                default: return -1;
            }
        default:
            return -1;
    }
}

static int16_t anm_get_gun_effect_idx(uint16_t current_frame, gun_type_t type)
{
    switch(type)
    {
        case PISTOL:
            switch(current_frame)
            {
                case 1: return 5; // o efeito só é aplicado no frame 1
                default: return -1;
            }
        case SHOTGUN:
            switch(current_frame)
            {
                case 0: return 4; // primeiro frame de efeito
                case 1: return 5; // segundo frame de efeito
                default: return -1;
            }
        case NONE:
        case FIST:
        case CHAINSAW:
        case CHAINGUN:
        case ROCKET:
        case PLASMA:
        case BFG:
        case SUPER_SHT:
            return -1;
        default:
            return -1;
    }
}
animation_t anm_create_animation(uint16_t first_sprite_idx, uint16_t sprite_count, bool play_on_awake, gun_type_t type)
{
    return (animation_t) { 
        .is_playing = play_on_awake,
        .first_sprite_idx = first_sprite_idx, 
        .sprite_count = sprite_count, 
        .current_idx = 0, 
        .type = type,
        .accumulator_time = 0.f 
    };
}

void anm_update(animation_t *animation)
{
    double delta_time = t_get_delta_time();
    if (animation->is_playing)
    {
        animation->accumulator_time += delta_time;

        if (animation->accumulator_time >= ANIMATION_TICK)
        {
            animation->accumulator_time -= ANIMATION_TICK;

            if (animation->type != NONE)
            {
                int16_t current_idx = anm_get_gun_frame_idx(animation->current_frame + 1, animation->type);
                if (current_idx < 0)
                {
                    animation->current_idx = 0;
                    animation->is_playing = false;
                    animation->accumulator_time = 0.f;
                    animation->current_frame = 0;
                }
                else
                {
                    animation->current_idx = current_idx;
                    animation->current_frame++;
                }
            }
            else
            {
                animation->current_idx = (animation->current_idx + 1) % animation->sprite_count;
                if (animation->current_idx == 0)
                {
                    animation->is_playing= false;
                    animation->accumulator_time = 0.f;
                    animation->current_frame = 0;
                }
                else
                    animation->current_frame++;
            }
        }
    }
}

void anm_render(animation_t *animation, float normalized_velocity)
{
    double time = t_get_time();
    float offset_x = 0, offset_y = 0;
    float bob_x = sin(time * BOB_SPEED) * BOB_RANGE * normalized_velocity;
    float bob_y = fabs(cos(time * BOB_SPEED)) * BOB_RANGE * normalized_velocity;
    if (animation->is_playing && animation->type != NONE)
    {
        bob_x = 0;
        bob_y = 0;
        int16_t effect_id = anm_get_gun_effect_idx(animation->current_frame, animation->type);
        if (effect_id >= 0)
        {
            image_t *effect = a_get_sprite(animation->first_sprite_idx + effect_id);
            offset_x = effect->left_offset;
            offset_y = effect->top_offset - TOP_OFFSET_ALLIGN;
            for (uint32_t x = 0; x < effect->width; x++)
                for(uint32_t y = 0; y < effect->height; y++)
                    r_draw_pixel(x - offset_x, y - offset_y, effect->data[y * effect->width + x]);
        }
    }

    image_t *sprite = a_get_sprite(animation->first_sprite_idx + animation->current_idx);
    offset_x = sprite->left_offset + bob_x;
    offset_y = sprite->top_offset - TOP_OFFSET_ALLIGN - bob_y;
        
    for (uint32_t x = 0; x < sprite->width; x++)
        for(uint32_t y = 0; y < sprite->height; y++)
            r_draw_pixel(x - offset_x, y - offset_y, sprite->data[y * sprite->width + x]);
}
