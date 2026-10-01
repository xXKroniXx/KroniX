/* KroniX Engine — Renderer GPU (OpenGL 3.3 core): matematyka, budowanie siatek, oświetlenie, postprocess, UI 2D. */
#ifndef RENDER_H
#define RENDER_H
#include <math.h>
#include <stdint.h>

/* ------------------------------------------------------------------ matematyka */
typedef struct { float x, y, z; } V3;
typedef struct { float m[16]; } M4; /* kolumnowo, jak w OpenGL */

static inline V3 v3(float x, float y, float z) { V3 r = {x, y, z}; return r; }
static inline V3 v3add(V3 a, V3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline V3 v3sub(V3 a, V3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline V3 v3mul(V3 a, float s) { return v3(a.x * s, a.y * s, a.z * s); }
static inline float v3dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 v3cross(V3 a, V3 b) { return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
static inline float v3len(V3 a) { return sqrtf(v3dot(a, a)); }
static inline V3 v3norm(V3 a) { float l = v3len(a); return l > 1e-8f ? v3mul(a, 1.0f / l) : a; }

M4 m4_identity(void);
M4 m4_mul(M4 a, M4 b);
M4 m4_translate(float x, float y, float z);
M4 m4_scale(float x, float y, float z);
M4 m4_rotx(float a);
M4 m4_roty(float a);
M4 m4_rotz(float a);
M4 m4_perspective(float fovy, float aspect, float zn, float zf);
M4 m4_ortho(float l, float r, float b, float t, float n, float f);
V3 m4_point(M4 m, V3 p);
V3 m4_dir(M4 m, V3 d);

/* ------------------------------------------------------------------ siatki */
enum { VF_ALPHATEST = 1, VF_UNLIT = 2, VF_NOFOG = 4, VF_WATER = 8, VF_FOLIAGE = 16, VF_FIRE = 32 };
typedef struct {
  float p[3], n[3], uv[3]; /* uv[2] = warstwa tablicy tekstur */
  uint8_t c[4];            /* kolor (sRGB) + a = emisja */
  uint8_t bone, flags, pad[2];
} Vert;

typedef struct { Vert *v; int n, cap; } MB; /* budowniczy siatki */
typedef struct { unsigned vao, vbo; int n, cap; float bmin[3], bmax[3]; } GMesh;

void mb_init(MB *b);
void mb_free(MB *b);
void mb_reset(MB *b);
Vert *mb_push(MB *b, int n);
/* wspólne parametry „malowania” kolejnych prymitywów */
typedef struct { uint32_t col; int layer; float emis; int bone; int flags; float uvs; } Paint;
extern Paint P_; /* bieżący pędzel */
void mb_paint(uint32_t col, int layer); /* ustawia kolor i warstwę tekstury, zeruje resztę */
void mb_tri(MB *b, V3 a, V3 c, V3 d, const float *uva, const float *uvc, const float *uvd);
void mb_quad(MB *b, V3 a, V3 c, V3 d, V3 e, float u0, float v0, float u1, float v1); /* a-c-d-e CCW, uv a=(u0,v1) */
void mb_quad_world(MB *b, V3 a, V3 c, V3 d, V3 e); /* uv z położenia w świecie * P_.uvs */
void mb_box(MB *b, V3 mn, V3 mx);                    /* prostopadłościan, uv w skali świata */
void mb_box_faces(MB *b, V3 mn, V3 mx, int faceMask); /* bity: 1 -x, 2 +x, 4 -y, 8 +y, 16 -z, 32 +z */
void mb_cyl(MB *b, V3 base, float r0, float r1, float h, int seg, int caps);
void mb_cyl_x(MB *b, V3 c, float r, float len, int seg, int caps); /* oś X */
void mb_cyl_z(MB *b, V3 c, float r, float len, int seg, int caps); /* oś Z */
void mb_sphere(MB *b, V3 c, float rx, float ry, float rz, int seg);
void mb_rbox(MB *b, V3 mn, V3 mx, float bevel); /* fazowany prostopadłościan (miękki wygląd) */
void mb_append_tf(MB *dst, const MB *src, M4 tf);
void mb_colorize_ao(MB *b, float (*ao)(float x, float y, float z));

GMesh gm_upload(const MB *b);
void gm_update(GMesh *g, const MB *b); /* dynamiczne */
void gm_free(GMesh *g);

/* ------------------------------------------------------------------ renderer */
typedef struct {
  int msaa;        /* 0, 2, 4 */
  int bloom;       /* 0/1 */
  int scale;       /* skala renderu 3D w % (50..100) */
  int maxLights;   /* 4..16 */
  int vsync;
  int fov;         /* stopnie */
} RCfg;
extern RCfg g_cfg;

typedef struct {
  V3 pos;
  float yaw, pitch, fov;
} RCam;

typedef struct {
  float skyTop[3], skyHorizon[3], skyGlow[3];
  float ambSky[3], ambGround[3];
  float sunDir[3], sunCol[3];
  float fogCol[3], fogDensity;
  float wet;        /* 0..1 mokra nawierzchnia */
  float rain;       /* 0..1 intensywność deszczu */
  float exposure;
  int interior;     /* bez nieba */
  float moon;       /* 0..1 widoczność księżyca */
  float stars;
} REnv;

extern int g_winW, g_winH;
extern float g_time;      /* sekundy (animacje shaderów) */
extern M4 g_view, g_proj, g_vp;
extern V3 g_camPos;

int r_init(void);               /* 0 = ok */
extern char g_glError[1024];    /* pierwszy błąd kompilacji shaderów (pusty = brak) */
void r_resize(int w, int h);
void r_frame_begin(const RCam *cam, const REnv *env);
void r_light(float x, float y, float z, float r, float g, float b, float radius);
void r_lights_commit(void);
void r_draw(const GMesh *m, const M4 *model, uint32_t tint);
void r_draw_bones(const GMesh *m, const M4 *model, const M4 *bones, int nbones, uint32_t tint);
void r_glow(float x, float y, float z, float size, float r, float g, float b); /* addytywna poświata */
void r_glow_flush(void);
void r_rain(float intensity);
void r_viewmodel_begin(float fov);
void r_viewmodel_end(void);
M4 r_cam_to_world(void); /* lokalny układ kamery: +x prawo, +y góra, -z przód */
void r_frame_end(void);          /* postprocess na ekran */
int r_project(float x, float y, float z, float *sx, float *sy); /* do UI (współrzędne 1280x720) */
int r_screenshot(const char *path);
void r_grade(float sat, float warm, float vignette, float flash); /* korekcja koloru klatki */

/* tekstury: tablica warstw 512x512 + osobne */
#define TEXSZ 512
unsigned r_texarray(int layers, const uint8_t *rgba); /* RGBA 512x512 x layers */
void r_set_world_textures(unsigned arr);
unsigned r_tex2d(int w, int h, const uint8_t *rgba, int filter); /* RGBA8 */

/* render do tekstury (portrety) */
typedef struct { unsigned fbo, tex, depth; int w, h; } RTarget;
RTarget rt_create(int w, int h);
void rt_begin(RTarget *t, uint32_t clear);
void rt_end(void);
void r_portrait_begin(RTarget *t, V3 eye, V3 target, float fov);
void r_portrait_end(void);

/* ------------------------------------------------------------------ UI 2D (wirtualnie 1280x720) */
#define UI_W 1280
#define UI_H 720
enum { FONT_SANS, FONT_BOLD, FONT_SERIF };
void d2_begin(void);
void d2_end(void);
void d2_rect(float x, float y, float w, float h, uint32_t col);
void d2_grad(float x, float y, float w, float h, uint32_t top, uint32_t bot);
void d2_hgrad(float x, float y, float w, float h, uint32_t left, uint32_t right);
void d2_rrect(float x, float y, float w, float h, float r, uint32_t col);
void d2_rrect_line(float x, float y, float w, float h, float r, float th, uint32_t col);
void d2_shadow(float x, float y, float w, float h, float r, float soft, uint32_t col);
void d2_circle(float cx, float cy, float r, uint32_t col);
void d2_line(float x0, float y0, float x1, float y1, float th, uint32_t col);
void d2_image(unsigned tex, float x, float y, float w, float h, float u0, float v0, float u1, float v1, uint32_t col);
void d2_clip(float x, float y, float w, float h);
void d2_noclip(void);
float d2_text(int font, float size, float x, float y, const char *s, uint32_t col); /* y = góra wiersza */
float d2_text_sh(int font, float size, float x, float y, const char *s, uint32_t col);
float d2_text_w(int font, float size, const char *s);
void d2_text_c(int font, float size, float cx, float y, const char *s, uint32_t col);
void d2_text_r(int font, float size, float xr, float y, const char *s, uint32_t col);
int d2_wrap(int font, float size, const char *s, float maxw, char out[][256], int maxlines);
float d2_line_h(int font, float size);
void d2_text_n(int font, float size, float x, float y, const char *s, uint32_t col, int maxChars);
void d2_tex_init(void);
extern float g_uiScale, g_uiOffX, g_uiOffY; /* odwzorowanie UI -> piksele okna */

#define ARGB(a, r, g, b) ((uint32_t)(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))
#define WITH_A(c, a) (((c) & 0xFFFFFFu) | ((uint32_t)(a) << 24))

#endif
