/* Walka FPP: bronie gracza (hitscan), widok broni, HUD, sztuczna inteligencja
 * przeciwników i sojuszników (wykrywanie, pościg po siatce, strzelanie). */
#include "engine.h"

const char *WPN_NAMES[WPN_COUNT] = {"Pięści", "Colt 1911", "Strzelba", "Thompson"};
typedef struct { int dmg, delay, pellets, clip, reload, sfx, autof; float spread, range; } WpnDef;
static const WpnDef WD[WPN_COUNT] = {
  {16, 22, 1, 0, 0, SFX_PUNCH, 0, 0.0f, 1.15f},
  {30, 14, 1, 7, 55, SFX_PISTOL, 0, 0.012f, 40.0f},
  {14, 46, 7, 6, 85, SFX_SHOTGUN, 0, 0.08f, 16.0f},
  {18, 6, 1, 50, 95, SFX_TOMMY, 1, 0.04f, 35.0f},
};
/* broń wroga: opóźnienie między strzałami i zasięg */
static const int EN_DELAY[WPN_COUNT] = {40, 58, 85, 20};
static const float EN_RANGE[WPN_COUNT] = {1.0f, 14.0f, 9.0f, 15.0f};

static struct { int cool, reloadT, kick, flashT, hitMark, swapT, punchT, aimEnemy; } C;

/* cząstki: krew (kropelki), iskry (świecące), dym */
typedef struct { float x, y, z, vx, vy, vz; int t, life; u32 c; int kind; } Spark;
#define NSPARK 160
static Spark sparks[NSPARK];
static int sparkNext;

void combat_init(void) { memset(&C, 0, sizeof C); memset(sparks, 0, sizeof sparks); }

static void spark1(float x, float y, float z, float vx, float vy, float vz, int life, u32 c, int kind) {
  Spark *s = &sparks[sparkNext];
  sparkNext = (sparkNext + 1) % NSPARK;
  *s = (Spark){x, y, z, vx, vy, vz, life, life, c, kind};
}
static float frand(void) { return (rand() % 2001) / 1000.0f - 1.0f; }
/* c == czerwony -> krew, inaczej iskry/pył */
static void spark(float x, float y, float z, u32 c) {
  int blood = ((c >> 16) & 255) > 150 && ((c >> 8) & 255) < 120;
  for (int k = 0; k < (blood ? 9 : 6); k++) {
    if (blood) spark1(x, y, z, frand() * 0.02f, 0.01f + frand() * 0.015f, frand() * 0.02f, 30 + rand() % 20, 0xFF6A0A0E, 0);
    else spark1(x, y, z, frand() * 0.03f, 0.02f + frand() * 0.02f, frand() * 0.03f, 10 + rand() % 8, 0xFFFFC060, 1);
  }
  if (!blood) spark1(x, y, z, 0, 0.003f, 0, 40, 0xFF8A8478, 2);
}

static void alert_noise(float x, float z, float radius) {
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis || e->dead || !(e->hostile || e->ally)) continue;
    float dx = e->x - x, dz = e->z - z;
    if (dx * dx + dz * dz < radius * radius && e->state < 2) { e->state = 1; e->stateT = 10 + rand() % 20; }
  }
}

static float bullet_block_t(float ox, float oy, float oz, float dx, float dy, float dz, float range) {
  for (float t = 0.05f; t < range; t += 0.04f) {
    float x = ox + dx * t, y = oy + dy * t, z = oz + dz * t;
    if (y < 0.0f) return t;
    int tx = (int)floorf(x), tz = (int)floorf(z);
    if (world_solid_at(x, z)) {
      MapDef *m = &S.maps[W.map];
      int c = (tx >= 0 && tz >= 0 && tx < m->w && tz < m->h) ? m->tiles[tz][tx] : 'x';
      /* niskie rekwizyty zatrzymują tylko nisko lecące kule */
      float h = 9.0f;
      if (strchr("XktbhPdeBAm", c)) h = 0.6f;
      if (strchr("TpLFIY", c)) h = (c == 'T') ? 2.4f : 0.0f;
      if (c == 'c' || c == 'C') h = 0.8f;
      if (y < h) return t;
    }
  }
  return range;
}

