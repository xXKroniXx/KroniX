/* Świat 3D: logika mapy (kolizje, encje, ruch gracza FPP, interakcja, drzwi, zdarzenia)
 * oraz rysowanie sceny przez renderer GPU (geometria z city.c, postacie z human.c). */
#include "engine.h"

World W;

static MapDef *M(void) { return &S.maps[W.map]; }
static int tile_at(int x, int z) {
  MapDef *m = M();
  if (x < 0 || z < 0 || x >= m->w || z >= m->h) return 'x';
  return m->tiles[z][x];
}

int world_solid_at(float x, float z) { return tile_solid(tile_at((int)floorf(x), (int)floorf(z))); }

int world_los(float x0, float z0, float x1, float z1) {
  float dx = x1 - x0, dz = z1 - z0;
  float d = sqrtf(dx * dx + dz * dz);
  int steps = (int)(d * 6) + 1;
  for (int i = 1; i < steps; i++) {
    float t = (float)i / steps;
    if (tile_blocks_sight(tile_at((int)floorf(x0 + dx * t), (int)floorf(z0 + dz * t)))) return 0;
  }
  return 1;
}

/* ---------------------------------------------------------------- encje */
static int ent_cond(EntDef *d) {
  for (int k = 0; k < 3; k++)
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

/* ---------------------------------------------------------------- znacznik celu */
void world_marker_set(int map, int x, int y, const char *npcId) {
  int t = 0, mx = 0, my = 0, mm = -1;
  if (npcId) {
    /* NPC: najpierw widoczny wariant (spełnione warunki), potem dowolny */
    for (int pass = 0; pass < 2 && mm < 0; pass++)
      for (int i = 0; i < S.nmaps && mm < 0; i++)
        for (int k = 0; k < S.maps[i].nents; k++) {
          EntDef *d = &S.maps[i].ents[k];
          if (strcmp(d->id, npcId) || (pass == 0 && !ent_cond(d))) continue;
          mm = i; mx = d->x; my = d->y; t = 2;
          break;
        }
    if (mm < 0) fprintf(stderr, "znacznik: brak NPC '%s'\n", npcId);
  } else if (map >= 0) { mm = map; mx = x; my = y; t = 1; }
  H.vars[var_find("_CEL_T", 1)] = t;
  H.vars[var_find("_CEL_M", 1)] = mm;
  H.vars[var_find("_CEL_X", 1)] = mx;
  H.vars[var_find("_CEL_Y", 1)] = my;
  H.vars[var_find("_CEL_E", 1)] = 0;
  if (npcId) for (int i = 0; i < S.nmaps; i++) for (int k = 0; k < S.maps[i].nents; k++)
    if (!strcmp(S.maps[i].ents[k].id, npcId)) { H.vars[var_find("_CEL_E", 1)] = i * 1000 + k + 1; i = S.nmaps; break; }
}

int world_marker_pos(float *x, float *z) {
  int t = H.vars[var_find("_CEL_T", 1)];
  if (!t) return 0;
  int mm = H.vars[var_find("_CEL_M", 1)];
  float tx = H.vars[var_find("_CEL_X", 1)] + 0.5f, tz = H.vars[var_find("_CEL_Y", 1)] + 0.5f;
  if (mm == W.map) {
    if (t == 2) { /* żywy NPC na tej mapie */
      int e = H.vars[var_find("_CEL_E", 1)] - 1;
      if (e >= 0) for (int i = 0; i < W.n; i++) if (W.ents[i].d == &S.maps[e / 1000].ents[e % 1000]) { tx = W.ents[i].x; tz = W.ents[i].z; break; }
    }
    *x = tx; *z = tz;
    return 1;
  }
  /* cel na innej mapie: drzwi prowadzące do niej, a z wnętrza — wyjście */
  float best = 1e9f;
  int found = 0;
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (e->d->type != ENT_WARP || e->d->toMap < 0 || !e->vis) continue;
    float d = (e->d->toMap == mm ? 0 : 1000) + fabsf(e->x - W.px) + fabsf(e->z - W.pz);
    if (d < best) { best = d; *x = e->x; *z = e->z; found = 1; }
  }
  return found;
}

static float dir_angle(int dir) {
  return dir == DIR_DOWN ? 0.0f : dir == DIR_UP ? PI_F : dir == DIR_RIGHT ? PI_F * 0.5f : -PI_F * 0.5f;
}

