/* KroniX: Rodzina — warstwy tekstur gry (cegła, asfalt, szyldy, plakaty...). Narzędzia
 * proceduralne (szum, relief, napisy) są w silniku: engine/gfx/kx_proc.h. */
#define KXP_SHORT_NAMES
#include "kx_proc.h"
#include "assets.h"
#include "font_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


/* ---------------------------------------------------------------- warstwy */
static void t_white(void) { fill(rgb(0xE8E8E8), 0.25f); grain(0.06f, 8, 1); }
static void t_cloth(void) {
  fill(rgb(0xD8D8D8), 0.05f);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float w = 0.92f + 0.08f * sinf(x * 1.6f) * sinf(y * 1.6f) + (rnd01(x, y, 3) - 0.5f) * 0.05f;
      PX(x, y).r *= w; PX(x, y).g *= w; PX(x, y).b *= w;
    }
  grain(0.1f, 6, 4);
}
static void t_skin(void) { fill(rgb(0xF2F0EE), 0.3f); grain(0.05f, 12, 5); }

static void t_asphalt(void) {
  fill(rgb(0x2E2F33), 0.32f);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      float n = fbm(u, v, 6, 5, 11);
      C4 c = mixc(rgb(0x26272B), rgb(0x3E3F44), n);
      float sp = rnd01(x, y, 12);
      if (sp > 0.93f) c = mulc(c, 1.25f);
      if (sp < 0.05f) c = mulc(c, 0.75f);
      float f2;
      float w = worley(u, v, 5, 13, &f2, NULL);
      float crack = f2 - w;
      if (crack < 0.035f && fbm(u, v, 9, 3, 14) > 0.55f) c = mulc(c, 0.55f);
      float pud = fbm(u + 0.3f, v, 3, 4, 15);
      float gloss = 0.3f + n * 0.1f;
      if (pud > 0.6f) { float k = (pud - 0.6f) / 0.06f; if (k > 1) k = 1; c = mixc(c, mulc(c, 0.55f), k); gloss = gloss + (1.0f - gloss) * k; }
      c.a = gloss;
      PX(x, y) = c;
    }
}
static void t_sidewalk(void) {
  fill(rgb(0x8A8780), 0.22f);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      int slab = (x / 256) + (y / 256) * 2;
      float n = fbm(u, v, 8, 4, 21 + slab);
      C4 c = mixc(rgb(0x7A776F), rgb(0x9D9990), n);
      c = mulc(c, 0.94f + 0.12f * rnd01(slab, 0, 22));
      float st = fbm(u, v, 3, 3, 23);
      if (st > 0.62f) c = mulc(c, 0.88f);
      int ex = x % 256, ey = y % 256;
      float h = 1;
      if (ex < 3 || ex > 252 || ey < 3 || ey > 252) { c = mulc(c, 0.55f); h = 0.3f; }
      c.a = 0.22f + (st > 0.66f ? 0.5f : 0);
      PX(x, y) = c;
      HX(x, y) = h;
    }
  relief(1.5f, 0.0f);
}
static void t_cobble(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N, f2;
      int id;
      float d1 = worley(u, v, 10, 31, &f2, &id);
      float edge = f2 - d1;
      float h = edge > 0.12f ? 1 : edge / 0.12f;
      h = sqrtf(h);
      C4 c = mixc(rgb(0x4E4A46), rgb(0x77706A), (hsh(id, 0, 32) & 255) / 255.0f);
      c = mulc(c, 0.85f + 0.3f * fbm(u, v, 16, 3, 33));
      if (edge < 0.05f) c = mixc(rgb(0x221F1C), c, edge / 0.05f);
      c.a = edge < 0.05f ? 0.75f : 0.35f + 0.25f * (1 - d1);
      PX(x, y) = c;
      HX(x, y) = h;
    }
  relief(2.5f, 0.35f);
}
static void bricks(uint32_t c0, uint32_t c1, uint32_t mortar, int rows, int cols, int seed) {
  int bh = N / rows, bw = N / cols;
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int row = y / bh;
      int off = (row & 1) ? bw / 2 : 0;
      int col = ((x + off) / bw) % cols;
      int lx = (x + off) % bw, ly = y % bh;
      float u = (float)x / N, v = (float)y / N;
      int mort = lx < 4 || ly < 4;
      C4 c;
      float h;
      if (mort) {
        c = mulc(rgb(mortar), 0.85f + 0.3f * fbm(u, v, 32, 2, seed + 1));
        h = 0.2f;
      } else {
        float t = rnd01(col, row, seed);
        c = mixc(rgb(c0), rgb(c1), t);
        c = mulc(c, 0.82f + 0.3f * fbm(u, v, 24, 4, seed + 2));
        if (rnd01(col, row, seed + 3) > 0.9f) c = mulc(c, 0.75f);
        float e = lx < 7 || ly < 7 || lx > bw - 3 || ly > bh - 3 ? 0.85f : 1.0f;
        h = e - 0.1f * fbm(u, v, 40, 2, seed + 4);
      }
      c.a = 0.12f;
      PX(x, y) = c;
      HX(x, y) = h;
    }
  relief(2.2f, 0.25f);
  /* zacieki od deszczu */
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float s = fbm((float)x / N, (float)y / N * 0.15f, 12, 3, seed + 9);
      if (s > 0.62f) { C4 *p = &PX(x, y); float k = (s - 0.62f) * 2.0f; p->r *= 1 - k; p->g *= 1 - k; p->b *= 1 - k; }
    }
}
static void t_stone(void) {
  int rows = 4, cols = 2;
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int bh = N / rows, bw = N / cols;
      int row = y / bh, off = (row & 1) ? bw / 2 : 0;
      int lx = (x + off) % bw, ly = y % bh, col = ((x + off) / bw) % cols;
      float u = (float)x / N, v = (float)y / N;
      C4 c = mixc(rgb(0xB8AC92), rgb(0xD2C7AE), rnd01(col, row, 41));
      c = mulc(c, 0.86f + 0.22f * fbm(u, v, 16, 5, 42));
      float h = 1;
      if (lx < 3 || ly < 3) { c = mulc(c, 0.6f); h = 0.2f; }
      else if (lx < 8 || ly < 8) h = 0.8f;
      c.a = 0.15f;
      PX(x, y) = c;
      HX(x, y) = h;
    }
  relief(1.8f, 0.2f);
}
static void t_plaster(void) {
  fill(rgb(0xD9CFBB), 0.1f);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      float n = fbm(u, v, 4, 5, 51);
      PX(x, y) = mixc(rgb(0xC9BEA6), rgb(0xE6DDCA), n);
      PX(x, y).a = 0.1f;
      HX(x, y) = fbm(u, v, 32, 3, 52);
    }
  relief(0.8f, 0);
}
static void damask(C4 bg, C4 fg, int seed) {
  fill(bg, 0.12f);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      /* motyw w komórce 128x170, symetria lustrzana */
      int cw = 128, ch = 128;
      int cx = x % cw, cy = y % ch;
      int sh = ((y / ch) & 1) ? cw / 2 : 0;
      cx = (x + sh) % cw;
      float px = fabsf(cx - cw / 2.0f) / (cw / 2.0f), py = (cy - ch / 2.0f) / (ch / 2.0f);
      float m = 0;
      float r1 = sqrtf(px * px * 2.2f + (py + 0.1f) * (py + 0.1f));
      if (r1 < 0.55f && r1 > 0.38f) m = 1;
      if (fabsf(py + 0.1f) < 0.08f && px < 0.75f) m = 1;
      float r2 = sqrtf((px - 0.35f) * (px - 0.35f) * 4 + (py - 0.55f) * (py - 0.55f) * 3);
      if (r2 < 0.35f) m = 1;
      float r3 = sqrtf(px * px * 6 + (py + 0.75f) * (py + 0.75f) * 6);
      if (r3 < 0.5f) m = 1;
      if (px < 0.06f && py > -0.5f && py < 0.9f) m = 1;
      float n = fbm((float)x / N, (float)y / N, 8, 3, seed);
      C4 c = m ? fg : bg;
      c = mulc(c, 0.9f + 0.2f * n);
      c.a = m ? 0.35f : 0.1f;
      PX(x, y) = c;
    }
}
static void t_wall_green(void) {
  fill(rgb(0x2F4A3A), 0.1f);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int sx = x % 64;
      C4 c = sx < 32 ? rgb(0x2E4838) : rgb(0x36533F);
      if (sx == 0 || sx == 1) c = rgb(0xA88B46);
      if (sx >= 46 && sx < 50 && (y % 32) < 4) c = rgb(0xB89A55);
      c = mulc(c, 0.9f + 0.2f * fbm((float)x / N, (float)y / N, 8, 3, 61));
      c.a = 0.12f;
      PX(x, y) = c;
    }
}
static void planks(uint32_t a, uint32_t b, int count, float gloss, int seed, int stagger) {
  int pw = N / count;
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int p = y / pw;
      int joint = stagger ? (int)(rnd01(p, 0, seed) * N) : 0;
      int seg = ((x + joint) % N) / (N / 2);
      float u = (float)x / N, v = (float)y / N;
      float t = rnd01(p, seg, seed + 1);
      C4 c = mixc(rgb(a), rgb(b), t);
      float g = fbm(u * 0.25f + t, v * 4.0f, 8, 4, seed + 2);
      float rings = 0.5f + 0.5f * sinf((v * count * 3.0f + g * 6.0f) * 3.14159f * 2);
      c = mulc(c, 0.8f + 0.25f * g + 0.08f * rings);
      int ly = y % pw, lx = (x + joint) % (N / 2);
      float h = 1;
      if (ly < 2 || (stagger && lx < 2)) { c = mulc(c, 0.45f); h = 0.3f; }
      c.a = gloss;
      PX(x, y) = c;
      HX(x, y) = h;
    }
  relief(1.2f, 0.1f);
}
static void t_parquet(void) {
  int cell = 64;
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int cx = x / cell, cy = y / cell;
      int dir = (cx + cy) & 1;
      float lx = (float)(x % cell) / cell, ly = (float)(y % cell) / cell;
      float a = dir ? lx : ly, along = dir ? ly : lx;
      int strip = (int)(a * 4);
      float g = fbm(along * 0.5f + strip * 0.3f + cx * 0.17f, a * 2 + cy * 0.21f, 8, 4, 71);
      C4 c = mixc(rgb(0x6B4426), rgb(0x9A6A3E), rnd01(cx * 4 + strip, cy, 72));
      c = mulc(c, 0.8f + 0.3f * g);
      float h = 1;
      if (fmodf(a * 4, 1.0f) < 0.05f || x % cell < 1 || y % cell < 1) { c = mulc(c, 0.55f); h = 0.4f; }
      c.a = 0.55f;
      PX(x, y) = c;
      HX(x, y) = h;
    }
  relief(1.0f, 0.1f);
}
static void marble(C4 base, C4 vein, float u, float v, int seed, C4 *out) {
  float n = fbm(u, v, 4, 5, seed);
  float vv = fabsf(sinf((u + v * 0.6f + n * 1.8f) * 9.0f));
  float vk = powf(1 - vv, 8);
  *out = mixc(base, vein, vk * 0.8f);
  *out = mulc(*out, 0.93f + 0.1f * fbm(u, v, 16, 3, seed + 5));
}
static void t_checker(void) {
  int cell = N / 4;
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int w = ((x / cell) + (y / cell)) & 1;
      C4 c;
      marble(w ? rgb(0xE8E2D6) : rgb(0x1E1E22), w ? rgb(0x9A958C) : rgb(0x55555E), (float)x / N, (float)y / N, w ? 81 : 82, &c);
      float h = 1;
      if (x % cell < 2 || y % cell < 2) { c = rgb(0x6A665E); h = 0.4f; }
      c.a = 0.85f;
      PX(x, y) = c;
      HX(x, y) = h;
    }
  relief(0.8f, 0);
}
static void t_marble(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      C4 c;
      marble(rgb(0xE9E4DA), rgb(0xA49E92), (float)x / N, (float)y / N, 85, &c);
      c.a = 0.85f;
      PX(x, y) = c;
    }
}
static void t_carpet(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)(x % 128) / 128 - 0.5f, v = (float)(y % 128) / 128 - 0.5f;
      float d = fabsf(u) + fabsf(v);
      C4 c = rgb(0x6E1620);
      if (d > 0.42f && d < 0.47f) c = rgb(0xB88A3A);
      if (d < 0.14f) c = rgb(0x2A3A5A);
      if (d > 0.2f && d < 0.24f) c = rgb(0x8E2A2E);
      if (fabsf(u) < 0.02f || fabsf(v) < 0.02f) c = mixc(c, rgb(0x4A0E14), 0.5f);
      c = mulc(c, 0.85f + 0.25f * rnd01(x, y, 91) * 0.5f + 0.15f * fbm((float)x / N, (float)y / N, 16, 3, 92));
      c.a = 0.02f;
      PX(x, y) = c;
    }
}
static void t_concrete(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      C4 c = mixc(rgb(0x6C6A66), rgb(0x8C8983), fbm(u, v, 6, 5, 101));
      if (fbm(u, v, 3, 3, 102) > 0.63f) c = mulc(c, 0.8f);
      c.a = 0.2f;
      PX(x, y) = c;
    }
  speckle(0.05f, 103);
}
static void t_roof(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      C4 c = mixc(rgb(0x1E1E20), rgb(0x34322F), fbm(u, v, 16, 4, 111));
      c.a = 0.2f;
      PX(x, y) = c;
    }
  speckle(0.08f, 112);
}
static void t_gravel(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N, f2;
      int id;
      float d = worley(u, v, 48, 121, &f2, &id);
      C4 c = mixc(rgb(0x4A443C), rgb(0x7A7266), (hsh(id, 1, 122) & 255) / 255.0f);
      c = mulc(c, 0.7f + 0.5f * (1 - d));
      c = mixc(c, rgb(0x3A332A), fbm(u, v, 4, 4, 123) * 0.5f);
      c.a = 0.15f + (fbm(u, v, 3, 3, 124) > 0.62f ? 0.6f : 0);
      PX(x, y) = c;
      HX(x, y) = 1 - d;
    }
  relief(1.2f, 0.2f);
}

