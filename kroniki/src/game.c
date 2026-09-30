/* Rdzeń: stan bohatera, zapis/wczytanie, tryby gry, pętla klatki, sterownik testów. */
#include "engine.h"
#include <time.h>

int g_btn_raw[BTN_COUNT];
int g_quit, g_fullscreen_toggle;
char g_data_dir[512];
Input in;
Hero H;
int g_mode = MODE_TITLE;
long g_frame;
int g_fade;
int g_autoplay;
int g_testdrv;
int g_autofade;
extern int g_forceChoice;
extern int g_forceQ[64], g_forceN, g_forceI;
extern const char STORY_TXT[];

const SkillDef SKILLS[SK_COUNT] = {
  {"hak", "Hak", "Potężny cios w jednego wroga (185% siły).", 3, 1},
  {"opatrunek", "Opatrunek", "Szybko opatrujesz rany (ok. 35% zdrowia).", 4, 3},
  {"seria", "Seria z Thompsona", "Grad kul we wszystkich wrogów (110% siły).", 6, 99},
  {"zastraszenie", "Zastraszenie", "Słabi wrogowie uciekają, reszta traci pewność.", 4, 5},
};

/* ---------------------------------------------------------------- input */
void input_update(void) {
  for (int b = 0; b < BTN_COUNT; b++) {
    int h = g_btn_raw[b] != 0;
    in.pressed[b] = h && !in.held[b];
    in.holdT[b] = h ? in.holdT[b] + 1 : 0;
    in.held[b] = h;
    in.rep[b] = in.pressed[b] || (in.holdT[b] > 18 && (in.holdT[b] - 18) % 5 == 0);
  }
}
void input_clear(void) {
  for (int b = 0; b < BTN_COUNT; b++) in.pressed[b] = in.rep[b] = 0;
}

/* ---------------------------------------------------------------- bohater */
int xp_next(int lvl) { return 10 * lvl * lvl + 10 * lvl; }
int hero_atk(void) { return H.atk + (H.weapon >= 0 ? S.items[H.weapon].power : 0); }
int hero_def(void) { return H.def + (H.armor >= 0 ? S.items[H.armor].power : 0) + (H.charm >= 0 ? S.items[H.charm].power : 0); }
int hero_mag(void) { return hero_atk(); }

void hero_new(void) {
  memset(&H, 0, sizeof H);
  H.lvl = 1; H.hp = H.mhp = 42; H.mp = H.mmp = 8;
  H.atk = 7; H.def = 3; H.spd = 5; H.gold = 15;
  H.weapon = H.armor = H.charm = -1;
  for (int i = 0; i < SK_COUNT; i++) if (SKILLS[i].lvl <= 1) H.skills[i] = 1;
}

int hero_gain_xp(int xp, char msgs[][160], int maxMsgs) {
  int n = 0;
  H.xp += xp;
  while (H.xp >= xp_next(H.lvl)) {
    H.xp -= xp_next(H.lvl);
    H.lvl++;
    H.mhp += 10; H.mmp += 2; H.atk += 2; H.def += 1; H.spd += H.lvl % 2;
    H.hp = H.hp + 12 > H.mhp ? H.mhp : H.hp + 12;
    H.mp = H.mmp;
    if (n < maxMsgs) snprintf(msgs[n++], 160, "Awans! Tomek osiąga poziom %d.", H.lvl);
    sfx_play(SFX_LEVEL);
    for (int i = 0; i < SK_COUNT; i++)
      if (SKILLS[i].lvl == H.lvl && !H.skills[i]) {
        H.skills[i] = 1;
        if (n < maxMsgs) snprintf(msgs[n++], 160, "Nowa sztuczka: %s!", SKILLS[i].name);
      }
  }
  return n;
}

void quest_set(const char *id, const char *text) {
  for (int i = 0; i < H.nq; i++)
    if (!strcmp(H.q[i].id, id)) {
      snprintf(H.q[i].text, sizeof H.q[i].text, "%s", text);
      H.q[i].done = 0;
      /* przesuń na koniec, żeby było "bieżące" */
      Quest q = H.q[i];
      for (int k = i; k < H.nq - 1; k++) H.q[k] = H.q[k + 1];
      H.q[H.nq - 1] = q;
      ui_toast("Dziennik zaktualizowany");
      return;
    }
  if (H.nq >= MAX_QUESTS) {
    for (int k = 0; k < H.nq - 1; k++) H.q[k] = H.q[k + 1];
    H.nq--;
  }
  Quest *q = &H.q[H.nq++];
  snprintf(q->id, sizeof q->id, "%s", id);
  snprintf(q->text, sizeof q->text, "%s", text);
  q->done = 0;
  ui_toast("Nowe zadanie w dzienniku");
}

