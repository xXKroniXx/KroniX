/* Interfejs: menu (mysz + klawiatura), pauza (status/ekwipunek/dziennik/zapis/opcje), sklep,
 * ekran tytułowy z żywą sceną 3D, śmierć, zakończenia, powiadomienia. Styl: art déco. */
#include "engine.h"

/* ---------------------------------------------------------------- elementy */
void ui_panel(float x, float y, float w, float h) {
  d2_shadow(x, y, w, h, 14, 26, 0xB0000000);
  d2_grad(x, y, w, h, 0xF21A161C, 0xF20E0C10);
  d2_rrect_line(x, y, w, h, 12, 1.5f, 0x90E8B84A);
  d2_rrect_line(x + 6, y + 6, w - 12, h - 12, 8, 1.0f, 0x30E8B84A);
  /* narożne ornamenty */
  float o[4][2] = {{x + 6, y + 6}, {x + w - 6, y + 6}, {x + 6, y + h - 6}, {x + w - 6, y + h - 6}};
  for (int i = 0; i < 4; i++) {
    float cx = o[i][0], cy = o[i][1];
    d2_line(cx - 5, cy, cx, cy - 5, 2, UI_GOLD); d2_line(cx, cy - 5, cx + 5, cy, 2, UI_GOLD);
    d2_line(cx + 5, cy, cx, cy + 5, 2, UI_GOLD); d2_line(cx, cy + 5, cx - 5, cy, 2, UI_GOLD);
  }
}

void ui_button(float x, float y, float w, float h, const char *label, int sel, int en) {
  if (sel) {
    d2_shadow(x, y, w, h, 10, 12, 0x90000000);
    d2_grad(x, y, w, h, 0xF0362A1A, 0xF0201810);
    d2_rrect_line(x, y, w, h, 10, 2, UI_GOLD);
    d2_text(FONT_SANS, h * 0.42f, x + 16, y + h * 0.22f, "\xe2\x96\xb6", UI_GOLD);
  } else {
    d2_rrect(x, y, w, h, 10, 0x90141016);
    d2_rrect_line(x, y, w, h, 10, 1, 0x40E8B84A);
  }
  d2_text_sh(FONT_BOLD, h * 0.42f, x + 44, y + h * 0.24f, label, !en ? UI_DIM : sel ? 0xFFFFE6A8 : UI_CREAM);
}

/* ---------------------------------------------------------------- menu */
void menu_init(Menu *m) { memset(m, 0, sizeof *m); m->lmx = m->lmy = -9999; }
void menu_add(Menu *m, const char *label, int enabled) {
  if (m->n >= 16) return;
  m->items[m->n] = label;
  m->enabled[m->n] = enabled;
  m->n++;
}
static int menu_hit(Menu *m) {
  if (in.mx < 0 || m->rh <= 0) return -1;
  for (int i = 0; i < m->n; i++)
    if (in.mx >= m->dx && in.mx < m->dx + m->dw && in.my >= m->dy + i * m->rh && in.my < m->dy + i * m->rh + m->rh - 8) return i;
  return -1;
}
int menu_input(Menu *m) {
  if (!m->n) return -2;
  if (in.rep[BTN_UP]) { m->cur = (m->cur + m->n - 1) % m->n; sfx_play(SFX_BLIP); }
  if (in.rep[BTN_DOWN]) { m->cur = (m->cur + 1) % m->n; sfx_play(SFX_BLIP); }
  int h = menu_hit(m);
  if (h >= 0 && (in.mx != m->lmx || in.my != m->lmy) && h != m->cur) { m->cur = h; sfx_play(SFX_BLIP); }
  m->lmx = in.mx; m->lmy = in.my;
  if (in.pressed[BTN_A] || (in.click && h >= 0)) {
    if (in.click) m->cur = h;
    if (m->enabled[m->cur]) { sfx_play(SFX_OK); return m->cur; }
    sfx_play(SFX_CANCEL);
  }
  if (in.pressed[BTN_B]) { sfx_play(SFX_CANCEL); return -2; }
  return -1;
}
void menu_draw(Menu *m, float x, float y, float w) {
  m->dx = x; m->dy = y; m->dw = w; m->rh = 54;
  for (int i = 0; i < m->n; i++) ui_button(x, y + i * m->rh, w, m->rh - 8, m->items[i], i == m->cur, m->enabled[i]);
}

