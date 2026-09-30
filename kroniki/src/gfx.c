/* Programowy renderer: wszystko rysowane piksel po pikselu do bufora fb. */
#include "engine.h"

u32 fb[SCREEN_W * SCREEN_H];
int g_shake;

u32 pal(char c) {
  switch (c) {
    case 'k': return RGB(0x1a, 0x1c, 0x2c); /* czerń granatowa */
    case 'p': return RGB(0x5d, 0x27, 0x5d); /* fiolet */
    case 'r': return RGB(0xb1, 0x3e, 0x53); /* czerwień */
    case 'o': return RGB(0xef, 0x7d, 0x57); /* pomarańcz */
    case 'y': return RGB(0xff, 0xcd, 0x75); /* żółty */
    case 'l': return RGB(0xa7, 0xf0, 0x70); /* jasna zieleń */
    case 'g': return RGB(0x38, 0xb7, 0x64); /* zieleń */
    case 't': return RGB(0x25, 0x71, 0x79); /* morski */
    case 'n': return RGB(0x29, 0x36, 0x6f); /* granat */
    case 'b': return RGB(0x3b, 0x5d, 0xc9); /* niebieski */
    case 'c': return RGB(0x41, 0xa6, 0xf6); /* błękit */
    case 'a': return RGB(0x73, 0xef, 0xf7); /* akwamaryna */
    case 'w': return RGB(0xf4, 0xf4, 0xf4); /* biel */
    case 's': return RGB(0x94, 0xb0, 0xc2); /* srebro */
    case 'd': return RGB(0x56, 0x6c, 0x86); /* szary */
    case 'e': return RGB(0x33, 0x3c, 0x57); /* ciemnoszary */
    case 'f': return RGB(0xf0, 0xc8, 0xa0); /* skóra */
    case 'h': return RGB(0xc8, 0x8a, 0x64); /* cień skóry */
    case 'u': return RGB(0x8b, 0x5a, 0x3c); /* brąz */
    case 'v': return RGB(0x5a, 0x3a, 0x28); /* ciemny brąz */
    case 'x': return RGB(0x1f, 0x4d, 0x33); /* ciemna zieleń */
    case 'm': return RGB(0xc0, 0xca, 0xd6); /* mgła */
    case 'z': return RGB(0xe8, 0xb0, 0x30); /* złoto */
    case 'j': return RGB(0x7a, 0x2a, 0x3a); /* bordo */
    case 'q': return RGB(0x3a, 0x2a, 0x4a); /* śliwka */
    case 'i': return RGB(0x14, 0x10, 0x1c); /* prawie czarny */
    default: return 0;
  }
}

u32 blend(u32 a, u32 b, int t) {
  int ar = (a >> 16) & 255, ag = (a >> 8) & 255, ab = a & 255;
  int br = (b >> 16) & 255, bg = (b >> 8) & 255, bb = b & 255;
  return RGB(ar + ((br - ar) * t >> 8), ag + ((bg - ag) * t >> 8), ab + ((bb - ab) * t >> 8));
}

Sprite *spr_new(int w, int h) {
  Sprite *s = (Sprite *)calloc(1, sizeof(Sprite));
  s->w = w; s->h = h;
  s->px = (u32 *)calloc((size_t)w * h, sizeof(u32));
  return s;
}

Sprite *spr_art(const char *const *rows, int n, const u32 *remap) {
  int w = (int)strlen(rows[0]);
  Sprite *s = spr_new(w, n);
  for (int y = 0; y < n; y++) {
    const char *r = rows[y];
    for (int x = 0; x < w && r[x]; x++) {
      char c = r[x];
      u32 col = 0;
      if (c >= 'A' && c <= 'Z') col = remap ? remap[c - 'A'] : 0;
      else col = pal(c);
      s->px[y * w + x] = col;
    }
  }
  return s;
}

void gfx_clear(u32 c) {
  for (int i = 0; i < SCREEN_W * SCREEN_H; i++) fb[i] = c;
}

void gfx_pset(int x, int y, u32 c) {
  if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H) fb[y * SCREEN_W + x] = c;
}

void gfx_pset_a(int x, int y, u32 c, int a) {
  if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H) {
    u32 *p = &fb[y * SCREEN_W + x];
    *p = blend(*p, c, a);
  }
}

void gfx_rect(int x, int y, int w, int h, u32 c) {
  int x0 = CLAMP(x, 0, SCREEN_W), x1 = CLAMP(x + w, 0, SCREEN_W);
  int y0 = CLAMP(y, 0, SCREEN_H), y1 = CLAMP(y + h, 0, SCREEN_H);
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++) fb[yy * SCREEN_W + xx] = c;
}

