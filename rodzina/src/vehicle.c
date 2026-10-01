/* Samochody: jazda gracza (zręcznościowa fizyka, kamera zza auta), zaparkowane auta
 * do „pożyczenia” i ruch uliczny na arteriach dużego miasta.
 * Jednostki: 1 kafel ≈ 1,9 m, prędkość w kaflach/s. Model auta ma przód w +x. */
#include "engine.h"
#include <math.h>

#define MAXCARS 48
#define NCOL 8
typedef struct {
  float x, z, ang, speed, steer, wheelT, lean, pitch, prevSpeed;
  int type, col, alive, ai;
  int route, wp;      /* trasa i następny punkt trasy (AI) */
  int stuckT, hornT;
  float crashT;
} Car;

static Car cars[MAXCARS];
static int ncars, drive = -1, enterT;
static GMesh carMesh[8][NCOL], wheelMesh[8];
static float camYaw, camOff, camDist = 3.6f, camT;
static int camInit;

static const uint32_t COLS[NCOL] = {0xFF1C1E24, 0xFF5A1C22, 0xFF2A4A34, 0xFF2C3A5A, 0xFF8A7A5A, 0xFF3A2E26, 0xFFB8AC90, 0xFF6A6E74};

/* ---------------------------------------------------------------- typy pojazdów */
enum { VT_SEDAN, VT_COUPE, VT_ROADSTER, VT_LIMO, VT_TAXI, VT_POLICE, VT_VAN, VT_PICKUP, VT_N };
typedef struct {
  const char *name;
  float L, W;          /* połowa długości i szerokości nadwozia */
  float wr, wfx, wrx;  /* promień koła, oś przednia i tylna (x) */
  float vmax, accel, grip; /* prędkość max (kafle/s), przyspieszenie, przyczepność skrętu */
  int fixedCol;        /* 0 = kolor z palety */
  uint32_t col;
} VSpec;
static const VSpec VS[VT_N] = {
  {"Bolt Model B", 1.05f, 0.42f, 0.17f, 0.62f, -0.62f, 13.0f, 7.5f, 1.9f, 0, 0},
  {"Falcon Coupé", 0.98f, 0.41f, 0.17f, 0.58f, -0.58f, 14.5f, 8.5f, 2.0f, 0, 0},
  {"Celeste Roadster", 0.95f, 0.40f, 0.18f, 0.57f, -0.57f, 16.5f, 9.5f, 2.2f, 0, 0},
  {"Lassiter V16", 1.36f, 0.45f, 0.18f, 0.88f, -0.82f, 14.0f, 6.5f, 1.6f, 1, 0xFF15161A},
  {"City Cab", 1.05f, 0.42f, 0.17f, 0.62f, -0.62f, 12.5f, 7.0f, 1.9f, 1, 0xFFE0B020},
  {"Police Interceptor", 1.05f, 0.42f, 0.17f, 0.62f, -0.62f, 15.5f, 9.0f, 2.0f, 1, 0xFF15161A},
  {"Hartman Van", 1.15f, 0.47f, 0.19f, 0.72f, -0.68f, 10.5f, 5.5f, 1.5f, 0, 0},
  {"Hartman Pickup", 1.12f, 0.44f, 0.18f, 0.7f, -0.66f, 12.0f, 6.5f, 1.7f, 0, 0},
};
#define WHEEL_COL 0xFFB0B0B0

static MB *VB;
static void vbox(float x0, float y0, float z0, float x1, float y1, float z1) { mb_box(VB, v3(x0, y0, z0), v3(x1, y1, z1)); }
static void vrbx(float x0, float y0, float z0, float x1, float y1, float z1, float e) { mb_rbox(VB, v3(x0, y0, z0), v3(x1, y1, z1), e); }
static void vwin(float x0, float x1, float y0, float y1, float z, int side) { /* szyba boczna */
  if (side > 0) mb_quad(VB, v3(x1, y0, z), v3(x0, y0, z), v3(x0, y1, z), v3(x1, y1, z), 0, 0, 2, 1);
  else mb_quad(VB, v3(x0, y0, z), v3(x1, y0, z), v3(x1, y1, z), v3(x0, y1, z), 0, 0, 2, 1);
}