void ui_cursor(void) {
  if (in.mx < 0 || in.my < 0) return;
  float x = (float)in.mx, y = (float)in.my;
  d2_line(x, y, x, y + 20, 3.5f, 0xFF101010);
  d2_line(x, y, x + 14, y + 14, 3.5f, 0xFF101010);
  d2_line(x + 1, y + 2, x + 1, y + 17, 1.6f, 0xFFF0E6D0);
  d2_line(x + 1, y + 2, x + 12, y + 13, 1.6f, 0xFFF0E6D0);
}

/* ---------------------------------------------------------------- powiadomienia */
static char toastQ[6][120];
static int toastN, toastT;
void ui_toast(const char *s) {
  if (g_autoplay) return;
  if (toastN >= 6) return;
  snprintf(toastQ[toastN++], 120, "%s", s);
  if (toastN == 1) toastT = 150;
}
void ui_toast_draw(void) {
  if (!toastN) return;
  if (--toastT <= 0) {
    for (int i = 1; i < toastN; i++) memcpy(toastQ[i - 1], toastQ[i], 120);
    toastN--;
    toastT = 150;
    if (!toastN) return;
  }
  float in_ = toastT > 135 ? (150 - toastT) / 15.0f : toastT < 15 ? toastT / 15.0f : 1;
  float w = d2_text_w(FONT_BOLD, 20, toastQ[0]) + 60, x = (UI_W - w) / 2, y = 84 - (1 - in_) * 30;
  int a = (int)(in_ * 255);
  d2_shadow(x, y, w, 44, 10, 12, WITH_A(0, a * 3 / 5));
  d2_rrect(x, y, w, 44, 10, WITH_A(0x16121A, a * 9 / 10));
  d2_rrect_line(x, y, w, 44, 10, 1.5f, WITH_A(0xE8B84A, a * 3 / 4));
  d2_text(FONT_SANS, 18, x + 14, y + 11, "\xe2\x97\x8f", WITH_A(0xE8B84A, a));
  d2_text_sh(FONT_BOLD, 20, x + 38, y + 10, toastQ[0], WITH_A(0xF0E6D0, a));
}

/* ---------------------------------------------------------------- ustawienia */
int g_quality = 2;
static const char *QNAMES[4] = {"Niska", "Średnia", "Wysoka", "Ultra"};
void quality_apply(int q) {
  g_quality = q < 0 ? 0 : q > 3 ? 3 : q;
  static const int MS[4] = {0, 0, 4, 4}, BL[4] = {0, 1, 1, 1}, SC[4] = {60, 80, 100, 100}, LI[4] = {6, 10, 16, 16};
  g_cfg.msaa = MS[g_quality]; g_cfg.bloom = BL[g_quality]; g_cfg.scale = SC[g_quality]; g_cfg.maxLights = LI[g_quality];
  if (g_render) r_resize(g_winW, g_winH);
}
static void settings_path(char *p, int n) { snprintf(p, n, "%skronix_rodzina_ustawienia.txt", g_data_dir); }
void settings_save(void) {
  char p[600];
  settings_path(p, sizeof p);
  FILE *f = fopen(p, "w");
  if (!f) return;
  fprintf(f, "glosnosc %d\nmysz %d\njakosc %d\nfov %d\n", g_volume, g_mouse_sens, g_quality, g_cfg.fov);
  fclose(f);
}
void settings_load(void) {
  char p[600], k[32];
  int v;
  settings_path(p, sizeof p);
  FILE *f = fopen(p, "r");
  if (!f) return;
  while (fscanf(f, "%31s %d", k, &v) == 2) {
    if (!strcmp(k, "glosnosc")) g_volume = CLAMP(v, 0, 10);
    else if (!strcmp(k, "mysz")) g_mouse_sens = CLAMP(v, 1, 10);
    else if (!strcmp(k, "jakosc")) g_quality = CLAMP(v, 0, 3);
    else if (!strcmp(k, "fov")) g_cfg.fov = CLAMP(v, 60, 100);
  }
  fclose(f);
  quality_apply(g_quality);
}

/* ---------------------------------------------------------------- pauza */
static Menu pm;
static int pSub, pItemCur, pOptCur, pQuestScroll;
static int itemList[MAX_ITEMS], nItemList;
#define NOPTS 5

void ui_open_pause(void) {
  menu_init(&pm);
  menu_add(&pm, "Status", 1);
  menu_add(&pm, "Ekwipunek", 1);
  menu_add(&pm, "Dziennik", 1);
  menu_add(&pm, "Zapisz grę", !tycoon_mission_active() && !script_running());
  menu_add(&pm, "Opcje", 1);
  menu_add(&pm, "Menu główne", 1);
  menu_add(&pm, "Wróć do gry", 1);
  pSub = 0;
  game_set_mode(MODE_MENU);
  sfx_play(SFX_OK);
}

