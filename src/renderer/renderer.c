#include "renderer.h"
#include "window.h"
#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#define EYE_Z 1.65f
#define HFOV PI/2
#define VFOV 0.5f
#define ZNEAR 0.0001f
#define ZFAR  128.0f

static SDL_Renderer *renderer = NULL;
static SDL_Texture *screen_texture = NULL;
static uint32_t *screen_buffer = NULL;
static uint32_t screen_buffer_size = 0;
static uint16_t scrnw = 0, scrnh = 0;
static vec2f_t zdl, zdr, near_left, near_right, far_left, far_right;
static vec3f_t camera_pos;
static float camera_angle_cos, camera_angle_sin;

static bool r_init_screen(uint16_t scrn_w, uint16_t scrn_h)
{
    screen_buffer_size = scrn_w * scrn_h * sizeof(uint32_t);
    screen_buffer = malloc(screen_buffer_size);

    if (screen_buffer != NULL)
    {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
        screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, scrn_w, scrn_h);
        memset(screen_buffer, 0, screen_buffer_size);
        
        if (screen_texture != NULL)
            return true;

        free(screen_buffer);
    }

    return false;
}

// see: https://en.wikipedia.org/wiki/Line–line_intersection
// compute intersection of two line segments, returns (NAN, NAN) if there is
// no intersection.
static vec2f_t r_intersect_segs(vec2f_t a0, vec2f_t a1, vec2f_t b0, vec2f_t b1) 
{
    const float d = ((a0.x - a1.x) * (b0.y - b1.y)) - ((a0.y - a1.y) * (b0.x - b1.x));

    if (fabsf(d) < 0.000001f) { return (vec2f_t) { NAN, NAN }; }

    const float t = (((a0.x - b0.x) * (b0.y - b1.y)) - ((a0.y - b0.y) * (b0.x - b1.x))) / d;
    const float u = (((a0.x - b0.x) * (a0.y - a1.y)) - ((a0.y - b0.y) * (a0.x - a1.x))) / d;
    return (t >= 0 && t <= 1 && u >= 0 && u <= 1) ? ((vec2f_t) {
            a0.x + (t * (a1.x - a0.x)),
            a0.y + (t * (a1.y - a0.y)) 
        })
        : ((vec2f_t) { NAN, NAN });
}

static vec2f_t r_rotate(vec2f_t v, float angle) 
{
    return (vec2f_t) {
        (v.x * cos(angle)) - (v.y * sin(angle)),
        (v.x * sin(angle)) + (v.y * cos(angle)),
    };
}

// convert angle in [-(HFOV / 2)..+(HFOV / 2)] to X coordinate
static int r_screen_angle_to_x(float angle) 
{
    return ((int) (scrnw / 2)) * (1.0f - tan(((angle + (HFOV / 2.0)) / HFOV) * PI_2 - PI_4));
}

// noramlize angle to +/-PI
static float r_normalize_angle(float angle) 
{
    return angle - (TAU * floor((angle + PI) / TAU));
}

// world space -> camera space (translate and rotate)
static vec2f_t r_world_pos_to_camera(vec2f_t point) 
{
    const vec2f_t u = { point.x - camera_pos.x, point.y - camera_pos.y };
    return (vec2f_t) {
        u.x * camera_angle_cos - u.y * camera_angle_sin,
        u.x * camera_angle_sin + u.y * camera_angle_cos,
    };
}

static void r_verline(int x, int y0, int y1, uint32_t color) 
{
    for (int y = y0; y <= y1; y++)
        screen_buffer[scrnw * y + x] = color;
}

static uint32_t abgr_mul(uint32_t col, uint32_t alpha) 
{
    const uint32_t br = ((col & 0xFF00FF) * alpha) >> 8;
    const uint32_t g  = ((col & 0x00FF00) * alpha) >> 8;

    return 0xFF000000 | (br & 0xFF00FF) | (g & 0x00FF00);
}

static void r_clip_behind_player(vec2f_t *a, float *z0, vec2f_t b, float z1)
{
    float distance_plane_a = a->y;
    float distance_plane_b = b.y;
    float d = distance_plane_a - distance_plane_b;
    if (d == 0) d = 1;
    float s = distance_plane_a / (distance_plane_a - distance_plane_b);
    a->x = a->x + s * (b.x - a->x);
    a->y = a->y + s * (b.y - a->y);
    if (a->y == 0) a->y = 1;

    *z0 = *z0 + s * (z1 - (*z0));
}