static void kill_ent(Ent *e, int byPlayer) {
  e->dead = 1;
  e->deadT = 24;
  e->state = 0;
  sfx_play(SFX_DIE);
  if (byPlayer && e->enemyDef >= 0 && e->hostile) {
    int cash = S.enemies[e->enemyDef].cash;
    if (cash > 0) {
      H.gold += cash;
      char b[32];
      snprintf(b, sizeof b, "+%d $", cash);
      ui_toast(b);
    }
    int w = H.weapon;
    if (w == WPN_PISTOL) H.ammo[w] += 4;
    else if (w == WPN_TOMMY) H.ammo[w] += 12;
    else if (w == WPN_SHOTGUN) H.ammo[w] += 2;
  }
}

static void damage_ent(Ent *e, int dmg, int byPlayer) {
  if (e->dead) return;
  e->hp -= dmg;
  e->hitT = 8;
  if (e->state < 2) e->state = 2;
  if (e->hp <= 0) kill_ent(e, byPlayer);
}

static Ent *ray_ent(float ox, float oy, float oz, float dx, float dy, float dz, float maxT, int wantHostile, float *hitT, float *hitY) {
  Ent *best = NULL;
  float bt = maxT;
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis || e->dead) continue;
    if (wantHostile ? !e->hostile : !(e->ally)) continue;
    float fx = ox - e->x, fz = oz - e->z, r = 0.26f;
    float a = dx * dx + dz * dz, b = 2 * (dx * fx + dz * fz), c = fx * fx + fz * fz - r * r;
    float disc = b * b - 4 * a * c;
    if (disc < 0 || a < 1e-6f) continue;
    float t = (-b - sqrtf(disc)) / (2 * a);
    if (t < 0 || t > bt) continue;
    float y = oy + dy * t;
    if (y < 0 || y > 0.93f) continue;
    bt = t; best = e;
    if (hitY) *hitY = y;
  }
  if (hitT) *hitT = bt;
  return best;
}

static void player_shot(float spreadYaw, float spreadPitch, int dmg, float range) {
  float yaw = W.yaw + spreadYaw, pitch = W.pitch + spreadPitch;
  float dx = sinf(yaw) * cosf(pitch), dy = sinf(pitch), dz = cosf(yaw) * cosf(pitch);
  float oy = 0.78f;
  float wt = bullet_block_t(W.px, oy, W.pz, dx, dy, dz, range);
  float ht, hy;
  Ent *e = ray_ent(W.px, oy, W.pz, dx, dy, dz, wt, 1, &ht, &hy);
  if (e) {
    int head = hy > 0.74f;
    damage_ent(e, head ? dmg * 2 : dmg, 1);
    C.hitMark = head ? 14 : 8;
    spark(W.px + dx * ht, oy + dy * ht, W.pz + dz * ht, 0xFFB13E53);
  } else if (wt < range) {
    spark(W.px + dx * wt, oy + dy * wt, W.pz + dz * wt, 0xFFFFCD75);
  }
}

static void fire(void) {
  int w = H.weapon;
  const WpnDef *d = &WD[w];
  if (w != WPN_FISTS) {
    if (H.clip[w] <= 0) {
      if (H.ammo[w] > 0) { C.reloadT = d->reload; sfx_play(SFX_RELOAD); }
      else { sfx_play(SFX_EMPTY); C.cool = 20; }
      return;
    }
    H.clip[w]--;
    C.flashT = 4;

    alert_noise(W.px, W.pz, 16.0f);
  } else {
    C.punchT = 14;
  }
  sfx_play(d->sfx);
  C.cool = d->delay;
  C.kick = w == WPN_SHOTGUN ? 14 : w == WPN_TOMMY ? 4 : 8;
  for (int p = 0; p < d->pellets; p++) {
    float sy = ((rand() % 2001) / 1000.0f - 1.0f) * d->spread;
    float sp = ((rand() % 2001) / 1000.0f - 1.0f) * d->spread * 0.6f;
    player_shot(sy, sp, d->dmg, d->range);
  }
}