static void build_items(void) {
  nItemList = 0;
  for (int i = 0; i < S.nitems; i++) if (H.inv[i] > 0) itemList[nItemList++] = i;
  if (pItemCur >= nItemList) pItemCur = nItemList ? nItemList - 1 : 0;
}

static const int AMMO_PACK[WPN_COUNT] = {0, 14, 8, 50};
static void use_item(int it) {
  ItemDef *d = &S.items[it];
  switch (d->type) {
    case IT_HEAL: case IT_ELIXIR:
      if (H.hp >= H.mhp) { sfx_play(SFX_CANCEL); return; }
      H.hp = H.hp + d->power > H.mhp ? H.mhp : H.hp + d->power;
      H.inv[it]--;
      sfx_play(SFX_HEAL);
      break;
    case IT_AMMO:
      if (d->power <= 0 || d->power >= WPN_COUNT) return;
      H.ammo[d->power] += AMMO_PACK[d->power];
      H.inv[it]--;
      combat_top_up(d->power);
      sfx_play(SFX_RELOAD);
      break;
    default: sfx_play(SFX_CANCEL); break;
  }
  build_items();
}

static void opt_change(int d) {
  switch (pOptCur) {
    case 0: g_volume = CLAMP(g_volume + d, 0, 10); break;
    case 1: g_mouse_sens = CLAMP(g_mouse_sens + d, 1, 10); break;
    case 2: quality_apply(g_quality + d); break;
    case 3: g_cfg.fov = CLAMP(g_cfg.fov + d * 5, 60, 100); break;
    case 4: if (d) g_fullscreen_toggle = 1; break;
  }
  sfx_play(SFX_BLIP);
  settings_save();
}

void ui_pause_update(void) {
  if (pSub == 0) {
    int r = menu_input(&pm);
    if (r == -2 || r == 6) { game_set_mode(MODE_WORLD); return; }
    if (r == 1) { pSub = 1; pItemCur = 0; build_items(); }
    if (r == 2) { pSub = 2; pQuestScroll = 0; }
    if (r == 3) ui_toast(save_game() ? "Gra zapisana" : "Nie udało się zapisać!");
    if (r == 4) { pSub = 3; pOptCur = 0; }
    if (r == 5) { pSub = 4; }
  } else if (pSub == 1) {
    if (in.pressed[BTN_B]) { pSub = 0; sfx_play(SFX_CANCEL); return; }
    if (nItemList) {
      if (in.rep[BTN_UP]) { pItemCur = (pItemCur + nItemList - 1) % nItemList; sfx_play(SFX_BLIP); }
      if (in.rep[BTN_DOWN]) { pItemCur = (pItemCur + 1) % nItemList; sfx_play(SFX_BLIP); }
      if (in.pressed[BTN_A]) use_item(itemList[pItemCur]);
    }
  } else if (pSub == 2) {
    if (in.pressed[BTN_B] || in.pressed[BTN_A]) { pSub = 0; sfx_play(SFX_CANCEL); }
    if (in.rep[BTN_DOWN] || in.pressed[BTN_WNEXT]) pQuestScroll++;
    if (in.rep[BTN_UP] && pQuestScroll > 0) pQuestScroll--;
  } else if (pSub == 3) {
    if (in.pressed[BTN_B]) { pSub = 0; sfx_play(SFX_CANCEL); return; }
    if (in.rep[BTN_UP]) { pOptCur = (pOptCur + NOPTS - 1) % NOPTS; sfx_play(SFX_BLIP); }
    if (in.rep[BTN_DOWN]) { pOptCur = (pOptCur + 1) % NOPTS; sfx_play(SFX_BLIP); }
    /* mysz: wiersze opcji */
    for (int i = 0; i < NOPTS; i++) {
      float y = 168 + i * 58;
      if (in.mx >= 450 && in.mx < 1140 && in.my >= y && in.my < y + 48) {
        pOptCur = i;
        if (in.click) opt_change(in.mx > 900 ? 1 : -1);
      }
    }
    if (in.rep[BTN_LEFT]) opt_change(-1);
    if (in.rep[BTN_RIGHT]) opt_change(1);
    if (in.pressed[BTN_A] && pOptCur == 4) opt_change(1);
  } else if (pSub == 4) {
    if (in.pressed[BTN_B]) { pSub = 0; sfx_play(SFX_CANCEL); }
    if (in.pressed[BTN_A] || in.click) { ui_title_enter(); }
  }
}