/* okno: szprosy, firanki po bokach, odbicie */
static void t_window(void) {
  fill(rgb(0xBFC8D0), 0.95f);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      C4 c = mixc(rgb(0xC9D2DA), rgb(0x8E98A4), v);
      float refl = 0.5f + 0.5f * sinf((u * 1.3f - v) * 6.0f);
      c = mulc(c, 0.85f + 0.2f * refl);
      /* firanki */
      float cur = 0;
      if (u < 0.22f) cur = 1 - u / 0.22f;
      if (u > 0.78f) cur = (u - 0.78f) / 0.22f;
      if (cur > 0) {
        float fold = 0.75f + 0.25f * sinf(u * 90.0f);
        c = mixc(c, mulc(rgb(0xF2E2C2), fold), 0.85f * (cur > 0.3f ? 1 : cur / 0.3f));
      }
      c.a = 0.95f;
      PX(x, y) = c;
    }
  /* szprosy (ciemne) */
  C4 m = rgb(0x1A1714);
  m.a = 0.3f;
  rectc(0, 0, N, 14, m, 0, 0);
  rectc(0, N - 14, N, 14, m, 0, 0);
  rectc(0, 0, 14, N, m, 0, 0);
  rectc(N - 14, 0, 14, N, m, 0, 0);
  rectc(0, N / 2 - 10, N, 20, m, 0, 0);
  rectc(N / 2 - 6, 0, 12, N, m, 0, 0);
  rectc(0, N / 4 - 4, N, 8, m, 0, 0);
  rectc(0, 3 * N / 4 - 4, N, 8, m, 0, 0);
}
static void t_shopwin(void) {
  fill(rgb(0x2A2620), 0.9f);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float v = (float)y / N;
      C4 c = mixc(rgb(0x6A5A44), rgb(0x2A2218), v);
      c.a = 0.9f;
      PX(x, y) = c;
    }
  /* półki z towarem */
  for (int s = 0; s < 3; s++) {
    int sy = 150 + s * 120;
    rectc(20, sy, N - 40, 10, rgb(0x5A3A22), 0, 0);
    for (int k = 0; k < 9; k++) {
      int gx = 40 + k * 50 + (int)(rnd01(k, s, 131) * 10);
      int gh = 30 + (int)(rnd01(k, s, 132) * 50);
      uint32_t cols[6] = {0xC8A050, 0x8A3A2A, 0x3A6A8A, 0xD8D0B0, 0x6A8A3A, 0xA05A8A};
      C4 g = rgb(cols[hsh(k, s, 133) % 6]);
      if (rnd01(k, s, 134) > 0.5f) disc((float)gx + 15, (float)sy - gh / 2, gh / 2.2f, g, 1);
      else rectc(gx, sy - gh, 30, gh, g, 0, 0);
    }
  }
  /* odblask szyby */
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      float r = fmaxf(0, sinf((u * 1.5f + v) * 5.0f));
      C4 *p = &PX(x, y);
      p->r += r * 0.08f; p->g += r * 0.09f; p->b += r * 0.1f;
    }
  C4 m = rgb(0x18120C);
  m.a = 0.3f;
  rectc(0, 0, N, 12, m, 0, 0);
  rectc(0, N - 12, N, 12, m, 0, 0);
  rectc(0, 0, 12, N, m, 0, 0);
  rectc(N - 12, 0, 12, N, m, 0, 0);
  rectc(N / 2 - 5, 0, 10, N, m, 0, 0);
}
static void t_door(void) {
  planks(0x4A2E1A, 0x5A3A22, 1, 0.4f, 141, 0);
  for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) HX(x, y) = 0.6f;
  /* płyciny */
  for (int k = 0; k < 2; k++) {
    int px = 60 + k * 210, py = 280;
    for (int y = py; y < py + 190; y++)
      for (int x = px; x < px + 180; x++) {
        int e = (x - px < 12 || y - py < 12 || px + 180 - x < 12 || py + 190 - y < 12);
        HX(x, y) = e ? 0.85f : 0.5f;
      }
  }
  /* szyba w górnej części */
  for (int y = 50; y < 230; y++)
    for (int x = 60; x < 450; x++) {
      float u = (float)(x - 60) / 390, v = (float)(y - 50) / 180;
      C4 c = mixc(rgb(0xD8C590), rgb(0x8A7A50), v);
      c = mulc(c, 0.85f + 0.2f * sinf((u - v) * 8));
      c.a = 0.9f;
      PX(x, y) = c;
      HX(x, y) = 0.4f;
    }
  rectc(250, 50, 10, 180, rgb(0x2A1A10), 0.7f, 1);
  relief(2.0f, 0.2f);
  disc(430, 300, 13, rgb(0xC9A040), 1);
  disc(427, 297, 5, rgb(0xF2DA8A), 1);
}
static void t_awning(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int stripe = (x / 64) & 1;
      C4 c = stripe ? rgb(0xE6DCC8) : rgb(0x8E2228);
      float v = (float)y / N;
      c = mulc(c, 0.75f + 0.3f * (1 - v));
      c = mulc(c, 0.92f + 0.12f * fbm((float)x / N, v, 16, 3, 151));
      /* falbana: wycięcie */
      float sc = 0.86f + 0.08f * fabsf(sinf((x % 64) / 64.0f * 3.14159f));
      c.a = v > sc ? 0 : 1;
      PX(x, y) = c;
    }
}
static void t_wood(uint32_t a, uint32_t b, float gloss, int seed) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      float g = fbm(u * 0.3f, v * 3.0f, 8, 5, seed);
      float r = 0.5f + 0.5f * sinf((v * 14.0f + g * 9.0f) * 3.14159f);
      C4 c = mixc(rgb(a), rgb(b), r * 0.6f + g * 0.4f);
      c.a = gloss;
      PX(x, y) = c;
    }
}
static void t_metal(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      C4 c = mixc(rgb(0x2A2C30), rgb(0x44474D), fbm(u, v, 6, 4, 161));
      float scr = fbm(u * 8, v * 0.3f, 16, 2, 162);
      if (scr > 0.7f) c = mulc(c, 1.3f);
      c.a = 0.55f;
      PX(x, y) = c;
    }
}
static void t_brass(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      C4 c = mixc(rgb(0x8A6A2A), rgb(0xD8B460), fbm(u, v, 4, 4, 171));
      c.a = 0.85f;
      PX(x, y) = c;
    }
}
static void t_crate(void) {
  planks(0x9A7A4E, 0xB89462, 5, 0.12f, 181, 0);
  C4 f = rgb(0x6E5232);
  rectc(0, 0, N, 30, f, 1, 1);
  rectc(0, N - 30, N, 30, f, 1, 1);
  rectc(0, 0, 30, N, f, 1, 1);
  rectc(N - 30, 0, 30, N, f, 1, 1);
  for (int k = -14; k <= 14; k++)
    for (int t = 30; t < N - 30; t++) { PX(t + k, t) = mulc(f, 1.05f); HX(t + k, t) = 1; }
  relief(1.5f, 0.1f);
  text_center(FONT_BOLD, 50, N / 2, 300, "MAPLE SYRUP", rgb(0x2A1A0E), 2, 0.05f);
  text_center(FONT_BOLD, 34, N / 2, 350, "ONTARIO \xe2\x80\xa2 CANADA", rgb(0x3A2410), 1, 0.05f);
}
static void t_barrel(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int st = x / 42;
      float u = (float)x / N, v = (float)y / N;
      C4 c = mixc(rgb(0x5E3A1E), rgb(0x7E5230), rnd01(st, 0, 191));
      c = mulc(c, 0.8f + 0.3f * fbm(u * 0.4f, v * 3, 8, 4, 192));
      float h = x % 42 < 2 ? 0.3f : 1;
      if (x % 42 < 2) c = mulc(c, 0.5f);
      if ((y > 70 && y < 100) || (y > 412 && y < 442)) { c = mixc(rgb(0x3A3A3E), rgb(0x55555C), fbm(u, v, 16, 2, 193)); h = 1.2f; }
      c.a = 0.3f;
      PX(x, y) = c;
      HX(x, y) = h;
    }
  relief(1.5f, 0.1f);
}
static void t_corrugated(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      float w = sinf(x / 512.0f * 3.14159f * 2 * 16);
      C4 c = mixc(rgb(0x5A6068), rgb(0x7A8088), fbm(u, v, 4, 4, 201));
      float rust = fbm(u, v * 0.3f, 8, 4, 202);
      if (rust > 0.55f) c = mixc(c, rgb(0x7A4A2A), (rust - 0.55f) * 3);
      c.a = 0.4f;
      PX(x, y) = c;
      HX(x, y) = 0.5f + 0.5f * w;
    }
  relief(1.5f, 0.1f);
}
static void t_water(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      float n = fbm(u, v, 8, 5, 211);
      C4 c = mixc(rgb(0x0E1A22), rgb(0x24404E), n);
      c.a = 1;
      PX(x, y) = c;
      HX(x, y) = n;
    }
  relief(3.0f, 0);
}
static void t_grass(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      C4 c = mixc(rgb(0x2E4422), rgb(0x52703A), fbm(u, v, 12, 5, 221));
      c = mulc(c, 0.85f + 0.3f * rnd01(x, y, 222));
      c.a = 0.2f;
      PX(x, y) = c;
    }
}
static void t_tin(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)(x % 128) / 128 - 0.5f, v = (float)(y % 128) / 128 - 0.5f;
      float d = fmaxf(fabsf(u), fabsf(v));
      float r = sqrtf(u * u + v * v);
      float h = 0.5f;
      if (d > 0.44f) h = 0.9f;
      else if (d > 0.38f) h = 0.3f;
      if (r < 0.22f) h = 0.8f - r;
      if (fabsf(fabsf(u) - fabsf(v)) < 0.03f && d < 0.38f) h = 0.7f;
      C4 c = rgb(0xD9CDB0);
      c.a = 0.5f;
      PX(x, y) = c;
      HX(x, y) = h;
    }
  relief(2.5f, 0.15f);
}
static void t_wainscot(void) {
  t_wood(0x3A2414, 0x5A3820, 0.5f, 231);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int lx = x % 256;
      float h = 0.8f;
      if (lx > 30 && lx < 226 && y > 60 && y < 452) h = (lx < 44 || lx > 212 || y < 74 || y > 438) ? 0.6f : 0.5f;
      HX(x, y) = h;
    }
  rectc(0, 0, N, 26, rgb(0x2A1A0E), 1, 1);
  relief(2.0f, 0.2f);
}
static void t_tile_white(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int bw = 128, bh = 64;
      int row = y / bh, off = (row & 1) ? bw / 2 : 0;
      int lx = (x + off) % bw, ly = y % bh;
      C4 c = mulc(rgb(0xE8E6DE), 0.95f + 0.08f * rnd01((x + off) / bw, row, 241));
      float h = 1;
      if (lx < 3 || ly < 3) { c = rgb(0x9A968C); h = 0.3f; }
      c.a = 0.8f;
      PX(x, y) = c;
      HX(x, y) = h;
    }
  relief(1.5f, 0);
}
static void t_tablecloth(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int a = (x / 64) & 1, b = (y / 64) & 1;
      C4 c = (a && b) ? rgb(0xB0232C) : (a || b) ? rgb(0xD88A8C) : rgb(0xF2EEE6);
      c = mulc(c, 0.92f + 0.08f * sinf(x * 1.3f) * sinf(y * 1.3f));
      c.a = 0.1f;
      PX(x, y) = c;
    }
}
static void t_leather(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N, f2;
      float d = worley(u, v, 40, 251, &f2, NULL);
      C4 c = mixc(rgb(0x3A1E12), rgb(0x5E3420), fbm(u, v, 6, 4, 252));
      c = mulc(c, 0.85f + 0.2f * (f2 - d));
      c.a = 0.6f;
      PX(x, y) = c;
    }
}
static void t_velvet(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N;
      float fold = 0.55f + 0.45f * sinf(u * 3.14159f * 2 * 6 + sinf(u * 20) * 0.4f);
      C4 c = mulc(rgb(0x7A1018), 0.5f + 0.7f * fold);
      c.a = 0.25f;
      PX(x, y) = c;
    }
}
static void t_books(void) {
  fill(rgb(0x2A1A10), 0.3f);
  for (int s = 0; s < 4; s++) {
    int y0 = s * 128;
    rectc(0, y0 + 116, N, 12, rgb(0x3E2614), 0, 0);
    int x = 0;
    while (x < N) {
      int w = 14 + (int)(rnd01(x, s, 261) * 22);
      int h = 70 + (int)(rnd01(x, s, 262) * 40);
      uint32_t cols[7] = {0x6A1A1A, 0x1A3A5A, 0x2A4A2A, 0x5A4A2A, 0x3A2A4A, 0x7A5A2A, 0x1E1E22};
      C4 c = rgb(cols[hsh(x, s, 263) % 7]);
      c = mulc(c, 0.8f + 0.3f * rnd01(x, s, 264));
      c.a = 0.35f;
      if (x + w > N) w = N - x;
      rectc(x, y0 + 116 - h, w - 1, h, c, 0, 0);
      rectc(x + 2, y0 + 116 - h + 12, w - 5, 4, rgb(0xC8A050), 0, 0);
      rectc(x + 2, y0 + 116 - 20, w - 5, 3, rgb(0xC8A050), 0, 0);
      x += w;
    }
  }
}
static void t_stained(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N, f2;
      int id;
      float d = worley(u, v, 6, 271, &f2, &id);
      uint32_t cols[6] = {0x8A1A2A, 0x1A3A8A, 0xC89A2A, 0x2A7A3A, 0x6A2A8A, 0x2A8A8A};
      C4 c = rgb(cols[hsh(id, 2, 272) % 6]);
      c = mulc(c, 0.75f + 0.4f * (1 - d));
      if (f2 - d < 0.06f) c = rgb(0x141414);
      float ax = fabsf(u - 0.5f), ay = v;
      if (ax > 0.42f || (ay < 0.25f && sqrtf((u - 0.5f) * (u - 0.5f) + (ay - 0.3f) * (ay - 0.3f) * 1.3f) > 0.42f)) c = rgb(0x221E1A);
      c.a = 0.8f;
      PX(x, y) = c;
    }
}

