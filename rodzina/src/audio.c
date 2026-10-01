/* Dźwięk: syntezator "małego zespołu jazzowego" + efekty + ambient miasta, stereo 44,1 kHz.
 *
 * Muzyka zapisana w MML (po jednej linii na kanał, do 6 kanałów):
 *   t tempo, o oktawa, < > zmiana oktawy, l długość domyślna, v głośność 0-15,
 *   @ instrument (patrz enum INS_*), S1 swing ósemek, nuty a-g (+/# krzyżyk, - bemol),
 *   r pauza, liczba = długość (4 ćwierćnuta), kropka = przedłużenie,
 *   (ceg)4 akord (oktawa wraca po nawiasie), [ ... ]n powtórzenie.
 * Instrument @4 (perkusja): c stopa, c+ rimshot, d szczotka/werbel, d+ werbel mocny,
 *   e szczotka "szur", f hi-hat, f+ hi-hat otwarty, g hi-hat stopą, g+ tom, a ride,
 *   a+ dzwon ride, b crash; w oktawie <= 2: c gong, d/e klocki drewniane.
 *
 * Efekty dźwiękowe są syntetyzowane raz przy starcie do buforów (z wariantami),
 * a odtwarzane z lekką losową zmianą wysokości. Wszystko przechodzi przez pogłos. */
#include "engine.h"
#include <math.h>
#include <ctype.h>

int g_volume = 7;

#define SR AUDIO_RATE
#define TWO_PI 6.28318530718f
#define NMUS 6

enum { INS_PIANO, INS_TRUMPET, INS_SAX, INS_BASS, INS_DRUM, INS_STRINGS, INS_ACCORD, INS_PLUCK, INS_ORGAN, INS_VIBES, INS_BRASS, INS_COUNT };
static const float INS_GAIN[INS_COUNT] = {0.34f, 0.34f, 0.32f, 0.42f, 0.75f, 0.16f, 0.2f, 0.34f, 0.2f, 0.45f, 0.18f};
static const float INS_PAN[INS_COUNT] = {-0.4f, 0.45f, 0.45f, 0.0f, 0.0f, -0.2f, 0.35f, -0.45f, 0.0f, 0.5f, 0.25f};

/* ---------------------------------------------------------------- narzędzia DSP */
#define SINN 4096
static float SINT[SINN + 1];
static inline float fsin(float ph) {
  ph -= (float)(int)ph;
  if (ph < 0) ph += 1;
  float x = ph * SINN;
  int i = (int)x;
  return SINT[i] + (SINT[i + 1] - SINT[i]) * (x - i);
}
static inline float rnd_f(unsigned *s) {
  *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
  return (float)(*s & 0xFFFFFF) / 8388608.0f - 1.0f;
}
static unsigned grng = 0x9E3779B9u;
static float frand(void) { return rnd_f(&grng) * 0.5f + 0.5f; } /* 0..1 */
static inline float lp_coef(float hz) { float c = 1.0f - expf(-TWO_PI * hz / SR); return c; }
static inline float softclip(float x) {
  if (x > 3) return 1;
  if (x < -3) return -1;
  return x * (27 + x * x) / (27 + 9 * x * x);
}
/* rezonator 2-biegunowy (formanty, klocki, kroki) */
typedef struct { float b0, a1, a2, y1, y2; } Reso;
static void reso_set(Reso *r, float hz, float bw) {
  float rr = expf(-3.14159f * bw / SR), w = TWO_PI * hz / SR;
  r->a1 = 2 * rr * cosf(w); r->a2 = -rr * rr; r->b0 = (1 - rr) * 1.2f;
}
static inline float reso(Reso *r, float x) {
  float y = r->b0 * x + r->a1 * r->y1 + r->a2 * r->y2;
  r->y2 = r->y1; r->y1 = y;
  return y;
}

/* ---------------------------------------------------------------- MML */
typedef struct { int ticks; float freq[4]; u8 nf, vol, inst, rest, drum[4]; } Note;
typedef struct { Note *n; int count; int tickSamples; } Seq;

static float note_freq(int octave, int semi) {
  int midi = (octave + 1) * 12 + semi;
  return 440.0f * powf(2.0f, (midi - 69) / 12.0f);
}
static int parse_num(const char **p, int def) {
  if (!isdigit((unsigned char)**p)) return def;
  int v = 0;
  while (isdigit((unsigned char)**p)) v = v * 10 + (*(*p)++ - '0');
  return v;
}
static int swing_map(int p) {
  int b = p % 48, base = p - b;
  return b >= 24 ? base + 32 + (b - 24) * 16 / 24 : base + b * 32 / 24;
}
static int parse_pitch(const char **p, char c, int oct, float *f, u8 *drum) {
  static const int semis[7] = {9, 11, 0, 2, 4, 5, 7};
  int semi = semis[c - 'a'];
  if (**p == '+' || **p == '#') { semi++; (*p)++; }
  else if (**p == '-') { semi--; (*p)++; }
  *f = note_freq(oct, semi);
  int s = (semi + 12) % 12;
  *drum = (u8)(oct <= 2 ? 12 + (s == 0 ? 0 : s <= 2 ? 1 : 2) : s);
  return 1;
}

static void compile(const char *src, Seq *out) {
  Note *buf = (Note *)calloc(4096, sizeof(Note));
  int n = 0, oct = 4, len = 48, vol = 10, inst = 0, tempo = 120, swing = 0, pos = 0;
  const char *p = src;
  while (*p && n < 4090) {
    if (*p == 'S') { p++; swing = parse_num(&p, 1); continue; }
    char c = (char)tolower((unsigned char)*p);
    p++;
    if (c == '(' || (c >= 'a' && c <= 'g') || c == 'r') {
      Note *nt = &buf[n];
      memset(nt, 0, sizeof *nt);
      if (c == '(') {
        int o2 = oct;
        while (*p && *p != ')') {
          char d = (char)tolower((unsigned char)*p++);
          if (d == '>') o2++;
          else if (d == '<') o2--;
          else if (d >= 'a' && d <= 'g' && nt->nf < 4) { parse_pitch(&p, d, o2, &nt->freq[nt->nf], &nt->drum[nt->nf]); nt->nf++; }
        }
        if (*p == ')') p++;
      } else if (c != 'r') {
        parse_pitch(&p, c, oct, &nt->freq[0], &nt->drum[0]);
        nt->nf = 1;
      }
      int l = parse_num(&p, 0);
      int ticks = l ? 192 / l : len;
      if (*p == '.') { ticks = ticks * 3 / 2; p++; }
      int start = swing ? swing_map(pos) : pos, end = swing ? swing_map(pos + ticks) : pos + ticks;
      pos += ticks;
      nt->ticks = end - start;
      nt->rest = c == 'r' || !nt->nf;
      nt->vol = (u8)vol;
      nt->inst = (u8)inst;
      n++;
    } else if (c == 'o') oct = parse_num(&p, 4);
    else if (c == '>') oct++;
    else if (c == '<') oct--;
    else if (c == 'l') { int l = parse_num(&p, 4); len = 192 / (l ? l : 4); if (*p == '.') { len = len * 3 / 2; p++; } }
    else if (c == 'v') vol = parse_num(&p, 10);
    else if (c == '@') inst = parse_num(&p, 0);
    else if (c == 't') tempo = parse_num(&p, 120);
  }
  out->n = buf;
  out->count = n;
  out->tickSamples = SR * 60 / (tempo * 48);
  if (out->tickSamples < 1) out->tickSamples = 1;
}

/* Rozwija [ ... ]n w tekst przed kompilacją. */
static char *expand(const char *src) {
  size_t cap = strlen(src) * 40 + 64;
  char *out = (char *)malloc(cap);
  size_t o = 0;
  for (const char *p = src; *p;) {
    if (*p == '[') {
      int depth = 1;
      const char *q = p + 1;
      while (*q && depth) { if (*q == '[') depth++; else if (*q == ']') depth--; q++; }
      size_t inner = (size_t)(q - p - 2);
      char *tmp = (char *)malloc(inner + 1);
      memcpy(tmp, p + 1, inner);
      tmp[inner] = 0;
      char *ex = expand(tmp);
      free(tmp);
      int times = 0;
      while (isdigit((unsigned char)*q)) times = times * 10 + (*q++ - '0');
      if (!times) times = 2;
      size_t el = strlen(ex);
      for (int t = 0; t < times && o + el + 2 < cap; t++) { memcpy(out + o, ex, el); o += el; out[o++] = ' '; }
      free(ex);
      p = q;
    } else {
      if (o + 2 < cap) out[o++] = *p;
      p++;
    }
  }
  out[o] = 0;
  return out;
}

/* ---------------------------------------------------------------- utwory */
typedef struct { const char *name; int loop; float reverb, vinyl; const char *ch[NMUS]; } Track;

