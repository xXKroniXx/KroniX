/* Świat 3D: budowanie siatki z map ASCII, oświetlenie, ruch gracza (FPP), kolizje,
 * postacie jako billboardy, interakcja (E), drzwi, zdarzenia, rysowanie sceny i HUD. */
#include "engine.h"

World W;

/* ---------------------------------------------------------------- kafelki 3D */
enum { K_FLOOR, K_BLOCK, K_PROP, K_WATER, K_BB, K_CAR };
typedef struct { char c; int kind; int floorTex; int sideTex; int topTex; float h; int flags; } TDef;
static const TDef TD[] = {
  {'.', K_FLOOR, TX_ASPHALT}, {'-', K_FLOOR, TX_ROADLINE}, {'|', K_FLOOR, TX_ROADLINE_V}, {',', K_FLOOR, TX_SIDEWALK},
  {':', K_FLOOR, TX_COBBLE}, {'"', K_FLOOR, TX_GRASS}, {'_', K_FLOOR, TX_WOOD}, {'q', K_FLOOR, TX_CHECKER},
  {'r', K_FLOOR, TX_CARPET}, {'o', K_FLOOR, TX_CONCRETE}, {'=', K_FLOOR, TX_PLANKS}, {'y', K_FLOOR, TX_RING}, {'s', K_FLOOR, TX_STAIRS},
  {'R', K_BLOCK, TX_SIDEWALK, TX_BRICK_WIN}, {'W', K_BLOCK, TX_SIDEWALK, TX_BRICK}, {'w', K_BLOCK, TX_SIDEWALK, TX_BRICK_WIN},
  {'G', K_BLOCK, TX_SIDEWALK, TX_SHOPWIN}, {'a', K_BLOCK, TX_SIDEWALK, TX_AWNING}, {'D', K_BLOCK, TX_SIDEWALK, TX_DOOR},
  {'#', K_BLOCK, TX_CONCRETE, TX_STONE}, {'g', K_BLOCK, TX_CONCRETE, TX_STAINED}, {'i', K_BLOCK, TX_WOOD, TX_WALLPAPER},
  {'j', K_BLOCK, TX_WOOD, TX_WALLPAPER_RED}, {'M', K_BLOCK, TX_CONCRETE, TX_WAREHOUSE}, {'Q', K_BLOCK, TX_CONCRETE, TX_PLASTER},
  {'x', K_BLOCK, TX_CONCRETE, TX_BLACK}, {'z', K_BLOCK, TX_WOOD, TX_CURTAIN}, {'n', K_BLOCK, TX_SIDEWALK, TX_POSTER},
  {'X', K_PROP, TX_CONCRETE, TX_CRATE, TX_CRATE, 0.55f}, {'k', K_PROP, TX_CONCRETE, TX_BARREL, TX_BARREL, 0.55f},
  {'t', K_PROP, TX_WOOD, TX_TABLE_SIDE, TX_TABLECLOTH, 0.42f}, {'h', K_PROP, TX_WOOD, TX_PEW, TX_PEW, 0.28f},
  {'b', K_PROP, TX_WOOD, TX_BAR_FRONT, TX_BAR_TOP, 0.6f}, {'S', K_PROP, TX_WOOD, TX_SHELF_BOTTLES, TX_BAR_TOP, 1.4f},
  {'P', K_PROP, TX_WOOD, TX_PIANO, TX_PIANO, 0.6f}, {'V', K_PROP, TX_CONCRETE, TX_VAT, TX_METAL, 1.3f},
  {'d', K_PROP, TX_WOOD, TX_BAR_FRONT, TX_DESK_TOP, 0.45f}, {'e', K_PROP, TX_WOOD, TX_PEW, TX_PEW, 0.35f},
  {'B', K_PROP, TX_WOOD, TX_PEW, TX_BED, 0.3f}, {'u', K_PROP, TX_WOOD, TX_BOOKS, TX_BAR_TOP, 1.4f},
  {'f', K_PROP, TX_WOOD, TX_FIRE, TX_STONE, 1.0f}, {'A', K_PROP, TX_CONCRETE, TX_ALTAR, TX_ALTAR, 0.55f},
  {'Y', K_PROP, TX_RING, TX_POLE, TX_POLE, 0.8f}, {'F', K_PROP, TX_SIDEWALK, TX_FENCE, TX_FENCE, 0.6f, RF_ALPHA},
  {'I', K_PROP, TX_CONCRETE, TX_BARS, TX_BARS, 1.6f, RF_ALPHA}, {'m', K_PROP, TX_CONCRETE, TX_METAL, TX_METAL, 0.9f},
  {'~', K_WATER, TX_WATER}, {'T', K_BB, TX_GRASS}, {'p', K_BB, TX_WOOD}, {'L', K_BB, TX_SIDEWALK},
  {'c', K_CAR, TX_ASPHALT}, {'C', K_CAR, TX_ASPHALT},
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
int tile_solid(int c) {
  tdef_init();
  if (c <= 0 || c >= 128 || !tdef[c]) return 1;
  return tdef[c]->kind != K_FLOOR;
}

static MapDef *M(void) { return &S.maps[W.map]; }
static int tile_at(int x, int z) {
  MapDef *m = M();
  if (x < 0 || z < 0 || x >= m->w || z >= m->h) return 'x';
  return m->tiles[z][x];
}
static const TDef *td_at(int x, int z) { tdef_init(); int c = tile_at(x, z); return tdef[c] ? tdef[c] : tdef['x']; }

int world_solid_at(float x, float z) {
  return tile_solid(tile_at((int)floorf(x), (int)floorf(z)));
}

/* linia widzenia po siatce (tylko bloki i wysokie rekwizyty zasłaniają) */
static int blocks_sight(int c) {
  const TDef *t = tdef[c];
  if (!t) return 1;
  if (t->kind == K_BLOCK) return 1;
  if (t->kind == K_PROP && t->h > 1.0f && !(t->flags & RF_ALPHA)) return 1;
  return 0;
}
int world_los(float x0, float z0, float x1, float z1) {
  float dx = x1 - x0, dz = z1 - z0;
  float d = sqrtf(dx * dx + dz * dz);
  int steps = (int)(d * 6) + 1;
  for (int i = 1; i < steps; i++) {
    float t = (float)i / steps;
    if (blocks_sight(tile_at((int)floorf(x0 + dx * t), (int)floorf(z0 + dz * t)))) return 0;
  }
  return 1;
}

/* ---------------------------------------------------------------- siatka */
typedef struct { Vtx v[4]; short tex, flags; float cx, cy, cz, rad, nx, nz; } Quad;
static Quad *quads;
static int nquads, capquads;
typedef struct { float x, y, z, r, i; } Light;
static Light lights[256];
static int nlights;
static float ambient;
typedef struct { float x, z, w, h; int kind; } Deco;
static Deco decos[512];
static int ndecos;

static float light_at(float x, float y, float z, float face) {
  float l = ambient;
  for (int i = 0; i < nlights; i++) {
    float dx = x - lights[i].x, dy = y - lights[i].y, dz = z - lights[i].z;
    float d2 = dx * dx + dy * dy + dz * dz;
    float r2 = lights[i].r * lights[i].r;
    if (d2 >= r2) continue;
    float f = 1.0f - sqrtf(d2) / lights[i].r;
    l += f * f * lights[i].i;
  }
  l *= face;
  return l > 1.5f ? 1.5f : l;
}

static void add_quad(float x0, float y0, float z0, float x1, float y1, float z1,
                     float x2, float y2, float z2, float x3, float y3, float z3,
                     float us, float vs, int tex, int flags, float face, float nx, float nz) {
  if (nquads >= capquads) {
    capquads = capquads ? capquads * 2 : 4096;
    quads = (Quad *)realloc(quads, sizeof(Quad) * capquads);
  }
  Quad *q = &quads[nquads++];
  float P[4][3] = {{x0, y0, z0}, {x1, y1, z1}, {x2, y2, z2}, {x3, y3, z3}};
  float U[4] = {0, us, us, 0}, V[4] = {vs, vs, 0, 0};
  q->cx = q->cy = q->cz = 0;
  for (int i = 0; i < 4; i++) {
    q->v[i].x = P[i][0]; q->v[i].y = P[i][1]; q->v[i].z = P[i][2];
    q->v[i].u = U[i]; q->v[i].v = V[i];
    q->v[i].l = light_at(P[i][0], P[i][1] + 0.2f, P[i][2], face);
    q->cx += P[i][0] * 0.25f; q->cy += P[i][1] * 0.25f; q->cz += P[i][2] * 0.25f;
  }
  float r = 0;
  for (int i = 0; i < 4; i++) {
    float dx = P[i][0] - q->cx, dy = P[i][1] - q->cy, dz = P[i][2] - q->cz;
    float d = sqrtf(dx * dx + dy * dy + dz * dz);
    if (d > r) r = d;
  }
  q->rad = r;
  q->tex = (short)tex; q->flags = (short)flags;
  q->nx = nx; q->nz = nz;
}

static float bheight(int x, int z) {
  MapDef *m = M();
  if (m->interior) return m->ceil;
  u32 h = (u32)(x / 5) * 2654435761u ^ (u32)(z / 7) * 40503u ^ (u32)W.map * 97u;
  return (m->floors + (int)(h % 3)) * 1.5f;
}

/* ściana pionowa między (x0,z0) a (x1,z1), normalna (nx,nz), segmenty po piętrach */
static void wall(float x0, float z0, float x1, float z1, float yb, float yt, int tex, int ground, float nx, float nz, int flags, int tx, int tz) {
  float face = nz > 0.5f ? 1.0f : nz < -0.5f ? 0.72f : nx > 0 ? 0.88f : 0.8f;
  MapDef *m = M();
  if (!ground || m->interior) { /* rekwizyty i wnętrza: tekstura dopasowana do wysokości */
    add_quad(x0, yb, z0, x1, yb, z1, x1, yt, z1, x0, yt, z0, 1, 1, tex, flags, face, nx, nz);
    return;
  }
  for (float y = yb; y < yt - 0.01f; y += 1.5f) {
    float y2 = y + 1.5f > yt ? yt : y + 1.5f;
    int t = tex;
    if (y > 0.1f) {
      /* wyższe piętra: okna, część zapalona */
      u32 hs = (u32)(tx * 73856093) ^ (u32)(tz * 19349663) ^ (u32)(y * 100);
      if (tex == TX_STONE || tex == TX_STAINED) t = TX_STONE_WIN;
      else if (tex == TX_WAREHOUSE) t = TX_WAREHOUSE;
      else t = (hs % 5 < 2 && m->night) ? TX_BRICK_WIN_LIT : TX_BRICK_WIN;
    } else if (ground && tex == TX_BRICK_WIN) {
      u32 hs = (u32)(tx * 2654435761u) ^ (u32)tz;
      if (m->night && hs % 3 == 0) t = TX_BRICK_WIN_LIT;
    }
    add_quad(x0, y, z0, x1, y, z1, x1, y2, z1, x0, y2, z0, 1, (y2 - y) / 1.5f, t, flags, face, nx, nz);
  }
}

static void box(float x0, float z0, float x1, float z1, float h, int side, int top, int flags, int tx, int tz) {
  wall(x0, z1, x1, z1, 0, h, side, 0, 0, 1, flags, tx, tz);  /* południe */
  wall(x1, z0, x0, z0, 0, h, side, 0, 0, -1, flags, tx, tz); /* północ */
  wall(x1, z1, x1, z0, 0, h, side, 0, 1, 0, flags, tx, tz);  /* wschód */
  wall(x0, z0, x0, z1, 0, h, side, 0, -1, 0, flags, tx, tz); /* zachód */
  add_quad(x0, h, z1, x1, h, z1, x1, h, z0, x0, h, z0, 1, 1, top, flags, 1.1f, 0, 0);
}

static void build_mesh(void) {
  MapDef *m = M();
  tdef_init();
  nquads = 0; nlights = 0; ndecos = 0;
  ambient = m->interior ? (m->night ? 0.55f : 0.8f) : (m->night ? 0.28f + (255 - m->night) / 600.0f : 0.95f);
  /* światła */
  for (int z = 0; z < m->h; z++)
    for (int x = 0; x < m->w; x++) {
      int c = m->tiles[z][x];
      if (nlights >= 256) break;
      if (c == 'L') { lights[nlights++] = (Light){x + 0.5f, 2.0f, z + 0.5f, 5.5f, 1.1f}; }
      else if (c == 'f') { lights[nlights++] = (Light){x + 0.5f, 0.6f, z + 0.5f, 4.0f, 0.9f}; }
      else if (m->interior && (x % 4 == 2) && (z % 4 == 2) && !tile_solid(c)) { lights[nlights++] = (Light){x + 0.5f, m->ceil - 0.2f, z + 0.5f, 4.5f, 0.55f}; }
      else if (c == 'G' && m->night) { lights[nlights++] = (Light){x + 0.5f, 0.8f, z + 1.2f, 2.5f, 0.5f}; }
    }
  for (int z = 0; z < m->h; z++)
    for (int x = 0; x < m->w; x++) {
      const TDef *t = td_at(x, z);
      float X = (float)x, Z = (float)z;
      int ceilT = m->bg[0] == 'w' ? TX_CEIL_WOOD : TX_CEIL_PLASTER;
      switch (t->kind) {
        case K_FLOOR: case K_BB: case K_CAR:
          add_quad(X, 0, Z + 1, X + 1, 0, Z + 1, X + 1, 0, Z, X, 0, Z, 1, 1, t->kind == K_CAR ? TX_ASPHALT : t->floorTex, 0, 1.0f, 0, 0);
          if (m->interior) add_quad(X, m->ceil, Z, X + 1, m->ceil, Z, X + 1, m->ceil, Z + 1, X, m->ceil, Z + 1, 1, 1, ceilT, 0, 0.9f, 0, 0);
          if (t->kind == K_BB && ndecos < 512) {
            int kind = t->c == 'T' ? BB_TREE : t->c == 'p' ? BB_PLANT : BB_LAMP;
            float w = kind == BB_TREE ? 1.6f : kind == BB_PLANT ? 0.45f : 0.22f;
            float h = kind == BB_TREE ? 2.6f : kind == BB_PLANT ? 0.7f : 2.2f;
            decos[ndecos++] = (Deco){X + 0.5f, Z + 0.5f, w, h, kind};
          }
          if (t->kind == K_CAR) {
            int left = t->c == 'c';
            float x0 = left ? X + 0.08f : X, x1 = left ? X + 1 : X + 0.92f;
            float z0 = Z + 0.12f, z1 = Z + 0.88f;
            wall(x0, z1, x1, z1, 0.08f, 0.52f, TX_CAR_SIDE, 0, 0, 1, 0, x, z);
            wall(x1, z0, x0, z0, 0.08f, 0.52f, TX_CAR_SIDE, 0, 0, -1, 0, x, z);
            if (left) wall(x0, z0, x0, z1, 0.08f, 0.52f, TX_CAR_BACK, 0, -1, 0, 0, x, z);
            else wall(x1, z1, x1, z0, 0.08f, 0.52f, TX_CAR_FRONT, 0, 1, 0, 0, x, z);
            add_quad(x0, 0.52f, z1, x1, 0.52f, z1, x1, 0.52f, z0, x0, 0.52f, z0, 1, 1, TX_CAR_TOP, 0, 1.0f, 0, 0);
            if (left) { /* kabina */
              float cx0 = X + 0.45f, cx1 = X + 1.0f;
              wall(cx0, z1 - 0.06f, cx1, z1 - 0.06f, 0.52f, 0.82f, TX_CAR_SIDE, 0, 0, 1, 0, x, z);
              wall(cx1, z0 + 0.06f, cx0, z0 + 0.06f, 0.52f, 0.82f, TX_CAR_SIDE, 0, 0, -1, 0, x, z);
              wall(cx0, z0 + 0.06f, cx0, z1 - 0.06f, 0.52f, 0.82f, TX_CAR_BACK, 0, -1, 0, 0, x, z);
              add_quad(cx0, 0.82f, z1 - 0.06f, cx1 + 0.4f, 0.82f, z1 - 0.06f, cx1 + 0.4f, 0.82f, z0 + 0.06f, cx0, 0.82f, z0 + 0.06f, 1, 1, TX_CAR_TOP, 0, 1.0f, 0, 0);
            } else {
              float cx1 = X + 0.4f;
              wall(X, z1 - 0.06f, cx1, z1 - 0.06f, 0.52f, 0.82f, TX_CAR_SIDE, 0, 0, 1, 0, x, z);
              wall(cx1, z0 + 0.06f, X, z0 + 0.06f, 0.52f, 0.82f, TX_CAR_SIDE, 0, 0, -1, 0, x, z);
              wall(cx1, z1 - 0.06f, cx1, z0 + 0.06f, 0.52f, 0.82f, TX_CAR_FRONT, 0, 1, 0, 0, x, z);
            }
          }
          break;
        case K_WATER:
          add_quad(X, -0.25f, Z + 1, X + 1, -0.25f, Z + 1, X + 1, -0.25f, Z, X, -0.25f, Z, 1, 1, TX_WATER, 0, 1.0f, 0, 0);
          for (int d = 0; d < 4; d++) {
            int nx = x + (d == 0) - (d == 1), nz = z + (d == 2) - (d == 3);
            if (td_at(nx, nz)->kind == K_WATER) continue;
            if (d == 0) wall(X + 1, Z, X + 1, Z + 1, -0.25f, 0, TX_CONCRETE, 0, -1, 0, 0, x, z);
            if (d == 1) wall(X, Z + 1, X, Z, -0.25f, 0, TX_CONCRETE, 0, 1, 0, 0, x, z);
            if (d == 2) wall(X + 1, Z + 1, X, Z + 1, -0.25f, 0, TX_CONCRETE, 0, 0, -1, 0, x, z);
            if (d == 3) wall(X, Z, X + 1, Z, -0.25f, 0, TX_CONCRETE, 0, 0, 1, 0, x, z);
          }
          break;
        case K_PROP: {
          add_quad(X, 0, Z + 1, X + 1, 0, Z + 1, X + 1, 0, Z, X, 0, Z, 1, 1, t->floorTex, 0, 1.0f, 0, 0);
          if (m->interior) add_quad(X, m->ceil, Z, X + 1, m->ceil, Z, X + 1, m->ceil, Z + 1, X, m->ceil, Z + 1, 1, 1, ceilT, 0, 0.9f, 0, 0);
          float in = (t->c == 'k' || t->c == 't' || t->c == 'h' || t->c == 'd') ? 0.12f : (t->c == 'Y') ? 0.42f : 0.02f;
          if (t->c == 'F' || t->c == 'I') {
            /* cienki płot/kraty: płaszczyzna przez środek, dwustronna */
            int horiz = !tile_solid(tile_at(x, z - 1)) || !tile_solid(tile_at(x, z + 1));
            if (horiz) { wall(X, Z + 0.5f, X + 1, Z + 0.5f, 0, t->h, t->sideTex, 0, 0, 1, RF_ALPHA, x, z); wall(X + 1, Z + 0.5f, X, Z + 0.5f, 0, t->h, t->sideTex, 0, 0, -1, RF_ALPHA, x, z); }
            else { wall(X + 0.5f, Z, X + 0.5f, Z + 1, 0, t->h, t->sideTex, 0, -1, 0, RF_ALPHA, x, z); wall(X + 0.5f, Z + 1, X + 0.5f, Z, 0, t->h, t->sideTex, 0, 1, 0, RF_ALPHA, x, z); }
          } else box(X + in, Z + in, X + 1 - in, Z + 1 - in, t->h, t->sideTex, t->topTex, t->flags, x, z);
          break;
        }
        case K_BLOCK: {
          float h = bheight(x, z);
          for (int d = 0; d < 4; d++) {
            int nx = x + (d == 0) - (d == 1), nz = z + (d == 2) - (d == 3);
            const TDef *n = td_at(nx, nz);
            float nh = n->kind == K_BLOCK ? bheight(nx, nz) : 0;
            if (n->kind == K_BLOCK && nh >= h) continue;
            if (nx < 0 || nz < 0 || nx >= m->w || nz >= m->h) continue;
            int tex = t->sideTex;
            int ground = 1;
            /* boczne ściany "masy" budynku i wyższe partie: cegła z oknami */
            if (t->c == 'R') tex = TX_BRICK_WIN;
            if (t->c == 'n' && d != 2) tex = TX_BRICK;
            float yb = n->kind == K_BLOCK ? nh : 0;
            if (d == 0) wall(X + 1, Z + 1, X + 1, Z, yb, h, tex, ground, 1, 0, 0, x, z);
            if (d == 1) wall(X, Z, X, Z + 1, yb, h, tex, ground, -1, 0, 0, x, z);
            if (d == 2) wall(X, Z + 1, X + 1, Z + 1, yb, h, tex, ground, 0, 1, 0, x, z);
            if (d == 3) wall(X + 1, Z, X, Z, yb, h, tex, ground, 0, -1, 0, x, z);
          }
          break;
        }
      }
    }
}

/* ---------------------------------------------------------------- encje */
static int ent_cond(EntDef *d) {
  for (int k = 0; k < 2; k++)
    if (d->condVar[k] >= 0) {
      int v = H.vars[d->condVar[k]] != 0;
      if (d->condNeg[k] ? v : !v) return 0;
    }
  return 1;
}
static int event_var(EntDef *d) {
  char name[48];
  snprintf(name, sizeof name, "_ev_%s_%d_%d", M()->id, d->x, d->y);
  return var_find(name, 1);
}

void world_refresh_visibility(void) {
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->d) continue;
    int prev = e->vis;
    e->vis = ent_cond(e->d) && (!e->dead || e->d->type == ENT_MOB);
    if (e->d->type == ENT_EVENT && e->d->once) {
      int v = event_var(e->d);
      if (v >= 0 && H.vars[v]) e->vis = 0;
    }
    if (e->d->type == ENT_MOB && e->vis && !prev && !e->hostile) {
      int ed = enemy_find(e->d->enemies);
      e->enemyDef = ed;
      e->hostile = !e->d->ally;
      e->ally = e->d->ally;
      e->hp = e->maxhp = ed >= 0 ? S.enemies[ed].hp : 30;
      e->state = 0;
    }
  }
}

