/* Maszyna wirtualna skryptów fabularnych + okno dialogowe i menu wyborów. */
#include "engine.h"

enum { VS_IDLE, VS_RUN, VS_SAY, VS_MENU, VS_BATTLE, VS_FADE, VS_WAIT, VS_WALK, VS_SHOP };

int g_forceChoice = -1;
int g_forceQ[64], g_forceN, g_forceI;

static struct {
  int state, pc, sp, stack[8];
  int speaker;
  Ent *self;
  int wait;
  char lines[32][160];
  int nlines, page, shown, pageChars, hasText;
  char name[40];
  Sprite *portrait;
  int mFirst, mCount, mCur;
  Mover *walkM;
  int walkDir, walkLeft;
  int fadeTarget;
  int loseTarget;
  int steps;
} V;

#define DLG_X 6
#define DLG_H 58
#define DLG_Y (SCREEN_H - DLG_H - 4)
#define DLG_W (SCREEN_W - 12)
#define DLG_LINES 3

int script_running(void) { return V.state != VS_IDLE; }

void script_stop(void) {
  V.state = VS_IDLE;
  V.self = NULL;
  V.sp = 0;
  V.hasText = 0;
}

void script_start(int idx, Ent *self) {
  if (idx < 0 || idx >= S.nscripts) return;
  memset(&V, 0, sizeof V);
  V.pc = S.scripts[idx].start;
  V.speaker = -1;
  V.self = self;
  V.state = VS_RUN;
  V.loseTarget = -1;
}

static int vis_len(const char *s) {
  int n = 0;
  const char *p = s;
  while (*p) {
    if (p[0] == '{' && p[1] && p[2] == '}') { p += 3; continue; }
    utf8_next(&p);
    n++;
  }
  return n;
}

static void page_setup(void) {
  V.shown = 0;
  V.pageChars = 0;
  for (int i = V.page * DLG_LINES; i < V.nlines && i < (V.page + 1) * DLG_LINES; i++) V.pageChars += vis_len(V.lines[i]);
}

static void say(const char *text, int sp) {
  V.name[0] = 0;
  V.portrait = NULL;
  if (sp >= 0 && sp < S.nspeakers) {
    snprintf(V.name, sizeof V.name, "%s", S.speakers[sp].name);
    if (S.speakers[sp].sprite[0] && strcmp(S.speakers[sp].sprite, "-")) V.portrait = art_portrait(S.speakers[sp].sprite);
  }
  int width = V.portrait ? DLG_W - 58 : DLG_W - 16;
  V.nlines = text_wrap(text, width, V.lines, 32);
  if (V.nlines == 0) { V.nlines = 1; V.lines[0][0] = 0; }
  V.page = 0;
  V.hasText = 1;
  page_setup();
  V.state = VS_SAY;
}

static void toast_fmt(const char *fmt, const char *a, int n) {
  char b[160];
  snprintf(b, sizeof b, fmt, a, n);
  ui_toast(b);
}

static int is_stat(int var) {
  for (int i = 0; i < S.nstats; i++)
    if (!strcmp(S.stats[i].var, S.varnames[var])) return i;
  return -1;
}

static Mover *mover_by_id(const char *id) {
  if (!strcmp(id, "hero")) return &W.hero;
  Ent *e = world_find_ent(id);
  return e ? &e->m : NULL;
}

