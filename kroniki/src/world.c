/* Świat gry: mapa kafelkowa, ruch postaci, NPC, zbiry na ulicach, drzwi,
 * interakcja, kamera, nocne oświetlenie i deszcz. */
#include "engine.h"

World W;

static const int DX[4] = {0, -1, 1, 0};
static const int DY[4] = {1, 0, 0, -1};

static MapDef *M(void) { return &S.maps[W.map]; }

static int tile_at(int tx, int ty) {
  MapDef *m = M();
  if (tx < 0 || ty < 0 || tx >= m->w || ty >= m->h) return 'x';
  return m->tiles[ty][tx];
}

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
    e->vis = !e->dead && ent_cond(e->d);
    if (e->d->type == ENT_EVENT && e->d->once) {
      int v = event_var(e->d);
      if (v >= 0 && H.vars[v]) e->vis = 0;
    }
  }
}

Ent *world_find_ent(const char *id) {
  for (int i = 0; i < W.n; i++)
    if (!strcmp(W.ents[i].d->id, id)) return &W.ents[i];
  return NULL;
}

static void mover_place(Mover *m, int tx, int ty, int dir) {
  memset(m, 0, sizeof *m);
  m->tx = tx; m->ty = ty; m->dir = dir;
  m->px = tx * TILE; m->py = ty * TILE;
  m->speed = 2;
}

void mover_step(Mover *m, int dir) {
  m->dir = dir;
  m->tx += DX[dir];
  m->ty += DY[dir];
  m->moving = 1;
  m->prog = 0;
}

void mover_update(Mover *m) {
  if (!m->moving) return;
  int sp = m->speed > 0 ? m->speed : 1;
  m->px += DX[m->dir] * sp;
  m->py += DY[m->dir] * sp;
  m->prog += sp;
  if (m->prog >= TILE) {
    m->px = m->tx * TILE;
    m->py = m->ty * TILE;
    m->moving = 0;
    m->step++;
  }
}

static int mover_frame(const Mover *m) {
  if (!m->moving) return 0;
  return m->prog < TILE / 2 ? 1 + (m->step & 1) : 0;
}

void world_load_map(int map, int tx, int ty, int dir) {
  if (map < 0 || map >= S.nmaps) return;
  int changed = map != W.map || W.n == 0;
  W.map = map;
  W.n = 0;
  MapDef *m = M();
  for (int i = 0; i < m->nents && W.n < MAX_ENTS; i++) {
    Ent *e = &W.ents[W.n++];
    memset(e, 0, sizeof *e);
    e->d = &m->ents[i];
    mover_place(&e->m, e->d->x, e->d->y, e->d->dir);
    e->m.speed = e->d->type == ENT_MOB ? 1 : 1;
    e->wanderT = 60 + rand() % 120;
  }
  mover_place(&W.hero, tx, ty, dir);
  W.grace = 45;
  if (changed) W.bannerT = 160;
  if (m->music[0]) music_play(m->music);
  world_refresh_visibility();
  int cx = tx * TILE + 8 - SCREEN_W / 2, cy = ty * TILE + 8 - SCREEN_H / 2;
  W.camx = cx; W.camy = cy;
}

static int ent_blocks(const Ent *e) {
  return e->vis && (e->d->type == ENT_NPC || e->d->type == ENT_OBJ || e->d->type == ENT_MOB);
}

int world_blocked(int tx, int ty, const Ent *self) {
  if (tile_solid(tile_at(tx, ty))) return 1;
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (e == self || !ent_blocks(e)) continue;
    if (e->m.tx == tx && e->m.ty == ty) return 1;
  }
  if (self && W.hero.tx == tx && W.hero.ty == ty) return 1;
  return 0;
}

/* Kto stoi przed bohaterem (z obsługą rozmowy przez ladę/biurko). */
static Ent *talk_target(void) {
  int fx = W.hero.tx + DX[W.hero.dir], fy = W.hero.ty + DY[W.hero.dir];
  for (int pass = 0; pass < 2; pass++) {
    for (int i = 0; i < W.n; i++) {
      Ent *e = &W.ents[i];
      if (!e->vis || e->d->script < 0) continue;
      if (e->d->type != ENT_NPC && e->d->type != ENT_OBJ) continue;
      if (e->m.tx == fx && e->m.ty == fy) return e;
    }
    int t = tile_at(fx, fy);
    if (t != 'b' && t != 'd' && t != 't') break;
    fx += DX[W.hero.dir]; fy += DY[W.hero.dir];
  }
  return NULL;
}