void quest_done(const char *id) {
  for (int i = 0; i < H.nq; i++)
    if (!strcmp(H.q[i].id, id) && !H.q[i].done) { H.q[i].done = 1; ui_toast("Zadanie wykonane"); }
}

/* ---------------------------------------------------------------- zapis */
static void save_path(char *out, size_t n) { snprintf(out, n, "%skronix_prohibicja_zapis.txt", g_data_dir); }

int save_exists(void) {
  char p[600];
  save_path(p, sizeof p);
  FILE *f = fopen(p, "r");
  if (!f) return 0;
  fclose(f);
  return 1;
}

int save_game(void) {
  char p[600];
  save_path(p, sizeof p);
  FILE *f = fopen(p, "w");
  if (!f) return 0;
  fprintf(f, "KRONIX1\n");
  fprintf(f, "map %s %d %d %d\n", S.maps[W.map].id, W.hero.tx, W.hero.ty, W.hero.dir);
  fprintf(f, "hero %d %d %d %d %d %d %d %d %d %d %ld\n", H.lvl, H.xp, H.hp, H.mhp, H.mp, H.mmp, H.atk, H.def, H.spd, H.gold, H.playFrames);
  fprintf(f, "equip %s %s %s\n", H.weapon >= 0 ? S.items[H.weapon].id : "-", H.armor >= 0 ? S.items[H.armor].id : "-", H.charm >= 0 ? S.items[H.charm].id : "-");
  for (int i = 0; i < S.nitems; i++) if (H.inv[i]) fprintf(f, "inv %s %d\n", S.items[i].id, H.inv[i]);
  for (int i = 0; i < SK_COUNT; i++) if (H.skills[i]) fprintf(f, "skill %s\n", SKILLS[i].id);
  for (int i = 0; i < S.nvars; i++) if (H.vars[i]) fprintf(f, "var %s %d\n", S.varnames[i], H.vars[i]);
  for (int i = 0; i < H.nq; i++) fprintf(f, "quest %s %d %s\n", H.q[i].id, H.q[i].done, H.q[i].text);
  fclose(f);
  return 1;
}

int load_game(void) {
  char p[600];
  save_path(p, sizeof p);
  FILE *f = fopen(p, "r");
  if (!f) return 0;
  char line[512];
  if (!fgets(line, sizeof line, f) || strncmp(line, "KRONIX1", 7)) { fclose(f); return 0; }
  hero_new();
  for (int i = 0; i < SK_COUNT; i++) H.skills[i] = 0;
  char mapId[64] = "";
  int mx = 0, my = 0, md = 0;
  while (fgets(line, sizeof line, f)) {
    line[strcspn(line, "\r\n")] = 0;
    char a[64], b[64], c[64];
    int v;
    if (!strncmp(line, "map ", 4)) sscanf(line + 4, "%63s %d %d %d", mapId, &mx, &my, &md);
    else if (!strncmp(line, "hero ", 5))
      sscanf(line + 5, "%d %d %d %d %d %d %d %d %d %d %ld", &H.lvl, &H.xp, &H.hp, &H.mhp, &H.mp, &H.mmp, &H.atk, &H.def, &H.spd, &H.gold, &H.playFrames);
    else if (!strncmp(line, "equip ", 6) && sscanf(line + 6, "%63s %63s %63s", a, b, c) == 3) {
      H.weapon = item_find(a); H.armor = item_find(b); H.charm = item_find(c);
    } else if (!strncmp(line, "inv ", 4) && sscanf(line + 4, "%63s %d", a, &v) == 2) {
      int it = item_find(a);
      if (it >= 0) H.inv[it] = v;
    } else if (!strncmp(line, "skill ", 6)) {
      for (int i = 0; i < SK_COUNT; i++) if (!strcmp(line + 6, SKILLS[i].id)) H.skills[i] = 1;
    } else if (!strncmp(line, "var ", 4) && sscanf(line + 4, "%63s %d", a, &v) == 2) {
      int k = var_find(a, 1);
      if (k >= 0) H.vars[k] = v;
    } else if (!strncmp(line, "quest ", 6) && H.nq < MAX_QUESTS) {
      int done = 0, off = 0;
      if (sscanf(line + 6, "%23s %d %n", a, &done, &off) >= 2) {
        Quest *q = &H.q[H.nq++];
        snprintf(q->id, sizeof q->id, "%s", a);
        q->done = done;
        snprintf(q->text, sizeof q->text, "%s", line + 6 + off);
      }
    }
  }
  fclose(f);
  int m = map_find(mapId);
  if (m < 0) return 0;
  script_stop();
  world_load_map(m, mx, my, md);
  g_autofade = 1;
  return 1;
}

