#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include "typedefs.h"

float u_magnitude_vec(float x, float y, float z);
vec2f_t u_normalize_vec2f(vec2f_t vec);
vec2f_t u_rotate_vec2f(vec2f_t vec, float angle);

float u_convert_bams_to_radians(int16_t bams);
float u_convert_degrees_to_radians(int16_t angle);
float u_normalize_angle(float angle);
#endif