/* twarze: 4x4 kafelki po 128 px; tło białe (kolor skóry z wierzchołków) */
static void face(int idx) {
  int ox = (idx % 4) * 128, oy = (idx / 4) * 128;
  int female = idx == 4 || idx == 5 || idx == 11;
  int old = idx == 3 || idx == 5;
  int asian = idx == 7;
  for (int y = 0; y < 128; y++)
    for (int x = 0; x < 128; x++) {
      float u = x / 128.0f, v = y / 128.0f;
      C4 c = rgb(0xF4F0EC);
      /* lekkie cieniowanie policzków i nosa */
      float cheek = expf(-((u - 0.5f) * (u - 0.5f) * 18 + (v - 0.62f) * (v - 0.62f) * 30));
      c = mulc(c, 0.93f + 0.05f * cheek);
      if (fabsf(u - 0.5f) < 0.06f && v > 0.42f && v < 0.68f) c = mulc(c, 0.93f);
      if (v > 0.66f && v < 0.7f && fabsf(u - 0.5f) < 0.07f) c = mulc(c, 0.8f);
      c.a = 0.3f;
      PX(ox + x, oy + y) = c;
    }
  float ey = 0.43f, ex = 0.2f, er = asian ? 5.5f : 7.5f;
  for (int s = -1; s <= 1; s += 2) {
    float cx = ox + 64 + s * ex * 128, cy = oy + ey * 128;
    disc(cx, cy, er + 2, rgb(0xFFFFFF), 1);
    disc(cx + s * 0.5f, cy + 0.5f, er * 0.65f, idx == 9 ? rgb(0x2A1A10) : rgb(0x3A2A1E), 1);
    disc(cx, cy, er * 0.35f, rgb(0x0A0808), 1);
    disc(cx - 1.5f, cy - 1.5f, 1.4f, rgb(0xFFFFFF), 0.9f);
    if (female) { for (int k = 0; k < 4; k++) disc(cx + s * (er - k * 2.5f), cy - er - 1, 1.6f, rgb(0x1A1210), 1); }
    /* brwi */
    float by = cy - er - (female ? 8 : 6);
    float tilt = idx == 9 ? 0.35f : idx == 15 ? -0.1f : 0.08f;
    for (int k = -10; k <= 10; k++) {
      float bx = cx + k * 1.0f;
      float yy = by + (k * s) * tilt * -1;
      disc(bx, yy, female ? 1.4f : (old ? 2.6f : 2.3f), old ? rgb(0xB0ACA8) : rgb(0x2A1E16), 0.95f);
    }
    if (old) for (int k = 0; k < 3; k++) for (int t = 0; t < 8; t++) disc(cx + s * (er + 4 + t), cy + k * 3 - 2 + t * 0.3f, 0.6f, rgb(0xC8B8A8), 0.6f);
  }
  /* usta */
  float my = oy + 0.77f * 128;
  for (int k = -12; k <= 12; k++) {
    float curve = idx == 15 ? -fabsf((float)k) * 0.25f + 3 : idx == 9 ? fabsf((float)k) * 0.15f : 0;
    float mx = ox + 64 + k;
    if (female) disc(mx, my - curve * 0.5f, 3.2f - fabsf((float)k) * 0.12f, rgb(0xB0283A), 1);
    else disc(mx, my - curve * 0.5f, 1.5f, rgb(0x8A4A40), 0.9f);
  }
  if (idx == 1 || idx == 3) for (int k = -16; k <= 16; k++) disc(ox + 64 + k, my - 8 + fabsf((float)k) * 0.15f, 3.6f - fabsf((float)k) * 0.1f, old ? rgb(0xC8C4C0) : rgb(0x2A1C12), 1);
  if (idx == 8) for (int k = -13; k <= 13; k++) if (abs(k) > 2) disc(ox + 64 + k, my - 7, 1.3f, rgb(0x1A1210), 1);
  if (idx == 2) for (int y = 84; y < 128; y++) for (int x = 20; x < 108; x++) if (rnd01(x, y, 281) > 0.55f) blendrect(ox + x, oy + y, 1, 1, rgb(0x5A4A40), 0.35f);
  if (idx == 10) for (int k = 0; k < 26; k++) disc(ox + 84 + k * 0.3f, oy + 40 + k, 1.2f, rgb(0xB06A6A), 0.8f);
  if (idx == 11) disc(ox + 82, oy + 92, 2.2f, rgb(0x2A1A10), 1);
  if (idx == 14) { /* dziecko: piegi */ for (int k = 0; k < 18; k++) disc(ox + 34 + rnd01(k, 1, 282) * 60, oy + 66 + rnd01(k, 2, 283) * 14, 1.2f, rgb(0xC08A60), 0.7f); }
}
static void t_faces(void) {
  fill(rgb(0xF4F0EC), 0.3f);
  for (int i = 0; i < 16; i++) face(i);
}
static void t_foliage(void) {
  for (int i = 0; i < N * N; i++) { I[i].r = 0.2f; I[i].g = 0.3f; I[i].b = 0.15f; I[i].a = 0; }
  for (int k = 0; k < 900; k++) {
    float cx = rnd01(k, 0, 291) * N, cy = rnd01(k, 1, 291) * N, r = 10 + rnd01(k, 2, 291) * 18;
    C4 c = mixc(rgb(0x24401E), rgb(0x5A7A30), rnd01(k, 3, 291));
    for (int y = (int)(cy - r); y <= (int)(cy + r); y++)
      for (int x = (int)(cx - r); x <= (int)(cx + r); x++) {
        float dx = (x - cx) / r, dy = (y - cy) / (r * 0.6f);
        if (dx * dx + dy * dy > 1) continue;
        C4 cc = mulc(c, 0.8f + 0.3f * (1 - dy));
        cc.a = 1;
        PX(x, y) = cc;
      }
  }
}
static void t_fence(void) {
  for (int i = 0; i < N * N; i++) { I[i] = rgb(0x1A1A1E); I[i].a = 0; }
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int lx = x % 64;
      int bar = lx >= 28 && lx < 36 && y > 40;
      int tip = y <= 40 && y > 4 && abs(lx - 32) < (y - 4) / 4;
      int rail = (y > 80 && y < 96) || (y > 440 && y < 456);
      if (bar || tip || rail) { PX(x, y) = mulc(rgb(0x2A2A30), 0.8f + 0.4f * (lx % 8) / 8.0f); PX(x, y).a = 1; }
    }
}
static void t_grate(void) {
  for (int i = 0; i < N * N; i++) { I[i] = rgb(0x22252A); I[i].a = 0; }
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++)
      if (x % 32 < 6 || y % 32 < 6) { PX(x, y) = mulc(rgb(0x30343A), 0.9f + 0.2f * rnd01(x, y, 301)); PX(x, y).a = 1; }
}
static void t_skyline(void) {
  fill(rgb(0x14161C), 0.1f);
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      int wx = x / 16, wy = y / 24;
      int lx = x % 16, ly = y % 24;
      C4 c = mulc(rgb(0x1A1C24), 0.9f + 0.2f * fbm((float)x / N, (float)y / N, 8, 3, 311));
      if (lx > 4 && lx < 12 && ly > 6 && ly < 18) {
        float r = rnd01(wx, wy, 312);
        if (r > 0.62f) c = mixc(rgb(0xF0B860), rgb(0xFFE0A0), rnd01(wx, wy, 313));
        else c = rgb(0x22252E);
      }
      c.a = 0.1f;
      PX(x, y) = c;
    }
}
static void t_newspaper(void) {
  fill(rgb(0xE6DFCC), 0.05f);
  grain(0.08f, 8, 321);
  text_center(FONT_SERIF, 40, N / 2, 64, "CHICAGO DAILY HERALD", rgb(0x141210), 0, 0.03f);
  rectc(20, 84, N - 40, 3, rgb(0x141210), 0, 0);
  text_center(FONT_SERIF, 30, N / 2, 130, "GANG WAR ON THE SOUTH SIDE", rgb(0x141210), 0, 0.04f);
  for (int col = 0; col < 3; col++)
    for (int l = 0; l < 24; l++) {
      int w = 140 - (int)(rnd01(col, l, 322) * 30);
      rectc(24 + col * 160, 170 + l * 13, w, 5, rgb(0x5A5650), 0, 0);
    }
}
static void t_mapwall(void) {
  fill(rgb(0xD8CCAA), 0.1f);
  grain(0.1f, 6, 331);
  for (int y = 0; y < N; y++) for (int x = 380; x < N; x++) PX(x, y) = mixc(rgb(0x7A9AB0), rgb(0x5A7A94), (float)(x - 380) / 132);
  for (int k = 0; k < N; k += 32) { rectc(k, 0, 2, N, rgb(0x9A8A6A), 0, 0); rectc(0, k, 380, 2, rgb(0x9A8A6A), 0, 0); }
  for (int y = 0; y < N; y++) { int x = 120 + (int)(30 * sinf(y * 0.012f)); rectc(x, y, 8, 1, rgb(0x6A8AA0), 0, 0); }
  disc(200, 220, 14, rgb(0xB02A2A), 1);
  disc(300, 140, 10, rgb(0x2A6A2A), 1);
  disc(250, 380, 10, rgb(0x2A2A8A), 1);
  text_draw(FONT_SERIF, 30, 20, 40, "CHICAGO", rgb(0x3A2A1A), 2, 0.03f);
}
static void t_bottles(void) {
  fill(rgb(0x2A1C12), 0.4f);
  for (int s = 0; s < 4; s++) {
    int base = s * 128 + 120;
    rectc(0, base, N, 8, rgb(0x4A2E1A), 0, 0);
    for (int k = 0; k < 16; k++) {
      float cx = 16 + k * 32 + rnd01(k, s, 341) * 6;
      uint32_t cols[5] = {0x2A5A2A, 0x6A3A12, 0x8A7A5A, 0x3A2A1A, 0xA0A8A0};
      C4 c = rgb(cols[hsh(k, s, 342) % 5]);
      int h = 70 + (int)(rnd01(k, s, 343) * 25);
      for (int y = base - h; y < base; y++)
        for (int x = (int)cx - 11; x <= (int)cx + 11; x++) {
          float t = (float)(y - (base - h)) / h;
          float w = t < 0.35f ? 4 : t < 0.45f ? 4 + (t - 0.35f) * 70 : 11;
          if (fabsf(x - cx) > w) continue;
          float sh = 0.7f + 0.5f * (1 - fabsf(x - cx) / w);
          C4 cc = mulc(c, sh);
          if (fabsf(x - cx + w * 0.4f) < 1.5f) cc = mulc(cc, 1.6f);
          if (t > 0.55f && t < 0.8f) cc = mixc(cc, rgb(0xE8DCC0), 0.85f);
          cc.a = 0.8f;
          PX(x, y) = cc;
        }
    }
  }
}
static void t_painting(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      C4 c;
      float hz = 0.55f + 0.05f * sinf(u * 9) * fbm(u, 0, 4, 3, 351);
      if (v < hz) c = mixc(rgb(0xE8B070), rgb(0x5A7AA0), v / hz * 1.2f > 1 ? 1 : v / hz * 1.2f);
      else c = mixc(rgb(0x2A5A7A), rgb(0x1A3A4A), (v - hz) * 3);
      if (v > hz - 0.12f && v < hz && u > 0.55f && u < 0.85f && fbm(u, v, 8, 3, 352) > 0.45f) c = rgb(0xC8A878);
      c = mulc(c, 0.9f + 0.15f * fbm(u, v, 32, 3, 353));
      if (x < 40 || y < 40 || x > N - 40 || y > N - 40) c = mixc(rgb(0x8A6A2A), rgb(0xD8B460), fbm(u, v, 16, 3, 354));
      c.a = (x < 40 || y < 40 || x > N - 40 || y > N - 40) ? 0.8f : 0.2f;
      PX(x, y) = c;
    }
}
static void t_fire(void) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      float n = fbm(u, v * 0.7f, 6, 5, 361);
      float t = (1 - v) * 1.3f - n * 0.9f;
      C4 c = t > 0.5f ? mixc(rgb(0xFF9A30), rgb(0xFFE8A0), (t - 0.5f) * 2) : mixc(rgb(0x200400), rgb(0xFF6010), t * 2);
      c.a = 1;
      PX(x, y) = c;
    }
}
static void poster(const char *top, const char *mid, const char *bot, uint32_t bg, uint32_t fg, uint32_t acc, int seed) {
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) {
      float u = (float)x / N, v = (float)y / N;
      C4 c = mixc(rgb(bg), mulc(rgb(bg), 0.6f), v);
      float ray = fabsf(atan2f(u - 0.5f, v - 0.85f));
      if (fmodf(ray * 8, 1.0f) < 0.5f && v < 0.8f) c = mulc(c, 1.12f);
      c = mulc(c, 0.9f + 0.15f * fbm(u, v, 8, 3, seed));
      if (fbm(u, v, 4, 4, seed + 1) > 0.7f) c = mixc(c, rgb(0xD8CCB0), 0.12f);
      c.a = 0.15f;
      PX(x, y) = c;
    }
  C4 F = rgb(fg), A = rgb(acc);
  rectc(18, 18, N - 36, 6, A, 0, 0);
  rectc(18, N - 24, N - 36, 6, A, 0, 0);
  text_center(FONT_SERIF, 46, N / 2, 110, top, F, 3, 0.04f);
  text_center(FONT_SERIF, 110, N / 2, 300, mid, A, 4, 0.06f);
  text_center(FONT_BOLD, 40, N / 2, 420, bot, F, 3, 0.03f);
}