static EntDef spawnDefs[32];
static int nspawn;

/* muzyka i tło dźwiękowe bieżącej mapy */
void world_audio(void) {
  MapDef *m = M();
  Region *g = W.region >= 0 ? &S.regions[W.region] : NULL;
  if (g && g->music[0]) music_play(g->music);
  else if (m->music[0]) music_play(m->music);
  int amb = g && g->ambient ? g->ambient : m->ambient ? m->ambient : m->interior ? AMB_ROOM : AMB_STREET;
  float space = amb == AMB_CHURCH || amb == AMB_WAREHOUSE ? 2 : m->interior ? 1 : 0;
  audio_ambience(amb, m->rain ? 1.0f : 0.0f, space);
}

void world_load_map(int map, int tx, int ty, int dir) {
  if (map < 0 || map >= S.nmaps) return;
  W.map = map;
  W.n = 0;
  nspawn = 0;
  MapDef *m = M();
  city_build(m);
  for (int i = 0; i < m->nents && W.n < MAX_ENTS; i++) {
    Ent *e = &W.ents[W.n++];
    memset(e, 0, sizeof *e);
    e->d = &m->ents[i];
    e->x = e->homeX = e->d->x + 0.5f;
    e->z = e->homeZ = e->d->y + 0.5f;
    e->ang = dir_angle(e->d->dir);
    e->walkT = 60 + rand() % 120;
    e->enemyDef = -1;
    e->htype = human_find(e->d->sprite);
    e->animT = (rand() % 1000) / 100.0f;
  }
  W.px = tx + 0.5f; W.pz = ty + 0.5f;
  W.ppx = W.px; W.ppz = W.pz; W.pbob = W.bob = 0;
  W.region = region_at(W.map, W.px, W.pz);
  snprintf(W.banner, sizeof W.banner, "%s", W.region >= 0 ? S.regions[W.region].name : m->name);
  W.yaw = dir_angle(dir); W.pitch = 0;
  W.grace = 30;
  W.bannerT = 170;
  world_audio();
  world_refresh_visibility();
  cars_init_map();
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
  d->condVar[0] = d->condVar[1] = d->condVar[2] = -1;
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
  e->htype = human_find(d->sprite);
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
  if (!blocked_circle(nx, *z, r) && !(self == NULL && cars_block(nx, *z, r))) *x = nx;
  float nz = *z + dz;
  if (!blocked_circle(*x, nz, r) && !(self == NULL && cars_block(*x, nz, r))) *z = nz;
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

void world_look(float dx, float dy) {
  W.yaw -= dx * 0.0032f;
  W.pitch -= dy * 0.0032f;
  W.pitch = CLAMP(W.pitch, -0.9f, 0.9f);
}

static void update_player(void) {
  if (car_player() >= 0) {
    /* za kierownicą: auto prowadzi vehicle.c, tu tylko zdarzenia i menu */
    W.bob = 0;
    check_tile_events();
    if (W.trans || script_running()) return;
    if (in.pressed[BTN_B]) { ui_open_pause(); return; }
    if (in.pressed[BTN_TAB] && tycoon_active() && !tycoon_mission_active()) { tycoon_open(); return; }
    return;
  }
  float spd = (in.held[BTN_RUN] ? 2.9f : 1.7f) / 60.0f;
  float fx = sinf(W.yaw), fz = cosf(W.yaw);
  float rx = -cosf(W.yaw), rz = sinf(W.yaw);
  float mx = 0, mz = 0;
  if (in.held[BTN_UP]) { mx += fx; mz += fz; }
  if (in.held[BTN_DOWN]) { mx -= fx; mz -= fz; }
  if (in.held[BTN_SR]) { mx += rx; mz += rz; }
  if (in.held[BTN_SL]) { mx -= rx; mz -= rz; }
  float ml = sqrtf(mx * mx + mz * mz);
  W.ppx = W.px; W.ppz = W.pz; W.pbob = W.bob;
  if (ml > 0.01f) {
    move_circle(&W.px, &W.pz, mx / ml * spd, mz / ml * spd, PR, NULL);
    W.bobT += in.held[BTN_RUN] ? 0.2f : 0.13f;
    if ((int)(W.bobT / PI_F) != (int)((W.bobT - 0.13f) / PI_F)) sfx_play(SFX_STEP);
  }
  static const float BOB[3] = {0, 0.005f, 0.012f};
  W.bob = ml > 0.01f ? sinf(W.bobT * 2) * BOB[CLAMP(g_headbob, 0, 2)] : W.bob * 0.8f;
  float turn = 2.4f / 60.0f;
  if (in.held[BTN_TL]) W.yaw += turn;
  if (in.held[BTN_TR]) W.yaw -= turn;
  world_look(in.mdx, in.mdy);
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
    if (world_enemies_alive() == 0 && cars_enter()) return;
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
  for (int i = 0; i < W.n; i++) W.ents[i].animT += 1.0f / 60.0f;
  if (W.grace > 0) W.grace--;
  if (W.bannerT > 0) W.bannerT--;
  if (W.hurtT > 0) W.hurtT--;
  /* wjazd do innej dzielnicy na dużej mapie: baner, muzyka, tło */
  if (S.nregions && !W.trans) {
    int r = region_at(W.map, W.px, W.pz);
    if (r >= 0 && r != W.region) {
      W.region = r;
      snprintf(W.banner, sizeof W.banner, "%s", S.regions[r].name);
      W.bannerT = 170;
      world_audio();
    }
  }
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
  cars_update();
  update_npcs();
  tycoon_world_tick();
}

/* ---------------------------------------------------------------- rysowanie */
static REnv env_for_map(const MapDef *m) {
  REnv e;
  memset(&e, 0, sizeof e);
  e.exposure = 1.0f;
  if (m->interior) {
    e.interior = 1;
    float f[3] = {0.035f, 0.026f, 0.022f};
    memcpy(e.fogCol, f, sizeof f);
    e.fogDensity = 0.035f;
    e.ambSky[0] = 0.2f; e.ambSky[1] = 0.165f; e.ambSky[2] = 0.135f;
    e.ambGround[0] = 0.07f; e.ambGround[1] = 0.055f; e.ambGround[2] = 0.045f;
    e.sunDir[0] = 0.3f; e.sunDir[1] = 0.8f; e.sunDir[2] = 0.4f;
    e.sunCol[0] = 0.05f; e.sunCol[1] = 0.045f; e.sunCol[2] = 0.04f;
    e.exposure = 1.05f;
    return e;
  }
  if (m->night) {
    float top[3] = {0.012f, 0.016f, 0.035f}, hor[3] = {0.11f, 0.08f, 0.12f}, glow[3] = {0.22f, 0.11f, 0.05f};
    memcpy(e.skyTop, top, 12); memcpy(e.skyHorizon, hor, 12); memcpy(e.skyGlow, glow, 12);
    e.fogCol[0] = 0.075f; e.fogCol[1] = 0.062f; e.fogCol[2] = 0.085f;
    e.fogDensity = m->rain ? 0.05f : 0.038f;
    e.ambSky[0] = 0.12f; e.ambSky[1] = 0.125f; e.ambSky[2] = 0.17f;
    e.ambGround[0] = 0.06f; e.ambGround[1] = 0.05f; e.ambGround[2] = 0.045f;
    e.sunDir[0] = -0.4f; e.sunDir[1] = 0.55f; e.sunDir[2] = 0.6f;
    e.sunCol[0] = 0.1f; e.sunCol[1] = 0.12f; e.sunCol[2] = 0.2f;
    e.wet = m->rain ? 1.0f : 0.35f;
    e.rain = m->rain ? 1.0f : 0;
    e.moon = m->rain ? 0.25f : 1.0f;
    e.stars = m->rain ? 0.1f : 1.0f;
    return e;
  }
  /* dzień (pochmurny, złote popołudnie) */
  float top[3] = {0.25f, 0.38f, 0.62f}, hor[3] = {0.75f, 0.7f, 0.62f}, glow[3] = {0.3f, 0.2f, 0.1f};
  memcpy(e.skyTop, top, 12); memcpy(e.skyHorizon, hor, 12); memcpy(e.skyGlow, glow, 12);
  e.fogCol[0] = 0.62f; e.fogCol[1] = 0.6f; e.fogCol[2] = 0.56f;
  e.fogDensity = 0.022f;
  e.ambSky[0] = 0.55f; e.ambSky[1] = 0.58f; e.ambSky[2] = 0.65f;
  e.ambGround[0] = 0.25f; e.ambGround[1] = 0.22f; e.ambGround[2] = 0.2f;
  e.sunDir[0] = -0.5f; e.sunDir[1] = 0.6f; e.sunDir[2] = 0.4f;
  e.sunCol[0] = 1.6f; e.sunCol[1] = 1.35f; e.sunCol[2] = 1.0f;
  e.wet = m->rain ? 0.8f : 0;
  e.rain = m->rain ? 1.0f : 0;
  e.exposure = 0.9f;
  return e;
}

static void draw_ent(Ent *e) {
  float dx = e->x - W.px, dz = e->z - W.pz;
  if (dx * dx + dz * dz > 40 * 40) return;
  if (e->htype < 0) {
    int md = objmodel_find(e->d->sprite);
    if (md < 0) return;
    float y = tile_prop_height(tile_at((int)floorf(e->x), (int)floorf(e->z)));
    if (y > 2) y = 0;
    M4 mm = m4_mul(m4_translate(e->x, y, e->z), m4_roty(e->ang));
    r_draw(&MODELS[md], &mm, 0);
    if (e->d->script >= 0) {
      float p = 0.5f + 0.5f * sinf(g_time * 3);
      r_glow(e->x, y + 0.08f, e->z, 0.25f, 0.25f * p, 0.2f * p, 0.08f * p);
    }
    return;
  }
  HumanPose p;
  memset(&p, 0, sizeof p);
  p.t = e->animT;
  p.weapon = e->enemyDef >= 0 ? S.enemies[e->enemyDef].weapon : 0;
  p.hitT = e->hitT / 8.0f;
  if (e->dead) { p.anim = AN_DEAD; p.deadT = 1.0f - e->deadT / 24.0f; }
  else if (e->shootT > 0) p.anim = AN_SHOOT;
  else if ((e->hostile || e->ally) && e->state >= 2) p.anim = e->moving ? AN_RUN : AN_AIM;
  else if (e->moving) p.anim = AN_WALK;
  else if (script_self() == e && script_blocking()) p.anim = AN_TALK;
  else p.anim = e->d->pose ? e->d->pose : AN_IDLE;
  if (p.anim == AN_AIM || p.anim == AN_SHOOT) {
    float d = sqrtf(dx * dx + dz * dz);
    p.aimPitch = atan2f(0.0f, d > 0.1f ? d : 0.1f);
  }
  float gy = world_ground(e->x, e->z);
  if (e->d->pose == AN_SIT && !e->dead && p.anim == AN_SIT) {
    M4 cm = m4_mul(m4_translate(e->x, gy, e->z), m4_roty(e->ang + PI_F));
    r_draw(&MODELS[MD_CHAIR], &cm, 0);
  }
  human_draw(e->htype, e->x, gy, e->z, e->ang, &p, 0);
}

void world_draw(void) {
  MapDef *m = M();
  REnv env = env_for_map(m);
  /* interpolacja między krokami logiki (płynność przy monitorach > 60 Hz) */
  float a = g_alpha < 0 ? 0 : g_alpha > 1 ? 1 : g_alpha;
  if (fabsf(W.px - W.ppx) + fabsf(W.pz - W.ppz) > 1.0f) a = 1;
  float cx = W.ppx + (W.px - W.ppx) * a, cz = W.ppz + (W.pz - W.ppz) * a, cb = W.pbob + (W.bob - W.pbob) * a;
  float gy = world_ground(cx, cz);
  static float camY;
  static double lastT;
  float target = gy + 0.78f;
  if (fabsf(camY - target) > 0.5f) camY = target;
  float dt = (float)(g_time - lastT);
  lastT = g_time;
  if (dt < 0 || dt > 0.1f) dt = 1.0f / 60;
  camY += (target - camY) * (1 - powf(0.75f, dt * 60)); /* wygładzanie krawężników niezależne od FPS */
  RCam cam = {v3(cx, camY + cb, cz), W.yaw, W.pitch, (float)g_cfg.fov};
  int driving = cars_camera(&cam, a);
  if (getenv("KX_CAM")) fprintf(stderr, "cam %.4f %.4f %.4f fov %.1f dt %.4f\n", cam.pos.x, cam.pos.y, cam.pos.z, cam.fov, dt);
  r_frame_begin(&cam, &env);
  city_lights(g_time);
  combat_lights();
  if (!driving) r_light(cx, camY + 0.3f, cz, 0.22f, 0.2f, 0.18f, 4.0f); /* delikatne doświetlenie wokół gracza */
  cars_lights(m->night);
  r_lights_commit();
  city_draw();
  cars_draw();
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis && !(e->dead && e->d->type == ENT_MOB)) continue;
    if (e->d->type == ENT_WARP || e->d->type == ENT_EVENT) continue;
    draw_ent(e);
  }
  /* słup światła nad celem misji */
  {
    float mx, mz;
    if (world_marker_pos(&mx, &mz) && !M()->interior) {
      float p = 0.75f + 0.25f * sinf(g_time * 3.0f);
      for (int k = 0; k < 9; k++) r_glow(mx, 0.4f + k * 0.7f, mz, 0.55f - k * 0.03f, 0.9f * p, 0.62f * p, 0.18f * p);
    }
  }
  combat_draw_world();
  city_glows();
  city_steam(g_time);
  r_glow_flush();
  if (env.rain > 0) r_rain(env.rain * 0.55f);
  if (!script_blocking() && g_mode == MODE_WORLD && !driving) combat_draw_view();
  float hurt = W.hurtT / 18.0f;
  r_grade(0.9f, 1.0f, 1.0f + hurt * 0.8f, 0);
  r_frame_end();
}

