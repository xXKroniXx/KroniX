/* Budowanie świata 3D z mapy ASCII: ulice z krawężnikami, kamienice z oknami we wnękach,
 * gzymsy, schody przeciwpożarowe, witryny, markizy, szyldy i neony, wnętrza z boazerią,
 * sufity z blachy tłoczonej, meble i oświetlenie. Cała mapa = jedna siatka statyczna. */
#include "engine.h"
#include "assets.h"

/* ---------------------------------------------------------------- kafelki */
enum { K_FLOOR, K_BLOCK, K_PROP, K_WATER, K_BB, K_CAR };
typedef struct { char c; int kind; int floorL; int wallL; float h; } TDef;
static const TDef TD[] = {
  {'.', K_FLOOR, L_ASPHALT}, {'-', K_FLOOR, L_ASPHALT}, {'|', K_FLOOR, L_ASPHALT}, {',', K_FLOOR, L_SIDEWALK},
  {':', K_FLOOR, L_COBBLE}, {'"', K_FLOOR, L_GRASS}, {'_', K_FLOOR, L_WOODFLOOR}, {'q', K_FLOOR, L_CHECKER},
  {'r', K_FLOOR, L_CARPET}, {'o', K_FLOOR, L_CONCRETE}, {'=', K_FLOOR, L_WOODFLOOR}, {'y', K_FLOOR, L_CLOTH}, {'s', K_FLOOR, L_STONE},
  {'R', K_BLOCK, L_SIDEWALK, L_BRICK}, {'W', K_BLOCK, L_SIDEWALK, L_BRICK}, {'w', K_BLOCK, L_SIDEWALK, L_BRICK},
  {'G', K_BLOCK, L_SIDEWALK, L_BRICK}, {'a', K_BLOCK, L_SIDEWALK, L_BRICK}, {'D', K_BLOCK, L_SIDEWALK, L_BRICK},
  {'#', K_BLOCK, L_CONCRETE, L_STONE}, {'g', K_BLOCK, L_CONCRETE, L_STONE}, {'i', K_BLOCK, L_WOODFLOOR, L_WALLPAPER_GREEN},
  {'j', K_BLOCK, L_WOODFLOOR, L_WALLPAPER_RED}, {'M', K_BLOCK, L_CONCRETE, L_CORRUGATED}, {'Q', K_BLOCK, L_CONCRETE, L_PLASTER},
  {'x', K_BLOCK, L_CONCRETE, L_ROOF}, {'H', K_BLOCK, L_SIDEWALK, L_STONE}, {'z', K_BLOCK, L_WOODFLOOR, L_VELVET}, {'n', K_BLOCK, L_SIDEWALK, L_BRICK},
  {'X', K_PROP, L_CONCRETE, 0, 0.55f}, {'k', K_PROP, L_CONCRETE, 0, 0.55f}, {'t', K_PROP, L_WOODFLOOR, 0, 0.42f},
  {'h', K_PROP, L_WOODFLOOR, 0, 0.28f}, {'b', K_PROP, L_WOODFLOOR, 0, 0.6f}, {'S', K_PROP, L_WOODFLOOR, 0, 1.4f},
  {'P', K_PROP, L_WOODFLOOR, 0, 0.6f}, {'V', K_PROP, L_CONCRETE, 0, 1.3f}, {'d', K_PROP, L_WOODFLOOR, 0, 0.45f},
  {'e', K_PROP, L_WOODFLOOR, 0, 0.35f}, {'B', K_PROP, L_WOODFLOOR, 0, 0.3f}, {'u', K_PROP, L_WOODFLOOR, 0, 1.4f},
  {'f', K_PROP, L_WOODFLOOR, 0, 1.0f}, {'A', K_PROP, L_STONE, 0, 0.55f}, {'Y', K_PROP, L_CLOTH, 0, 0.8f},
  {'F', K_PROP, L_SIDEWALK, 0, 0.6f}, {'I', K_PROP, L_CONCRETE, 0, 1.6f}, {'m', K_PROP, L_CONCRETE, 0, 0.9f},
  {'~', K_WATER, L_WATER}, {'T', K_BB, L_GRASS}, {'p', K_BB, L_WOODFLOOR}, {'L', K_BB, L_SIDEWALK},
  {'c', K_CAR, L_ASPHALT}, {'C', K_CAR, L_ASPHALT},
};
#define NTD ((int)(sizeof(TD) / sizeof(TD[0])))
static const TDef *tdef[128];
static void tdef_init(void) {
  static int done;
  if (done) return;
  done = 1;
  for (int i = 0; i < NTD; i++) tdef[(int)TD[i].c] = &TD[i];
}
int tile_valid(int c) { tdef_init(); return c > 0 && c < 128 && tdef[c] != NULL; }
int tile_solid(int c) { tdef_init(); if (c <= 0 || c >= 128 || !tdef[c]) return 1; return tdef[c]->kind != K_FLOOR; }
static const TDef *td_c(int c) { tdef_init(); return (c > 0 && c < 128 && tdef[c]) ? tdef[c] : tdef['x']; }
int tile_blocks_sight(int c) {
  const TDef *t = td_c(c);
  if (t->kind == K_BLOCK) return 1;
  if (t->kind == K_PROP && t->h > 1.0f && c != 'F' && c != 'I') return 1;
  return 0;
}
float tile_prop_height(int c) {
  const TDef *t = td_c(c);
  if (t->kind == K_BLOCK) return 9;
  if (t->kind == K_PROP) return t->h;
  if (t->kind == K_CAR) return 0.8f;
  if (c == 'T') return 2.4f;
  if (c == 'L') return 0.0f;
  return 0;
}

/* ---------------------------------------------------------------- stan mapy */
static MapDef *Mp;
static int Mw, Mh;
static int tile(int x, int z) { return (x < 0 || z < 0 || x >= Mw || z >= Mh) ? 'x' : Mp->tiles[z][x]; }
static const TDef *td(int x, int z) { return td_c(tile(x, z)); }
static int isfloor(int x, int z) { return td(x, z)->kind == K_FLOOR; }
static int isblock(int x, int z) { return td(x, z)->kind == K_BLOCK; }
static int open_t(int x, int z) { int k = td(x, z)->kind; return k != K_BLOCK && !(x < 0 || z < 0 || x >= Mw || z >= Mh); }
static uint32_t H32(int a, int b, int c) {
  uint32_t h = (uint32_t)a * 73856093u ^ (uint32_t)b * 19349663u ^ (uint32_t)c * 83492791u;
  h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
  return h;
}