/* napełnia pusty magazynek od razu (nowa broń, zakup amunicji) */
void combat_top_up(int w) {
  if (w <= WPN_FISTS || w >= WPN_COUNT || H.clip[w] > 0) return;
  int take = WD[w].clip < H.ammo[w] ? WD[w].clip : H.ammo[w];
  H.clip[w] += take; H.ammo[w] -= take;
}

void combat_update(void) {
  if (C.cool > 0) C.cool--;
  if (C.kick > 0) C.kick--;
  if (C.flashT > 0) C.flashT--;
  if (C.hitMark > 0) C.hitMark--;
  if (C.punchT > 0) C.punchT--;
  if (C.swapT > 0) C.swapT--;
  for (int i = 0; i < NSPARK; i++) {
    Spark *p = &sparks[i];
    if (p->t <= 0) continue;
    p->t--;
    p->x += p->vx; p->y += p->vy; p->z += p->vz;
    if (p->kind == 0) { p->vy -= 0.0018f; if (p->y < 0.01f) { p->y = 0.01f; p->vx = p->vz = p->vy = 0; } }
    if (p->kind == 1) p->vy -= 0.002f;
  }
  if (C.reloadT > 0) {
    if (--C.reloadT == 0) {
      int w = H.weapon;
      int need = WD[w].clip - H.clip[w];
      int take = need < H.ammo[w] ? need : H.ammo[w];
      H.clip[w] += take; H.ammo[w] -= take;
    }
    return;
  }
  /* zmiana broni */
  int want = -1;
  if (in.pressed[BTN_W1]) want = WPN_FISTS;
  if (in.pressed[BTN_W2]) want = WPN_PISTOL;
  if (in.pressed[BTN_W3]) want = WPN_SHOTGUN;
  if (in.pressed[BTN_W4]) want = WPN_TOMMY;
  if (in.pressed[BTN_WNEXT])
    for (int k = 1; k <= WPN_COUNT; k++) { int w = (H.weapon + k) % WPN_COUNT; if (H.hasWpn[w]) { want = w; break; } }
  if (want >= 0 && H.hasWpn[want] && want != H.weapon) { H.weapon = want; C.swapT = 12; C.cool = 12; sfx_play(SFX_RELOAD); }
  if (in.pressed[BTN_RELOAD] && H.weapon != WPN_FISTS && H.clip[H.weapon] < WD[H.weapon].clip && H.ammo[H.weapon] > 0) {
    C.reloadT = WD[H.weapon].reload; sfx_play(SFX_RELOAD); return;
  }
  if (H.weapon != WPN_FISTS && H.clip[H.weapon] <= 0 && H.ammo[H.weapon] > 0 && C.cool <= 0) {
    C.reloadT = WD[H.weapon].reload; sfx_play(SFX_RELOAD); return;
  }
  int trig = WD[H.weapon].autof ? in.held[BTN_FIRE] : in.pressed[BTN_FIRE];
  if (trig && C.cool <= 0 && C.swapT <= 0) fire();
  /* celownik nad wrogiem */
  float dx = sinf(W.yaw) * cosf(W.pitch), dy = sinf(W.pitch), dz = cosf(W.yaw) * cosf(W.pitch);
  C.aimEnemy = ray_ent(W.px, 0.78f, W.pz, dx, dy, dz, bullet_block_t(W.px, 0.78f, W.pz, dx, dy, dz, 30), 1, NULL, NULL) != NULL;
}

