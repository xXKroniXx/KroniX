/* Rdzeń: stan bohatera, zapis/wczytanie, tryby gry, pętla klatki, sterownik testów. */
#include "engine.h"
#include <time.h>

int g_btn_raw[BTN_COUNT];
float g_mouse_dx, g_mouse_dy;
int g_mouse_x = -1, g_mouse_y = -1, g_want_mouse_capture;
int g_quit, g_fullscreen_toggle;
char g_data_dir[512];
int g_mouse_sens = 5;
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

/* ---------------------------------------------------------------- input */
void input_update(void) {
  for (int b = 0; b < BTN_COUNT; b++) {
    int h = g_btn_raw[b] != 0;
    in.pressed[b] = h && !in.held[b];
    in.holdT[b] = h ? in.holdT[b] + 1 : 0;
    in.held[b] = h;
    in.rep[b] = in.pressed[b] || (in.holdT[b] > 18 && (in.holdT[b] - 18) % 5 == 0);
  }
  float sens = g_mouse_sens / 5.0f;
  in.mdx = g_mouse_dx * sens;
  in.mdy = g_mouse_dy * sens;
  g_mouse_dx = g_mouse_dy = 0;
  in.click = in.pressed[BTN_FIRE];
  in.mx = g_mouse_x; in.my = g_mouse_y;
}
void input_clear(void) {
  for (int b = 0; b < BTN_COUNT; b++) in.pressed[b] = in.rep[b] = 0;
  in.click = 0;
}

/* ---------------------------------------------------------------- bohater */
void hero_new(void) {
  memset(&H, 0, sizeof H);
  H.hp = H.mhp = 100;
  H.gold = 20;
  H.hasWpn[WPN_FISTS] = 1;
  H.weapon = WPN_FISTS;
}