static const Track TRACKS[] = {
  /* Ballada noir w a-moll: trąbka z tłumikiem, fortepian, kontrabas, szczotki. */
  {"tytul", 1, 0.45f, 1.0f, {
    "t66 @1 v11 o4 r4e4a4b4 >c2.<b8a8 a4.g8f4e-4 e2.r4 r4c4e4a4 g4.f8e4d4 f2e4d4 e2.r4 "
    "r4a4>c4e4< >d2.c8<b8 a4.g8f4d4 g+2e4r4 a4>c4e4d8c8< a4.f8e-2 d4e4f4g+4 a2.r4",
    "t66 @3 v12 o2 a2e2 g2e2 f2c2 e2<b2> a2e2 d2a2 g2d2 c2g2 f2c2 d2a2 <b2>f2 e2<b2> a2e2 f2c2 e2g+2 a1",
    "t66 @0 v6 o3 (gb>ce)2.(gb>ce)4 (ga>ce)2.(ga>ce)4 (e-ga>c)2.(e-ga>c)4 (dfg+b)2.(dfg+b)4 "
    "(gb>ce)2.(gb>ce)4 (fa>ce)2.(fa>ce)4 (fab>e)2.(fab>e)4 (egb>d)2.(egb>d)4 "
    "(ega>c)2.(ega>c)4 (fa>ce)2.(fa>ce)4 (dfa)2.(dfa)4 (dfg+b)2.(dfg+b)4 "
    "(gb>ce)2.(gb>ce)4 (e-ga>c)2.(e-ga>c)4 (dfg+b)2.(dfg+b)4 (gb>ce)1",
    "t66 @4 v5 o4 [(ce)4(dg)4e4(dg)4]15 (ce)4(dg)4(a)4r4",
    "t66 @5 v4 o4 r1 r1 r1 r1 e1 f1 f1 e1 e1 f1 f1 g+1 e1 e-1 d1 c1",
    0}},
  /* Polonia (Milwaukee Avenue): melancholijny mazurek — akordeon i kontrabas. */
  {"polonia", 1, 0.3f, 0, {
    "t150 @6 v10 o4 d4g4b-4 a4.g8f+4 a2d4 c4<b-4a4> b-4a4g4 g4>c4e-4< f+4a4>c4< b-2. "
    "d4f4b-4 a4.g8e-4 f4d4<b-4> g2. e-4g4>c4< b-4.a8g4 f+4a4>d4< g2.",
    "t150 @3 v12 o2 g4r2 d4r2 d4r2 <a4>r2 g4r2 c4r2 d4r2 g4r2 <b-4>r2 f4r2 <b-4>r2 g4r2 c4r2 g4r2 d4r2 g4r2",
    "t150 @6 v5 o3 r4(b->dg)4(b->dg)4 r4(b->dg)4(b->dg)4 r4(af+>c)4(af+>c)4 r4(af+>c)4(af+>c)4 "
    "r4(b->dg)4(b->dg)4 r4(g>ce-)4(g>ce-)4 r4(af+>c)4(af+>c)4 r4(b->dg)4(b->dg)4 "
    "r4(fb->d)4(fb->d)4 r4(a>ce-)4(a>ce-)4 r4(fb->d)4(fb->d)4 r4(b->dg)4(b->dg)4 "
    "r4(g>ce-)4(g>ce-)4 r4(b->dg)4(b->dg)4 r4(af+>c)4(af+>c)4 r4(b->dg)4(b->dg)4",
    "t150 @4 v4 o4 [c4f4f4]16",
    "t150 @2 v6 o4 r2. r2. r2. r2. r2. r2. r2. r2. f2. e-2. d2. d2. c2. d2. c2. <b-2.>",
    0}},
  /* Speakeasy: blues w F, swing — saksofon, potem trąbka; walking bass, charleston, ride. */
  {"bar", 1, 0.3f, 0, {
    "t168 S1 @2 v11 o4 r8a-8a8>c8r8c4<a8 b-4a-4f4r4 r8a-8a8>c8e-8c8<a8f8 g4f4r2 "
    "r8d8f8a-8b-4a-8f8 d8f8r4r2 r8a-8a8>c8r8c4<a8 f+4a4>c4e-4< d4b-4g8f8d4 e4g4b-4>c4< a8f8r4r8c8d8f8 a-8a8f2r4 "
    "@1 v10 r8a-8a8>c8r8c4<a8 b-4a-4f4r4 r8a-8a8>c8e-8c8<a8f8 g4f4r2 "
    "r8d8f8a-8b-4a-8f8 d8f8r4r2 r8a-8a8>c8r8c4<a8 f+4a4>c4e-4< d4b-4g8f8d4 e4g4b-4>c4< a8f8r4r8c8d8f8 a-8a8f2r4",
    "t168 S1 @3 v12 o2 [f4a4>c4<b4 b-4>d4f4<e4 f4a4>c4e-4< >d4c4<a4b4 b-4a-4g4f4 e-4d4c4e4 f4a4>c4c+4< >d4c4<a4f+4 "
    "g4b-4>d4c+4< >c4<b-4g4e4 f4a4d4f+4 g4b-4>c4<e4]2",
    "t168 S1 @0 v6 o3 [(e-a>d)4.(e-a>d)8r2 (da->c)4.(da->c)8r2 (e-a>d)4.(e-a>d)8r2 (e-a>d)4.(e-a>d)8r2 "
    "(da->c)4.(da->c)8r2 (da->c)4.(da->c)8r2 (e-a>d)4.(e-a>d)8r2 (>cf+b)4.(>cf+b)8r2 "
    "(fb->d)4.(fb->d)8r2 (eb->d)4.(eb->d)8r2 (e-a>d)4.(e-a>d)8r4 (>cf+b)4 (fb->d)4.(fb->d)8r4 (eb->d)4]2",
    "t168 S1 @4 v6 o4 (ab)4(ag)8a8a4(ag)8a8 [a4(ag)8a8a4(ag)8a8]23",
    0, 0}},
  /* Mała Italia (Taylor Street): neapolitański walc — akordeon, mandolina, kontrabas. */
  {"italia", 1, 0.3f, 0, {
    "t138 @6 v10 o4 a4d4f4 a2g4 f4e4d4 c+2. e4g4b-4 a2g4 f4e4c+4 d2. a4>d4f4 e2d4 c4<b-4a4 g2. b-4a4g4 f2e4 d4e4c+4 d2.",
    "t138 @3 v12 o2 d4r2 <a4>r2 d4r2 <a4>r2 <a4>r2 d4r2 <a4>r2 d4r2 d4r2 c4r2 f4r2 g4r2 g4r2 d4r2 <a4>r2 d4r2",
    "t138 @7 v5 o4 r4(fa>d)4(fa>d)4 r4(fa>d)4(fa>d)4 r4(fa>d)4(fa>d)4 r4(c+eg)4(c+eg)4 r4(c+eg)4(c+eg)4 "
    "r4(fa>d)4(fa>d)4 r4(c+eg)4(c+eg)4 r4(fa>d)4(fa>d)4 r4(fa>d)4(fa>d)4 r4(egb-)4(egb-)4 r4(fa>c)4(fa>c)4 "
    "r4(gb->d)4(gb->d)4 r4(gb->d)4(gb->d)4 r4(fa>d)4(fa>d)4 r4(c+eg)4(c+eg)4 r4(fa>d)4(fa>d)4",
    "t138 @4 v3 o4 [r4f4f4]16",
    0, 0}},
  /* Doki: mgła nad rzeką — smyczki, puls kontrabasu, wibrafon. */
  {"doki", 1, 0.55f, 0, {
    "t70 @5 v6 o3 (dfa)1 (dfa)1 (<b->df)1 (<b->df)1 (<gb->d)1 (<gb->d)1 (<ac+e)1 (<ac+e)1",
    "t70 @3 v12 o2 [d4.d8r2]2 [<b-4.b-8>r2]2 [<g4.g8>r2]2 [<a4.a8>r2]2",
    "t70 @9 v8 o5 r2a4r4 f2.r4 r2g4r4 d2.r4 r2b-4a4 g2.r4 r4e4c+4e4 <a2.>r4",
    "t70 @4 v4 o4 [c4c8r8r2]8",
    0, 0}},
  /* Walka: szybki swing w f-moll — sekcja dęta, walking bass, werbel. */
  {"walka", 1, 0.25f, 0, {
    "t176 S1 @1 v10 o5 r2c8c8c4 d-4.c8r2 r2d-8d-8d-4 e-4.d-8r2 r2f8f8f4 e4.c8r2 r4a-8g8f8e-8c8<a-8> <b4>r4r2",
    "t176 S1 @3 v12 o2 f4a-4>c4e-4< >f4e-4c4<b4 b-4>d-4f4a-4< >g4f4e-4d4< d-4f4a-4b4 >c4<b-4g4e4 f4a-4>c4d-4< >c4<b-4g4e4",
    "t176 S1 @10 v9 o4 r8(a->ce-)8r4r8(a->ce-)8r4 r8(a->ce-)8r4r8(a->ce-)8r4 r8(a->d-f)8r4r8(a->d-f)8r4 r8(a->d-f)8r4r8(a->d-f)8r4 "
    "r8(fa-b)8r4r8(fa-b)8r4 r8(eb->d)8r4r8(eb->d)8r4 r8(a->ce-)8r4r8(a->ce-)8r4 r8(eb->d)8r4r8(eb->d)8r4",
    "t176 S1 @4 v6 o4 [(ac)4(ad)8a8(ac)4(ad)8a8]7 (ac)4(ad)8a8(d+)8(d)8(d+)8(d+)8",
    0, 0}},
  /* Boss: groźne c-moll — ostinato, dęte, smyczki, mocny werbel. */
  {"boss", 1, 0.3f, 0, {
    "t150 @10 v10 o4 g2a-4g4 f+2.r4 g4>c4e-4d4< c2.r4 a-2g4f4 e-2.r4 d4f4a-4g4 b2r2",
    "t150 @3 v13 o2 [c8c8>c8<c8e-8c8f+8g8]4 [<a-8a-8>a-8<a-8>c8<a-8>e-8d8]2 [<g8g8>g8<g8>b8<g8>d8f8]2",
    "t150 @5 v6 o3 (ce-g)1 (ce-g)1 (ce-g)1 (ce-g)1 (ce-a-)1 (ce-a-)1 (dfgb)1 (dfgb)1",
    "t150 @4 v7 o4 (bc)4(cd+)8c8c4d+4 [c4(cd+)8c8c4d+4]7",
    0, 0}},
  {"wygrana", 0, 0.35f, 0, {
    "t120 @1 v12 o5 c8c8c8d4.e8g2.",
    "t120 @10 v9 o4 (ceg)8(ceg)8(ceg)8(dfa)4.(egb)8(g>ce)2.",
    "t120 @3 v12 o2 c8c8c8d4.e8c2.",
    "t120 @4 v7 o4 (cb)8d8d8d4.d8(cb)2.",
    0, 0}},
  /* Zakończenie: słodko-gorzka ballada — fortepian i smyczki. */
  {"koniec", 1, 0.5f, 0.6f, {
    "t60 @0 v9 o4 e4.d8c4g4 a2.g4 f4.e8d4c4 d2.r4 e4.d8c4>c4< b2g2 a4.g8f4a4 g2.r4 "
    ">c4.<b8a4e4 g2b2 a4.g8f4e4 e2g2 f4.e8d4a4 g4f4d4<b4> c2.r4 r1",
    "t60 @3 v11 o2 c1 <a1> f1 g1 c1 e1 f1 g1 <a1> e1 f1 c1 d1 g1 c1 c1",
    "t60 @5 v5 o3 (eg>c)1 (ea>c)1 (fa>c)1 (gb>d)1 (eg>c)1 (egb)1 (fa>c)1 (fgb>d)1 "
    "(ea>c)1 (egb)1 (fa>c)1 (eg>c)1 (dfa>c)1 (fgb>d)1 (eg>c)1 (eg>c)1",
    0, 0, 0}},
  /* Biuro (Księga Rodziny): spokojna ballada AABA w B-dur — wibrafon, fortepian, bas, szczotki. */
  {"biuro", 1, 0.4f, 0.5f, {
    "t76 S1 @9 v10 o4 [>d4.c8d4f4< b-2a4g4 e-4.d8c4e-4 a2f4r4 f4.e8f4a4 b2a4g4 e-4g4f4e-4 d2.r4]2 "
    "g4.f8g4b-4 g-4f4e-4c4 f4.e8f4a4 b2g4r4 e-4.d8e-4g4 a2f4r4 g4f4e-4c4 e-4d4c4r4 "
    ">d4.c8d4f4< b-2a4g4 e-4.d8c4e-4 a2f4r4 f4.e8f4a4 b2a4g4 e-4g4f4e-4 d2.r4",
    "t76 S1 @3 v12 o2 [b-2f2 g2d2 c2g2 f2c2 d2a2 g2d2 c2f2 b-2f2]2 e-2b-2 e-2a-2 d2a2 g2d2 c2g2 f2c2 c2g2 f2c2 "
    "b-2f2 g2d2 c2g2 f2c2 d2a2 g2d2 c2f2 b-2f2",
    "t76 S1 @0 v6 o3 [(dfa>c)2.(dfa>c)4 (fb->d)2.(fb->d)4 (e-gb->d)2.(e-gb->d)4 (e-ga>c)2.(e-ga>c)4 "
    "(fa>ce)2.(fa>ce)4 (fab>e)2.(fab>e)4 (e-gb->d)2(e-ga>c)2 (dfa>c)1]2 "
    "(gb->df)2.(gb->df)4 (g-b->d-f)2(g-b->c)2 (fa>ce)2.(fa>ce)4 (fab>e)2.(fab>e)4 (e-gb->d)2.(e-gb->d)4 (e-ga>c)2.(e-ga>c)4 (e-gb->d)2.(e-gb->d)4 (e-ga>c)1 "
    "(dfa>c)2.(dfa>c)4 (fb->d)2.(fb->d)4 (e-gb->d)2.(e-gb->d)4 (e-ga>c)2.(e-ga>c)4 (fa>ce)2.(fa>ce)4 (fab>e)2.(fab>e)4 (e-gb->d)2(e-ga>c)2 (dfa>c)1",
    "t76 S1 @4 v5 o4 [(ce)4(dg)4e4(dg)4]32",
    0, 0}},
  /* Akcja: misja — ostinato, pizzicato, długie dęte. */
  {"akcja", 1, 0.3f, 0, {
    "t132 @10 v8 o4 a1 a2g2 b-1 a2g2 g1 f2e2 e1 c+1",
    "t132 @3 v12 o2 [d8d8d8d8d8d8c8d8]2 [<b-8b-8b-8b-8b-8b-8a8b-8>]2 [<g8g8g8g8g8g8f+8g8>]2 [<a8a8a8a8a8a8g+8a8>]2",
    "t132 @7 v7 o4 [d8f8a8f8d8f8a8>d8<]2 [d8f8b-8f8d8f8b-8>d8<]2 [d8g8b-8g8d8g8b-8>d8<]2 [c+8e8a8e8c+8e8a8>c+8<]2",
    "t132 @4 v6 o4 [c8f8d8f8c8c8d8f8]8",
    0, 0}},
  /* Pogrzeb Dona: organy i smyczki. */
  {"pogrzeb", 1, 0.7f, 0, {
    "t54 @5 v8 o4 a2.g4 f2g2 e2.d4 d1 f2.e4 d2e2 c+2e2 d1",
    "t54 @8 v6 o3 (dfa)1 (dgb-)1 (c+eg)1 (dfa)1 (dfb-)1 (dgb-)1 (c+eg)1 (dfa)1",
    "t54 @8 v7 o2 d1 g1 a1 d1 <b-1> g1 a1 d1",
    0, 0, 0}},
  /* Chinatown: pentatonika — szarpane struny, gong, klocki, burdon. */
  {"chiny", 1, 0.45f, 0, {
    "t88 @7 v9 o5 e4g8a8>c4<a8g8 e4d8c8d2 e4g8a8>c4d4 c4<a8g8a2 a4>c8<a8g4e8d8 e4d8c8<a4>c4 d8e8g8a8g4e4 d8c8<a8>c8<a2>",
    "t88 @3 v11 o2 a2e2 a2d2 a2e2 a2e2 a2g2 a2e2 d2g2 a2e2",
    "t88 @5 v4 o3 [(a>e)1]8",
    "t88 @4 v6 o2 c4d8d8e4d4 [d4d8d8e4d4]3 c4d8d8e4d4 [d4d8d8e4d4]3",
    0, 0}},
  /* Kościół: chorał organowy. */
  {"kosciol", 1, 0.8f, 0, {
    "t60 @8 v8 o4 e2f2 g2e2 d2e4f4 e1 g2a2 g2f2 e2d2 c1",
    "t60 @8 v5 o3 (ceg)1 (ceg)1 (<b>dg)1 (ceg)1 (ceg)1 (cfa)1 (<b>dg)1 (ceg)1",
    "t60 @8 v6 o2 c1 c1 g1 c1 c1 f1 g1 c1",
    0, 0, 0}},
  /* Noc: samotny fortepian w deszczu (przejścia, ulice nocą). */
  {"noc", 1, 0.55f, 0.4f, {
    "t58 @0 v8 o4 r4e8f8a4>c4< b2.a4 g+4.e8f4d4 e2.r4 r4c8d8e4a4 g2.f4 d4.f8e4<b4> a2.r4",
    "t58 @0 v5 o2 (a>e)1 (f>c)1 (d>a)1 (e>b)1 (a>e)1 (d>a)1 (e>d)1 (a>e)1",
    "t58 @3 v9 o2 a1 f1 d1 e1 a1 d1 e1 a1",
    0, 0, 0}},
};
#define NTRACKS ((int)(sizeof(TRACKS) / sizeof(TRACKS[0])))
static Seq trackSeq[NTRACKS][NMUS];
static int curTrack = -1;