static void exec(void) {
  V.steps = 0;
  while (V.state == VS_RUN) {
    if (++V.steps > 2000) {
      fprintf(stderr, "skrypt: za dużo kroków (pętla?) w linii %d\n", S.cmds[V.pc].line);
      script_stop();
      return;
    }
    if (V.pc < 0 || V.pc >= S.ncmds) { script_stop(); return; }
    Cmd *c = &S.cmds[V.pc++];
    switch (c->op) {
      case OP_SAY: say(c->s[0], c->i[0] == -2 ? V.speaker : c->i[0]); break;
      case OP_AS: V.speaker = c->i[0]; break;
      case OP_MENU: {
        V.mFirst = c->i[0];
        V.mCount = c->i[1];
        V.mCur = 0;
        for (int i = 0; i < V.mCount; i++)
          if (cond_check(S.opts[V.mFirst + i].condType, S.opts[V.mFirst + i].condA, S.opts[V.mFirst + i].condB)) { V.mCur = i; break; }
        V.state = VS_MENU;
        break;
      }
      case OP_GOTO: V.pc = c->i[0]; break;
      case OP_IF: if (cond_check(c->i[0], c->i[1], c->i[2])) V.pc = c->i[3]; break;
      case OP_SET: H.vars[c->i[0]] = c->i[1]; break;
      case OP_ADD: {
        H.vars[c->i[0]] += c->i[1];
        int st = is_stat(c->i[0]);
        if (st >= 0 && c->i[1]) {
          char b[80];
          snprintf(b, sizeof b, "%s %+d", S.stats[st].label, c->i[1]);
          ui_toast(b);
        }
        break;
      }
      case OP_GIVE:
        H.inv[c->i[0]] += c->i[1];
        if (c->i[1] > 1) toast_fmt("Otrzymano: %s x%d", S.items[c->i[0]].name, c->i[1]);
        else toast_fmt("Otrzymano: %s", S.items[c->i[0]].name, 0);
        sfx_play(SFX_CHEST);
        break;
      case OP_TAKE:
        H.inv[c->i[0]] -= c->i[1];
        if (H.inv[c->i[0]] < 0) H.inv[c->i[0]] = 0;
        if (H.weapon == c->i[0] && !H.inv[c->i[0]]) H.weapon = -1;
        if (H.armor == c->i[0] && !H.inv[c->i[0]]) H.armor = -1;
        if (H.charm == c->i[0] && !H.inv[c->i[0]]) H.charm = -1;
        toast_fmt("Oddano: %s", S.items[c->i[0]].name, 0);
        break;
      case OP_EQUIP: {
        int it = c->i[0];
        if (H.inv[it] <= 0) H.inv[it] = 1;
        if (S.items[it].type == IT_WEAPON) H.weapon = it;
        else if (S.items[it].type == IT_ARMOR) H.armor = it;
        else if (S.items[it].type == IT_CHARM) H.charm = it;
        break;
      }
      case OP_CASH: {
        H.gold += c->i[0];
        if (H.gold < 0) H.gold = 0;
        char b[40];
        snprintf(b, sizeof b, "%+d $", c->i[0]);
        ui_toast(b);
        if (c->i[0] > 0) sfx_play(SFX_CHEST);
        break;
      }
      case OP_HEAL: H.hp = H.mhp; H.mp = H.mmp; sfx_play(SFX_HEAL); break;
      case OP_HURT: H.hp -= c->i[0]; if (H.hp < 1) H.hp = 1; gfx_shake(4); sfx_play(SFX_HIT); break;
      case OP_BATTLE:
        V.loseTarget = c->i[1];
        V.state = VS_BATTLE;
        battle_start(c->s[0], c->i[0], c->s[1] ? c->s[1] : S.maps[W.map].bg, 1);
        break;
      case OP_WARP: if (c->i[0] >= 0) world_load_map(c->i[0], c->i[1], c->i[2], c->i[3]); break;
      case OP_FADE: V.fadeTarget = c->i[0] ? 255 : 0; V.state = VS_FADE; break;
      case OP_WAIT: V.wait = c->i[0]; V.state = VS_WAIT; break;
      case OP_SFX: sfx_play(c->i[0]); break;
      case OP_MUSIC: music_play(strcmp(c->s[0], "-") ? c->s[0] : NULL); break;
      case OP_SHAKE: gfx_shake(c->i[0]); break;
      case OP_QUEST: quest_set(c->s[0], c->s[1]); break;
      case OP_DONE: quest_done(c->s[0]); break;
      case OP_SHOP: {
        const char *ids[MAX_ARGS];
        for (int i = 0; i < c->n; i++) ids[i] = S.items[c->i[i]].id;
        ui_open_shop(ids, c->n);
        V.state = VS_SHOP;
        break;
      }
      case OP_SAVE: ui_toast(save_game() ? "Gra zapisana" : "Nie udało się zapisać!"); break;
      case OP_ENDING: script_stop(); ui_ending_start(c->s[0], c->s[1]); return;
      case OP_LEARN:
        if (c->i[0] >= 0 && !H.skills[c->i[0]]) {
          H.skills[c->i[0]] = 1;
          toast_fmt("Nowa umiejętność: %s", SKILLS[c->i[0]].name, 0);
          sfx_play(SFX_LEVEL);
        }
        break;
      case OP_FACE: { Mover *m = mover_by_id(c->s[0]); if (m) m->dir = c->i[0]; break; }
      case OP_WALK: {
        Mover *m = mover_by_id(c->s[0]);
        if (!m) break;
        V.walkM = m; V.walkDir = c->i[0]; V.walkLeft = c->i[1];
        V.state = VS_WALK;
        break;
      }
      case OP_CALL:
        if (V.sp < 8) { V.stack[V.sp++] = V.pc; V.pc = S.scripts[c->i[0]].start; }
        break;
      case OP_TOAST: ui_toast(c->s[0]); break;
      case OP_XP: {
        char lv[6][160];
        toast_fmt("+%s%d doświadczenia", "", c->i[0]);
        int n = hero_gain_xp(c->i[0], lv, 6);
        for (int k = 0; k < n; k++) ui_toast(lv[k]);
        break;
      }
      case OP_CHANCE: if (rand() % 100 < c->i[0]) V.pc = c->i[1]; break;
      case OP_END:
        if (V.sp > 0) V.pc = V.stack[--V.sp];
        else { script_stop(); world_refresh_visibility(); return; }
        break;
      default: break;
    }
  }
  world_refresh_visibility();
}

