/* KroniX: Rodzina — wspólny nagłówek silnika.
 * Rdzeń jest niezależny od platformy: rysuje do bufora `fb` (480x270, ARGB),
 * czyta przyciski i ruch myszy, generuje dźwięk w `audio_render`. */
#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCREEN_W UI_W
#define SCREEN_H UI_H
#define AUDIO_RATE 44100 /* stereo, próbki przeplatane L,P */
typedef uint32_t u32;
typedef uint8_t u8;

#define RGB(r, g, b) ((u32)(0xFF000000u | ((u32)(r) << 16) | ((u32)(g) << 8) | (u32)(b)))
#define PI_F 3.14159265f
#define CLAMP(v, a, b) ((v) < (a) ? (a) : (v) > (b) ? (b) : (v))

/* ------------------------------------------------------------------ platforma */
enum {
  BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, /* menu + ruch przód/tył */
  BTN_A, BTN_B, BTN_RUN, BTN_FIRE, BTN_RELOAD,
  BTN_SL, BTN_SR, BTN_TL, BTN_TR,       /* krok w bok (A/D), obrót (strzałki) */
  BTN_TAB, BTN_W1, BTN_W2, BTN_W3, BTN_W4, BTN_WNEXT, BTN_N,
  BTN_COUNT
};

extern int g_btn_raw[BTN_COUNT];
extern float g_mouse_dx, g_mouse_dy; /* ruch myszy w tej klatce (piksele) */
extern int g_mouse_x, g_mouse_y;      /* pozycja kursora w buforze (-1 poza) */
extern int g_want_mouse_capture;      /* rdzeń prosi o przechwycenie myszy (tryb FPP) */
extern int g_quit;
extern int g_fullscreen_toggle;
extern char g_data_dir[512];

void game_init(int argc, char **argv);
void game_frame(void);
void audio_render(int16_t *out, int n);

typedef struct {
  int held[BTN_COUNT], pressed[BTN_COUNT], rep[BTN_COUNT], holdT[BTN_COUNT];
  float mdx, mdy;
  int click, mx, my;
} Input;
extern Input in;
void input_update(void);
void input_clear(void);

/* ------------------------------------------------------------------ grafika (GPU) */
#include "render.h"
#include "assets.h"
#define LINE_H 22
/* paleta UI (ARGB) */
#define UI_GOLD 0xFFE8B84Au
#define UI_GOLD_D 0xFF9A7A34u
#define UI_CREAM 0xFFF0E6D0u
#define UI_GREY 0xFFA9B4C2u
#define UI_DIM 0xFF6A7080u
#define UI_RED 0xFFE0605Au
#define UI_GREEN 0xFF7CC97Au
#define UI_BLUE 0xFF7FB2F0u
#define UI_PANEL 0xE8141016u
#define UI_PANEL2 0xF01C1820u
int utf8_next(const char **p);
void ui_panel(float x, float y, float w, float h);        /* panel art-deco z cieniem */
void ui_button(float x, float y, float w, float h, const char *label, int selected, int enabled);
void ui_cursor(void);
extern int g_render;          /* czy jest kontekst GL (testy bez okna: 0) */
void game_draw(void);         /* rysuje klatkę (platforma woła po aktualizacjach) */
void gfx_shake(int amount);
extern int g_shake;

/* ------------------------------------------------------------------ dźwięk */
enum { SFX_BLIP, SFX_OK, SFX_CANCEL, SFX_HIT, SFX_CRIT, SFX_FIRE, SFX_HEAL, SFX_DOOR,
       SFX_CHEST, SFX_LEVEL, SFX_ENCOUNTER, SFX_FLEE, SFX_DIE, SFX_MAGIC, SFX_TEXT,
       SFX_PISTOL, SFX_TOMMY, SFX_SHOTGUN, SFX_PUNCH, SFX_RELOAD, SFX_EMPTY, SFX_STEP, SFX_HURT,
       SFX_HORN, SFX_PHONE, SFX_GLASS, SFX_BELL, SFX_TYPE, SFX_PAPER, SFX_CAR, SFX_COUNT };
void audio_init(void);
void music_play(const char *name);
void sfx_play(int id);
void sfx_play_at(int id, float dist);
void sfx_play_pan(int id, float gain, float pan);
/* tło dźwiękowe: rodzaj miejsca, deszcz 0..1, akustyka 0 plener / 1 pokój / 2 hala */
enum { AMB_NONE, AMB_STREET, AMB_HARBOR, AMB_ROOM, AMB_CROWD, AMB_OFFICE, AMB_CHURCH, AMB_WAREHOUSE };
void audio_ambience(int kind, float rain, float space);
int sfx_find(const char *name);
int music_exists(const char *name);
int art_exists(const char *name);
extern int g_volume;
extern int g_mouse_sens;