static void bar_h(float x, float y, float w, float v, float max, u32 c0, u32 c1) {
  d2_rrect(x, y, w, 12, 6, 0xFF2A2228);
  float f = max > 0 ? v / max : 0;
  if (f > 1) f = 1;
  if (f > 0) d2_grad(x + 2, y + 2, (w - 4) * f, 8, c0, c1);
}

void ui_pause_draw(void) {
  d2_rect(0, 0, UI_W, UI_H, 0x9A08060A);
  d2_text_sh(FONT_SERIF, 54, 80, 34, "Pauza", UI_GOLD);
  menu_draw(&pm, 80, 120, 290);
  char b[200];
  long sec = H.playFrames / 60;
  snprintf(b, sizeof b, "Czas gry: %ld:%02ld  \xe2\x80\xa2  $ %d", sec / 3600, (sec / 60) % 60, H.gold);
  d2_text_sh(FONT_SANS, 18, 84, 520, b, UI_GREY);

  float px = 420, py = 100, pw = 780, ph = 520;
  ui_panel(px, py, pw, ph);
  float x = px + 40, y = py + 34;
  int view = pSub;
  if (pSub == 0) view = pm.cur == 1 ? 1 : pm.cur == 2 ? 2 : pm.cur == 4 ? 3 : 0;
  if (pSub == 4) view = 4;
  if (view == 0) {
    unsigned por = human_portrait(human_find("tomek"));
    d2_rrect(x, y, 200, 200, 12, 0xFF2A2430);
    if (por) d2_image(por, x + 4, y + 4, 192, 192, 0, 1, 1, 0, 0xFFFFFFFF);
    d2_rrect_line(x, y, 200, 200, 12, 2, UI_GOLD);
    float sx = x + 236;
    d2_text_sh(FONT_SERIF, 34, sx, y, "Tomasz Kowalski", UI_GOLD);
    d2_text_sh(FONT_SANS, 18, sx, y + 44, var_find("SZEF", 0) >= 0 && H.vars[var_find("SZEF", 0)] ? "Głowa rodziny Marino" : "Chłopak z Back of the Yards", UI_GREY);
    snprintf(b, sizeof b, "Zdrowie  %d / %d", H.hp, H.mhp);
    d2_text_sh(FONT_SANS, 18, sx, y + 84, b, UI_CREAM);
    bar_h(sx + 180, y + 89, 260, (float)H.hp, (float)H.mhp, 0xFF8AD07A, 0xFF3A8A4A);
    snprintf(b, sizeof b, "Kamizelka  %d", H.armor);
    d2_text_sh(FONT_SANS, 18, sx, y + 114, b, UI_CREAM);
    bar_h(sx + 180, y + 119, 260, (float)H.armor, 100, 0xFF9AC8F0, 0xFF4A7AB0);
    snprintf(b, sizeof b, "Gotówka  $ %d", H.gold);
    d2_text_sh(FONT_SANS, 18, sx, y + 144, b, 0xFF9AE08A);
    d2_text_sh(FONT_SERIF, 22, x, y + 226, "Broń", UI_GOLD);
    float wy = y + 260;
    for (int w = 0; w < WPN_COUNT; w++) {
      if (!H.hasWpn[w]) continue;
      if (w == WPN_FISTS) snprintf(b, sizeof b, "%s", WPN_NAMES[w]);
      else snprintf(b, sizeof b, "%s   %d + %d", WPN_NAMES[w], H.clip[w], H.ammo[w]);
      d2_text_sh(FONT_SANS, 18, x + 10, wy, b, H.weapon == w ? UI_GOLD : UI_CREAM);
      wy += 28;
    }
    d2_text_sh(FONT_SERIF, 22, x + 360, y + 226, "Reputacja", UI_GOLD);
    for (int i = 0; i < S.nstats; i++) {
      int v = var_find(S.stats[i].var, 0);
      int val = v >= 0 ? H.vars[v] : 0;
      float ry = y + 262 + i * 40;
      d2_text_sh(FONT_SANS, 17, x + 360, ry, S.stats[i].label, UI_CREAM);
      float bx = x + 540, bw = 160;
      d2_rrect(bx, ry + 4, bw, 12, 6, 0xFF2A2228);
      float c = bx + bw / 2, f = CLAMP(val, -100, 100) / 100.0f * bw / 2;
      if (f >= 0) d2_rect(c, ry + 6, f, 8, UI_GREEN); else d2_rect(c + f, ry + 6, -f, 8, UI_RED);
      d2_rect(c - 1, ry + 2, 2, 16, UI_CREAM);
    }
  } else if (view == 1) {
    d2_text_sh(FONT_SERIF, 30, x, y, "Ekwipunek", UI_GOLD);
    if (pSub != 1) build_items();
    if (!nItemList) d2_text_sh(FONT_SANS, 20, x, y + 60, "Pusto w kieszeniach.", UI_GREY);
    int first = pItemCur > 8 ? pItemCur - 8 : 0;
    for (int i = first; i < nItemList && i < first + 9; i++) {
      int it = itemList[i];
      float ry = y + 56 + (i - first) * 40;
      int sel = pSub == 1 && i == pItemCur;
      if (sel) d2_rrect(x - 10, ry - 6, pw - 60, 38, 8, 0xA0362A1A);
      d2_text_sh(FONT_SANS, 20, x + 6, ry, S.items[it].name, sel ? UI_GOLD : S.items[it].type == IT_KEY ? UI_BLUE : UI_CREAM);
      snprintf(b, sizeof b, "x%d", H.inv[it]);
      d2_text_r(FONT_BOLD, 20, px + pw - 50, ry, b, UI_GREY);
    }
    if (nItemList) {
      char lines[3][256];
      int n = d2_wrap(FONT_SANS, 18, S.items[itemList[pItemCur]].desc, pw - 80, lines, 3);
      d2_rect(x, py + ph - 96, pw - 80, 1, 0x60E8B84A);
      for (int k = 0; k < n; k++) d2_text_sh(FONT_SANS, 18, x, py + ph - 84 + k * 24, lines[k], UI_GREY);
      if (pSub == 1) d2_text_r(FONT_SANS, 15, px + pw - 40, py + ph - 34, "E: użyj   Esc: wróć", UI_DIM);
    }
  } else if (view == 2) {
    d2_text_sh(FONT_SERIF, 30, x, y, "Dziennik", UI_GOLD);
    int line = 0;
    float yy = y + 56;
    d2_clip(px + 10, y + 50, pw - 20, ph - 100);
    for (int pass = 0; pass < 2; pass++)
      for (int i = H.nq - 1; i >= 0; i--) {
        if (H.q[i].done != pass) continue;
        char lines[5][256];
        int n = d2_wrap(FONT_SANS, 19, H.q[i].text, pw - 120, lines, 5);
        for (int k = 0; k < n; k++, line++) {
          if (line < pQuestScroll) continue;
          if (k == 0) d2_text(FONT_SANS, 19, x, yy, pass ? "\xe2\x9c\x93" : "\xe2\x80\xa2", pass ? UI_DIM : UI_GOLD);
          d2_text_sh(FONT_SANS, 19, x + 26, yy, lines[k], pass ? UI_DIM : UI_CREAM);
          yy += 27;
        }
        yy += 10;
      }
    d2_noclip();
    if (!H.nq) d2_text_sh(FONT_SANS, 20, x, y + 60, "Brak wpisów.", UI_GREY);
  } else if (view == 3) {
    d2_text_sh(FONT_SERIF, 30, x, y, "Opcje", UI_GOLD);
    const char *names[NOPTS] = {"Głośność", "Czułość myszy", "Jakość grafiki", "Pole widzenia", "Pełny ekran (F11)"};
    for (int i = 0; i < NOPTS; i++) {
      float ry = 168 + i * 58;
      int sel = pSub == 3 && pOptCur == i;
      d2_rrect(450, ry, 690, 48, 10, sel ? 0xC0362A1A : 0x80141016);
      if (sel) d2_rrect_line(450, ry, 690, 48, 10, 1.5f, UI_GOLD);
      d2_text_sh(FONT_SANS, 20, 476, ry + 12, names[i], sel ? UI_GOLD : UI_CREAM);
      char v[64] = "";
      if (i == 0) snprintf(v, sizeof v, "%d", g_volume);
      if (i == 1) snprintf(v, sizeof v, "%d", g_mouse_sens);
      if (i == 2) snprintf(v, sizeof v, "%s", QNAMES[g_quality]);
      if (i == 3) snprintf(v, sizeof v, "%d°", g_cfg.fov);
      if (i == 4) snprintf(v, sizeof v, "przełącz");
      if (i <= 1) bar_h(760, ry + 18, 200, (float)(i == 0 ? g_volume : g_mouse_sens), 10, 0xFFE8C860, 0xFFB08A30);
      d2_text_c(FONT_BOLD, 20, 1060, ry + 12, v, UI_CREAM);
      d2_text(FONT_SANS, 20, 990, ry + 12, "\xe2\x86\x90", UI_DIM);
      d2_text(FONT_SANS, 20, 1112, ry + 12, "\xe2\x86\x92", UI_DIM);
    }
    d2_text_sh(FONT_SERIF, 20, x, 470, "Sterowanie", UI_GOLD);
    d2_text_sh(FONT_SANS, 16, x, 500, "WASD — ruch   Mysz — rozglądanie   Shift — bieg   E — rozmowa / drzwi", UI_GREY);
    d2_text_sh(FONT_SANS, 16, x, 524, "LPM — strzał   R — przeładowanie   1-4 / kółko — broń   Esc — menu", UI_GREY);
    d2_text_sh(FONT_SANS, 16, x, 548, "Tab — Księga Rodziny   N — koniec dnia   F11 / Alt+Enter — pełny ekran", UI_GREY);
  } else if (view == 4) {
    d2_text_sh(FONT_SERIF, 32, x, y + 40, "Wrócić do menu głównego?", UI_GOLD);
    d2_text_sh(FONT_SANS, 20, x, y + 100, "Niezapisany postęp przepadnie.", UI_CREAM);
    d2_text_sh(FONT_SANS, 18, x, y + 150, "Enter / klik — tak      Esc — nie", UI_GREY);
  }
}