/* ---------------------------------------------------------------- AI */
static int bfs_next(int sx, int sz, int gx, int gz, int *nx, int *nz) {
  MapDef *m = &S.maps[W.map];
  static short prev[MAX_MAP_H * MAX_MAP_W];
  static int queue[MAX_MAP_H * MAX_MAP_W];
  if (sx == gx && sz == gz) return 0;
  for (int i = 0; i < m->w * m->h; i++) prev[i] = -1;
  int head = 0, tail = 0, start = sz * m->w + sx, goal = gz * m->w + gx;
  if (gx < 0 || gz < 0 || gx >= m->w || gz >= m->h) return 0;
  prev[start] = (short)start;
  queue[tail++] = start;
  int found = 0;
  while (head < tail && tail < 1500) {
    int cur = queue[head++];
    if (cur == goal) { found = 1; break; }
    int cx = cur % m->w, cz = cur / m->w;
    static const int D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int k = 0; k < 4; k++) {
      int x = cx + D[k][0], z = cz + D[k][1];
      if (x < 0 || z < 0 || x >= m->w || z >= m->h) continue;
      int id = z * m->w + x;
      if (prev[id] >= 0 || (tile_solid(m->tiles[z][x]) && id != goal)) continue;
      prev[id] = (short)cur;
      queue[tail++] = id;
    }
  }
  if (!found) return 0;
  int cur = goal;
  while (prev[cur] != start) cur = prev[cur];
  *nx = cur % m->w; *nz = cur / m->w;
  return 1;
}

static Ent *nearest_foe(Ent *self, int wantHostile, float *dist) {
  Ent *best = NULL;
  float bd = 1e9f;
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (e == self || !e->vis || e->dead) continue;
    if (wantHostile ? !e->hostile : !e->ally) continue;
    float dx = e->x - self->x, dz = e->z - self->z;
    float d = sqrtf(dx * dx + dz * dz);
    if (d < bd && world_los(self->x, self->z, e->x, e->z)) { bd = d; best = e; }
  }
  if (dist) *dist = bd;
  return best;
}

static void face(Ent *e, float tx, float tz) { e->ang = atan2f(tx - e->x, tz - e->z); }

static void move_towards(Ent *e, float tx, float tz, float speed) {
  float dx = tx - e->x, dz = tz - e->z;
  float d = sqrtf(dx * dx + dz * dz);
  if (d < 0.05f) { e->moving = 0; return; }
  int ex = (int)floorf(e->x), ez = (int)floorf(e->z);
  if (!world_los(e->x, e->z, tx, tz)) {
    if (--e->walkT <= 0 || (e->tx == 0 && e->tz == 0)) {
      int nx, nz;
      if (bfs_next(ex, ez, (int)floorf(tx), (int)floorf(tz), &nx, &nz)) { e->tx = nx + 0.5f; e->tz = nz + 0.5f; }
      e->walkT = 20;
    }
    dx = e->tx - e->x; dz = e->tz - e->z;
    d = sqrtf(dx * dx + dz * dz);
    if (d < 0.05f) { e->walkT = 0; return; }
  }
  e->ang = atan2f(dx, dz);
  float ox = e->x, oz = e->z;
  move_circle(&e->x, &e->z, dx / d * speed, dz / d * speed, 0.2f, e);
  e->moving = fabsf(ox - e->x) + fabsf(oz - e->z) > 0.0005f;
  if (!e->moving) e->walkT = 0;
}

