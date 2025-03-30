#include "utils.h"
#include <math.h>

float u_magnitude_vec(float x, float y, float z)
{
    return sqrt(x * x + y * y + z * z);
}

vec2f_t u_normalize_vec2f(vec2f_t vec)
{
    float mag = sqrt(vec.x * vec.x + vec.y * vec.y);
    if (mag == 0) return (vec2f_t) {.x = 0, .y = 0};
    return (vec2f_t){ vec.x / mag, vec.y / mag };
}

vec2f_t u_rotate_vec2f(vec2f_t vec, float angle)
{
    float cosT = cos(angle);
    float sinT = sin(angle);
    return (vec2f_t){ vec.x * cosT - vec.y * sinT, vec.x * sinT + vec.y * cosT };
}

float u_convert_bams_to_radians(int16_t bams)
{
    // return ((uint16_t)bams) * TAU / 65536.0f;
    return (bams * PI) / 32768.0f; // 32768 corresponde a 180 graus
}

float u_convert_degrees_to_radians(int16_t angle)
{
    return (angle * PI) / 180.f;
}

float u_normalize_angle(float angle)
{
    angle = fmodf(angle, 2 * PI);
    return angle >= 0 ? angle : angle + 2 * PI;
}