/* ---------------------------------------------------------------- tryby */
void game_set_mode(int m) {
  g_mode = m;
  input_clear();
}

/* ---------------------------------------------------------------- debug / testy */
void game_debug(const char *cmdline) {
  char buf[256];
  snprintf(buf, sizeof buf, "%s", cmdline);
  char *t[8];
  int n = 0;
  for (char *s = strtok(buf, " "); s && n < 8; s = strtok(NULL, " ")) t[n++] = s;
  if (!n) return;
  if (!strcmp(t[0], "newgame")) {
    hero_new();
    world_load_map(map_find(S.startMap), S.startX, S.startY, S.startDir);
    game_set_mode(MODE_WORLD);
    g_fade = 0;
    if (S.startScript[0]) script_start(script_find(S.startScript), NULL);
  } else if (!strcmp(t[0], "warp") && n >= 4) {
    script_stop();
    world_load_map(map_find(t[1]), atoi(t[2]), atoi(t[3]), n > 4 ? atoi(t[4]) : DIR_DOWN);
    game_set_mode(MODE_WORLD);
    g_fade = 0;
  } else if (!strcmp(t[0], "set") && n >= 3) H.vars[var_find(t[1], 1)] = atoi(t[2]);
  else if (!strcmp(t[0], "give") && n >= 2) { int it = item_find(t[1]); if (it >= 0) H.inv[it] += n > 2 ? atoi(t[2]) : 1; }
  else if (!strcmp(t[0], "equip") && n >= 2) { int it = item_find(t[1]); if (it >= 0) { H.inv[it]++; if (S.items[it].type == IT_WEAPON) H.weapon = it; else if (S.items[it].type == IT_ARMOR) H.armor = it; } }
  else if (!strcmp(t[0], "cash") && n >= 2) H.gold += atoi(t[1]);
  else if (!strcmp(t[0], "script") && n >= 2) { int s = script_find(t[1]); if (s >= 0) script_start(s, NULL); else fprintf(stderr, "debug: brak skryptu %s\n", t[1]); }
  else if (!strcmp(t[0], "battle") && n >= 2) battle_start(t[1], 0, S.maps[W.map].bg, 0);
  else if (!strcmp(t[0], "level") && n >= 2) { char m[8][160]; while (H.lvl < atoi(t[1])) hero_gain_xp(xp_next(H.lvl) - H.xp, m, 8); }
  else if (!strcmp(t[0], "heal")) { H.hp = H.mhp; H.mp = H.mmp; }
  else if (!strcmp(t[0], "title")) ui_title_enter();
  else if (!strcmp(t[0], "mode") && n >= 2) game_set_mode(!strcmp(t[1], "world") ? MODE_WORLD : !strcmp(t[1], "menu") ? MODE_MENU : MODE_TITLE);
  else if (!strcmp(t[0], "pause")) ui_open_pause();
  else if (!strcmp(t[0], "save")) printf("save: %d\n", save_game());
  else if (!strcmp(t[0], "load")) printf("load: %d\n", load_game());
  else if (!strcmp(t[0], "dump")) {
    printf("[dump] mapa=%s pos=%d,%d tryb=%d poz=%d hp=%d/%d $=%d skrypt=%d\n", S.maps[W.map].id, W.hero.tx, W.hero.ty, g_mode, H.lvl, H.hp, H.mhp, H.gold, script_running());
    for (int i = 0; i < S.nvars; i++) if (H.vars[i] && S.varnames[i][0] != '_') printf("  %s=%d\n", S.varnames[i], H.vars[i]);
  } else fprintf(stderr, "debug: nieznane polecenie '%s'\n", t[0]);
}