/* ---------------------------------------------------------------- sklep */
static int shopIds[MAX_ARGS], shopN, shopCur;
void ui_open_shop(const char **ids, int n) {
  shopN = 0;
  for (int i = 0; i < n && shopN < MAX_ARGS; i++) {
    int it = item_find(ids[i]);
    if (it >= 0) shopIds[shopN++] = it;
  }
  shopCur = 0;
  game_set_mode(MODE_SHOP);
}
static void shop_buy(void) {
  ItemDef *d = &S.items[shopIds[shopCur]];
  if (H.gold >= d->price) {
    if (d->type == IT_WEAPON && H.hasWpn[d->power]) { H.ammo[d->power] += AMMO_PACK[d->power]; }
    else if (d->type == IT_WEAPON) { H.hasWpn[d->power] = 1; H.ammo[d->power] += AMMO_PACK[d->power]; H.weapon = d->power; combat_top_up(d->power); }
    else if (d->type == IT_ARMOR) { if (H.armor >= 100) { sfx_play(SFX_CANCEL); return; } H.armor = H.armor + d->power > 100 ? 100 : H.armor + d->power; }
    else H.inv[shopIds[shopCur]]++;
    H.gold -= d->price;
    sfx_play(SFX_CHEST);
  } else sfx_play(SFX_CANCEL);
}
void ui_shop_update(void) {
  if (g_autoplay || in.pressed[BTN_B] || !shopN) { game_set_mode(MODE_WORLD); sfx_play(SFX_CANCEL); return; }
  if (in.rep[BTN_UP]) { shopCur = (shopCur + shopN - 1) % shopN; sfx_play(SFX_BLIP); }
  if (in.rep[BTN_DOWN]) { shopCur = (shopCur + 1) % shopN; sfx_play(SFX_BLIP); }
  for (int i = 0; i < shopN; i++) {
    float ry = 186 + i * 44;
    if (in.mx >= 290 && in.mx < 990 && in.my >= ry - 6 && in.my < ry + 34) {
      if (shopCur != i && (in.mdx != 0 || in.mdy != 0 || 1)) shopCur = i;
      if (in.click) shop_buy();
    }
  }
  if (in.pressed[BTN_A]) shop_buy();
}
void ui_shop_draw(void) {
  float x = 270, y = 90, w = 740, h = 140 + shopN * 44 + 70;
  d2_rect(0, 0, UI_W, UI_H, 0x80000000);
  ui_panel(x, y, w, h);
  d2_text_sh(FONT_SERIF, 34, x + 30, y + 26, "Co podać?", UI_GOLD);
  char b[96];
  snprintf(b, sizeof b, "Masz: $ %d", H.gold);
  d2_text_r(FONT_BOLD, 22, x + w - 30, y + 34, b, 0xFF9AE08A);
  for (int i = 0; i < shopN; i++) {
    ItemDef *d = &S.items[shopIds[i]];
    float ry = 186 + i * 44;
    int sel = i == shopCur, can = H.gold >= d->price;
    if (sel) { d2_rrect(x + 20, ry - 6, w - 40, 40, 8, 0xC0362A1A); d2_rrect_line(x + 20, ry - 6, w - 40, 40, 8, 1.5f, UI_GOLD); }
    d2_text_sh(FONT_SANS, 20, x + 40, ry, d->name, !can ? UI_DIM : sel ? 0xFFFFE6A8 : UI_CREAM);
    if (d->type == IT_WEAPON) snprintf(b, sizeof b, H.hasWpn[d->power] ? "amunicja" : "nowa broń");
    else if (d->type == IT_ARMOR) snprintf(b, sizeof b, "masz %d", H.armor);
    else snprintf(b, sizeof b, "masz %d", H.inv[shopIds[i]]);
    d2_text_sh(FONT_SANS, 16, x + 470, ry + 3, b, UI_GREY);
    snprintf(b, sizeof b, "$%d", d->price);
    d2_text_r(FONT_BOLD, 20, x + w - 40, ry, b, can ? 0xFF9AE08A : UI_RED);
  }
  char lines[2][256];
  int n = d2_wrap(FONT_SANS, 18, S.items[shopIds[shopCur]].desc, w - 80, lines, 2);
  for (int k = 0; k < n; k++) d2_text_sh(FONT_SANS, 18, x + 40, y + h - 72 + k * 24, lines[k], UI_GREY);
  d2_text_r(FONT_SANS, 15, x + w - 30, y + h - 30, "E / klik: kup    Esc: wyjdź", UI_DIM);
}