static void start_warp(EntDef *d) {
  W.trans = 1;
  W.tMap = d->toMap; W.tX = d->toX; W.tY = d->toY; W.tDir = d->toDir;
  sfx_play(SFX_DOOR);
}

static void arrive(void) {
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis || e->m.tx != W.hero.tx || e->m.ty != W.hero.ty) continue;
    if (e->d->type == ENT_WARP && e->d->toMap >= 0) { start_warp(e->d); return; }
    if (e->d->type == ENT_EVENT && e->d->script >= 0) {
      if (e->d->once) { int v = event_var(e->d); if (v >= 0) H.vars[v] = 1; }
      script_start(e->d->script, NULL);
      return;
    }
  }
}

static void update_hero(void) {
  Mover *h = &W.hero;
  int wasMoving = h->moving;
  mover_update(h);
  if (wasMoving && !h->moving) { arrive(); if (W.trans || script_running()) return; }
  if (h->moving) return;

  if (in.pressed[BTN_A]) {
    Ent *t = talk_target();
    if (t) {
      if (t->d->type == ENT_NPC) t->m.dir = 3 - h->dir;
      script_start(t->d->script, t);
      return;
    }
  }
  if (in.pressed[BTN_B]) { ui_open_pause(); return; }

  static const int order[4] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};
  static const int dirOf[4] = {DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT};
  for (int k = 0; k < 4; k++) {
    int b = order[k];
    if (!in.held[b]) continue;
    int d = dirOf[k];
    if (h->dir != d && in.holdT[b] < 5) { h->dir = d; return; }
    h->dir = d;
    if (!world_blocked(h->tx + DX[d], h->ty + DY[d], NULL)) {
      h->speed = in.held[BTN_RUN] ? 4 : 2;
      mover_step(h, d);
    }
    return;
  }
}

static void update_ents(void) {
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis) continue;
    mover_update(&e->m);
    if (e->m.moving) continue;
    int type = e->d->type;
    if (type == ENT_MOB) {
      int dx = W.hero.tx - e->m.tx, dy = W.hero.ty - e->m.ty;
      int dist = abs(dx) + abs(dy);
      if (dist <= 1 && W.grace == 0 && !W.trans && !W.hero.moving && !script_running() && g_mode == MODE_WORLD) {
        e->m.dir = dx > 0 ? DIR_RIGHT : dx < 0 ? DIR_LEFT : dy > 0 ? DIR_DOWN : DIR_UP;
        W.battleMob = i;
        battle_start(e->d->enemies, 0, M()->bg, 0);
        return;
      }
      e->busy = dist <= 5;
      if (e->busy && --e->wanderT <= 0) {
        e->wanderT = 14;
        int d1 = abs(dx) > abs(dy) ? (dx > 0 ? DIR_RIGHT : DIR_LEFT) : (dy > 0 ? DIR_DOWN : DIR_UP);
        int d2 = abs(dx) > abs(dy) ? (dy > 0 ? DIR_DOWN : DIR_UP) : (dx > 0 ? DIR_RIGHT : DIR_LEFT);
        if (!world_blocked(e->m.tx + DX[d1], e->m.ty + DY[d1], e)) mover_step(&e->m, d1);
        else if ((dx && dy) && !world_blocked(e->m.tx + DX[d2], e->m.ty + DY[d2], e)) mover_step(&e->m, d2);
        else e->m.dir = d1;
        continue;
      }
    }
    if ((e->d->wander || type == ENT_MOB) && !e->busy && !script_running()) {
      if (--e->wanderT > 0) continue;
      e->wanderT = 60 + rand() % 140;
      int d = rand() % 4;
      int nx = e->m.tx + DX[d], ny = e->m.ty + DY[d];
      if (abs(nx - e->d->x) > 2 || abs(ny - e->d->y) > 2) { e->m.dir = d; continue; }
      if (!world_blocked(nx, ny, e)) mover_step(&e->m, d);
      else e->m.dir = d;
    }
  }
}