/* nadwozie bez kół; przód w +x */
static void build_body(MB *b, int t, uint32_t paint) {
  VB = b;
  const VSpec *s = &VS[t];
  float L = s->L, W = s->W;
  uint32_t trim = 0xFFC8CCD2, dark = 0xFF1E1C1A, glass = 0xFF1A2430;
  int open = t == VT_ROADSTER;
  /* dolna część i maska */
  mb_paint(paint, L_PAINT);
  if (t == VT_VAN) {
    vrbx(-L, 0.16f, -W, L * 0.95f, 0.42f, W, 0.05f);
    vrbx(L * 0.3f, 0.4f, -W * 0.62f, L * 0.95f, 0.55f, W * 0.62f, 0.05f);   /* maska */
    vrbx(-L, 0.4f, -W, L * 0.32f, 1.02f, W, 0.06f);                          /* skrzynia */
    mb_paint(0xFFE8E0C8, L_PAINT);
    vbox(-L * 0.9f, 0.62f, W + 0.002f, L * 0.1f, 0.84f, W + 0.008f);
    vbox(-L * 0.9f, 0.62f, -W - 0.008f, L * 0.1f, 0.84f, -W - 0.002f);
    mb_paint(glass, L_WINDOW);
    mb_quad(VB, v3(L * 0.33f, 0.58f, -W * 0.85f), v3(L * 0.33f, 0.58f, W * 0.85f), v3(L * 0.33f, 0.9f, W * 0.85f), v3(L * 0.33f, 0.9f, -W * 0.85f), 0, 0, 1, 1);
    vwin(L * 0.05f, L * 0.3f, 0.6f, 0.88f, W + 0.003f, 1);
    vwin(L * 0.05f, L * 0.3f, 0.6f, 0.88f, -W - 0.003f, -1);
  } else {
    vrbx(-L, 0.16f, -W, L * 0.92f, 0.46f, W, 0.06f);
    vrbx(L * 0.22f, 0.42f, -W * 0.62f, L * 0.92f, 0.54f, W * 0.62f, 0.05f); /* długa maska */
    if (t == VT_PICKUP) {
      vrbx(-0.15f, 0.44f, -W * 0.95f, L * 0.26f, 0.84f, W * 0.95f, 0.06f); /* kabina */
      mb_paint(0xFF5A3A22, L_WOOD_LIGHT);
      vbox(-L, 0.44f, -W * 0.92f, -0.17f, 0.47f, W * 0.92f);              /* paka */
      mb_paint(paint, L_PAINT);
      for (int sd = -1; sd <= 1; sd += 2) vbox(-L, 0.44f, sd * W * 0.92f - 0.02f, -0.17f, 0.62f, sd * W * 0.92f + 0.02f);
      vbox(-L - 0.01f, 0.44f, -W * 0.92f, -L + 0.03f, 0.62f, W * 0.92f);
      mb_paint(0xFFFFFFFF, L_CRATE);
      vbox(-L * 0.85f, 0.47f, -0.3f, -L * 0.45f, 0.72f, 0.05f);
      vbox(-L * 0.5f, 0.47f, 0.05f, -0.25f, 0.68f, 0.32f);
      mb_paint(glass, L_WINDOW);
      mb_quad(VB, v3(L * 0.27f, 0.52f, -W * 0.82f), v3(L * 0.27f, 0.52f, W * 0.82f), v3(L * 0.27f, 0.78f, W * 0.82f), v3(L * 0.27f, 0.78f, -W * 0.82f), 0, 0, 1, 1);
      vwin(-0.1f, L * 0.22f, 0.52f, 0.78f, W * 0.96f, 1);
      vwin(-0.1f, L * 0.22f, 0.52f, 0.78f, -W * 0.96f, -1);
    } else if (open) {
      /* kabriolet: niska przednia szyba, siedzenia, złożony dach */
      mb_paint(0xFF3A2416, L_LEATHER);
      vbox(-L * 0.55f, 0.44f, -W * 0.85f, L * 0.05f, 0.62f, W * 0.85f);
      vbox(-L * 0.6f, 0.44f, -W * 0.85f, -L * 0.48f, 0.78f, W * 0.85f);
      mb_paint(0xFF2A2420, L_CLOTH);
      vrbx(-L * 0.95f, 0.44f, -W * 0.8f, -L * 0.62f, 0.62f, W * 0.8f, 0.05f);
      mb_paint(trim, L_CHROME);
      vbox(L * 0.18f, 0.46f, -W * 0.8f, L * 0.2f, 0.72f, W * 0.8f);
      mb_paint(0xFF6A8090, L_WINDOW);
      mb_quad(VB, v3(L * 0.2f, 0.5f, -W * 0.78f), v3(L * 0.2f, 0.5f, W * 0.78f), v3(L * 0.2f, 0.7f, W * 0.78f), v3(L * 0.2f, 0.7f, -W * 0.78f), 0, 0, 1, 1);
      mb_paint(paint, L_PAINT);
      vrbx(-L, 0.4f, -W, -L * 0.6f, 0.52f, W, 0.05f); /* tylny bagażnik */
    } else {
      /* kabina: limuzyna dłuższa, coupé krótsza i opadająca */
      float c0 = t == VT_COUPE ? -0.55f : t == VT_LIMO ? -1.05f : -0.75f, c1 = t == VT_LIMO ? 0.42f : 0.28f;
      float top = t == VT_COUPE ? 0.78f : t == VT_LIMO ? 0.86f : 0.82f;
      vrbx(c0, 0.44f, -W * 0.95f, c1, top, W * 0.95f, 0.07f);
      if (t == VT_COUPE) vrbx(-L * 0.98f, 0.42f, -W * 0.8f, c0 + 0.05f, 0.6f, W * 0.8f, 0.08f); /* opadający tył */
      if (t == VT_LIMO) { mb_paint(0xFF2A2C30, L_LEATHER); vrbx(c0 + 0.02f, top - 0.02f, -W * 0.9f, c0 + 0.6f, top + 0.02f, W * 0.9f, 0.02f); }
      mb_paint(glass, L_WINDOW);
      mb_quad(VB, v3(c1 + 0.01f, 0.5f, -W * 0.85f), v3(c1 + 0.01f, 0.5f, W * 0.85f), v3(c1 + 0.01f, top - 0.04f, W * 0.85f), v3(c1 + 0.01f, top - 0.04f, -W * 0.85f), 0, 0, 1, 1);
      if (t == VT_LIMO) {
        vwin(c0 + 0.08f, -0.42f, 0.52f, top - 0.04f, W * 0.96f, 1); vwin(-0.36f, c1 - 0.05f, 0.52f, top - 0.04f, W * 0.96f, 1);
        vwin(c0 + 0.08f, -0.42f, 0.52f, top - 0.04f, -W * 0.96f, -1); vwin(-0.36f, c1 - 0.05f, 0.52f, top - 0.04f, -W * 0.96f, -1);
      } else {
        vwin(c0 + 0.05f, c1 - 0.05f, 0.52f, top - 0.04f, W * 0.96f, 1);
        vwin(c0 + 0.05f, c1 - 0.05f, 0.52f, top - 0.04f, -W * 0.96f, -1);
      }
      mb_quad(VB, v3(c0 - 0.01f, 0.52f, W * 0.8f), v3(c0 - 0.01f, 0.52f, -W * 0.8f), v3(c0 - 0.01f, top - 0.06f, -W * 0.8f), v3(c0 - 0.01f, top - 0.06f, W * 0.8f), 0, 0, 1, 1);
      if (t == VT_TAXI) {
        /* pas szachownicy i lampa na dachu */
        for (int k = 0; k < 14; k++) {
          mb_paint(k & 1 ? 0xFF141414 : 0xFFC8C8C0, L_PAINT);
          float x0 = -L + k * (L * 1.9f / 14), x1 = x0 + L * 1.9f / 14;
          vbox(x0, 0.3f, W + 0.001f, x1, 0.36f, W + 0.008f);
          vbox(x0, 0.3f, -W - 0.008f, x1, 0.36f, -W - 0.001f);
        }
        mb_paint(0xFFFFE8A0, L_WHITE); P_.emis = 0.2f;
        vrbx(-0.3f, top, -0.16f, -0.02f, top + 0.09f, 0.16f, 0.02f);
      }
      if (t == VT_POLICE) {
        mb_paint(0xFFC8C8C8, L_PAINT); /* białe drzwi */
        vbox(c0 + 0.1f, 0.2f, W + 0.002f, c1 - 0.05f, 0.44f, W + 0.008f);
        vbox(c0 + 0.1f, 0.2f, -W - 0.008f, c1 - 0.05f, 0.44f, -W - 0.002f);
        mb_paint(0xFFE8B84A, L_BRASS); /* gwiazda */
        mb_cyl_z(VB, v3((c0 + c1) * 0.5f, 0.33f, W + 0.012f), 0.06f, 0.004f, 5, 3);
        mb_cyl_z(VB, v3((c0 + c1) * 0.5f, 0.33f, -W - 0.016f), 0.06f, 0.004f, 5, 3);
        mb_paint(0xFF2A2C30, L_METAL);
        vbox(-0.15f, top, -0.05f, -0.05f, top + 0.05f, 0.05f);
        mb_paint(0xFFFF3020, L_WHITE); P_.emis = 0.6f;
        mb_sphere(VB, v3(-0.1f, top + 0.09f, 0), 0.05f, 0.05f, 0.05f, 8); /* syrena */
      }
    }
  }
  /* błotniki nad kołami */
  mb_paint(t == VT_POLICE ? 0xFF15161A : paint, L_PAINT);
  for (int i = 0; i < 4; i++) {
    float x = (i & 1) ? s->wfx : s->wrx, z = (i & 2) ? W + 0.02f : -W - 0.02f;
    float hw = (t == VT_VAN || t == VT_PICKUP) ? 0.1f : 0.085f;
    mb_arch_z(VB, v3(x, s->wr, z), s->wr + 0.07f, 0.03f, hw, 0.12f, PI_F - 0.08f, 10);
  }
  mb_paint(dark, L_METAL);
  vbox(s->wrx + 0.17f, 0.12f, -W - 0.12f, s->wfx - 0.17f, 0.15f, W + 0.12f); /* stopnie */
  /* grill, zderzaki, reflektory, światła tylne */
  mb_paint(trim, L_CHROME);
  vrbx(L * 0.9f, 0.2f, -0.2f, L * 0.95f, 0.5f, 0.2f, 0.02f);
  vbox(L * 0.95f, 0.14f, -W - 0.05f, L + 0.04f, 0.19f, W + 0.05f);
  vbox(-L - 0.06f, 0.14f, -W - 0.05f, -L - 0.01f, 0.19f, W + 0.05f);
  for (int sd = -1; sd <= 1; sd += 2) {
    mb_paint(trim, L_CHROME);
    mb_cyl_x(VB, v3(L * 0.95f, 0.5f, sd * 0.3f), 0.07f, 0.06f, 10, 3);
    mb_paint(0xFFE8E0C0, L_WHITE); P_.emis = 0.08f;
    mb_cyl_x(VB, v3(L * 0.985f, 0.5f, sd * 0.3f), 0.055f, 0.01f, 10, 3);
    mb_paint(0xFF8A1A1A, L_WHITE); P_.emis = 0.1f;
    vbox(-L - 0.02f, 0.4f, sd * 0.32f - 0.03f, -L, 0.45f, sd * 0.32f + 0.03f);
  }
  /* zapasowe koło na tyle (sedan, limuzyna, taxi) */
  if (t == VT_SEDAN || t == VT_LIMO || t == VT_TAXI || t == VT_POLICE) {
    mb_paint(WHEEL_COL, L_TIRE);
    mb_cyl_x(VB, v3(-L - 0.1f, 0.42f, 0), 0.16f, 0.09f, 14, 3);
  }
}