/* ---------------------------------------------------------------- głosy muzyki */
#define NPART 8
typedef struct {
  int on, ch, inst, drum, t, relT;
  float freq, vel, gl, gr;
  float ph[NPART], a[NPART], m[NPART];
  float lp, lp2, hp, z;
  unsigned rng;
} Slot;
#define NSLOT 72
static Slot slots[NSLOT];

typedef struct { const Seq *seq; int idx, left, done; } Chan;
static Chan chans[NMUS];
static int musLoop;
static float musRev = 0.3f, musVinyl;

static void slot_release_ch(int ch) {
  for (int i = 0; i < NSLOT; i++)
    if (slots[i].on && slots[i].ch == ch && slots[i].relT < 0 && slots[i].inst != INS_DRUM) slots[i].relT = 0;
}

static Slot *slot_alloc(void) {
  int best = 0, bestScore = -1;
  for (int i = 0; i < NSLOT; i++) {
    if (!slots[i].on) return &slots[i];
    int score = slots[i].relT >= 0 ? slots[i].relT + 1000000 : slots[i].t;
    if (score > bestScore) { bestScore = score; best = i; }
  }
  return &slots[best];
}

static void slot_start(int ch, int inst, float freq, int drum, float vel) {
  Slot *s = slot_alloc();
  memset(s, 0, sizeof *s);
  s->on = 1; s->ch = ch; s->inst = inst; s->drum = drum; s->relT = -1;
  s->freq = freq; s->vel = vel;
  s->rng = 0x1234567u + (unsigned)(freq * 977) + grng;
  float pan = INS_PAN[inst % INS_COUNT];
  if (inst == INS_PIANO || inst == INS_VIBES) pan += (freq - 300) / 1600.0f;
  if (inst == INS_DRUM) {
    static const float DP[15] = {0, -0.1f, -0.1f, -0.1f, 0.05f, 0.35f, 0.35f, 0.3f, -0.35f, -0.4f, -0.4f, 0.45f, 0, -0.3f, 0.3f};
    pan = DP[drum % 15];
  }
  if (pan < -0.8f) pan = -0.8f;
  if (pan > 0.8f) pan = 0.8f;
  s->gl = sqrtf(0.5f * (1 - pan)) * 1.41f;
  s->gr = sqrtf(0.5f * (1 + pan)) * 1.41f;
  float f = freq;
  switch (inst) {
    case INS_PIANO: {
      static const float A[6] = {1, 0.42f, 0.24f, 0.15f, 0.08f, 0.05f};
      float D = 2.4f - f / 500.0f;
      if (D < 0.35f) D = 0.35f;
      for (int k = 0; k < 6; k++) { s->a[k] = A[k] * (k * f < 9000); s->m[k] = expf(-(1 + 0.8f * k) / (D * SR)); }
      break;
    }
    case INS_BASS: {
      static const float A[4] = {1, 0.5f, 0.2f, 0.1f}, R[4] = {1.1f, 2.2f, 3.6f, 5.5f};
      for (int k = 0; k < 4; k++) { s->a[k] = A[k]; s->m[k] = expf(-R[k] / SR); }
      break;
    }
    case INS_PLUCK: {
      static const float A[6] = {1, 0.55f, 0.38f, 0.25f, 0.15f, 0.1f};
      for (int k = 0; k < 6; k++) { s->a[k] = A[k] * (k * f < 9000); s->m[k] = expf(-(2.5f + 2.2f * k) / SR); }
      break;
    }
    case INS_VIBES: {
      s->a[0] = 1; s->m[0] = expf(-0.6f / SR);
      s->a[1] = 0.28f; s->m[1] = expf(-4.0f / SR);
      s->a[2] = 0.07f * (f * 10 < 10000); s->m[2] = expf(-14.0f / SR);
      break;
    }
    case INS_TRUMPET: case INS_SAX: {
      float fc = inst == INS_TRUMPET ? 1700 : 900, bw = inst == INS_TRUMPET ? 700 : 900;
      for (int k = 0; k < NPART; k++) {
        float hz = f * (k + 1);
        float form = 1.0f / (1.0f + ((hz - fc) / bw) * ((hz - fc) / bw));
        float w = (inst == INS_TRUMPET ? 0.35f : 0.6f) / powf((float)(k + 1), 0.8f) + form * 0.8f;
        if (inst == INS_SAX && k > 0 && (k & 1)) w *= 0.75f;
        s->a[k] = hz < 10000 ? w : 0;
      }
      break;
    }
    case INS_ORGAN: {
      static const float H[6] = {1, 2, 3, 4, 6, 8}, A[6] = {1, 0.55f, 0.25f, 0.3f, 0.1f, 0.14f};
      for (int k = 0; k < 6; k++) { s->a[k] = A[k] * (f * H[k] < 9000); s->m[k] = H[k]; }
      break;
    }
    default: break;
  }
}

