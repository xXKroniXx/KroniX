/* Proceduralne tekstury 32x32 dla świata 3D i billboardy (drzewo, roślina, latarnia). */
#include "engine.h"

Tex *TEX[TX_COUNT];
Sprite *BB[BB_COUNT];
static Tex *waterFr[4], *fireFr[4];

static Tex *T;
static u32 hash3(int x, int y, int z) {
  u32 h = (u32)x * 374761393u + (u32)y * 668265263u + (u32)z * 2147483647u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}
static void tp(int x, int y, u32 c) { if ((unsigned)x < (unsigned)T->w && (unsigned)y < (unsigned)T->h) T->px[y * T->w + x] = c; }
static void tr(int x, int y, int w, int h, u32 c) { for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) tp(i, j, c); }
static void noise(u32 c, int n, int seed) { for (int i = 0; i < n; i++) { u32 h = hash3(i, seed, 7); tp(h % T->w, (h >> 8) % T->h, c); } }
static void vary(int amt, int seed) { /* delikatne zróżnicowanie jasności */
  for (int i = 0; i < T->w * T->h; i++) {
    u32 c = T->px[i];
    if (!(c >> 24)) continue;
    int d = (int)(hash3(i, seed, 3) % (2 * amt + 1)) - amt;
    int r = CLAMP((int)((c >> 16) & 255) + d, 0, 255), g = CLAMP((int)((c >> 8) & 255) + d, 0, 255), b = CLAMP((int)(c & 255) + d, 0, 255);
    T->px[i] = RGB(r, g, b);
  }
}
static Tex *mk(void) { T = tex_new(5, 5); return T; }

static void bricks(u32 brick, u32 mortar, int seed) {
  tr(0, 0, 32, 32, mortar);
  for (int y = 0; y < 32; y += 4)
    for (int x = ((y / 4) % 2) * 4 - 8; x < 32; x += 8) {
      u32 c = blend(brick, (hash3(x, y, seed) & 1) ? pal('i') : pal('o'), (int)(hash3(x, y, seed + 1) % 40));
      tr(x + 1, y, 7, 3, c);
      tr(x + 1, y, 7, 1, blend(c, pal('w'), 30));
    }
  vary(6, seed);
}

static void window(int x, int y, int w, int h, int lit, int seed) {
  tr(x - 1, y - 1, w + 2, h + 3, RGB(0x5a, 0x50, 0x48));
  u32 glass = lit ? RGB(0xf0, 0xc0, 0x60) : RGB(0x22, 0x2c, 0x44);
  tr(x, y, w, h, glass);
  if (lit) { tr(x, y, w, 2, RGB(0xff, 0xe0, 0x90)); tr(x + 1, y + h - 4, 3, 4, RGB(0x90, 0x50, 0x30)); }
  else { tp(x + 1, y + 1, RGB(0x5a, 0x6a, 0x8a)); tp(x + 2, y + 1, RGB(0x5a, 0x6a, 0x8a)); tp(x + 1, y + 2, RGB(0x5a, 0x6a, 0x8a)); }
  tr(x + w / 2, y, 1, h, RGB(0x3a, 0x30, 0x28));
  tr(x, y + h / 2, w, 1, RGB(0x3a, 0x30, 0x28));
  tr(x - 2, y + h + 1, w + 4, 2, RGB(0xa0, 0x98, 0x90));
  (void)seed;
}

static void planks(u32 base, int vertical, int seed) {
  tr(0, 0, 32, 32, base);
  for (int i = 0; i < 32; i += 8) {
    if (vertical) tr(i, 0, 1, 32, blend(base, pal('i'), 120));
    else tr(0, i, 32, 1, blend(base, pal('i'), 120));
    int off = (int)(hash3(i, seed, 1) % 32);
    if (vertical) tr(i + 1, off, 6, 1, blend(base, pal('i'), 60));
    else tr(off, i + 1, 1, 6, blend(base, pal('i'), 60));
  }
  for (int k = 0; k < 40; k++) {
    u32 h = hash3(k, seed, 5);
    int x = h % 32, y = (h >> 8) % 32;
    if (vertical) tr(x, y, 1, 3, blend(base, pal('i'), 40));
    else tr(x, y, 3, 1, blend(base, pal('i'), 40));
  }
  vary(5, seed);
}

