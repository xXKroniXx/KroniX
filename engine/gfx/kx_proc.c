/* KroniX Engine — narzędzia proceduralnych tekstur: płótno 512x512 RGBA+wysokość, szum zawijany
 * (value noise, fBm, Worley), prostokąty, koła, relief z mapy wysokości, ziarno, napisy z atlasu SDF,
 * składanie warstw w tablicę tekstur GPU. Gra rysuje warstwy, silnik robi resztę. */
#include "kx.h"
#include "kx_proc.h"
#include "font_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define N KXP_N
KxC4 *kxp_img;
float *kxp_hgt;
static uint8_t *atlasA;
static uint8_t *layers;
static int nlayers;

void kxp_begin(void) {
  kxp_img = (KxC4 *)malloc(sizeof(KxC4) * N * N);
  kxp_hgt = (float *)malloc(sizeof(float) * N * N);
  int na = FONT_ATLAS_W * FONT_ATLAS_H;
  atlasA = (uint8_t *)malloc(na);
  int o = 0;
  for (int i = 0; i + 1 < FONT_RLE_LEN && o < na; i += 2)
    for (int k = 0; k < FONT_RLE[i + 1] && o < na; k++) atlasA[o++] = FONT_RLE[i];
}
void kxp_end(void) {
  free(kxp_img); free(kxp_hgt); free(atlasA); free(layers);
  kxp_img = NULL; kxp_hgt = NULL; atlasA = NULL; layers = NULL;
}
void kxp_layers_begin(int n) { nlayers = n; layers = (uint8_t *)malloc((size_t)N * N * 4 * n); }
void kxp_layer_store(int L) {
  uint8_t *dst = layers + (size_t)L * N * N * 4;
  for (int i = 0; i < N * N; i++) {
    float c[4] = {kxp_img[i].r, kxp_img[i].g, kxp_img[i].b, kxp_img[i].a};
    for (int k = 0; k < 4; k++) dst[i * 4 + k] = (uint8_t)(c[k] < 0 ? 0 : c[k] > 1 ? 255 : c[k] * 255 + 0.5f);
  }
}
unsigned kxp_layers_finish(const char *dumpPpm) {
  if (dumpPpm) { /* podgląd: wszystkie warstwy w siatce 8 kolumn, pomniejszone 4x */
    int cols = 8, rows = (nlayers + 7) / 8, s = N / 4;
    FILE *f = fopen(dumpPpm, "wb");
    if (f) {
      fprintf(f, "P6 %d %d 255\n", cols * s, rows * s);
      for (int y = 0; y < rows * s; y++)
        for (int x = 0; x < cols * s; x++) {
          int L = (y / s) * cols + x / s;
          uint8_t px[3] = {40, 40, 40};
          if (L < nlayers) { uint8_t *q = layers + ((size_t)L * N * N + (size_t)((y % s) * 4) * N + (x % s) * 4) * 4; px[0] = q[0]; px[1] = q[1]; px[2] = q[2]; }
          fwrite(px, 1, 3, f);
        }
      fclose(f);
    }
  }
  unsigned t = g_render ? r_texarray(nlayers, layers) : 0;
  return t;
}

