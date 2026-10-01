/* Proceduralne postacie (billboardy 24x48): widoki przód/tył/bok, pozy chodu,
 * celowania, strzału, trafienia i śmierci. Każda postać to zestaw parametrów. */
#include "engine.h"

#define AW 24
#define AH 48

enum { HAT_NONE, HAT_FEDORA, HAT_CAP, HAT_POLICE, HAT_TOQUE, HAT_CLOCHE, HAT_TOP, HAT_SCARF };
enum { TOP_SUIT, TOP_VEST, TOP_SHIRT, TOP_COAT, TOP_DRESS, TOP_APRON, TOP_UNIFORM, TOP_CASSOCK, TOP_BARE };

typedef struct {
  const char *name;
  u32 skin, hair; int hat; u32 hatc, band;
  int top; u32 topc, top2, tie, shirt;
  u32 pants, shoes;
  int mustache, beard, build, longhair, child;
} CharParam;

#define C(r, g, b) RGB(r, g, b)
#define SK C(0xf0, 0xc8, 0xa0)
#define SK2 C(0xc8, 0x92, 0x68)
#define SK3 C(0xe0, 0xb8, 0x88)
#define BLK C(0x1c, 0x1a, 0x22)

static const CharParam CP[] = {
  /* name     skin hair         hat         hatc              band            top          topc              top2              tie               shirt             pants             shoes  must beard build long child */
  {"tomek",   SK, C(0x6a,0x44,0x2a), HAT_CAP,    C(0x56,0x5c,0x68), C(0x3a,0x3e,0x48), TOP_VEST, C(0x4a,0x42,0x3a), C(0x6a,0x46,0x2e), 0, C(0xe8,0xe4,0xda), C(0x3a,0x3c,0x48), BLK, 0,0,1,0,0},
  {"don",     SK3, C(0xc0,0xc0,0xc8), HAT_NONE, 0,0, TOP_SUIT, C(0x1e,0x1c,0x24), C(0x10,0x10,0x16), C(0x8a,0x1e,0x28), C(0xf0,0xf0,0xf0), C(0x1e,0x1c,0x24), BLK, 1,0,2,0,0},
  {"vito",    SK3, C(0x1a,0x16,0x14), HAT_FEDORA, C(0x3a,0x38,0x40), C(0x14,0x12,0x16), TOP_SUIT, C(0x6a,0x6a,0x72), C(0x4a,0x4a,0x52), C(0xd8,0xb0,0x30), C(0xf0,0xf0,0xf0), C(0x5a,0x5a,0x62), BLK, 0,0,1,0,0},
  {"lucia",   SK, C(0x16,0x12,0x14), HAT_NONE, 0,0, TOP_DRESS, C(0xa8,0x1e,0x30), C(0x7a,0x14,0x22), 0, 0, C(0xa8,0x1e,0x30), BLK, 0,0,0,1,0},
  {"sean",    SK, C(0xc8,0x5a,0x2a), HAT_FEDORA, C(0x1e,0x40,0x2c), C(0x0e,0x20,0x16), TOP_SUIT, C(0x1e,0x44,0x2e), C(0x14,0x30,0x20), C(0xe8,0xb0,0x30), C(0xf0,0xf0,0xf0), C(0x1e,0x44,0x2e), BLK, 0,0,1,0,0},
  {"paddy",   SK, C(0xc8,0x5a,0x2a), HAT_CAP,    C(0x2a,0x4a,0x30), C(0x1a,0x30,0x20), TOP_VEST, C(0x7a,0x7a,0x82), C(0x2a,0x2a,0x30), 0, C(0xd8,0xd0,0xc0), C(0x4a,0x4a,0x52), BLK, 1,0,2,0,0},
  {"doyle",   SK, C(0x5a,0x3a,0x28), HAT_POLICE, C(0x1c,0x24,0x4a), C(0xe8,0xb0,0x30), TOP_UNIFORM, C(0x22,0x2c,0x5a), C(0x16,0x1e,0x40), 0, C(0xe8,0xb0,0x30), C(0x16,0x1e,0x40), BLK, 1,0,2,0,0},
  {"walsh",   SK, C(0x5a,0x3a,0x28), HAT_FEDORA, C(0x6a,0x52,0x3a), C(0x2a,0x1e,0x14), TOP_COAT, C(0xb0,0x94,0x6a), C(0x8a,0x70,0x4e), C(0x5a,0x2a,0x2a), C(0xe0,0xe0,0xe0), C(0x4a,0x40,0x38), BLK, 0,0,1,0,0},
  {"gliniarz",SK, C(0x5a,0x3a,0x28), HAT_POLICE, C(0x1c,0x24,0x4a), C(0xb8,0xb8,0xc0), TOP_UNIFORM, C(0x26,0x32,0x66), C(0x1a,0x24,0x4c), 0, C(0xb8,0xb8,0xc0), C(0x1a,0x24,0x4c), BLK, 0,0,1,0,0},
  {"zbir",    SK, C(0x1a,0x16,0x14), HAT_FEDORA, C(0x1a,0x18,0x20), C(0x0a,0x0a,0x0e), TOP_SUIT, C(0x2a,0x28,0x32), C(0x1c,0x1a,0x22), C(0x1c,0x1a,0x22), C(0xd8,0xd8,0xd8), C(0x2a,0x28,0x32), BLK, 0,0,1,0,0},
  {"zbir2",   SK, C(0xb8,0x6a,0x3a), HAT_CAP,    C(0x2a,0x4a,0x30), C(0x1a,0x30,0x20), TOP_VEST, C(0x8a,0x8a,0x92), C(0x3a,0x2e,0x24), 0, C(0xd8,0xd0,0xc0), C(0x4a,0x44,0x3e), BLK, 0,0,1,0,0},
  {"dokowiec",SK2, C(0x4a,0x30,0x20), HAT_CAP,   C(0x2a,0x3a,0x6a), C(0x1a,0x24,0x4a), TOP_SHIRT, C(0x9a,0x5a,0x3a), C(0x7a,0x44,0x2a), 0, 0, C(0x2a,0x32,0x5a), BLK, 0,1,2,0,0},
  {"bokser",  SK, C(0xc8,0x5a,0x2a), HAT_NONE, 0,0, TOP_BARE, SK, C(0xd8,0xa8,0x80), 0, 0, C(0x1e,0x6a,0x3a), BLK, 0,0,2,0,0},
  {"piekarz", SK, C(0x6a,0x44,0x2a), HAT_TOQUE,  C(0xf0,0xf0,0xf0), C(0xc8,0xc8,0xc8), TOP_APRON, C(0xf0,0xf0,0xf0), C(0xd8,0xd8,0xd8), 0, C(0xf0,0xf0,0xf0), C(0x4a,0x44,0x3e), BLK, 1,0,2,0,0},
  {"feliks",  SK3, C(0xb0,0xb0,0xb8), HAT_NONE, 0,0, TOP_VEST, C(0x5a,0x3a,0x28), C(0x3a,0x26,0x18), C(0x2a,0x36,0x6a), C(0xe8,0xe4,0xda), C(0x3a,0x3a,0x44), BLK, 1,0,0,0,0},
  {"stefek",  SK, C(0x6a,0x44,0x2a), HAT_CAP,    C(0x6a,0x6a,0x72), C(0x4a,0x4a,0x52), TOP_SHIRT, C(0x3a,0x5a,0xa0), C(0x2a,0x40,0x7a), 0, 0, C(0x5a,0x3a,0x28), BLK, 0,0,0,0,1},
  {"mickey",  SK, C(0xb0,0xb0,0xb8), HAT_CAP,    C(0x8a,0x1e,0x28), C(0x5a,0x14,0x1a), TOP_SHIRT, C(0x8a,0x8a,0x92), C(0x6a,0x6a,0x72), 0, 0, C(0x3a,0x3a,0x44), BLK, 1,0,1,0,0},
  {"kelner",  SK3, C(0x1a,0x16,0x14), HAT_NONE, 0,0, TOP_VEST, C(0xf0,0xf0,0xf0), C(0x1c,0x1a,0x22), C(0x1c,0x1a,0x22), C(0xf0,0xf0,0xf0), C(0x1c,0x1a,0x22), BLK, 0,0,0,0,0},
  {"helena",  SK, C(0xd8,0xd8,0xe0), HAT_SCARF,  C(0x6a,0x2a,0x6a), C(0x4a,0x1a,0x4a), TOP_DRESS, C(0x4a,0x3a,0x5a), C(0x36,0x28,0x44), 0, 0, C(0x4a,0x3a,0x5a), BLK, 0,0,0,0,0},
  {"pan",     SK, C(0x5a,0x3a,0x28), HAT_FEDORA, C(0x6a,0x4a,0x30), C(0x3a,0x28,0x18), TOP_SUIT, C(0x7a,0x5a,0x3a), C(0x5a,0x42,0x2a), C(0x8a,0x1e,0x28), C(0xe8,0xe4,0xda), C(0x5a,0x42,0x2a), BLK, 0,0,1,0,0},
  {"pani",    SK, C(0x5a,0x3a,0x28), HAT_CLOCHE, C(0x3a,0x5a,0x9a), C(0xe8,0xe0,0xd8), TOP_DRESS, C(0x4a,0x7a,0xc0), C(0x36,0x5a,0x96), 0, 0, C(0x4a,0x7a,0xc0), BLK, 0,0,0,1,0},
  {"robotnik",SK2, C(0x1a,0x16,0x14), HAT_CAP,   C(0x5a,0x44,0x30), C(0x3a,0x2a,0x1e), TOP_SHIRT, C(0x3a,0x4a,0x6a), C(0x2a,0x36,0x4e), 0, 0, C(0x5a,0x44,0x30), BLK, 0,0,1,0,0},
  {"menel",   SK2, C(0x7a,0x7a,0x82), HAT_NONE, 0,0, TOP_COAT, C(0x5a,0x4a,0x3a), C(0x42,0x36,0x2a), 0, C(0x8a,0x8a,0x8a), C(0x3a,0x34,0x2e), BLK, 0,1,0,0,0},
  {"mafioso", SK3, C(0x1a,0x16,0x14), HAT_FEDORA, C(0x4a,0x4a,0x52), C(0x1a,0x1a,0x1e), TOP_SUIT, C(0x16,0x14,0x1a), C(0x0e,0x0c,0x12), C(0xf0,0xf0,0xf0), C(0x2a,0x2a,0x30), C(0x16,0x14,0x1a), BLK, 0,0,1,0,0},
  {"lucky",   SK3, C(0x1a,0x16,0x14), HAT_FEDORA, C(0x4a,0x2a,0x5a), C(0x1a,0x10,0x20), TOP_SUIT, C(0x5a,0x2e,0x6a), C(0x40,0x20,0x4c), C(0xe8,0xb0,0x30), C(0xf0,0xf0,0xf0), C(0x40,0x20,0x4c), BLK, 1,0,1,0,0},
  {"bronek",  SK, C(0xb0,0xb0,0xb8), HAT_CAP,    C(0x6a,0x4a,0x30), C(0x4a,0x32,0x20), TOP_VEST, C(0x8a,0x6a,0x4a), C(0x5a,0x42,0x2a), 0, C(0xe8,0xe4,0xda), C(0x4a,0x40,0x38), BLK, 1,0,2,0,0},
  {"zoska",   SK, C(0xe8,0xc0,0x60), HAT_NONE, 0,0, TOP_APRON, C(0xa8,0x2a,0x36), C(0xf0,0xf0,0xf0), 0, 0, C(0xa8,0x2a,0x36), BLK, 0,0,1,1,0},
  {"ksiadz",  SK, C(0x5a,0x5a,0x62), HAT_NONE, 0,0, TOP_CASSOCK, C(0x16,0x14,0x1a), C(0x0e,0x0c,0x12), 0, C(0xf0,0xf0,0xf0), C(0x16,0x14,0x1a), BLK, 0,0,1,0,0},
  {"mama",    SK, C(0xb0,0xb0,0xb8), HAT_SCARF,  C(0x2a,0x46,0x8a), C(0x6a,0x8a,0xc8), TOP_DRESS, C(0x2a,0x2c,0x44), C(0x1c,0x1e,0x30), 0, 0, C(0x2a,0x2c,0x44), BLK, 0,0,1,0,0},
  {"janek",   SK, C(0xe8,0xc0,0x60), HAT_CAP,    C(0x8a,0x6a,0x4a), C(0x5a,0x42,0x2a), TOP_SHIRT, C(0x6a,0xa8,0xe8), C(0x4a,0x80,0xc0), 0, 0, C(0x5a,0x44,0x30), BLK, 0,0,0,0,0},
  {"lee",     C(0xe8,0xc8,0x98), C(0x10,0x10,0x14), HAT_NONE, 0,0, TOP_COAT, C(0x1a,0x2a,0x3a), C(0xc8,0xa0,0x30), 0, C(0x10,0x10,0x14), C(0x1a,0x1a,0x22), BLK, 1,0,1,0,0},
  {"triada",  C(0xe8,0xc8,0x98), C(0x10,0x10,0x14), HAT_CAP, C(0x1a,0x1a,0x22), C(0x0a,0x0a,0x0e), TOP_SHIRT, C(0x1a,0x1a,0x22), C(0x10,0x10,0x16), 0, 0, C(0x1a,0x1a,0x22), BLK, 0,0,1,0,0},
  {"kane",    SK, C(0xd8,0xd8,0xe0), HAT_TOP,    C(0x10,0x10,0x14), C(0x5a,0x14,0x1a), TOP_SUIT, C(0xe8,0xe4,0xd8), C(0xc0,0xbc,0xb0), C(0x10,0x10,0x14), C(0xf0,0xf0,0xf0), C(0xe8,0xe4,0xd8), C(0x3a,0x1e,0x14), 1,0,2,0,0},
  {"ochroniarz", SK, C(0x3a,0x2a,0x20), HAT_FEDORA, C(0x5a,0x5a,0x62), C(0x2a,0x2a,0x30), TOP_COAT, C(0x3a,0x3a,0x44), C(0x2a,0x2a,0x32), C(0x1c,0x1a,0x22), C(0xd8,0xd8,0xd8), C(0x2a,0x2a,0x32), BLK, 0,0,2,0,0},
  {"russo",   SK3, C(0x2a,0x1a,0x10), HAT_FEDORA, C(0x5a,0x3a,0x22), C(0x2a,0x1a,0x10), TOP_SUIT, C(0x6a,0x46,0x2a), C(0x4a,0x30,0x1c), C(0xb0,0x30,0x30), C(0xe0,0xd8,0xc8), C(0x4a,0x30,0x1c), BLK, 0,0,1,0,0},
  {"agent",   SK, C(0x3a,0x2a,0x20), HAT_FEDORA, C(0x4a,0x4e,0x56), C(0x1a,0x1a,0x1e), TOP_SUIT, C(0x5a,0x60,0x6a), C(0x42,0x48,0x52), C(0x1c,0x24,0x4a), C(0xf0,0xf0,0xf0), C(0x42,0x48,0x52), BLK, 0,0,1,0,0},
  {"gino",    SK3, C(0x1a,0x16,0x14), HAT_CAP,   C(0x3a,0x3a,0x44), C(0x1a,0x1a,0x22), TOP_VEST, C(0xe0,0xd8,0xc8), C(0x3a,0x3a,0x44), 0, C(0xe0,0xd8,0xc8), C(0x3a,0x3a,0x44), BLK, 1,0,1,0,0},
  {"enzo",    SK3, C(0x8a,0x8a,0x92), HAT_NONE, 0,0, TOP_SUIT, C(0x2a,0x2a,0x3a), C(0x1a,0x1a,0x28), C(0x3a,0x5a,0x3a), C(0xf0,0xf0,0xf0), C(0x2a,0x2a,0x3a), BLK, 0,0,0,0,0},
  {"sal",     SK3, C(0x1a,0x16,0x14), HAT_FEDORA, C(0x2a,0x2a,0x30), C(0x8a,0x1e,0x28), TOP_SUIT, C(0x3a,0x2a,0x22), C(0x2a,0x1e,0x18), C(0x8a,0x1e,0x28), C(0xf0,0xf0,0xf0), C(0x3a,0x2a,0x22), BLK, 1,0,2,0,0},
};
#define NCP ((int)(sizeof(CP) / sizeof(CP[0])))