void combat_ent_update(Ent *e) {
  if (e->dead) { if (e->deadT > 0) e->deadT--; return; }
  if (e->hitT > 0) e->hitT--;
  if (e->shootT > 0) e->shootT--;
  if (e->cool > 0) e->cool--;
  if (e->enemyDef < 0) return;
  EnemyDef *d = &S.enemies[e->enemyDef];
  float speed = (d->speed > 0 ? d->speed : 10) / 600.0f;
  int wpn = d->weapon;

  /* cel: wróg sojusznika albo gracz/sojusznik dla wroga */
  float tx, tz, dist;
  Ent *target = NULL;
  if (e->ally) {
    target = nearest_foe(e, 1, &dist);
    if (!target) { /* idź za graczem */
      float dx = W.px - e->x, dz = W.pz - e->z;
      float dp = sqrtf(dx * dx + dz * dz);
      if (dp > 2.0f) move_towards(e, W.px, W.pz, speed * 1.3f); else e->moving = 0;
      e->state = 0;
      return;
    }
    tx = target->x; tz = target->z;
  } else {
    tx = W.px; tz = W.pz;
    float dx = tx - e->x, dz = tz - e->z;
    dist = sqrtf(dx * dx + dz * dz);
    float da;
    Ent *ally = nearest_foe(e, 0, &da);
    if (ally && da < dist * 0.7f) { target = ally; tx = ally->x; tz = ally->z; dist = da; }
  }
  int los = world_los(e->x, e->z, tx, tz);

  if (e->state == 0) {
    if (!e->ally) {
      float fx = sinf(e->ang), fz = cosf(e->ang);
      float dot = dist > 0.01f ? ((tx - e->x) * fx + (tz - e->z) * fz) / dist : 1;
      if (los && dist < 11 && (dot > 0.3f || dist < 3.0f)) { e->state = 1; e->stateT = 18 + rand() % 22; }
      else if (e->d->wander) move_towards(e, e->homeX + sinf(W.time * 0.01f + e->d->x) * 1.5f, e->homeZ + cosf(W.time * 0.013f + e->d->y) * 1.5f, speed * 0.4f);
    } else e->state = 2;
    return;
  }
  if (e->state == 1) {
    face(e, tx, tz);
    e->moving = 0;
    if (--e->stateT <= 0) e->state = 2;
    return;
  }
  /* walka */
  float range = EN_RANGE[wpn];
  if (!los || dist > range * 0.85f) { move_towards(e, tx, tz, speed); return; }
  if (wpn == WPN_FISTS && dist > 0.75f) { move_towards(e, tx, tz, speed * 1.2f); return; }
  face(e, tx, tz);
  e->moving = 0;
  if ((W.time + e->d->x * 7) % 90 < 20 && wpn != WPN_FISTS) { /* krok w bok */
    float sx = cosf(e->ang), sz = -sinf(e->ang);
    float s = ((e->d->y + W.time / 90) % 2) ? 1.0f : -1.0f;
    move_circle(&e->x, &e->z, sx * speed * 0.6f * s, sz * speed * 0.6f * s, 0.2f, e);
    e->moving = 1;
  }
  if (e->cool > 0) return;
  e->cool = EN_DELAY[wpn] + rand() % 25;
  e->shootT = 6;
  int sfx = wpn == WPN_FISTS ? SFX_PUNCH : wpn == WPN_TOMMY ? SFX_TOMMY : wpn == WPN_SHOTGUN ? SFX_SHOTGUN : SFX_PISTOL;
  float pd = sqrtf((W.px - e->x) * (W.px - e->x) + (W.pz - e->z) * (W.pz - e->z));
  sfx_play_at(sfx, pd);
  int bursts = wpn == WPN_TOMMY ? 3 : 1;
  for (int b = 0; b < bursts; b++) {
    float chance = d->acc / 100.0f * (1.0f - dist / (range * 1.6f));
    if (!e->ally && in.held[BTN_RUN] && (in.held[BTN_UP] || in.held[BTN_SL] || in.held[BTN_SR])) chance *= 0.65f;
    if (wpn == WPN_SHOTGUN && dist < 4) chance += 0.2f;
    if ((rand() % 1000) / 1000.0f < chance) {
      int dmg = d->dmg * (85 + rand() % 31) / 100;
      if (e->ally) { if (target) damage_ent(target, dmg, 0); }
      else if (target) damage_ent(target, dmg, 0);
      else world_hurt_player(dmg);
    }
  }
}

/* ---------------------------------------------------------------- rysowanie (GPU) */
static GMesh blob;
static void ensure_blob(void) {
  if (blob.vao) return;
  MB b;
  mb_init(&b);
  mb_paint(0xFFFFFFFF, L_WHITE);
  mb_sphere(&b, v3(0, 0, 0), 1, 1, 1, 6);
  blob = gm_upload(&b);
  mb_free(&b);
}