typedef struct { float x, y, z, r, g, b, rad; int kind; float phase; } CLight;
static CLight clights[400];
static int nclights;
static GMesh cityMesh;
static MB mb, mbFar;
#define CHS 16
#define MAXCH 96
typedef struct { GMesh m; float x0, z0, x1, z1; } Chunk;
static Chunk chunks[MAXCH];
static int nchunks;
static void split_chunks(void);
/* duże miasto budujemy raz: po wyjściu z wnętrza wraca gotowa geometria */
typedef struct { MapDef *m; GMesh far; Chunk ch[MAXCH]; int nch; CLight l[400]; int nl; } CitySlot;
static CitySlot bigSlot;
static int curIsBig;
static int interior;

static void addlight(float x, float y, float z, float r, float g, float b, float rad, int kind) {
  if (nclights >= 400) return;
  clights[nclights++] = (CLight){x, y, z, r, g, b, rad, kind, (x * 3.7f + z * 1.3f)};
}

/* ---------------------------------------------------------------- budynki */
static float bheight(int x, int z) {
  if (interior) return Mp->ceil;
  uint32_t h = H32(x / 5, z / 6, 7);
  if (tile(x, z) == 'H') return (Mp->floors + 5 + (int)(H32(x / 4, z / 3, 8) % 6)) * 1.5f; /* wieżowce Loopu */
  return (Mp->floors + (int)(h % 3)) * 1.5f;
}
static int bmaterial(int x, int z, int c) {
  if (c == '#' || c == 'g') return L_STONE;
  if (c == 'M') return L_CORRUGATED;
  if (c == 'H') return H32(x / 4, z / 3, 9) % 3 ? L_STONE : L_PLASTER;
  uint32_t h = H32(x / 5, z / 6, 11);
  int k = h % 6;
  return k < 3 ? L_BRICK : k < 5 ? L_BRICK_DARK : L_STONE;
}
static uint32_t bcolor(int x, int z) {
  uint32_t h = H32(x / 5, z / 6, 13);
  static const uint32_t C[6] = {0xFFFFFFFF, 0xFFF2E6E0, 0xFFE8E0D8, 0xFFFFF0E6, 0xFFE0D8D0, 0xFFF8F0E8};
  return C[h % 6];
}

/* ściana z otworem okiennym: pas wzdłuż wektora (ux,uz), normalna (nx,nz) */
typedef struct { float ox, oz, ux, uz, nx, nz; } Face; /* początek krawędzi, kierunek wzdłuż (długość 1), normalna */

static V3 fp(const Face *f, float s, float y, float d) { return v3(f->ox + f->ux * s + f->nx * d, y, f->oz + f->uz * s + f->nz * d); }
/* czworokąt na fasadzie: od s0 do s1 wzdłuż, y0..y1, wysunięty o d */
static void fquad(const Face *f, float s0, float s1, float y0, float y1, float d) {
  mb_quad_world(&mb, fp(f, s0, y0, d), fp(f, s1, y0, d), fp(f, s1, y1, d), fp(f, s0, y1, d));
}
static void fquad_uv(const Face *f, float s0, float s1, float y0, float y1, float d, float u0, float v0, float u1, float v1) {
  mb_quad(&mb, fp(f, s0, y0, d), fp(f, s1, y0, d), fp(f, s1, y1, d), fp(f, s0, y1, d), u0, v0, u1, v1);
}
/* prostopadłościan przyklejony do fasady */
static void fbox(const Face *f, float s0, float s1, float y0, float y1, float d0, float d1) {
  V3 a = fp(f, s0, y0, d0), b = fp(f, s1, y1, d1);
  mb_box(&mb, v3(fminf(a.x, b.x), y0, fminf(a.z, b.z)), v3(fmaxf(a.x, b.x), y1, fmaxf(a.z, b.z)));
}

/* okno we wnęce: ściana wokół, ościeża, parapet, szyba (opcjonalnie zapalona) */
static void window_unit(const Face *f, float y, float wallL, uint32_t wcol, float ww, float wh, float wy, int lit, int shop) {
  float s0 = 0.5f - ww / 2, s1 = 0.5f + ww / 2, y0 = y + wy, y1 = y0 + wh;
  float dep = 0.07f;
  mb_paint(wcol, (int)wallL);
  fquad(f, 0, s0, y, y + 1.5f, 0);
  fquad(f, s1, 1, y, y + 1.5f, 0);
  fquad(f, s0, s1, y, y0, 0);
  fquad(f, s0, s1, y1, y + 1.5f, 0);
  /* ościeża */
  mb_paint(0xFFB8B0A0, L_PLASTER);
  mb_quad_world(&mb, fp(f, s0, y0, 0), fp(f, s0, y0, -dep), fp(f, s0, y1, -dep), fp(f, s0, y1, 0));
  mb_quad_world(&mb, fp(f, s1, y0, -dep), fp(f, s1, y0, 0), fp(f, s1, y1, 0), fp(f, s1, y1, -dep));
  mb_quad_world(&mb, fp(f, s0, y1, 0), fp(f, s0, y1, -dep), fp(f, s1, y1, -dep), fp(f, s1, y1, 0));
  /* parapet i nadproże */
  mb_paint(0xFFCFC6B4, L_STONE);
  fbox(f, s0 - 0.04f, s1 + 0.04f, y0 - 0.045f, y0, -dep, 0.045f);
  if (!shop) fbox(f, s0 - 0.03f, s1 + 0.03f, y1, y1 + 0.06f, 0, 0.02f);
  /* szyba */
  if (shop) {
    mb_paint(lit ? 0xFFFFE6B8 : 0xFF8A8A8A, L_SHOPWIN);
    P_.emis = lit ? 0.55f : 0.0f;
  } else {
    uint32_t h = H32((int)(f->ox * 7), (int)(f->oz * 7), (int)(y * 10));
    if (lit) {
      static const uint32_t W[4] = {0xFFFFD49A, 0xFFFFC27A, 0xFFF0E0B0, 0xFFFFB070};
      mb_paint(W[h % 4], L_WINDOW);
      P_.emis = 0.35f + (h % 5) * 0.06f;
    } else mb_paint(0xFF4A5868, L_WINDOW);
  }
  fquad_uv(f, s0, s1, y0, y1, -dep, 0, 0, 1, 1);
}

static void cornice(const Face *f, float h) {
  mb_paint(0xFFC8BEA8, L_STONE);
  fbox(f, -0.0f, 1.0f, h - 0.22f, h - 0.12f, 0, 0.08f);
  fbox(f, -0.0f, 1.0f, h - 0.12f, h, 0, 0.14f);
  mb_paint(0xFF9A9080, L_STONE);
  fbox(f, 0, 1, 1.42f, 1.5f, 0, 0.05f);
}

