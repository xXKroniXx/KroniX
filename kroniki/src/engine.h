/* KroniX: Kroniki Mgły — wspólny nagłówek silnika.
 * Silnik jest niezależny od platformy: rysuje do bufora `fb` (384x216, ARGB),
 * czyta stan przycisków z `g_btn_raw` i generuje dźwięk w `audio_render`.
 * Warstwa platformy (Win32 / headless) tylko to wyświetla i odtwarza. */
#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 384
#define SCREEN_H 216
#define TILE 16
#define AUDIO_RATE 22050

typedef uint32_t u32;
typedef uint8_t u8;

#define RGB(r, g, b) ((u32)(0xFF000000u | ((u32)(r) << 16) | ((u32)(g) << 8) | (u32)(b)))
#define CLAMP(v, a, b) ((v) < (a) ? (a) : (v) > (b) ? (b) : (v))

/* ------------------------------------------------------------------ platform */
enum { BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B, BTN_RUN, BTN_COUNT };

extern int g_btn_raw[BTN_COUNT]; /* ustawiane przez platformę co klatkę */
extern int g_quit;               /* rdzeń prosi o zamknięcie */
extern int g_fullscreen_toggle;  /* rdzeń prosi o przełączenie pełnego ekranu */
extern char g_data_dir[512];     /* katalog na zapis gry (z ukośnikiem na końcu) */
extern u32 fb[SCREEN_W * SCREEN_H];

void game_init(int argc, char **argv);
void game_frame(void);
void audio_render(int16_t *out, int n);

/* ------------------------------------------------------------------ input */
typedef struct {
  int held[BTN_COUNT], pressed[BTN_COUNT], rep[BTN_COUNT], holdT[BTN_COUNT];
} Input;
extern Input in;
void input_update(void);
void input_clear(void);

/* ------------------------------------------------------------------ gfx */
typedef struct { int w, h; u32 *px; } Sprite; /* alfa 0 = przezroczysty */

u32 pal(char c);
u32 blend(u32 a, u32 b, int t); /* t: 0..255 */
Sprite *spr_new(int w, int h);
Sprite *spr_art(const char *const *rows, int n, const u32 *remap); /* remap['A'..'Z'] */
void gfx_clear(u32 c);
void gfx_pset(int x, int y, u32 c);
void gfx_pset_a(int x, int y, u32 c, int a);
void gfx_rect(int x, int y, int w, int h, u32 c);
void gfx_rect_a(int x, int y, int w, int h, u32 c, int a);
void gfx_vgrad(int x, int y, int w, int h, u32 top, u32 bot);
void gfx_circle(int cx, int cy, int r, u32 c, int a);
void gfx_blit(const Sprite *s, int x, int y, int flip, int scale);
void gfx_blit_fx(const Sprite *s, int x, int y, int flip, int scale, u32 tint, int tintAmt, int alpha);
void gfx_darken(int amt);
void gfx_window(int x, int y, int w, int h);
void gfx_shake(int amount);
extern int g_shake;
void save_bmp(const char *path);

/* ------------------------------------------------------------------ font */
#define LINE_H 11
void font_init(void);
int utf8_next(const char **p);
int text_width(const char *s);
int text_draw(int x, int y, const char *s, u32 col);
int text_draw_sh(int x, int y, const char *s, u32 col); /* z cieniem */
int text_draw_big(int x, int y, const char *s, u32 col, int scale);
int text_wrap(const char *s, int maxw, char out[][160], int maxlines);
int text_draw_n(int x, int y, const char *s, u32 col, int maxChars); /* typewriter */

/* ------------------------------------------------------------------ art */
enum { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP };
typedef struct { char name[24]; Sprite *fr[4][3]; } CharSprite;
void art_init(void);
Sprite *art_get(const char *name);    /* obiekty, wrogowie */
CharSprite *art_char(const char *name); /* postacie z animacją chodu */
Sprite *art_portrait(const char *name); /* twarz (klatka stojąca w dół) */
Sprite *art_tile(int c, int variant, int frame);
int tile_solid(int c);
int art_exists(const char *name);
int art_validate(void);
int tile_valid(int c);