void world_battle_done(int result) {
  if (W.battleMob < 0 || W.battleMob >= W.n) return;
  Ent *e = &W.ents[W.battleMob];
  if (result == 1) { e->dead = 1; e->vis = 0; }
  else if (result == 3) {
    W.grace = 150;
    /* zbir cofa się o krok */
    int d = 3 - e->m.dir;
    if (!world_blocked(e->m.tx + DX[d], e->m.ty + DY[d], e)) mover_step(&e->m, d);
  }
  W.battleMob = -1;
}

void world_update(void) {
  W.time++;
  if (W.grace > 0) W.grace--;
  if (W.bannerT > 0) W.bannerT--;

  if (W.trans == 1) { /* ściemnianie przed zmianą mapy */
    g_fade += 24;
    if (g_fade >= 255) {
      g_fade = 255;
      world_load_map(W.tMap, W.tX, W.tY, W.tDir);
      W.trans = 2;
    }
    return;
  }
  if (W.trans == 2) {
    g_fade -= 24;
    if (g_fade <= 0) { g_fade = 0; W.trans = 0; }
  }

  if (script_running()) {
    script_update();
    for (int i = 0; i < W.n; i++) mover_update(&W.ents[i].m);
    mover_update(&W.hero);
  } else if (g_mode == MODE_WORLD) {
    update_hero();
    if (g_mode == MODE_WORLD) update_ents();
  }

  MapDef *m = M();
  int tx = W.hero.px + 8 - SCREEN_W / 2, ty = W.hero.py + 8 - SCREEN_H / 2;
  if (m->w * TILE <= SCREEN_W) tx = (m->w * TILE - SCREEN_W) / 2;
  else tx = CLAMP(tx, 0, m->w * TILE - SCREEN_W);
  if (m->h * TILE <= SCREEN_H) ty = (m->h * TILE - SCREEN_H) / 2;
  else ty = CLAMP(ty, 0, m->h * TILE - SCREEN_H);
  W.camx = tx; W.camy = ty;
}

/* ---------------------------------------------------------------- rysowanie */
static u32 orig[SCREEN_W * SCREEN_H];

static void light(int cx, int cy, int r, int strength) {
  int r2 = r * r;
  for (int y = cy - r; y <= cy + r; y++) {
    if (y < 0 || y >= SCREEN_H) continue;
    for (int x = cx - r; x <= cx + r; x++) {
      if (x < 0 || x >= SCREEN_W) continue;
      int d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
      if (d2 >= r2) continue;
      int f = strength * (r2 - d2) / r2;
      u32 o = orig[y * SCREEN_W + x];
      u32 warm = blend(o, RGB(0xff, 0xd8, 0x90), 40);
      fb[y * SCREEN_W + x] = blend(fb[y * SCREEN_W + x], warm, f > 255 ? 255 : f);
    }
  }
}

static int same_group(int a, int b, const char *group) {
  return strchr(group, a) && strchr(group, b);
}