static void chan_next(Chan *c, int ci) {
  c->idx++;
  if (c->idx >= c->seq->count) { c->done = 1; return; }
  const Note *n = &c->seq->n[c->idx];
  c->left = n->ticks * c->seq->tickSamples;
  if (n->rest) { slot_release_ch(ci); return; }
  if (n->inst != INS_DRUM) slot_release_ch(ci);
  float vel = n->vol / 15.0f;
  for (int k = 0; k < n->nf; k++) {
    float v = vel * (0.92f + 0.16f * frand()); /* "ludzka" dynamika */
    slot_start(ci, n->inst, n->freq[k], n->drum[k], v);
  }
}

/* odtwarzanie jednej próbki instrumentu; zwraca próbkę mono, gaszenie slotu przez s->on = 0 */
static float slot_sample(Slot *s) {
  float ts = s->t * (1.0f / SR);
  float f = s->freq, out = 0, env = 1;
  float rel = 1;
  if (s->relT >= 0) {
    static const float RT[INS_COUNT] = {0.16f, 0.07f, 0.08f, 0.07f, 1, 0.5f, 0.05f, 0.12f, 0.22f, 0.7f, 0.1f};
    rel = expf(-s->relT / (RT[s->inst] * SR));
    if (rel < 0.001f) { s->on = 0; return 0; }
    s->relT++;
  }
  switch (s->inst) {
    case INS_PIANO: {
      float sum = 0;
      for (int k = 0; k < 6; k++) {
        float hk = (k + 1) * (1 + 0.0004f * (k + 1) * (k + 1));
        s->ph[k] += f * hk / SR;
        if (s->ph[k] > 1) s->ph[k] -= 1;
        sum += s->a[k] * fsin(s->ph[k]);
        s->a[k] *= s->m[k];
      }
      float att = ts < 0.002f ? ts / 0.002f : 1;
      float n = rnd_f(&s->rng);
      s->lp += (n - s->lp) * 0.25f;
      sum += s->lp * 0.5f * expf(-ts * 140);
      out = sum * att;
      if (s->a[0] < 0.0005f) s->on = 0;
      break;
    }
    case INS_BASS: {
      float fr = f * (1 + 0.012f * expf(-ts * 40));
      float sum = 0;
      for (int k = 0; k < 4; k++) {
        s->ph[k] += fr * (k + 1) / SR;
        if (s->ph[k] > 1) s->ph[k] -= 1;
        sum += s->a[k] * fsin(s->ph[k]);
        s->a[k] *= s->m[k];
      }
      float n = rnd_f(&s->rng);
      s->lp += (n - s->lp) * 0.06f;
      sum += s->lp * 1.4f * expf(-ts * 70);
      float att = ts < 0.004f ? ts / 0.004f : 1;
      out = sum * att;
      if (s->a[0] < 0.0005f) s->on = 0;
      break;
    }
    case INS_PLUCK: {
      float sum = 0;
      for (int k = 0; k < 6; k++) {
        s->ph[k] += f * (k + 1) / SR;
        if (s->ph[k] > 1) s->ph[k] -= 1;
        sum += s->a[k] * fsin(s->ph[k]);
        s->a[k] *= s->m[k];
      }
      out = sum * (ts < 0.002f ? ts / 0.002f : 1);
      if (s->a[0] < 0.0005f) s->on = 0;
      break;
    }
    case INS_VIBES: {
      float sum = 0;
      static const float H[3] = {1, 4, 10};
      for (int k = 0; k < 3; k++) {
        s->ph[k] += f * H[k] / SR;
        if (s->ph[k] > 1) s->ph[k] -= 1;
        sum += s->a[k] * fsin(s->ph[k]);
        s->a[k] *= s->m[k];
      }
      float trem = 1 - 0.3f * (0.5f + 0.5f * fsin(ts * 5.2f));
      out = sum * trem * (ts < 0.003f ? ts / 0.003f : 1);
      if (s->a[0] < 0.0005f) s->on = 0;
      break;
    }
    case INS_TRUMPET: case INS_SAX: {
      float vib = ts > 0.25f ? (ts - 0.25f) / 0.3f : 0;
      if (vib > 1) vib = 1;
      float depth = s->inst == INS_TRUMPET ? 0.0045f : 0.006f;
      float scoop = 1 - (s->inst == INS_TRUMPET ? 0.035f : 0.02f) * expf(-ts * 28);
      float fr = f * scoop * (1 + depth * vib * fsin(ts * 5.3f));
      float sum = 0;
      for (int k = 0; k < NPART; k++) {
        s->ph[k] += fr * (k + 1) / SR;
        if (s->ph[k] > 1) s->ph[k] -= 1;
        if (s->a[k] > 0) sum += s->a[k] * fsin(s->ph[k]);
      }
      float n = rnd_f(&s->rng);
      s->lp += (n - s->lp) * 0.3f;
      sum += (n - s->lp) * (s->inst == INS_SAX ? 0.12f : 0.05f);
      float att = s->inst == INS_TRUMPET ? 0.035f : 0.05f;
      env = ts < att ? ts / att : 0.75f + 0.25f * expf(-(ts - att) * 1.5f);
      out = sum * env * 0.45f;
      break;
    }
    case INS_STRINGS: {
      float vib = 1 + 0.003f * fsin(ts * 5.0f + f * 0.01f);
      float sum = 0;
      static const float DT[3] = {0.996f, 1.0f, 1.0042f};
      for (int k = 0; k < 3; k++) {
        s->ph[k] += f * DT[k] * vib / SR;
        if (s->ph[k] > 1) s->ph[k] -= 1;
        sum += 2 * s->ph[k] - 1;
      }
      float c = lp_coef(fminf(3200, f * 5));
      s->lp += (sum - s->lp) * c;
      s->lp2 += (s->lp - s->lp2) * c;
      env = ts < 0.35f ? ts / 0.35f : 1;
      out = s->lp2 * env * 0.6f;
      break;
    }
    case INS_ACCORD: {
      float sum = 0;
      static const float DT[2] = {0.9975f, 1.0025f};
      for (int k = 0; k < 2; k++) {
        s->ph[k] += f * DT[k] / SR;
        if (s->ph[k] > 1) s->ph[k] -= 1;
        sum += s->ph[k] < 0.35f ? 1.0f : -0.54f;
      }
      s->lp += (sum - s->lp) * lp_coef(2600);
      s->lp2 += (s->lp - s->lp2) * lp_coef(4000);
      env = ts < 0.025f ? ts / 0.025f : 1;
      out = s->lp2 * env * (1 - 0.1f * fsin(ts * 6.0f)) * 0.55f;
      break;
    }
    case INS_ORGAN: {
      float sum = 0;
      float vib = 1 + 0.0015f * fsin(ts * 6.3f);
      for (int k = 0; k < 6; k++) {
        s->ph[k] += f * s->m[k] * vib / SR;
        if (s->ph[k] > 1) s->ph[k] -= 1;
        sum += s->a[k] * fsin(s->ph[k]);
      }
      env = ts < 0.07f ? ts / 0.07f : 1;
      out = sum * env;
      break;
    }
    case INS_BRASS: {
      float sum = 0;
      static const float DT[2] = {0.997f, 1.003f};
      for (int k = 0; k < 2; k++) {
        s->ph[k] += f * DT[k] / SR;
        if (s->ph[k] > 1) s->ph[k] -= 1;
        sum += 2 * s->ph[k] - 1;
      }
      float fe = ts < 0.03f ? ts / 0.03f : 0.45f + 0.55f * expf(-(ts - 0.03f) * 5);
      float c = lp_coef(fminf(8000, f * (1.5f + 7 * fe)));
      s->lp += (sum - s->lp) * c;
      s->lp2 += (s->lp - s->lp2) * c;
      env = ts < 0.03f ? ts / 0.03f : 0.8f + 0.2f * expf(-(ts - 0.03f) * 3);
      out = s->lp2 * env * 0.7f;
      break;
    }
    case INS_DRUM: {
      float n = rnd_f(&s->rng);
      switch (s->drum) {
        case 0: { /* stopa — miękka, jazzowa */
          float fr = 48 + 70 * expf(-ts * 32);
          s->ph[0] += fr / SR;
          out = fsin(s->ph[0]) * expf(-ts * 9) * 1.1f;
          if (ts > 0.6f) s->on = 0;
          break;
        }
        case 1: { /* rimshot */
          s->ph[0] += 1700.0f / SR;
          s->lp += (n - s->lp) * 0.5f;
          out = (fsin(s->ph[0]) * 0.5f + (n - s->lp)) * expf(-ts * 70) * 0.7f;
          if (ts > 0.15f) s->on = 0;
          break;
        }
        case 2: case 3: { /* szczotka / werbel */
          int hard = s->drum == 3;
          s->lp += (n - s->lp) * (hard ? 0.55f : 0.32f);
          float hp = s->lp - s->lp2;
          s->lp2 += (s->lp - s->lp2) * 0.05f;
          float att = ts < 0.003f ? ts / 0.003f : 1;
          out = hp * expf(-ts * (hard ? 14 : 20)) * att * (hard ? 1.3f : 0.8f);
          if (hard) { s->ph[0] += 185.0f / SR; out += fsin(s->ph[0]) * expf(-ts * 28) * 0.5f; }
          if (ts > 0.5f) s->on = 0;
          break;
        }
        case 4: { /* szur szczotką */
          s->lp += (n - s->lp) * 0.4f;
          float hp = s->lp - s->lp2;
          s->lp2 += (s->lp - s->lp2) * 0.08f;
          float d = 0.38f;
          out = ts < d ? hp * sinf(3.14159f * ts / d) * 0.32f : 0;
          if (ts > d) s->on = 0;
          break;
        }
        case 5: case 6: case 7: { /* hi-hat */
          static const float MF[6] = {3402, 2531, 4091, 5137, 3780, 4800};
          float m = 0;
          for (int k = 0; k < 6; k++) { s->ph[k] += MF[k] / SR; if (s->ph[k] > 1) s->ph[k] -= 1; m += s->ph[k] < 0.5f ? 1 : -1; }
          float x = m * 0.12f + n * 0.5f;
          float hp = x - s->lp;
          s->lp += (x - s->lp) * 0.6f;
          float dec = s->drum == 5 ? 55 : s->drum == 6 ? 7 : 40;
          out = hp * expf(-ts * dec) * (s->drum == 7 ? 0.35f : 0.42f);
          if (ts > (s->drum == 6 ? 0.8f : 0.2f)) s->on = 0;
          break;
        }
        case 8: { /* tom */
          float fr = 85 + 30 * expf(-ts * 20);
          s->ph[0] += fr / SR;
          out = fsin(s->ph[0]) * expf(-ts * 7) * 0.9f;
          if (ts > 0.7f) s->on = 0;
          break;
        }
        case 9: case 10: case 11: { /* ride / dzwon / crash */
          static const float MF[6] = {3150, 4410, 5180, 6620, 7390, 2690};
          float m = 0;
          for (int k = 0; k < 6; k++) { s->ph[k] += MF[k] / SR; if (s->ph[k] > 1) s->ph[k] -= 1; m += fsin(s->ph[k]); }
          float x = m * (s->drum == 10 ? 0.3f : 0.12f) + n * (s->drum == 11 ? 0.9f : 0.35f);
          float hp = x - s->lp;
          s->lp += (x - s->lp) * 0.45f;
          if (s->drum == 11) out = hp * expf(-ts * 1.6f) * 0.5f;
          else out = hp * (expf(-ts * 22) * 0.35f + expf(-ts * 2.2f) * 0.16f);
          if (ts > (s->drum == 11 ? 3.0f : 1.6f)) s->on = 0;
          break;
        }
        case 12: { /* gong */
          static const float GF[5] = {78, 119, 163, 231, 307};
          float m = 0;
          for (int k = 0; k < 5; k++) { s->ph[k] += GF[k] * (1 + 0.01f * expf(-ts)) / SR; m += fsin(s->ph[k]) / (1 + k * 0.6f); }
          float bloom = ts < 0.08f ? ts / 0.08f : 1;
          out = m * bloom * expf(-ts * 0.55f) * 0.45f;
          if (ts > 6) s->on = 0;
          break;
        }
        case 13: case 14: { /* klocek drewniany */
          s->ph[0] += (s->drum == 13 ? 820.0f : 1240.0f) / SR;
          out = fsin(s->ph[0]) * expf(-ts * 55) * 0.8f;
          if (ts > 0.12f) s->on = 0;
          break;
        }
        default: s->on = 0;
      }
      break;
    }
  }
  s->t++;
  return out * rel * s->vel * INS_GAIN[s->inst];
}