static void fire_escape(const Face *f, float h) {
  for (float y = 1.5f; y < h - 0.3f; y += 1.5f) {
    mb_paint(0xFFFFFFFF, L_GRATE); P_.flags = VF_ALPHATEST;
    V3 a = fp(f, -0.2f, y, 0.02f), b = fp(f, 1.2f, y, 0.02f), c = fp(f, 1.2f, y, 0.5f), d = fp(f, -0.2f, y, 0.5f);
    mb_quad(&mb, a, d, c, b, 0, 0, 2, 0.7f);
    mb_quad(&mb, a, b, c, d, 0, 0, 2, 0.7f);
    mb_paint(0xFF2A2C30, L_METAL);
    fbox(f, -0.2f, 1.2f, y + 0.42f, y + 0.45f, 0.47f, 0.5f);
    for (int k = 0; k < 5; k++) fbox(f, -0.2f + k * 0.35f, -0.18f + k * 0.35f, y, y + 0.45f, 0.48f, 0.5f);
    fbox(f, -0.2f, 1.2f, y - 0.03f, y, 0.47f, 0.5f);
    /* drabina */
    if (y + 1.5f < h - 0.3f) {
      for (int s = 0; s < 2; s++) fbox(f, 0.8f + s * 0.25f, 0.82f + s * 0.25f, y, y + 1.5f, 0.3f, 0.32f);
      for (int r = 1; r < 8; r++) fbox(f, 0.8f, 1.07f, y + r * 0.19f, y + r * 0.19f + 0.02f, 0.3f, 0.32f);
    }
  }
}

static int neonIdx;
static void shop_front(const Face *f, int c, uint32_t wcol, int wallL, int tx, int tz) {
  int lit = Mp->night || 1;
  window_unit(f, 0, wallL, wcol, 0.78f, 0.85f, 0.18f, lit, 1);
  /* szyld nad witryną */
  uint32_t h = H32(tx, tz, 21);
  int sign = Mp->district == 1 && h % 4 == 0 ? 8 : (int)(h % 16);
  if (sign == 8 || sign == 9 || sign == 15 || sign == 10) sign = (int)(h % 8);
  float row = (sign / 2) / 8.0f, col = (sign % 2) * 0.5f;
  mb_paint(0xFFFFFFFF, L_SIGNS);
  P_.emis = Mp->night ? 0.25f : 0;
  fquad_uv(f, 0.06f, 0.94f, 1.1f, 1.32f, 0.03f, col, row, col + 0.5f, row + 0.125f);
  mb_paint(0xFF2A2420, L_WOOD);
  fbox(f, 0.04f, 0.96f, 1.08f, 1.34f, 0, 0.028f);
  if (lit) addlight(fp(f, 0.5f, 0, 0.6f).x, 0.75f, fp(f, 0.5f, 0, 0.6f).z, 1.6f, 1.2f, 0.7f, 2.6f, 0);
  if (c == 'a') {
    /* markiza */
    mb_paint(H32(tx, tz, 22) % 2 ? 0xFFFFFFFF : 0xFFB8D8B8, L_AWNING); P_.flags = VF_ALPHATEST;
    V3 a = fp(f, -0.02f, 1.05f, 0.62f), b = fp(f, 1.02f, 1.05f, 0.62f), cc = fp(f, 1.02f, 1.4f, 0.0f), d = fp(f, -0.02f, 1.4f, 0.0f);
    mb_quad(&mb, a, b, cc, d, 0, 1, 2, 0);
    mb_quad(&mb, b, a, d, cc, 0, 1, 2, 0);
    mb_paint(0xFF22252A, L_METAL);
    fbox(f, -0.02f, 0.0f, 1.0f, 1.05f, 0.0f, 0.62f);
    fbox(f, 1.0f, 1.02f, 1.0f, 1.05f, 0.0f, 0.62f);
  }
  /* neon prostopadły do ściany */
  if (Mp->night && H32(tx, tz, 23) % 3 == 0) {
    int w = neonIdx++ % 8;
    static const float NC[8][3] = {{0.3f, 0.8f, 1.0f}, {1.0f, 0.25f, 0.35f}, {1.0f, 0.75f, 0.25f}, {0.4f, 1.0f, 0.55f}, {1.0f, 0.4f, 0.8f}, {1.0f, 0.3f, 0.2f}, {0.4f, 0.6f, 1.0f}, {1.0f, 0.8f, 0.35f}};
    mb_paint(0xFFFFFFFF, L_NEON); P_.emis = 1.0f;
    V3 a = fp(f, 0.88f, 1.55f, 0.08f), b = fp(f, 0.88f, 1.55f, 0.72f), cc = fp(f, 0.88f, 1.85f, 0.72f), d = fp(f, 0.88f, 1.85f, 0.08f);
    mb_quad(&mb, a, b, cc, d, 0, w / 8.0f, 1, (w + 1) / 8.0f);
    mb_quad(&mb, b, a, d, cc, 0, w / 8.0f, 1, (w + 1) / 8.0f);
    mb_paint(0xFF1A1A1E, L_METAL);
    fbox(f, 0.86f, 0.9f, 1.52f, 1.55f, 0.05f, 0.75f);
    fbox(f, 0.86f, 0.9f, 1.85f, 1.88f, 0.05f, 0.75f);
    fbox(f, 0.86f, 0.9f, 1.52f, 1.88f, 0.72f, 0.75f);
    V3 lp = fp(f, 0.88f, 1.7f, 0.5f);
    addlight(lp.x, lp.y, lp.z, NC[w][0] * 2.2f, NC[w][1] * 2.2f, NC[w][2] * 2.2f, 3.5f, 1);
  }
}

static void door_unit(const Face *f, int wallL, uint32_t wcol, int interiorDoor) {
  float s0 = 0.25f, s1 = 0.75f, y1 = interiorDoor ? 1.05f : 1.1f, dep = 0.1f;
  mb_paint(wcol, wallL);
  fquad(f, 0, s0, 0, interiorDoor ? Mp->ceil : 1.5f, 0);
  fquad(f, s1, 1, 0, interiorDoor ? Mp->ceil : 1.5f, 0);
  fquad(f, s0, s1, y1, interiorDoor ? Mp->ceil : 1.5f, 0);
  mb_paint(0xFF6A5A48, L_WOOD);
  mb_quad_world(&mb, fp(f, s0, 0, 0), fp(f, s0, 0, -dep), fp(f, s0, y1, -dep), fp(f, s0, y1, 0));
  mb_quad_world(&mb, fp(f, s1, 0, -dep), fp(f, s1, 0, 0), fp(f, s1, y1, 0), fp(f, s1, y1, -dep));
  mb_quad_world(&mb, fp(f, s0, y1, 0), fp(f, s0, y1, -dep), fp(f, s1, y1, -dep), fp(f, s1, y1, 0));
  mb_paint(0xFFFFFFFF, L_DOOR);
  fquad_uv(f, s0, s1, 0, y1, -dep, 0, 0, 1, 1);
  mb_paint(0xFF3A2414, L_WOOD);
  fbox(f, s0 - 0.04f, s0, 0, y1 + 0.04f, 0, 0.03f);
  fbox(f, s1, s1 + 0.04f, 0, y1 + 0.04f, 0, 0.03f);
  fbox(f, s0 - 0.04f, s1 + 0.04f, y1, y1 + 0.05f, 0, 0.03f);
  if (!interiorDoor) {
    mb_paint(0xFF8A847A, L_STONE);
    fbox(f, s0 - 0.06f, s1 + 0.06f, 0, 0.05f, -dep, 0.12f);
    /* kinkiet nad drzwiami */
    V3 p = fp(f, 0.5f, y1 + 0.22f, 0.0f);
    float yaw = atan2f(f->nx, f->nz);
    model_add(&mb, MD_SCONCE, p.x, p.y, p.z, yaw, 1.0f, 0);
    V3 lp = fp(f, 0.5f, y1 + 0.2f, 0.25f);
    addlight(lp.x, lp.y, lp.z, 1.8f, 1.35f, 0.8f, 2.8f, 0);
  }
}