static void r_draw_wall(sector_t *sector, wall_t *wall)
{
    // Transformando as posições do mundo em relação ao player
    vec2f_t op0 = r_world_pos_to_camera(v2i_to_v2f(wall->a));
    vec2f_t op1 = r_world_pos_to_camera(v2i_to_v2f(wall->b));
    vec2f_t op2 = op0;
    vec2f_t op3 = op1;
    float z0 = 0 - camera_pos.z;
    float z1 = 0 - camera_pos.z;
    float z2 = z0 - 5;
    float z3 = z1 - 5;
    
    op0 = (vec2f_t){op0.x * 200/op0.y + scrnw/2, z0*200/op0.y + scrnh/2};
    op1 = (vec2f_t){op1.x * 200/op1.y + scrnw/2, z1*200/op1.y + scrnh/2};
    op2 = (vec2f_t){op2.x * 200/op2.y + scrnw/2, z2*200/op2.y + scrnh/2};
    op3 = (vec2f_t){op3.x * 200/op3.y + scrnw/2, z3*200/op3.y + scrnh/2};
    
    if (op0.y < 1 && op1.y < 1) return;

    if (op0.y < 1)
    {
        r_clip_behind_player(&op0, &z0, op1, z1);
        r_clip_behind_player(&op2, &z2, op3, z3);
    }

    if (op1.y < 1)
    {
        r_clip_behind_player(&op1, &z1, op0, z0);
        r_clip_behind_player(&op3, &z3, op2, z2);
    }

    int dy_bottom = op1.y - op0.y;
    int dy_top = op3.y - op2.y;
    int dx = op1.x - op0.x;
    if (dx == 0) dx = 1;

    int xs = op0.x;

    if (op0.x < 1) op0.x = 1;
    if (op1.x < 1) op1.x = 1;
    if (op0.x > scrnw - 1) op0.x = scrnw - 1;
    if (op1.x > scrnw - 1) op1.x = scrnw - 1;

    for (int x = op0.x; x < op1.x; x++)
    {
        int y1 = dy_bottom * (x - xs + 0.5) / dx + op0.y;
        int y2 = dy_top * (x - xs + 0.5) / dx + op2.y;

        if (y1 < 1) y1 = 1;
        if (y2 < 1) y2 = 1;
        if (y1 > scrnh - 1) y1 = scrnh - 1;
        if (y2 > scrnh - 1) y2 = scrnh - 1;

        for (int y = y1; y < y2; y++)
            screen_buffer[scrnw * y + x] = 0xFF0000FF;
    }

    //if (op0.x > 0 && op0.x < scrnw && op0.y > 0 && op0.y < scrnh) r_verline(op0.x, op0.y, op0.y, 0xFF0000FF);
    //if (op1.x > 0 && op1.x < scrnw && op1.y > 0 && op1.y < scrnh) r_verline(op1.x, op1.y, op1.y, 0xFF0000FF);

    /*
    // wall clipped pos
    vec2f_t cp0 = op0, cp1 = op1;

    // both are negative -> wall is entirely behind player
    if (cp0.y <= 0 && cp1.y <= 0) return;

    // angle-clip against view frustum
    float ap0 = r_normalize_angle(atan2(cp0.y, cp0.x) - PI_2);
    float ap1 = r_normalize_angle(atan2(cp1.y, cp1.x) - PI_2);

    // clip against view frustum if both angles are not clearly within
    // HFOV
    if (cp0.y < ZNEAR || cp1.y < ZNEAR || ap0 > (HFOV / 2) || ap1 < (-HFOV / 2)) 
    {
        const vec2f_t il = r_intersect_segs(cp0, cp1, near_left, far_left);
        const vec2f_t ir = r_intersect_segs(cp0, cp1, near_right, far_right);

        // recompute angles if points change
        if (!isnan(il.x)) 
        {
            cp0 = il;
            ap0 = r_normalize_angle(atan2(cp0.y, cp0.x) - PI_2);
        }

        if (!isnan(ir.x)) 
        {
            cp1 = ir;
            ap1 = r_normalize_angle(atan2(cp1.y, cp1.x) - PI_2);
        }
    }

    if (ap0 < ap1 || (ap0 < (-HFOV / 2) && ap1 < (-HFOV / 2)) || (ap0 > (HFOV / 2) && ap1 > (HFOV / 2))) return;

    // "true" xs before portal clamping
    const int
        tx0 = r_screen_angle_to_x(ap0),
        tx1 = r_screen_angle_to_x(ap1);

    // bounds check against portal window
    if (tx0 > entry.x1 || tx1 < entry.x0) { return; }
    
    const int wallshade =
    16 * (sin(atan2f(
        wall->b.x - wall->a.x,
        wall->b.y - wall->b.y)) + 1.0f);
        
        const int
        x0 = clamp(tx0, entry.x0, entry.x1),
        x1 = clamp(tx1, entry.x0, entry.x1);
        
        const float
        z_floor = sector->z_floor,
        z_ceil = sector->z_ceil,
        nz_floor = wall->is_portal ? state.sectors.arr[wall->portal].z_floor : 0,
        nz_ceil = wall->is_portal ? state.sectors.arr[wall->portal].z_ceil : 0;
        
    const float
    sy0 = ifnan((VFOV * scrnh) / cp0.y, 1e10),
    sy1 = ifnan((VFOV * scrnh) / cp1.y, 1e10);
    
    const int
    yf0  = (scrnh / 2) + (int) (( z_floor - EYE_Z) * sy0),
    yc0  = (scrnh / 2) + (int) (( z_ceil  - EYE_Z) * sy0),
    yf1  = (scrnh / 2) + (int) (( z_floor - EYE_Z) * sy1),
    yc1  = (scrnh / 2) + (int) (( z_ceil  - EYE_Z) * sy1),
    nyf0 = (scrnh / 2) + (int) ((nz_floor - EYE_Z) * sy0),
    nyc0 = (scrnh / 2) + (int) ((nz_ceil  - EYE_Z) * sy0),
    nyf1 = (scrnh / 2) + (int) ((nz_floor - EYE_Z) * sy1),
    nyc1 = (scrnh / 2) + (int) ((nz_ceil  - EYE_Z) * sy1),
    txd = tx1 - tx0,
    yfd = yf1 - yf0,
    ycd = yc1 - yc0,
    nyfd = nyf1 - nyf0,
    nycd = nyc1 - nyc0;
    
    for (int x = x0; x <= x1; x++) 
    {
        int shade = x == x0 || x == x1 ? 192 : (255 - wallshade);
        
        // calculate progress along x-axis via tx{0,1} so that walls
        // which are partially cut off due to portal edges still have
        // proper heights
        const float xp = ifnan((x - tx0) / (float) txd, 0);
        
        // get y coordinates for this x
        const int
        tyf = (int) (xp * yfd) + yf0,
        tyc = (int) (xp * ycd) + yc0,
        yf = clamp(tyf, state.y_lo[x], state.y_hi[x]),
        yc = clamp(tyc, state.y_lo[x], state.y_hi[x]);
        
        // floor
        if (yf > state.y_lo[x])
        verline(x, state.y_lo[x], yf, 0xFFFF0000);
        // ceiling
        if (yc < state.y_hi[x])
        verline(x, yc, state.y_hi[x], 0xFF00FFFF);

        if (wall->is_portal) 
        {
            const int
            tnyf = (int) (xp * nyfd) + nyf0,
            tnyc = (int) (xp * nycd) + nyc0,
            nyf = clamp(tnyf, state.y_lo[x], state.y_hi[x]),
            nyc = clamp(tnyc, state.y_lo[x], state.y_hi[x]);
            
            verline(x, nyc, yc, abgr_mul(0xFF00FF00, shade));
            
            verline(x, yf, nyf, abgr_mul(0xFF0000FF, shade));
            
            state.y_hi[x] = clamp(min(min(yc, nyc), state.y_hi[x]), 0, scrnh - 1);
            
            state.y_lo[x] = clamp(max(max(yf, nyf), state.y_lo[x]), 0, scrnh - 1);
        } 
        else 
        verline(x, yf, yc, abgr_mul(0xFFD0D0D0, shade));
    }
    */
}

