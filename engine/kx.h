/* KroniX Engine — nagłówek główny (kontrakt platforma <-> gra).
 *
 * Silnik jest wspólny dla gier KroniX i rośnie z każdą grą. Gra:
 *   1. dołącza ten nagłówek (oraz gfx/render.h, audio/kx_audio.h według potrzeb),
 *   2. implementuje cztery funkcje kx_game_* poniżej,
 *   3. linkuje się z engine/core, engine/gfx, engine/audio i jedną platformą (engine/platform).
 * Platforma (Windows: okno + WGL + waveOut, Linux: tryb testowy/OSMesa) woła kx_game_frame()
 * w stałym kroku 60 Hz i kx_game_draw() raz na klatkę ekranu. */
#ifndef KX_H
#define KX_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define KX_VERSION "1.2.0"
#define KX_VERSION_NUM 10200 /* major*10000 + minor*100 + patch */

typedef uint32_t u32;
typedef uint8_t u8;
#define PI_F 3.14159265f
#define CLAMP(v, a, b) ((v) < (a) ? (a) : (v) > (b) ? (b) : (v))
#define RGB(r, g, b) ((u32)(0xFF000000u | ((u32)(r) << 16) | ((u32)(g) << 8) | (u32)(b)))

/* ------------------------------------------------------------------ akcje wejścia
 * Platforma mapuje klawiaturę, mysz i pad na ten stały zestaw akcji. */
enum {
  BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, /* menu + ruch przód/tył */
  BTN_A, BTN_B, BTN_RUN, BTN_FIRE, BTN_RELOAD,
  BTN_SL, BTN_SR, BTN_TL, BTN_TR,       /* krok w bok (A/D), obrót (strzałki) */
  BTN_TAB, BTN_W1, BTN_W2, BTN_W3, BTN_W4, BTN_WNEXT, BTN_N,
  BTN_COUNT
};

/* stan surowy — wypełnia platforma */
extern int g_btn_raw[BTN_COUNT];
extern float g_mouse_dx, g_mouse_dy; /* ruch myszy od ostatniego odczytu (piksele) */
extern int g_mouse_x, g_mouse_y;      /* kursor w przestrzeni UI 1280x720 (-1 poza) */
extern int g_want_mouse_capture;      /* gra prosi o przechwycenie myszy (tryb FPP) */
extern int g_quit;
extern int g_fullscreen_toggle;
extern char g_data_dir[512];          /* katalog obok .exe (zapisy, ustawienia) */
extern int g_render;                  /* jest kontekst GL (0 = testy bez obrazu) */
extern float g_alpha;                 /* 0..1: położenie klatki między krokami logiki */
extern int g_mouse_sens;              /* 1..10 */

/* stan przetworzony — wypełnia input_update() raz na krok logiki */
typedef struct {
  int held[BTN_COUNT], pressed[BTN_COUNT], rep[BTN_COUNT], holdT[BTN_COUNT];
  float mdx, mdy;
  int click, mx, my;
} Input;
extern Input in;
void input_update(void);
void input_clear(void);

int utf8_next(const char **p); /* dekoduje znak UTF-8 i przesuwa wskaźnik */

/* ------------------------------------------------------------------ implementuje gra */
void kx_game_init(int argc, char **argv); /* po utworzeniu kontekstu GL (jeśli g_render) */
void kx_game_frame(void);                 /* logika, stały krok 60 Hz */
void kx_game_draw(void);                  /* obraz: scena 3D + UI */
void kx_game_look(void);                  /* opcjonalnie: obrót myszą w każdej klatce ekranu */

#endif