static void draw_edges(int x, int y, int sx, int sy, int t) {
  int up = tile_at(x, y - 1), dn = tile_at(x, y + 1), lf = tile_at(x - 1, y), rt = tile_at(x + 1, y);
  if (t == 'y') { /* ring: liny */
    u32 rope = pal('r'), rope2 = pal('w');
    if (!same_group(t, up, "yY")) { gfx_rect(sx, sy + 1, 16, 1, rope); gfx_rect(sx, sy + 3, 16, 1, rope2); }
    if (!same_group(t, dn, "yY")) { gfx_rect(sx, sy + 14, 16, 1, rope); gfx_rect(sx, sy + 12, 16, 1, rope2); }
    if (!same_group(t, lf, "yY")) { gfx_rect(sx + 1, sy, 1, 16, rope); gfx_rect(sx + 3, sy, 1, 16, rope2); }
    if (!same_group(t, rt, "yY")) { gfx_rect(sx + 14, sy, 1, 16, rope); gfx_rect(sx + 12, sy, 1, 16, rope2); }
  } else if (t == 'R') {
    if (dn != 'R') { gfx_rect(sx, sy + 13, 16, 3, RGB(0x6a, 0x62, 0x66)); gfx_rect(sx, sy + 13, 16, 1, RGB(0x8a, 0x82, 0x86)); }
    if (lf != 'R') gfx_rect(sx, sy, 2, 16, RGB(0x2a, 0x24, 0x28));
    if (rt != 'R') gfx_rect(sx + 14, sy, 2, 16, RGB(0x2a, 0x24, 0x28));
    if (up != 'R') gfx_rect(sx, sy, 16, 2, RGB(0x5a, 0x54, 0x58));
  } else if (t == 'r') {
    u32 g = pal('z');
    if (up != 'r') gfx_rect(sx, sy, 16, 1, g);
    if (dn != 'r') gfx_rect(sx, sy + 15, 16, 1, g);
    if (lf != 'r') gfx_rect(sx, sy, 1, 16, g);
    if (rt != 'r') gfx_rect(sx + 15, sy, 1, 16, g);
  } else if (t == ',' || t == 'L' || t == 'F') {
    const char *road = ".-|cC";
    u32 curb = RGB(0x5a, 0x5a, 0x64);
    if (strchr(road, dn)) gfx_rect(sx, sy + 14, 16, 2, curb);
    if (strchr(road, up)) gfx_rect(sx, sy, 16, 2, curb);
    if (strchr(road, lf)) gfx_rect(sx, sy, 2, 16, curb);
    if (strchr(road, rt)) gfx_rect(sx + 14, sy, 2, 16, curb);
  } else if (t == '~') {
    u32 foam = RGB(0x8a, 0xb0, 0xc8);
    if (up != '~') gfx_rect(sx, sy, 16, 1, foam);
    if (lf != '~') gfx_rect(sx, sy, 1, 16, foam);
    if (rt != '~') gfx_rect(sx + 15, sy, 1, 16, foam);
  }
}

static void draw_ent_sprite(Ent *e) {
  int sx = e->m.px - W.camx, sy = e->m.py - W.camy;
  if (sx < -32 || sy < -32 || sx > SCREEN_W + 16 || sy > SCREEN_H + 16) return;
  CharSprite *cs = art_char(e->d->sprite);
  if (cs) {
    gfx_rect_a(sx + 3, sy + 14, 10, 2, pal('i'), 110);
    gfx_blit(cs->fr[e->m.dir][mover_frame(&e->m)], sx, sy - 2, 0, 1);
    if (e->d->type == ENT_MOB && e->busy && (W.time / 10) % 2)
      text_draw_sh(sx + 7, sy - 14, "!", pal('r'));
  } else {
    Sprite *s = art_get(e->d->sprite);
    if (s) gfx_blit(s, sx, sy, 0, 1);
  }
}