/* światła: błysk lufy gracza i wrogów */
void combat_lights(void) {
  if (C.flashT > 0 && H.weapon != WPN_FISTS) {
    M4 cw = r_cam_to_world();
    V3 p = m4_point(cw, v3(0.1f, -0.05f, -0.6f));
    r_light(p.x, p.y, p.z, 4.5f, 3.2f, 1.6f, 5.0f);
  }
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis || e->dead || e->shootT < 4) continue;
    if (e->enemyDef >= 0 && S.enemies[e->enemyDef].weapon == WPN_FISTS) continue;
    r_light(e->x + sinf(e->ang) * 0.4f, 0.6f, e->z + cosf(e->ang) * 0.4f, 4.0f, 2.8f, 1.4f, 4.0f);
  }
}

void combat_draw_world(void) {
  ensure_blob();
  for (int i = 0; i < NSPARK; i++) {
    Spark *p = &sparks[i];
    if (p->t <= 0) continue;
    float k = (float)p->t / p->life;
    if (p->kind == 1) { r_glow(p->x, p->y, p->z, 0.05f, 2.0f * k, 1.4f * k, 0.5f * k); continue; }
    float sz = p->kind == 0 ? 0.012f + (1 - k) * 0.01f : 0.03f + (1 - k) * 0.1f;
    if (p->kind == 2 && k < 0.05f) continue;
    M4 m = m4_mul(m4_translate(p->x, p->y, p->z), m4_scale(sz, p->kind == 0 && p->y < 0.02f ? sz * 0.15f : sz, sz));
    r_draw(&blob, &m, p->c);
  }
  /* błysk lufy wrogów */
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis || e->dead || e->shootT < 3) continue;
    if (e->enemyDef >= 0 && S.enemies[e->enemyDef].weapon == WPN_FISTS) continue;
    float fx = sinf(e->ang), fz = cosf(e->ang);
    float rx = -cosf(e->ang), rz = sinf(e->ang);
    r_glow(e->x + fx * 0.42f + rx * 0.12f, 0.62f, e->z + fz * 0.42f + rz * 0.12f, 0.18f, 3.0f, 2.0f, 0.8f);
  }
}

void combat_draw_view(void) {
  if (g_mode != MODE_WORLD) return;
  float reload = 0, swap = 0;
  if (C.reloadT > 0) { int t = C.reloadT, tot = WD[H.weapon].reload; reload = sinf((float)t / tot * PI_F); }
  if (C.swapT > 0) swap = C.swapT / 12.0f;
  float punch = C.punchT > 6 ? (14 - C.punchT) / 8.0f : C.punchT / 6.0f;
  r_viewmodel_begin(52);
  weapon_draw_fpp(H.weapon, W.bobT, C.kick / 14.0f, reload, swap, C.punchT > 0 ? punch : 0, C.flashT > 0);
  r_viewmodel_end();
}

