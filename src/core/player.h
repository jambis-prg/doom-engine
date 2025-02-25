#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "typedefs.h"

typedef struct _player
{
    vec3f_t position, velocity;
    float angle, angle_sin, angle_cos, yaw;
    uint32_t sector_id;
} player_t;

#endif