void world_draw(void) {
  MapDef *m = M();
  int frame = (W.time / 16) % 4;
  int x0 = W.camx / TILE - 1, y0 = W.camy / TILE - 1;
  int x1 = (W.camx + SCREEN_W) / TILE + 1, y1 = (W.camy + SCREEN_H) / TILE + 1;
  gfx_clear(pal('i'));
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++) {
      int t = tile_at(x, y);
      int sx = x * TILE - W.camx, sy = y * TILE - W.camy;
      u32 h = (u32)(x * 73856093) ^ (u32)(y * 19349663);
      gfx_blit(art_tile(t, (int)(h % 4), frame), sx, sy, 0, 1);
      draw_edges(x, y, sx, sy, t);
    }

  /* obiekty i postacie posortowane po y */
  Ent *list[MAX_ENTS + 1];
  int n = 0;
  for (int i = 0; i < W.n; i++) {
    Ent *e = &W.ents[i];
    if (!e->vis || e->d->type == ENT_WARP || e->d->type == ENT_EVENT) continue;
    list[n++] = e;
  }
  for (int i = 1; i < n; i++) {
    Ent *k = list[i];
    int j = i - 1;
    while (j >= 0 && list[j]->m.py > k->m.py) { list[j + 1] = list[j]; j--; }
    list[j + 1] = k;
  }
  CharSprite *hero = art_char("tomek");
  int heroDrawn = 0;
  for (int i = 0; i <= n; i++) {
    if (!heroDrawn && (i == n || list[i]->m.py > W.hero.py)) {
      int sx = W.hero.px - W.camx, sy = W.hero.py - W.camy;
      gfx_rect_a(sx + 3, sy + 14, 10, 2, pal('i'), 110);
      if (hero) gfx_blit(hero->fr[W.hero.dir][mover_frame(&W.hero)], sx, sy - 2, 0, 1);
      heroDrawn = 1;
    }
    if (i < n) draw_ent_sprite(list[i]);
  }

  /* dymek nad rozmówcą */
  if (!script_running() && !W.hero.moving && g_mode == MODE_WORLD) {
    Ent *t = talk_target();
    if (t) {
      int sx = t->m.px - W.camx + 9, sy = t->m.py - W.camy - 13 + ((W.time / 15) % 2);
      gfx_rect(sx, sy, 9, 7, pal('i'));
      gfx_rect(sx + 1, sy + 1, 7, 5, pal('w'));
      gfx_pset(sx + 2, sy + 7, pal('i'));
      gfx_pset(sx + 2, sy + 3, pal('i')); gfx_pset(sx + 4, sy + 3, pal('i')); gfx_pset(sx + 6, sy + 3, pal('i'));
    }
  }

  /* noc: przyciemnienie + światła */
  if (m->night > 0) {
    memcpy(orig, fb, sizeof orig);
    gfx_darken(m->night);
    for (int y = y0; y <= y1; y++)
      for (int x = x0; x <= x1; x++) {
        int t = tile_at(x, y);
        int sx = x * TILE - W.camx + 8, sy = y * TILE - W.camy + 8;
        u32 h = (u32)(x * 73856093) ^ (u32)(y * 19349663);
        if (t == 'L') light(sx, sy - 4, 44, 230);
        else if (t == 'w' && (h % 4) % 2) light(sx, sy + 6, 20, 150);
        else if (t == 'f') light(sx, sy, 36, 200);
        else if (t == 'G') light(sx, sy + 6, 18, 120);
      }
    light(W.hero.px - W.camx + 8, W.hero.py - W.camy + 6, 30, 140);
  }
  if (m->rain) {
    for (int i = 0; i < 90; i++) {
      u32 hsh = (u32)(i * 2654435761u);
      int x = (int)((hsh % 420) + (W.time * 3)) % 420 - 20;
      int y = (int)(((hsh >> 9) % 240) + W.time * 7) % 240 - 12;
      for (int k = 0; k < 6; k++) gfx_pset_a(x - k / 2, y + k, RGB(0xa0, 0xb8, 0xd8), 110);
    }
  }

  /* baner z nazwą miejsca */
  if (W.bannerT > 0 && m->name[0]) {
    int a = W.bannerT > 130 ? (160 - W.bannerT) * 8 : W.bannerT < 30 ? W.bannerT * 8 : 240;
    int w = text_width(m->name) * 2 + 24;
    int x = (SCREEN_W - w) / 2;
    gfx_rect_a(x, 14, w, 28, pal('i'), a > 200 ? 200 : a);
    gfx_rect_a(x, 14, w, 1, pal('z'), a);
    gfx_rect_a(x, 41, w, 1, pal('z'), a);
    if (a > 120) text_draw_big(x + 12, 17, m->name, pal('y'), 2);
  }

  /* HUD: pieniądze i bieżące zadanie */
  if (!script_running() && g_mode == MODE_WORLD && W.bannerT == 0) {
    char b[32];
    snprintf(b, sizeof b, "$ %d", H.gold);
    int w = text_width(b) + 10;
    gfx_rect_a(SCREEN_W - w - 4, 4, w, 13, pal('i'), 170);
    text_draw_sh(SCREEN_W - w + 1, 5, b, pal('l'));
    for (int i = H.nq - 1; i >= 0; i--)
      if (!H.q[i].done) {
        char lines[2][160];
        int nl = text_wrap(H.q[i].text, 200, lines, 2);
        gfx_rect_a(4, 4, 208, 5 + nl * LINE_H, pal('i'), 150);
        for (int k = 0; k < nl; k++) text_draw_sh(8, 5 + k * LINE_H, lines[k], k ? pal('s') : pal('w'));
        break;
      }
  }
}