/* ---------------------------------------------------------------- pogłos (Freeverb, uproszczony) */
static const int CL[4] = {1116, 1188, 1277, 1356}, AL[2] = {556, 441};
static float combBuf[2][4][1400], apBuf[2][2][600];
static int combIdx[2][4], apIdx[2][2];
static float combLp[2][4];
static float revFb = 0.8f, revFbT = 0.8f, revDamp = 0.3f;

static void reverb(float in, float *ol, float *or_) {
  float o[2];
  for (int c = 0; c < 2; c++) {
    float acc = 0;
    for (int k = 0; k < 4; k++) {
      int len = CL[k] + c * 23;
      float *b = combBuf[c][k];
      float y = b[combIdx[c][k]];
      combLp[c][k] = y * (1 - revDamp) + combLp[c][k] * revDamp;
      b[combIdx[c][k]] = in + combLp[c][k] * revFb;
      if (++combIdx[c][k] >= len) combIdx[c][k] = 0;
      acc += y;
    }
    for (int k = 0; k < 2; k++) {
      int len = AL[k] + c * 23;
      float *b = apBuf[c][k];
      float bo = b[apIdx[c][k]];
      float y = -acc + bo;
      b[apIdx[c][k]] = acc + bo * 0.5f;
      if (++apIdx[c][k] >= len) apIdx[c][k] = 0;
      acc = y;
    }
    o[c] = acc;
  }
  *ol = o[0]; *or_ = o[1];
}

/* ---------------------------------------------------------------- efekty dźwiękowe (bufory) */
#define NVAR 3
typedef struct { float *d; int n; } Buf;
static Buf sfxBuf[SFX_COUNT][NVAR];
static int sfxNVar[SFX_COUNT];
static const char *SFXNAMES[SFX_COUNT] = {"blip", "ok", "cancel", "hit", "crit", "shot", "heal", "door",
                                          "cash", "level", "encounter", "flee", "die", "magic", "text",
                                          "pistol", "tommy", "shotgun", "punch", "reload", "empty", "step", "hurt",
                                          "horn", "phone", "glass", "bell", "type", "paper", "car"};
static const float SFX_GAIN[SFX_COUNT] = {0.35f, 0.45f, 0.4f, 0.8f, 0.9f, 0.9f, 0.45f, 0.6f, 0.6f, 0.45f, 0.6f, 0.5f, 0.7f, 0.45f, 0.16f,
                                          0.95f, 0.85f, 1.0f, 0.75f, 0.55f, 0.5f, 0.3f, 0.7f, 0.6f, 0.5f, 0.5f, 0.6f, 0.4f, 0.4f, 0.5f};

static float *buf_new(Buf *b, float sec) {
  b->n = (int)(sec * SR);
  b->d = (float *)calloc((size_t)b->n, sizeof(float));
  return b->d;
}
static void buf_norm(Buf *b, float peak) {
  float m = 1e-6f;
  for (int i = 0; i < b->n; i++) m = fmaxf(m, fabsf(b->d[i]));
  for (int i = 0; i < b->n; i++) b->d[i] *= peak / m;
}
/* dzwonek / nuta FM-owa dodana do bufora */
static void add_bell(float *d, int n, float t0, float f, float amp, float dec, float ratio) {
  int s0 = (int)(t0 * SR);
  for (int i = s0; i < n; i++) {
    float t = (i - s0) / (float)SR;
    float e = expf(-t * dec);
    if (e < 1e-4f) break;
    float mod = fsin(f * ratio * t) * 2.0f * e;
    d[i] += fsin(f * t + mod * 0.16f) * e * amp * (t < 0.002f ? t / 0.002f : 1);
  }
}
static void add_vibe(float *d, int n, float t0, float f, float amp) {
  int s0 = (int)(t0 * SR);
  for (int i = s0; i < n; i++) {
    float t = (i - s0) / (float)SR;
    float e = expf(-t * 2.2f);
    d[i] += (fsin(f * t) + 0.25f * fsin(f * 4 * t) * expf(-t * 6)) * e * amp * (t < 0.003f ? t / 0.003f : 1);
  }
}
static void add_click(float *d, int n, float t0, float hz, float amp, float dec, unsigned *r) {
  int s0 = (int)(t0 * SR);
  float lp = 0;
  for (int i = s0; i < n; i++) {
    float t = (i - s0) / (float)SR;
    float e = expf(-t * dec);
    if (e < 1e-4f) break;
    float x = rnd_f(r);
    float hp = x - lp;
    lp += (x - lp) * 0.3f;
    d[i] += (hp * 0.6f + fsin(hz * t) * 0.5f) * e * amp;
  }
}
static void add_thump(float *d, int n, float t0, float f0, float f1, float amp, float dec) {
  int s0 = (int)(t0 * SR);
  float ph = 0;
  for (int i = s0; i < n; i++) {
    float t = (i - s0) / (float)SR;
    float e = expf(-t * dec);
    if (e < 1e-4f) break;
    ph += (f1 + (f0 - f1) * expf(-t * 30)) / SR;
    d[i] += fsin(ph) * e * amp;
  }
}
/* szum filtrowany z obwiednią (wystrzał, kroki, uderzenia) */
static void add_noise(float *d, int n, float t0, float lpHz, float hpHz, float amp, float att, float dec, unsigned *r) {
  int s0 = (int)(t0 * SR);
  float lp = 0, lp2 = 0, hl = 0, cl = lp_coef(lpHz), ch = lp_coef(hpHz);
  for (int i = s0; i < n; i++) {
    float t = (i - s0) / (float)SR;
    float e = (att > 0 && t < att ? t / att : 1) * expf(-t * dec);
    if (t > att && e < 1e-4f) break;
    float x = rnd_f(r);
    lp += (x - lp) * cl;
    lp2 += (lp - lp2) * cl;
    hl += (lp2 - hl) * ch;
    d[i] += (lp2 - hl) * e * amp;
  }
}
/* głos "uh" — źródło piłokształtne przez dwa formanty */
static void add_grunt(float *d, int n, float t0, float f0, float dur, float amp, float F1, float F2, unsigned *r) {
  Reso a, b;
  memset(&a, 0, sizeof a); memset(&b, 0, sizeof b);
  reso_set(&a, F1, 90); reso_set(&b, F2, 120);
  int s0 = (int)(t0 * SR);
  float ph = 0;
  for (int i = s0; i < n; i++) {
    float t = (i - s0) / (float)SR;
    if (t > dur) break;
    float e = sinf(3.14159f * t / dur);
    e = e * e;
    ph += f0 * (1 - 0.25f * t / dur) / SR;
    if (ph > 1) ph -= 1;
    float src = (2 * ph - 1) + rnd_f(r) * 0.3f;
    d[i] += (reso(&a, src) + 0.6f * reso(&b, src)) * e * amp;
  }
}