static char *tdLines[4096];
static int tdN, tdI, tdWait, tdHold[BTN_COUNT];
static int btn_parse(const char *s) {
  static const char *names[BTN_COUNT] = {"up", "down", "left", "right", "a", "b", "run"};
  for (int i = 0; i < BTN_COUNT; i++) if (!strcmp(s, names[i])) return i;
  return -1;
}

void testdrv_load(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) { fprintf(stderr, "brak skryptu testowego %s\n", path); return; }
  char line[512];
  while (fgets(line, sizeof line, f) && tdN < 4096) {
    line[strcspn(line, "\r\n")] = 0;
    if (!line[0] || line[0] == '#') continue;
    tdLines[tdN] = (char *)malloc(strlen(line) + 1);
    strcpy(tdLines[tdN++], line);
  }
  fclose(f);
  g_testdrv = 1;
}

void testdrv_frame(void) {
  for (int b = 0; b < BTN_COUNT; b++) {
    g_btn_raw[b] = tdHold[b] > 0;
    if (tdHold[b] > 0) tdHold[b]--;
  }
  if (tdWait > 0) { tdWait--; return; }
  while (tdI < tdN) {
    char *l = tdLines[tdI++];
    char cmd[32] = "", arg[400] = "";
    sscanf(l, "%31s %399[^\n]", cmd, arg);
    if (!strcmp(cmd, "wait")) { tdWait = atoi(arg); return; }
    if (!strcmp(cmd, "press")) { int b = btn_parse(arg); if (b >= 0) { tdHold[b] = 1; g_btn_raw[b] = 1; } tdWait = 2; return; }
    if (!strcmp(cmd, "hold")) {
      char bn[16]; int fr = 10;
      sscanf(arg, "%15s %d", bn, &fr);
      int b = btn_parse(bn);
      if (b >= 0) { tdHold[b] = fr; g_btn_raw[b] = 1; }
      tdWait = fr;
      return;
    }
    if (!strcmp(cmd, "shot")) { save_bmp(arg); continue; }
    if (!strcmp(cmd, "debug")) { game_debug(arg); continue; }
    if (!strcmp(cmd, "autoplay")) { g_autoplay = atoi(arg); continue; }
    if (!strcmp(cmd, "choose")) {
      g_forceN = g_forceI = 0;
      for (char *t = strtok(arg, " "); t && g_forceN < 64; t = strtok(NULL, " ")) g_forceQ[g_forceN++] = atoi(t);
      continue;
    }
    if (!strcmp(cmd, "log")) { printf("[test] %s\n", arg); continue; }
    if (!strcmp(cmd, "expect")) {
      char vn[64]; int want = 0;
      sscanf(arg, "%63s %d", vn, &want);
      int got = 0;
      if (!strcmp(vn, "mode")) got = g_mode;
      else if (!strcmp(vn, "gold")) got = H.gold;
      else if (!strcmp(vn, "lvl")) got = H.lvl;
      else if (!strcmp(vn, "script")) got = script_running();
      else if (!strcmp(vn, "map")) got = W.map;
      else { int v = var_find(vn, 0); got = v >= 0 ? H.vars[v] : 0; }
      printf("[test] %s %s: oczekiwano %d, jest %d\n", got == want ? "OK  " : "BŁĄD", vn, want, got);
      continue;
    }
    if (!strcmp(cmd, "untilidle")) { /* czekaj aż skończy się skrypt/walka */
      if (script_running() || g_mode == MODE_BATTLE || W.trans) { tdI--; tdWait = 1; return; }
      continue;
    }
    if (!strcmp(cmd, "quit")) { g_quit = 1; return; }
    fprintf(stderr, "test: nieznane polecenie %s\n", cmd);
  }
}