/* ---------------------------------------------------------------- szum (zawijany) */
uint32_t kxp_hash(int x, int y, int s) {
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)s * 2246822519u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}
float kxp_rnd01(int x, int y, int s) { return (kxp_hash(x, y, s) & 0xFFFFFF) / 16777216.0f; }
static float smooth(float t) { return t * t * (3 - 2 * t); }
float kxp_vnoise(float x, float y, int per, int seed) {
  int xi = (int)floorf(x), yi = (int)floorf(y);
  float fx = smooth(x - xi), fy = smooth(y - yi);
  int x0 = ((xi % per) + per) % per, y0 = ((yi % per) + per) % per, x1 = (x0 + 1) % per, y1 = (y0 + 1) % per;
  float a = kxp_rnd01(x0, y0, seed), b = kxp_rnd01(x1, y0, seed), c = kxp_rnd01(x0, y1, seed), d = kxp_rnd01(x1, y1, seed);
  return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
/* u,v w [0,1), per = liczba komórek bazowej oktawy */
float kxp_fbm(float u, float v, int per, int oct, int seed) {
  float s = 0, amp = 0.5f, tot = 0;
  for (int o = 0; o < oct; o++) {
    s += kxp_vnoise(u * per, v * per, per, seed + o * 31) * amp;
    tot += amp;
    amp *= 0.5f;
    per *= 2;
  }
  return s / tot;
}
/* Worley: odległość do najbliższego punktu i id komórki */
float kxp_worley(float u, float v, int per, int seed, float *f2, int *cid) {
  float x = u * per, y = v * per;
  int xi = (int)floorf(x), yi = (int)floorf(y);
  float d1 = 9, d2 = 9;
  int id = 0;
  for (int j = -1; j <= 1; j++)
    for (int i = -1; i <= 1; i++) {
      int cx = xi + i, cy = yi + j;
      int wx = ((cx % per) + per) % per, wy = ((cy % per) + per) % per;
      float px = cx + kxp_rnd01(wx, wy, seed) * 0.8f + 0.1f, py = cy + kxp_rnd01(wx, wy, seed + 7) * 0.8f + 0.1f;
      float dx = px - x, dy = py - y, d = sqrtf(dx * dx + dy * dy);
      if (d < d1) { d2 = d1; d1 = d; id = wx * 977 + wy; }
      else if (d < d2) d2 = d;
    }
  if (f2) *f2 = d2;
  if (cid) *cid = id;
  return d1;
}

/* ---------------------------------------------------------------- operacje na obrazie */
KxC4 kxp_rgb(uint32_t c) { KxC4 r = {((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f, 1}; return r; }
KxC4 kxp_mix(KxC4 a, KxC4 b, float t) { KxC4 r = {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t}; return r; }
KxC4 kxp_mul(KxC4 a, float s) { KxC4 r = {a.r * s, a.g * s, a.b * s, a.a}; return r; }
#define PX(x, y) kxp_img[(((y) & (N - 1)) * N) + ((x) & (N - 1))]
#define HX(x, y) kxp_hgt[(((y) & (N - 1)) * N) + ((x) & (N - 1))]

void kxp_fill(KxC4 c, float gloss) {
  for (int i = 0; i < N * N; i++) { kxp_img[i] = c; kxp_img[i].a = gloss; kxp_hgt[i] = 0; }
}
void kxp_rect(int x0, int y0, int w, int h, KxC4 c, float hgt, int setH) {
  for (int y = y0; y < y0 + h; y++)
    for (int x = x0; x < x0 + w; x++) {
      float a = PX(x, y).a;
      PX(x, y) = c;
      PX(x, y).a = c.a >= 0 ? c.a : a;
      if (setH) HX(x, y) = hgt;
    }
}
void kxp_blendrect(int x0, int y0, int w, int h, KxC4 c, float t) {
  for (int y = y0; y < y0 + h; y++)
    for (int x = x0; x < x0 + w; x++) { float a = PX(x, y).a; PX(x, y) = kxp_mix(PX(x, y), c, t); PX(x, y).a = a; }
}
void kxp_disc(float cx, float cy, float r, KxC4 c, float t) {
  for (int y = (int)(cy - r - 1); y <= (int)(cy + r + 1); y++)
    for (int x = (int)(cx - r - 1); x <= (int)(cx + r + 1); x++) {
      float d = sqrtf((x + 0.5f - cx) * (x + 0.5f - cx) + (y + 0.5f - cy) * (y + 0.5f - cy));
      float k = r + 0.5f - d;
      if (k <= 0) continue;
      if (k > 1) k = 1;
      float a = PX(x, y).a;
      PX(x, y) = kxp_mix(PX(x, y), c, k * t);
      PX(x, y).a = a;
    }
}
/* relief: oświetlenie z mapy wysokości (światło z lewej-góry) */
void kxp_relief(float strength, float ao) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float dx = HX(x + 1, y) - HX(x - 1, y), dy = HX(x, y + 1) - HX(x, y - 1);
      float l = 1.0f + (-dx * 0.6f - dy * 0.8f) * strength;
      float occ = 1.0f - ao * (1.0f - HX(x, y));
      if (l < 0.3f) l = 0.3f;
      if (l > 1.6f) l = 1.6f;
      KxC4 *p = &PX(x, y);
      p->r *= l * occ; p->g *= l * occ; p->b *= l * occ;
    }
}
void kxp_grain(float amt, int per, int seed) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float n = kxp_fbm((float)x / N, (float)y / N, per, 4, seed) - 0.5f;
      KxC4 *p = &PX(x, y);
      float s = 1 + n * amt;
      p->r *= s; p->g *= s; p->b *= s;
    }
}
void kxp_speckle(float amt, int seed) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float n = kxp_rnd01(x, y, seed) - 0.5f;
      KxC4 *p = &PX(x, y);
      p->r += n * amt; p->g += n * amt; p->b += n * amt;
    }
}