/* ---------------------------------------------------------------- HUD */
static void minimap(float x0, float y0, float sz) {
  MapDef *m = &S.maps[W.map];
  d2_shadow(x0, y0, sz, sz, 14, 12, 0x90000000);
  d2_rrect(x0, y0, sz, sz, 14, 0xC010121A);
  d2_clip(x0 + 3, y0 + 3, sz - 6, sz - 6);
  float cx = x0 + sz / 2, cy = y0 + sz / 2, ts = 9.0f;
  float ca = cosf(W.yaw), sa = sinf(W.yaw);
  int R = (int)(sz / ts * 0.75f) + 1;
  for (int dz = -R; dz <= R; dz++)
    for (int dx = -R; dx <= R; dx++) {
      int tx = (int)floorf(W.px) + dx, tz = (int)floorf(W.pz) + dz;
      if (tx < 0 || tz < 0 || tx >= m->w || tz >= m->h) continue;
      int c = m->tiles[tz][tx];
      u32 col;
      if (c == '~') col = 0xFF1E3A50;
      else if (tile_solid(c) && tile_prop_height(c) > 2) col = (c == 'D') ? 0xFFB8903A : 0xFF2C2A30;
      else if (tile_solid(c)) col = 0xFF4A4650;
      else if (c == ',') col = 0xFF6A6870;
      else if (c == '.' || c == '-' || c == '|') col = 0xFF3E3E46;
      else col = 0xFF5A5048;
      /* obrót: kierunek patrzenia gracza = góra minimapy */
      float wx = tx + 0.5f - W.px, wz = tz + 0.5f - W.pz;
      float sx = -(wx * ca - wz * sa), sy = -(wx * sa + wz * ca);
      float px = cx + sx * ts, py = cy + sy * ts;
      float hx = -ca * ts * 0.5f, hy = -sa * ts * 0.5f;
      d2_line(px - hx, py - hy, px + hx, py + hy, ts + 0.6f, col);
    }
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis || e->d->type == ENT_EVENT) continue;
    if (e->dead) continue;
    float wx = e->x - W.px, wz = e->z - W.pz;
    float sx = -(wx * ca - wz * sa), sy = -(wx * sa + wz * ca);
    u32 col = e->hostile ? UI_RED : e->ally ? UI_GREEN : e->d->type == ENT_WARP ? 0 : e->d->type == ENT_OBJ ? UI_GOLD : 0xFFE8E8E8;
    if (!col) continue;
    d2_circle(cx + sx * ts, cy + sy * ts, e->hostile ? 4.5f : 3.5f, col);
  }
  d2_noclip();
  /* gracz */
  d2_line(cx, cy - 8, cx - 6, cy + 6, 3, UI_GOLD);
  d2_line(cx, cy - 8, cx + 6, cy + 6, 3, UI_GOLD);
  d2_rrect_line(x0, y0, sz, sz, 14, 2, 0xA0E8B84A);
}