Ent *world_find_ent(const char *id) {
  for (int i = 0; i < W.n; i++)
    if (W.ents[i].d && !strcmp(W.ents[i].d->id, id)) return &W.ents[i];
  return NULL;
}

static float dir_angle(int dir) {
  return dir == DIR_DOWN ? 0.0f : dir == DIR_UP ? PI_F : dir == DIR_RIGHT ? PI_F * 0.5f : -PI_F * 0.5f;
}

static EntDef spawnDefs[32];
static int nspawn;

void world_load_map(int map, int tx, int ty, int dir) {
  if (map < 0 || map >= S.nmaps) return;
  W.map = map;
  W.n = 0;
  nspawn = 0;
  MapDef *m = M();
  build_mesh();
  for (int i = 0; i < m->nents && W.n < MAX_ENTS; i++) {
    Ent *e = &W.ents[W.n++];
    memset(e, 0, sizeof *e);
    e->d = &m->ents[i];
    e->x = e->homeX = e->d->x + 0.5f;
    e->z = e->homeZ = e->d->y + 0.5f;
    e->ang = dir_angle(e->d->dir);
    e->walkT = 60 + rand() % 120;
    e->enemyDef = -1;
  }
  W.px = tx + 0.5f; W.pz = ty + 0.5f;
  W.yaw = dir_angle(dir); W.pitch = 0;
  W.grace = 30;
  W.bannerT = 170;
  if (m->music[0]) music_play(m->music);
  world_refresh_visibility();
}