static void gen_gunshot(Buf *b, float sec, float crackDec, float boomHz, float boomDec, float body, unsigned *r) {
  float *d = buf_new(b, sec);
  /* trzask (bardzo krótki, szerokopasmowy) */
  add_noise(d, b->n, 0, 9000, 300, 1.4f, 0.0003f, crackDec, r);
  /* korpus — średnie częstotliwości */
  add_noise(d, b->n, 0, 1600, 80, body, 0.001f, crackDec * 0.35f, r);
  /* uderzenie basowe */
  add_thump(d, b->n, 0, boomHz * 2.2f, boomHz, 1.2f, boomDec);
  /* echo od ścian */
  add_noise(d, b->n, 0.07f, 1200, 100, body * 0.25f, 0.01f, 9, r);
  for (int i = 0; i < b->n; i++) d[i] = softclip(d[i] * 1.6f);
  buf_norm(b, 0.95f);
}

static void sfx_generate(void) {
  unsigned r = 0xC0FFEEu;
  for (int id = 0; id < SFX_COUNT; id++) {
    int nv = (id == SFX_STEP || id == SFX_PISTOL || id == SFX_TOMMY || id == SFX_PUNCH || id == SFX_HIT || id == SFX_TEXT) ? NVAR : 1;
    sfxNVar[id] = nv;
    for (int v = 0; v < nv; v++) {
      Buf *b = &sfxBuf[id][v];
      float *d;
      float jit = 1 + (v - 1) * 0.06f;
      switch (id) {
        case SFX_BLIP:
          d = buf_new(b, 0.06f);
          add_click(d, b->n, 0, 1800, 0.3f, 220, &r);
          add_thump(d, b->n, 0, 900, 600, 0.5f, 90);
          break;
        case SFX_OK:
          d = buf_new(b, 0.5f);
          add_bell(d, b->n, 0, 659, 0.5f, 7, 2.0f);
          add_bell(d, b->n, 0.07f, 988, 0.5f, 6, 2.0f);
          break;
        case SFX_CANCEL:
          d = buf_new(b, 0.35f);
          add_bell(d, b->n, 0, 494, 0.5f, 9, 1.0f);
          add_bell(d, b->n, 0.06f, 330, 0.5f, 9, 1.0f);
          break;
        case SFX_HIT:
          d = buf_new(b, 0.25f);
          add_thump(d, b->n, 0, 160 * jit, 55, 0.9f, 22);
          add_noise(d, b->n, 0, 900, 60, 0.9f, 0.001f, 30, &r);
          break;
        case SFX_CRIT:
          d = buf_new(b, 0.35f);
          add_thump(d, b->n, 0, 180, 50, 1.0f, 18);
          add_noise(d, b->n, 0, 6000, 900, 0.8f, 0.0005f, 60, &r);
          add_noise(d, b->n, 0, 900, 60, 0.8f, 0.001f, 25, &r);
          break;
        case SFX_FIRE: case SFX_PISTOL:
          gen_gunshot(b, 0.7f, 38 * jit, 62 * jit, 11, 0.9f, &r);
          break;
        case SFX_TOMMY:
          gen_gunshot(b, 0.35f, 55 * jit, 70 * jit, 16, 0.75f, &r);
          break;
        case SFX_SHOTGUN:
          gen_gunshot(b, 1.1f, 22, 48, 6, 1.3f, &r);
          d = b->d;
          add_click(d, b->n, 0.62f, 2200, 0.25f, 120, &r); /* przeładowanie pompką */
          add_thump(d, b->n, 0.62f, 300, 150, 0.08f, 60);
          add_click(d, b->n, 0.78f, 1900, 0.3f, 100, &r);
          add_thump(d, b->n, 0.78f, 280, 140, 0.08f, 60);
          break;
        case SFX_HEAL:
          d = buf_new(b, 1.0f);
          add_vibe(d, b->n, 0, 523, 0.35f); add_vibe(d, b->n, 0.08f, 659, 0.35f); add_vibe(d, b->n, 0.16f, 784, 0.35f);
          break;
        case SFX_DOOR: {
          d = buf_new(b, 0.95f);
          /* skrzypnięcie zawiasów */
          float ph = 0, lp = 0;
          for (int i = 0; i < (int)(0.5f * SR); i++) {
            float t = i / (float)SR;
            float f = 210 + 60 * sinf(t * 9) + 40 * sinf(t * 23);
            ph += f / SR;
            if (ph > 1) ph -= 1;
            float x = (ph < 0.15f ? 1.0f : -0.18f) * (0.6f + 0.4f * fsin(t * 37));
            lp += (x - lp) * 0.2f;
            d[i] += lp * sinf(3.14159f * t / 0.5f) * 0.25f;
          }
          add_click(d, b->n, 0.55f, 900, 0.5f, 90, &r);
          add_thump(d, b->n, 0.55f, 140, 70, 0.8f, 25);
          break;
        }
        case SFX_CHEST: /* kasa fiskalna */
          d = buf_new(b, 1.1f);
          for (int k = 0; k < 4; k++) add_click(d, b->n, k * 0.035f, 1200 + k * 200, 0.35f, 140, &r);
          add_thump(d, b->n, 0.14f, 200, 90, 0.35f, 30);
          add_bell(d, b->n, 0.16f, 2093, 0.32f, 4, 1.41f);
          add_bell(d, b->n, 0.16f, 2637, 0.2f, 5, 1.41f);
          break;
        case SFX_LEVEL:
          d = buf_new(b, 1.4f);
          add_vibe(d, b->n, 0, 523, 0.3f); add_vibe(d, b->n, 0.1f, 659, 0.3f);
          add_vibe(d, b->n, 0.2f, 784, 0.3f); add_vibe(d, b->n, 0.3f, 1047, 0.35f);
          break;
        case SFX_ENCOUNTER: {
          d = buf_new(b, 1.2f);
          static const float F[4] = {196, 233, 277, 330};
          for (int k = 0; k < 4; k++) {
            float ph = 0, lp = 0;
            for (int i = 0; i < b->n; i++) {
              float t = i / (float)SR;
              ph += F[k] / SR; if (ph > 1) ph -= 1;
              lp += ((2 * ph - 1) - lp) * lp_coef(600 + 3000 * expf(-t * 6));
              d[i] += lp * (t < 0.02f ? t / 0.02f : expf(-(t - 0.02f) * 2.5f)) * 0.3f;
            }
          }
          add_noise(d, b->n, 0, 9000, 2000, 0.5f, 0.001f, 2.5f, &r);
          add_thump(d, b->n, 0, 120, 45, 0.8f, 8);
          break;
        }
        case SFX_FLEE:
          d = buf_new(b, 0.45f);
          add_noise(d, b->n, 0, 2500, 400, 0.8f, 0.2f, 6, &r);
          break;
        case SFX_DIE:
          d = buf_new(b, 0.8f);
          add_grunt(d, b->n, 0, 120, 0.45f, 0.7f, 560, 950, &r);
          add_thump(d, b->n, 0.35f, 110, 45, 0.9f, 10);
          add_noise(d, b->n, 0.35f, 700, 50, 0.6f, 0.002f, 18, &r);
          break;
        case SFX_MAGIC:
          d = buf_new(b, 1.0f);
          for (int k = 0; k < 6; k++) add_bell(d, b->n, k * 0.05f, 1047 * powf(1.122f, (float)k), 0.18f, 5, 3.5f);
          break;
        case SFX_TEXT: /* maszyna do pisania, bardzo cicho */
          d = buf_new(b, 0.05f);
          add_click(d, b->n, 0, 2500 * jit, 0.5f, 260, &r);
          add_thump(d, b->n, 0.003f, 400, 220, 0.25f, 120);
          break;
        case SFX_PUNCH:
          d = buf_new(b, 0.25f);
          add_thump(d, b->n, 0, 140 * jit, 55, 1.0f, 24);
          add_noise(d, b->n, 0, 2400, 200, 0.7f, 0.0008f, 55, &r);
          break;
        case SFX_RELOAD:
          d = buf_new(b, 0.5f);
          add_click(d, b->n, 0, 2800, 0.6f, 150, &r);
          add_click(d, b->n, 0.17f, 2300, 0.5f, 120, &r);
          add_thump(d, b->n, 0.17f, 500, 300, 0.2f, 70);
          add_click(d, b->n, 0.36f, 3200, 0.7f, 160, &r);
          break;
        case SFX_EMPTY:
          d = buf_new(b, 0.1f);
          add_click(d, b->n, 0, 3000, 0.7f, 180, &r);
          break;
        case SFX_STEP:
          d = buf_new(b, 0.16f);
          add_noise(d, b->n, 0, 2200 * jit, 120, 0.8f, 0.004f, 38, &r);
          add_thump(d, b->n, 0, 120 * jit, 60, 0.35f, 35);
          add_noise(d, b->n, 0.03f, 4000, 1200, 0.15f, 0.002f, 60, &r); /* "szur" podeszwy */
          break;
        case SFX_HURT:
          d = buf_new(b, 0.4f);
          add_thump(d, b->n, 0, 130, 55, 0.8f, 20);
          add_grunt(d, b->n, 0.02f, 135, 0.22f, 0.9f, 620, 1050, &r);
          break;
        case SFX_HORN: { /* klakson "a-u-ga" */
          d = buf_new(b, 0.9f);
          float ph = 0, lp = 0;
          for (int i = 0; i < b->n; i++) {
            float t = i / (float)SR;
            float f = t < 0.25f ? 160 + 200 * t / 0.25f : 360 - 90 * fminf(1, (t - 0.25f) / 0.3f);
            ph += f / SR; if (ph > 1) ph -= 1;
            float x = (2 * ph - 1) + (ph < 0.3f ? 0.6f : -0.2f);
            lp += (x - lp) * lp_coef(1600);
            float e = (t < 0.03f ? t / 0.03f : 1) * (t > 0.75f ? fmaxf(0, 1 - (t - 0.75f) / 0.15f) : 1);
            d[i] = lp * e;
          }
          break;
        }
        case SFX_PHONE: { /* dzwonek telefonu (dwa dzwonki młoteczka) */
          d = buf_new(b, 1.4f);
          for (int k = 0; k < 24; k++) {
            add_bell(d, b->n, k * 0.04f, 1400, 0.22f, 18, 2.76f);
            add_bell(d, b->n, k * 0.04f + 0.02f, 1630, 0.22f, 18, 2.76f);
          }
          break;
        }
        case SFX_GLASS:
          d = buf_new(b, 0.6f);
          add_bell(d, b->n, 0, 2600, 0.4f, 12, 2.3f);
          add_bell(d, b->n, 0, 3710, 0.3f, 15, 2.3f);
          add_bell(d, b->n, 0, 5200, 0.15f, 20, 2.3f);
          break;
        case SFX_BELL: /* dzwon kościelny */
          d = buf_new(b, 4.0f);
          add_bell(d, b->n, 0, 196, 0.5f, 0.9f, 2.0f);
          add_bell(d, b->n, 0, 392 * 1.19f, 0.25f, 1.2f, 1.0f);
          add_bell(d, b->n, 0, 588, 0.18f, 1.5f, 1.4f);
          add_bell(d, b->n, 0, 98, 0.3f, 0.7f, 1.0f);
          break;
        case SFX_TYPE:
          d = buf_new(b, 0.12f);
          add_click(d, b->n, 0, 2200, 0.7f, 120, &r);
          add_thump(d, b->n, 0, 600, 250, 0.4f, 60);
          break;
        case SFX_PAPER:
          d = buf_new(b, 0.5f);
          for (int k = 0; k < 6; k++) add_noise(d, b->n, k * 0.06f + frand() * 0.03f, 7000, 1500, 0.4f, 0.01f, 25, &r);
          break;
        case SFX_CAR: { /* silnik auta z lat 30. — odjazd */
          d = buf_new(b, 2.5f);
          float ph = 0, lp = 0;
          for (int i = 0; i < b->n; i++) {
            float t = i / (float)SR;
            float f = 28 + 22 * fminf(1, t / 1.2f);
            ph += f / SR; if (ph > 1) ph -= 1;
            float x = (ph < 0.2f ? 1.0f : -0.25f) + rnd_f(&r) * 0.3f;
            lp += (x - lp) * lp_coef(500);
            d[i] = lp * (t < 0.2f ? t / 0.2f : 1) * fmaxf(0, 1 - t / 2.5f);
          }
          break;
        }
        default: d = buf_new(b, 0.05f);
      }
      buf_norm(b, 0.9f);
    }
  }
}