static Actor actors[NCP];

/* ---------------------------------------------------------------- malowanie */
static Sprite *S_;
static void P(int x, int y, u32 c) { if ((unsigned)x < AW && (unsigned)y < AH) S_->px[y * AW + x] = c; }
static void R_(int x, int y, int w, int h, u32 c) { for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) P(i, j, c); }
static u32 dk(u32 c) { return blend(c, RGB(0, 0, 0), 70); }
static u32 lt(u32 c) { return blend(c, RGB(255, 255, 255), 40); }

static void outline(void) {
  u32 o = RGB(0x14, 0x10, 0x1c);
  Sprite *cp = spr_new(AW, AH);
  memcpy(cp->px, S_->px, sizeof(u32) * AW * AH);
  for (int y = 0; y < AH; y++)
    for (int x = 0; x < AW; x++) {
      if (cp->px[y * AW + x] >> 24) continue;
      int n = 0;
      if (x > 0 && (cp->px[y * AW + x - 1] >> 24)) n++;
      if (x < AW - 1 && (cp->px[y * AW + x + 1] >> 24)) n++;
      if (y > 0 && (cp->px[(y - 1) * AW + x] >> 24)) n++;
      if (y < AH - 1 && (cp->px[(y + 1) * AW + x] >> 24)) n++;
      if (n) S_->px[y * AW + x] = o;
    }
  free(cp->px); free(cp);
}