/* koło: oś w z, środek w (0,0,0) */
static void build_wheel(MB *b, float r) {
  mb_paint(WHEEL_COL, L_TIRE);
  mb_cyl_z(b, v3(0, 0, 0), r, 0.11f, 14, 3);
  mb_paint(0xFFE8E4DA, L_WHITE);
  mb_cyl_z(b, v3(0, 0, 0.056f), r * 0.6f, 0.004f, 14, 3);
  mb_cyl_z(b, v3(0, 0, -0.056f), r * 0.6f, 0.004f, 14, 3);
  mb_paint(0xFFB8BCC2, L_CHROME);
  mb_cyl_z(b, v3(0, 0, 0.06f), r * 0.3f, 0.008f, 8, 3);
  mb_cyl_z(b, v3(0, 0, -0.06f), r * 0.3f, 0.008f, 8, 3);
  /* szprychy — widać obrót */
  mb_paint(0xFF8A8E94, L_CHROME);
  for (int k = 0; k < 3; k++) {
    float a = k * 1.0472f, c = cosf(a), s = sinf(a);
    for (int sd = -1; sd <= 1; sd += 2) {
      V3 p0 = v3(-c * r * 0.55f, -s * r * 0.55f, sd * 0.059f), p1 = v3(c * r * 0.55f, s * r * 0.55f, sd * 0.059f);
      V3 n = v3(-s * 0.02f, c * 0.02f, 0);
      if (sd > 0) mb_quad(b, v3add(p0, n), v3sub(p0, n), v3sub(p1, n), v3add(p1, n), 0, 0, 1, 1);
      else mb_quad(b, v3sub(p0, n), v3add(p0, n), v3add(p1, n), v3sub(p1, n), 0, 0, 1, 1);
    }
  }
}

