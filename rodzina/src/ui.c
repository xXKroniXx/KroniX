/* Interfejs: menu, pauza (status/ekwipunek/dziennik/zapis/opcje), sklep,
 * ekran tytułowy, koniec gry, zakończenia, powiadomienia. */
#include "engine.h"

/* ---------------------------------------------------------------- menu */
void menu_init(Menu *m) { memset(m, 0, sizeof *m); }
void menu_add(Menu *m, const char *label, int enabled) {
  if (m->n >= 16) return;
  m->items[m->n] = label;
  m->enabled[m->n] = enabled;
  m->n++;
}
int menu_input(Menu *m) {
  if (!m->n) return -2;
  if (in.rep[BTN_UP]) { m->cur = (m->cur + m->n - 1) % m->n; sfx_play(SFX_BLIP); }
  if (in.rep[BTN_DOWN]) { m->cur = (m->cur + 1) % m->n; sfx_play(SFX_BLIP); }
  if (in.pressed[BTN_A]) {
    if (m->enabled[m->cur]) { sfx_play(SFX_OK); return m->cur; }
    sfx_play(SFX_CANCEL);
  }
  if (in.pressed[BTN_B]) { sfx_play(SFX_CANCEL); return -2; }
  return -1;
}
void menu_draw(Menu *m, int x, int y, int w) {
  gfx_window(x, y, w, m->n * 13 + 8);
  for (int i = 0; i < m->n; i++) {
    u32 c = !m->enabled[i] ? pal('d') : i == m->cur ? pal('y') : pal('w');
    if (i == m->cur) {
      gfx_rect_a(x + 3, y + 4 + i * 13, w - 6, 13, pal('b'), 80);
      text_draw(x + 6, y + 5 + i * 13, "\xe2\x96\xb6", pal('y'));
    }
    text_draw_sh(x + 15, y + 5 + i * 13, m->items[i], c);
  }
}

/* ---------------------------------------------------------------- toast */
static char toastQ[6][120];
static int toastN, toastT;
void ui_toast(const char *s) {
  if (g_autoplay) return;
  if (toastN >= 6) return;
  snprintf(toastQ[toastN++], 120, "%s", s);
  if (toastN == 1) toastT = 100;
}
void ui_toast_draw(void) {
  if (!toastN) return;
  if (--toastT <= 0) {
    for (int i = 1; i < toastN; i++) memcpy(toastQ[i - 1], toastQ[i], 120);
    toastN--;
    toastT = 100;
    if (!toastN) return;
  }
  int w = text_width(toastQ[0]) + 16;
  int x = (SCREEN_W - w) / 2, y = 66;
  int a = toastT > 90 ? (100 - toastT) * 20 : toastT < 12 ? toastT * 18 : 220;
  gfx_rect_a(x, y, w, 15, pal('i'), a);
  gfx_rect_a(x, y + 14, w, 1, pal('z'), a);
  if (a > 100) text_draw_sh(x + 8, y + 2, toastQ[0], pal('y'));
}

/* ---------------------------------------------------------------- pauza */
static Menu pm;
static int pSub, pItemCur, pOptCur, pQuestScroll;
static int itemList[MAX_ITEMS], nItemList;

void ui_open_pause(void) {
  menu_init(&pm);
  menu_add(&pm, "Status", 1);
  menu_add(&pm, "Ekwipunek", 1);
  menu_add(&pm, "Dziennik", 1);
  menu_add(&pm, "Zapisz grę", !tycoon_mission_active() && !script_running());
  menu_add(&pm, "Opcje", 1);
  menu_add(&pm, "Menu główne", 1);
  menu_add(&pm, "Wróć", 1);
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
      sfx_play(SFX_RELOAD);
      break;
    default: sfx_play(SFX_CANCEL); break;
  }
  build_items();
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
    if (in.rep[BTN_DOWN]) pQuestScroll++;
    if (in.rep[BTN_UP] && pQuestScroll > 0) pQuestScroll--;
  } else if (pSub == 3) {
    if (in.pressed[BTN_B]) { pSub = 0; sfx_play(SFX_CANCEL); return; }
    if (in.rep[BTN_UP]) { pOptCur = (pOptCur + 2) % 3; sfx_play(SFX_BLIP); }
    if (in.rep[BTN_DOWN]) { pOptCur = (pOptCur + 1) % 3; sfx_play(SFX_BLIP); }
    if (pOptCur == 0) {
      if (in.rep[BTN_LEFT] && g_volume > 0) { g_volume--; sfx_play(SFX_BLIP); }
      if (in.rep[BTN_RIGHT] && g_volume < 10) { g_volume++; sfx_play(SFX_BLIP); }
    } else if (pOptCur == 2) {
      if (in.rep[BTN_LEFT] && g_mouse_sens > 1) { g_mouse_sens--; sfx_play(SFX_BLIP); }
      if (in.rep[BTN_RIGHT] && g_mouse_sens < 10) { g_mouse_sens++; sfx_play(SFX_BLIP); }
    } else if (in.pressed[BTN_A]) { g_fullscreen_toggle = 1; sfx_play(SFX_OK); }
  } else if (pSub == 4) {
    if (in.pressed[BTN_B]) { pSub = 0; sfx_play(SFX_CANCEL); }
    if (in.pressed[BTN_A]) { ui_title_enter(); }
  }
}


