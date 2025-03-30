#ifndef ANIMATION_H_INCLUDED
#define ANIMATION_H_INCLUDED

#include "typedefs.h"

#define BOB_SPEED 5.f
#define BOB_RANGE 12.f

typedef enum _gun_type { NONE, FIST, CHAINSAW, PISTOL, SHOTGUN, CHAINGUN, ROCKET, PLASMA, BFG, SUPER_SHT } gun_type_t;

typedef struct _animation
{
    bool is_playing;
    uint16_t first_sprite_idx, sprite_count, current_idx, current_frame;
    gun_type_t type; // Se for do tipo NONE ent nao eh uma animacao de arma
    float accumulator_time;
} animation_t;

animation_t anm_create_animation(uint16_t first_sprite_idx, uint16_t sprite_count, bool play_on_awake, gun_type_t type);

void anm_update(animation_t *animation);
void anm_render(animation_t *animation, float normalized_velocity);

#endif