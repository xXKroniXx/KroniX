/* Zasoby proceduralne: warstwy tekstur, modele rekwizytów, postacie 3D. */
#ifndef ASSETS_H
#define ASSETS_H
#include "render.h"

enum {
  L_WHITE, L_CLOTH, L_SKIN, L_ASPHALT, L_SIDEWALK, L_COBBLE, L_BRICK, L_BRICK_DARK, L_STONE, L_PLASTER,
  L_WALLPAPER_RED, L_WALLPAPER_GREEN, L_WOODFLOOR, L_PARQUET, L_CHECKER, L_CARPET, L_CONCRETE, L_ROOF,
  L_WINDOW, L_SHOPWIN, L_DOOR, L_AWNING, L_WOOD, L_WOOD_LIGHT, L_METAL, L_BRASS, L_CRATE, L_BARREL,
  L_CORRUGATED, L_WATER, L_POSTER1, L_POSTER2, L_POSTER3, L_SIGNS, L_NEON, L_TABLECLOTH, L_LEATHER,
  L_VELVET, L_BOOKS, L_STAINED, L_GRASS, L_TIN_CEILING, L_WAINSCOT, L_TILE_WHITE, L_FACES, L_FOLIAGE,
  L_FENCE, L_GRATE, L_SKYLINE, L_NEWSPAPER, L_MAPWALL, L_MARBLE, L_BOTTLES, L_PAINTING, L_FIRE, L_GRAVEL,
  L_COUNT
};

void tex_generate(void);          /* tworzy tablicę tekstur i ustawia ją w rendererze */
extern const char *SIGN_TEXT[16]; /* szyldy (8 rzędów po 2 w warstwie L_SIGNS) */

/* modele rekwizytów */
enum {
  MD_LAMP, MD_HYDRANT, MD_TRASH, MD_BENCH, MD_NEWSSTAND, MD_TABLE, MD_CHAIR, MD_BAR, MD_STOOL, MD_SHELF,
  MD_PIANO, MD_DESK, MD_BOOKCASE, MD_FIREPLACE, MD_CRATE, MD_CRATES, MD_BARREL, MD_VAT, MD_PEW, MD_ALTAR,
  MD_BED, MD_FENCE, MD_BARS, MD_MACHINE, MD_TREE, MD_PLANT, MD_CAR, MD_TRUCK, MD_CHANDELIER, MD_CANDLE,
  MD_MAILBOX, MD_PHONE, MD_RADIO, MD_GRAMOPHONE, MD_COUNTER, MD_SAFE, MD_RING, MD_TANK, MD_CEILLAMP,
  MD_SOFA, MD_BOAT, MD_LEDGER, MD_PHOTO, MD_MONEY, MD_BOTTLE, MD_BRIEFCASE, MD_SCONCE,
  MD_UPBASS, MD_DRUMKIT, MD_MIC, MD_BOXRING, MD_ROULETTE, MD_CHALKBOARD, MD_SAX, MD_COUNT
};
extern GMesh MODELS[MD_COUNT];
void models_build(void);
void model_add(MB *dst, int model, float x, float y, float z, float yaw, float scale, uint32_t tint);
extern MB MODEL_MB[MD_COUNT]; /* geometria CPU (do wtapiania w statyczną siatkę mapy) */
int objmodel_find(const char *name); /* model przedmiotu dla obiektu na mapie (np. "ledger") */

/* postacie */
#define NBONES 16
enum { B_ROOT, B_PELVIS, B_SPINE, B_HEAD, B_ARM_L, B_FORE_L, B_ARM_R, B_FORE_R, B_LEG_L, B_SHIN_L, B_LEG_R, B_SHIN_R, B_GUN };
enum { AN_IDLE, AN_WALK, AN_RUN, AN_AIM, AN_SHOOT, AN_HIT, AN_DEAD, AN_TALK, AN_SIT, AN_DANCE, AN_PLAY };
typedef struct {
  int anim;
  float t;          /* czas animacji */
  float aimPitch;
  int weapon;       /* 0 pięści, 1 pistolet, 2 strzelba, 3 tommy */
  float deadT;      /* 0..1 upadek */
  float hitT;
  float talk;
} HumanPose;
int human_find(const char *name);
int human_count(void);
const char *human_name(int i);
void humans_build(void);
void human_draw(int type, float x, float y, float z, float yaw, const HumanPose *pose, uint32_t tint);
unsigned human_portrait(int type); /* tekstura 256x256 z głową postaci */
void humans_portraits_build(void);

/* broń w widoku FPP */
void weapons_build(void);
void weapon_draw_fpp(int weapon, float bob, float kick, float reload, float swap, float punch, int flash);

#endif