/* ---------------------------------------------------------------- tytuł */
static Menu tm;
static int titleT;
static int titleMap = -1;
void ui_title_enter(void) {
  menu_init(&tm);
  menu_add(&tm, "Nowa gra", 1);
  menu_add(&tm, "Kontynuuj", save_exists());
  menu_add(&tm, "Opcje: jakość grafiki", 1);
  menu_add(&tm, "Pełny ekran", 1);
  menu_add(&tm, "Wyjście", 1);
  if (save_exists()) tm.cur = 1;
  titleT = 0;
  script_stop();
  titleMap = map_find("italia");
  if (titleMap < 0) titleMap = map_find(S.startMap);
  if (titleMap >= 0) world_load_map(titleMap, 1, 5, DIR_RIGHT);
  W.bannerT = 0;
  game_set_mode(MODE_TITLE);
  music_play("tytul");
  audio_ambience(AMB_STREET, 1.0f, 0);
}
void ui_title_update(void) {
  titleT++;
  for (int i = 0; i < W.n; i++) W.ents[i].animT += 1.0f / 60.0f;
  /* kamera dryfuje wzdłuż ulicy */
  MapDef *m = titleMap >= 0 ? &S.maps[titleMap] : NULL;
  if (m) {
    float t = titleT / 60.0f;
    float len = m->w - 6.0f;
    float u = fmodf(t * 0.35f, len * 2);
    if (u > len) u = len * 2 - u;
    W.px = 3 + u;
    W.pz = m->h * 0.5f + sinf(t * 0.2f) * 0.6f;
    W.yaw = 1.5708f + sinf(t * 0.13f) * 0.5f;
    W.pitch = 0.08f + sinf(t * 0.17f) * 0.05f;
  }
  int r = menu_input(&tm);
  if (r == 0) {
    hero_new();
    combat_init();
    tycoon_init();
    int mm = map_find(S.startMap);
    world_load_map(mm, S.startX, S.startY, S.startDir);
    game_set_mode(MODE_WORLD);
    g_fade = 255;
    g_autofade = S.startScript[0] ? 0 : 1;
    if (S.startScript[0]) script_start(script_find(S.startScript), NULL);
  } else if (r == 1) {
    if (load_game()) { if (g_mode != MODE_TYCOON) game_set_mode(MODE_WORLD); g_fade = 255; }
    else ui_toast("Nie udało się wczytać zapisu.");
  } else if (r == 2) { quality_apply((g_quality + 1) % 4); settings_save(); }
  else if (r == 3) g_fullscreen_toggle = 1;
  else if (r == 4) g_quit = 1;
}

