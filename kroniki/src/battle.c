/* Walka turowa: bohater kontra do 4 przeciwników. */
#include "engine.h"

enum { P_INTRO, P_CMD, P_SKILL, P_ITEM, P_TARGET, P_ROUND, P_END };
enum { A_ATTACK, A_SKILL, A_ITEM, A_GUARD, A_FLEE };

typedef struct { int def, hp, mhp, alive, flash, dying, atkMod, fled, shake; } Foe;
typedef struct { int x, y, t; char s[24]; u32 c; } Pop;

static struct {
  Foe f[4]; int nf;
  int phase, cmdCur, subCur, target;
  int noflee, fromScript, boss;
  char bg[24];
  char msg[12][160]; int nmsg, msgT;
  int guard, t, result, heroFlash;
  int queue[5], qn, qi;
  int act, actArg;
  int subIds[16], nsub;
  Pop pop[8];
  int slash, slashFoe;
} B;

static void msg(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#include <stdarg.h>
static void msg(const char *fmt, ...) {
  if (B.nmsg >= 12) return;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(B.msg[B.nmsg++], 160, fmt, ap);
  va_end(ap);
  if (B.nmsg == 1) B.msgT = 0;
}

static void popup(int x, int y, const char *s, u32 c) {
  for (int i = 0; i < 8; i++)
    if (B.pop[i].t <= 0) {
      B.pop[i].x = x; B.pop[i].y = y; B.pop[i].t = 40; B.pop[i].c = c;
      snprintf(B.pop[i].s, sizeof B.pop[i].s, "%s", s);
      return;
    }
}

static int foe_scale(int i) { return S.enemies[B.f[i].def].scale > 0 ? S.enemies[B.f[i].def].scale : 3; }
static int foe_x(int i) {
  int total = 0;
  for (int k = 0; k < B.nf; k++) total += 16 * foe_scale(k) + 12;
  int x = (SCREEN_W - total) / 2;
  for (int k = 0; k < i; k++) x += 16 * foe_scale(k) + 12;
  return x + 6;
}
static int foe_cx(int i) { return foe_x(i) + 8 * foe_scale(i); }

static int rnd(int a, int b) { return a + rand() % (b - a + 1); }

static int hero_has_gun(void) {
  if (H.weapon < 0) return 0;
  const char *id = S.items[H.weapon].id;
  return !strncmp(id, "rew", 3) || !strncmp(id, "tom", 3) || !strncmp(id, "pis", 3) || !strncmp(id, "strz", 4);
}

void battle_start(const char *list, int noflee, const char *bg, int fromScript) {
  memset(&B, 0, sizeof B);
  char tmp[128];
  snprintf(tmp, sizeof tmp, "%s", list ? list : "");
  for (char *e = strtok(tmp, ","); e && B.nf < 4; e = strtok(NULL, ",")) {
    int d = enemy_find(e);
    if (d < 0) continue;
    Foe *f = &B.f[B.nf++];
    f->def = d;
    f->hp = f->mhp = S.enemies[d].hp;
    f->alive = 1;
    f->atkMod = 100;
    if (S.enemies[d].boss) B.boss = 1;
  }
  B.noflee = noflee || B.boss;
  B.fromScript = fromScript;
  snprintf(B.bg, sizeof B.bg, "%s", bg && bg[0] ? bg : "street");
  if (B.nf == 0) { B.result = 1; B.phase = P_END; }
  char names[160] = "";
  for (int i = 0; i < B.nf; i++) {
    if (i) strncat(names, ", ", sizeof names - strlen(names) - 1);
    strncat(names, S.enemies[B.f[i].def].name, sizeof names - strlen(names) - 1);
  }
  msg("%s: %s!", B.nf > 1 ? "Na drodze stają" : "Na drodze staje", names);
  B.phase = P_INTRO;
  sfx_play(SFX_ENCOUNTER);
  music_play(B.boss ? "boss" : "walka");
  game_set_mode(MODE_BATTLE);
}

static int alive_count(void) {
  int n = 0;
  for (int i = 0; i < B.nf; i++) if (B.f[i].alive) n++;
  return n;
}
static int first_alive(void) {
  for (int i = 0; i < B.nf; i++) if (B.f[i].alive) return i;
  return 0;
}

static int phys_damage(int atk, int def, int pct) {
  int base = atk * pct / 100 * rnd(90, 110) / 100 - def * 6 / 10;
  return base < 1 ? 1 : base;
}

static void hurt_foe(int i, int dmg, int crit) {
  Foe *f = &B.f[i];
  f->hp -= dmg;
  f->flash = 12;
  f->shake = 8;
  char b[24];
  snprintf(b, sizeof b, crit ? "%d!" : "%d", dmg);
  popup(foe_cx(i), 60, b, crit ? pal('y') : pal('w'));
  B.slash = 12; B.slashFoe = i;
  if (f->hp <= 0) {
    f->hp = 0;
    f->alive = 0;
    f->dying = 24;
    msg("%s pada na ziemię!", S.enemies[f->def].name);
    sfx_play(SFX_DIE);
  }
}

static void hero_attack(int i, int pct) {
  int crit = rand() % 100 < 10;
  int dmg = phys_damage(hero_atk(), S.enemies[B.f[i].def].def, pct);
  if (crit) dmg = dmg * 16 / 10;
  sfx_play(hero_has_gun() ? SFX_FIRE : (crit ? SFX_CRIT : SFX_HIT));
  msg("%s%s: %d obrażeń.", crit ? "Trafienie krytyczne! " : "", S.enemies[B.f[i].def].name, dmg);
  hurt_foe(i, dmg, crit);
}

static void hero_turn(void) {
  int i = B.target;
  if (!B.f[i].alive) i = first_alive();
  switch (B.act) {
    case A_ATTACK:
      msg("Tomek %s!", hero_has_gun() ? "strzela" : "uderza");
      hero_attack(i, 100);
      break;
    case A_GUARD:
      msg("Tomek unosi gardę i łapie oddech.");
      H.mp = H.mp + 2 > H.mmp ? H.mmp : H.mp + 2;
      break;
    case A_FLEE:
      if (rand() % 100 < 55 + (H.spd - 5) * 3) {
        msg("Udało się zwiać!");
        sfx_play(SFX_FLEE);
        B.result = 3;
      } else msg("Nie ma dokąd uciec!");
      break;
    case A_ITEM: {
      ItemDef *it = &S.items[B.actArg];
      H.inv[B.actArg]--;
      int hp = 0, mp = 0;
      if (it->type == IT_HEAL) hp = it->power;
      else if (it->type == IT_MANA) mp = it->power;
      else if (it->type == IT_ELIXIR) { hp = it->power; mp = it->power / 5; }
      H.hp = H.hp + hp > H.mhp ? H.mhp : H.hp + hp;
      H.mp = H.mp + mp > H.mmp ? H.mmp : H.mp + mp;
      msg("Tomek używa: %s.", it->name);
      sfx_play(SFX_HEAL);
      break;
    }
    case A_SKILL: {
      int sk = B.actArg;
      H.mp -= SKILLS[sk].mp;
      msg("%s!", SKILLS[sk].name);
      if (sk == SK_FIRE) hero_attack(i, 185);
      else if (sk == SK_HEAL) {
        int h = H.mhp * 35 / 100 + H.lvl * 2;
        H.hp = H.hp + h > H.mhp ? H.mhp : H.hp + h;
        msg("Tomek odzyskuje %d zdrowia.", h);
        sfx_play(SFX_HEAL);
      } else if (sk == SK_WHIRL) {
        for (int k = 0; k < B.nf; k++) if (B.f[k].alive) hero_attack(k, 110);
      } else if (sk == SK_LIGHT) {
        sfx_play(SFX_MAGIC);
        gfx_shake(5);
        int any = 0;
        for (int k = 0; k < B.nf; k++) {
          Foe *f = &B.f[k];
          if (!f->alive) continue;
          EnemyDef *d = &S.enemies[f->def];
          if (!d->boss && f->hp * 100 / f->mhp < 40) {
            f->alive = 0; f->fled = 1; f->dying = 24;
            msg("%s ucieka w popłochu!", d->name);
            any = 1;
          } else {
            f->atkMod = f->atkMod * 3 / 4 < 50 ? 50 : f->atkMod * 3 / 4;
            msg("%s traci pewność siebie.", d->name);
            any = 1;
          }
        }
        (void)any;
      }
      break;
    }
  }
}

static void foe_turn(int i) {
  Foe *f = &B.f[i];
  if (!f->alive) return;
  EnemyDef *d = &S.enemies[f->def];
  int atk = d->atk * f->atkMod / 100;
  int useSkill = d->skill[0] && rand() % 100 < d->skillRate;
  int dmg = 0;
  if (useSkill) {
    msg("%s: %s!", d->name, d->skill);
    switch (d->skillType) {
      case ES_DMG: dmg = phys_damage(atk, hero_def(), d->skillPow); break;
      case ES_MAG: dmg = atk * d->skillPow / 100 * rnd(90, 110) / 100 - hero_def() / 4; break;
      case ES_DRAINHP:
        dmg = phys_damage(atk, hero_def(), d->skillPow);
        f->hp = f->hp + dmg / 2 > f->mhp ? f->mhp : f->hp + dmg / 2;
        break;
      case ES_DRAINMP: {
        int l = d->skillPow / 10 > H.mp ? H.mp : d->skillPow / 10;
        H.mp -= l;
        msg("Tomek traci zimną krew (-%d).", l);
        break;
      }
      case ES_HEAL: {
        int h = f->mhp * d->skillPow / 100;
        f->hp = f->hp + h > f->mhp ? f->mhp : f->hp + h;
        msg("%s odzyskuje siły.", d->name);
        sfx_play(SFX_HEAL);
        break;
      }
    }
  } else {
    dmg = phys_damage(atk, hero_def(), 100);
  }
  if (dmg > 0 || (useSkill && (d->skillType == ES_DMG || d->skillType == ES_MAG || d->skillType == ES_DRAINHP))) {
    if (dmg < 1) dmg = 1;
    if (B.guard) dmg = (dmg + 1) / 2;
    if (!useSkill) msg("%s atakuje: %d obrażeń.", d->name, dmg);
    else msg("Tomek traci %d zdrowia.", dmg);
    H.hp -= dmg;
    if (H.hp < 0) H.hp = 0;
    B.heroFlash = 14;
    gfx_shake(dmg > H.mhp / 5 ? 7 : 4);
    sfx_play(SFX_HIT);
    char b[16];
    snprintf(b, sizeof b, "-%d", dmg);
    popup(262, 150, b, pal('r'));
  }
}

static void begin_round(void) {
  B.qn = 0;
  B.queue[B.qn++] = -1; /* bohater */
  for (int i = 0; i < B.nf; i++) if (B.f[i].alive) B.queue[B.qn++] = i;
  int sp[5];
  for (int k = 0; k < B.qn; k++) sp[k] = (B.queue[k] < 0 ? H.spd : S.enemies[B.f[B.queue[k]].def].spd) * 10 + rand() % 25;
  if (B.act == A_GUARD || B.act == A_ITEM) sp[0] += 1000; /* garda i przedmioty zawsze pierwsze */
  for (int a = 0; a < B.qn; a++)
    for (int b = a + 1; b < B.qn; b++)
      if (sp[b] > sp[a]) { int t = sp[a]; sp[a] = sp[b]; sp[b] = t; t = B.queue[a]; B.queue[a] = B.queue[b]; B.queue[b] = t; }
  B.qi = 0;
  B.guard = B.act == A_GUARD;
  B.phase = P_ROUND;
}

static void finish_victory(void) {
  int xp = 0, gold = 0;
  for (int i = 0; i < B.nf; i++) {
    EnemyDef *d = &S.enemies[B.f[i].def];
    xp += B.f[i].fled ? d->xp / 2 : d->xp;
    if (!B.f[i].fled) gold += d->gold;
  }
  H.gold += gold;
  msg("Wygrana! +%d dośw., +%d $", xp, gold);
  for (int i = 0; i < B.nf; i++) {
    EnemyDef *d = &S.enemies[B.f[i].def];
    int it = item_find(d->drop);
    if (it >= 0 && !B.f[i].fled && rand() % 100 < d->dropRate) {
      H.inv[it]++;
      msg("Znaleziono: %s.", S.items[it].name);
    }
  }
  char lv[6][160];
  int n = hero_gain_xp(xp, lv, 6);
  for (int i = 0; i < n; i++) msg("%s", lv[i]);
  music_play("wygrana");
  B.result = 1;
}

static void end_battle(void) {
  int r = B.result;
  B.phase = P_END;
  game_set_mode(MODE_WORLD);
  if (r != 2 || B.fromScript) {
    MapDef *m = &S.maps[W.map];
    if (m->music[0]) music_play(m->music);
  }
  if (B.fromScript) script_battle_done(r);
  else {
    world_battle_done(r);
    if (r == 2) game_set_mode(MODE_GAMEOVER);
  }
}

/* ---------------------------------------------------------------- update */
static int msg_pending(void) {
  if (B.nmsg == 0) return 0;
  B.msgT++;
  int need = g_autoplay ? 1 : 50;
  if (B.msgT >= need || (B.msgT > 8 && (in.pressed[BTN_A] || in.held[BTN_A] && B.msgT > 20))) {
    for (int i = 1; i < B.nmsg; i++) memcpy(B.msg[i - 1], B.msg[i], 160);
    B.nmsg--;
    B.msgT = 0;
  }
  return 1;
}

static void build_sub(int kind) {
  B.nsub = 0;
  if (kind == P_SKILL) {
    for (int i = 0; i < SK_COUNT; i++) if (H.skills[i]) B.subIds[B.nsub++] = i;
  } else {
    for (int i = 0; i < S.nitems && B.nsub < 16; i++) {
      int t = S.items[i].type;
      if (H.inv[i] > 0 && (t == IT_HEAL || t == IT_MANA || t == IT_ELIXIR)) B.subIds[B.nsub++] = i;
    }
  }
  B.subCur = 0;
}

static void choose_target_or_go(int needsTarget) {
  if (needsTarget && alive_count() > 1 && !g_autoplay) {
    B.target = first_alive();
    B.phase = P_TARGET;
  } else {
    B.target = first_alive();
    begin_round();
  }
}

static void autoplay_pick(void) {
  /* prosta taktyka do testów automatycznych */
  if (H.hp < H.mhp / 3) {
    if (H.skills[SK_HEAL] && H.mp >= SKILLS[SK_HEAL].mp) { B.act = A_SKILL; B.actArg = SK_HEAL; begin_round(); return; }
    for (int i = 0; i < S.nitems; i++)
      if (H.inv[i] > 0 && S.items[i].type == IT_HEAL) { B.act = A_ITEM; B.actArg = i; begin_round(); return; }
  }
  if (alive_count() > 1 && H.skills[SK_WHIRL] && H.mp >= SKILLS[SK_WHIRL].mp) { B.act = A_SKILL; B.actArg = SK_WHIRL; begin_round(); return; }
  if (H.skills[SK_FIRE] && H.mp >= SKILLS[SK_FIRE].mp) { B.act = A_SKILL; B.actArg = SK_FIRE; choose_target_or_go(1); return; }
  B.act = A_ATTACK;
  choose_target_or_go(1);
}

void battle_update(void) {
  B.t++;
  for (int i = 0; i < B.nf; i++) {
    if (B.f[i].flash) B.f[i].flash--;
    if (B.f[i].shake) B.f[i].shake--;
    if (B.f[i].dying) B.f[i].dying--;
  }
  for (int i = 0; i < 8; i++) if (B.pop[i].t > 0) { B.pop[i].t--; if (B.pop[i].t % 2) B.pop[i].y--; }
  if (B.heroFlash) B.heroFlash--;
  if (B.slash) B.slash--;

  if (msg_pending()) return;

  switch (B.phase) {
    case P_INTRO:
      if (B.t > 20) B.phase = P_CMD;
      break;
    case P_CMD:
      if (g_autoplay) { autoplay_pick(); break; }
      if (in.rep[BTN_UP]) { B.cmdCur = (B.cmdCur + 4) % 5; sfx_play(SFX_BLIP); }
      if (in.rep[BTN_DOWN]) { B.cmdCur = (B.cmdCur + 1) % 5; sfx_play(SFX_BLIP); }
      if (in.pressed[BTN_A]) {
        sfx_play(SFX_OK);
        switch (B.cmdCur) {
          case 0: B.act = A_ATTACK; choose_target_or_go(1); break;
          case 1: build_sub(P_SKILL); if (B.nsub) B.phase = P_SKILL; else { msg("Nie znasz jeszcze żadnych sztuczek."); } break;
          case 2: build_sub(P_ITEM); if (B.nsub) B.phase = P_ITEM; else { msg("Nie masz nic przydatnego."); } break;
          case 3: B.act = A_GUARD; begin_round(); break;
          case 4:
            if (B.noflee) { msg("Tu nie ma ucieczki!"); sfx_play(SFX_CANCEL); }
            else { B.act = A_FLEE; begin_round(); }
            break;
        }
      }
      break;
    case P_SKILL: case P_ITEM:
      if (in.rep[BTN_UP]) { B.subCur = (B.subCur + B.nsub - 1) % B.nsub; sfx_play(SFX_BLIP); }
      if (in.rep[BTN_DOWN]) { B.subCur = (B.subCur + 1) % B.nsub; sfx_play(SFX_BLIP); }
      if (in.pressed[BTN_B]) { B.phase = P_CMD; sfx_play(SFX_CANCEL); break; }
      if (in.pressed[BTN_A]) {
        int id = B.subIds[B.subCur];
        if (B.phase == P_SKILL) {
          if (H.mp < SKILLS[id].mp) { sfx_play(SFX_CANCEL); break; }
          sfx_play(SFX_OK);
          B.act = A_SKILL; B.actArg = id;
          choose_target_or_go(id == SK_FIRE);
        } else {
          sfx_play(SFX_OK);
          B.act = A_ITEM; B.actArg = id;
          begin_round();
        }
      }
      break;
    case P_TARGET:
      if (in.rep[BTN_LEFT] || in.rep[BTN_UP]) {
        do B.target = (B.target + B.nf - 1) % B.nf; while (!B.f[B.target].alive);
        sfx_play(SFX_BLIP);
      }
      if (in.rep[BTN_RIGHT] || in.rep[BTN_DOWN]) {
        do B.target = (B.target + 1) % B.nf; while (!B.f[B.target].alive);
        sfx_play(SFX_BLIP);
      }
      if (in.pressed[BTN_B]) { B.phase = P_CMD; sfx_play(SFX_CANCEL); }
      if (in.pressed[BTN_A]) { sfx_play(SFX_OK); begin_round(); }
      break;
    case P_ROUND:
      if (B.result == 1 || B.result == 2 || B.result == 3) { B.phase = P_END; break; }
      if (B.qi >= B.qn) {
        H.mp = H.mp + 1 > H.mmp ? H.mmp : H.mp + 1;
        B.guard = 0;
        B.phase = P_CMD;
        break;
      }
      {
        int who = B.queue[B.qi++];
        if (who < 0) hero_turn();
        else foe_turn(who);
      }
      if (H.hp <= 0) { msg("Tomek pada na bruk..."); B.result = 2; sfx_play(SFX_DIE); }
      else if (B.result != 3 && alive_count() == 0) finish_victory();
      break;
    case P_END:
      end_battle();
      break;
  }
}

/* ---------------------------------------------------------------- rysowanie */
void battle_draw_bg(const char *bg, int t) {
  if (!strcmp(bg, "bar") || !strcmp(bg, "office")) {
    gfx_vgrad(0, 0, SCREEN_W, 120, RGB(0x2a, 0x18, 0x14), RGB(0x4a, 0x2c, 0x1c));
    for (int x = 0; x < SCREEN_W; x += 8) gfx_rect(x, 0, 4, 90, RGB(0x30, 0x1c, 0x16));
    for (int x = 20; x < SCREEN_W; x += 90) { gfx_circle(x, 30, 14, RGB(0xff, 0xc8, 0x70), 60); gfx_circle(x, 30, 4, pal('y'), 220); }
    gfx_rect(0, 90, SCREEN_W, 30, RGB(0x3a, 0x22, 0x14));
    for (int y = 120; y < SCREEN_H; y++) gfx_rect(0, y, SCREEN_W, 1, (y / 6) % 2 ? RGB(0x5a, 0x3a, 0x24) : RGB(0x4e, 0x32, 0x20));
  } else if (!strcmp(bg, "docks")) {
    gfx_vgrad(0, 0, SCREEN_W, 110, RGB(0x0c, 0x10, 0x22), RGB(0x1c, 0x2a, 0x44));
    gfx_circle(320, 26, 10, RGB(0xe8, 0xe8, 0xd0), 230);
    for (int i = 0; i < 4; i++) { int x = 30 + i * 100; gfx_rect(x, 30, 4, 80, pal('i')); gfx_rect(x, 30, 50, 4, pal('i')); gfx_rect(x + 46, 34, 1, 30, pal('i')); }
    gfx_rect(0, 100, SCREEN_W, 20, RGB(0x10, 0x20, 0x34));
    for (int i = 0; i < 20; i++) gfx_rect((i * 41 + t) % SCREEN_W, 104 + (i % 3) * 5, 10, 1, RGB(0x3a, 0x5a, 0x7a));
    for (int y = 120; y < SCREEN_H; y++) gfx_rect(0, y, SCREEN_W, 1, (y / 3) % 5 == 0 ? RGB(0x40, 0x2c, 0x1e) : RGB(0x6a, 0x4a, 0x32));
  } else if (!strcmp(bg, "ring")) {
    gfx_clear(RGB(0x10, 0x0c, 0x14));
    for (int i = 0; i < 60; i++) { int x = (i * 37) % SCREEN_W, y = 60 + (i * 13) % 40; gfx_circle(x, y, 6, RGB(0x2a, 0x22, 0x2e), 255); }
    gfx_circle(SCREEN_W / 2, 20, 70, RGB(0xff, 0xf0, 0xc0), 40);
    gfx_rect(0, 118, SCREEN_W, SCREEN_H - 118, RGB(0xd8, 0xcc, 0xb0));
    gfx_rect(0, 112, SCREEN_W, 2, pal('r')); gfx_rect(0, 106, SCREEN_W, 2, pal('w'));
  } else if (!strcmp(bg, "warehouse") || !strcmp(bg, "police")) {
    int pol = !strcmp(bg, "police");
    gfx_vgrad(0, 0, SCREEN_W, 120, pol ? RGB(0x3a, 0x40, 0x4a) : RGB(0x22, 0x20, 0x24), pol ? RGB(0x5a, 0x60, 0x6a) : RGB(0x3a, 0x34, 0x34));
    for (int x = 0; x < SCREEN_W; x += 64) { gfx_rect(x, 0, 6, 120, pal('i')); gfx_rect(x, 16, 64, 3, pal('e')); }
    if (!pol) for (int i = 0; i < 3; i++) { gfx_circle(60 + i * 130, 80, 30, RGB(0xa0, 0x5a, 0x30), 255); gfx_circle(52 + i * 130, 72, 10, RGB(0xe0, 0x9a, 0x5a), 160); }
    else for (int x = 30; x < SCREEN_W; x += 80) gfx_rect(x, 40, 40, 30, RGB(0x2a, 0x3a, 0x5a));
    gfx_rect(0, 118, SCREEN_W, SCREEN_H - 118, pol ? RGB(0x6a, 0x6a, 0x72) : RGB(0x4a, 0x48, 0x4e));
  } else { /* street */
    gfx_vgrad(0, 0, SCREEN_W, 110, RGB(0x0a, 0x0c, 0x1e), RGB(0x24, 0x22, 0x3e));
    for (int i = 0; i < 9; i++) {
      int x = i * 46 - 10, h = 50 + (i * 37) % 45;
      gfx_rect(x, 110 - h, 42, h, RGB(0x16, 0x14, 0x22));
      for (int wy = 110 - h + 6; wy < 104; wy += 10)
        for (int wx = x + 5; wx < x + 38; wx += 9)
          if (((wx * 7 + wy * 3 + i) % 5) < 2) gfx_rect(wx, wy, 4, 5, RGB(0xe0, 0xb8, 0x60));
    }
    gfx_rect(0, 110, SCREEN_W, 8, RGB(0x6a, 0x6a, 0x72));
    gfx_rect(0, 118, SCREEN_W, SCREEN_H - 118, RGB(0x2e, 0x30, 0x3a));
    for (int x = 0; x < SCREEN_W; x += 40) gfx_rect(x, 160, 20, 2, RGB(0x8a, 0x80, 0x50));
    gfx_circle(60, 70, 36, RGB(0xff, 0xd8, 0x90), 30);
    gfx_rect(58, 60, 3, 58, pal('i'));
    gfx_circle(59, 58, 4, pal('y'), 255);
  }
}

static void bar(int x, int y, int w, int val, int max, u32 col) {
  gfx_rect(x, y, w, 5, pal('i'));
  int f = max > 0 ? (w - 2) * (val < 0 ? 0 : val) / max : 0;
  gfx_rect(x + 1, y + 1, f, 3, col);
}

void battle_draw(void) {
  battle_draw_bg(B.bg, B.t);
  /* przeciwnicy */
  for (int i = 0; i < B.nf; i++) {
    Foe *f = &B.f[i];
    if (!f->alive && !f->dying) continue;
    EnemyDef *d = &S.enemies[f->def];
    int sc = foe_scale(i);
    CharSprite *cs = art_char(d->sprite);
    Sprite *s = cs ? cs->fr[DIR_DOWN][(B.t / 30 + i) % 2 ? 0 : 0] : art_get(d->sprite);
    int x = foe_x(i) + (f->shake ? ((f->shake % 2) ? 3 : -3) : 0);
    int y = 124 - 16 * sc + ((B.t / 20 + i) % 2);
    int alpha = f->alive ? 255 : f->dying * 10;
    gfx_rect_a(x + 2 * sc, 122, 12 * sc, 3, pal('i'), 120);
    gfx_blit_fx(s, x, y, 0, sc, f->flash ? pal('w') : d->tint, f->flash ? 200 : d->tintAmt, alpha);
    if (f->alive) {
      bar(foe_x(i) + 4 * sc, 127, 8 * sc, f->hp, f->mhp, pal('r'));
      if (B.phase == P_TARGET && B.target == i && (B.t / 8) % 2)
        text_draw_sh(foe_cx(i) - 3, y - 12, "\xe2\x96\xbc", pal('y'));
    }
  }
  if (B.slash) {
    int cx = foe_cx(B.slashFoe), cy = 90;
    if (hero_has_gun()) { gfx_circle(cx, cy, B.slash, pal('y'), 160); gfx_circle(cx, cy, B.slash / 2, pal('w'), 220); }
    else for (int k = -1; k <= 1; k++) for (int j = 0; j < 20; j++) gfx_pset(cx - 10 + j, cy - 10 + j + k * 3, pal('w'));
  }
  for (int i = 0; i < 8; i++)
    if (B.pop[i].t > 0) text_draw_big(B.pop[i].x - text_width(B.pop[i].s), B.pop[i].y, B.pop[i].s, B.pop[i].c, 2);

  /* komunikat */
  if (B.nmsg > 0) {
    gfx_window(6, 4, SCREEN_W - 12, 20);
    text_draw_sh(12, 8, B.msg[0], pal('w'));
  }

  /* menu komend */
  int my = 136;
  gfx_window(6, my, 118, SCREEN_H - my - 4);
  static const char *cmds[5] = {"Atak", "Sztuczki", "Przedmioty", "Garda", "Ucieczka"};
  for (int i = 0; i < 5; i++) {
    u32 c = (i == 4 && B.noflee) ? pal('d') : (B.phase == P_CMD && i == B.cmdCur ? pal('y') : pal('w'));
    if (B.phase == P_CMD && i == B.cmdCur) text_draw(12, my + 6 + i * 14, "\xe2\x96\xb6", pal('y'));
    text_draw_sh(20, my + 6 + i * 14, cmds[i], c);
  }
  /* status bohatera */
  int sx = 130;
  gfx_window(sx, my, SCREEN_W - sx - 6, SCREEN_H - my - 4);
  if (B.heroFlash) gfx_rect_a(sx + 2, my + 2, SCREEN_W - sx - 10, SCREEN_H - my - 8, pal('r'), B.heroFlash * 8);
  char b[64];
  snprintf(b, sizeof b, "Tomek   Poz. %d", H.lvl);
  text_draw_sh(sx + 8, my + 6, b, pal('y'));
  text_draw_sh(sx + 8, my + 22, "\xe2\x99\xa5 Zdrowie", pal('w'));
  snprintf(b, sizeof b, "%d/%d", H.hp, H.mhp);
  text_draw_sh(sx + 150, my + 22, b, H.hp < H.mhp / 4 ? pal('r') : pal('w'));
  bar(sx + 8, my + 34, 230, H.hp, H.mhp, pal('g'));
  text_draw_sh(sx + 8, my + 44, "Zimna krew", pal('w'));
  snprintf(b, sizeof b, "%d/%d", H.mp, H.mmp);
  text_draw_sh(sx + 150, my + 44, b, pal('w'));
  bar(sx + 8, my + 56, 230, H.mp, H.mmp, pal('c'));
  if (B.guard) text_draw_sh(sx + 190, my + 6, "GARDA", pal('c'));

  /* podmenu */
  if (B.phase == P_SKILL || B.phase == P_ITEM) {
    int w = 220, h = B.nsub * 12 + 22, x = 40, y = my - h + 20;
    gfx_window(x, y, w, h);
    for (int i = 0; i < B.nsub; i++) {
      int id = B.subIds[i];
      char line[96];
      u32 c = pal('w');
      if (B.phase == P_SKILL) {
        snprintf(line, sizeof line, "%s (%d ZK)", SKILLS[id].name, SKILLS[id].mp);
        if (H.mp < SKILLS[id].mp) c = pal('d');
      } else snprintf(line, sizeof line, "%s x%d", S.items[id].name, H.inv[id]);
      if (i == B.subCur) { c = c == pal('d') ? c : pal('y'); text_draw(x + 6, y + 5 + i * 12, "\xe2\x96\xb6", pal('y')); }
      text_draw_sh(x + 14, y + 5 + i * 12, line, c);
    }
    int id = B.subIds[B.subCur];
    const char *desc = B.phase == P_SKILL ? SKILLS[id].desc : S.items[id].desc;
    char lines[1][160];
    text_wrap(desc, w - 12, lines, 1);
    text_draw(x + 6, y + h - 13, lines[0], pal('s'));
  }
  if (B.t < 16) gfx_rect_a(0, 0, SCREEN_W, SCREEN_H, pal('w'), (16 - B.t) * 14);
}