const char *SIGN_TEXT[16] = {
  "BAKERY", "DELICATESSEN", "BARBER SHOP", "PAWN SHOP", "HARDWARE", "LAUNDRY", "TOBACCO", "PHARMACY",
  "TRATTORIA BELLA NAPOLI", "O'HARA'S PUB", "HOTEL LEXINGTON", "TAILOR", "GROCERY", "CHOP SUEY", "TEA HOUSE", "GINO'S TOOLS"};
static void t_signs(void) {
  for (int r = 0; r < 8; r++)
    for (int c = 0; c < 2; c++) {
      int x0 = c * 256, y0 = r * 64;
      int idx = r * 2 + c;
      uint32_t bgs[4] = {0x1E2A22, 0x2A1A1E, 0x1A1E2A, 0x2A2418};
      for (int y = y0; y < y0 + 64; y++)
        for (int x = x0; x < x0 + 256; x++) {
          C4 cc = mulc(rgb(bgs[idx % 4]), 0.85f + 0.2f * fbm((float)x / N, (float)y / N, 16, 3, 371));
          if (y - y0 < 4 || y - y0 > 59 || x - x0 < 4 || x - x0 > 251) cc = rgb(0xB89A55);
          cc.a = 0.5f;
          PX(x, y) = cc;
        }
      const char *t = SIGN_TEXT[idx];
      float sz = 40;
      while (text_w(FONT_SERIF, sz, t, 2) > 236 && sz > 14) sz -= 1;
      text_center(FONT_SERIF, sz, x0 + 128, y0 + 32 + sz * 0.35f, t, rgb(0xE8C878), 2, 0.04f);
    }
}
static void t_neon(void) {
  fill(rgb(0x0A0A0C), 0.3f);
  const char *w[8] = {"JAZZ", "BAR", "HOTEL", "CAFE", "DANCING", "OPEN", "BILLIARDS", "CASINO"};
  uint32_t cols[8] = {0x40D0FF, 0xFF4060, 0xFFC040, 0x60FF90, 0xFF60D0, 0xFF5030, 0x60A0FF, 0xFFD060};
  for (int i = 0; i < 8; i++) {
    int y0 = i * 64;
    float sz = 50;
    while (text_w(FONT_BOLD, sz, w[i], 4) > 480) sz -= 2;
    /* poświata */
    for (int g = 3; g >= 1; g--) text_center(FONT_BOLD, sz, N / 2, y0 + 50, w[i], mulc(rgb(cols[i]), 0.25f), 4, 0.06f * g);
    text_center(FONT_BOLD, sz, N / 2, y0 + 50, w[i], mixc(rgb(cols[i]), rgb(0xFFFFFF), 0.45f), 4, 0.0f);
  }
}