/* ---------------------------------------------------------------- trasy ruchu (pasy prawostronne) */
typedef struct { int n; float p[8][2]; } Route;
static const Route ROUTES[] = {
  /* duży pierścień zgodnie ze wskazówkami zegara: Grand → Halsted → Cermak → Ashland */
  {4, {{7.5f, 16.5f}, {84.5f, 16.5f}, {84.5f, 88.5f}, {7.5f, 88.5f}}},
  /* przeciwnie: Ashland w dół → Cermak → Halsted w górę → Grand */
  {4, {{5.5f, 14.5f}, {5.5f, 90.5f}, {86.5f, 90.5f}, {86.5f, 14.5f}}},
  /* ulica główna (Polonia–Italia–Loop) → Michigan → Chinatown/Cermak → Ashland */
  {4, {{7.5f, 52.5f}, {131.5f, 52.5f}, {131.5f, 88.5f}, {7.5f, 88.5f}}},
  /* odwrotnie */
  {4, {{5.5f, 90.5f}, {133.5f, 90.5f}, {133.5f, 50.5f}, {5.5f, 50.5f}}},
};
#define NROUTES ((int)(sizeof(ROUTES) / sizeof(ROUTES[0])))

/* ---------------------------------------------------------------- pomocnicze */
static float fwdx(float a) { return cosf(a); }
static float fwdz(float a) { return -sinf(a); }
static float wrap_pi(float a) { while (a > PI_F) a -= 2 * PI_F; while (a < -PI_F) a += 2 * PI_F; return a; }

static int car_blocked_t(int type, float x, float z, float a) {
  float fx = fwdx(a), fz = fwdz(a), off = VS[type].L - 0.37f;
  for (int k = -1; k <= 1; k++) {
    float cx = x + fx * off * k, cz = z + fz * off * k;
    float r = VS[type].W - 0.02f;
    if (world_solid_at(cx - r, cz - r) || world_solid_at(cx + r, cz - r) || world_solid_at(cx - r, cz + r) || world_solid_at(cx + r, cz + r)) return 1;
  }
  return 0;
}

/* odległość między autami (3 koła na auto) */
static int cars_touch(const Car *a, float ax, float az, const Car *b, float *px, float *pz) {
  float best = 1e9f;
  for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++) {
      float oa = VS[a->type].L - 0.37f, ob = VS[b->type].L - 0.37f;
      float x0 = ax + fwdx(a->ang) * oa * i, z0 = az + fwdz(a->ang) * oa * i;
      float x1 = b->x + fwdx(b->ang) * ob * j, z1 = b->z + fwdz(b->ang) * ob * j;
      float dx = x0 - x1, dz = z0 - z1, d = sqrtf(dx * dx + dz * dz);
      if (d < best) { best = d; if (d > 0.001f) { *px = dx / d; *pz = dz / d; } else { *px = 1; *pz = 0; } }
    }
  return best < VS[a->type].W + VS[b->type].W;
}

int car_player(void) { return drive; }

int cars_block(float x, float z, float r) {
  for (int i = 0; i < ncars; i++) {
    Car *c = &cars[i];
    if (!c->alive || i == drive) continue;
    /* prostokąt auta w jego układzie */
    float dx = x - c->x, dz = z - c->z;
    float lx = dx * fwdx(c->ang) + dz * fwdz(c->ang);
    float lz = -dx * fwdz(c->ang) + dz * fwdx(c->ang);
    if (fabsf(lx) < VS[c->type].L + 0.03f + r && fabsf(lz) < VS[c->type].W + 0.02f + r) return 1;
  }
  return 0;
}

static int tile_at(int x, int z) {
  MapDef *m = &S.maps[W.map];
  return (x < 0 || z < 0 || x >= m->w || z >= m->h) ? '#' : m->tiles[z][x];
}