bool r_init(uint16_t scrn_w, uint16_t scrn_h)
{
    SDL_Window *win = (SDL_Window*)w_get_handler();
    scrnw = scrn_w;
    scrnh = scrn_h;

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_SOFTWARE);

    if (renderer == NULL)
        return false;

    if (!r_init_screen(scrn_w, scrn_h))
    {
        SDL_DestroyRenderer(renderer);
        return false;
    }

    SDL_RenderSetLogicalSize(renderer, scrn_w, scrn_h);

    // calculate edges of near/far planes (looking down +Y axis)
    zdl = r_rotate(((vec2f_t) { 0.0f, 1.0f }), HFOV / 2.0f);
    zdr = r_rotate(((vec2f_t) { 0.0f, 1.0f }), -HFOV / 2.0f);
    near_left = (vec2f_t) { zdl.x * ZNEAR, zdl.y * ZNEAR };
    near_right = (vec2f_t) { zdr.x * ZNEAR, zdr.y * ZNEAR };
    far_left = (vec2f_t) { zdl.x * ZFAR, zdl.y * ZFAR };
    far_right = (vec2f_t) { zdr.x * ZFAR, zdr.y * ZFAR };
    return true;
}

void r_begin_draw(const player_t *player)
{
    memset(screen_buffer, 0, screen_buffer_size);

    camera_pos = (vec3f_t){ player->position.x, player->position.y, player->position.z };
    camera_angle_cos = player->angle_cos;
    camera_angle_sin = player->angle_sin;
}

void r_draw_sectors(sector_t *sectors, wall_t *walls, queue_sector_t *queue)
{
    for (uint32_t i = 0; i < queue->size; i++) 
    {
        uint32_t id = queue->arr[(queue->front + i) % MAX_QUEUE];

        for (uint32_t j = 0; j < sectors[id].num_walls; j++) 
            r_draw_wall(&sectors[id], &walls[sectors[id].first_wall_id + j]);
    }
}

void r_end_draw()
{
    SDL_UpdateTexture(screen_texture, NULL, screen_buffer, scrnw * sizeof(uint32_t));
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void r_shutdown()
{
    SDL_DestroyTexture(screen_texture);
    free(screen_buffer);
    SDL_DestroyRenderer(renderer);
}
