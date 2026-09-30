/* Grafika gry zdefiniowana w kodzie: szablony postaci (przemalowywane paletą),
 * nakładki (broda, chusta, poroże...), obiekty, wrogowie i proceduralne kafelki. */
#include "engine.h"

/* ---------------------------------------------------------------- szablony postaci
 * Symbole: O kontur, H włosy/kaptur, I cień włosów, F skóra, G cień skóry, E oczy,
 * C ubranie, D cień ubrania, B pasek/akcent, P spodnie, S buty. Kropka = przezroczyste.
 * Kierunki: [0]=dół(przód) [1]=góra(tył) [2]=bok(prawo). Nogi (2 ostatnie wiersze)
 * mają osobne warianty dla klatek chodu. */
typedef struct {
  const char *name;
  const char *body[3][14];   /* wiersze 0..13 */
  const char *legs[3][3][2]; /* [kier][klatka][wiersz 14..15] */
} Template;

static const Template TMPL[] = {
  {"man",
   {{"................", ".....OOOOOO.....", "....OHHHHHHO....", "...OHHHHHHHHO...", "...OHIHHHHIHO...",
     "...OHFFFFFFHO...", "...OFEFFFFEFO...", "...OGFFFFFFGO...", "....OGFFFFGO....", "....ODCCCCDO....",
     "...ODCCCCCCDO...", "...OFDCBBCDFO...", "....OPPPPPPO....", "....OPPOOPPO...."},
    {"................", ".....OOOOOO.....", "....OHHHHHHO....", "...OHHHHHHHHO...", "...OHHHHHHHHO...",
     "...OHHHHHHHHO...", "...OIHHHHHHIO...", "...OIIHHHHIIO...", "....OGIIIIGO....", "....ODCCCCDO....",
     "...ODCCCCCCDO...", "...OFDCCCCDFO...", "....OPPPPPPO....", "....OPPOOPPO...."},
    {"................", ".....OOOOO......", "....OHHHHHO.....", "...OHHHHHHHO....", "...OHHHHHHHO....",
     "...OHHHFFFFO....", "...OHHFFFEFO....", "...OIHFFFFFO....", "....OIGFFGO.....", ".....ODCCO......",
     "....ODCCCDO.....", "....ODCFCBO.....", ".....OPPPO......", ".....OPPPO......"}},
   {{{"....OPPOOPPO....", "....OSSOOSSO...."}, {"....OPPOOSSO....", "....OSSO.OO....."}, {"....OSSOOPPO....", ".....OO.OSSO...."}},
    {{"....OPPOOPPO....", "....OSSOOSSO...."}, {"....OPPOOSSO....", "....OSSO.OO....."}, {"....OSSOOPPO....", ".....OO.OSSO...."}},
    {{".....OPPPO......", ".....OSSSSO....."}, {"....OPPOPPO.....", "....OSSOOSSO...."}, {".....OPPPO......", "....OSSOSSO....."}}}},
  {"woman",
   {{"................", ".....OOOOOO.....", "....OHHHHHHO....", "...OHHHHHHHHO...", "...OHIHHHHIHO...",
     "...OHFFFFFFHO...", "..OHFEFFFFEFHO..", "..OHGFFFFFFGHO..", "..OHOGFFFFGOHO..", "..OHODCCCCDOHO..",
     "...ODCCCCCCDO...", "...OFDCBBCDFO...", "...OCCCCCCCCO...", "...ODCCCCCCDO..."},
    {"................", ".....OOOOOO.....", "....OHHHHHHO....", "...OHHHHHHHHO...", "...OHHHHHHHHO...",
     "...OHHHHHHHHO...", "..OHHHHHHHHHHO..", "..OHIHHHHHHIHO..", "..OHIIHHHHIIHO..", "..OHODIIIIDOHO..",
     "...ODCCCCCCDO...", "...OFDCCCCDFO...", "...OCCCCCCCCO...", "...ODCCCCCCDO..."},
    {"................", ".....OOOOO......", "....OHHHHHO.....", "...OHHHHHHHO....", "...OHHHHHHHO....",
     "..OHHHHFFFFO....", "..OHHHFFFEFO....", "..OHHIFFFFFO....", "..OHHIGFFGO.....", "..OHHODCCO......",
     "...OHODCCCO.....", "....ODCFCBO.....", "....OCCCCCO.....", "....ODCCCDO....."}},
   {{{"...OODDDDDDOO...", ".....OSOOSO....."}, {"...OODDDDDDOO...", ".....OSO.OO....."}, {"...OODDDDDDOO...", "......OO.OSO...."}},
    {{"...OODDDDDDOO...", ".....OSOOSO....."}, {"...OODDDDDDOO...", ".....OSO.OO....."}, {"...OODDDDDDOO...", "......OO.OSO...."}},
    {{"....OODDDDO.....", ".....OSSSO......"}, {"....OODDDDO.....", "....OSO.OSO....."}, {"....OODDDDO.....", ".....OSSO......."}}}},
  {"child",
   {{"................", "................", "................", ".....OOOOOO.....", "....OHHHHHHO....",
     "....OHIHHIHO....", "....OHFFFFHO....", "....OFEFFEFO....", "....OGFFFFGO....", ".....OOGGOO.....",
     ".....ODCCDO.....", "....OFDBBDFO....", ".....OCCCCO.....", ".....ODCCDO....."},
    {"................", "................", "................", ".....OOOOOO.....", "....OHHHHHHO....",
     "....OHHHHHHO....", "....OHHHHHHO....", "....OIHHHHIO....", "....OGIIIIGO....", ".....OOGGOO.....",
     ".....ODCCDO.....", "....OFDCCDFO....", ".....OCCCCO.....", ".....ODCCDO....."},
    {"................", "................", "................", ".....OOOOO......", "....OHHHHHO.....",
     "....OHHHHHO.....", "....OHHFFFO.....", "....OHHFEFO.....", "....OIGFFGO.....", ".....OOGGO......",
     ".....ODCCO......", ".....ODFCO......", ".....OCCCO......", ".....ODCDO......"}},
   {{{".....OPOOPO.....", ".....OSOOSO....."}, {".....OPOOSO.....", ".....OSO.O......"}, {".....OSOOPO.....", "......O.OSO....."}},
    {{".....OPOOPO.....", ".....OSOOSO....."}, {".....OPOOSO.....", ".....OSO.O......"}, {".....OSOOPO.....", "......O.OSO....."}},
    {{".....OPPO.......", ".....OSSSO......"}, {"....OPOPO.......", "....OSOOSO......"}, {".....OPPO.......", ".....OSSO......."}}}},
  {"robe",
   {{"................", ".....OOOOOO.....", "....OHHHHHHO....", "...OHHHHHHHHO...", "...OHHIIIIHHO...",
     "...OHIFFFFIHO...", "...OHFEFFEFHO...", "...OHGFFFFGHO...", "...OHOGFFGOHO...", "..ODCHOOOOHCDO..",
     "..ODCCCCCCCCDO..", "..OFDCCBBCCDFO..", "...ODCCCCCCDO...", "...ODCCCCCCDO..."},
    {"................", ".....OOOOOO.....", "....OHHHHHHO....", "...OHHHHHHHHO...", "...OHHHHHHHHO...",
     "...OHHHHHHHHO...", "...OHHHHHHHHO...", "...OIHHHHHHIO...", "...OIIHHHHIIO...", "..ODCIIIIIICDO..",
     "..ODCCCCCCCCDO..", "..OFDCCCCCCDFO..", "...ODCCCCCCDO...", "...ODCCCCCCDO..."},
    {"................", ".....OOOOO......", "....OHHHHHO.....", "...OHHHHHHHO....", "...OHHHHIIHO....",
     "...OHHHIFFFO....", "...OHHHFFEFO....", "...OHHIGFFFO....", "...OIHOGFGO.....", "...ODCHOOO......",
     "...ODCCCCCO.....", "...ODCCFCBO.....", "...ODCCCCCO.....", "...ODCCCCCO....."}},
   {{{"...ODDCCCCDDO...", "....OOOOOOOO...."}, {"...ODDCCCCDDO...", "....OSOOOOOO...."}, {"...ODDCCCCDDO...", "....OOOOOOSO...."}},
    {{"...ODDCCCCDDO...", "....OOOOOOOO...."}, {"...ODDCCCCDDO...", "....OSOOOOOO...."}, {"...ODDCCCCDDO...", "....OOOOOOSO...."}},
    {{"...ODDDCCCDO....", "....OOOOOOO....."}, {"...ODDDCCCDO....", "....OSOOOOSO...."}, {"...ODDDCCCDO....", "....OOOSOOO....."}}}},
};
#define NTMPL ((int)(sizeof(TMPL) / sizeof(TMPL[0])))