/* ---------------------------------------------------------------- tworzenie */
void cars_init_map(void) {
  audio_engine(0, 0);
  ncars = 0;
  drive = -1;
  camInit = 0;
  MapDef *m = &S.maps[W.map];
  if (m->interior) return;
  /* zaparkowane auta z kafli 'c' */
  for (int z = 0; z < m->h; z++)
    for (int x = 0; x < m->w && ncars < MAXCARS; x++) {
      if (m->tiles[z][x] != 'c') continue;
      int near = 0;
      for (int j = 0; j < ncars; j++) if (fabsf(cars[j].x - (x + 0.5f)) + fabsf(cars[j].z - (z + 0.5f)) < 2.5f) near = 1;
      if (near) continue;
      Car *c = &cars[ncars++];
      memset(c, 0, sizeof *c);
      c->x = x + 0.5f; c->z = z + 0.5f;
      int alongX = !tile_solid(tile_at(x - 1, z)) && !tile_solid(tile_at(x + 1, z)) && (tile_solid(tile_at(x, z - 1)) || tile_solid(tile_at(x, z + 1)) || tile_at(x, z - 1) == ',' || tile_at(x, z + 1) == ',');
      c->ang = alongX ? ((x * 7 + z) % 2 ? 0 : PI_F) : ((x + z * 3) % 2 ? PI_F / 2 : -PI_F / 2);
      /* zaparkowane: zwykłe auta; na Racine przy trattorii — limuzyna i kabriolet Rodziny */
      static const int PT[6] = {VT_SEDAN, VT_COUPE, VT_SEDAN, VT_PICKUP, VT_VAN, VT_ROADSTER};
      c->type = PT[(x * 7 + z * 13) % 6];
      c->col = (x * 13 + z * 7) % NCOL;
      if (S.nregions && x == 61 && z == 58) { c->type = VT_LIMO; c->col = 0; }
      if (S.nregions && x == 63 && z == 61) { c->type = VT_ROADSTER; c->col = 1; }
      c->alive = 1;
    }
  /* ruch uliczny tylko na dużej mapie miasta */
  if (m->w * m->h < 4000 || !S.nregions) return;
  for (int k = 0; k < 14 && ncars < MAXCARS; k++) {
    int r = k % NROUTES;
    const Route *rt = &ROUTES[r];
    int seg = (k / NROUTES) % rt->n;
    float t = 0.15f + 0.23f * ((k * 7) % 4);
    const float *a = rt->p[seg], *b = rt->p[(seg + 1) % rt->n];
    Car *c = &cars[ncars++];
    memset(c, 0, sizeof *c);
    c->x = a[0] + (b[0] - a[0]) * t; c->z = a[1] + (b[1] - a[1]) * t;
    c->ang = atan2f(-(b[1] - a[1]), b[0] - a[0]);
    c->ai = 1; c->route = r; c->wp = (seg + 1) % rt->n;
    c->col = (k * 5 + 3) % NCOL;
    static const int TT[14] = {VT_SEDAN, VT_TAXI, VT_VAN, VT_COUPE, VT_SEDAN, VT_PICKUP, VT_TAXI, VT_POLICE,
                               VT_SEDAN, VT_LIMO, VT_ROADSTER, VT_VAN, VT_TAXI, VT_COUPE};
    c->type = TT[k];
    c->alive = 1;
    c->speed = 3;
  }
}

void cars_build(void) {
  for (int t = 0; t < VT_N; t++) {
    for (int i = 0; i < NCOL; i++) {
      if (VS[t].fixedCol && i > 0) { carMesh[t][i] = carMesh[t][0]; continue; }
      MB b;
      mb_init(&b);
      build_body(&b, t, VS[t].fixedCol ? VS[t].col : COLS[i]);
      carMesh[t][i] = gm_upload(&b);
      mb_free(&b);
    }
    MB w;
    mb_init(&w);
    build_wheel(&w, VS[t].wr);
    wheelMesh[t] = gm_upload(&w);
    mb_free(&w);
  }
}

/* ---------------------------------------------------------------- wsiadanie / wysiadanie */
int cars_near(void) {
  int best = -1;
  float bd = 1.9f;
  for (int i = 0; i < ncars; i++) {
    Car *c = &cars[i];
    if (!c->alive) continue;
    float d = sqrtf((c->x - W.px) * (c->x - W.px) + (c->z - W.pz) * (c->z - W.pz));
    if (d < bd) { bd = d; best = i; }
  }
  return best;
}

int cars_enter(void) {
  int i = cars_near();
  if (i < 0) return 0;
  Car *c = &cars[i];
  if (c->ai) {
    /* kierowca ucieka z auta */
    c->ai = 0;
    ui_toast("Kierowca ucieka w popłochu");
  }
  drive = i;
  enterT = 20;
  c->speed = 0;
  camYaw = c->ang; camOff = 0; camInit = 1;
  sfx_play(SFX_DOOR);
  static int hint;
  if (!hint++) ui_toast("W/S — gaz/hamulec, A/D — skręt, LPM — klakson, E — wysiądź");
  return 1;
}

static void cars_exit(void) {
  Car *c = &cars[drive];
  if (fabsf(c->speed) > 2.5f) return;
  /* drzwi kierowcy po lewej, potem prawa strona, tył, przód */
  float fx = fwdx(c->ang), fz = fwdz(c->ang);
  float lx = fz, lz = -fx; /* lewo względem przodu */
  float cand[4][2] = {{lx * 0.95f, lz * 0.95f}, {-lx * 0.95f, -lz * 0.95f}, {-fx * 1.6f, -fz * 1.6f}, {fx * 1.6f, fz * 1.6f}};
  for (int k = 0; k < 4; k++) {
    float x = c->x + cand[k][0], z = c->z + cand[k][1];
    if (world_solid_at(x - 0.2f, z - 0.2f) || world_solid_at(x + 0.2f, z + 0.2f) || world_solid_at(x - 0.2f, z + 0.2f) || world_solid_at(x + 0.2f, z - 0.2f)) continue;
    W.px = W.ppx = x; W.pz = W.ppz = z;
    W.yaw = c->ang + PI_F / 2;
    W.pitch = 0;
    c->speed = 0;
    drive = -1;
    sfx_play(SFX_DOOR);
    audio_engine(0, 0);
    return;
  }
}