void world_spawn(const char *enemyId, int tx, int ty, int ally) {
  if (nspawn >= 32 || W.n >= MAX_ENTS) return;
  int ed = enemy_find(enemyId);
  EntDef *d = &spawnDefs[nspawn++];
  memset(d, 0, sizeof *d);
  d->type = ENT_MOB;
  snprintf(d->id, sizeof d->id, "spawn%d", nspawn);
  snprintf(d->sprite, sizeof d->sprite, "%s", ed >= 0 ? S.enemies[ed].sprite : "zbir");
  snprintf(d->enemies, sizeof d->enemies, "%s", enemyId);
  d->x = tx; d->y = ty;
  d->condVar[0] = d->condVar[1] = -1;
  d->script = -1;
  d->ally = ally;
  Ent *e = &W.ents[W.n++];
  memset(e, 0, sizeof *e);
  e->d = d;
  e->x = e->homeX = tx + 0.5f;
  e->z = e->homeZ = ty + 0.5f;
  e->ang = atan2f(W.px - e->x, W.pz - e->z);
  e->vis = 1;
  e->enemyDef = ed;
  e->hostile = !ally;
  e->ally = ally;
  e->hp = e->maxhp = ed >= 0 ? S.enemies[ed].hp : 30;
  e->state = 1; /* od razu czujni */
}