/* ------------------------------------------------------------------ audio */
enum { SFX_BLIP, SFX_OK, SFX_CANCEL, SFX_HIT, SFX_CRIT, SFX_FIRE, SFX_HEAL, SFX_DOOR,
       SFX_CHEST, SFX_LEVEL, SFX_ENCOUNTER, SFX_FLEE, SFX_DIE, SFX_MAGIC, SFX_TEXT, SFX_COUNT };
void audio_init(void);
void music_play(const char *name);
void sfx_play(int id);
int sfx_find(const char *name);
int music_exists(const char *name);
extern int g_volume; /* 0..10 */

/* ------------------------------------------------------------------ story data */
#define MAX_MAPS 24
#define MAX_MAP_W 64
#define MAX_MAP_H 48
#define MAX_ENTS 72
#define MAX_ITEMS 48
#define MAX_ENEMIES 32
#define MAX_SPEAKERS 40
#define MAX_SCRIPTS 320
#define MAX_CMDS 6000
#define MAX_VARS 160
#define MAX_ARGS 10
#define MAX_QUESTS 16

enum { ENT_NPC, ENT_OBJ, ENT_WARP, ENT_EVENT, ENT_MOB };

typedef struct {
  int type;
  char id[24];
  int x, y, dir;
  char sprite[24];
  int script;           /* indeks skryptu albo -1 */
  int wander;
  int condVar[2], condNeg[2]; /* widoczność zależna od zmiennych (-1 = brak) */
  int toMap, toX, toY, toDir; /* warp */
  char enemies[64];     /* mob: lista wrogów, np. "wilk,wilk" */
  int once;             /* event: odpala się tylko raz */
} EntDef;

typedef struct {
  char id[24], name[48], music[24], bg[24];
  int w, h, night, rain;
  u8 tiles[MAX_MAP_H][MAX_MAP_W];
  EntDef ents[MAX_ENTS];
  int nents;
} MapDef;

enum { IT_HEAL, IT_MANA, IT_ELIXIR, IT_WEAPON, IT_ARMOR, IT_CHARM, IT_KEY };
typedef struct { char id[24], name[40], desc[120]; int type, power, price; } ItemDef;

enum { EL_NONE, EL_FIRE, EL_LIGHT };
enum { ES_DMG, ES_MAG, ES_DRAINHP, ES_DRAINMP, ES_HEAL };
typedef struct {
  char id[24], name[40], sprite[24];
  int hp, atk, def, spd, xp, gold;
  int weak, resist;
  char skill[40]; int skillType, skillPow, skillRate;
  char drop[24]; int dropRate;
  int boss;
  u32 tint; int tintAmt;
  int scale;
} EnemyDef;

typedef struct { char id[24], name[40], sprite[24]; } Speaker;

typedef struct {
  int op, n, line;
  int i[MAX_ARGS];
  const char *s[MAX_ARGS];
} Cmd;

typedef struct { char name[40]; int start, len; } Script;
typedef struct { const char *text; int target; int condType, condA, condB; } MenuOpt;
typedef struct { char var[32]; char label[40]; } StatDef;
#define MAX_OPTS 1200
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
  int errors;
} Story;
extern Story S;

int story_load(const char *text);
int cond_check(int type, int a, int b);
enum { CND_NONE, CND_IF, CND_IFNOT, CND_GE, CND_LT, CND_GOLD, CND_HAS };
enum { OP_END, OP_SAY, OP_AS, OP_MENU, OP_GOTO, OP_IF, OP_SET, OP_ADD, OP_GIVE, OP_TAKE, OP_CASH, OP_HEAL,
       OP_BATTLE, OP_WARP, OP_FADE, OP_WAIT, OP_SFX, OP_MUSIC, OP_SHAKE, OP_QUEST, OP_DONE, OP_SHOP,
       OP_SAVE, OP_ENDING, OP_LEARN, OP_FACE, OP_WALK, OP_CALL, OP_TOAST, OP_CHANCE, OP_HURT, OP_EQUIP, OP_NOP };