void script_battle_done(int result) {
  if (V.state != VS_BATTLE) return;
  if (result == 2) { /* przegrana */
    if (V.loseTarget >= 0) {
      H.hp = H.mhp / 3 > 0 ? H.mhp / 3 : 1;
      V.pc = V.loseTarget;
      V.state = VS_RUN;
    } else {
      script_stop();
      game_set_mode(MODE_GAMEOVER);
    }
    return;
  }
  V.state = VS_RUN;
}

static int opt_enabled(int i) {
  MenuOpt *o = &S.opts[V.mFirst + i];
  return cond_check(o->condType, o->condA, o->condB);
}

void script_update(void) {
  switch (V.state) {
    case VS_IDLE: return;
    case VS_RUN: break;
    case VS_SAY:
      if (g_autoplay) { V.state = VS_RUN; break; }
      if (V.shown < V.pageChars) {
        int before = V.shown;
        V.shown += in.held[BTN_A] ? 3 : 1;
        if (V.shown / 3 != before / 3) sfx_play(SFX_TEXT);
        if (in.pressed[BTN_A] || in.pressed[BTN_B]) V.shown = V.pageChars;
        return;
      }
      if (in.pressed[BTN_A] || in.pressed[BTN_B]) {
        if ((V.page + 1) * DLG_LINES < V.nlines) { V.page++; page_setup(); sfx_play(SFX_BLIP); return; }
        /* jeśli następna komenda to menu, zostaw tekst widoczny */
        V.state = VS_RUN;
        if (!(V.pc < S.ncmds && S.cmds[V.pc].op == OP_MENU)) V.hasText = 0;
        break;
      }
      return;
    case VS_MENU: {
      int chosen = -1;
      if (g_autoplay) {
        if (g_forceI < g_forceN) {
          g_forceChoice = g_forceQ[g_forceI++];
          if (g_forceChoice >= V.mCount || !opt_enabled(g_forceChoice))
            printf("[test] UWAGA: wybór %d niedostępny w menu (linia %d)\n", g_forceChoice, S.cmds[V.pc - 1].line);
        }
        if (g_forceChoice >= 0 && g_forceChoice < V.mCount && opt_enabled(g_forceChoice)) chosen = g_forceChoice;
        else {
          int en[16], ne = 0;
          for (int i = 0; i < V.mCount && ne < 16; i++) if (opt_enabled(i)) en[ne++] = i;
          chosen = ne ? en[rand() % ne] : 0;
        }
        g_forceChoice = -1;
      } else {
        if (in.rep[BTN_UP]) { V.mCur = (V.mCur + V.mCount - 1) % V.mCount; sfx_play(SFX_BLIP); }
        if (in.rep[BTN_DOWN]) { V.mCur = (V.mCur + 1) % V.mCount; sfx_play(SFX_BLIP); }
        if (in.pressed[BTN_A]) {
          if (opt_enabled(V.mCur)) { chosen = V.mCur; sfx_play(SFX_OK); }
          else sfx_play(SFX_CANCEL);
        }
      }
      if (chosen >= 0) {
        V.pc = S.opts[V.mFirst + chosen].target;
        V.hasText = 0;
        V.state = VS_RUN;
        break;
      }
      return;
    }
    case VS_BATTLE: return;
    case VS_FADE:
      if (g_fade < V.fadeTarget) g_fade = g_fade + 17 > 255 ? 255 : g_fade + 17;
      else if (g_fade > V.fadeTarget) g_fade = g_fade - 17 < 0 ? 0 : g_fade - 17;
      if (g_fade == V.fadeTarget) V.state = VS_RUN;
      else return;
      break;
    case VS_WAIT:
      if (g_autoplay || --V.wait <= 0) V.state = VS_RUN;
      else return;
      break;
    case VS_WALK:
      if (V.walkM->moving) return;
      if (V.walkLeft > 0) {
        V.walkLeft--;
        mover_step(V.walkM, V.walkDir);
        return;
      }
      V.state = VS_RUN;
      break;
    case VS_SHOP:
      if (g_mode == MODE_SHOP) return;
      V.state = VS_RUN;
      break;
  }
  exec();
}