/* ---------------------------------------------------------------- generacja */
void tex_generate(void) {
  kxp_begin();
  kxp_layers_begin(L_COUNT);
  for (int L = 0; L < L_COUNT; L++) {
    switch (L) {
      case L_WHITE: t_white(); break;
      case L_CLOTH: t_cloth(); break;
      case L_SKIN: t_skin(); break;
      case L_ASPHALT: t_asphalt(); break;
      case L_SIDEWALK: t_sidewalk(); break;
      case L_COBBLE: t_cobble(); break;
      case L_BRICK: bricks(0x8A3A28, 0xA8503A, 0xB0A898, 16, 6, 401); break;
      case L_BRICK_DARK: bricks(0x4A2E22, 0x6A4230, 0x8A8478, 16, 6, 411); break;
      case L_STONE: t_stone(); break;
      case L_PLASTER: t_plaster(); break;
      case L_WALLPAPER_RED: damask(rgb(0x5A1218), rgb(0x7A2A26), 421); break;
      case L_WALLPAPER_GREEN: t_wall_green(); break;
      case L_WOODFLOOR: planks(0x5A3A20, 0x7E5430, 6, 0.5f, 431, 1); break;
      case L_PARQUET: t_parquet(); break;
      case L_CHECKER: t_checker(); break;
      case L_CARPET: t_carpet(); break;
      case L_CONCRETE: t_concrete(); break;
      case L_ROOF: t_roof(); break;
      case L_WINDOW: t_window(); break;
      case L_SHOPWIN: t_shopwin(); break;
      case L_DOOR: t_door(); break;
      case L_AWNING: t_awning(); break;
      case L_WOOD: t_wood(0x3A2214, 0x5E3A22, 0.45f, 441); break;
      case L_WOOD_LIGHT: t_wood(0x7A5634, 0xA47A4C, 0.4f, 451); break;
      case L_METAL: t_metal(); break;
      case L_BRASS: t_brass(); break;
      case L_CRATE: t_crate(); break;
      case L_BARREL: t_barrel(); break;
      case L_CORRUGATED: t_corrugated(); break;
      case L_WATER: t_water(); break;
      case L_POSTER1: poster("THE BLUE MOON", "JAZZ", "NIGHTLY \xe2\x80\xa2 DANCING", 0x1A2A4A, 0xE8DCC0, 0xE8B84A, 461); break;
      case L_POSTER2: poster("VOTE FOR", "KANE", "ALDERMAN \xe2\x80\xa2 1933", 0x5A1A1A, 0xF0E6D0, 0xF0E6D0, 471); break;
      case L_POSTER3: poster("A CENTURY OF", "1933", "PROGRESS \xe2\x80\xa2 CHICAGO", 0x1A3A3A, 0xF0E6D0, 0xF0B850, 481); break;
      case L_SIGNS: t_signs(); break;
      case L_NEON: t_neon(); break;
      case L_TABLECLOTH: t_tablecloth(); break;
      case L_LEATHER: t_leather(); break;
      case L_VELVET: t_velvet(); break;
      case L_BOOKS: t_books(); break;
      case L_STAINED: t_stained(); break;
      case L_GRASS: t_grass(); break;
      case L_TIN_CEILING: t_tin(); break;
      case L_WAINSCOT: t_wainscot(); break;
      case L_TILE_WHITE: t_tile_white(); break;
      case L_FACES: t_faces(); break;
      case L_FOLIAGE: t_foliage(); break;
      case L_FENCE: t_fence(); break;
      case L_GRATE: t_grate(); break;
      case L_SKYLINE: t_skyline(); break;
      case L_NEWSPAPER: t_newspaper(); break;
      case L_MAPWALL: t_mapwall(); break;
      case L_MARBLE: t_marble(); break;
      case L_BOTTLES: t_bottles(); break;
      case L_PAINTING: t_painting(); break;
      case L_FIRE: t_fire(); break;
      case L_GRAVEL: t_gravel(); break;
    }
    kxp_layer_store(L);
  }
  r_set_world_textures(kxp_layers_finish(getenv("KRONIX_DUMP_TEX")));
  kxp_end();
}