void quest_set(const char *id, const char *text) {
  for (int i = 0; i < H.nq; i++)
    if (!strcmp(H.q[i].id, id)) {
      snprintf(H.q[i].text, sizeof H.q[i].text, "%s", text);
      H.q[i].done = 0;
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
static void save_path(char *out, size_t n) { snprintf(out, n, "%skronix_rodzina_zapis.txt", g_data_dir); }

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
  fprintf(f, "KRONIX3D 1\n");
  fprintf(f, "map %s %d %d %d\n", S.maps[W.map].id, (int)floorf(W.px), (int)floorf(W.pz), 0);
  fprintf(f, "yaw %f\n", W.yaw);
  fprintf(f, "hero %d %d %d %d %ld %d\n", H.hp, H.mhp, H.armor, H.gold, H.playFrames, H.weapon);
  for (int w = 0; w < WPN_COUNT; w++) fprintf(f, "wpn %d %d %d %d\n", w, H.hasWpn[w], H.ammo[w], H.clip[w]);
  fprintf(f, "obj %s\n", H.objective);
  for (int i = 0; i < S.nitems; i++) if (H.inv[i]) fprintf(f, "inv %s %d\n", S.items[i].id, H.inv[i]);
  for (int i = 0; i < S.nvars; i++) if (H.vars[i]) fprintf(f, "var %s %d\n", S.varnames[i], H.vars[i]);
  for (int i = 0; i < H.nq; i++) fprintf(f, "quest %s %d %s\n", H.q[i].id, H.q[i].done, H.q[i].text);
  tycoon_save(f);
  fclose(f);
  return 1;
}

int load_game(void) {
  char p[600];
  save_path(p, sizeof p);
  FILE *f = fopen(p, "r");
  if (!f) return 0;
  static char line[1024];
  if (!fgets(line, sizeof line, f) || strncmp(line, "KRONIX3D", 8)) { fclose(f); return 0; }
  hero_new();
  combat_init();
  tycoon_init();
  char mapId[64] = "";
  int mx = 0, my = 0, md = 0;
  float yaw = 0;
  while (fgets(line, sizeof line, f)) {
    line[strcspn(line, "\r\n")] = 0;
    char a[64];
    int v, w, x1, x2;
    if (!strncmp(line, "map ", 4)) sscanf(line + 4, "%63s %d %d %d", mapId, &mx, &my, &md);
    else if (!strncmp(line, "yaw ", 4)) yaw = (float)atof(line + 4);
    else if (!strncmp(line, "hero ", 5)) sscanf(line + 5, "%d %d %d %d %ld %d", &H.hp, &H.mhp, &H.armor, &H.gold, &H.playFrames, &H.weapon);
    else if (!strncmp(line, "wpn ", 4) && sscanf(line + 4, "%d %d %d %d", &w, &v, &x1, &x2) == 4 && w >= 0 && w < WPN_COUNT) { H.hasWpn[w] = v; H.ammo[w] = x1; H.clip[w] = x2; }
    else if (!strncmp(line, "obj ", 4)) snprintf(H.objective, sizeof H.objective, "%s", line + 4);
    else if (!strncmp(line, "inv ", 4) && sscanf(line + 4, "%63s %d", a, &v) == 2) { int it = item_find(a); if (it >= 0) H.inv[it] = v; }
    else if (!strncmp(line, "var ", 4) && sscanf(line + 4, "%63s %d", a, &v) == 2) { int k = var_find(a, 1); if (k >= 0) H.vars[k] = v; }
    else if (!strncmp(line, "quest ", 6) && H.nq < MAX_QUESTS) {
      int done = 0, off = 0;
      if (sscanf(line + 6, "%23s %d %n", a, &done, &off) >= 2) {
        Quest *q = &H.q[H.nq++];
        snprintf(q->id, sizeof q->id, "%s", a);
        q->done = done;
        snprintf(q->text, sizeof q->text, "%s", line + 6 + off);
      }
    } else tycoon_load_line(line);
  }
  fclose(f);
  int m = map_find(mapId);
  if (!strncmp(mapId, "op_", 3) && S.hqMap[0]) { m = map_find(S.hqMap); mx = S.hqX; my = S.hqY; md = S.hqDir; } /* zapis z akcji: wracamy do kwatery */
  if (m < 0) return 0;
  if (H.hp <= 0) H.hp = H.mhp;
  script_stop();
  world_load_map(m, mx, my, md);
  W.yaw = yaw;
  game_set_mode(MODE_WORLD);
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
    combat_init();
    tycoon_init();
    world_load_map(map_find(S.startMap), S.startX, S.startY, S.startDir);
    game_set_mode(MODE_WORLD);
    g_fade = 0;
    if (S.startScript[0]) script_start(script_find(S.startScript), NULL);
  } else if (!strcmp(t[0], "warp") && n >= 4) {
    script_stop();
    world_load_map(map_find(t[1]), atoi(t[2]), atoi(t[3]), n > 4 ? atoi(t[4]) : DIR_DOWN);
    game_set_mode(MODE_WORLD);
    g_fade = 0;
  } else if (!strcmp(t[0], "look") && n >= 3) { W.yaw = (float)atof(t[1]); W.pitch = (float)atof(t[2]); }
  else if (!strcmp(t[0], "pos") && n >= 3) { W.px = (float)atof(t[1]); W.pz = (float)atof(t[2]); }
  else if (!strcmp(t[0], "set") && n >= 3) H.vars[var_find(t[1], 1)] = atoi(t[2]);
  else if (!strcmp(t[0], "give") && n >= 2) { int it = item_find(t[1]); if (it >= 0) H.inv[it] += n > 2 ? atoi(t[2]) : 1; }
  else if (!strcmp(t[0], "weapon") && n >= 2) { int w = atoi(t[1]); if (w >= 0 && w < WPN_COUNT) { H.hasWpn[w] = 1; H.ammo[w] += 100; H.weapon = w; } }
  else if (!strcmp(t[0], "cash") && n >= 2) H.gold += atoi(t[1]);
  else if (!strcmp(t[0], "script") && n >= 2) { int s = script_find(t[1]); if (s >= 0) script_start(s, NULL); else fprintf(stderr, "debug: brak skryptu %s\n", t[1]); }
  else if (!strcmp(t[0], "spawn") && n >= 4) world_spawn(t[1], atoi(t[2]), atoi(t[3]), n > 4);
  else if (!strcmp(t[0], "heal")) { H.hp = H.mhp; }
  else if (!strcmp(t[0], "killall")) { for (int i = 0; i < W.n; i++) if (W.ents[i].hostile && !W.ents[i].dead) { W.ents[i].dead = 1; W.ents[i].deadT = 24; } }
  else if (!strcmp(t[0], "god")) g_autoplay = 2;
  else if (!strcmp(t[0], "title")) ui_title_enter();
  else if (!strcmp(t[0], "tycoon")) { if (!tycoon_active()) tycoon_start(10); tycoon_open(); }
  else if (!strcmp(t[0], "mission") && n >= 3) printf("mission: %d\n", tycoon_debug_mission(atoi(t[1]), atoi(t[2])));
  else if (!strcmp(t[0], "endday")) { int k = n > 1 ? atoi(t[1]) : 1; for (int i = 0; i < k; i++) tycoon_end_day(); }
  else if (!strcmp(t[0], "mode") && n >= 2) game_set_mode(!strcmp(t[1], "world") ? MODE_WORLD : !strcmp(t[1], "menu") ? MODE_MENU : MODE_TITLE);
  else if (!strcmp(t[0], "pause")) ui_open_pause();
  else if (!strcmp(t[0], "save")) printf("save: %d\n", save_game());
  else if (!strcmp(t[0], "load")) printf("load: %d\n", load_game());
  else if (!strcmp(t[0], "dump")) {
    printf("[dump] mapa=%s pos=%.1f,%.1f tryb=%d hp=%d $=%d skrypt=%d wrogowie=%d\n", S.maps[W.map].id, W.px, W.pz, g_mode, H.hp, H.gold, script_running(), world_enemies_alive());
    for (int i = 0; i < S.nvars; i++) if (H.vars[i] && S.varnames[i][0] != '_') printf("  %s=%d\n", S.varnames[i], H.vars[i]);
    for (int i = 0; i < W.n; i++) {
      Ent *e = &W.ents[i];
      if (e->hostile || e->ally) printf("  ent %s %.1f,%.1f hp=%d st=%d dead=%d vis=%d\n", e->d->id, e->x, e->z, e->hp, e->state, e->dead, e->vis);
    }
  } else fprintf(stderr, "debug: nieznane polecenie '%s'\n", t[0]);
}