static void make_water(int f) {
  mk();
  tr(0, 0, 32, 32, RGB(0x16, 0x2c, 0x42));
  for (int i = 0; i < 14; i++) {
    int y = (i * 5 + (int)(hash3(i, 1, 1) % 7)) % 32, x = (int)((hash3(i, 2, 2) % 32) + f * 3) % 32;
    tr(x, y, 5, 1, RGB(0x2e, 0x50, 0x70));
    tp((x + 2) % 32, (y + 31) % 32, RGB(0x4a, 0x78, 0x98));
  }
  waterFr[f] = T;
}

static void make_fire(int f) {
  mk();
  bricks(RGB(0x7a, 0x3a, 0x30), RGB(0x40, 0x22, 0x1e), 90);
  tr(4, 8, 24, 24, RGB(0x10, 0x0a, 0x0a));
  for (int i = 0; i < 10; i++) {
    int x = 6 + (int)((i * 7 + f * 3) % 20), h = 5 + (int)((i * 5 + f * 2) % 12);
    tr(x, 30 - h, 3, h, i % 2 ? pal('o') : pal('y'));
    tp(x + 1, 29 - h, pal('r'));
  }
  tr(4, 28, 24, 4, RGB(0x3a, 0x22, 0x18));
  tr(2, 4, 28, 4, pal('s'));
  fireFr[f] = T;
}

void textures_animate(int t) {
  TEX[TX_WATER] = waterFr[(t / 12) % 4];
  TEX[TX_FIRE] = fireFr[(t / 6) % 4];
}

static Sprite *bb_tree(void) {
  Sprite *s = spr_new(32, 48);
  for (int y = 0; y < 48; y++)
    for (int x = 0; x < 32; x++) {
      u32 c = 0;
      if (y > 30 && x >= 14 && x <= 17) c = x < 16 ? RGB(0x6a, 0x46, 0x2e) : RGB(0x4a, 0x30, 0x20);
      float dx = (x - 16) / 14.0f, dy = (y - 16) / 16.0f;
      float d = dx * dx + dy * dy;
      u32 h = hash3(x, y, 77);
      if (d < 1.0f - (h % 20) / 100.0f) c = (x + y + (int)(h % 5)) % 7 < 2 ? RGB(0x4a, 0x8a, 0x4a) : (dx + dy < -0.2f ? RGB(0x3a, 0x7a, 0x44) : RGB(0x22, 0x52, 0x32));
      s->px[y * 32 + x] = c;
    }
  return s;
}

static Sprite *bb_plant(void) {
  Sprite *s = spr_new(16, 24);
  for (int y = 16; y < 24; y++) for (int x = 4; x < 12; x++) s->px[y * 16 + x] = y == 16 ? RGB(0xc0, 0x74, 0x40) : RGB(0xa0, 0x5a, 0x30);
  for (int i = 0; i < 40; i++) {
    u32 h = hash3(i, 9, 9);
    int x = 3 + (int)(h % 10), y = 2 + (int)((h >> 8) % 14);
    s->px[y * 16 + x] = i % 3 ? RGB(0x38, 0xa0, 0x58) : RGB(0x1f, 0x60, 0x3a);
  }
  return s;
}

static Sprite *bb_lamp(void) {
  Sprite *s = spr_new(8, 48);
  for (int y = 6; y < 48; y++) for (int x = 3; x < 5; x++) s->px[y * 8 + x] = pal('i');
  for (int y = 44; y < 48; y++) for (int x = 2; x < 6; x++) s->px[y * 8 + x] = pal('i');
  for (int y = 0; y < 7; y++) for (int x = 1; x < 7; x++) s->px[y * 8 + x] = pal('i');
  for (int y = 1; y < 6; y++) for (int x = 2; x < 6; x++) s->px[y * 8 + x] = RGB(0xff, 0xe8, 0xa0);
  return s;
}

