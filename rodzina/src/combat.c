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

typedef struct { float x, y, z; int t; u32 c; } Spark;
static Spark sparks[48];

void combat_init(void) { memset(&C, 0, sizeof C); memset(sparks, 0, sizeof sparks); }

static void spark(float x, float y, float z, u32 c) {
  for (int i = 0; i < 48; i++)
    if (sparks[i].t <= 0) { sparks[i] = (Spark){x, y, z, 10, c}; return; }
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
    spark(W.px + dx * ht, oy + dy * ht, W.pz + dz * ht, pal('r'));
  } else if (wt < range) {
    spark(W.px + dx * wt, oy + dy * wt, W.pz + dz * wt, pal('y'));
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
    r3d_ambient_flash(0.35f);
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
  if (C.flashT > 0) { if (--C.flashT == 0) r3d_ambient_flash(0); }
  if (C.hitMark > 0) C.hitMark--;
  if (C.punchT > 0) C.punchT--;
  if (C.swapT > 0) C.swapT--;
  for (int i = 0; i < 48; i++) if (sparks[i].t > 0) sparks[i].t--;
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

/* ---------------------------------------------------------------- widok broni i HUD */
static void hand(int x, int y, int w, int h) {
  gfx_rect(x, y, w, h, RGB(0xd8, 0xa8, 0x80));
  gfx_rect(x, y, w, 2, RGB(0xf0, 0xc8, 0xa0));
  gfx_rect(x, y + h - 2, w, 2, RGB(0xa8, 0x78, 0x58));
  gfx_rect(x - 2, y + h, w + 4, 30, RGB(0x4a, 0x42, 0x3a)); /* rękaw */
}

void combat_draw_view(void) {
  if (g_mode != MODE_WORLD) return;
  int bx = (int)(sinf(W.bobT) * 6), by = (int)(fabsf(cosf(W.bobT)) * 5);
  int ky = C.kick * 2, rl = 0;
  if (C.reloadT > 0) { int t = C.reloadT, tot = WD[H.weapon].reload; rl = (int)(sinf((float)t / tot * PI_F) * 60); }
  if (C.swapT > 0) rl += C.swapT * 6;
  int cx = SCREEN_W / 2 + bx, base = SCREEN_H + by + ky + rl;
  u32 metal = RGB(0x2a, 0x2a, 0x32), metal2 = RGB(0x46, 0x46, 0x52), wood = RGB(0x7a, 0x4a, 0x2a), wood2 = RGB(0x5a, 0x34, 0x1c);
  switch (H.weapon) {
    case WPN_FISTS: {
      int p = C.punchT > 6 ? (14 - C.punchT) * 9 : C.punchT * 9;
      hand(cx - 130, base - 46, 34, 28);
      hand(cx + 70 - p / 2, base - 46 - p, 36 + p / 4, 30 + p / 6);
      break;
    }
    case WPN_PISTOL:
      hand(cx + 34, base - 48, 26, 30);
      gfx_rect(cx + 30, base - 74, 22, 44, metal);
      gfx_rect(cx + 32, base - 92, 14, 22, metal2);
      gfx_rect(cx + 32, base - 92, 14, 3, RGB(0x6a, 0x6a, 0x76));
      gfx_rect(cx + 37, base - 96, 4, 4, metal);
      break;
    case WPN_SHOTGUN:
      hand(cx + 50, base - 40, 30, 26);
      gfx_rect(cx + 30, base - 60, 40, 60, wood);
      gfx_rect(cx + 30, base - 60, 6, 60, wood2);
      gfx_rect(cx + 20, base - 120, 16, 66, metal);
      gfx_rect(cx + 36, base - 116, 10, 60, metal2);
      gfx_rect(cx + 16, base - 82, 32, 14, wood);
      break;
    case WPN_TOMMY:
      hand(cx - 60, base - 54, 30, 24);
      hand(cx + 60, base - 40, 28, 26);
      gfx_rect(cx - 20, base - 70, 90, 30, metal);
      gfx_rect(cx - 30, base - 104, 14, 50, metal2);
      gfx_circle(cx + 8, base - 40, 22, metal2, 255);
      gfx_circle(cx + 8, base - 40, 8, metal, 255);
      gfx_rect(cx - 60, base - 64, 40, 18, wood);
      gfx_rect(cx + 50, base - 52, 26, 50, wood);
      gfx_rect(cx + 50, base - 52, 6, 50, wood2);
      break;
  }
  if (C.flashT > 0 && H.weapon != WPN_FISTS) {
    int fx = H.weapon == WPN_TOMMY ? cx - 23 : H.weapon == WPN_SHOTGUN ? cx + 28 : cx + 39;
    int fy = H.weapon == WPN_TOMMY ? base - 110 : H.weapon == WPN_SHOTGUN ? base - 126 : base - 100;
    gfx_circle(fx, fy, 16, pal('o'), 160);
    gfx_circle(fx, fy, 9, pal('y'), 230);
    gfx_circle(fx, fy, 4, pal('w'), 255);
  }
}

void combat_draw_hud(void) {
  /* iskry trafień */
  for (int i = 0; i < 48; i++) {
    if (sparks[i].t <= 0) continue;
    float sx, sy, dp;
    if (!r3d_project(sparks[i].x, sparks[i].y, sparks[i].z, &sx, &sy, &dp)) continue;
    int r = sparks[i].t > 6 ? 2 : 1;
    gfx_rect((int)sx - r, (int)sy - r, r * 2, r * 2, sparks[i].c);
  }
  if (g_mode != MODE_WORLD) return;
  int cx = SCREEN_W / 2, cy = SCREEN_H / 2;
  u32 ch = C.aimEnemy ? pal('r') : pal('w');
  gfx_rect(cx - 6, cy, 4, 1, ch); gfx_rect(cx + 3, cy, 4, 1, ch);
  gfx_rect(cx, cy - 6, 1, 4, ch); gfx_rect(cx, cy + 3, 1, 4, ch);
  if (C.hitMark > 0) for (int k = 2; k < 6; k++) { gfx_pset(cx - k, cy - k, pal('y')); gfx_pset(cx + k, cy - k, pal('y')); gfx_pset(cx - k, cy + k, pal('y')); gfx_pset(cx + k, cy + k, pal('y')); }
  /* zdrowie */
  char b[64];
  gfx_rect_a(6, SCREEN_H - 24, 128, 18, pal('i'), 170);
  snprintf(b, sizeof b, "\xe2\x99\xa5 %d", H.hp);
  text_draw_sh(11, SCREEN_H - 21, b, H.hp < 30 ? pal('r') : pal('w'));
  gfx_rect(52, SCREEN_H - 18, 76, 6, pal('i'));
  gfx_rect(53, SCREEN_H - 17, 74 * (H.hp > 0 ? H.hp : 0) / (H.mhp ? H.mhp : 100), 4, H.hp < 30 ? pal('r') : pal('g'));
  if (H.armor > 0) { snprintf(b, sizeof b, "Kamizelka %d", H.armor); text_draw_sh(11, SCREEN_H - 36, b, pal('c')); }
  /* amunicja */
  if (H.weapon != WPN_FISTS) snprintf(b, sizeof b, "%s  %d / %d", WPN_NAMES[H.weapon], H.clip[H.weapon], H.ammo[H.weapon]);
  else snprintf(b, sizeof b, "%s", WPN_NAMES[H.weapon]);
  int w = text_width(b) + 12;
  gfx_rect_a(SCREEN_W - w - 6, SCREEN_H - 24, w, 18, pal('i'), 170);
  text_draw_sh(SCREEN_W - w, SCREEN_H - 21, b, C.reloadT ? pal('d') : pal('y'));
  if (C.reloadT) text_draw_sh(SCREEN_W - w, SCREEN_H - 36, "Przeładowanie...", pal('s'));
  /* pieniądze i cel */
  snprintf(b, sizeof b, "$ %d", H.gold);
  w = text_width(b) + 10;
  gfx_rect_a(SCREEN_W - w - 6, 6, w, 14, pal('i'), 160);
  text_draw_sh(SCREEN_W - w - 1, 8, b, pal('l'));
  if (H.objective[0] && !script_blocking()) {
    char lines[2][160];
    int n = text_wrap(H.objective, 240, lines, 2);
    gfx_rect_a(6, 6, 250, 6 + n * LINE_H, pal('i'), 150);
    for (int k = 0; k < n; k++) text_draw_sh(10, 8 + k * LINE_H, lines[k], k ? pal('s') : pal('y'));
  }
  int en = world_enemies_alive();
  if (en > 0) {
    snprintf(b, sizeof b, "Wrogowie: %d", en);
    text_draw_sh(SCREEN_W - text_width(b) - 8, 24, b, pal('r'));
  }
}