/* ------------------------------------------------------------------ dane fabuły */
#define MAX_MAPS 32
#define MAX_MAP_W 64
#define MAX_MAP_H 64
#define MAX_ENTS 96
#define MAX_ITEMS 64
#define MAX_ENEMIES 32
#define MAX_SPEAKERS 64
#define MAX_SCRIPTS 400
#define MAX_CMDS 9000
#define MAX_VARS 256
#define MAX_ARGS 10
#define MAX_QUESTS 16

enum { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP };
enum { ENT_NPC, ENT_OBJ, ENT_WARP, ENT_EVENT, ENT_MOB };

typedef struct {
  int type;
  char id[24];
  int x, y, dir;
  char sprite[24];
  int script;
  int wander;
  int condVar[2], condNeg[2];
  int toMap, toX, toY, toDir;
  char enemies[64];
  int once;
  int ally; /* mob sojuszniczy */
} EntDef;

typedef struct {
  char id[24], name[48], music[24], bg[24];
  int w, h, night, rain, interior, floors, district, ambient; /* ambient: AMB_* (0 = automatycznie) */
  float ceil;
  int ex, ey, edir; /* punkt wejścia przy podróży (-1 = brak) */
  u8 tiles[MAX_MAP_H][MAX_MAP_W];
  EntDef ents[MAX_ENTS];
  int nents;
} MapDef;

enum { IT_HEAL, IT_MANA, IT_ELIXIR, IT_WEAPON, IT_ARMOR, IT_CHARM, IT_KEY, IT_AMMO };
typedef struct { char id[24], name[40], desc[120]; int type, power, price; } ItemDef;

/* wróg w walce FPS */
enum { WPN_FISTS, WPN_PISTOL, WPN_SHOTGUN, WPN_TOMMY, WPN_COUNT };
typedef struct {
  char id[24], name[40], sprite[24];
  int hp, dmg, acc, speed, cash, weapon, boss;
  int hp_unused_pad;
} EnemyDef;

typedef struct { char id[24], name[40], sprite[24]; } Speaker;
typedef struct { int op, n, line; int i[MAX_ARGS]; const char *s[MAX_ARGS]; } Cmd;
typedef struct { char name[40]; int start, len; } Script;
typedef struct { const char *text; int target; int condType, condA, condB; } MenuOpt;
typedef struct { char var[32]; char label[40]; } StatDef;
#define MAX_OPTS 2000
#define MAX_STATS 8

typedef struct {
  char title[64];
  MapDef maps[MAX_MAPS]; int nmaps;
  ItemDef items[MAX_ITEMS]; int nitems;
  EnemyDef enemies[MAX_ENEMIES]; int nenemies;
  Speaker speakers[MAX_SPEAKERS]; int nspeakers;
  Script scripts[MAX_SCRIPTS]; int nscripts;
  Cmd cmds[MAX_CMDS]; int ncmds;
  char varnames[MAX_VARS][32]; int nvars;
  int varWrites[MAX_VARS], varReads[MAX_VARS];
  MenuOpt opts[MAX_OPTS]; int nopts;
  StatDef stats[MAX_STATS]; int nstats;
  char startMap[24]; int startX, startY, startDir; char startScript[40];
  char hqMap[24]; int hqX, hqY, hqDir;
  int errors;
} Story;
extern Story S;

int story_load(const char *text);
int cond_check(int type, int a, int b);
enum { CND_NONE, CND_IF, CND_IFNOT, CND_GE, CND_LT, CND_GOLD, CND_HAS };
enum { OP_END, OP_SAY, OP_AS, OP_MENU, OP_GOTO, OP_IF, OP_SET, OP_ADD, OP_GIVE, OP_TAKE, OP_CASH, OP_HEAL,
       OP_BATTLE, OP_WARP, OP_FADE, OP_WAIT, OP_SFX, OP_MUSIC, OP_SHAKE, OP_QUEST, OP_DONE, OP_SHOP,
       OP_SAVE, OP_ENDING, OP_LEARN, OP_FACE, OP_WALK, OP_CALL, OP_TOAST, OP_CHANCE, OP_HURT, OP_EQUIP, OP_XP,
       OP_SPAWN, OP_WAITKILL, OP_WEAPON, OP_TYCOON, OP_OBJECTIVE, OP_KILLALL, OP_TRUST, OP_NOP };
int map_find(const char *id);
int item_find(const char *id);
int enemy_find(const char *id);
int script_find(const char *name);
int speaker_find(const char *id);
int var_find(const char *name, int create);
int tile_valid(int c);
int tile_solid(int c);
int tile_blocks_sight(int c);
float tile_prop_height(int c);
void city_build(MapDef *m);
void city_lights(float time);
void city_glows(void);
void city_draw(void);
float world_ground(float x, float z);