/* ---------------------------------------------------------------- fizyka */
static void car_physics(Car *c, float thr, float steerIn, float dt) {
  const VSpec *vs = &VS[c->type];
  const float vmax = vs->vmax;
  c->prevSpeed = c->speed;
  if (thr > 0) {
    if (c->speed < -0.2f) c->speed += 16 * dt;
    else c->speed += vs->accel * (1 - c->speed / vmax) * dt;
  } else if (thr < 0) {
    if (c->speed > 0.2f) c->speed -= 15 * dt;
    else c->speed -= 4.5f * dt;
    if (c->speed < -4.5f) c->speed = -4.5f;
  } else {
    float f = 2.0f * dt;
    c->speed = fabsf(c->speed) < f ? 0 : c->speed - (c->speed > 0 ? f : -f);
  }
  c->speed *= 1 - 0.12f * dt;
  c->steer += (steerIn - c->steer) * fminf(1, 7 * dt);
  float sp = fabsf(c->speed);
  float yawRate = c->steer * vs->grip * fminf(1, sp / 2.5f) * (1 - 0.45f * fminf(1, sp / vmax));
  if (c->speed < 0) yawRate = -yawRate;
  float na = c->ang + yawRate * dt;
  float nx = c->x + fwdx(na) * c->speed * dt, nz = c->z + fwdz(na) * c->speed * dt;
  if (!car_blocked_t(c->type, nx, nz, na)) { c->x = nx; c->z = nz; c->ang = na; }
  else if (!car_blocked_t(c->type, nx, c->z, na)) { c->x = nx; c->ang = na; c->speed *= 0.9f; }
  else if (!car_blocked_t(c->type, c->x, nz, na)) { c->z = nz; c->ang = na; c->speed *= 0.9f; }
  else {
    if (fabsf(c->speed) > 3.5f && c->crashT <= 0) {
      sfx_play_at(SFX_CRIT, sqrtf((c->x - W.px) * (c->x - W.px) + (c->z - W.pz) * (c->z - W.pz)));
      if (c == &cars[drive]) gfx_shake((int)fminf(12, fabsf(c->speed)));
      c->crashT = 0.5f;
    }
    c->speed *= -0.25f;
  }
  if (c->crashT > 0) c->crashT -= dt;
  c->wheelT += c->speed * dt / vs->wr;
  /* przechył w zakręcie i nurkowanie przy hamowaniu (wygładzone) */
  float acc = (c->speed - c->prevSpeed) / dt;
  c->lean += (-yawRate * c->speed * 0.012f - c->lean) * fminf(1, 6 * dt);
  c->pitch += (CLAMP(acc * 0.004f, -0.05f, 0.05f) - c->pitch) * fminf(1, 5 * dt);
  c->ang = wrap_pi(c->ang);
}

static void car_vs_world(int i) {
  Car *c = &cars[i];
  /* inne auta */
  for (int j = 0; j < ncars; j++) {
    if (j == i || !cars[j].alive) continue;
    float nx = 0, nz = 0;
    if (cars_touch(c, c->x, c->z, &cars[j], &nx, &nz)) {
      float rel = fabsf(c->speed - cars[j].speed);
      if (!car_blocked_t(c->type, c->x + nx * 0.05f, c->z + nz * 0.05f, c->ang)) { c->x += nx * 0.05f; c->z += nz * 0.05f; }
      /* hamuje tylko wtedy, gdy jedzie na przeszkodę */
      float vx = fwdx(c->ang) * c->speed, vz = fwdz(c->ang) * c->speed;
      if (vx * nx + vz * nz < 0) {
        if (rel > 3 && c->crashT <= 0) {
          sfx_play_at(SFX_CRIT, sqrtf((c->x - W.px) * (c->x - W.px) + (c->z - W.pz) * (c->z - W.pz)));
          if (i == drive) gfx_shake(8);
          c->crashT = 0.5f;
          /* zderzenie popycha drugie auto */
          if (!cars[j].ai || i == drive) cars[j].speed += c->speed * 0.4f * (fwdx(c->ang) * fwdx(cars[j].ang) + fwdz(c->ang) * fwdz(cars[j].ang));
        }
        c->speed *= 0.35f;
      }
    }
  }
  /* przechodnie i wrogowie */
  for (int k = 0; k < W.n; k++) {
    Ent *e = &W.ents[k];
    if (!e->vis || e->dead || (e->d->type != ENT_NPC && e->d->type != ENT_MOB)) continue;
    float dx = e->x - c->x, dz = e->z - c->z;
    float lx = dx * fwdx(c->ang) + dz * fwdz(c->ang);
    float lz = -dx * fwdz(c->ang) + dz * fwdx(c->ang);
    if (fabsf(lx) > VS[c->type].L + 0.2f || fabsf(lz) > VS[c->type].W + 0.2f) continue;
    if (e->hostile && fabsf(c->speed) > 4.0f && i == drive) {
      combat_damage_ent(e, 200);
      gfx_shake(6);
      continue;
    }
    /* odskakuje na bok */
    float side = lz >= 0 ? 1 : -1;
    float px = -fwdz(c->ang) * side, pz = fwdx(c->ang) * side;
    move_circle(&e->x, &e->z, px * 0.12f, pz * 0.12f, 0.2f, e);
    if (fabsf(c->speed) > 1.5f) {
      c->speed *= 0.5f;
      if (c->hornT <= 0 && i == drive) { sfx_play(SFX_HURT); c->hornT = 40; }
    }
  }
  if (c->hornT > 0) c->hornT--;
}