/* nakładki UI świata: podpowiedź interakcji, nazwa lokacji, HUD */
void world_draw_ui(void) {
  if (!script_blocking()) combat_draw_hud();
  if (!script_blocking() && !W.trans && g_mode == MODE_WORLD) {
    Ent *t = look_target(NULL);
    char b[128] = "";
    if (t) {
      const char *nm = t->d->type == ENT_NPC ? "Rozmawiaj" : "Zbadaj";
      int sp = speaker_find(t->d->id);
      if (sp >= 0 && t->d->type == ENT_NPC) snprintf(b, sizeof b, "%s  \xe2\x80\x94  %s", nm, S.speakers[sp].name);
      else snprintf(b, sizeof b, "%s", nm);
    } else {
      Ent *door = look_door();
      if (door && door->d->toMap >= 0) snprintf(b, sizeof b, "Wejdź  \xe2\x80\x94  %s", S.maps[door->d->toMap].name);
      else if (car_player() < 0 && cars_near() >= 0 && world_enemies_alive() == 0) snprintf(b, sizeof b, "Wsiądź do auta");
    }
    if (car_player() >= 0) b[0] = 0;
    if (b[0]) {
      float w = d2_text_w(FONT_BOLD, 20, b) + 70;
      float x = (UI_W - w) / 2, y = UI_H / 2 + 60;
      d2_shadow(x, y, w, 40, 10, 10, 0x80000000);
      d2_rrect(x, y, w, 40, 10, 0xC8141016);
      d2_rrect_line(x, y, w, 40, 10, 1.5f, 0x90E8B84A);
      d2_rrect(x + 8, y + 7, 26, 26, 6, UI_GOLD);
      d2_text_c(FONT_BOLD, 18, x + 21, y + 9, "E", 0xFF141016);
      d2_text_sh(FONT_BOLD, 20, x + 46, y + 8, b, UI_CREAM);
    }
  }
  if (W.bannerT > 0 && W.banner[0]) {
    float a = W.bannerT > 140 ? (170 - W.bannerT) / 30.0f : W.bannerT < 40 ? W.bannerT / 40.0f : 1.0f;
    int ia = (int)(a * 255);
    float w = d2_text_w(FONT_SERIF, 46, W.banner);
    float x = (UI_W - w) / 2;
    d2_grad(x - 120, 92, w + 240, 76, WITH_A(0x000000, ia * 0), WITH_A(0x000000, ia * 0));
    d2_rect(UI_W / 2 - (w / 2 + 60) * a, 100, (w + 120) * a, 1.5f, WITH_A(UI_GOLD, ia));
    d2_rect(UI_W / 2 - (w / 2 + 60) * a, 160, (w + 120) * a, 1.5f, WITH_A(UI_GOLD, ia));
    d2_text_sh(FONT_SERIF, 46, x, 106, W.banner, WITH_A(UI_CREAM, ia));
  }
}