/* ------------------------------------------------------------------ bohater */
typedef struct { char id[24]; char text[120]; int done; } Quest;
typedef struct {
  int hp, mhp, armor;
  int gold;
  int inv[MAX_ITEMS];
  int vars[MAX_VARS];
  int hasWpn[WPN_COUNT], ammo[WPN_COUNT], clip[WPN_COUNT];
  int weapon;
  Quest q[MAX_QUESTS]; int nq;
  long playFrames;
  char objective[120];
} Hero;
extern Hero H;
void quest_set(const char *id, const char *text);
void quest_done(const char *id);
int save_exists(void);
int save_game(void);
int load_game(void);
void hero_new(void);

enum { MODE_TITLE, MODE_WORLD, MODE_BATTLE, MODE_MENU, MODE_SHOP, MODE_GAMEOVER, MODE_ENDING, MODE_TYCOON, MODE_TRAVEL };
extern int g_mode;
extern long g_frame;
void game_set_mode(int m);
extern int g_fade;
extern int g_autofade;

/* ------------------------------------------------------------------ świat 3D */
typedef struct {
  EntDef *d;
  float x, z, ang;       /* pozycja (środek w jednostkach kafli), kierunek patrzenia */
  float homeX, homeZ;
  int vis, dead, busy;
  /* walka */
  int hostile, ally, hp, maxhp, state, stateT, cool, hitT, deadT, enemyDef;
  float tx, tz;          /* cel ruchu */
  int walkT, moving, shootT;
  int talkT;
  int htype;        /* typ postaci 3D (-1 = przedmiot) */
  float animT;
} Ent;

typedef struct {
  int map;
  Ent ents[MAX_ENTS]; int n;
  float px, pz, yaw, pitch, bob, bobT;
  int time, trans, tMap, tX, tY, tDir, grace;
  int bannerT;
  int hurtT, flashT;
  int enemiesAlive;
} World;
extern World W;

void world_audio(void);
void world_load_map(int map, int tx, int ty, int dir);
void world_update(void);
void world_draw(void);
void world_refresh_visibility(void);
Ent *world_find_ent(const char *id);
int world_solid_at(float x, float z);
int world_los(float x0, float z0, float x1, float z1);
void world_spawn(const char *enemyId, int tx, int ty, int ally);
int world_enemies_alive(void);
void world_hurt_player(int dmg);
void world_kill_all(void);
void move_circle(float *x, float *z, float dx, float dz, float r, Ent *self);
void world_draw_ui(void);

/* walka */
void combat_init(void);
void combat_update(void);
void combat_draw_view(void);
void combat_ent_update(Ent *e);
void combat_draw_hud(void);
void combat_lights(void);
void combat_draw_world(void);
void combat_top_up(int w);
extern const char *WPN_NAMES[WPN_COUNT];

/* ------------------------------------------------------------------ skrypty */
void script_start(int idx, Ent *self);
int script_running(void);
int script_blocking(void); /* czy skrypt zatrzymuje gracza (dialog) */
Ent *script_self(void);
void script_update(void);
void script_draw(void);
void script_stop(void);

/* ------------------------------------------------------------------ ui */
typedef struct {
  const char *items[24];
  int enabled[24];
  int n, cur, scroll, visible;
  float dx, dy, dw, rh; /* ostatnie położenie (mysz) */
  int lmx, lmy;
} Menu;
void menu_init(Menu *m);
void menu_add(Menu *m, const char *label, int enabled);
int menu_input(Menu *m);
void menu_draw(Menu *m, float x, float y, float w);
void settings_save(void);
void settings_load(void);
void quality_apply(int preset);
extern int g_quality;
void ui_open_pause(void);
void ui_pause_update(void);
void ui_pause_draw(void);
void ui_open_shop(const char **ids, int n);
void ui_shop_update(void);
void ui_shop_draw(void);
void ui_title_enter(void);
void ui_title_update(void);
void ui_title_draw(void);
void ui_gameover_update(void);
void ui_gameover_draw(void);
void ui_ending_start(const char *title, const char *text);
void ui_ending_update(void);
void ui_ending_draw(void);
void ui_toast(const char *msg);
void ui_toast_draw(void);

/* ------------------------------------------------------------------ tycoon */
void tycoon_init(void);
void tycoon_start(int trust);
void tycoon_open(void);
void tycoon_update(void);
void tycoon_draw(void);
void tycoon_end_day(void);
void tycoon_save(FILE *f);
int tycoon_load_line(const char *line);
int tycoon_active(void);
void tycoon_travel_open(void);
void tycoon_travel_update(void);
void tycoon_travel_draw(void);
int tycoon_sim_days(int days, int verbose); /* test: symulacja z prostą AI gracza */
void tycoon_world_tick(void);   /* co klatkę w świecie 3D (akcje osobiste) */
int tycoon_mission_active(void);
void tycoon_mission_fail(void); /* gracz zginął podczas akcji */
int tycoon_debug_mission(int type, int target);

/* ------------------------------------------------------------------ test / debug */
void testdrv_load(const char *path);
void testdrv_frame(void);
extern int g_testdrv;
extern int g_autoplay;
void game_debug(const char *cmd);
int story_fuzz(int rounds);

#endif