static void exterior_face(const Face *f, int c, int tx, int tz, float h, float yb) {
  int wl = bmaterial(tx, tz, c);
  uint32_t wc = bcolor(tx, tz);
  for (float y = yb; y < h - 0.01f; y += 1.5f) {
    int ground = y < 0.1f;
    if (ground) {
      if (c == 'D') { door_unit(f, wl, wc, 0); continue; }
      if (c == 'G' || c == 'a') { shop_front(f, c, wc, wl, tx, tz); continue; }
      if (c == 'n') {
        mb_paint(wc, wl); fquad(f, 0, 1, y, y + 1.5f, 0);
        int pl = L_POSTER1 + (int)(H32(tx, tz, 31) % 3);
        mb_paint(0xFFFFFFFF, pl);
        fquad_uv(f, 0.15f, 0.85f, 0.25f, 1.15f, 0.01f, 0, 0, 1, 1);
        continue;
      }
      if (c == 'W' || c == 'M' || c == '#' || c == 'x') { mb_paint(wc, wl); fquad(f, 0, 1, y, y + 1.5f, 0); continue; }
    }
    if (c == 'M' || c == 'x' || c == 'W') { mb_paint(wc, wl); fquad(f, 0, 1, y, y + 1.5f, 0); continue; }
    uint32_t hh = H32(tx * 3 + (int)(f->nx * 2), tz * 3 + (int)(f->nz * 2), (int)(y * 2));
    int lit = Mp->night ? (hh % 100 < 38) : 0;
    if (y >= 4.4f) {
      /* wyższe piętra: płaska ściana + szyba (mniej geometrii, z ulicy różnica niewidoczna) */
      mb_paint(wc, wl); fquad(f, 0, 1, y, y + 1.5f, 0);
      if (lit) {
        static const uint32_t WC[4] = {0xFFFFD49A, 0xFFFFC27A, 0xFFF0E0B0, 0xFFFFB070};
        mb_paint(WC[hh % 4], L_WINDOW); P_.emis = 0.35f + (hh % 5) * 0.06f;
      } else mb_paint(0xFF4A5868, L_WINDOW);
      fquad_uv(f, 0.29f, 0.71f, y + 0.38f, y + 1.16f, 0.006f, 0, 0, 1, 1);
      continue;
    }
    window_unit(f, y, wl, wc, 0.42f, 0.78f, 0.38f, lit, 0);
  }
  cornice(f, h);
  /* zacieniony pas przy ziemi (fałszywe AO) */
  if (yb < 0.01f) {
    mb_paint(0xFF5A5650, L_CONCRETE);
    fbox(f, 0, 1, 0, 0.12f, 0, 0.025f);
  }
}

/* ---------------------------------------------------------------- wnętrza */
static void interior_face(const Face *f, int c, int tx, int tz) {
  float ch = Mp->ceil;
  const TDef *t = td_c(c);
  int wl = t->wallL;
  if (c == 'D') { door_unit(f, L_WALLPAPER_RED, 0xFFFFFFFF, 1); return; }
  if (c == 'g') { /* witraż */
    mb_paint(0xFFFFFFFF, L_STONE); fquad(f, 0, 1, 0, ch, 0);
    mb_paint(0xFFFFFFFF, L_STAINED); P_.emis = 0.45f;
    fquad_uv(f, 0.2f, 0.8f, 0.6f, ch - 0.3f, 0.01f, 0, 0, 1, 1);
    return;
  }
  if (c == 'M' || c == '#' || c == 'x' || c == 'R' || c == 'w' || c == 'W') { mb_paint(0xFFFFFFFF, wl == L_ROOF ? L_CONCRETE : wl); fquad(f, 0, 1, 0, ch, 0); return; }
  if (c == 'z') { mb_paint(0xFFFFFFFF, L_VELVET); fquad_uv(f, 0, 1, 0, ch, 0.02f, 0, 0, 1, ch); return; }
  /* boazeria + tapeta + listwy */
  mb_paint(0xFFFFFFFF, L_WAINSCOT);
  fquad_uv(f, 0, 1, 0, 0.42f, 0.03f, 0, 0.6f, 0.5f, 1.0f);
  mb_paint(0xFF3A2414, L_WOOD);
  fbox(f, 0, 1, 0.42f, 0.46f, 0, 0.05f);
  fbox(f, 0, 1, 0, 0.05f, 0, 0.04f);
  mb_paint(0xFFFFFFFF, wl);
  fquad(f, 0, 1, 0.46f, ch, 0);
  mb_paint(0xFFD8CCB0, L_PLASTER);
  fbox(f, 0, 1, ch - 0.06f, ch, 0, 0.06f);
  uint32_t h = H32(tx, tz, (int)(f->nx * 3 + f->nz * 7));
  if (h % 9 == 0 && c != 'Q') {
    mb_paint(0xFFFFFFFF, L_PAINTING);
    fquad_uv(f, 0.22f, 0.78f, 0.65f, 1.15f, 0.015f, 0, 0, 1, 1);
  } else if (h % 9 == 3) {
    V3 p = fp(f, 0.5f, 0.95f, 0);
    model_add(&mb, MD_SCONCE, p.x, p.y, p.z, atan2f(f->nx, f->nz), 1.0f, 0);
    V3 lp = fp(f, 0.5f, 0.95f, 0.25f);
    addlight(lp.x, lp.y, lp.z, 1.3f, 1.0f, 0.6f, 2.6f, 0);
  }
}

/* ---------------------------------------------------------------- podłoga, rekwizyty */
static float ground_h(int x, int z) { return tile(x, z) == ',' && !interior ? 0.06f : 0.0f; }
float world_ground(float x, float z) { return Mp ? ground_h((int)floorf(x), (int)floorf(z)) : 0; }