static void ai_drive(int i) {
  Car *c = &cars[i];
  const Route *rt = &ROUTES[c->route];
  const float *t = rt->p[c->wp];
  float dx = t[0] - c->x, dz = t[1] - c->z;
  float dist = sqrtf(dx * dx + dz * dz);
  if (dist < 1.2f) { c->wp = (c->wp + 1) % rt->n; return; }
  float want = atan2f(-dz, dx);
  float da = wrap_pi(want - c->ang);
  float steer = CLAMP(da * 2.2f, -1, 1);
  float target = dist < 5 ? 2.6f : fabsf(da) > 0.4f ? 2.4f : 5.5f;
  /* przeszkoda z przodu: auto, gracz, przechodzień */
  float fx = fwdx(c->ang), fz = fwdz(c->ang);
  int block = 0;
  for (int j = 0; j < ncars && !block; j++) {
    if (j == i || !cars[j].alive) continue;
    float ox = cars[j].x - c->x, oz = cars[j].z - c->z;
    float ahead = ox * fx + oz * fz, side = fabsf(-ox * fz + oz * fx);
    if (ahead > 0 && ahead < 3.6f && side < 1.0f) block = 1;
  }
  if (drive < 0) {
    float ox = W.px - c->x, oz = W.pz - c->z;
    float ahead = ox * fx + oz * fz, side = fabsf(-ox * fz + oz * fx);
    if (ahead > 0 && ahead < 3.0f && side < 0.9f) block = 2;
  }
  for (int k = 0; k < W.n && !block; k++) {
    Ent *e = &W.ents[k];
    if (!e->vis || e->dead || (e->d->type != ENT_NPC && e->d->type != ENT_MOB)) continue;
    float ox = e->x - c->x, oz = e->z - c->z;
    float ahead = ox * fx + oz * fz, side = fabsf(-ox * fz + oz * fx);
    if (ahead > 0 && ahead < 2.6f && side < 0.8f) block = 1;
  }
  float thr;
  if (block) {
    thr = c->speed > 0.1f ? -1 : 0;
    if (++c->stuckT > 150 && c->hornT <= 0) {
      float d = sqrtf((c->x - W.px) * (c->x - W.px) + (c->z - W.pz) * (c->z - W.pz));
      if (d < 14) sfx_play_at(SFX_HORN, d + 4);
      c->hornT = 240;
    }
  } else {
    c->stuckT = 0;
    thr = c->speed < target ? 1 : (c->speed > target + 0.5f ? -1 : 0);
  }
  car_physics(c, thr, steer, 1.0f / 60);
}

void cars_update(void) {
  float dt = 1.0f / 60;
  for (int i = 0; i < ncars; i++) {
    Car *c = &cars[i];
    if (!c->alive) continue;
    if (i == drive) {
      int ctl = !script_blocking() && !W.trans;
      float thr = 0, st = 0;
      if (ctl) {
        if (in.held[BTN_UP]) thr += 1;
        if (in.held[BTN_DOWN]) thr -= 1;
        if (in.held[BTN_SL] || in.held[BTN_LEFT]) st += 1;
        if (in.held[BTN_SR] || in.held[BTN_RIGHT]) st -= 1;
        if (in.pressed[BTN_FIRE]) sfx_play(SFX_HORN);
        if (enterT > 0) enterT--;
        else if (in.pressed[BTN_A] && fabsf(c->speed) < 2.5f) { cars_exit(); continue; }
      } else thr = c->speed > 0.2f ? -1 : 0;
      car_physics(c, thr, st, dt);
      car_vs_world(i);
      W.ppx = W.px; W.ppz = W.pz;
      W.px = c->x; W.pz = c->z;
      audio_engine(fminf(1, fabsf(c->speed) / VS[c->type].vmax), 1);
      /* kamera: kierunek auta z opóźnieniem + odchylenie myszą */
      camOff -= in.mdx * 0.004f;
      if (fabsf(in.mdx) < 0.01f) camT += dt; else camT = 0;
      if (camT > 1.2f) camOff *= 0.94f;
      camOff = CLAMP(camOff, -2.6f, 2.6f);
      float target = c->ang + (c->speed < -0.5f ? PI_F : 0);
      if (!camInit) { camYaw = target; camInit = 1; }
      camYaw += wrap_pi(target - camYaw) * fminf(1, 3.2f * dt);
    } else if (c->ai) {
      ai_drive(i);
      car_vs_world(i);
    } else if (fabsf(c->speed) > 0.01f) {
      car_physics(c, 0, 0, dt);
      car_vs_world(i);
    }
  }
}

/* kamera zza auta; zwraca 1, gdy gracz prowadzi */
int cars_camera(RCam *cam, float alpha) {
  if (drive < 0) return 0;
  Car *c = &cars[drive];
  (void)alpha;
  float yaw = camYaw + camOff;
  float fx = fwdx(yaw), fz = fwdz(yaw);
  float want = 3.6f + fminf(1.4f, fabsf(c->speed) * 0.1f);
  /* kamera nie wchodzi w ściany */
  float d = 1.0f;
  for (; d < want; d += 0.2f)
    if (world_solid_at(c->x - fx * d, c->z - fz * d)) { d -= 0.3f; break; }
  camDist += (d - camDist) * 0.2f;
  if (camDist > d) camDist = d;
  float tx = c->x, ty = 0.62f, tz = c->z;
  float px = c->x - fx * camDist, py = 1.25f + camDist * 0.08f, pz = c->z - fz * camDist;
  cam->pos = v3(px, py, pz);
  cam->yaw = atan2f(tx - px, tz - pz);
  float hd = sqrtf((tx - px) * (tx - px) + (tz - pz) * (tz - pz));
  cam->pitch = atan2f(ty - py, hd);
  cam->fov = (float)g_cfg.fov + fminf(10, fabsf(c->speed) * 0.7f);
  if (getenv("KX_CAR")) fprintf(stderr, "auto %.2f %.2f ang %.2f v %.2f | kam %.2f %.2f yaw %.2f\n", c->x, c->z, c->ang, c->speed, px, pz, cam->yaw);
  return 1;
}