void combat_draw_hud(void) {
  if (g_mode != MODE_WORLD) return;
  char b[96];
  float cx = UI_W / 2.0f, cy = UI_H / 2.0f;
  /* celownik */
  u32 ch = C.aimEnemy ? 0xE8F05050 : 0xD0F0F0F0;
  float sp = 7 + (C.kick > 0 ? C.kick * 0.8f : 0) + (in.held[BTN_UP] || in.held[BTN_DOWN] || in.held[BTN_SL] || in.held[BTN_SR] ? 3 : 0);
  if (H.weapon != WPN_FISTS) {
    d2_line(cx - sp - 9, cy, cx - sp, cy, 2, ch); d2_line(cx + sp, cy, cx + sp + 9, cy, 2, ch);
    d2_line(cx, cy - sp - 9, cx, cy - sp, 2, ch); d2_line(cx, cy + sp, cx, cy + sp + 9, 2, ch);
  }
  d2_circle(cx, cy, 2, ch);
  if (C.hitMark > 0) {
    u32 hc = C.hitMark > 10 ? 0xFFFF5040 : 0xFFF0E0B0;
    d2_line(cx - 14, cy - 14, cx - 6, cy - 6, 2.5f, hc); d2_line(cx + 14, cy - 14, cx + 6, cy - 6, 2.5f, hc);
    d2_line(cx - 14, cy + 14, cx - 6, cy + 6, 2.5f, hc); d2_line(cx + 14, cy + 14, cx + 6, cy + 6, 2.5f, hc);
  }
  /* zdrowie i pancerz */
  float hx = 28, hy = UI_H - 92;
  d2_shadow(hx, hy, 330, 64, 12, 14, 0x90000000);
  d2_rrect(hx, hy, 330, 64, 12, 0xC8121016);
  d2_rrect_line(hx, hy, 330, 64, 12, 1.5f, 0x70E8B84A);
  int lowhp = H.hp < 30;
  d2_text(FONT_SANS, 30, hx + 16, hy + 12, "\xe2\x99\xa5", lowhp ? (((g_frame / 15) & 1) ? UI_RED : 0xFF801818) : UI_RED);
  snprintf(b, sizeof b, "%d", H.hp);
  d2_text_sh(FONT_BOLD, 28, hx + 50, hy + 12, b, UI_CREAM);
  float bw = 200, bx = hx + 116;
  d2_rrect(bx, hy + 18, bw, 14, 7, 0xFF2A2228);
  float f = (float)(H.hp > 0 ? H.hp : 0) / (H.mhp ? H.mhp : 100);
  if (f > 0) d2_grad(bx + 2, hy + 20, (bw - 4) * f, 10, lowhp ? 0xFFF05A4A : 0xFF8AD07A, lowhp ? 0xFF9A2020 : 0xFF3A8A4A);
  if (H.armor > 0) {
    d2_rrect(bx, hy + 40, bw, 8, 4, 0xFF2A2228);
    d2_grad(bx + 1, hy + 41, (bw - 2) * H.armor / 100.0f, 6, 0xFF9AC8F0, 0xFF4A7AB0);
    d2_text(FONT_SANS, 13, hx + 50, hy + 40, "Kamizelka", UI_BLUE);
  }
  /* broń i amunicja */
  float ax = UI_W - 300, ay = UI_H - 92;
  d2_shadow(ax, ay, 272, 64, 12, 14, 0x90000000);
  d2_rrect(ax, ay, 272, 64, 12, 0xC8121016);
  d2_rrect_line(ax, ay, 272, 64, 12, 1.5f, 0x70E8B84A);
  d2_text_sh(FONT_SERIF, 20, ax + 16, ay + 8, WPN_NAMES[H.weapon], UI_GOLD);
  if (H.weapon != WPN_FISTS) {
    snprintf(b, sizeof b, "%d", H.clip[H.weapon]);
    d2_text_r(FONT_BOLD, 32, ax + 196, ay + 18, b, C.reloadT ? UI_DIM : (H.clip[H.weapon] == 0 ? UI_RED : UI_CREAM));
    snprintf(b, sizeof b, "/ %d", H.ammo[H.weapon]);
    d2_text_sh(FONT_SANS, 20, ax + 202, ay + 28, b, UI_GREY);
    if (C.reloadT) d2_text_sh(FONT_SANS, 15, ax + 16, ay + 38, "Przeładowanie...", UI_GREY);
    else if (H.clip[H.weapon] == 0 && H.ammo[H.weapon] == 0) d2_text_sh(FONT_SANS, 15, ax + 16, ay + 38, "Brak amunicji", UI_RED);
  } else d2_text_sh(FONT_SANS, 15, ax + 16, ay + 36, "1-4 / kółko: zmiana broni", UI_DIM);
  /* pieniądze */
  snprintf(b, sizeof b, "$ %d", H.gold);
  float mw = d2_text_w(FONT_BOLD, 24, b) + 34;
  d2_rrect(UI_W - mw - 28, 24, mw, 42, 10, 0xB8121016);
  d2_text_sh(FONT_BOLD, 24, UI_W - mw - 11, 30, b, 0xFF9AE08A);
  /* minimapa */
  minimap(UI_W - 196, 78, 168);
  int en = world_enemies_alive();
  if (en > 0) {
    snprintf(b, sizeof b, "Wrogowie: %d", en);
    d2_text_r(FONT_BOLD, 18, UI_W - 30, 254, b, UI_RED);
  }
  /* cel misji */
  if (H.objective[0]) {
    char lines[3][256];
    int n = d2_wrap(FONT_SANS, 19, H.objective, 400, lines, 3);
    float h = 40 + n * 24;
    d2_shadow(28, 24, 440, h, 10, 12, 0x80000000);
    d2_rrect(28, 24, 440, h, 10, 0xB8121016);
    d2_rect(28, 32, 4, h - 16, UI_GOLD);
    d2_text(FONT_BOLD, 13, 46, 32, "CEL", UI_GOLD_D);
    for (int k = 0; k < n; k++) d2_text_sh(FONT_SANS, 19, 46, 52 + k * 24, lines[k], UI_CREAM);
  }
  if (W.hurtT > 0) d2_rect(0, 0, UI_W, UI_H, WITH_A(0x8A0A0A, W.hurtT * 4));
}