/* ---------------------------------------------------------------- rysowanie */
static void draw_dialog(int full) {
  gfx_window(DLG_X, DLG_Y, DLG_W, DLG_H);
  int tx = DLG_X + 8, ty = DLG_Y + 7;
  if (V.portrait) {
    gfx_rect(DLG_X + 6, DLG_Y + 6, 38, 38, pal('i'));
    gfx_rect(DLG_X + 7, DLG_Y + 7, 36, 36, RGB(0x3a, 0x34, 0x4c));
    gfx_blit(V.portrait, DLG_X + 9, DLG_Y + 9, 0, 2);
    tx = DLG_X + 52;
  }
  if (V.name[0]) { text_draw_sh(tx, ty - 2, V.name, pal('y')); ty += 10; }
  int remain = full ? 99999 : V.shown;
  for (int i = V.page * DLG_LINES; i < V.nlines && i < (V.page + 1) * DLG_LINES; i++) {
    int L = vis_len(V.lines[i]);
    text_draw_n(tx, ty, V.lines[i], pal('w'), remain);
    remain -= L;
    if (remain <= 0) break;
    ty += LINE_H;
  }
  if (!full && V.shown >= V.pageChars && (g_frame / 20) % 2)
    text_draw(DLG_X + DLG_W - 12, DLG_Y + DLG_H - 12, (V.page + 1) * DLG_LINES < V.nlines ? "\xe2\x96\xbc" : "\xe2\x96\xb6", pal('y'));
}

void script_draw(void) {
  if (V.state == VS_SAY) draw_dialog(0);
  if (V.state == VS_MENU) {
    if (V.hasText) draw_dialog(1);
    int w = 60;
    for (int i = 0; i < V.mCount; i++) {
      int tw = text_width(S.opts[V.mFirst + i].text) + 22;
      if (tw > w) w = tw;
    }
    if (w > SCREEN_W - 16) w = SCREEN_W - 16;
    int h = V.mCount * 12 + 8;
    int x = SCREEN_W - w - 8;
    int y = (V.hasText ? DLG_Y : SCREEN_H - 8) - h - 2;
    gfx_window(x, y, w, h);
    for (int i = 0; i < V.mCount; i++) {
      int en = opt_enabled(i);
      u32 col = !en ? pal('d') : (i == V.mCur ? pal('y') : pal('w'));
      if (i == V.mCur) {
        gfx_rect_a(x + 3, y + 4 + i * 12, w - 6, 12, pal('b'), 90);
        text_draw(x + 6, y + 5 + i * 12, "\xe2\x96\xb6", pal('y'));
      }
      text_draw_sh(x + 14, y + 5 + i * 12, S.opts[V.mFirst + i].text, col);
    }
  }
}