void cars_lights(int night) {
  /* reflektory: auto gracza i najbliższe auta AI */
  for (int i = 0; i < ncars; i++) {
    Car *c = &cars[i];
    if (!c->alive || (!c->ai && i != drive)) continue;
    float d = fabsf(c->x - W.px) + fabsf(c->z - W.pz);
    if (i != drive && d > 30) continue;
    float fx = fwdx(c->ang), fz = fwdz(c->ang);
    float k = night ? 1.0f : 0.3f;
    if (i == drive || d < 14) r_light(c->x + fx * 2.4f, 0.5f, c->z + fz * 2.4f, 2.2f * k, 2.0f * k, 1.5f * k, 5.0f);
  }
}

void cars_draw(void) {
  for (int i = 0; i < ncars; i++) {
    Car *c = &cars[i];
    if (!c->alive) continue;
    float d2 = (c->x - W.px) * (c->x - W.px) + (c->z - W.pz) * (c->z - W.pz);
    if (d2 > 90 * 90) continue;
    const VSpec *vs = &VS[c->type];
    float bob = (i == drive || c->ai) ? sinf(c->wheelT * 0.7f) * 0.003f * fminf(1, fabsf(c->speed)) : 0;
    M4 base = m4_mul(m4_translate(c->x, 0, c->z), m4_roty(c->ang));
    /* nadwozie: zawieszenie (przechył, nurkowanie) */
    M4 body = m4_mul(base, m4_mul(m4_translate(0, bob + 0.02f, 0), m4_mul(m4_rotx(c->lean), m4_rotz(c->pitch))));
    r_draw(&carMesh[c->type][c->col], &body, 0);
    /* koła: obrót wokół osi, przednie skręcają */
    if (d2 < 45 * 45)
      for (int w = 0; w < 4; w++) {
        float wx = (w & 1) ? vs->wfx : vs->wrx, wz = (w & 2) ? vs->W - 0.02f : -vs->W + 0.02f;
        float st = (w & 1) ? c->steer * 0.45f : 0;
        M4 wm = m4_mul(base, m4_mul(m4_translate(wx, vs->wr, wz), m4_mul(m4_roty(st), m4_rotz(-c->wheelT))));
        r_draw(&wheelMesh[c->type], &wm, 0);
      }
    if (c->ai || i == drive) {
      float fx = fwdx(c->ang), fz = fwdz(c->ang), lx = fz, lz = -fx;
      for (int s = -1; s <= 1; s += 2) {
        r_glow(c->x + fx * (vs->L - 0.02f) + lx * 0.3f * s, 0.5f, c->z + fz * (vs->L - 0.02f) + lz * 0.3f * s, 0.32f, 1.1f, 1.0f, 0.75f);
        r_glow(c->x - fx * (vs->L + 0.02f) + lx * 0.32f * s, 0.42f, c->z - fz * (vs->L + 0.02f) + lz * 0.32f * s, 0.14f, 0.9f, 0.08f, 0.05f);
      }
      if (c->type == VT_POLICE) { /* koguty: czerwono-niebieskie błyski */
        float ph = fmodf(g_time * 3 + i, 1.0f);
        r_glow(c->x - fx * 0.1f, 0.96f, c->z - fz * 0.1f, 0.5f, ph < 0.5f ? 1.6f : 0.1f, 0.1f, ph < 0.5f ? 0.1f : 1.8f);
      }
      if (c->type == VT_TAXI) r_glow(c->x - fx * 0.16f, 0.92f, c->z - fz * 0.16f, 0.3f, 0.8f, 0.6f, 0.2f);
      /* spaliny przy ruszaniu i na zimno */
      if (fabsf(c->speed) > 0.3f && d2 < 30 * 30) {
        float ph = fmodf(g_time * 1.6f + i * 0.3f, 1.0f);
        r_puff(c->x - fx * (vs->L + 0.15f + ph * 0.6f), 0.18f + ph * 0.25f, c->z - fz * (vs->L + 0.15f + ph * 0.6f), 0.12f + ph * 0.25f, 0.35f, 0.34f, 0.33f, 0.18f * (1 - ph));
      }
    }
  }
}

/* test: wszystkie typy aut w rzędzie przed graczem */
void cars_showcase(void) {
  ncars = 0;
  for (int t = 0; t < VT_N; t++) {
    Car *c = &cars[ncars++];
    memset(c, 0, sizeof *c);
    c->x = W.px + 3.0f + t * 2.4f; c->z = W.pz + (t & 1 ? 1.1f : -1.1f);
    c->ang = (t & 1) ? 2.3f : 0.85f; /* ukośnie, widok 3/4 */ c->type = t; c->col = (t * 3) % NCOL; c->alive = 1;
  }
}

/* prędkościomierz (mph) */
void cars_hud(void) {
  if (drive < 0) return;
  Car *c = &cars[drive];
  float mph = fabsf(c->speed) * 1.9f * 2.237f;
  float x = UI_W - 270, y = UI_H - 110, w = 240, h = 80;
  d2_shadow(x, y, w, h, 12, 12, 0x80000000);
  d2_rrect(x, y, w, h, 12, 0xD0121016);
  d2_rrect_line(x, y, w, h, 12, 1.5f, 0xA0E8B84A);
  char b[32];
  snprintf(b, sizeof b, "%d", (int)(mph + 0.5f));
  d2_text_sh(FONT_SERIF, 46, x + 22, y + 12, b, UI_CREAM);
  d2_text(FONT_BOLD, 16, x + 120, y + 22, "MPH", UI_GOLD);
  d2_text(FONT_SANS, 14, x + 120, y + 46, c->speed < -0.3f ? "wsteczny" : VS[c->type].name, UI_GREY);
  /* pasek */
  d2_rrect(x + 16, y + h - 12, w - 32, 4, 2, 0x50FFFFFF);
  d2_rrect(x + 16, y + h - 12, (w - 32) * fminf(1, mph / 60), 4, 2, UI_GOLD);
}