int world_enemies_alive(void) {
  int n = 0;
  for (int i = 0; i < W.n; i++) if (W.ents[i].vis && W.ents[i].hostile && !W.ents[i].dead) n++;
  return n;
}

void world_kill_all(void) {
  for (int i = 0; i < W.n; i++)
    if (W.ents[i].hostile) { W.ents[i].dead = 1; W.ents[i].vis = 0; }
}

void world_hurt_player(int dmg) {
  if (W.grace > 0 || g_autoplay > 1) return;
  if (H.armor > 0) {
    int a = dmg / 2 < H.armor ? dmg / 2 : H.armor;
    H.armor -= a;
    dmg -= a;
  }
  H.hp -= dmg;
  W.hurtT = 18;
  sfx_play(SFX_HURT);
  if (H.hp <= 0) {
    H.hp = 0;
    if (tycoon_mission_active()) { tycoon_mission_fail(); return; }
    script_stop();
    game_set_mode(MODE_GAMEOVER);
  }
}

/* ---------------------------------------------------------------- ruch */
#define PR 0.22f
static int blocked_circle(float x, float z, float r) {
  return world_solid_at(x - r, z - r) || world_solid_at(x + r, z - r) || world_solid_at(x - r, z + r) || world_solid_at(x + r, z + r);
}