static int face_dir_open(int x, int z, float *yaw) {
  /* zwraca kierunek do najbliższego otwartego kafla (dla mebli przy ścianie) */
  static const int D[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
  int best = -1, bs = -1;
  for (int k = 0; k < 4; k++) {
    int nx = x + D[k][0], nz = z + D[k][1];
    if (!isfloor(nx, nz)) continue;
    int score = 1 + (isblock(x - D[k][0], z - D[k][1]) ? 4 : 0) + (isfloor(nx + D[k][0], nz + D[k][1]) ? 1 : 0);
    if (score > bs) { bs = score; best = k; }
  }
  if (best < 0) { *yaw = 0; return 0; }
  *yaw = atan2f((float)D[best][0], (float)D[best][1]);
  return 1;
}

static void floor_tile(int x, int z, int c) {
  const TDef *t = td_c(c);
  float X = (float)x, Z = (float)z;
  int fl = t->floorL;
  if (t->kind == K_CAR) fl = L_ASPHALT;
  if (t->kind == K_PROP || t->kind == K_BB) {
    /* podłoga pod rekwizytem jak sąsiedzi */
    fl = t->floorL;
    for (int k = 0; k < 4; k++) {
      int nx = x + (k == 0) - (k == 1), nz = z + (k == 2) - (k == 3);
      if (isfloor(nx, nz)) { fl = td(nx, nz)->floorL; break; }
    }
  }
  float y = (fl == L_SIDEWALK && !interior) ? 0.06f : 0;
  mb_paint(0xFFFFFFFF, fl);
  P_.uvs = (fl == L_ASPHALT) ? 0.5f : (fl == L_SIDEWALK) ? 0.5f : (fl == L_CHECKER) ? 0.5f : (fl == L_CARPET) ? 0.7f : 1.0f;
  if (c == 'y') { mb_paint(0xFFE8E0D0, L_CLOTH); y = 0.2f; }
  mb_quad_world(&mb, v3(X, y, Z), v3(X, y, Z + 1), v3(X + 1, y, Z + 1), v3(X + 1, y, Z));
  /* krawężnik */
  if (y > 0.01f && c != 'y') {
    for (int k = 0; k < 4; k++) {
      int nx = x + (k == 0) - (k == 1), nz = z + (k == 2) - (k == 3);
      int nc = tile(nx, nz);
      const TDef *n = td_c(nc);
      int lower = (n->kind == K_FLOOR && nc != ',') || n->kind == K_CAR;
      if (!lower) continue;
      mb_paint(0xFFB0ACA4, L_CONCRETE);
      if (k == 0) mb_quad_world(&mb, v3(X + 1, 0, Z + 1), v3(X + 1, 0, Z), v3(X + 1, y, Z), v3(X + 1, y, Z + 1));
      if (k == 1) mb_quad_world(&mb, v3(X, 0, Z), v3(X, 0, Z + 1), v3(X, y, Z + 1), v3(X, y, Z));
      if (k == 2) mb_quad_world(&mb, v3(X, 0, Z + 1), v3(X + 1, 0, Z + 1), v3(X + 1, y, Z + 1), v3(X, y, Z + 1));
      if (k == 3) mb_quad_world(&mb, v3(X + 1, 0, Z), v3(X, 0, Z), v3(X, y, Z), v3(X + 1, y, Z));
    }
  }
  if (c == 'y') {
    mb_paint(0xFF2A1A14, L_WOOD);
    mb_box(&mb, v3(X, 0, Z), v3(X + 1, 0.2f, Z + 1));
  }
  /* oznakowanie poziome */
  if (c == '-' || c == '|') {
    mb_paint(0xFFE8C860, L_WHITE);
    for (int s = 0; s < 2; s++) {
      if (c == '-') mb_quad_world(&mb, v3(X + 0.1f + s * 0.5f, 0.004f, Z + 0.47f), v3(X + 0.1f + s * 0.5f, 0.004f, Z + 0.53f), v3(X + 0.35f + s * 0.5f, 0.004f, Z + 0.53f), v3(X + 0.35f + s * 0.5f, 0.004f, Z + 0.47f));
      else mb_quad_world(&mb, v3(X + 0.47f, 0.004f, Z + 0.1f + s * 0.5f), v3(X + 0.47f, 0.004f, Z + 0.35f + s * 0.5f), v3(X + 0.53f, 0.004f, Z + 0.35f + s * 0.5f), v3(X + 0.53f, 0.004f, Z + 0.1f + s * 0.5f));
    }
  }
  /* sufit */
  if (interior) {
    int tin = Mp->bg[0] == 'w' ? 0 : 1;
    mb_paint(tin ? 0xFFA89880 : 0xFFFFFFFF, tin ? L_TIN_CEILING : L_PLASTER);
    float ch = Mp->ceil;
    mb_quad_world(&mb, v3(X, ch, Z), v3(X + 1, ch, Z), v3(X + 1, ch, Z + 1), v3(X, ch, Z + 1));
    if (!tin) {
      mb_paint(0xFF3A2414, L_WOOD);
      if (x % 2 == 0) mb_box(&mb, v3(X - 0.05f, ch - 0.1f, Z), v3(X + 0.05f, ch, Z + 1));
    }
  }
}

static void prop_tile(int x, int z, int c) {
  float X = x + 0.5f, Z = z + 0.5f;
  float yaw = 0;
  uint32_t h = H32(x, z, 41);
  switch (c) {
    case 'X': model_add(&mb, h % 3 ? MD_CRATE : MD_CRATES, X, 0, Z, (h % 7) * 0.2f - 0.6f, 1.0f, 0); break;
    case 'k': model_add(&mb, MD_BARREL, X, 0, Z, (h % 9) * 0.7f, 1.0f, 0); if (h % 2) model_add(&mb, MD_BARREL, X + 0.22f, 0, Z - 0.18f, 1.0f, 0.9f, 0); break;
    case 't': {
      model_add(&mb, MD_TABLE, X, 0, Z, 0, 1.0f, 0);
      static const int D[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
      for (int k = 0; k < 4; k++) {
        if (!isfloor(x + D[k][0], z + D[k][1])) continue;
        float cyaw = atan2f((float)-D[k][0], (float)-D[k][1]);
        model_add(&mb, MD_CHAIR, X + D[k][0] * 0.38f, 0, Z + D[k][1] * 0.38f, cyaw, 1.0f, 0);
      }
      break;
    }
    case 'h': face_dir_open(x, z, &yaw); model_add(&mb, MD_PEW, X, 0, Z, interior ? 3.14159f : yaw, 1.0f, 0); break;
    case 'e': face_dir_open(x, z, &yaw); model_add(&mb, MD_BENCH, X, 0.0f, Z, yaw + 3.14159f, 1.0f, 0); break;
    case 'b': {
      face_dir_open(x, z, &yaw);
      model_add(&mb, MD_BAR, X, 0, Z, yaw, 1.0f, 0);
      float fx = sinf(yaw), fz = cosf(yaw);
      if (isfloor((int)floorf(X + fx), (int)floorf(Z + fz)) && h % 2) model_add(&mb, MD_STOOL, X + fx * 0.55f, 0, Z + fz * 0.55f, 0, 1.0f, 0);
      break;
    }
    case 'S': face_dir_open(x, z, &yaw); model_add(&mb, MD_SHELF, X - sinf(yaw) * 0.3f, 0, Z - cosf(yaw) * 0.3f, yaw, 1.0f, 0); break;
    case 'u': face_dir_open(x, z, &yaw); model_add(&mb, MD_BOOKCASE, X - sinf(yaw) * 0.32f, 0, Z - cosf(yaw) * 0.32f, yaw, 1.0f, 0); break;
    case 'P': face_dir_open(x, z, &yaw); model_add(&mb, MD_PIANO, X - sinf(yaw) * 0.2f, 0, Z - cosf(yaw) * 0.2f, yaw, 1.0f, 0); break;
    case 'd': face_dir_open(x, z, &yaw); model_add(&mb, MD_DESK, X, 0, Z, yaw + 3.14159f, 1.0f, 0); break;
    case 'f':
      face_dir_open(x, z, &yaw);
      model_add(&mb, MD_FIREPLACE, X - sinf(yaw) * 0.3f, 0, Z - cosf(yaw) * 0.3f, yaw, 1.0f, 0);
      addlight(X + sinf(yaw) * 0.2f, 0.35f, Z + cosf(yaw) * 0.2f, 3.0f, 1.4f, 0.5f, 4.5f, 2);
      break;
    case 'B': face_dir_open(x, z, &yaw); model_add(&mb, MD_BED, X, 0, Z, yaw, 1.0f, 0); break;
    case 'A':
      model_add(&mb, MD_ALTAR, X, 0, Z, 0, 1.0f, 0);
      addlight(X, 0.8f, Z + 0.3f, 1.4f, 1.0f, 0.55f, 2.5f, 2);
      break;
    case 'V': model_add(&mb, MD_VAT, X, 0, Z, (h % 4) * 1.57f, 1.0f, 0); break;
    case 'm': model_add(&mb, MD_MACHINE, X, 0, Z, (h % 4) * 1.57f, 1.0f, 0); break;
    case 'Y': model_add(&mb, MD_RING, X, 0, Z, 0, 1.0f, 0); break;
    case 'F': case 'I': {
      int horiz = isfloor(x, z - 1) || isfloor(x, z + 1);
      model_add(&mb, c == 'F' ? MD_FENCE : MD_BARS, X, 0, Z, horiz ? 0 : 1.5708f, 1.0f, 0);
      break;
    }
  }
}

static void bb_tile(int x, int z, int c) {
  float X = x + 0.5f, Z = z + 0.5f;
  if (c == 'T') model_add(&mb, MD_TREE, X, 0.06f, Z, (x * 7 + z) * 0.4f, 1.0f, 0);
  else if (c == 'p') model_add(&mb, MD_PLANT, X, 0, Z, 0, 1.0f, 0);
  else if (c == 'L') {
    float y = interior ? 0 : 0.06f;
    y = ground_h(x, z);
    if (interior) { /* lampa stojąca */
      model_add(&mb, MD_PLANT, X, 0, Z, 0, 1.0f, 0);
      addlight(X, 1.2f, Z, 1.8f, 1.4f, 0.9f, 4, 0);
    } else {
      model_add(&mb, MD_LAMP, X, y, Z, 0, 1.0f, 0);
      addlight(X, y + 2.05f, Z, 3.4f, 2.5f, 1.45f, 7.5f, 3);
    }
  }
}

static uint32_t carColor(uint32_t h) {
  static const uint32_t C[8] = {0xFF22252C, 0xFF5A1C22, 0xFF2A4A34, 0xFF2C3A5A, 0xFF8A7A5A, 0xFF3A2E26, 0xFFB8AC90, 0xFF1A1C20};
  return C[h % 8];
}
static void cars(void) {
  for (int z = 0; z < Mh; z++)
    for (int x = 0; x < Mw; x++) {
      int c = tile(x, z);
      if (c == 'c') {
        /* orientacja wzdłuż ulicy: jeśli sąsiedzi w osi X są drogą — wzdłuż X */
        int alongX = isfloor(x - 1, z) || isfloor(x + 1, z) || tile(x - 1, z) == 'c' || tile(x + 1, z) == 'c';
        float yaw = alongX ? ((H32(x, z, 51) % 2) ? 0 : 3.14159f) : 1.5708f;
        model_add(&mb, MD_CAR, x + 0.5f, 0, z + 0.5f, yaw, 1.0f, carColor(H32(x, z, 52)));
      } else if (c == 'C' && tile(x - 1, z) != 'C') {
        int len = 0;
        while (tile(x + len, z) == 'C') len++;
        if (len >= 2) model_add(&mb, MD_TRUCK, x + len * 0.5f, 0, z + 0.5f, 0, 1.0f, 0xFF2A3A2A);
        else model_add(&mb, MD_CAR, x + 0.5f, 0, z + 0.5f, 0, 1.0f, carColor(H32(x, z, 53)));
      } else if (c == 'C' && tile(x, z - 1) == 'C') {
        /* pionowa ciężarówka obsłużona wyżej tylko w poziomie */
      }
    }
}

/* drobne elementy uliczne na chodnikach */
static void street_furniture(void) {
  if (interior) return;
  for (int z = 0; z < Mh; z++)
    for (int x = 0; x < Mw; x++) {
      if (tile(x, z) != ',') continue;
      uint32_t h = H32(x, z, 61);
      if (h % 100 > 9) continue;
      /* przy krawężniku (sąsiad to jezdnia) i nie przy drzwiach */
      int roadDir = -1;
      static const int D[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
      for (int k = 0; k < 4; k++) { int n = tile(x + D[k][0], z + D[k][1]); if (n == '.' || n == '-' || n == '|' || n == 'c') roadDir = k; }
      int nearDoor = 0;
      for (int k = 0; k < 4; k++) { int n = tile(x + D[k][0], z + D[k][1]); if (n == 'D' || n == 'G' || n == 'a') nearDoor = 1; }
      float X = x + 0.5f, Z = z + 0.5f;
      if (roadDir >= 0) {
        float ox = D[roadDir][0] * 0.32f, oz = D[roadDir][1] * 0.32f;
        int pick = h % 4;
        int md = pick == 0 ? MD_HYDRANT : pick == 1 ? MD_TRASH : pick == 2 ? MD_MAILBOX : MD_HYDRANT;
        model_add(&mb, md, X + ox, 0.06f, Z + oz, atan2f((float)D[roadDir][0], (float)D[roadDir][1]), 1.0f, 0);
      } else if (!nearDoor) {
        int pick = h % 3;
        float yaw;
        face_dir_open(x, z, &yaw);
        model_add(&mb, pick == 0 ? MD_NEWSSTAND : pick == 1 ? MD_TRASH : MD_BENCH, X, 0.06f, Z, yaw, 1.0f, 0);
      }
    }
}

/* panorama: pierścień budynków w oddali */
static void skyline(void) {
  if (interior) return;
  float cx = Mw * 0.5f, cz = Mh * 0.5f;
  float R = (Mw > Mh ? Mw : Mh) * 0.5f + 22;
  int n = 46;
  for (int i = 0; i < n; i++) {
    float a0 = i * 6.2831853f / n, a1 = (i + 1) * 6.2831853f / n;
    uint32_t h = H32(i, 0, 71);
    float rr = R + (h % 12);
    float ht = 6 + (h % 23) + ((h >> 8) % 3 == 0 ? 14 : 0);
    V3 p0 = v3(cx + cosf(a0) * rr, 0, cz + sinf(a0) * rr), p1 = v3(cx + cosf(a1) * rr, 0, cz + sinf(a1) * rr);
    mb_paint(0xFF9AA0B0, L_SKYLINE); P_.emis = Mp->night ? 0.35f : 0; P_.flags = VF_NOFOG;
    float w = sqrtf((p1.x - p0.x) * (p1.x - p0.x) + (p1.z - p0.z) * (p1.z - p0.z));
    /* ściana zwrócona do środka */
    mb_quad(&mb, v3(p1.x, -1, p1.z), v3(p0.x, -1, p0.z), v3(p0.x, ht, p0.z), v3(p1.x, ht, p1.z), 0, 0, w / 8.0f, (ht + 1) / 12.0f);
    if ((h >> 12) % 4 == 0) { /* iglica */
      float mx = (p0.x + p1.x) * 0.5f, mz = (p0.z + p1.z) * 0.5f;
      mb_paint(0xFF2A2C34, L_METAL); P_.flags = VF_NOFOG;
      mb_cyl(&mb, v3(mx, ht, mz), 0.8f, 0.05f, 6 + (h % 6), 6, 0);
      mb_paint(0xFFFF3020, L_WHITE); P_.emis = 1; P_.flags = VF_NOFOG;
      mb_sphere(&mb, v3(mx, ht + 6 + (h % 6), mz), 0.2f, 0.2f, 0.2f, 6);
    }
  }
}

/* ---------------------------------------------------------------- budowa */
void city_build(MapDef *m) {
  Mp = m; Mw = m->w; Mh = m->h;
  interior = m->interior;
  tdef_init();
  if (g_render && bigSlot.m == m) {
    cityMesh = bigSlot.far;
    memcpy(chunks, bigSlot.ch, sizeof(Chunk) * bigSlot.nch); nchunks = bigSlot.nch;
    memcpy(clights, bigSlot.l, sizeof(CLight) * bigSlot.nl); nclights = bigSlot.nl;
    curIsBig = 1;
    return;
  }
  nclights = 0;
  neonIdx = 0;
  if (!mb.v) mb_init(&mb);
  mb_reset(&mb);
  for (int z = 0; z < Mh; z++)
    for (int x = 0; x < Mw; x++) {
      int c = tile(x, z);
      const TDef *t = td_c(c);
      float X = (float)x, Z = (float)z;
      switch (t->kind) {
        case K_FLOOR: case K_BB: case K_CAR: case K_PROP:
          floor_tile(x, z, c);
          if (t->kind == K_PROP) prop_tile(x, z, c);
          if (t->kind == K_BB) bb_tile(x, z, c);
          break;
        case K_WATER: {
          mb_paint(0xFF6A8A98, L_WATER); P_.flags = VF_WATER; P_.uvs = 0.35f;
          mb_quad_world(&mb, v3(X, -0.3f, Z), v3(X, -0.3f, Z + 1), v3(X + 1, -0.3f, Z + 1), v3(X + 1, -0.3f, Z));
          for (int k = 0; k < 4; k++) {
            int nx = x + (k == 0) - (k == 1), nz = z + (k == 2) - (k == 3);
            if (td(nx, nz)->kind == K_WATER || nx < 0 || nz < 0 || nx >= Mw || nz >= Mh) continue;
            mb_paint(0xFF8A847A, L_STONE);
            if (k == 0) mb_quad_world(&mb, v3(X + 1, -0.6f, Z), v3(X + 1, -0.6f, Z + 1), v3(X + 1, 0, Z + 1), v3(X + 1, 0, Z));
            if (k == 1) mb_quad_world(&mb, v3(X, -0.6f, Z + 1), v3(X, -0.6f, Z), v3(X, 0, Z), v3(X, 0, Z + 1));
            if (k == 2) mb_quad_world(&mb, v3(X + 1, -0.6f, Z + 1), v3(X, -0.6f, Z + 1), v3(X, 0, Z + 1), v3(X + 1, 0, Z + 1));
            if (k == 3) mb_quad_world(&mb, v3(X, -0.6f, Z), v3(X + 1, -0.6f, Z), v3(X + 1, 0, Z), v3(X, 0, Z));
          }
          break;
        }
        case K_BLOCK: {
          float h = bheight(x, z);
          for (int k = 0; k < 4; k++) {
            int nx = x + (k == 0) - (k == 1), nz = z + (k == 2) - (k == 3);
            if (nx < 0 || nz < 0 || nx >= Mw || nz >= Mh) continue;
            const TDef *n = td(nx, nz);
            float nh = n->kind == K_BLOCK ? bheight(nx, nz) : 0;
            if (n->kind == K_BLOCK && (interior || nh >= h)) continue;
            Face f;
            /* krawędź z normalną na zewnątrz bloku; „wzdłuż” tak, by patrząc na ścianę iść w prawo */
            if (k == 0) f = (Face){X + 1, Z + 1, 0, -1, 1, 0};
            else if (k == 1) f = (Face){X, Z, 0, 1, -1, 0};
            else if (k == 2) f = (Face){X, Z + 1, 1, 0, 0, 1};
            else f = (Face){X + 1, Z, -1, 0, 0, -1};
            if (interior) interior_face(&f, c, x, z);
            else exterior_face(&f, c, x, z, h, n->kind == K_BLOCK ? nh : 0);
          }
          if (!interior) {
            /* dach */
            mb_paint(0xFFFFFFFF, L_ROOF);
            mb_quad_world(&mb, v3(X, h, Z), v3(X, h, Z + 1), v3(X + 1, h, Z + 1), v3(X + 1, h, Z));
            uint32_t hh = H32(x, z, 81);
            if (hh % 37 == 0 && isblock(x + 1, z) && isblock(x, z + 1)) model_add(&mb, MD_TANK, X + 1, h, Z + 1, 0, 1.0f, 0);
          }
          break;
        }
      }
    }
  /* schody przeciwpożarowe na wybranych fasadach */
  if (!interior)
    for (int z = 0; z < Mh; z++)
      for (int x = 0; x < Mw; x++) {
        int c = tile(x, z);
        if (c != 'R' && c != 'w') continue;
        uint32_t hh = H32(x, z, 91);
        if (hh % 9) continue;
        for (int k = 2; k < 4; k++) {
          int nx = x, nz = z + (k == 2 ? 1 : -1);
          if (!open_t(nx, nz) || !open_t(nx - 1, nz) || !open_t(nx + 1, nz)) continue;
          if (!isblock(x - 1, z) || !isblock(x + 1, z)) continue;
          Face f = k == 2 ? (Face){(float)x, (float)z + 1, 1, 0, 0, 1} : (Face){(float)x + 1, (float)z, -1, 0, 0, -1};
          fire_escape(&f, bheight(x, z));
        }
      }
  /* światła wnętrz: żyrandole w siatce */
  if (interior)
    for (int z = 1; z < Mh - 1; z++)
      for (int x = 1; x < Mw - 1; x++) {
        if (!isfloor(x, z) || x % 4 != 2 || z % 4 != 2) continue;
        float ch = Mp->ceil;
        int warehouse = Mp->bg[0] == 'm' || td_c(tile(0, 0))->wallL == L_CORRUGATED;
        model_add(&mb, warehouse ? MD_CEILLAMP : MD_CHANDELIER, x + 0.5f, ch, z + 0.5f, 0, 1.0f, 0);
        addlight(x + 0.5f, ch - 0.6f, z + 0.5f, 2.0f, 1.55f, 0.95f, 6.5f, 0);
      }
  /* reflektory nad sceną (podłoga 'y' przy ścianie) */
  if (interior)
    for (int z = 1; z < Mh - 1; z++)
      for (int x = 1; x < Mw - 1; x++) {
        if (tile(x, z) != 'y' || !isblock(x, z - 1) || x % 5 != 2) continue;
        int k = (x / 5) % 3;
        float r = k == 1 ? 0.9f : 2.4f, g = k == 1 ? 1.1f : 1.5f, b = k == 1 ? 2.6f : 0.7f;
        addlight(x + 0.5f, Mp->ceil - 0.25f, z + 1.6f, r, g, b, 4.5f, 0);
      }
  cars();
  street_furniture();
  skyline();
  if (g_render) {
    if (cityMesh.vao && !curIsBig) gm_free(&cityMesh);
    split_chunks();
    cityMesh = gm_upload(&mbFar);
    curIsBig = 0;
    if (Mw * Mh > 4000) {
      if (bigSlot.m && bigSlot.m != m) { gm_free(&bigSlot.far); for (int i = 0; i < bigSlot.nch; i++) gm_free(&bigSlot.ch[i].m); }
      bigSlot.m = m; bigSlot.far = cityMesh;
      memcpy(bigSlot.ch, chunks, sizeof(Chunk) * nchunks); bigSlot.nch = nchunks;
      memcpy(bigSlot.l, clights, sizeof(CLight) * nclights); bigSlot.nl = nclights;
      curIsBig = 1;
    }
    if (getenv("KX_STAT")) fprintf(stderr, "miasto: %d wierzchołków, %d kwartałów, %d świateł\n", mb.n, nchunks, nclights);
  }
  mb_paint(0xFFFFFFFF, L_WHITE);
}

/* ---------------------------------------------------------------- kwartały (rysowanie tylko w zasięgu wzroku) */
static void split_chunks(void) {
  if (!curIsBig) for (int i = 0; i < nchunks; i++) gm_free(&chunks[i].m);
  nchunks = 0;
  if (!mbFar.v) mb_init(&mbFar);
  mb_reset(&mbFar);
  int cw = (Mw + CHS - 1) / CHS, chh = (Mh + CHS - 1) / CHS;
  if (interior || cw * chh > MAXCH || cw * chh <= 1) { mb_append_tf(&mbFar, &mb, m4_identity()); return; }
  static MB parts[MAXCH];
  for (int i = 0; i < cw * chh; i++) { if (!parts[i].v) mb_init(&parts[i]); mb_reset(&parts[i]); }
  for (int t = 0; t + 2 < mb.n; t += 3) {
    Vert *v = &mb.v[t];
    float cx = (v[0].p[0] + v[1].p[0] + v[2].p[0]) / 3, cz = (v[0].p[2] + v[1].p[2] + v[2].p[2]) / 3;
    int ix = (int)floorf(cx / CHS), iz = (int)floorf(cz / CHS);
    MB *dst = (ix < 0 || iz < 0 || ix >= cw || iz >= chh) ? &mbFar : &parts[iz * cw + ix];
    Vert *o = mb_push(dst, 3);
    memcpy(o, v, sizeof(Vert) * 3);
  }
  for (int iz = 0; iz < chh; iz++)
    for (int ix = 0; ix < cw; ix++) {
      MB *p = &parts[iz * cw + ix];
      if (!p->n) continue;
      Chunk *c = &chunks[nchunks++];
      c->m = gm_upload(p);
      c->x0 = ix * CHS; c->z0 = iz * CHS; c->x1 = c->x0 + CHS; c->z1 = c->z0 + CHS;
    }
}

/* ---------------------------------------------------------------- rysowanie */
void city_lights(float time) {
  for (int i = 0; i < nclights; i++) {
    CLight *l = &clights[i];
    float k = 1;
    if (l->kind == 2) k = 0.8f + 0.2f * sinf(time * 11 + l->phase) * sinf(time * 7.3f + l->phase * 2);
    if (l->kind == 1) k = (fmodf(time * 0.37f + l->phase, 9.0f) < 0.12f) ? 0.2f : 1.0f; /* mrugający neon */
    r_light(l->x, l->y, l->z, l->r * k, l->g * k, l->b * k, l->rad);
  }
}
void city_glows(void) {
  for (int i = 0; i < nclights; i++) {
    CLight *l = &clights[i];
    if (l->kind == 3) r_glow(l->x, l->y, l->z, 0.5f, l->r * 0.35f, l->g * 0.35f, l->b * 0.35f);
    if (l->kind == 1) r_glow(l->x, l->y, l->z, 0.9f, l->r * 0.2f, l->g * 0.2f, l->b * 0.2f);
  }
}
void city_draw(void) {
  r_draw(&cityMesh, NULL, 0);
  V3 c = g_camPos;
  float fx = sinf(W.yaw), fz = cosf(W.yaw);
  for (int i = 0; i < nchunks; i++) {
    Chunk *k = &chunks[i];
    float nx = c.x < k->x0 ? k->x0 : c.x > k->x1 ? k->x1 : c.x;
    float nz = c.z < k->z0 ? k->z0 : c.z > k->z1 ? k->z1 : c.z;
    float dx = nx - c.x, dz = nz - c.z;
    if (dx * dx + dz * dz > 85.0f * 85.0f) continue;
    /* za plecami kamery */
    float mx = (k->x0 + k->x1) * 0.5f - c.x, mz = (k->z0 + k->z1) * 0.5f - c.z;
    if (mx * fx + mz * fz < -CHS * 0.75f) continue;
    r_draw(&k->m, NULL, 0);
  }
}
int city_verts(void) { return cityMesh.n; }