void ui_title_draw(void) {
  float a = titleT < 90 ? titleT / 90.0f : 1;
  d2_hgrad(-200, 0, 1000, UI_H, 0xD0000000, 0x00000000);
  d2_grad(0, UI_H - 200, UI_W, 200, 0x00000000, 0xC0000000);
  int ia = (int)(a * 255);
  /* logo */
  d2_text(FONT_SERIF, 112, 84, 64, "KroniX", WITH_A(0x000000, ia / 2));
  d2_text(FONT_SERIF, 112, 80, 60, "KroniX", WITH_A(0xE8B84A, ia));
  d2_rect(84, 196, 420 * a, 2, WITH_A(0xE8B84A, ia));
  char rod[] = "R  O  D  Z  I  N  A";
  d2_text_sh(FONT_SERIF, 40, 86, 206, rod, WITH_A(0xF0E6D0, ia));
  d2_text_sh(FONT_SANS, 19, 88, 262, "Chicago, Illinois  \xe2\x80\xa2  1932", WITH_A(0xA9B4C2, ia));
  menu_draw(&tm, 84, 330, 380);
  char q[64];
  snprintf(q, sizeof q, "Grafika: %s", QNAMES[g_quality]);
  d2_text_sh(FONT_SANS, 15, 88, 620, q, UI_DIM);
  d2_text_r(FONT_SANS, 15, UI_W - 40, UI_H - 36, "Enter / klik — wybierz     Strzałki — ruch", UI_DIM);
}