static char *tdLines[4096];
static int tdN, tdI, tdWait, tdHold[BTN_COUNT];
static float tdMouseX, tdMouseY;
static int btn_parse(const char *s) {
  static const char *names[BTN_COUNT] = {"up", "down", "left", "right", "a", "b", "run", "fire", "reload",
                                         "sl", "sr", "tl", "tr", "tab", "w1", "w2", "w3", "w4", "wnext", "n"};
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
  g_mouse_dx = tdMouseX; g_mouse_dy = tdMouseY;
  if (tdWait > 0) { tdWait--; if (!tdWait) tdMouseX = tdMouseY = 0; return; }
  tdMouseX = tdMouseY = 0;
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
    if (!strcmp(cmd, "mouse")) { int fr = 10; sscanf(arg, "%f %f %d", &tdMouseX, &tdMouseY, &fr); g_mouse_dx = tdMouseX; g_mouse_dy = tdMouseY; tdWait = fr; return; }
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
      else if (!strcmp(vn, "script")) got = script_running();
      else if (!strcmp(vn, "map")) got = W.map;
      else if (!strcmp(vn, "enemies")) got = world_enemies_alive();
      else { int v = var_find(vn, 0); got = v >= 0 ? H.vars[v] : 0; }
      printf("[test] %s %s: oczekiwano %d, jest %d\n", got == want ? "OK  " : "BŁĄD", vn, want, got);
      continue;
    }
    if (!strcmp(cmd, "untilidle")) {
      if (script_running() || W.trans) { tdI--; tdWait = 1; return; }
      continue;
    }
    if (!strcmp(cmd, "quit")) { g_quit = 1; return; }
    fprintf(stderr, "test: nieznane polecenie %s\n", cmd);
  }
}

