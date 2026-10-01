/* KroniX: Rodzina — nagłówek gry (dane fabuły, świat, walka, tycoon).
 * Silnik (okno, grafika, UI, dźwięk) jest w ../engine — patrz engine/README.md. */
#ifndef ENGINE_H
#define ENGINE_H

#include "kx.h"
#include "kx_audio.h"

#define SCREEN_W UI_W
#define SCREEN_H UI_H

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
void ui_panel(float x, float y, float w, float h);        /* panel art-deco z cieniem */
void ui_button(float x, float y, float w, float h, const char *label, int selected, int enabled);
void ui_cursor(void);
void gfx_shake(int amount);
extern int g_shake;

/* ------------------------------------------------------------------ dźwięk */
enum { SFX_BLIP, SFX_OK, SFX_CANCEL, SFX_HIT, SFX_CRIT, SFX_FIRE, SFX_HEAL, SFX_DOOR,
       SFX_CHEST, SFX_LEVEL, SFX_ENCOUNTER, SFX_FLEE, SFX_DIE, SFX_MAGIC, SFX_TEXT,
       SFX_PISTOL, SFX_TOMMY, SFX_SHOTGUN, SFX_PUNCH, SFX_RELOAD, SFX_EMPTY, SFX_STEP, SFX_HURT,
       SFX_HORN, SFX_PHONE, SFX_GLASS, SFX_BELL, SFX_TYPE, SFX_PAPER, SFX_CAR, SFX_TRAIN, SFX_COUNT };
void game_audio_setup(void); /* sounds.c: rejestruje utwory i efekty w silniku */
int art_exists(const char *name);
extern int g_headbob;   /* kołysanie kamery przy chodzeniu: 0 wył., 1 słabe, 2 normalne */
void world_look(float dx, float dy);

/* ------------------------------------------------------------------ dane fabuły */
#define MAX_MAPS 32
#define MAX_MAP_W 160
#define MAX_MAP_H 120
#define MAX_ENTS 192
#define MAX_REGIONS 24
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
  int condVar[3], condNeg[3];
  int toMap, toX, toY, toDir;
  char enemies[64];
  int once;
  int ally; /* mob sojuszniczy */
  int pose; /* stała animacja NPC: AN_SIT / AN_DANCE / AN_PLAY (0 = zwykła) */
} EntDef;

/* region dużej mapy (dawne osobne mapy ulic) */
typedef struct { char id[24], name[64], music[24]; int map, x, y, w, h, district, ambient, rain, ex, ey, edir; } Region;

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
  Region regions[MAX_REGIONS]; int nregions;
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
int map_resolve(const char *id, int *x, int *y); /* mapa albo region: dodaje przesunięcie regionu */
int region_at(int map, float x, float z);          /* -1 = brak */
int region_find(const char *id);
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
void city_steam(float time);
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
  float ppx, ppz, pbob; /* poprzedni krok logiki (interpolacja rysowania) */
  int region;          /* region dużej mapy, w którym jest gracz (-1 brak) */
  char banner[64];     /* nazwa miejsca na banerze */
  int time, trans, tMap, tX, tY, tDir, grace;
  int bannerT;
  int hurtT, flashT;
  int enemiesAlive;
} World;
extern World W;

void world_audio(void);
/* samochody (vehicle.c) */
void cars_build(void);
void cars_init_map(void);
void cars_update(void);
void cars_draw(void);
void cars_lights(int night);
void cars_hud(void);
int cars_camera(RCam *cam, float alpha);
int cars_near(void);
int cars_enter(void);
int cars_block(float x, float z, float r);
int car_player(void);
void cars_showcase(void);
/* streetlife.c: przechodnie i kolejka nadziemna */
void streetlife_build(void);
void streetlife_map(void);
void streetlife_update(void);
void streetlife_lights(int night);
void streetlife_draw(void);
void combat_damage_ent(Ent *e, int dmg);
/* znacznik celu misji: mapa+kafel albo NPC (id); map<0 i id NULL = brak */
void world_marker_set(int map, int x, int y, const char *npcId);
int world_marker_pos(float *x, float *z); /* pozycja na bieżącej mapie (albo drzwi prowadzące do celu) */
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
void tycoon_script_cmd(const char *key, const char *arg, int n);
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
int tycoon_debug_beat(int b);
int tycoon_debug_mission(int type, int target);

/* ------------------------------------------------------------------ test / debug */
void testdrv_load(const char *path);
void testdrv_frame(void);
extern int g_testdrv;
extern int g_autoplay;
void game_debug(const char *cmd);
int story_fuzz(int rounds);

#endif