/* ---------------------------------------------------------------- śmierć */
static Menu gm;
static int goT;
void ui_gameover_update(void) {
  if (goT == 0) {
    menu_init(&gm);
    menu_add(&gm, "Wczytaj zapis", save_exists());
    menu_add(&gm, "Menu główne", 1);
    if (!save_exists()) gm.cur = 1;
    music_play("-");
  }
  goT++;
  if (goT < 60) return;
  int r = menu_input(&gm);
  if (r == 0) { goT = 0; if (load_game()) { if (g_mode != MODE_TYCOON) game_set_mode(MODE_WORLD); g_fade = 255; } }
  else if (r == 1) { goT = 0; ui_title_enter(); }
}
void ui_gameover_draw(void) {
  int a = goT * 4 > 200 ? 200 : goT * 4;
  d2_rect(0, 0, UI_W, UI_H, WITH_A(0x2A0408, a));
  if (goT > 20) {
    int ta = (goT - 20) * 8 > 255 ? 255 : (goT - 20) * 8;
    d2_text_c(FONT_SERIF, 96, UI_W / 2, 170, "Nie żyjesz", WITH_A(0xE0605A, ta));
    d2_text_c(FONT_SANS, 22, UI_W / 2, 300, "W tym mieście drugiej szansy nie ma. Prawie.", WITH_A(0xF0E6D0, ta));
  }
  if (goT >= 60) menu_draw(&gm, UI_W / 2 - 170, 390, 340);
}

/* ---------------------------------------------------------------- zakończenie */
static char endTitle[80];
static char endText[2400];
static char endLines[48][256];
static int endN, endT;
void ui_ending_start(const char *title, const char *text) {
  snprintf(endTitle, sizeof endTitle, "%s", title);
  snprintf(endText, sizeof endText, "%s", text);
  endN = g_render ? d2_wrap(FONT_SANS, 24, text, 820, endLines, 48) : 1;
  endT = 0;
  game_set_mode(MODE_ENDING);
  music_play("koniec");
  audio_ambience(AMB_NONE, 0.4f, 1);
  H.vars[var_find("_UKONCZONO", 1)] = 1;
}
void ui_ending_update(void) {
  endT++;
  int total = 90 + endN * 50 + 200;
  if (g_autoplay && endT > 10) { ui_title_enter(); return; }
  if (endT > 120 && in.held[BTN_A]) endT += 3;
  if (endT > total && (in.pressed[BTN_A] || in.click)) ui_title_enter();
}
void ui_ending_draw(void) {
  d2_rect(0, 0, UI_W, UI_H, 0xFF06050A);
  int a = endT * 4 > 255 ? 255 : endT * 4;
  d2_text_c(FONT_SERIF, 64, UI_W / 2, 50, endTitle, WITH_A(0xE8B84A, a));
  d2_rect(UI_W / 2 - 200, 140, 400, 2, WITH_A(0xE8B84A, a));
  int shown = (endT - 60) / 50;
  if (shown >= endN) shown = endN - 1;
  float y0 = 180, maxY = UI_H - 120;
  if (y0 + shown * 34 > maxY) y0 -= y0 + shown * 34 - maxY;
  for (int i = 0; i < endN && i <= shown; i++) {
    float y = y0 + i * 34;
    if (y < 150) continue;
    int la = i == shown ? ((endT - 60) % 50) * 6 : 255;
    if (la > 255) la = 255;
    d2_text_sh(FONT_SANS, 24, 230, y, endLines[i], WITH_A(0xF0E6D0, la));
  }
  int total = 90 + endN * 50;
  if (endT > total + 200) d2_text_c(FONT_SANS, 18, UI_W / 2, UI_H - 50, "Enter — menu główne", UI_GREY);
}