/* ---------------------------------------------------------------- tekst w teksturze */
static const FontGlyph *fglyph(int font, int cp) {
  for (int i = 0; i < FONT_NGLYPHS; i++) if (FONT_GLYPHS[i].font == font && FONT_GLYPHS[i].cp == cp) return &FONT_GLYPHS[i];
  return NULL;
}
static int utf8n(const char **p) {
  const unsigned char *s = (const unsigned char *)*p;
  int c = *s;
  if (c < 0x80) { *p += 1; return c; }
  if ((c & 0xE0) == 0xC0) { *p += 2; return ((c & 0x1F) << 6) | (s[1] & 0x3F); }
  if ((c & 0xF0) == 0xE0) { *p += 3; return ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); }
  *p += 1;
  return '?';
}
float kxp_text_w(int font, float size, const char *s, float track) {
  float w = 0, sc = size / FONT_BASE;
  while (*s) { const FontGlyph *g = fglyph(font, utf8n(&s)); if (g) w += g->adv * sc + track; }
  return w;
}
/* rysuje tekst (y = linia bazowa); glow > 0 = poświata neonu */
void kxp_text(int font, float size, float x, float yb, const char *s, KxC4 col, float track, float bold) {
  float sc = size / FONT_BASE;
  while (*s) {
    const FontGlyph *g = fglyph(font, utf8n(&s));
    if (!g) continue;
    if (g->w > 0) {
      float gx = x + g->xoff * sc, gy = yb + (g->yoff + FONT_METRICS[font][0]) * sc - FONT_METRICS[font][0] * sc;
      int x0 = (int)floorf(gx), y0 = (int)floorf(gy), x1 = (int)ceilf(gx + g->w * sc), y1 = (int)ceilf(gy + g->h * sc);
      for (int py = y0; py < y1; py++)
        for (int px = x0; px < x1; px++) {
          if (px < 0 || py < 0 || px >= N || py >= N) continue;
          float u = (px + 0.5f - gx) / sc, v = (py + 0.5f - gy) / sc;
          int ax = g->x + (int)u, ay = g->y + (int)v;
          if (u < 0 || v < 0 || u >= g->w || v >= g->h) continue;
          float d = atlasA[ay * FONT_ATLAS_W + ax] / 255.0f;
          float edge = 0.5f - bold;
          float a = (d - edge) * (FONT_SPREAD * sc * 2.0f) + 0.5f;
          if (a <= 0) continue;
          if (a > 1) a = 1;
          float keep = PX(px, py).a;
          PX(px, py) = kxp_mix(PX(px, py), col, a);
          PX(px, py).a = keep;
        }
    }
    x += g->adv * sc + track;
  }
}
void kxp_text_center(int font, float size, float cx, float yb, const char *s, KxC4 col, float track, float bold) {
  kxp_text(font, size, cx - kxp_text_w(font, size, s, track) * 0.5f, yb, s, col, track, bold);
}