void move_circle(float *x, float *z, float dx, float dz, float r, Ent *self) {
  (void)0;
  float nx = *x + dx;
  if (!blocked_circle(nx, *z, r)) *x = nx;
  float nz = *z + dz;
  if (!blocked_circle(*x, nz, r)) *z = nz;
  /* odpychanie od postaci */
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (e == self || !e->vis || e->dead) continue;
    if (e->d->type != ENT_NPC && e->d->type != ENT_MOB) continue;
    float ex = *x - e->x, ez = *z - e->z;
    float d = sqrtf(ex * ex + ez * ez);
    float min = r + 0.22f;
    if (d < min && d > 0.001f) {
      float px = *x + ex / d * (min - d), pz = *z + ez / d * (min - d);
      if (!blocked_circle(px, pz, r)) { *x = px; *z = pz; }
    }
  }
}

static Ent *look_target(float *outDist) {
  Ent *best = NULL;
  float bd = 1.7f;
  float fx = sinf(W.yaw), fz = cosf(W.yaw);
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis || e->dead || e->d->script < 0) continue;
    if (e->d->type != ENT_NPC && e->d->type != ENT_OBJ) continue;
    if (e->hostile) continue;
    float dx = e->x - W.px, dz = e->z - W.pz;
    float d = sqrtf(dx * dx + dz * dz);
    if (d > bd || d < 0.01f) continue;
    float dot = (dx * fx + dz * fz) / d;
    if (dot < 0.86f) continue;
    if (!world_los(W.px, W.pz, e->x, e->z)) continue;
    best = e; bd = d;
  }
  if (outDist) *outDist = bd;
  return best;
}