void textures_init(void) {
  for (int f = 0; f < 4; f++) { make_water(f); make_fire(f); }
  TEX[TX_WATER] = waterFr[0];
  TEX[TX_FIRE] = fireFr[0];

  mk(); tr(0, 0, 32, 32, RGB(0x3a, 0x3c, 0x46)); noise(RGB(0x30, 0x32, 0x3c), 90, 1); noise(RGB(0x4a, 0x4c, 0x56), 50, 2); TEX[TX_ASPHALT] = T;
  mk(); memcpy(T->px, TEX[TX_ASPHALT]->px, 32 * 32 * 4); tr(0, 14, 18, 3, RGB(0xc8, 0xb4, 0x60)); TEX[TX_ROADLINE] = T;
  mk(); memcpy(T->px, TEX[TX_ASPHALT]->px, 32 * 32 * 4); tr(14, 0, 3, 18, RGB(0xc8, 0xb4, 0x60)); TEX[TX_ROADLINE_V] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x9a, 0x9a, 0xa2));
  tr(0, 15, 32, 1, RGB(0x74, 0x74, 0x7e)); tr(0, 31, 32, 1, RGB(0x74, 0x74, 0x7e)); tr(15, 0, 1, 15, RGB(0x74, 0x74, 0x7e)); tr(31, 16, 1, 15, RGB(0x74, 0x74, 0x7e));
  noise(RGB(0x88, 0x88, 0x90), 40, 3); vary(4, 3); TEX[TX_SIDEWALK] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x3a, 0x36, 0x40));
  for (int y = 0; y < 32; y += 5) for (int x = (y / 5 % 2) * 3; x < 32; x += 6) { tr(x, y, 5, 4, RGB(0x66, 0x60, 0x6c)); tr(x, y, 5, 1, RGB(0x80, 0x7a, 0x86)); }
  vary(8, 4); TEX[TX_COBBLE] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x2e, 0x7a, 0x44)); noise(RGB(0x26, 0x66, 0x3a), 200, 5); noise(RGB(0x4a, 0x98, 0x5a), 80, 6); TEX[TX_GRASS] = T;
  mk(); planks(RGB(0x7a, 0x4e, 0x32), 0, 7); TEX[TX_WOOD] = T;
  mk(); for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) tp(x, y, ((x / 16) + (y / 16)) % 2 ? RGB(0x26, 0x20, 0x2c) : RGB(0xe0, 0xd4, 0xbc)); vary(4, 8); TEX[TX_CHECKER] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x86, 0x20, 0x2c));
  for (int y = 2; y < 32; y += 8) for (int x = (y / 8 % 2) * 8 + 2; x < 32; x += 16) { tp(x, y, pal('z')); tp(x + 1, y + 1, pal('z')); tp(x - 1, y + 1, pal('z')); tp(x, y + 2, pal('z')); }
  vary(5, 9); TEX[TX_CARPET] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x6a, 0x6a, 0x72)); noise(RGB(0x5c, 0x5c, 0x64), 120, 10); noise(RGB(0x7a, 0x7a, 0x82), 60, 11);
  tr(0, 31, 32, 1, RGB(0x5a, 0x5a, 0x62)); tr(31, 0, 1, 32, RGB(0x5a, 0x5a, 0x62)); TEX[TX_CONCRETE] = T;
  mk(); planks(RGB(0x86, 0x62, 0x44), 1, 12); TEX[TX_PLANKS] = T;
  mk(); tr(0, 0, 32, 32, RGB(0xd8, 0xcc, 0xb0)); noise(RGB(0xc4, 0xb8, 0x9c), 60, 13); TEX[TX_RING] = T;
  mk(); for (int y = 0; y < 32; y += 8) { tr(0, y, 32, 6, RGB(0x7a, 0x6e, 0x66)); tr(0, y, 32, 1, RGB(0x96, 0x8a, 0x80)); tr(0, y + 6, 32, 2, RGB(0x4a, 0x40, 0x3c)); } TEX[TX_STAIRS] = T;

  mk(); bricks(RGB(0x8a, 0x3e, 0x32), RGB(0x5a, 0x2c, 0x26), 20); TEX[TX_BRICK] = T;
  mk(); bricks(RGB(0x8a, 0x3e, 0x32), RGB(0x5a, 0x2c, 0x26), 21); window(9, 6, 14, 18, 0, 1); TEX[TX_BRICK_WIN] = T;
  mk(); bricks(RGB(0x8a, 0x3e, 0x32), RGB(0x5a, 0x2c, 0x26), 22); window(9, 6, 14, 18, 1, 2); TEX[TX_BRICK_WIN_LIT] = T;
  mk(); bricks(RGB(0x7a, 0x3a, 0x30), RGB(0x50, 0x28, 0x22), 23);
  tr(2, 4, 28, 24, RGB(0x2a, 0x20, 0x1c)); tr(3, 5, 26, 20, RGB(0x4c, 0x6c, 0x8c)); tr(4, 6, 6, 1, RGB(0xb0, 0xd0, 0xe8)); tr(4, 7, 1, 5, RGB(0xb0, 0xd0, 0xe8));
  tr(3, 19, 26, 6, RGB(0x6a, 0x46, 0x2e));
  for (int i = 0; i < 6; i++) { tr(5 + i * 4, 14, 2, 5, i % 2 ? pal('o') : pal('g')); tp(5 + i * 4, 13, pal('i')); }
  tr(0, 28, 32, 4, RGB(0xa0, 0x98, 0x90)); TEX[TX_SHOPWIN] = T;
  mk(); bricks(RGB(0x8a, 0x3e, 0x32), RGB(0x5a, 0x2c, 0x26), 24);
  tr(6, 2, 20, 30, RGB(0x2a, 0x1a, 0x12)); tr(8, 4, 16, 28, RGB(0x5a, 0x34, 0x22));
  tr(10, 6, 12, 8, RGB(0xe0, 0xb8, 0x60)); tr(15, 6, 2, 8, RGB(0x2a, 0x1a, 0x12));
  tr(10, 17, 12, 12, RGB(0x4a, 0x2a, 0x1c)); tr(11, 18, 10, 10, RGB(0x5e, 0x36, 0x24)); tr(20, 20, 2, 2, pal('z')); TEX[TX_DOOR] = T;
  mk(); for (int x = 0; x < 32; x++) tr(x, 0, 1, 26, (x / 4) % 2 ? RGB(0xe8, 0xe0, 0xd8) : RGB(0xa8, 0x2c, 0x38));
  for (int x = 0; x < 32; x += 4) tr(x, 26, 3, 2 + (x / 4) % 2 * 2, (x / 4) % 2 ? RGB(0xe8, 0xe0, 0xd8) : RGB(0xa8, 0x2c, 0x38));
  tr(0, 0, 32, 2, RGB(0x5a, 0x1a, 0x22)); TEX[TX_AWNING] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x6e, 0x6a, 0x66));
  for (int y = 0; y < 32; y += 8) for (int x = (y / 8 % 2) * 8 - 8; x < 32; x += 16) { tr(x + 1, y + 1, 14, 6, RGB(0x8a, 0x86, 0x80)); tr(x + 1, y + 1, 14, 1, RGB(0xa4, 0xa0, 0x98)); }
  vary(6, 25); TEX[TX_STONE] = T;
  mk(); memcpy(T->px, TEX[TX_STONE]->px, 32 * 32 * 4); window(10, 5, 12, 20, 0, 3); TEX[TX_STONE_WIN] = T;
  mk(); tr(0, 0, 32, 32, RGB(0xc8, 0xbc, 0xa4)); noise(RGB(0xb8, 0xac, 0x94), 80, 26); tr(0, 28, 32, 4, RGB(0x6a, 0x46, 0x2e)); TEX[TX_PLASTER] = T;
  mk(); for (int x = 0; x < 32; x++) tr(x, 0, 1, 24, (x % 8) < 4 ? RGB(0x2e, 0x5a, 0x44) : RGB(0x26, 0x4c, 0x3a));
  for (int y = 3; y < 24; y += 8) for (int x = 2; x < 32; x += 8) tp(x, y, pal('z'));
  tr(0, 24, 32, 8, RGB(0x5a, 0x3a, 0x28)); tr(0, 24, 32, 1, RGB(0x9b, 0x6a, 0x48)); tr(0, 31, 32, 1, pal('i')); TEX[TX_WALLPAPER] = T;
  mk(); for (int x = 0; x < 32; x++) tr(x, 0, 1, 24, (x % 8) < 4 ? RGB(0x6a, 0x1e, 0x26) : RGB(0x58, 0x18, 0x20));
  for (int y = 4; y < 24; y += 8) for (int x = 6; x < 32; x += 8) { tp(x, y, pal('z')); tp(x, y + 1, pal('y')); }
  tr(0, 24, 32, 8, RGB(0x3a, 0x22, 0x16)); tr(0, 24, 32, 1, RGB(0x8a, 0x5a, 0x3a)); TEX[TX_WALLPAPER_RED] = T;
  mk(); for (int x = 0; x < 32; x++) tr(x, 0, 1, 32, (x % 4) < 2 ? RGB(0x6a, 0x70, 0x76) : RGB(0x56, 0x5c, 0x62));
  noise(RGB(0x8a, 0x5a, 0x3a), 30, 27); tr(0, 0, 32, 2, RGB(0x3a, 0x3e, 0x44)); TEX[TX_WAREHOUSE] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x3c, 0x36, 0x3a)); noise(RGB(0x4c, 0x46, 0x4a), 100, 28); TEX[TX_ROOF] = T;
  mk(); tr(0, 0, 32, 32, RGB(0xb8, 0xae, 0x98)); noise(RGB(0xa8, 0x9e, 0x88), 60, 29); TEX[TX_CEIL_PLASTER] = T;
  mk(); planks(RGB(0x5a, 0x3a, 0x26), 0, 30); tr(0, 14, 32, 4, RGB(0x3a, 0x24, 0x16)); TEX[TX_CEIL_WOOD] = T;

  mk(); tr(0, 0, 32, 32, RGB(0xa8, 0x7a, 0x4c));
  tr(0, 0, 32, 3, RGB(0x7a, 0x52, 0x30)); tr(0, 29, 32, 3, RGB(0x7a, 0x52, 0x30)); tr(0, 0, 3, 32, RGB(0x7a, 0x52, 0x30)); tr(29, 0, 3, 32, RGB(0x7a, 0x52, 0x30));
  for (int i = 3; i < 29; i++) { tr(i, i, 2, 1, RGB(0x7a, 0x52, 0x30)); tr(30 - i, i, 2, 1, RGB(0x7a, 0x52, 0x30)); }
  tr(8, 12, 16, 7, RGB(0x96, 0x6a, 0x40)); /* napis */
  for (int i = 0; i < 5; i++) tr(10 + i * 3, 14, 2, 3, RGB(0x3a, 0x22, 0x14));
  vary(5, 31); TEX[TX_CRATE] = T;
  mk(); planks(RGB(0x86, 0x56, 0x34), 1, 32); tr(0, 5, 32, 3, RGB(0x4a, 0x4a, 0x52)); tr(0, 24, 32, 3, RGB(0x4a, 0x4a, 0x52)); TEX[TX_BARREL] = T;
  mk(); for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) tp(x, y, ((x / 4) + (y / 4)) % 2 ? RGB(0xe8, 0xe0, 0xd4) : RGB(0xb0, 0x30, 0x38)); TEX[TX_TABLECLOTH] = T;
  mk(); tr(0, 0, 32, 32, RGB(0xd8, 0xd0, 0xc4)); tr(0, 0, 32, 10, RGB(0xe8, 0xe0, 0xd4));
  for (int x = 0; x < 32; x += 4) tr(x, 10, 2, 22, RGB(0xc4, 0xbc, 0xb0)); TEX[TX_TABLE_SIDE] = T;
  mk(); planks(RGB(0x4a, 0x2a, 0x1a), 1, 33); for (int x = 2; x < 32; x += 10) { tr(x, 4, 7, 24, RGB(0x5a, 0x34, 0x22)); tr(x, 4, 7, 1, RGB(0x7a, 0x4e, 0x32)); }
  tr(0, 30, 32, 2, pal('z')); TEX[TX_BAR_FRONT] = T;
  mk(); planks(RGB(0x6a, 0x3e, 0x26), 0, 34); tp(8, 8, pal('o')); tp(8, 9, pal('o')); tp(20, 18, pal('w')); TEX[TX_BAR_TOP] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x3a, 0x22, 0x16));
  for (int y = 0; y < 32; y += 10) {
    for (int x = 1; x < 31; x += 3) {
      u32 h = hash3(x, y, 35);
      u32 col = h % 4 == 0 ? pal('o') : h % 4 == 1 ? pal('g') : h % 4 == 2 ? RGB(0xc8, 0x8a, 0x30) : pal('c');
      int bh = 5 + (int)(h % 3);
      tr(x, y + 9 - bh, 2, bh, col); tp(x, y + 9 - bh - 1, pal('i')); tp(x, y + 9 - bh + 1, blend(col, pal('w'), 90));
    }
    tr(0, y + 9, 32, 1, RGB(0x8a, 0x5a, 0x3a));
  }
  TEX[TX_SHELF_BOTTLES] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x16, 0x14, 0x1a)); tr(0, 16, 32, 8, pal('w')); for (int x = 1; x < 32; x += 3) tr(x, 16, 1, 8, pal('i'));
  for (int x = 2; x < 32; x += 6) tr(x, 16, 2, 5, pal('i')); tr(2, 3, 12, 4, RGB(0x3a, 0x38, 0x44)); TEX[TX_PIANO] = T;
  mk(); tr(0, 0, 32, 32, RGB(0xb0, 0x64, 0x34)); for (int x = 0; x < 32; x += 8) tr(x, 0, 2, 32, RGB(0x8a, 0x48, 0x24));
  tr(0, 6, 32, 2, RGB(0x6a, 0x3a, 0x20)); tr(0, 24, 32, 2, RGB(0x6a, 0x3a, 0x20)); noise(RGB(0xe0, 0x9a, 0x5a), 30, 36); TEX[TX_VAT] = T;
  mk(); planks(RGB(0x6e, 0x46, 0x2c), 0, 37); tr(6, 8, 12, 10, pal('w')); tr(8, 10, 8, 1, pal('d')); tr(8, 13, 6, 1, pal('d')); tr(22, 6, 4, 8, pal('g')); TEX[TX_DESK_TOP] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x3a, 0x22, 0x16));
  for (int y = 0; y < 32; y += 8) { for (int x = 1; x < 31; x += 2) { u32 h = hash3(x, y, 38); tr(x, y + 1 + (int)(h % 2), 2, 6 - (int)(h % 2), h % 3 == 0 ? pal('r') : h % 3 == 1 ? pal('n') : pal('x')); } tr(0, y + 7, 32, 1, RGB(0x8a, 0x5a, 0x3a)); }
  TEX[TX_BOOKS] = T;
  mk(); tr(0, 0, 32, 32, 0); for (int x = 1; x < 32; x += 5) { tr(x, 0, 2, 32, pal('d')); tr(x, 0, 1, 32, pal('s')); } tr(0, 2, 32, 2, pal('e')); tr(0, 28, 32, 2, pal('e')); TEX[TX_BARS] = T;
  mk(); tr(0, 0, 32, 32, 0); tr(0, 6, 32, 2, pal('i')); tr(0, 26, 32, 2, pal('i')); for (int x = 1; x < 32; x += 4) { tr(x, 2, 2, 30, pal('i')); tp(x, 1, pal('d')); } TEX[TX_FENCE] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x22, 0x22, 0x2c)); tr(0, 0, 32, 3, RGB(0x44, 0x44, 0x52));
  tr(4, 3, 10, 10, RGB(0x5a, 0x7a, 0x9a)); tr(18, 3, 10, 10, RGB(0x5a, 0x7a, 0x9a)); tr(15, 3, 2, 10, RGB(0x22, 0x22, 0x2c));
  tr(0, 16, 32, 1, RGB(0x8a, 0x8a, 0x96)); for (int i = 0; i < 2; i++) { int cx = 7 + i * 18; for (int y = 20; y < 32; y++) for (int x = cx - 5; x < cx + 5; x++) { int d = (x - cx) * (x - cx) + (y - 26) * (y - 26); if (d < 26) tp(x, y, d < 6 ? pal('s') : pal('i')); } }
  TEX[TX_CAR_SIDE] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x22, 0x22, 0x2c)); tr(4, 2, 24, 10, RGB(0x5a, 0x7a, 0x9a)); tr(8, 14, 16, 10, pal('s'));
  for (int y = 15; y < 24; y += 2) tr(9, y, 14, 1, pal('d'));
  tr(1, 16, 5, 5, pal('y')); tr(26, 16, 5, 5, pal('y')); tr(0, 26, 32, 3, pal('s')); TEX[TX_CAR_FRONT] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x22, 0x22, 0x2c)); tr(6, 2, 20, 8, RGB(0x5a, 0x7a, 0x9a)); tr(2, 16, 4, 3, pal('r')); tr(26, 16, 4, 3, pal('r')); tr(10, 22, 12, 4, pal('w')); tr(0, 27, 32, 3, pal('s')); TEX[TX_CAR_BACK] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x26, 0x26, 0x30)); tr(2, 2, 28, 28, RGB(0x2e, 0x2e, 0x3a)); TEX[TX_CAR_TOP] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x4a, 0x4e, 0x56)); noise(RGB(0x5e, 0x62, 0x6a), 80, 40); for (int y = 0; y < 32; y += 8) tr(0, y, 32, 1, RGB(0x36, 0x3a, 0x42)); TEX[TX_METAL] = T;
  /* plakaty */
  mk(); bricks(RGB(0x8a, 0x3e, 0x32), RGB(0x5a, 0x2c, 0x26), 41);
  tr(7, 3, 18, 25, RGB(0xe0, 0xd4, 0xb0)); tr(9, 5, 14, 3, pal('i'));
  for (int y = 10; y < 19; y++) for (int x = 11; x < 21; x++) { int d = (x - 16) * (x - 16) + (y - 14) * (y - 14); if (d < 16) tp(x, y, pal('h')); }
  tr(12, 12, 2, 1, pal('i')); tr(18, 12, 2, 1, pal('i')); tr(10, 21, 12, 2, pal('r')); tr(10, 24, 12, 1, pal('i')); TEX[TX_POSTER] = T;
  mk(); bricks(RGB(0x8a, 0x3e, 0x32), RGB(0x5a, 0x2c, 0x26), 42);
  tr(6, 4, 20, 24, RGB(0x2a, 0x3a, 0x6a)); tr(8, 6, 16, 3, pal('y')); tr(9, 11, 14, 10, RGB(0xd8, 0xb0, 0x60));
  tr(13, 13, 6, 8, pal('i')); tr(14, 11, 4, 2, pal('i')); tr(8, 23, 16, 2, pal('w')); TEX[TX_POSTER2] = T;
  mk(); planks(RGB(0x5a, 0x34, 0x22), 0, 43); TEX[TX_PEW] = T;
  mk(); tr(0, 0, 32, 32, pal('w')); tr(0, 0, 32, 8, pal('s')); tr(0, 12, 32, 20, RGB(0x6a, 0x7a, 0x6a)); tr(0, 12, 32, 2, RGB(0x4a, 0x5a, 0x4a)); TEX[TX_BED] = T;
  mk(); tr(0, 0, 32, 32, pal('w')); tr(0, 22, 32, 10, RGB(0xc8, 0xa8, 0x40)); tr(14, 4, 4, 14, pal('z')); tr(9, 8, 14, 4, pal('z')); TEX[TX_ALTAR] = T;
  mk(); tr(0, 0, 32, 32, RGB(0x6e, 0x6a, 0x66)); tr(6, 2, 20, 28, pal('i'));
  for (int y = 3; y < 29; y++) for (int x = 7; x < 25; x++) { u32 h = hash3(x / 3, y / 3, 44); tp(x, y, h % 4 == 0 ? pal('r') : h % 4 == 1 ? pal('y') : h % 4 == 2 ? pal('b') : pal('p')); }
  tr(15, 3, 2, 26, pal('i')); tr(7, 14, 18, 2, pal('i')); TEX[TX_STAINED] = T;
  mk(); tr(0, 0, 32, 32, pal('i')); tr(0, 0, 32, 32, RGB(0x1a, 0x1a, 0x22)); tr(12, 0, 3, 32, RGB(0x3a, 0x3a, 0x44)); TEX[TX_POLE] = T;
  mk(); for (int x = 0; x < 32; x++) tr(x, 0, 1, 32, (x % 6) < 3 ? RGB(0x8a, 0x1a, 0x26) : RGB(0x6a, 0x12, 0x1c)); TEX[TX_CURTAIN] = T;
  mk(); tr(0, 0, 32, 32, pal('i')); TEX[TX_BLACK] = T;

  BB[BB_TREE] = bb_tree();
  BB[BB_PLANT] = bb_plant();
  BB[BB_LAMP] = bb_lamp();
}