void gfx_rect_a(int x, int y, int w, int h, u32 c, int a) {
  int x0 = CLAMP(x, 0, SCREEN_W), x1 = CLAMP(x + w, 0, SCREEN_W);
  int y0 = CLAMP(y, 0, SCREEN_H), y1 = CLAMP(y + h, 0, SCREEN_H);
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++) {
      u32 *p = &fb[yy * SCREEN_W + xx];
      *p = blend(*p, c, a);
    }
}

void gfx_vgrad(int x, int y, int w, int h, u32 top, u32 bot) {
  for (int i = 0; i < h; i++) gfx_rect(x, y + i, w, 1, blend(top, bot, h > 1 ? i * 255 / (h - 1) : 0));
}

void gfx_circle(int cx, int cy, int r, u32 c, int a) {
  for (int y = -r; y <= r; y++)
    for (int x = -r; x <= r; x++)
      if (x * x + y * y <= r * r) gfx_pset_a(cx + x, cy + y, c, a);
}

void gfx_blit_fx(const Sprite *s, int x, int y, int flip, int scale, u32 tint, int tintAmt, int alpha) {
  if (!s) return;
  if (scale < 1) scale = 1;
  for (int sy = 0; sy < s->h; sy++) {
    for (int sx = 0; sx < s->w; sx++) {
      u32 c = s->px[sy * s->w + (flip ? s->w - 1 - sx : sx)];
      if (!(c >> 24)) continue;
      if (tintAmt) c = blend(c, tint, tintAmt);
      for (int dy = 0; dy < scale; dy++)
        for (int dx = 0; dx < scale; dx++) {
          int px = x + sx * scale + dx, py = y + sy * scale + dy;
          if ((unsigned)px >= SCREEN_W || (unsigned)py >= SCREEN_H) continue;
          if (alpha >= 255) fb[py * SCREEN_W + px] = c;
          else fb[py * SCREEN_W + px] = blend(fb[py * SCREEN_W + px], c, alpha);
        }
    }
  }
}

void gfx_blit(const Sprite *s, int x, int y, int flip, int scale) {
  gfx_blit_fx(s, x, y, flip, scale, 0, 0, 255);
}

void gfx_darken(int amt) {
  if (amt <= 0) return;
  if (amt > 255) amt = 255;
  for (int i = 0; i < SCREEN_W * SCREEN_H; i++) fb[i] = blend(fb[i], RGB(0, 0, 0), amt);
}

/* Okno interfejsu: półprzezroczyste tło, podwójna ramka, zaokrąglone rogi. */
void gfx_window(int x, int y, int w, int h) {
  u32 fill = RGB(0x18, 0x14, 0x2a), outer = pal('i'), inner = pal('s'), inner2 = pal('d');
  gfx_rect_a(x + 1, y + 1, w - 2, h - 2, fill, 232);
  gfx_rect(x + 2, y, w - 4, 1, outer);
  gfx_rect(x + 2, y + h - 1, w - 4, 1, outer);
  gfx_rect(x, y + 2, 1, h - 4, outer);
  gfx_rect(x + w - 1, y + 2, 1, h - 4, outer);
  gfx_pset(x + 1, y + 1, outer); gfx_pset(x + w - 2, y + 1, outer);
  gfx_pset(x + 1, y + h - 2, outer); gfx_pset(x + w - 2, y + h - 2, outer);
  gfx_rect(x + 2, y + 1, w - 4, 1, inner);
  gfx_rect(x + 2, y + h - 2, w - 4, 1, inner2);
  gfx_rect(x + 1, y + 2, 1, h - 4, inner);
  gfx_rect(x + w - 2, y + 2, 1, h - 4, inner2);
}

void gfx_shake(int amount) {
  if (amount > g_shake) g_shake = amount;
}

void save_bmp(const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f) return;
  int rowSize = SCREEN_W * 3;
  int dataSize = rowSize * SCREEN_H;
  unsigned char h[54] = {'B', 'M'};
  int fileSize = 54 + dataSize;
  memcpy(h + 2, &fileSize, 4);
  h[10] = 54; h[14] = 40;
  int w = SCREEN_W, hh = SCREEN_H;
  memcpy(h + 18, &w, 4); memcpy(h + 22, &hh, 4);
  h[26] = 1; h[28] = 24;
  memcpy(h + 34, &dataSize, 4);
  fwrite(h, 1, 54, f);
  unsigned char *row = (unsigned char *)malloc(rowSize);
  for (int y = SCREEN_H - 1; y >= 0; y--) {
    for (int x = 0; x < SCREEN_W; x++) {
      u32 c = fb[y * SCREEN_W + x];
      row[x * 3] = c & 255; row[x * 3 + 1] = (c >> 8) & 255; row[x * 3 + 2] = (c >> 16) & 255;
    }
    fwrite(row, 1, rowSize, f);
  }
  free(row);
  fclose(f);
}