static Ent *look_door(void) {
  float fx = sinf(W.yaw), fz = cosf(W.yaw);
  for (float t = 0.3f; t < 1.3f; t += 0.1f) {
    int tx = (int)floorf(W.px + fx * t), tz = (int)floorf(W.pz + fz * t);
    if (!tile_solid(tile_at(tx, tz))) continue;
    for (int i = 0; i < W.n; i++) {
      Ent *e = &W.ents[i];
      if (e->vis && e->d->type == ENT_WARP && e->d->x == tx && e->d->y == tz) return e;
    }
    return NULL;
  }
  return NULL;
}

static void start_warp(EntDef *d) {
  W.trans = 1;
  W.tMap = d->toMap; W.tX = d->toX; W.tY = d->toY; W.tDir = d->toDir;
  sfx_play(SFX_DOOR);
}

static int lastTileX = -1, lastTileZ = -1;
static void check_tile_events(void) {
  int tx = (int)floorf(W.px), tz = (int)floorf(W.pz);
  if (tx == lastTileX && tz == lastTileZ) return;
  lastTileX = tx; lastTileZ = tz;
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis || e->d->x != tx || e->d->y != tz) continue;
    if (e->d->type == ENT_WARP && e->d->toMap >= 0 && !tile_solid(tile_at(tx, tz))) { start_warp(e->d); return; }
    if (e->d->type == ENT_EVENT && e->d->script >= 0) {
      if (e->d->once) { int v = event_var(e->d); if (v >= 0) H.vars[v] = 1; }
      script_start(e->d->script, NULL);
      return;
    }
  }
}

static void update_player(void) {
  float spd = (in.held[BTN_RUN] ? 2.9f : 1.7f) / 60.0f;
  float fx = sinf(W.yaw), fz = cosf(W.yaw);
  float rx = -cosf(W.yaw), rz = sinf(W.yaw);
  float mx = 0, mz = 0;
  if (in.held[BTN_UP]) { mx += fx; mz += fz; }
  if (in.held[BTN_DOWN]) { mx -= fx; mz -= fz; }
  if (in.held[BTN_SR]) { mx += rx; mz += rz; }
  if (in.held[BTN_SL]) { mx -= rx; mz -= rz; }
  float ml = sqrtf(mx * mx + mz * mz);
  if (ml > 0.01f) {
    move_circle(&W.px, &W.pz, mx / ml * spd, mz / ml * spd, PR, NULL);
    W.bobT += in.held[BTN_RUN] ? 0.2f : 0.13f;
    if ((int)(W.bobT / PI_F) != (int)((W.bobT - 0.13f) / PI_F)) sfx_play(SFX_STEP);
  }
  W.bob = ml > 0.01f ? sinf(W.bobT * 2) * 0.012f : W.bob * 0.8f;
  float turn = 2.4f / 60.0f;
  if (in.held[BTN_TL]) W.yaw += turn;
  if (in.held[BTN_TR]) W.yaw -= turn;
  W.yaw -= in.mdx * 0.0032f;
  W.pitch -= in.mdy * 0.0032f;
  W.pitch = CLAMP(W.pitch, -0.9f, 0.9f);
  check_tile_events();
  if (W.trans || script_running()) return;
  if (in.pressed[BTN_A]) {
    Ent *t = look_target(NULL);
    if (t) {
      if (t->d->type == ENT_NPC) t->ang = atan2f(W.px - t->x, W.pz - t->z);
      script_start(t->d->script, t);
      return;
    }
    Ent *door = look_door();
    if (door && door->d->toMap >= 0) { start_warp(door->d); return; }
  }
  if (in.pressed[BTN_B]) { ui_open_pause(); return; }
  if (in.pressed[BTN_TAB] && tycoon_active() && !tycoon_mission_active()) { tycoon_open(); return; }
}