static void draw_gun(int x, int y, int flash, int side) {
  u32 g = RGB(0x2a, 0x2a, 0x30);
  if (side) { R_(x, y, 5, 2, g); R_(x, y + 2, 2, 2, g); }
  else {
    R_(x - 1, y - 2, 5, 5, g);
    R_(x - 1, y - 2, 5, 1, RGB(0x5a, 0x5a, 0x66));
    R_(x, y - 1, 3, 3, RGB(0x05, 0x05, 0x08));
    P(x + 1, y, RGB(0x30, 0x10, 0x10));
  }
  if (flash) {
    int fx = side ? x + 6 : x + 1, fy = side ? y + 1 : y + 1;
    R_(fx - 1, fy - 1, 3, 3, pal('y'));
    P(fx, fy, pal('w'));
    P(fx - 2, fy, pal('o')); P(fx + 2, fy, pal('o')); P(fx, fy - 2, pal('o')); P(fx, fy + 2, pal('o'));
  }
}

static void paint(const CharParam *c, int view, int pose) {
  int big = c->build == 2, thin = c->build == 0;
  int sh = big ? 7 : thin ? 5 : 6;  /* połowa szerokości barków */
  int cx = 12;
  int walk = pose == POSE_WALK1 ? 1 : pose == POSE_WALK2 ? 2 : 0;
  int aim = pose == POSE_AIM || pose == POSE_SHOOT;
  int headY = 3, torsoY = 12, legY = 28, footY = 45;
  u32 skin = c->skin, top = c->topc, top2 = c->top2, pants = c->pants;
  int longGarment = c->top == TOP_COAT || c->top == TOP_DRESS || c->top == TOP_CASSOCK;
  int hemY = c->top == TOP_DRESS ? 40 : c->top == TOP_CASSOCK ? 45 : 38;

  if (view != VIEW_SIDE) {
    /* nogi */
    int lup = walk == 1 ? 2 : 0, rup = walk == 2 ? 2 : 0;
    R_(cx - 5, legY, 4, footY - legY - lup, pants);
    R_(cx + 1, legY, 4, footY - legY - rup, dk(pants));
    R_(cx - 6, footY - lup, 5, 2, c->shoes);
    R_(cx + 1, footY - rup, 5, 2, c->shoes);
    /* tułów */
    for (int y = torsoY; y < legY + 1; y++) {
      int w = sh - (y - torsoY) / 6;
      if (w < sh - 2) w = sh - 2;
      R_(cx - w, y, w * 2, 1, top);
      R_(cx + w - 2, y, 2, 1, top2);
    }
    if (longGarment)
      for (int y = legY; y < hemY; y++) {
        int w = sh - 1 + (y - legY) / 4;
        R_(cx - w, y, w * 2, 1, top);
        R_(cx + w - 2, y, 2, 1, top2);
      }
    if (view == VIEW_FRONT) {
      if (c->top == TOP_SUIT || c->top == TOP_UNIFORM || c->top == TOP_COAT) {
        for (int y = torsoY; y < torsoY + 7; y++) { int w = 3 - (y - torsoY) / 2; if (w > 0) R_(cx - w, y, w * 2, 1, c->shirt ? c->shirt : pal('w')); }
        if (c->tie) R_(cx - 1, torsoY, 2, 9, c->tie);
        P(cx - 1, torsoY + 11, dk(top)); P(cx - 1, torsoY + 14, dk(top));
      } else if (c->top == TOP_VEST) {
        R_(cx - sh, torsoY, sh * 2, 7, c->shirt);
        for (int y = torsoY + 2; y < legY; y++) { R_(cx - sh + 1, y, sh - 2, 1, top); R_(cx + 1, y, sh - 2, 1, top2); }
        if (c->tie) R_(cx - 1, torsoY, 2, 8, c->tie);
      } else if (c->top == TOP_APRON) {
        R_(cx - sh + 2, torsoY + 4, sh * 2 - 4, legY - torsoY + 6, c->top2 == top ? pal('w') : c->top2);
      } else if (c->top == TOP_UNIFORM) {
        R_(cx - 1, torsoY + 2, 2, 12, c->shirt);
      } else if (c->top == TOP_CASSOCK) {
        R_(cx - 1, torsoY, 2, 2, c->shirt);
        for (int y = torsoY + 3; y < 40; y += 3) P(cx, y, dk(top));
      } else if (c->top == TOP_BARE) {
        R_(cx - 3, torsoY + 3, 2, 2, dk(skin)); R_(cx + 1, torsoY + 3, 2, 2, dk(skin));
        R_(cx - sh, legY - 2, sh * 2, 3, pants);
      }
      if (c->top == TOP_UNIFORM || c->top == TOP_VEST || c->top == TOP_SUIT) R_(cx - sh + 1, legY - 1, sh * 2 - 2, 1, dk(top2));
    } else {
      R_(cx - 2, torsoY, 4, 1, c->shirt ? c->shirt : top);
    }
    /* ręce */
    if (aim && view == VIEW_FRONT) {
      R_(cx - sh - 1, torsoY + 1, 3, 5, top); R_(cx + sh - 2, torsoY + 1, 3, 5, top2);
      R_(cx - sh + 1, torsoY + 5, sh - 1, 3, top); R_(cx + 1, torsoY + 5, sh - 1, 3, top2);
      R_(cx - 2, torsoY + 5, 4, 3, skin);
      draw_gun(cx - 1, torsoY + 4, pose == POSE_SHOOT, 0);
    } else {
      int swing = walk == 1 ? 1 : walk == 2 ? -1 : 0;
      int armTop = torsoY + 1, armLen = 13;
      R_(cx - sh - 2, armTop + swing, 2, armLen, top);
      R_(cx + sh, armTop - swing, 2, armLen, top2);
      R_(cx - sh - 2, armTop + armLen + swing, 2, 2, skin);
      R_(cx + sh, armTop + armLen - swing, 2, 2, skin);
    }
    /* głowa */
    int hw = 4;
    R_(cx - hw, headY + 1, hw * 2, 8, skin);
    R_(cx - hw + 1, headY, hw * 2 - 2, 1, skin);
    R_(cx - hw + 1, headY + 9, hw * 2 - 2, 1, skin);
    R_(cx - 1, headY + 10, 2, 2, dk(skin));
    if (view == VIEW_FRONT) {
      P(cx - 2, headY + 5, BLK); P(cx + 1, headY + 5, BLK);
      P(cx - 2, headY + 4, lt(skin)); P(cx + 1, headY + 4, lt(skin));
      P(cx, headY + 6, dk(skin));
      if (c->mustache) R_(cx - 2, headY + 7, 4, 1, c->hair == c->skin ? BLK : dk(c->hair));
      else P(cx, headY + 8, dk(skin)), P(cx - 1, headY + 8, dk(skin));
      if (c->beard) R_(cx - 3, headY + 7, 6, 3, c->hair);
      R_(cx - hw, headY, hw * 2, 2, c->hair);
      P(cx - hw, headY + 2, c->hair); P(cx + hw - 1, headY + 2, c->hair);
      if (c->longhair) { R_(cx - hw - 1, headY + 1, 2, 11, c->hair); R_(cx + hw - 1, headY + 1, 2, 11, c->hair); }
    } else {
      R_(cx - hw, headY, hw * 2, 9, c->hair);
      R_(cx - hw + 1, headY + 9, hw * 2 - 2, 1, dk(skin));
      if (c->longhair) R_(cx - hw, headY + 9, hw * 2, 5, c->hair);
    }
  } else {
    /* ----- widok z boku (w prawo) ----- */
    int bx = 9;
    /* nogi */
    if (walk) {
      int f = walk == 1 ? 1 : -1;
      R_(bx + 1 + 2 * f, legY, 3, footY - legY, pants);
      R_(bx + 1 - 2 * f, legY, 3, footY - legY, dk(pants));
      R_(bx + 1 + 2 * f, footY, 5, 2, c->shoes);
      R_(bx + 1 - 2 * f, footY, 5, 2, c->shoes);
    } else {
      R_(bx + 1, legY, 4, footY - legY, pants);
      R_(bx + 1, footY, 6, 2, c->shoes);
    }
    /* tułów */
    int tw = big ? 7 : 6;
    R_(bx, torsoY, tw, legY - torsoY + 1, top);
    R_(bx, torsoY, 2, legY - torsoY + 1, top2);
    if (longGarment) R_(bx - 1, legY, tw + 2, hemY - legY, top);
    if (c->top == TOP_APRON) R_(bx + tw - 1, torsoY + 4, 2, legY - torsoY + 4, c->top2 == top ? pal('w') : c->top2);
    if (c->tie && c->top != TOP_BARE) R_(bx + tw - 1, torsoY, 1, 8, c->tie);
    /* ręka */
    if (aim) {
      R_(bx + 2, torsoY + 2, 3, 4, top);
      R_(bx + 4, torsoY + 4, 9, 2, top);
      R_(bx + 12, torsoY + 4, 2, 2, skin);
      draw_gun(bx + 13, torsoY + 3, pose == POSE_SHOOT, 1);
    } else {
      int sw = walk == 1 ? 2 : walk == 2 ? -2 : 0;
      R_(bx + 2 + sw / 2, torsoY + 1, 3, 12, top2);
      R_(bx + 2 + sw, torsoY + 13, 3, 2, skin);
    }
    /* głowa */
    R_(bx, headY + 1, 8, 8, skin);
    R_(bx + 1, headY, 6, 1, skin);
    R_(bx + 1, headY + 9, 6, 1, skin);
    P(bx + 8, headY + 5, skin); P(bx + 8, headY + 6, dk(skin));
    P(bx + 6, headY + 4, BLK);
    if (c->mustache) R_(bx + 5, headY + 7, 3, 1, dk(c->hair));
    if (c->beard) R_(bx + 3, headY + 7, 5, 3, c->hair);
    R_(bx, headY, 5, 4, c->hair);
    R_(bx, headY + 4, 3, 4, c->hair);
    if (c->longhair) R_(bx - 1, headY + 2, 3, 12, c->hair);
    R_(bx + 3, headY + 10, 3, 2, dk(skin));
  }

  /* kapelusze */
  int side = view == VIEW_SIDE;
  int hx = side ? 9 : cx - 4, hwid = 8;
  switch (c->hat) {
    case HAT_FEDORA:
      R_(hx + 1, 0, hwid - 2, 3, c->hatc);
      R_(hx, 2, hwid, 1, c->band);
      R_(side ? hx - 2 : hx - 2, 3, hwid + 4, 1, c->hatc);
      P(hx + 3, 0, dk(c->hatc));
      break;
    case HAT_CAP:
      R_(hx, 1, hwid, 3, c->hatc);
      if (side) R_(hx + 5, 3, 5, 1, c->band);
      else if (view == VIEW_FRONT) R_(hx - 1, 3, hwid + 2, 1, c->band);
      break;
    case HAT_POLICE:
      R_(hx - 1, 0, hwid + 2, 3, c->hatc);
      R_(hx, 3, hwid, 1, BLK);
      if (view == VIEW_FRONT) P(cx, 1, c->band);
      if (side) R_(hx + 5, 3, 4, 1, BLK);
      break;
    case HAT_TOQUE: R_(hx, 0, hwid, 3, c->hatc); R_(hx, 3, hwid, 1, c->band); break;
    case HAT_CLOCHE: R_(hx - 1, 1, hwid + 2, 3, c->hatc); R_(hx - 1, 3, hwid + 2, 1, c->band); break;
    case HAT_TOP: R_(hx + 1, 0, hwid - 2, 3, c->hatc); R_(hx + 1, 2, hwid - 2, 1, c->band); R_(hx - 1, 3, hwid + 2, 1, c->hatc); break;
    case HAT_SCARF:
      R_(hx - 1, 2, hwid + 2, 3, c->hatc);
      if (view == VIEW_FRONT) { R_(hx - 1, 5, 2, 6, c->hatc); R_(hx + hwid - 1, 5, 2, 6, c->hatc); P(hx + 2, 3, c->band); P(hx + 5, 4, c->band); }
      else R_(hx - 1, 5, hwid + 2, 5, c->hatc);
      break;
  }
  if (pose == POSE_HIT) for (int i = 0; i < 6; i++) P(cx - 2 + (i * 5) % 6, torsoY + 3 + i, i % 2 ? pal('r') : pal('j'));
}