/* Uruchamia każdy skrypt z losowymi wyborami, sprawdzając czy się kończy. */
int story_fuzz(int rounds) {
  int stuck = 0;
  g_autoplay = 1;
  for (int r = 0; r < rounds; r++)
    for (int i = 0; i < S.nscripts; i++) {
      hero_new();
      H.gold = 1000;
      char m[8][160];
      hero_gain_xp(4000, m, 8);
      world_load_map(map_find(S.startMap), S.startX, S.startY, S.startDir);
      game_set_mode(MODE_WORLD);
      script_start(i, NULL);
      int f;
      for (f = 0; f < 30000; f++) {
        game_frame();
        if (!script_running() && g_mode != MODE_BATTLE && g_mode != MODE_SHOP) break;
      }
      if (f >= 30000) { fprintf(stderr, "fuzz: skrypt '%s' się nie kończy (tryb %d)\n", S.scripts[i].name, g_mode); stuck++; script_stop(); }
    }
  g_autoplay = 0;
  return stuck;
}

/* ---------------------------------------------------------------- init / klatka */
static char *read_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *b = (char *)malloc(n + 1);
  if (fread(b, 1, n, f) != (size_t)n) { free(b); fclose(f); return NULL; }
  b[n] = 0;
  fclose(f);
  return b;
}

void game_init(int argc, char **argv) {
  srand((unsigned)time(NULL));
  font_init();
  art_init();
  audio_init();
  const char *storyPath = NULL;
  int validate = 0;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--story") && i + 1 < argc) storyPath = argv[++i];
    else if (!strcmp(argv[i], "--validate")) validate = 1;
    else if (!strcmp(argv[i], "--seed") && i + 1 < argc) srand((unsigned)atoi(argv[++i]));
  }
  char *ext = storyPath ? read_file(storyPath) : NULL;
  int errs = story_load(ext ? ext : STORY_TXT);
  if (errs) fprintf(stderr, "Fabuła: %d błędów\n", errs);
  hero_new();
  world_load_map(map_find(S.startMap), S.startX, S.startY, S.startDir);
  if (validate) {
    int stuck = story_fuzz(2);
    printf("walidacja: %d błędów, %d zawieszonych skryptów, %d skryptów, %d komend, %d map\n", errs, stuck, S.nscripts, S.ncmds, S.nmaps);
    exit(errs || stuck ? 1 : 0);
  }
  for (int i = 1; i < argc; i++)
    if (!strcmp(argv[i], "--script") && i + 1 < argc) testdrv_load(argv[++i]);
  ui_title_enter();
}

static void post_shake(void) {
  if (g_shake <= 0) return;
  int dx = (rand() % 3 - 1) * (g_shake > 3 ? 2 : 1), dy = (rand() % 3 - 1);
  static u32 tmp[SCREEN_W * SCREEN_H];
  memcpy(tmp, fb, sizeof tmp);
  for (int y = 0; y < SCREEN_H; y++)
    for (int x = 0; x < SCREEN_W; x++) {
      int sx = x - dx, sy = y - dy;
      fb[y * SCREEN_W + x] = (sx >= 0 && sy >= 0 && sx < SCREEN_W && sy < SCREEN_H) ? tmp[sy * SCREEN_W + sx] : pal('i');
    }
  g_shake--;
}

void game_frame(void) {
  if (g_testdrv) testdrv_frame();
  input_update();
  switch (g_mode) {
    case MODE_TITLE: ui_title_update(); break;
    case MODE_WORLD: H.playFrames++; world_update(); break;
    case MODE_BATTLE: battle_update(); break;
    case MODE_MENU: ui_pause_update(); break;
    case MODE_SHOP: ui_shop_update(); break;
    case MODE_GAMEOVER: ui_gameover_update(); break;
    case MODE_ENDING: ui_ending_update(); break;
  }
  if (g_autofade && g_mode == MODE_WORLD) {
    g_fade -= 15;
    if (g_fade <= 0) { g_fade = 0; g_autofade = 0; }
  }
  if (g_autoplay && g_mode == MODE_MENU) game_set_mode(MODE_WORLD);

  if (g_autoplay && g_testdrv) { g_frame++; return; } /* szybkie testy: bez rysowania */
  switch (g_mode) {
    case MODE_TITLE: ui_title_draw(); break;
    case MODE_WORLD: world_draw(); gfx_darken(g_fade); script_draw(); break;
    case MODE_BATTLE: battle_draw(); break;
    case MODE_MENU: world_draw(); ui_pause_draw(); break;
    case MODE_SHOP: world_draw(); ui_shop_draw(); break;
    case MODE_GAMEOVER: world_draw(); ui_gameover_draw(); break;
    case MODE_ENDING: ui_ending_draw(); break;
  }
  post_shake();
  ui_toast_draw();
  g_frame++;
}
