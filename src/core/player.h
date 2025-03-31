#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "typedefs.h"
#include "assets/animation.h"

typedef struct _player
{
    vec3f_t position, velocity;
    float angle;
    uint32_t sector_id, weapon_index, bullet_count[4], health, armor, killed_enemies_count;
    gun_type_t weapon_type[4];
} player_t;

#endif