static void update_npcs(void) {
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis) continue;
    if (e->d->type == ENT_MOB) { combat_ent_update(e); continue; }
    if (e->d->type != ENT_NPC) continue;
    float dx = W.px - e->x, dz = W.pz - e->z;
    float d = sqrtf(dx * dx + dz * dz);
    if (d < 2.2f) { /* patrzy na gracza */
      float target = atan2f(dx, dz);
      float da = target - e->ang;
      while (da > PI_F) da -= 2 * PI_F;
      while (da < -PI_F) da += 2 * PI_F;
      e->ang += da * 0.12f;
      e->moving = 0;
      continue;
    }
    if (!e->d->wander) continue;
    if (e->moving) {
      float mx = e->tx - e->x, mz = e->tz - e->z;
      float ml = sqrtf(mx * mx + mz * mz);
      if (ml < 0.05f || --e->walkT <= 0) { e->moving = 0; e->walkT = 90 + rand() % 180; continue; }
      e->ang = atan2f(mx, mz);
      float ox = e->x, oz = e->z;
      move_circle(&e->x, &e->z, mx / ml * 0.012f, mz / ml * 0.012f, 0.2f, e);
      if (fabsf(ox - e->x) + fabsf(oz - e->z) < 0.001f) e->moving = 0;
    } else if (--e->walkT <= 0) {
      e->tx = e->homeX + (rand() % 400 - 200) / 100.0f;
      e->tz = e->homeZ + (rand() % 400 - 200) / 100.0f;
      e->moving = 1;
      e->walkT = 200;
    }
  }
}

void world_update(void) {
  W.time++;
  if (W.grace > 0) W.grace--;
  if (W.bannerT > 0) W.bannerT--;
  if (W.hurtT > 0) W.hurtT--;
  textures_animate(W.time);
  if (W.trans == 1) {
    g_fade += 24;
    if (g_fade >= 255) { g_fade = 255; world_load_map(W.tMap, W.tX, W.tY, W.tDir); W.trans = 2; lastTileX = W.tX; lastTileZ = W.tY; }
    return;
  }
  if (W.trans == 2) { g_fade -= 24; if (g_fade <= 0) { g_fade = 0; W.trans = 0; } }
  if (script_running()) script_update();
  if (g_mode != MODE_WORLD) return;
  if (!script_blocking()) {
    update_player();
    if (g_mode != MODE_WORLD) return;
    combat_update();
  }
  update_npcs();
  tycoon_world_tick();
}

/* ---------------------------------------------------------------- rysowanie */
static void draw_actor(Ent *e) {
  Actor *a = actor_get(e->d->sprite);
  float dx = e->x - W.px, dz = e->z - W.pz;
  float dist = sqrtf(dx * dx + dz * dz);
  if (dist > 26) return;
  float l = light_at(e->x, 0.6f, e->z, 1.0f);
  if (e->hitT > 0) l = 1.4f;
  if (!a) {
    Sprite *s = art_get(e->d->sprite);
    const TDef *t = td_at((int)floorf(e->x), (int)floorf(e->z));
    float y0 = t->kind == K_PROP ? t->h : 0.0f; /* przedmiot leżący na stole/biurku */
    if (s) r3d_billboard(e->x, y0, e->z, y0 > 0 ? 0.32f : 0.42f, y0 > 0 ? 0.32f : 0.42f, s, 0, l + 0.2f, 0);
    return;
  }
  if (e->dead) {
    Sprite *s = e->deadT > 12 ? a->dead[0] : a->dead[1];
    r3d_billboard(e->x, 0.0f, e->z, 0.45f, 0.9f, s, e->ang > 0, l, 0);
    return;
  }
  /* który widok: kąt między kierunkiem postaci a kierunkiem do kamery */
  float toCam = atan2f(-dx, -dz);
  float rel = toCam - e->ang;
  while (rel > PI_F) rel -= 2 * PI_F;
  while (rel < -PI_F) rel += 2 * PI_F;
  int view, flip = 0;
  if (fabsf(rel) < PI_F * 0.25f) view = VIEW_FRONT;
  else if (fabsf(rel) > PI_F * 0.75f) view = VIEW_BACK;
  else { view = VIEW_SIDE; flip = rel < 0; }
  int pose = POSE_STAND;
  if (e->hitT > 0) pose = POSE_HIT;
  else if (e->shootT > 0) pose = POSE_SHOOT;
  else if (e->state >= 2 && e->hostile) pose = e->moving ? ((W.time / 8) % 2 ? POSE_WALK1 : POSE_WALK2) : POSE_AIM;
  else if (e->ally && e->state >= 2) pose = e->moving ? ((W.time / 8) % 2 ? POSE_WALK1 : POSE_WALK2) : POSE_AIM;
  else if (e->moving) pose = (W.time / 10) % 2 ? POSE_WALK1 : POSE_WALK2;
  if (view == VIEW_BACK && (pose == POSE_AIM || pose == POSE_SHOOT)) pose = POSE_STAND;
  r3d_billboard(e->x, 0.0f, e->z, 0.45f, 0.9f, a->fr[view][pose], flip, l, 0);
}