typedef struct { const Buf *b; float pos, rate, gain, gl, gr; int on; } Player;
#define NPLAY 20
static Player play[NPLAY];

/* ---------------------------------------------------------------- ambient */
static int ambKind = AMB_NONE;
static float ambRain, ambRainT, ambLevel, ambSpace;
static struct {
  float rl1, rl2, rr1, rr2;    /* deszcz L/P */
  float rum, rum2;             /* dudnienie miasta */
  float wat, wat2;             /* woda */
  int evT;                     /* do następnego zdarzenia */
  int clockT, clockN;
  /* gwar */
  float vph[8], vpitch[8], venv[8], vtarget[8];
  int vT[8];
  Reso vf[8];
  /* zdarzenia długie: pociąg, syrena mgłowa */
  int trainT, fogT;
  float trainPh, fogPh[2], fogLp;
  int clinkT;
} A;

void audio_ambience(int kind, float rain, float space) {
  if (kind != ambKind) {
    ambKind = kind;
    ambLevel = 0;
    A.evT = SR * (5 + (int)(frand() * 10));
    A.trainT = A.fogT = 0;
  }
  ambRainT = rain;
  ambSpace = space;
  revFbT = space >= 2 ? 0.88f : space >= 1 ? 0.8f : 0.76f;
}

static void amb_event(void) {
  float r = frand();
  switch (ambKind) {
    case AMB_STREET:
      if (r < 0.35f) { A.trainT = SR * 9; }
      else if (r < 0.75f) {
        for (int i = 0; i < NPLAY; i++) if (!play[i].on) {
          float pan = frand() * 1.4f - 0.7f;
          play[i] = (Player){&sfxBuf[SFX_HORN][0], 0, 0.85f + frand() * 0.3f, 0.05f + frand() * 0.04f, sqrtf(0.5f * (1 - pan)), sqrtf(0.5f * (1 + pan)), 1};
          break;
        }
      } else {
        for (int i = 0; i < NPLAY; i++) if (!play[i].on) {
          play[i] = (Player){&sfxBuf[SFX_CAR][0], 0, 0.8f + frand() * 0.4f, 0.06f, 0.6f, 0.8f, 1};
          break;
        }
      }
      A.evT = SR * (18 + (int)(frand() * 35));
      break;
    case AMB_HARBOR:
      A.fogT = SR * 5;
      A.evT = SR * (25 + (int)(frand() * 35));
      break;
    case AMB_CHURCH:
      for (int i = 0; i < NPLAY; i++) if (!play[i].on) { play[i] = (Player){&sfxBuf[SFX_BELL][0], 0, 1, 0.05f, 0.7f, 0.7f, 1}; break; }
      A.evT = SR * (60 + (int)(frand() * 60));
      break;
    default:
      A.evT = SR * 30;
  }
}

static void ambience_sample(float *l, float *r, float *send) {
  ambRain += (ambRainT - ambRain) * 0.00005f;
  ambLevel += ((ambKind != AMB_NONE ? 1.0f : 0.0f) - ambLevel) * 0.00003f;
  float L = 0, R = 0, S = 0;
  int interior = ambKind == AMB_ROOM || ambKind == AMB_CROWD || ambKind == AMB_OFFICE || ambKind == AMB_CHURCH || ambKind == AMB_WAREHOUSE;
  /* deszcz */
  if (ambRain > 0.001f) {
    float nl = rnd_f(&grng), nr = rnd_f(&grng);
    float c = interior ? 0.02f : 0.16f;
    A.rl1 += (nl - A.rl1) * c; A.rl2 += (A.rl1 - A.rl2) * c;
    A.rr1 += (nr - A.rr1) * c; A.rr2 += (A.rr1 - A.rr2) * c;
    float g = ambRain * (interior ? 0.5f : 0.22f);
    L += (A.rl1 * 0.6f + A.rl2) * g;
    R += (A.rr1 * 0.6f + A.rr2) * g;
    /* krople na parapetach/rynnach */
    if (!interior && (grng & 0xFFF) < 3) {
      for (int i = 0; i < NPLAY; i++) if (!play[i].on) {
        float pan = frand() * 1.6f - 0.8f;
        play[i] = (Player){&sfxBuf[SFX_TEXT][0], 0, 0.4f + frand() * 0.5f, 0.08f * ambRain, sqrtf(0.5f * (1 - pan)), sqrtf(0.5f * (1 + pan)), 1};
        break;
      }
    }
  }
  if (ambKind == AMB_NONE) { *l = L; *r = R; *send = 0; return; }
  float n = rnd_f(&grng);
  /* dudnienie miasta / szum pomieszczenia */
  A.rum += (n - A.rum) * 0.004f;
  A.rum2 += (A.rum - A.rum2) * 0.004f;
  float rumble = A.rum2 * (ambKind == AMB_STREET ? 2.2f : ambKind == AMB_HARBOR ? 1.6f : 0.8f);
  L += rumble; R += rumble;
  if (ambKind == AMB_HARBOR) {
    /* woda uderzająca o nabrzeże */
    A.wat += (n - A.wat) * 0.02f;
    A.wat2 += (A.wat - A.wat2) * 0.05f;
    static float wt;
    wt += 1.0f / SR;
    float lap = 0.5f + 0.5f * fsin(wt * 0.23f) * fsin(wt * 0.11f + 0.3f);
    lap = lap * lap;
    L += A.wat2 * lap * 0.5f; R += A.wat2 * (1 - lap) * 0.5f + A.wat2 * 0.1f;
  }
  if (ambKind == AMB_CROWD) {
    /* gwar rozmów: 8 "głosów" z formantami i sylabami */
    for (int v = 0; v < 8; v++) {
      if (--A.vT[v] <= 0) {
        int talk = frand() < 0.7f;
        A.vT[v] = (int)(SR * (talk ? 0.07f + frand() * 0.18f : 0.2f + frand() * 1.1f));
        A.vtarget[v] = talk ? 0.4f + frand() * 0.6f : 0;
        if (A.vpitch[v] == 0) A.vpitch[v] = 95 + frand() * 120;
        reso_set(&A.vf[v], 300 + frand() * 600, 140);
      }
      A.venv[v] += (A.vtarget[v] - A.venv[v]) * 0.004f;
      A.vph[v] += A.vpitch[v] * (1 + 0.03f * rnd_f(&grng)) / SR;
      if (A.vph[v] > 1) A.vph[v] -= 1;
      float src = (2 * A.vph[v] - 1) * 0.6f + rnd_f(&grng) * 0.4f;
      float y = reso(&A.vf[v], src) * A.venv[v] * 0.05f;
      float pan = (v - 3.5f) / 5.0f;
      L += y * (1 - pan); R += y * (1 + pan);
    }
    if (--A.clinkT <= 0) {
      A.clinkT = (int)(SR * (1.5f + frand() * 5));
      for (int i = 0; i < NPLAY; i++) if (!play[i].on) {
        float pan = frand() * 1.4f - 0.7f;
        play[i] = (Player){&sfxBuf[SFX_GLASS][0], 0, 0.85f + frand() * 0.35f, 0.07f, sqrtf(0.5f * (1 - pan)), sqrtf(0.5f * (1 + pan)), 1};
        break;
      }
    }
  }
  if (ambKind == AMB_OFFICE) {
    /* zegar ścienny */
    if (--A.clockT <= 0) {
      A.clockT = SR;
      A.clockN++;
      for (int i = 0; i < NPLAY; i++) if (!play[i].on) {
        play[i] = (Player){&sfxBuf[SFX_TEXT][0], 0, (A.clockN & 1) ? 0.55f : 0.47f, 0.12f, 0.9f, 0.5f, 1};
        break;
      }
    }
  }
  /* pociąg kolejki "L" przejeżdżający w oddali */
  if (A.trainT > 0) {
    float t = 1 - A.trainT / (float)(SR * 9);
    float e = sinf(3.14159f * t);
    A.trainPh += 1.0f / SR;
    float clack = fmodf(A.trainPh, 0.36f);
    float c = clack < 0.02f ? (1 - clack / 0.02f) : 0;
    float c2 = fabsf(clack - 0.12f) < 0.015f ? 0.7f : 0;
    float x = A.rum2 * 6 + (c + c2) * rnd_f(&grng) * 0.25f;
    L += x * e * 0.5f; R += x * e * 0.5f;
    S += x * e * 0.4f;
    A.trainT--;
  }
  /* syrena mgłowa */
  if (A.fogT > 0) {
    float t = 5 - A.fogT / (float)SR;
    float e = t < 0.5f ? t / 0.5f : t > 3.5f ? fmaxf(0, 1 - (t - 3.5f) / 1.5f) : 1;
    float x = 0;
    for (int k = 0; k < 2; k++) {
      A.fogPh[k] += (k ? 98.6f : 97.9f) / SR;
      if (A.fogPh[k] > 1) A.fogPh[k] -= 1;
      x += 2 * A.fogPh[k] - 1;
    }
    A.fogLp += (x - A.fogLp) * lp_coef(380);
    float y = A.fogLp * e * 0.09f;
    L += y; R += y; S += y * 2;
    A.fogT--;
  }
  if (--A.evT <= 0) amb_event();
  *l = L * ambLevel; *r = R * ambLevel; *send = S * ambLevel;
}