/* Nakładki: 16 wierszy na kierunek ([0]=przód [1]=tył [2]=bok), '.' = bez zmian.
 * Litery: Y/Z kapelusz (główny/opaska), K daszek, T krawat, L biały kołnierz,
 * M wąsy/broda, U szelki, A fartuch. */
typedef struct { const char *name; const char *rows[3][16]; } Overlay;
static const Overlay OVL[] = {
  {"cap",
   {{0, ".....OOOOOO.....", "....OYYYYYYO....", "...OYYYYYYYYO...", "..OZZZZZZZZZZO..", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, ".....OOOOOO.....", "....OYYYYYYO....", "...OYYYYYYYYO...", "...OZYYYYYYZO...", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, ".....OOOOO......", "....OYYYYYO.....", "...OYYYYYYYO....", "...OZZZZZZZZZO..", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
  {"fedora",
   {{".....OOOOOO.....", "....OYYYYYYO....", "....OYYYYYYO....", "...OZZZZZZZZO...", ".OOYYYYYYYYYYOO.", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {".....OOOOOO.....", "....OYYYYYYO....", "....OYYYYYYO....", "...OZZZZZZZZO...", ".OOYYYYYYYYYYOO.", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {".....OOOOO......", "....OYYYYYO.....", "....OYYYYYO.....", "...OZZZZZZZO....", ".OOYYYYYYYYYYO..", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
  {"police",
   {{0, "...OOOOOOOOOO...", "..OYYYYYYYYYYO..", "...OYYYZZYYYO...", "...OKKKKKKKKO...", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, "...OOOOOOOOOO...", "..OYYYYYYYYYYO..", "...OYYYYYYYYO...", "...OYYYYYYYYO...", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, "...OOOOOOOOO....", "..OYYYYYYYYYO...", "...OYYYYYZYO....", "...OYYYKKKKKKO..", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
  {"toque",
   {{"....OOOOOOOO....", "...OYYYYYYYYO...", "...OYYYYYYYYO...", "....OZZZZZZO....", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {"....OOOOOOOO....", "...OYYYYYYYYO...", "...OYYYYYYYYO...", "....OZZZZZZO....", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {"....OOOOOOO.....", "...OYYYYYYYO....", "...OYYYYYYYO....", "....OZZZZZO.....", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
  {"tie",
   {{0, 0, 0, 0, 0, 0, 0, 0, 0, "......LTTL......", ".......TT.......", ".......TT.......", 0, 0, 0, 0},
    {0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, "........LT......", ".........T......", 0, 0, 0, 0, 0}}},
  {"mustache",
   {{0, 0, 0, 0, 0, 0, 0, ".....MMMMMM.....", 0, 0, 0, 0, 0, 0, 0, 0},
    {0},
    {0, 0, 0, 0, 0, 0, 0, "........MMM.....", 0, 0, 0, 0, 0, 0, 0, 0}}},
  {"beard",
   {{0, 0, 0, 0, 0, 0, 0, "...OMFFFFFFMO...", "...OMMMMMMMMO...", "....OMMMMMMO....", ".....OMMMMO.....", 0, 0, 0, 0, 0},
    {0},
    {0, 0, 0, 0, 0, 0, 0, "...OIHMFFFFO....", "....OMMMMMO.....", ".....OMMMMO.....", 0, 0, 0, 0, 0, 0}}},
  {"suspenders",
   {{0, 0, 0, 0, 0, 0, 0, 0, 0, "......U..U......", "......U..U......", "......U..U......", 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, "......U..U......", ".......UU.......", "......U..U......", 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, ".......U........", ".......U........", 0, 0, 0, 0, 0}}},
  {"collar",
   {{0, 0, 0, 0, 0, 0, 0, 0, 0, ".......LL.......", 0, 0, 0, 0, 0, 0},
    {0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, "........L.......", 0, 0, 0, 0, 0, 0}}},
  {"apron",
   {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "....OAAAAAAO....", "...OFAAAAAAFO...", "....OAAAAAAO....", "....OAAAAAAO....", 0, 0},
    {0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ".......AAO......", ".......AAO......", ".......AAO......", 0, 0, 0}}},
  {"kerchief",
   {{0, ".....OOOOOO.....", "....OYYZYYYO....", "...OYYYYYZYYO...", "...OYZYYYYYYO...", "...OYFFFFFFYO...", 0, 0, "....OGFYYFGO....", 0, 0, 0, 0, 0, 0, 0},
    {0, ".....OOOOOO.....", "....OYYZYYYO....", "...OYYYYYZYYO...", "...OYZYYYYYYO...", "...OYYYYZYYYO...", "...OYYYYYYYYO...", "....OYYOOYYO....", ".....OYO.OYO....", 0, 0, 0, 0, 0, 0, 0},
    {0, ".....OOOOO......", "....OYYZYYO.....", "...OYYYYYZYO....", "...OYZYYYYYO....", "...OYYYFFFFO....", "..OYYYFFFEFO....", "..OYOIFFFFFO....", 0, 0, 0, 0, 0, 0, 0, 0}}},
};
#define NOVL ((int)(sizeof(OVL) / sizeof(OVL[0])))

/* Obsada: szablon, kolory (pary litera->paleta), nakładki (po przecinku). */
typedef struct { const char *name, *tmpl, *colors, *ovl; } CharDef;
static const CharDef CHARS[] = {
  {"tomek", "man", "Hu Iv Cw Ds Pe Bv Yd Ze Uv", "cap,suspenders"},
  {"janek", "man", "Hy Iz Cc Db Pu Yu Zv", "cap"},
  {"mama", "woman", "Hs Id Cn Dk Bs Yb Zc", "kerchief"},
  {"zoska", "woman", "Hy Iz Cr Dj Aw", "apron"},
  {"ksiadz", "robe", "Hd Ie Ck Di Bk Lw", "collar"},
  {"bronek", "man", "Hs Id Cu Dv Pv Yu Zv Ms", "cap,mustache"},
  {"don", "man", "Hs Id Ck Di Pi Tr Lw Ms Sk", "tie,mustache"},
  {"vito", "man", "Hk Ie Cd De Pe Ty Lw Ye Zk", "fedora,tie"},
  {"lucia", "woman", "Hk Ie Cr Dj Bz", 0},
  {"sean", "man", "Ho Ir Cx Di Pi Tz Lw Yx Zk", "fedora,tie"},
  {"paddy", "man", "Ho Ir Cs Dd Pe Yx Ze Uk Mo", "cap,suspenders,mustache"},
  {"doyle", "man", "Hv Iu Cn Dk Pk Bz Yn Zz Mu", "police,mustache"},
  {"walsh", "robe", "Hv Iu Ch Du Bv Yv Zk", "fedora"},
  {"gliniarz", "man", "Hu Iv Cb Dn Pn Bk Yn Zz", "police"},
  {"zbir", "man", "Hk Ie Ce Di Pi Tk Lw Yi Zk", "fedora,tie"},
  {"zbir2", "man", "Ho Iu Cs Dd Pe Yx Ze Uk", "cap,suspenders"},
  {"dokowiec", "man", "Hu Iv Co Dj Pn Yb Zn", "cap"},
  {"bokser", "man", "Ho Iu Cf Dh Px Bw Sk", 0},
  {"piekarz", "man", "Hu Iv Cw Ds Pe Aw Yw Zs Mv", "toque,apron,mustache"},
  {"feliks", "man", "Hs Id Cv Dk Pe Ms Tn Lw", "tie,mustache"},
  {"stefek", "child", "Hu Iv Cb Dn Pv Yd Ze", "cap"},
  {"mickey", "man", "Hs Id Cs Dd Pe Yr Zj Ms", "cap,mustache"},
  {"kelner", "man", "Hk Ie Cw Ds Pk Tk Lw", "tie"},
  {"helena", "woman", "Hw Is Cp Dq Yp Zr", "kerchief"},
  {"pan", "man", "Hv Iu Cu Dv Pv Tr Lw Yu Zv", "fedora,tie"},
  {"pani", "woman", "Hk Ie Cc Db Bz", 0},
  {"robotnik", "man", "Hk Ie Cn Dk Pu Yu Zv", "cap"},
  {"menel", "robe", "Hd Ie Cu Dv Md", "beard"},
  {"mafioso", "man", "Hk Ie Ck Di Pi Tw Lw Yd Zk", "fedora,tie"},
  {"lucky", "man", "Hk Ie Cp Dq Pe Tz Lw Yq Zk", "fedora,tie"},
};
#define NCHARS ((int)(sizeof(CHARS) / sizeof(CHARS[0])))

static CharSprite charSprites[NCHARS];

static void build_remap(u32 remap[26], const char *colors) {
  const char *defs = "Oi Ff Gh Ek Sk Pe Bv Ye Zk Tr Lw Mv Uv Aw Kk";
  for (int pass = 0; pass < 2; pass++)
    for (const char *p = pass ? colors : defs; p && p[0] && p[1];) {
      if (p[0] >= 'A' && p[0] <= 'Z' && p[1] != ' ') { remap[p[0] - 'A'] = pal(p[1]); p += 2; }
      else p++;
    }
}

static Sprite *build_char_frame(const Template *t, const Overlay **ov, int nov, int d, int frame, const u32 *remap) {
  const char *rows[16];
  char buf[16][17];
  for (int r = 0; r < 16; r++) {
    const char *src = r < 14 ? t->body[d][r] : t->legs[d][frame][r - 14];
    memcpy(buf[r], src, 16);
    buf[r][16] = 0;
    for (int k = 0; k < nov; k++)
      if (ov[k]->rows[d][r]) {
        const char *o = ov[k]->rows[d][r];
        for (int c = 0; c < 16 && o[c]; c++)
          if (o[c] != '.') buf[r][c] = o[c];
      }
    rows[r] = buf[r];
  }
  Sprite *s = spr_art(rows, 16, remap);
  if (frame != 0) { /* podskok tułowia w klatkach chodu */
    for (int y = 0; y < 13; y++)
      for (int x = 0; x < 16; x++) s->px[y * 16 + x] = s->px[(y + 1) * 16 + x];
  }
  return s;
}

static Sprite *flip_sprite(const Sprite *s) {
  Sprite *f = spr_new(s->w, s->h);
  for (int y = 0; y < s->h; y++)
    for (int x = 0; x < s->w; x++) f->px[y * s->w + x] = s->px[y * s->w + (s->w - 1 - x)];
  return f;
}

static void build_chars(void) {
  for (int i = 0; i < NCHARS; i++) {
    const CharDef *cd = &CHARS[i];
    const Template *t = NULL;
    const Overlay *ov[4];
    int nov = 0;
    for (int k = 0; k < NTMPL; k++) if (!strcmp(TMPL[k].name, cd->tmpl)) t = &TMPL[k];
    if (cd->ovl) {
      char tmp[64];
      snprintf(tmp, sizeof tmp, "%s", cd->ovl);
      for (char *tok = strtok(tmp, ","); tok && nov < 4; tok = strtok(NULL, ","))
        for (int k = 0; k < NOVL; k++) if (!strcmp(OVL[k].name, tok)) ov[nov++] = &OVL[k];
    }
    if (!t) continue;
    u32 remap[26] = {0};
    build_remap(remap, cd->colors);
    CharSprite *cs = &charSprites[i];
    snprintf(cs->name, sizeof cs->name, "%s", cd->name);
    for (int f = 0; f < 3; f++) {
      cs->fr[DIR_DOWN][f] = build_char_frame(t, ov, nov, 0, f, remap);
      cs->fr[DIR_UP][f] = build_char_frame(t, ov, nov, 1, f, remap);
      cs->fr[DIR_RIGHT][f] = build_char_frame(t, ov, nov, 2, f, remap);
      cs->fr[DIR_LEFT][f] = flip_sprite(cs->fr[DIR_RIGHT][f]);
    }
  }
}

CharSprite *art_char(const char *name) {
  for (int i = 0; i < NCHARS; i++)
    if (!strcmp(charSprites[i].name, name)) return &charSprites[i];
  return NULL;
}

Sprite *art_portrait(const char *name) {
  CharSprite *c = art_char(name);
  if (c) return c->fr[DIR_DOWN][0];
  return art_get(name);
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

int art_exists(const char *name) { return art_get(name) || art_char(name); }

/* ---------------------------------------------------------------- kafelki proceduralne */
static u32 hash3(int x, int y, int z) {
  u32 h = (u32)x * 374761393u + (u32)y * 668265263u + (u32)z * 2147483647u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

static Sprite *T;
static void tp(int x, int y, u32 c) { if ((unsigned)x < 16 && (unsigned)y < 16) T->px[y * 16 + x] = c; }
static void tr(int x, int y, int w, int h, u32 c) { for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) tp(i, j, c); }
static void noise(u32 c, int n, int seed) { for (int i = 0; i < n; i++) { u32 h = hash3(i, seed, 77); tp(h % 16, (h >> 8) % 16, c); } }

#define TILE_CHARS ".-|,:\"_qro=ysRWwDGaLcC~Xk#itAhbSPVIdegTFpufBYx"
#define NVAR 4
#define NFRAMES 4
static Sprite *tiles[128][NVAR][NFRAMES];

#define ASPHALT RGB(0x3a, 0x3c, 0x48)
#define BRICK RGB(0x8a, 0x3e, 0x32)
#define MORTAR RGB(0x5e, 0x2a, 0x26)
#define WOOD RGB(0x7a, 0x4e, 0x32)

static void asphalt(int v) { tr(0, 0, 16, 16, ASPHALT); noise(RGB(0x32, 0x34, 0x3e), 16, v + 1); noise(RGB(0x48, 0x4a, 0x56), 8, v + 2); }
static void grass(int v) {
  tr(0, 0, 16, 16, RGB(0x2e, 0x8a, 0x4e));
  noise(RGB(0x26, 0x76, 0x42), 18, v * 3 + 1);
  noise(RGB(0x4a, 0xa8, 0x5e), 5, v * 3 + 2);
}
static void bricks(int v) {
  tr(0, 0, 16, 16, MORTAR);
  for (int y = 0; y < 16; y += 4)
    for (int x = (y / 4 % 2) * 4 - 4; x < 16; x += 8) {
      tr(x + 1, y, 7, 3, BRICK);
      tr(x + 1, y, 7, 1, RGB(0xa4, 0x52, 0x40));
    }
  noise(RGB(0x70, 0x30, 0x2a), 5, v + 9);
}
static void concrete(int v) { tr(0, 0, 16, 16, RGB(0x6a, 0x6a, 0x72)); noise(RGB(0x5c, 0x5c, 0x64), 18, v + 30); noise(RGB(0x7a, 0x7a, 0x82), 8, v + 31); }
static void woodfloor(int v) {
  tr(0, 0, 16, 16, WOOD);
  for (int y = 0; y < 16; y += 4) { tr(0, y + 3, 16, 1, pal('v')); tp((y * 5 + v * 3) % 16, y + 1, pal('v')); tr((y * 3 + v * 7) % 16, y, 1, 3, pal('v')); }
  noise(RGB(0x8e, 0x5e, 0x3e), 6, v + 20);
}
static void checker(void) {
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++) tp(x, y, ((x / 8) + (y / 8)) % 2 ? RGB(0x2a, 0x24, 0x30) : RGB(0xe8, 0xdc, 0xc4));
}

static void make_tile(int c, int v, int f) {
  T = spr_new(16, 16);
  switch (c) {
    case '.': asphalt(v); break;
    case '-': asphalt(v); tr(2, 7, 8, 2, RGB(0xc8, 0xb4, 0x60)); break;
    case '|': asphalt(v); tr(7, 2, 2, 8, RGB(0xc8, 0xb4, 0x60)); break;
    case ',': tr(0, 0, 16, 16, RGB(0x9a, 0x9a, 0xa2));
      tr(0, 7, 16, 1, RGB(0x7a, 0x7a, 0x84)); tr(0, 15, 16, 1, RGB(0x7a, 0x7a, 0x84));
      tr(7, 0, 1, 7, RGB(0x7a, 0x7a, 0x84)); tr(15, 8, 1, 7, RGB(0x7a, 0x7a, 0x84));
      noise(RGB(0xaa, 0xaa, 0xb2), 5, v + 3); noise(RGB(0x88, 0x88, 0x90), 4, v + 4);
      break;
    case ':': tr(0, 0, 16, 16, RGB(0x4a, 0x46, 0x50));
      for (int y = 0; y < 16; y += 4) for (int x = (y / 4 % 2) * 2; x < 16; x += 4) { tr(x, y, 3, 3, RGB(0x6a, 0x66, 0x70)); tp(x, y, RGB(0x80, 0x7c, 0x88)); }
      break;
    case '"': grass(v); break;
    case '_': woodfloor(v); break;
    case 'q': checker(); break;
    case 'r': tr(0, 0, 16, 16, RGB(0x8e, 0x24, 0x30));
      for (int y = 1; y < 16; y += 4) for (int x = (y / 4 % 2) * 4 + 1; x < 16; x += 8) tp(x, y, RGB(0xb4, 0x3a, 0x44));
      break;
    case 'o': concrete(v); if (v == 2) { tp(3, 4, pal('e')); tp(4, 5, pal('e')); tp(5, 5, pal('e')); tp(6, 6, pal('e')); } break;
    case '=': tr(0, 0, 16, 16, pal('i'));
      for (int x = 0; x < 16; x += 4) { tr(x, 0, 3, 16, RGB(0x86, 0x62, 0x44)); tr(x, 0, 1, 16, RGB(0x9e, 0x78, 0x56)); tp(x + 1, (x * 3 + v * 5) % 16, pal('v')); }
      break;
    case 'y': tr(0, 0, 16, 16, RGB(0xd8, 0xcc, 0xb0)); noise(RGB(0xc4, 0xb8, 0x9c), 10, v + 50); break;
    case 's': tr(0, 0, 16, 16, RGB(0x4a, 0x40, 0x3c));
      for (int y = 0; y < 16; y += 4) { tr(0, y, 16, 3, RGB(0x7a, 0x6e, 0x66)); tr(0, y, 16, 1, RGB(0x96, 0x8a, 0x80)); }
      break;
    case 'R': tr(0, 0, 16, 16, RGB(0x3c, 0x36, 0x3a)); noise(RGB(0x4c, 0x46, 0x4a), 20, v + 60); noise(RGB(0x30, 0x2a, 0x2e), 10, v + 61);
      if (v == 3) { tr(4, 4, 5, 5, RGB(0x5a, 0x54, 0x58)); tr(5, 3, 3, 1, pal('d')); } /* komin */
      break;
    case 'W': bricks(v); break;
    case 'w': bricks(v);
      tr(3, 2, 10, 11, pal('i'));
      tr(4, 3, 8, 9, (v % 2) ? RGB(0xf0, 0xc8, 0x6a) : RGB(0x2c, 0x3c, 0x5c));
      tr(7, 3, 2, 9, pal('i')); tr(4, 7, 8, 1, pal('i'));
      tr(2, 13, 12, 2, pal('s'));
      break;
    case 'D': bricks(v);
      tr(3, 1, 10, 15, pal('i')); tr(4, 2, 8, 14, RGB(0x5a, 0x34, 0x22));
      tr(5, 3, 6, 4, RGB(0xe0, 0xb8, 0x60)); tr(7, 3, 2, 4, pal('i'));
      tr(5, 9, 6, 5, RGB(0x4a, 0x2a, 0x1c)); tp(10, 10, pal('z'));
      break;
    case 'G': bricks(v);
      tr(1, 2, 14, 12, pal('i'));
      tr(2, 3, 12, 10, RGB(0x4c, 0x6c, 0x8c));
      tr(3, 4, 3, 1, RGB(0x9c, 0xc0, 0xdc)); tr(3, 5, 1, 3, RGB(0x9c, 0xc0, 0xdc));
      tr(3, 10, 10, 3, pal('u'));
      tp(5, 9, pal('y')); tp(6, 9, pal('y')); tp(9, 9, pal('r')); tp(10, 8, pal('w')); tp(10, 9, pal('w'));
      tr(1, 14, 14, 1, pal('s'));
      break;
    case 'a': for (int x = 0; x < 16; x++) tr(x, 0, 1, 12, (x / 3) % 2 ? RGB(0xe8, 0xe0, 0xd8) : RGB(0xb0, 0x30, 0x3c));
      for (int x = 0; x < 16; x += 3) tr(x, 12, 2, 1 + (x / 3) % 2, (x / 3) % 2 ? RGB(0xe8, 0xe0, 0xd8) : RGB(0xb0, 0x30, 0x3c));
      tr(0, 13, 16, 3, pal('i')); tr(0, 0, 16, 1, pal('j'));
      break;
    case 'L': tr(0, 0, 16, 16, RGB(0x9a, 0x9a, 0xa2));
      tr(0, 7, 16, 1, RGB(0x7a, 0x7a, 0x84)); tr(7, 0, 1, 7, RGB(0x7a, 0x7a, 0x84));
      tr(7, 4, 2, 11, pal('i')); tr(6, 14, 4, 2, pal('i'));
      tr(5, 1, 6, 4, pal('i')); tr(6, 2, 4, 2, pal('y')); tp(7, 2, pal('w'));
      break;
    case 'c': asphalt(v); /* tył auta */
      tr(1, 3, 15, 10, pal('i')); tr(2, 4, 14, 8, RGB(0x2a, 0x2a, 0x36));
      tr(3, 2, 4, 2, pal('i')); tr(3, 12, 4, 2, pal('i'));
      tr(8, 5, 5, 6, RGB(0x5a, 0x7a, 0x9a)); tr(8, 5, 5, 1, RGB(0x8a, 0xaa, 0xca));
      tr(1, 5, 1, 2, pal('r')); tr(1, 9, 1, 2, pal('r'));
      break;
    case 'C': asphalt(v); /* przód auta */
      tr(0, 3, 15, 10, pal('i')); tr(0, 4, 14, 8, RGB(0x2a, 0x2a, 0x36));
      tr(9, 2, 4, 2, pal('i')); tr(9, 12, 4, 2, pal('i'));
      tr(0, 5, 4, 6, RGB(0x5a, 0x7a, 0x9a));
      tr(6, 5, 7, 6, RGB(0x34, 0x34, 0x42)); tr(6, 7, 7, 2, RGB(0x44, 0x44, 0x52));
      tr(14, 4, 1, 8, pal('s')); tp(14, 5, pal('y')); tp(14, 10, pal('y'));
      break;
    case '~': tr(0, 0, 16, 16, RGB(0x1e, 0x3a, 0x52));
      for (int i = 0; i < 5; i++) {
        int y = (i * 4 + v * 2) % 16, x = (i * 7 + f * 2 + v * 5) % 16;
        tr(x, y, 4, 1, RGB(0x3a, 0x62, 0x82)); tp(x + 1, y > 0 ? y - 1 : 0, RGB(0x5a, 0x8a, 0xa8));
      }
      break;
    case 'X': concrete(v);
      tr(1, 1, 14, 14, pal('i')); tr(2, 2, 12, 12, RGB(0xa0, 0x74, 0x48));
      for (int i = 2; i < 14; i++) { tp(i, i, pal('v')); tp(15 - i, i, pal('v')); }
      tr(2, 2, 12, 1, RGB(0xc0, 0x94, 0x64)); tr(2, 13, 12, 1, pal('v'));
      break;
    case 'k': concrete(v);
      for (int y = 1; y < 15; y++)
        for (int x = 1; x < 15; x++) {
          int dx = x - 8, dy = y - 8, d = dx * dx + dy * dy;
          if (d < 44) tp(x, y, d > 34 ? pal('i') : d > 20 ? WOOD : d > 12 ? pal('d') : RGB(0x8e, 0x5e, 0x3e));
        }
      break;
    case '#': tr(0, 0, 16, 16, RGB(0x8a, 0x86, 0x7e)); noise(RGB(0x7a, 0x76, 0x6e), 12, v + 80);
      tr(0, 13, 16, 3, RGB(0x5a, 0x56, 0x50)); tr(0, 13, 16, 1, RGB(0x3a, 0x36, 0x30));
      break;
    case 'i': for (int x = 0; x < 16; x++) tr(x, 0, 1, 11, (x % 4) < 2 ? RGB(0x2e, 0x5a, 0x44) : RGB(0x26, 0x4c, 0x3a));
      for (int x = 1; x < 16; x += 4) tp(x, 3, pal('z'));
      tr(0, 11, 16, 5, pal('v')); tr(0, 11, 16, 1, RGB(0x9b, 0x6a, 0x48)); tr(0, 15, 16, 1, pal('i'));
      break;
    case 't': woodfloor(v);
      tr(1, 1, 14, 14, pal('i'));
      for (int y = 2; y < 14; y++) for (int x = 2; x < 14; x++) tp(x, y, ((x / 3) + (y / 3)) % 2 ? RGB(0xe8, 0xe0, 0xd4) : RGB(0xb8, 0x34, 0x3c));
      tr(7, 5, 2, 5, pal('o')); tr(7, 4, 2, 1, pal('y')); tp(10, 9, pal('w')); tp(11, 9, pal('w'));
      break;
    case 'A': tr(0, 0, 16, 16, RGB(0x6a, 0x60, 0x70));
      tr(1, 3, 14, 11, pal('i')); tr(2, 4, 12, 9, pal('w')); tr(2, 11, 12, 2, RGB(0xc8, 0xa8, 0x40));
      tr(7, 5, 2, 6, pal('z')); tr(5, 7, 6, 2, pal('z'));
      tp(3, 5, pal('y')); tp(12, 5, pal('y'));
      break;
    case 'h': woodfloor(v);
      tr(4, 3, 8, 10, pal('i')); tr(5, 4, 6, 8, RGB(0x6a, 0x3e, 0x26)); tr(5, 4, 6, 2, RGB(0x8a, 0x5a, 0x3a));
      tr(5, 12, 1, 3, pal('i')); tr(10, 12, 1, 3, pal('i'));
      break;
    case 'b': tr(0, 0, 16, 16, pal('i'));
      tr(0, 1, 16, 5, RGB(0x6a, 0x3e, 0x26)); tr(0, 1, 16, 1, RGB(0x9a, 0x6a, 0x46));
      tr(0, 6, 16, 9, RGB(0x4a, 0x2a, 0x1a));
      for (int x = 1; x < 16; x += 5) tr(x, 7, 3, 7, RGB(0x5a, 0x34, 0x22));
      tr(0, 15, 16, 1, pal('z'));
      if (v % 2) { tp(5, 2, pal('o')); tp(5, 3, pal('o')); tp(11, 3, pal('w')); }
      break;
    case 'S': tr(0, 0, 16, 16, RGB(0x3a, 0x22, 0x16));
      for (int y = 1; y < 15; y += 5) {
        for (int x = 1; x < 15; x += 2) {
          u32 col = (x + y + v) % 4 == 0 ? pal('o') : (x + y + v) % 4 == 1 ? pal('g') : (x + y + v) % 4 == 2 ? RGB(0xc8, 0x8a, 0x30) : pal('c');
          tr(x, y + 1, 1, 3, col); tp(x, y, pal('i'));
        }
        tr(0, y + 4, 16, 1, pal('u'));
      }
      break;
    case 'P': woodfloor(v);
      tr(0, 1, 16, 13, pal('i')); tr(1, 2, 14, 7, RGB(0x1c, 0x1a, 0x22)); tr(2, 3, 5, 2, RGB(0x3a, 0x38, 0x44));
      tr(1, 9, 14, 3, pal('w'));
      for (int x = 2; x < 15; x += 2) tr(x, 9, 1, 2, pal('i'));
      tr(1, 12, 14, 1, RGB(0x1c, 0x1a, 0x22));
      break;
    case 'V': concrete(v);
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
          int dx = x - 8, dy = y - 8, d = dx * dx + dy * dy;
          if (d < 60) tp(x, y, d > 48 ? pal('i') : d > 36 ? RGB(0xa0, 0x5a, 0x30) : (dx + dy < -2 ? RGB(0xe0, 0x9a, 0x5a) : RGB(0xc0, 0x74, 0x40)));
        }
      tr(6, 6, 4, 4, RGB(0x6a, 0x3a, 0x20)); tp(7, 7, pal('d'));
      break;
    case 'I': concrete(v);
      for (int x = 1; x < 16; x += 3) { tr(x, 0, 2, 16, pal('d')); tr(x, 0, 1, 16, pal('s')); }
      tr(0, 2, 16, 1, pal('e')); tr(0, 13, 16, 1, pal('e'));
      break;
    case 'd': tr(0, 0, 16, 16, RGB(0x6a, 0x6a, 0x72));
      tr(0, 1, 16, 13, pal('i')); tr(1, 2, 14, 10, RGB(0x6e, 0x46, 0x2c)); tr(1, 2, 14, 1, RGB(0x8e, 0x62, 0x44));
      tr(3, 4, 6, 5, pal('w')); tr(4, 5, 4, 1, pal('d')); tr(4, 7, 3, 1, pal('d'));
      tr(11, 3, 2, 4, pal('g')); tr(10, 3, 4, 1, pal('y'));
      break;
    case 'e': tr(0, 0, 16, 16, RGB(0x6a, 0x60, 0x70));
      tr(0, 3, 16, 9, pal('i')); tr(0, 4, 16, 4, RGB(0x5a, 0x34, 0x22)); tr(0, 8, 16, 3, RGB(0x4a, 0x2a, 0x1a)); tr(0, 4, 16, 1, RGB(0x7a, 0x4e, 0x32));
      break;
    case 'g': tr(0, 0, 16, 16, RGB(0x8a, 0x86, 0x7e));
      tr(3, 1, 10, 13, pal('i'));
      for (int y = 2; y < 13; y++) for (int x = 4; x < 12; x++) tp(x, y, ((x + y) % 3 == 0) ? pal('r') : ((x * y) % 5 == 0) ? pal('y') : (x < 8 ? pal('b') : pal('p')));
      tr(4, 2, 8, 1, pal('i')); tr(7, 2, 2, 11, pal('i'));
      tr(0, 14, 16, 2, RGB(0x5a, 0x56, 0x50));
      break;
    case 'T': grass(v);
      tr(6, 11, 4, 5, pal('v')); tr(7, 11, 1, 5, pal('u'));
      for (int y = 0; y < 13; y++)
        for (int x = 0; x < 16; x++) {
          int dx = x - 8, dy = y - 6, d = dx * dx + dy * dy;
          if (d < 50) tp(x, y, d > 38 ? pal('i') : (dx + dy < -3 ? pal('g') : pal('x')));
        }
      noise(pal('g'), 6, v + 50);
      break;
    case 'F': tr(0, 0, 16, 16, RGB(0x9a, 0x9a, 0xa2));
      tr(0, 3, 16, 1, pal('i')); tr(0, 12, 16, 1, pal('i'));
      for (int x = 1; x < 16; x += 3) { tr(x, 1, 1, 13, pal('i')); tp(x, 0, pal('d')); }
      break;
    case 'p': tr(0, 0, 16, 16, RGB(0x2a, 0x24, 0x30));
      tr(4, 10, 8, 6, pal('i')); tr(5, 10, 6, 5, RGB(0xa0, 0x5a, 0x30));
      for (int i = 0; i < 8; i++) { int x = 8 + (int)((i % 4) - 2) * 2, y = 2 + i; tr(x - 2, y, 5, 1, i % 2 ? pal('g') : pal('x')); }
      tp(3, 4, pal('g')); tp(12, 5, pal('g')); tp(2, 6, pal('x')); tp(13, 7, pal('x'));
      break;
    case 'u': tr(0, 0, 16, 16, pal('v'));
      tr(1, 0, 14, 16, pal('i'));
      for (int y = 1; y < 15; y += 5) {
        tr(2, y, 12, 4, pal('v'));
        for (int x = 2; x < 14; x += 2) tr(x, y + (x % 3 == 0), 1, 4 - (x % 3 == 0), (x / 2) % 3 == 0 ? pal('r') : (x / 2) % 3 == 1 ? pal('n') : pal('x'));
        tr(2, y + 4, 12, 1, pal('u'));
      }
      break;
    case 'f': bricks(v);
      tr(2, 4, 12, 12, pal('i')); tr(3, 5, 10, 10, RGB(0x1c, 0x14, 0x14));
      for (int i = 0; i < 6; i++) { int x = 4 + (i * 3 + f) % 8, h2 = 2 + (i + f) % 4; tr(x, 14 - h2, 2, h2, i % 2 ? pal('o') : pal('y')); }
      tr(1, 2, 14, 2, pal('s'));
      break;
    case 'B': woodfloor(v);
      tr(2, 0, 12, 16, pal('i')); tr(3, 1, 10, 14, pal('w')); tr(3, 1, 10, 4, pal('s'));
      tr(3, 6, 10, 9, RGB(0x6a, 0x7a, 0x6a)); tr(3, 6, 10, 1, RGB(0x4a, 0x5a, 0x4a));
      break;
    case 'Y': tr(0, 0, 16, 16, RGB(0xd8, 0xcc, 0xb0)); tr(5, 5, 6, 6, pal('i')); tr(6, 6, 4, 4, pal('r')); break;
    case 'x': tr(0, 0, 16, 16, pal('i')); break;
    default: tr(0, 0, 16, 16, RGB(255, 0, 255)); break;
  }
  tiles[c][v][f] = T;
}

Sprite *art_tile(int c, int variant, int frame) {
  if (c < 0 || c >= 128 || !strchr(TILE_CHARS, c)) c = 'x';
  return tiles[c][variant % NVAR][frame % NFRAMES];
}

int tile_solid(int c) {
  return strchr("RWwGaLcC~Xk#itAhbSPVIdegTFpufBYx", c) != NULL;
}

int tile_valid(int c) { return c > 0 && c < 128 && strchr(TILE_CHARS, c) != NULL; }

int art_validate(void) {
  int bad = 0;
  for (int i = 0; i < NART; i++) {
    int w = (int)strlen(ART[i].rows[0]);
    for (int r = 0; r < ART[i].h; r++)
      if (!ART[i].rows[r] || (int)strlen(ART[i].rows[r]) != w) { fprintf(stderr, "art %s row %d\n", ART[i].name, r); bad++; }
  }
  for (int t = 0; t < NTMPL; t++)
    for (int d = 0; d < 3; d++) {
      for (int r = 0; r < 14; r++) if (strlen(TMPL[t].body[d][r]) != 16) { fprintf(stderr, "tmpl %s d%d r%d\n", TMPL[t].name, d, r); bad++; }
      for (int f = 0; f < 3; f++) for (int r = 0; r < 2; r++) if (strlen(TMPL[t].legs[d][f][r]) != 16) { fprintf(stderr, "tmpl %s legs d%d f%d\n", TMPL[t].name, d, f); bad++; }
    }
  for (int o = 0; o < NOVL; o++)
    for (int d = 0; d < 3; d++)
      for (int r = 0; r < 16; r++) if (OVL[o].rows[d][r] && strlen(OVL[o].rows[d][r]) != 16) { fprintf(stderr, "ovl %s d%d r%d\n", OVL[o].name, d, r); bad++; }
  return bad;
}

void art_init(void) {
  build_chars();
  for (int i = 0; i < NART && nsprites < 64; i++) {
    u32 remap[26] = {0};
    build_remap(remap, ART[i].colors);
    sprites[nsprites].name = ART[i].name;
    sprites[nsprites].s = spr_art(ART[i].rows, ART[i].h, remap);
    nsprites++;
  }
  for (const char *p = TILE_CHARS; *p; p++)
    for (int v = 0; v < NVAR; v++)
      for (int f = 0; f < NFRAMES; f++) {
        int anim = (*p == '~' || *p == 'f');
        if (!anim && f > 0) { tiles[(int)*p][v][f] = tiles[(int)*p][v][0]; continue; }
        make_tile(*p, v, f);
      }
}
