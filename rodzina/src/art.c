/* Grafiki przedmiotów (billboardy w świecie 3D, ikony): pixel-art zdefiniowany w kodzie. */
#include "engine.h"

static void build_remap(u32 remap[26], const char *colors) {
  const char *defs = "Oi Ff Gh Ek Sk Pe Bv Ye Zk Tr Lw Mv Uv Aw Kk";
  for (int pass = 0; pass < 2; pass++)
    for (const char *p = pass ? colors : defs; p && p[0] && p[1];) {
      if (p[0] >= 'A' && p[0] <= 'Z' && p[1] != ' ') { remap[p[0] - 'A'] = pal(p[1]); p += 2; }
      else p++;
    }
}

/* ---------------------------------------------------------------- obiekty */
typedef struct { const char *name; const char *colors; int h; const char *rows[32]; } ArtDef;

static const ArtDef ART[] = {
  {"safe", "Oi As Bd Cz", 16, {
    "................", "................", "..OOOOOOOOOOOO..", "..OAAAAAAAAAAO..", "..OABBBBBBBBAO..",
    "..OABOOOOOOBAO..", "..OABOCCCCOBAO..", "..OABOCOOCOBAO..", "..OABOCOOCOBAO..", "..OABOCCCCOBAO..",
    "..OABOOOOOOBAO..", "..OABBBBBBBBAO..", "..OAAAAAAAAAAO..", "..OOOOOOOOOOOO..", "...OO......OO...", "................"}},
  {"safe_open", "Oi As Bd Kk Gg", 16, {
    "................", "................", "..OOOOOOOOOOOO..", "..OAAAAAAAAAAOO.", "..OAKKKKKKKKAOAO",
    "..OAKKKKKKKKAOAO", "..OAKGGKKKKKAOAO", "..OAKGGKKKKKAOAO", "..OAKKKKKKKKAOAO", "..OAKKKKKKKKAOAO",
    "..OAKKKKKKKKAOAO", "..OAKKKKKKKKAOAO", "..OAAAAAAAAAAOO.", "..OOOOOOOOOOOO..", "...OO......OO...", "................"}},
  {"crate", "Oi Au Bh", 16, {
    "................", "................", "..OOOOOOOOOOOO..", "..OAAAAAAAAAAO..", "..OABBBBBBBBAO..",
    "..OAABBBBBBAAO..", "..OABABBBBABAO..", "..OABBABBABBAO..", "..OABBBAABBBAO..", "..OABBBAABBBAO..",
    "..OABBABBABBAO..", "..OABABBBBABAO..", "..OAABBBBBBAAO..", "..OAAAAAAAAAAO..", "..OOOOOOOOOOOO..", "................"}},
  {"crate_open", "Oi Au Yy Kv", 16, {
    "................", "................", "..OOOOOOOOOOOO..", "..OAAAAAAAAAAO..", "..OAYYYYYYYYAO..",
    "..OAYKYYYKYYAO..", "..OAYKYYYKYYAO..", "..OAYYYYYYYYAO..", "..OAYYKYYYYKAO..", "..OAYYKYYYYKAO..",
    "..OAYYYYYYYYAO..", "..OAYYYYYYYYAO..", "..OAAAAAAAAAAO..", "..OOOOOOOOOOOO..", "................", "................"}},
  {"note", "Os Aw Bd Cm", 16, {
    "................", "................", "....OOOOOOO.....", "...OAAAAAAAO....", "...OABBBBBAAO...",
    "...OAAAAAAAAO...", "...OABBBBBBAO...", "...OAAAAAAAAO...", "...OABBBBAAAO...", "...OAAAAAAAAO...",
    "...OABBBBBBAO...", "...OAAAAAAACO...", "....OOAAAACCO...", "......OOOOO.....", "................", "................"}},
  {"ledger", "Oi Ax Bk Cz Dw", 16, {
    "................", "................", "................", "...OOOOOOOOOO...", "..OAAAAAAAAAAO..",
    "..OACCCCCCCCAO..", "..OACAAAAAACAO..", "..OACACCCCACAO..", "..OACAAAAAACAO..", "..OACCCCCCCCAO..",
    "..OAAAAAAAAAAO..", "..OBDDDDDDDDDO..", "..OBBBBBBBBBBO..", "...OOOOOOOOOO...", "................", "................"}},
  {"money", "Ox Ag Bl Cx Ww", 16, {
    "................", "................", "................", "................", "...OOOOOOOOOO...",
    "..OAAAAAAAAAAO..", "..OABBBAABBBAO..", "..OABCBAABCBAO..", "..OABBBWWBBBAO..", "..OAAAAWWAAAAO..",
    "..OABBBAABBBAO..", "..OAAAAAAAAAAO..", "...OOOOOOOOOO...", "................", "................", "................"}},
  {"bottle", "Oi Av Bo Co Dw Wr", 16, {
    "................", "......OOO.......", "......OAO.......", "......OBO.......", "......OBO.......",
    ".....OCCCO......", "....OCCCCCO.....", "....OCDDDCO.....", "....OCDWDCO.....", "....OCDDDCO.....",
    "....OCCCCCO.....", "....OCCCCCO.....", "....OCCCCCO.....", ".....OOOOO......", "................", "................"}},
  {"cap_item", "Oi Au Bv", 16, {
    "................", "................", "................", "................", "................",
    "......OOOOO.....", "....OOAAAAAO....", "...OAAAAAAAAO...", "..OAABAAAAAAAO..", "..OAAAAAAAAAAAOO",
    "...OBBBBBBBBBBBO", "....OOOOOOOOOOO.", "................", "................", "................", "................"}},
  {"tommy", "Oi Ae Bu Cd", 16, {
    "................", "................", "................", "................", "................",
    "......OO........", "OOOOOOAAOOOOOOO.", "OBBAAAAAAAAAAAAO", "OBBAAAAAAAAOOOO.", ".OOOOAOOCCO.....",
    ".....OAOCCCO....", "......OOCCCO....", ".......OCCO.....", "........OO......", "................", "................"}},
  {"pistol", "Oi Ae Bu Cd", 16, {
    "................", "................", "................", "................", "................",
    "................", "...OOOOOOOOOO...", "...OAAAAAAAAAO..", "...OAAAAAAAAOO..", "...OOAOOBBBO....",
    ".....OO.OBBBO...", "........OBBBO...", "........OOOO....", "................", "................", "................"}},
  {"hydrant", "Oi Ar Bj", 16, {
    "................", "................", "................", "......OOOO......", ".....OAAAAO.....",
    ".....OBBBBO.....", "....OOAAAAOO....", "...OBOAAAAOBO...", "...OBOAAAAOBO...", "....OOAAAAOO....",
    ".....OAAAAO.....", ".....OAAAAO.....", "....OBBBBBBO....", "....OOOOOOOO....", "................", "................"}},
  {"trash", "Oi As Bd", 16, {
    "................", "................", "................", "....OOOOOOOO....", "...OBBBBBBBBO...",
    "...OOOOOOOOOO...", "....OAABAABO....", "....OAABAABO....", "....OAABAABO....", "....OAABAABO....",
    "....OAABAABO....", "....OAABAABO....", "....OOOOOOOO....", "................", "................", "................"}},
  {"papers", "Oi Aw Bd Cs", 16, {
    "................", "................", "................", "................", "................",
    "...OOOOOOOOOO...", "..OAAAAAAAAAAO..", "..OABBBBABBBAO..", "..OAAAAAAAAAAO..", "..OABBABBBBBAO..",
    "..OAAAAAAAAAAO..", "..OCCCCCCCCCCO..", "..OAAAAAAAAAAO..", "..OCCCCCCCCCCO..", "...OOOOOOOOOO...", "................"}},
  {"radio", "Oi Au Bv Ch Zz", 16, {
    "................", "................", ".....OOOOOO.....", "...OOAAAAAAOO...", "..OAAAAAAAAAAO..",
    "..OABBBBBBBBAO..", "..OABCBCBCBBAO..", "..OABBBBBBBBAO..", "..OABCBCBCBBAO..", "..OABBBBBBBBAO..",
    "..OAAAAAAAAAAO..", "..OAAZAAAAZAAO..", "..OAAAAAAAAAAO..", "..OOOOOOOOOOOO..", "................", "................"}},
  {"sign", "Ou Av Bu Cy", 16, {
    "................", "................", "..OOOOOOOOOOOO..", ".OBBBBBBBBBBBBO.", ".OBAAAAAAAAAABO.",
    ".OBBBBBBBBBBBBO.", ".OBAAAAAAAAABBO.", ".OBBBBBBBBBBBBO.", "..OOOOOOOOOOOO..", "......OAAO......",
    "......OBAO......", "......OBAO......", "......OBAO......", ".....OOBAOO.....", "....OOOOOOOO....", "................"}},
  {"phone", "Oi Ak Bd Cz Ds", 16, {
    "................", ".....OOOOOO.....", "....OAAAAAAO....", "....OABBBBAO....", "....OABDDBAO....",
    "....OABDDBAO....", "....OABBBBAO....", "..OOOACCCCAO....", ".OAAOAAAAAAO....", ".OAOOACACACO....",
    ".OAO.OAAAAAO....", ".OAO.OACACAO....", ".OOO.OAAAAAO....", ".....OAAAAAO....", ".....OOOOOOO....", "................"}},
  {"sparkle", "Aw By", 16, {
    "................", "................", ".......A........", ".......A........", "................",
    "...B.......B....", "................", ".A.A...B...A.A..", "................", "...B.......B....",
    "................", ".......A........", ".......A........", "................", "................", "................"}},
};
#define NART ((int)(sizeof(ART) / sizeof(ART[0])))

typedef struct { const char *name; Sprite *s; } NamedSprite;
static NamedSprite sprites[64];
static int nsprites;

Sprite *art_get(const char *name) {
  for (int i = 0; i < nsprites; i++)
    if (!strcmp(sprites[i].name, name)) return sprites[i].s;
  return NULL;
}

int art_exists(const char *name) { return art_get(name) || actor_get(name); }

Sprite *art_portrait(const char *name) {
  Actor *a = actor_get(name);
  if (a) return a->portrait;
  return art_get(name);
}

void art_init(void) {
  for (int i = 0; i < NART && nsprites < 64; i++) {
    u32 remap[26] = {0};
    build_remap(remap, ART[i].colors);
    sprites[nsprites].name = ART[i].name;
    sprites[nsprites].s = spr_art(ART[i].rows, ART[i].h, remap);
    nsprites++;
  }
}