static Sprite *make(const CharParam *c, int view, int pose) {
  S_ = spr_new(AW, AH);
  paint(c, view, pose);
  outline();
  if (c->child) { /* dziecko: przeskaluj do 75% i postaw na ziemi */
    Sprite *s = spr_new(AW, AH);
    for (int y = 0; y < AH; y++)
      for (int x = 0; x < AW; x++) {
        int sx = (x - 3) * 4 / 3, sy = (y - 12) * 4 / 3;
        if (sx >= 0 && sy >= 0 && sx < AW && sy < AH) s->px[y * AW + x] = S_->px[sy * AW + sx];
      }
    free(S_->px); free(S_);
    return s;
  }
  return S_;
}

static Sprite *make_dead(const CharParam *c, int lying) {
  Sprite *st = make(c, VIEW_SIDE, POSE_STAND);
  Sprite *d = spr_new(AW, AH);
  if (!lying) { /* osuwanie się: ściśnięty w pionie */
    for (int y = 0; y < AH; y++)
      for (int x = 0; x < AW; x++) {
        int sy = (y - AH / 3) * 3 / 2;
        if (sy >= 0 && sy < AH) d->px[y * AW + x] = st->px[sy * AW + x];
      }
  } else { /* leży: obrót o 90 stopni, kałuża krwi */
    for (int y = AH - 12; y < AH; y++)
      for (int x = 0; x < AW; x++) {
        int dx = x - 12, dy = y - (AH - 3);
        if (dx * dx / 4 + dy * dy * 2 < 30) d->px[y * AW + x] = (x + y) % 3 ? RGB(0x6a, 0x10, 0x18) : RGB(0x8a, 0x18, 0x22);
      }
    for (int y = 0; y < AH; y++)
      for (int x = 0; x < AW; x++) {
        u32 c2 = st->px[y * AW + x];
        if (!(c2 >> 24)) continue;
        int nx = y / 2, ny = AH - 10 + (AW - 1 - x) / 3;
        if (nx < AW && ny < AH) d->px[ny * AW + nx] = c2;
      }
  }
  free(st->px); free(st);
  return d;
}

void actors_init(void) {
  for (int i = 0; i < NCP; i++) {
    Actor *a = &actors[i];
    snprintf(a->name, sizeof a->name, "%s", CP[i].name);
    for (int v = 0; v < VIEW_COUNT; v++)
      for (int p = 0; p < POSE_COUNT; p++) a->fr[v][p] = make(&CP[i], v, p);
    a->dead[0] = make_dead(&CP[i], 0);
    a->dead[1] = make_dead(&CP[i], 1);
    /* portret: głowa i barki z widoku z przodu */
    Sprite *pt = spr_new(20, 20);
    Sprite *f = a->fr[VIEW_FRONT][POSE_STAND];
    int oy = CP[i].child ? 10 : 0;
    for (int y = 0; y < 20; y++)
      for (int x = 0; x < 20; x++) pt->px[y * 20 + x] = f->px[(y + oy) * AW + x + 2];
    a->portrait = pt;
  }
}

Actor *actor_get(const char *name) {
  for (int i = 0; i < NCP; i++) if (!strcmp(actors[i].name, name)) return &actors[i];
  return NULL;
}
int actor_exists(const char *name) { return actor_get(name) != NULL; }
