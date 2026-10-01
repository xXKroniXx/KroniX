/* KroniX Engine — proceduralne tekstury (patrz kx_proc.c). */
#ifndef KX_PROC_H
#define KX_PROC_H
#include <stdint.h>
#include "render.h"

#define KXP_N TEXSZ
typedef struct { float r, g, b, a; } KxC4; /* a = połysk albo wycięcie */
extern KxC4 *kxp_img;  /* bieżące płótno KXP_N x KXP_N */
extern float *kxp_hgt; /* mapa wysokości do reliefu */

void kxp_begin(void);              /* przydziela płótno i dekoduje atlas czcionek */
void kxp_end(void);
void kxp_layers_begin(int n);      /* tablica warstw dla GPU */
void kxp_layer_store(int layer);   /* kopiuje płótno do warstwy */
unsigned kxp_layers_finish(const char *dumpPpm); /* wysyła na GPU (0 bez kontekstu GL) */

/* szum zawijany: u,v w [0,1), per = liczba komórek */
uint32_t kxp_hash(int x, int y, int s);
float kxp_rnd01(int x, int y, int s);
float kxp_vnoise(float x, float y, int per, int seed);
float kxp_fbm(float u, float v, int per, int oct, int seed);
float kxp_worley(float u, float v, int per, int seed, float *f2, int *cid);

KxC4 kxp_rgb(uint32_t c);
KxC4 kxp_mix(KxC4 a, KxC4 b, float t);
KxC4 kxp_mul(KxC4 a, float s);
void kxp_fill(KxC4 c, float gloss);
void kxp_rect(int x0, int y0, int w, int h, KxC4 c, float hgt, int setH); /* c.a < 0 = zachowaj połysk */
void kxp_blendrect(int x0, int y0, int w, int h, KxC4 c, float t);
void kxp_disc(float cx, float cy, float r, KxC4 c, float t);
void kxp_relief(float strength, float ao);
void kxp_grain(float amt, int per, int seed);
void kxp_speckle(float amt, int seed);
float kxp_text_w(int font, float size, const char *s, float track);
void kxp_text(int font, float size, float x, float yb, const char *s, KxC4 col, float track, float bold);
void kxp_text_center(int font, float size, float cx, float yb, const char *s, KxC4 col, float track, float bold);

#define KXP_PX(x, y) kxp_img[(((y) & (KXP_N - 1)) * KXP_N) + ((x) & (KXP_N - 1))]
#define KXP_HX(x, y) kxp_hgt[(((y) & (KXP_N - 1)) * KXP_N) + ((x) & (KXP_N - 1))]

#ifdef KXP_SHORT_NAMES /* krótkie nazwy dla generatorów tekstur gry */
#define C4 KxC4
#define N KXP_N
#define I kxp_img
#define Hm kxp_hgt
#define PX KXP_PX
#define HX KXP_HX
#define hsh kxp_hash
#define rnd01 kxp_rnd01
#define vnoise kxp_vnoise
#define fbm kxp_fbm
#define worley kxp_worley
#define rgb kxp_rgb
#define mixc kxp_mix
#define mulc kxp_mul
#define fill kxp_fill
#define rectc kxp_rect
#define blendrect kxp_blendrect
#define disc kxp_disc
#define relief kxp_relief
#define grain kxp_grain
#define speckle kxp_speckle
#define text_w kxp_text_w
#define text_draw kxp_text
#define text_center kxp_text_center
#endif
#endif