/* ---------------------------------------------------------------- API */
void audio_init(void) {
  for (int i = 0; i <= SINN; i++) SINT[i] = sinf(TWO_PI * i / SINN);
  for (int t = 0; t < NTRACKS; t++)
    for (int c = 0; c < NMUS; c++) {
      if (!TRACKS[t].ch[c]) continue;
      char *ex = expand(TRACKS[t].ch[c]);
      compile(ex, &trackSeq[t][c]);
      free(ex);
    }
  sfx_generate();
}

int music_exists(const char *name) {
  if (!strcmp(name, "-")) return 1;
  for (int t = 0; t < NTRACKS; t++) if (!strcmp(TRACKS[t].name, name)) return 1;
  return 0;
}

int sfx_find(const char *name) {
  for (int i = 0; i < SFX_COUNT; i++) if (!strcmp(SFXNAMES[i], name)) return i;
  return -1;
}

static void chans_start(int t) {
  for (int c = 0; c < NMUS; c++) {
    chans[c].seq = t >= 0 && TRACKS[t].ch[c] ? &trackSeq[t][c] : NULL;
    chans[c].idx = -1;
    chans[c].left = 0;
    chans[c].done = !chans[c].seq;
  }
}

void music_play(const char *name) {
  int t = -1;
  if (name) for (int i = 0; i < NTRACKS; i++) if (!strcmp(TRACKS[i].name, name)) t = i;
  if (t == curTrack) return;
  for (int c = 0; c < NMUS; c++) slot_release_ch(c);
  curTrack = t;
  musLoop = t >= 0 ? TRACKS[t].loop : 0;
  if (t >= 0) { musRev = TRACKS[t].reverb; musVinyl = TRACKS[t].vinyl; }
  chans_start(t);
}

static void sfx_play_ex(int id, float gain, float pan) {
  if (id < 0 || id >= SFX_COUNT) return;
  int slot = -1;
  if (id == SFX_TEXT) {
    for (int i = 0; i < NPLAY; i++) if (play[i].on && play[i].b == &sfxBuf[SFX_TEXT][0]) play[i].on = 0;
  }
  for (int i = 0; i < NPLAY; i++) if (!play[i].on) { slot = i; break; }
  if (slot < 0) {
    float best = 1e9f;
    for (int i = 0; i < NPLAY; i++) if (play[i].gain < best) { best = play[i].gain; slot = i; }
  }
  int v = sfxNVar[id] > 1 ? (int)(frand() * sfxNVar[id]) % sfxNVar[id] : 0;
  float rate = 1 + (frand() - 0.5f) * (id == SFX_STEP ? 0.16f : id >= SFX_PISTOL && id <= SFX_PUNCH ? 0.08f : 0.02f);
  play[slot] = (Player){&sfxBuf[id][v], 0, rate, gain * SFX_GAIN[id], sqrtf(0.5f * (1 - pan)) * 1.41f, sqrtf(0.5f * (1 + pan)) * 1.41f, 1};
}

void sfx_play(int id) { sfx_play_ex(id, 1.0f, 0); }

void sfx_play_at(int id, float dist) {
  if (dist > 24) return;
  float g = 1.0f / (1.0f + dist * 0.22f);
  sfx_play_ex(id, g, (frand() - 0.5f) * 0.6f);
}

void sfx_play_pan(int id, float gain, float pan) { sfx_play_ex(id, gain, pan); }

/* silnik auta: ciągły dźwięk, wysokość zależna od obrotów */
static float engRpm, engGain, engRpmT, engGainT, engPh, engPh2, engLp, engLp2;
void audio_engine(float rpm, float gain) { engRpmT = rpm; engGainT = gain; }
static float engine_sample(void) {
  engRpm += (engRpmT - engRpm) * 0.0004f;
  engGain += (engGainT - engGain) * 0.0008f;
  if (engGain < 0.001f) return 0;
  float f = 24 + engRpm * 62;
  engPh += f / SR; if (engPh > 1) engPh -= 1;
  engPh2 += f * 0.5f / SR; if (engPh2 > 1) engPh2 -= 1;
  float x = (engPh < 0.18f ? 1.0f : -0.22f) + 0.5f * (engPh2 < 0.3f ? 1.0f : -0.3f) + rnd_f(&grng) * (0.15f + engRpm * 0.25f);
  engLp += (x - engLp) * lp_coef(280 + engRpm * 900);
  engLp2 += (engLp - engLp2) * lp_coef(500 + engRpm * 1200);
  return engLp2 * engGain * (0.10f + engRpm * 0.08f);
}

/* renderuje n ramek stereo (przeplatane L,P) */
void audio_render(int16_t *out, int n) {
  float master = g_volume / 10.0f;
  float musL = 0, musR = 0;
  static float vinLp, vinHum;
  static float outLp[2];
  for (int i = 0; i < n; i++) {
    /* sekwencer */
    if (curTrack >= 0) {
      int allDone = 1;
      for (int c = 0; c < NMUS; c++) {
        Chan *ch = &chans[c];
        if (!ch->seq || ch->done) continue;
        allDone = 0;
        while (ch->left <= 0 && !ch->done) chan_next(ch, c);
        ch->left--;
      }
      if (allDone) {
        if (musLoop) chans_start(curTrack);
        else curTrack = -1;
      }
    }
    musL = musR = 0;
    for (int s = 0; s < NSLOT; s++) {
      if (!slots[s].on) continue;
      float v = slot_sample(&slots[s]);
      musL += v * slots[s].gl;
      musR += v * slots[s].gr;
    }
    /* trzaski płyty gramofonowej */
    if (musVinyl > 0 && curTrack >= 0) {
      float nn = rnd_f(&grng);
      vinLp += (nn - vinLp) * 0.3f;
      float crackle = ((grng & 0x3FFF) < 2) ? rnd_f(&grng) * 0.18f : 0;
      vinHum = (vinLp * 0.004f + crackle) * musVinyl;
      musL += vinHum; musR += vinHum;
    }
    float sfxL = 0, sfxR = 0;
    for (int p = 0; p < NPLAY; p++) {
      Player *pl = &play[p];
      if (!pl->on) continue;
      int ip = (int)pl->pos;
      if (ip + 1 >= pl->b->n) { pl->on = 0; continue; }
      float fr = pl->pos - ip;
      float v = (pl->b->d[ip] * (1 - fr) + pl->b->d[ip + 1] * fr) * pl->gain;
      sfxL += v * pl->gl;
      sfxR += v * pl->gr;
      pl->pos += pl->rate;
    }
    float aL, aR, aS;
    ambience_sample(&aL, &aR, &aS);
    float eng = engine_sample();
    sfxL += eng; sfxR += eng;
    revFb += (revFbT - revFb) * 0.0001f;
    float send = (musL + musR) * 0.5f * musRev + (sfxL + sfxR) * 0.5f * (0.12f + 0.12f * ambSpace) + aS + (aL + aR) * 0.1f;
    float rl, rr;
    reverb(send * 0.06f, &rl, &rr);
    float L = musL * 0.8f + sfxL + aL + rl, R = musR * 0.8f + sfxR + aR + rr;
    /* lekkie ocieplenie całości */
    outLp[0] += (L - outLp[0]) * 0.7f;
    outLp[1] += (R - outLp[1]) * 0.7f;
    L = softclip(outLp[0] * master);
    R = softclip(outLp[1] * master);
    out[i * 2] = (int16_t)(L * 30000);
    out[i * 2 + 1] = (int16_t)(R * 30000);
  }
}