int map_find(const char *id);
int item_find(const char *id);
int enemy_find(const char *id);
int script_find(const char *name);
int speaker_find(const char *id);
int var_find(const char *name, int create);

/* ------------------------------------------------------------------ hero / game */
enum { SK_FIRE, SK_HEAL, SK_WHIRL, SK_LIGHT, SK_COUNT };
typedef struct { const char *id, *name, *desc; int mp, lvl; } SkillDef;
extern const SkillDef SKILLS[SK_COUNT];

typedef struct { char id[24]; char text[120]; int done; } Quest;

typedef struct {
  int lvl, xp, hp, mhp, mp, mmp, atk, def, spd;
  int weapon, armor, charm;
  int gold;
  int inv[MAX_ITEMS];
  int skills[SK_COUNT];
  int vars[MAX_VARS];
  Quest q[MAX_QUESTS]; int nq;
  long playFrames;
} Hero;
extern Hero H;

int hero_atk(void);
int hero_def(void);
int hero_mag(void);
int xp_next(int lvl);
int hero_gain_xp(int xp, char msgs[][160], int maxMsgs); /* zwraca liczbę komunikatów */
void quest_set(const char *id, const char *text);
void quest_done(const char *id);
int save_exists(void);
int save_game(void);
int load_game(void);
void hero_new(void);

enum { MODE_TITLE, MODE_WORLD, MODE_BATTLE, MODE_MENU, MODE_SHOP, MODE_GAMEOVER, MODE_ENDING };
extern int g_mode;
extern long g_frame;
void game_set_mode(int m);
void fade_to(int mode_after, void (*cb)(void));
extern int g_fade;

/* ------------------------------------------------------------------ world */
typedef struct { int tx, ty, px, py, dir, moving, prog, speed, anim, step; } Mover;
typedef struct {
  EntDef *d;
  Mover m;
  int vis, dead, wanderT, busy;
} Ent;
typedef struct {
  int map;
  Ent ents[MAX_ENTS]; int n;
  Mover hero;
  int camx, camy, bannerT, time, grace;
  int eventTile; /* ostatni kafelek, na którym odpalił event (anty-powtórka) */
} World;
extern World W;

void world_load_map(int map, int tx, int ty, int dir);
void world_update(void);
void world_draw(void);
void world_refresh_visibility(void);
Ent *world_find_ent(const char *id);
int world_blocked(int tx, int ty, const Ent *self);
void mover_step(Mover *m, int dir);
void mover_update(Mover *m);

/* ------------------------------------------------------------------ script VM */
void script_start(int idx, Ent *self);
int script_running(void);
void script_update(void);
void script_draw(void);
void script_battle_done(int result);
void script_stop(void);

/* ------------------------------------------------------------------ battle */
void battle_start(const char *enemyList, int noflee, const char *bg, int fromScript);
void battle_update(void);
void battle_draw(void);
void battle_draw_bg(const char *bg, int t);

/* ------------------------------------------------------------------ ui */
typedef struct {
  const char *items[16];
  int enabled[16];
  int n, cur, scroll, visible;
} Menu;
void menu_init(Menu *m);
void menu_add(Menu *m, const char *label, int enabled);
int menu_input(Menu *m); /* >=0 wybór, -1 nic, -2 anuluj */
void menu_draw(Menu *m, int x, int y, int w);
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

/* ------------------------------------------------------------------ test / debug */
void testdrv_load(const char *path);
void testdrv_frame(void);
extern int g_testdrv;
extern int g_autoplay; /* testy: dialogi same się przewijają, wybory losowe, walki wygrywane */
void game_debug(const char *cmd);
int story_fuzz(int rounds);

#endif
