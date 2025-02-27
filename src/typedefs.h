#ifndef TYPEDEFS_H_INCLUDED
#define TYPEDEFS_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct _vec2i
{
    int x, y;
} vec2i_t;

typedef struct _vec2f
{
    float x, y;
} vec2f_t;

typedef struct _vec3f
{
    float x, y, z;
} vec3f_t;

#define v2f_to_v2i(_v) ({ __typeof__(_v) __v = (_v); (vec2i_t) { __v.x, __v.y }; })
#define v2i_to_v2f(_v) ({ __typeof__(_v) __v = (_v); (vec2f_t) { __v.x, __v.y }; })

#endif