void world_draw(void) {
  MapDef *m = M();
  Cam c = {W.px, 0.78f + W.bob, W.pz, W.yaw, W.pitch};
  r3d_begin(&c);
  u32 fogc = m->interior ? RGB(0x12, 0x0e, 0x10) : m->night ? RGB(0x16, 0x16, 0x26) : RGB(0x9a, 0xa4, 0xb0);
  float fe = m->interior ? 18.0f : m->night ? 22.0f : 40.0f;
  r3d_fog(fogc, m->interior ? 6.0f : 5.0f, fe);
  if (m->interior) gfx_clear(fogc);
  else r3d_sky(m->night > 0, W.time);
  float fx = sinf(W.yaw), fz = cosf(W.yaw);
  for (int i = 0; i < nquads; i++) {
    Quad *q = &quads[i];
    float dx = q->cx - W.px, dz = q->cz - W.pz;
    float d2 = dx * dx + dz * dz;
    float lim = fe + q->rad + 1;
    if (d2 > lim * lim) continue;
    if (dx * fx + dz * fz < -q->rad - 0.5f) continue; /* za plecami */
    if ((q->nx != 0 || q->nz != 0) && !(q->flags & RF_ALPHA)) {
      if ((W.px - q->cx) * q->nx + (W.pz - q->cz) * q->nz < 0) continue; /* tył ściany */
    }
    r3d_poly(q->v, 4, TEX[q->tex], q->flags);
  }
  for (int i = 0; i < ndecos; i++) {
    Deco *d = &decos[i];
    float dx = d->x - W.px, dz = d->z - W.pz;
    if (dx * dx + dz * dz > fe * fe) continue;
    r3d_billboard(d->x, 0.0f, d->z, d->w, d->h, BB[d->kind], 0, d->kind == BB_LAMP ? 1.0f : light_at(d->x, 1, d->z, 1), d->kind == BB_LAMP ? RF_FULLBRIGHT : 0);
  }
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis && !(e->dead && e->d->type == ENT_MOB)) continue;
    if (e->d->type == ENT_WARP || e->d->type == ENT_EVENT) continue;
    draw_actor(e);
  }
  if (m->rain) {
    for (int i = 0; i < 140; i++) {
      u32 h = (u32)(i * 2654435761u);
      int x = (int)((h % 520) + (W.time * 3)) % 520 - 20;
      int y = (int)(((h >> 9) % 300) + W.time * 9) % 300 - 15;
      for (int k = 0; k < 7; k++) gfx_pset_a(x - k / 3, y + k, RGB(0xa0, 0xb8, 0xd8), 90);
    }
  }
  if (!script_blocking()) { combat_draw_view(); combat_draw_hud(); }

  /* podpowiedź interakcji */
  if (!script_blocking() && !W.trans && g_mode == MODE_WORLD) {
    Ent *t = look_target(NULL);
    const char *msg = NULL;
    char b[96];
    if (t) {
      const char *nm = t->d->type == ENT_NPC ? "Rozmawiaj" : "Zbadaj";
      int sp = speaker_find(t->d->id);
      if (sp >= 0 && t->d->type == ENT_NPC) snprintf(b, sizeof b, "E  %s: %s", nm, S.speakers[sp].name);
      else snprintf(b, sizeof b, "E  %s", nm);
      msg = b;
    } else {
      Ent *door = look_door();
      if (door && door->d->toMap >= 0) {
        snprintf(b, sizeof b, "E  Wejdź: %s", S.maps[door->d->toMap].name);
        msg = b;
      }
    }
    if (msg) {
      int w = text_width(msg) + 12;
      gfx_rect_a((SCREEN_W - w) / 2, SCREEN_H / 2 + 18, w, 14, pal('i'), 170);
      text_draw_sh((SCREEN_W - w) / 2 + 6, SCREEN_H / 2 + 20, msg, pal('y'));
    }
  }
  if (W.bannerT > 0 && m->name[0]) {
    int a = W.bannerT > 140 ? (170 - W.bannerT) * 8 : W.bannerT < 30 ? W.bannerT * 8 : 240;
    int w = text_width(m->name) * 2 + 24, x = (SCREEN_W - w) / 2;
    gfx_rect_a(x, 30, w, 28, pal('i'), a > 200 ? 200 : a);
    gfx_rect_a(x, 30, w, 1, pal('z'), a);
    gfx_rect_a(x, 57, w, 1, pal('z'), a);
    if (a > 120) text_draw_big(x + 12, 33, m->name, pal('y'), 2);
  }
  if (W.hurtT > 0) {
    for (int y = 0; y < SCREEN_H; y++)
      for (int x = 0; x < SCREEN_W; x++) {
        int ex = abs(x - SCREEN_W / 2) * 256 / (SCREEN_W / 2), ey = abs(y - SCREEN_H / 2) * 256 / (SCREEN_H / 2);
        int e = (ex > ey ? ex : ey) - 120;
        if (e > 0) gfx_pset_a(x, y, pal('r'), e * W.hurtT / 18);
      }
  }
}

float world_light_at(float x, float z) { return light_at(x, 0.6f, z, 1.0f); }