void ui_pause_draw(void) {
  gfx_rect_a(0, 0, SCREEN_W, SCREEN_H, pal('i'), 140);
  menu_draw(&pm, 8, 8, 104);
  char b[160];
  snprintf(b, sizeof b, "$ %d", H.gold);
  gfx_window(8, 108, 104, 36);
  text_draw_sh(16, 113, b, pal('l'));
  long sec = H.playFrames / 60;
  snprintf(b, sizeof b, "Czas %ld:%02ld", sec / 3600, (sec / 60) % 60);
  text_draw_sh(16, 127, b, pal('s'));

  int px = 118, pw = SCREEN_W - px - 8, py = 8, ph = SCREEN_H - 16;
  gfx_window(px, py, pw, ph);
  int x = px + 10, y = py + 8;
  int view = pSub ? pSub : 0;
  if (pSub == 0) view = pm.cur == 1 ? 1 : pm.cur == 2 ? 2 : pm.cur == 4 ? 3 : 0;
  if (pSub == 4) view = 4;

  if (view == 0) {
    Actor *ac = actor_get("tomek");
    if (ac) gfx_blit_scaled(ac->fr[VIEW_FRONT][POSE_STAND], x, y, 48, 96, 0, 255);
    int sx = x + 60;
    text_draw_big(sx, y, "Tomek Kroniewski", pal('y'), 1);
    snprintf(b, sizeof b, "Zdrowie: %d / %d", H.hp, H.mhp);
    text_draw_sh(sx, y + 14, b, pal('w'));
    snprintf(b, sizeof b, "Kamizelka: %d", H.armor);
    text_draw_sh(sx, y + 26, b, pal('c'));
    snprintf(b, sizeof b, "Gotówka: $ %d", H.gold);
    text_draw_sh(sx, y + 38, b, pal('l'));
    text_draw_sh(sx, y + 54, "Broń:", pal('y'));
    int wy = y + 66;
    for (int w = 0; w < WPN_COUNT; w++) {
      if (!H.hasWpn[w]) continue;
      if (w == WPN_FISTS) snprintf(b, sizeof b, "%s%s", WPN_NAMES[w], H.weapon == w ? "  [w ręku]" : "");
      else snprintf(b, sizeof b, "%s  %d + %d%s", WPN_NAMES[w], H.clip[w], H.ammo[w], H.weapon == w ? "  [w ręku]" : "");
      text_draw_sh(sx + 6, wy, b, H.weapon == w ? pal('y') : pal('s'));
      wy += 11;
    }
    text_draw_sh(x, y + 120, "Reputacja", pal('y'));
    for (int i = 0; i < S.nstats; i++) {
      int v = var_find(S.stats[i].var, 0);
      int val = v >= 0 ? H.vars[v] : 0;
      int col = i % 2, row = i / 2;
      snprintf(b, sizeof b, "%s: %d", S.stats[i].label, val);
      text_draw_sh(x + col * 150, y + 133 + row * 12, b, val > 0 ? pal('l') : val < 0 ? pal('r') : pal('w'));
    }
  } else if (view == 1) {
    text_draw_sh(x, y, "Ekwipunek", pal('y'));
    if (!nItemList && pSub == 1) text_draw_sh(x, y + 16, "Pusto w kieszeniach.", pal('s'));
    if (pSub != 1) build_items();
    int first = pItemCur > 9 ? pItemCur - 9 : 0;
    for (int i = first; i < nItemList && i < first + 10; i++) {
      int it = itemList[i];
      int eq = 0;
      snprintf(b, sizeof b, "%s%s", S.items[it].name, eq ? " [E]" : "");
      u32 c = pSub == 1 && i == pItemCur ? pal('y') : S.items[it].type == IT_KEY ? pal('a') : pal('w');
      if (pSub == 1 && i == pItemCur) text_draw(x, y + 14 + (i - first) * 12, "\xe2\x96\xb6", pal('y'));
      text_draw_sh(x + 9, y + 14 + (i - first) * 12, b, c);
      snprintf(b, sizeof b, "x%d", H.inv[it]);
      text_draw_sh(x + pw - 40, y + 14 + (i - first) * 12, b, pal('s'));
    }
    if (pSub == 1 && nItemList) {
      char lines[3][160];
      int n = text_wrap(S.items[itemList[pItemCur]].desc, pw - 20, lines, 3);
      gfx_rect(x, py + ph - 42, pw - 20, 1, pal('d'));
      for (int k = 0; k < n; k++) text_draw_sh(x, py + ph - 38 + k * LINE_H, lines[k], pal('s'));
    }
  } else if (view == 2) {
    text_draw_sh(x, y, "Dziennik", pal('y'));
    int line = 0, yy = y + 14;
    for (int pass = 0; pass < 2; pass++)
      for (int i = H.nq - 1; i >= 0; i--) {
        if (H.q[i].done != pass) continue;
        char lines[4][160];
        int n = text_wrap(H.q[i].text, pw - 32, lines, 4);
        for (int k = 0; k < n; k++, line++) {
          if (line < pQuestScroll || yy > py + ph - 16) continue;
          if (k == 0) text_draw(x, yy, pass ? "\xe2\x9c\x93" : "\xe2\x80\xa2", pass ? pal('d') : pal('y'));
          text_draw_sh(x + 10, yy, lines[k], pass ? pal('d') : pal('w'));
          yy += LINE_H;
        }
        yy += 3;
      }
    if (!H.nq) text_draw_sh(x, y + 14, "Brak wpisów.", pal('s'));
  } else if (view == 3) {
    text_draw_sh(x, y, "Opcje", pal('y'));
    snprintf(b, sizeof b, "Głośność:  < %d >", g_volume);
    text_draw_sh(x + 10, y + 18, b, pSub == 3 && pOptCur == 0 ? pal('y') : pal('w'));
    text_draw_sh(x + 10, y + 32, "Przełącz pełny ekran (F11)", pSub == 3 && pOptCur == 1 ? pal('y') : pal('w'));
    snprintf(b, sizeof b, "Czułość myszy:  < %d >", g_mouse_sens);
    text_draw_sh(x + 10, y + 46, b, pSub == 3 && pOptCur == 2 ? pal('y') : pal('w'));
    text_draw_sh(x, y + 66, "Sterowanie:", pal('y'));
    text_draw_sh(x, y + 79, "WASD - ruch, mysz - rozglądanie, Shift - bieg", pal('s'));
    text_draw_sh(x, y + 90, "Lewy przycisk myszy / Ctrl - strzał, R - przeładuj", pal('s'));
    text_draw_sh(x, y + 101, "1-4 / kółko myszy - broń, E - rozmowa, drzwi", pal('s'));
    text_draw_sh(x, y + 112, "Tab - Księga Rodziny, N - koniec dnia (w Księdze)", pal('s'));
    text_draw_sh(x, y + 123, "Esc - menu, F11 / Alt+Enter - pełny ekran", pal('s'));
  } else if (view == 4) {
    text_draw_sh(x, y, "Wrócić do menu głównego?", pal('y'));
    text_draw_sh(x, y + 16, "Niezapisany postęp przepadnie.", pal('w'));
    text_draw_sh(x, y + 36, "Enter - tak     Esc - nie", pal('s'));
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
void ui_shop_update(void) {
  if (g_autoplay || in.pressed[BTN_B] || !shopN) { game_set_mode(MODE_WORLD); sfx_play(SFX_CANCEL); return; }
  if (in.rep[BTN_UP]) { shopCur = (shopCur + shopN - 1) % shopN; sfx_play(SFX_BLIP); }
  if (in.rep[BTN_DOWN]) { shopCur = (shopCur + 1) % shopN; sfx_play(SFX_BLIP); }
  if (in.pressed[BTN_A]) {
    ItemDef *d = &S.items[shopIds[shopCur]];
    if (H.gold >= d->price) {
      if (d->type == IT_WEAPON && H.hasWpn[d->power]) { H.ammo[d->power] += AMMO_PACK[d->power]; }
      else if (d->type == IT_WEAPON) { H.hasWpn[d->power] = 1; H.ammo[d->power] += AMMO_PACK[d->power]; H.weapon = d->power; }
      else if (d->type == IT_ARMOR) { if (H.armor >= 100) { sfx_play(SFX_CANCEL); return; } H.armor = H.armor + d->power > 100 ? 100 : H.armor + d->power; }
      else H.inv[shopIds[shopCur]]++;
      H.gold -= d->price;
      sfx_play(SFX_CHEST);
    } else sfx_play(SFX_CANCEL);
  }
}
void ui_shop_draw(void) {
  int x = 40, y = 20, w = SCREEN_W - 80, h = shopN * 13 + 64;
  gfx_window(x, y, w, h);
  text_draw_sh(x + 10, y + 7, "Co podać?", pal('y'));
  char b[96];
  snprintf(b, sizeof b, "Masz: $ %d", H.gold);
  text_draw_sh(x + w - text_width(b) - 10, y + 7, b, pal('l'));
  for (int i = 0; i < shopN; i++) {
    ItemDef *d = &S.items[shopIds[i]];
    u32 c = H.gold < d->price ? pal('d') : i == shopCur ? pal('y') : pal('w');
    if (i == shopCur) text_draw(x + 8, y + 24 + i * 13, "\xe2\x96\xb6", pal('y'));
    snprintf(b, sizeof b, "%s", d->name);
    text_draw_sh(x + 18, y + 24 + i * 13, b, c);
    if (d->type == IT_WEAPON) snprintf(b, sizeof b, H.hasWpn[d->power] ? "(amunicja)" : "(nowa broń)");
    else if (d->type == IT_ARMOR) snprintf(b, sizeof b, "(masz %d)", H.armor);
    else snprintf(b, sizeof b, "(masz %d)", H.inv[shopIds[i]]);
    text_draw_sh(x + w - 110, y + 24 + i * 13, b, pal('s'));
    snprintf(b, sizeof b, "$%d", d->price);
    text_draw_sh(x + w - 10 - text_width(b), y + 24 + i * 13, b, c);
  }
  char lines[2][160];
  int n = text_wrap(S.items[shopIds[shopCur]].desc, w - 20, lines, 2);
  for (int k = 0; k < n; k++) text_draw_sh(x + 10, y + h - 30 + k * LINE_H, lines[k], pal('s'));
}

/* ---------------------------------------------------------------- tytuł */
static Menu tm;
static int titleT;
void ui_title_enter(void) {
  menu_init(&tm);
  menu_add(&tm, "Nowa gra", 1);
  menu_add(&tm, "Kontynuuj", save_exists());
  menu_add(&tm, "Pełny ekran", 1);
  menu_add(&tm, "Wyjście", 1);
  if (save_exists()) tm.cur = 1;
  titleT = 0;
  game_set_mode(MODE_TITLE);
  music_play("tytul");
}
void ui_title_update(void) {
  titleT++;
  int r = menu_input(&tm);
  if (r == 0) {
    hero_new();
    combat_init();
    int m = map_find(S.startMap);
    world_load_map(m, S.startX, S.startY, S.startDir);
    game_set_mode(MODE_WORLD);
    g_fade = 255;
    g_autofade = S.startScript[0] ? 0 : 1; /* skrypt startowy sam robi „fade in” */
    if (S.startScript[0]) script_start(script_find(S.startScript), NULL);
  } else if (r == 1) {
    if (load_game()) { if (g_mode != MODE_TYCOON) game_set_mode(MODE_WORLD); g_fade = 255; }
    else ui_toast("Nie udało się wczytać zapisu.");
  } else if (r == 2) g_fullscreen_toggle = 1;
  else if (r == 3) g_quit = 1;
}

static void skyline(int t, int rain) {
  gfx_vgrad(0, 0, SCREEN_W, SCREEN_H, RGB(0x06, 0x06, 0x14), RGB(0x2a, 0x1c, 0x34));
  for (int i = 0; i < 40; i++) gfx_pset((i * 97) % SCREEN_W, (i * 53) % 90, pal('s'));
  gfx_circle(310, 40, 16, RGB(0xe8, 0xe4, 0xd0), 240);
  gfx_circle(304, 36, 16, RGB(0x10, 0x0c, 0x1e), 0);
  for (int i = 0; i < 14; i++) {
    int x = i * 30 - 12 + (i % 3) * 4, h = 60 + (i * 53) % 80;
    u32 c = RGB(0x12, 0x10, 0x1c);
    gfx_rect(x, SCREEN_H - h, 30, h, c);
    if (i % 4 == 1) { gfx_rect(x + 12, SCREEN_H - h - 18, 6, 18, c); gfx_rect(x + 14, SCREEN_H - h - 26, 2, 8, c); }
    for (int wy = SCREEN_H - h + 6; wy < SCREEN_H - 4; wy += 9)
      for (int wx = x + 4; wx < x + 26; wx += 7)
        if (((wx * 13 + wy * 7 + i + (t / 240)) % 7) < 2) gfx_rect(wx, wy, 3, 4, RGB(0xe8, 0xb8, 0x58));
  }
  if (rain)
    for (int i = 0; i < 120; i++) {
      u32 h = (u32)(i * 2654435761u);
      int x = (int)((h % 420) + t * 3) % 420 - 20, y = (int)(((h >> 9) % 240) + t * 7) % 240 - 12;
      for (int k = 0; k < 6; k++) gfx_pset_a(x - k / 2, y + k, RGB(0xa0, 0xb8, 0xd8), 90);
    }
}

void ui_title_draw(void) {
  skyline(titleT, 1);
  int lw = text_width("KroniX") * 5;
  text_draw_big((SCREEN_W - lw) / 2 + 2, 26, "KroniX", pal('i'), 5);
  text_draw_big((SCREEN_W - lw) / 2, 24, "KroniX", pal('z'), 5);
  const char *sub = "RODZINA";
  int sw = text_width(sub) * 2;
  text_draw_big((SCREEN_W - sw) / 2, 82, sub, pal('w'), 2);
  const char *yr = "Chicago, 1932 - 1934";
  text_draw_sh((SCREEN_W - text_width(yr)) / 2, 106, yr, pal('s'));
  menu_draw(&tm, (SCREEN_W - 110) / 2, 124, 110);
  text_draw((SCREEN_W - text_width("Enter - wybierz   Strzałki - ruch")) / 2, SCREEN_H - 12, "Enter - wybierz   Strzałki - ruch", pal('d'));
}

/* ---------------------------------------------------------------- koniec gry */
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
  gfx_darken(goT * 4 > 200 ? 200 : goT * 4);
  gfx_rect_a(0, 0, SCREEN_W, SCREEN_H, pal('j'), goT > 60 ? 90 : goT * 3 / 2);
  if (goT > 20) {
    const char *t = "NIE ŻYJESZ";
    text_draw_big((SCREEN_W - text_width(t) * 3) / 2, 50, t, pal('r'), 3);
    const char *s = "W tym mieście drugiej szansy nie ma. Prawie.";
    text_draw_sh((SCREEN_W - text_width(s)) / 2, 90, s, pal('w'));
  }
  if (goT >= 60) menu_draw(&gm, (SCREEN_W - 120) / 2, 120, 120);
}

/* ---------------------------------------------------------------- zakończenie */
static char endTitle[80];
static char endLines[40][160];
static int endN, endT;
void ui_ending_start(const char *title, const char *text) {
  snprintf(endTitle, sizeof endTitle, "%s", title);
  endN = text_wrap(text, SCREEN_W - 60, endLines, 40);
  endT = 0;
  game_set_mode(MODE_ENDING);
  music_play("koniec");
  H.vars[var_find("_UKONCZONO", 1)] = 1;
}
void ui_ending_update(void) {
  endT++;
  int total = 90 + endN * 40 + 200;
  if (g_autoplay && endT > 10) { ui_title_enter(); return; }
  if (endT > 120 && in.held[BTN_A]) endT += 3;
  if (endT > total && in.pressed[BTN_A]) ui_title_enter();
}
void ui_ending_draw(void) {
  skyline(endT, 0);
  gfx_darken(150);
  int a = endT * 4 > 255 ? 255 : endT * 4;
  (void)a;
  int tw = text_width(endTitle) * 2;
  text_draw_big((SCREEN_W - tw) / 2, 16, endTitle, pal('z'), 2);
  int shown = (endT - 60) / 40;
  if (shown >= endN) shown = endN - 1;
  int y0 = 48, maxY = SCREEN_H - 62;
  if (y0 + shown * LINE_H > maxY) y0 -= y0 + shown * LINE_H - maxY;
  for (int i = 0; i < endN && i <= shown; i++) {
    int y = y0 + i * LINE_H;
    if (y < 36) continue;
    text_draw_sh(30, y, endLines[i], pal('w'));
  }
  int total = 90 + endN * 40;
  if (endT > total) {
    const char *k = "KONIEC";
    text_draw_big((SCREEN_W - text_width(k) * 3) / 2, SCREEN_H - 50, k, pal('y'), 3);
    if (endT > total + 200) text_draw_sh((SCREEN_W - text_width("Enter - menu główne")) / 2, SCREEN_H - 14, "Enter - menu główne", pal('s'));
  }
}