/* Uruchamia każdy skrypt z losowymi wyborami, sprawdzając czy się kończy. */
int story_fuzz(int rounds) {
  int stuck = 0;
  g_autoplay = 2;
  for (int r = 0; r < rounds; r++)
    for (int i = 0; i < S.nscripts; i++) {
      hero_new();
      H.gold = 5000;
      world_load_map(map_find(S.startMap), S.startX, S.startY, S.startDir);
      game_set_mode(MODE_WORLD);
      script_start(i, NULL);
      int f;
      for (f = 0; f < 20000; f++) {
        game_frame();
        if (g_mode == MODE_TYCOON || g_mode == MODE_TRAVEL || g_mode == MODE_SHOP || g_mode == MODE_ENDING) game_set_mode(MODE_WORLD);
        if (!script_running()) break;
      }
      if (f >= 20000) { fprintf(stderr, "fuzz: skrypt '%s' się nie kończy\n", S.scripts[i].name); stuck++; script_stop(); }
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
  textures_init();
  actors_init();
  art_init();
  audio_init();
  combat_init();
  const char *storyPath = NULL;
  int validate = 0, simDays = 0;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--story") && i + 1 < argc) storyPath = argv[++i];
    else if (!strcmp(argv[i], "--validate")) validate = 1;
    else if (!strcmp(argv[i], "--seed") && i + 1 < argc) srand((unsigned)atoi(argv[++i]));
    else if (!strcmp(argv[i], "--simtycoon") && i + 1 < argc) simDays = atoi(argv[++i]);
  }
  char *ext = storyPath ? read_file(storyPath) : NULL;
  int errs = story_load(ext ? ext : STORY_TXT);
  if (errs) fprintf(stderr, "Fabuła: %d błędów\n", errs);
  hero_new();
  tycoon_init();
  world_load_map(map_find(S.startMap), S.startX, S.startY, S.startDir);
  if (validate) {
    int stuck = story_fuzz(2);
    printf("walidacja: %d błędów, %d zawieszonych skryptów, %d skryptów, %d komend, %d map\n", errs, stuck, S.nscripts, S.ncmds, S.nmaps);
    exit(errs || stuck ? 1 : 0);
  }
  if (simDays) { exit(tycoon_sim_days(simDays, 1)); }
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
    case MODE_MENU: ui_pause_update(); break;
    case MODE_SHOP: ui_shop_update(); break;
    case MODE_GAMEOVER: ui_gameover_update(); break;
    case MODE_ENDING: ui_ending_update(); break;
    case MODE_TYCOON: tycoon_update(); break;
    case MODE_TRAVEL: tycoon_travel_update(); break;
  }
  g_want_mouse_capture = g_mode == MODE_WORLD && !script_blocking();
  if (g_autofade && g_mode == MODE_WORLD) {
    g_fade -= 15;
    if (g_fade <= 0) { g_fade = 0; g_autofade = 0; }
  }
  if (g_autoplay && g_mode == MODE_MENU) game_set_mode(MODE_WORLD);
  if (g_autoplay && g_testdrv) { g_frame++; return; } /* szybkie testy: bez rysowania */

  switch (g_mode) {
    case MODE_TITLE: ui_title_draw(); break;
    case MODE_WORLD: world_draw(); gfx_darken(g_fade); script_draw(); break;
    case MODE_MENU: world_draw(); ui_pause_draw(); break;
    case MODE_SHOP: world_draw(); ui_shop_draw(); break;
    case MODE_GAMEOVER: world_draw(); ui_gameover_draw(); break;
    case MODE_ENDING: ui_ending_draw(); break;
    case MODE_TYCOON: tycoon_draw(); break;
    case MODE_TRAVEL: tycoon_travel_draw(); break;
  }
  post_shake();
  ui_toast_draw();
  if (g_mode != MODE_WORLD && in.mx >= 0 && in.my >= 0) { /* kursor myszy */
    static const char *cur[] = {"#", "##", "#w#", "#ww#", "#www#", "#wwww#", "#www##", "#w#", "##"};
    for (int y = 0; y < 9; y++)
      for (int x = 0; cur[y][x]; x++) gfx_pset(in.mx + x, in.my + y, cur[y][x] == '#' ? pal('i') : pal('w'));
  }
  g_frame++;
}
