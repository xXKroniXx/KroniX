/* Syntezator chiptune: 4 kanały muzyki + 2 kanały efektów.
 * Muzyka zapisana w MML: t tempo, o oktawa, < > zmiana oktawy, l długość,
 * v głośność 0-15, @ fala (0-2 prostokąt 12/25/50%, 3 trójkąt, 4 szum),
 * E obwiednia (0 organy, 1 szarpnięcie, 2 perkusja, 3 miękka), S1 swing,
 * nuty a-g (+/# krzyżyk, - bemol), r pauza, [ ... ]n powtórzenie. */
#include "engine.h"
#include <math.h>
#include <ctype.h>

int g_volume = 7;

typedef struct { float freq; int ticks; u8 vol, wave, env, rest; } Note;
typedef struct { Note *n; int count; int tickSamples; } Seq;

typedef struct {
  const Seq *seq;
  int idx, left;
  float phase, amp, decay, freq;
  int wave, env, vol, age, noteLen;
  unsigned lfsr;
  int done;
} Voice;

#define NMUS 4
#define NSFX 5 /* 0 = tekst, 1..4 = efekty (rotacja) */
static Voice mus[NMUS], sfxv[NSFX];
static float sfxGain[NSFX];
static int sfxNext;
static int musLoop;

/* ---------------------------------------------------------------- MML */
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

static int swing_map(int p) { return (p % 48) >= 24 ? p - (p % 48) + 32 + ((p % 48) - 24) * 16 / 24 : p - (p % 48) + (p % 48) * 32 / 24; }

static void compile(const char *src, Seq *out) {
  Note *buf = (Note *)malloc(sizeof(Note) * 4096);
  int n = 0, oct = 4, len = 48, vol = 10, wave = 2, env = 0, tempo = 120, swing = 0;
  int pos = 0;
  const char *p = src;
  while (*p && n < 4000) {
    char c = (char)tolower((unsigned char)*p);
    if (*p == 'S') { p++; swing = parse_num(&p, 1); continue; }
    if (*p == 'E') { p++; env = parse_num(&p, 0); continue; }
    p++;
    if (c >= 'a' && c <= 'g' || c == 'r') {
      static const int semis[7] = {9, 11, 0, 2, 4, 5, 7};
      int semi = c == 'r' ? 0 : semis[c - 'a'];
      if (*p == '+' || *p == '#') { semi++; p++; }
      else if (*p == '-') { semi--; p++; }
      int l = parse_num(&p, 0);
      int ticks = l ? 192 / l : len;
      if (*p == '.') { ticks = ticks * 3 / 2; p++; }
      int start = swing ? swing_map(pos) : pos, end = swing ? swing_map(pos + ticks) : pos + ticks;
      pos += ticks;
      Note *nt = &buf[n++];
      nt->ticks = end - start;
      nt->rest = c == 'r';
      nt->freq = c == 'r' ? 0 : note_freq(oct, semi);
      nt->vol = (u8)vol; nt->wave = (u8)wave; nt->env = (u8)env;
    } else if (c == 'o') oct = parse_num(&p, 4);
    else if (c == '>') oct++;
    else if (c == '<') oct--;
    else if (c == 'l') { int l = parse_num(&p, 4); len = 192 / (l ? l : 4); if (*p == '.') { len = len * 3 / 2; p++; } }
    else if (c == 'v') vol = parse_num(&p, 10);
    else if (c == '@') wave = parse_num(&p, 2);
    else if (c == 't') tempo = parse_num(&p, 120);
    /* [ ] rozwijane wcześniej przez expand() */
  }
  out->n = buf;
  out->count = n;
  out->tickSamples = AUDIO_RATE * 60 / (tempo * 48);
  if (out->tickSamples < 1) out->tickSamples = 1;
}

/* Rozwija [ ... ]n w tekst przed kompilacją. */
static char *expand(const char *src) {
  size_t cap = strlen(src) * 16 + 64;
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
typedef struct { const char *name; int loop; const char *ch[NMUS]; } Track;

static const Track TRACKS[] = {
  {"tytul", 1, {
    "t70 @1 E3 v10 o4 l4 e.d8ce a2ge f.e8df e1 e.d8ce a2b>c< b.a8g+b a1",
    "t70 @3 E0 v12 o2 l1 a a d e a f e a",
    "t70 @2 E1 v5 o3 l8 a>cec<a>cec< a>cec<a>cec< dfafdfaf eg+bg+eg+bg+ a>cec<a>cec< fa>c<afa>c<a eg+bg+eg+bg+ a>cec<a>cec<",
    0}},
  {"polonia", 1, {
    "t132 @1 E1 v9 o4 l8 egeg >c4<ba gfed e4c4 egeg >c4dc< bagb >c4r4< "
    "o4 df+af+ g4b4 agf+e d4r4 df+af+ g4b4 a>c<bg+ a4r4",
    "t132 @3 E1 v12 o2 l4 cg cg <g>d cg cg cg <g>d cg <d>a <g>d <d>a <g>d <d>a <g>d <e>b <a>e",
    "t132 @2 E2 v5 o4 l8 rerg rerg rdrf rerg rerg rerg rdrf rerg rf+ra rgrb rf+ra rgrb rf+ra rgrb rg+rb rarc",
    "t132 @4 E2 v4 o5 l8 [rcrc]16"}},
  {"bar", 1, {
    "t92 S1 @1 E1 v9 o4 r8gb-ge-cr4 c4e-8e8g4r4 r8>c<b-gb-4g4 e-8c8r4r2 "
    "r8fa>ce-4<c4 c8a8f8a8>c2< r8gece-eg4 c2r2 r8dfgb4>d4< c4a8f8e-4c4 e-8e8g8>c8<b-4g4 f8d8b8g8r2",
    "t92 S1 @3 E1 v12 o2 l4 cega cega cega cega fa>c<a fa>c<a cega cega gb>d<b fa>c<a cega gb>d<b",
    "t92 S1 @2 E1 v4 o4 l2 [e-g]4 [e-a]2 [e-g]2 [fb]1 [e-a]1 [e-g]1 [fb]1",
    "t92 S1 @4 E2 v4 o6 l8 [r4cc]24"}},
  {"italia", 1, {
    "t150 @2 E1 v9 o4 d2f4 a2g4 f4e4c+4 e2. g2b-4 a2f4 e4d4c+4 d2. "
    "a2>d4< >f2e4< >e4d4c+4< a2. b-2>d4< a2f4 e4c+4e4 d2.",
    "t150 @3 E1 v12 o2 l4 d<aa> d<aa> <aee> <aee> <gdd> d<aa> <aee> d<aa> "
    "d<aa> d<aa> <aee> <aee> <gdd> d<aa> <aee> d<aa>",
    "t150 @1 E2 v4 o4 l4 [rfa]2 [reg]2 rgb- rfa reg rfa [rfa]2 [reg]2 rgb- rfa reg rfa",
    0}},
  {"doki", 1, {
    "t80 @1 E3 v8 o4 r1 a2.g4 f2e2 d1 r1 a2.b-4 a2g2 a1",
    "t80 @3 E1 v12 o2 l8 [dddddddc]2 [<b->b-b-b-b-b-b-a]2 [dddddddc]2 [<a>aaaaaaa]2",
    "t80 @2 E1 v3 o5 l4 r2dr r2fr r2dr r2c+r r2dr r2fr r2dr r2c+r",
    "t80 @4 E2 v6 o3 l4 [cr8c8rr]8"}},
  {"walka", 1, {
    "t160 @1 E1 v10 o4 l8 aaa>c<a4ge ga>c<ag4ed eee gae4dc de>c<ba4r4 "
    "aaa>c<a4ge ga>c<ag4ed ede gab>c<b ag+ab>c4<r4",
    "t160 @3 E1 v12 o2 l8 [aa>a<a]4 [ff>f<f]2 [gg>g<g]2 [aa>a<a]4 [ff>f<f]2 [ee>e<e]2",
    "t160 @2 E2 v4 o4 l4 [rcre]4 [rcrf]2 [rdrg]2 [rcre]4 [rcrf]2 [rbrg+]2",
    "t160 @4 E2 v5 o4 l8 [c>c<cc>cc<c>c<]16"}},
  {"boss", 1, {
    "t170 @1 E1 v11 o4 l8 eeg e>d<e4 d+e g4ab>c4<b4 eeg e>d<e4 d+e b4ag+a2 "
    ">c4<bag4f+e f+gf+e d+4<b4> eeg e>d<e4 d+e b4>d+4e2<",
    "t170 @3 E1 v12 o2 l8 [ee>e<e]4 [cc>c<c]2 [<b>b<b>b]2 [ee>e<e]4 [cc>c<c]2 [<b>b<b>b]2",
    "t170 @0 E2 v4 o4 l4 [rgrb]4 [rgr>c<]2 [rf+rb]2 [rgrb]4 [rgr>c<]2 [rf+rd+]2",
    "t170 @4 E2 v6 o4 l8 [c>cc<c>c<c>cc<]16"}},
  {"wygrana", 0, {
    "t150 @1 E1 v11 o4 l16 cegb>c8<g8>c4.r4",
    "t150 @3 E1 v12 o2 l8 cgcgc4.r4",
    "t150 @2 E1 v5 o4 l16 egb>de8<b8>e4.r4",
    0}},
  {"koniec", 1, {
    "t76 @1 E3 v9 o4 l4 g.f8eg >c2<ba g.f8e>c< d1 e.d8ce a2g>c< b.a8gb >c1<",
    "t76 @3 E0 v12 o2 l1 c c f g a f g c",
    "t76 @2 E1 v4 o3 l8 [cegecege]2 fa>c<afa>c<a gb>d<bgb>d<b a>cec<a>cec< fa>c<afa>c<a gb>d<bgb>d<b cegecege",
    0}},
  {"biuro", 1, {
    "t84 S1 @1 E3 v8 o4 l8 r4 g4 f+g a4. g4 e4 d2 r4 r4 a4 g+a b4. a4 f4 e2 r4 r4 >c4< bag4 e4 d4 c4 e4 g2.",
    "t84 S1 @3 E1 v11 o2 l4 cgeg cgeg dafa dafa ea>c<a dafa gb>d<b cgec",
    "t84 S1 @2 E3 v3 o4 l1 [eg] [eg] [fa] [fa] [g>c<] [fa] [fb] [eg]",
    "t84 S1 @4 E2 v3 o6 l8 [rcrc]16"}},
  {"akcja", 1, {
    "t140 @1 E1 v9 o4 l8 d4.dfd e-4d4 d4.dfa g4f4 d4.dfd e-4d4 c4.<a>c e-4d4",
    "t140 @3 E1 v12 o2 l8 [dddd>d<ddd]4 [<g>ggg>g<ggg]2 [<a>aaa>a<aaa]2",
    "t140 @2 E2 v3 o4 l4 [rd]4 [rf]4 [rd]4 [re-]4",
    "t140 @4 E2 v5 o4 l8 [crcc>c<rcr]8"}},
  {"pogrzeb", 1, {
    "t50 @2 E3 v7 o4 l2 a. g4 f e d1 f. e4 d c+ d1",
    "t50 @3 E0 v10 o2 l1 d <a> <b-> <a> d",
    "t50 @0 E3 v4 o3 l1 [fa] [eg] [df] [eg] [fa]",
    0}},
  {"chiny", 1, {
    "t96 @1 E1 v8 o5 l8 e4ga>c4<ag e4dc d2 e4ga>c4d<c a4ge g2 a4>c<a g4ed e4dc <a2>",
    "t96 @3 E1 v11 o2 l4 aea>c< aeag aea>c< g>dg<g aea>c< aeag aea>c< e<a>e<a>",
    "t96 @0 E2 v3 o5 l4 [rere]4 [rdrd]2 [rere]2",
    "t96 @4 E2 v3 o6 l8 [crrc rcrr]8"}},
  {"kosciol", 1, {
    "t56 @2 E3 v6 o4 l2 e f g e d e f e",
    "t56 @3 E0 v10 o2 l1 c f c g",
    "t56 @0 E3 v5 o4 l2 c c e c <b> c d c",
    0}},
};
#define NTRACKS ((int)(sizeof(TRACKS) / sizeof(TRACKS[0])))
static Seq trackSeq[NTRACKS][NMUS];
static int curTrack = -1;

static const char *SFXSRC[SFX_COUNT] = {
  "t300 @2 E2 v7 o6 c32",
  "t300 @1 E2 v9 o5 c32g32",
  "t300 @1 E2 v9 o4 g32c32",
  "t300 @4 E2 v15 o3 c8",
  "t300 @4 E2 v15 o4 c16 o2 c8",
  "t300 @4 E2 v15 o5 c4",
  "t240 @3 E1 v12 o5 c16e16g16>c8",
  "t300 @4 E2 v8 o2 c16r32c16",
  "t300 @1 E1 v9 o6 e32g32>c16",
  "t240 @1 E1 v10 o5 c16e16g16>c16e16g8",
  "t400 @2 E0 v9 o4 c32c+32d32d+32e32f32f+32g32g+32a32",
  "t300 @2 E2 v9 o5 g32e32c32<g32",
  "t200 @1 E1 v10 o4 e8c8<a4",
  "t300 @4 E2 v12 o6 c16 o5 c16 o4 c8",
  "t500 @2 E2 v2 o6 c64",
  "t300 @4 E2 v15 o5 c16 o3 c16",
  "t400 @4 E2 v13 o5 c32 o4 c32",
  "t260 @4 E2 v15 o4 c8 o2 c8",
  "t300 @4 E2 v12 o2 c16",
  "t300 @2 E2 v8 o3 c32r16 o4 c32r32 o5 c32",
  "t400 @2 E2 v6 o6 c64",
  "t300 @4 E2 v3 o1 c64",
  "t300 @1 E2 v12 o3 c16<g16",
};
static const char *SFXNAMES[SFX_COUNT] = {"blip", "ok", "cancel", "hit", "crit", "shot", "heal", "door",
                                          "cash", "level", "encounter", "flee", "die", "magic", "text",
                                          "pistol", "tommy", "shotgun", "punch", "reload", "empty", "step", "hurt"};
static Seq sfxSeq[SFX_COUNT];

void audio_init(void) {
  for (int t = 0; t < NTRACKS; t++)
    for (int c = 0; c < NMUS; c++) {
      if (!TRACKS[t].ch[c]) continue;
      char *ex = expand(TRACKS[t].ch[c]);
      compile(ex, &trackSeq[t][c]);
      free(ex);
    }
  for (int i = 0; i < SFX_COUNT; i++) compile(SFXSRC[i], &sfxSeq[i]);
  for (int i = 0; i < NMUS; i++) mus[i].lfsr = 1;
  for (int i = 0; i < NSFX; i++) sfxv[i].lfsr = 1, sfxGain[i] = 1;
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

static void voice_start(Voice *v, const Seq *s) {
  v->seq = s;
  v->idx = -1;
  v->left = 0;
  v->done = !s || !s->count;
}

void music_play(const char *name) {
  int t = -1;
  if (name) for (int i = 0; i < NTRACKS; i++) if (!strcmp(TRACKS[i].name, name)) t = i;
  if (t == curTrack) return;
  curTrack = t;
  musLoop = t >= 0 ? TRACKS[t].loop : 0;
  for (int c = 0; c < NMUS; c++) {
    mus[c].amp = 0;
    voice_start(&mus[c], t >= 0 && TRACKS[t].ch[c] ? &trackSeq[t][c] : NULL);
  }
}

static void sfx_play_gain(int id, float gain) {
  if (id < 0 || id >= SFX_COUNT) return;
  int slot = 0;
  if (id != SFX_TEXT) {
    /* wolny kanał, a jeśli brak — najstarszy w rotacji */
    slot = -1;
    for (int i = 1; i < NSFX; i++) if (sfxv[i].done || !sfxv[i].seq) { slot = i; break; }
    if (slot < 0) { slot = 1 + sfxNext; sfxNext = (sfxNext + 1) % (NSFX - 1); }
  }
  Voice *v = &sfxv[slot];
  v->amp = 0;
  sfxGain[slot] = gain;
  voice_start(v, &sfxSeq[id]);
}

void sfx_play(int id) { sfx_play_gain(id, 1.0f); }

void sfx_play_at(int id, float dist) {
  if (dist > 16) return;
  float g = 1.0f / (1.0f + dist * 0.25f);
  sfx_play_gain(id, g);
}

static void next_note(Voice *v) {
  v->idx++;
  if (v->idx >= v->seq->count) { v->done = 1; v->amp = 0; return; }
  const Note *n = &v->seq->n[v->idx];
  v->left = n->ticks * v->seq->tickSamples;
  v->noteLen = v->left;
  v->age = 0;
  v->wave = n->wave;
  v->env = n->env;
  v->vol = n->rest ? 0 : n->vol;
  v->freq = n->freq;
  if (!n->rest) {
    v->amp = v->env == 3 ? 0.0f : 1.0f;
    float t = v->env == 1 ? 0.35f : v->env == 2 ? 0.07f : 1e9f;
    v->decay = expf(-1.0f / (t * AUDIO_RATE));
  }
}

static float voice_sample(Voice *v) {
  if (v->done || !v->seq) return 0;
  while (v->left <= 0 && !v->done) next_note(v);
  if (v->done) return 0;
  v->left--;
  v->age++;
  if (!v->vol) return 0;
  /* obwiednia */
  if (v->env == 3) { if (v->amp < 0.85f) v->amp += 1.0f / (0.05f * AUDIO_RATE); }
  else if (v->env == 0) { if (v->age < 100) v->amp = v->age / 100.0f; else v->amp = 1.0f; }
  else v->amp *= v->decay;
  float rel = v->left < 220 ? v->left / 220.0f : 1.0f;
  float s;
  float inc = v->freq / AUDIO_RATE;
  if (v->wave == 4) {
    v->phase += inc * 8;
    while (v->phase >= 1) {
      v->phase -= 1;
      unsigned bit = ((v->lfsr >> 0) ^ (v->lfsr >> 1)) & 1;
      v->lfsr = (v->lfsr >> 1) | (bit << 14);
    }
    s = (v->lfsr & 1) ? 1.0f : -1.0f;
  } else {
    v->phase += inc;
    if (v->phase >= 1) v->phase -= 1;
    if (v->wave == 3) s = 4.0f * fabsf(v->phase - 0.5f) - 1.0f;
    else {
      float duty = v->wave == 0 ? 0.125f : v->wave == 1 ? 0.25f : 0.5f;
      s = v->phase < duty ? 1.0f : -1.0f;
    }
  }
  return s * v->amp * rel * (v->vol / 15.0f);
}

void audio_render(int16_t *out, int n) {
  float master = g_volume / 10.0f;
  for (int i = 0; i < n; i++) {
    float mix = 0;
    for (int c = 0; c < NMUS; c++) mix += voice_sample(&mus[c]) * (mus[c].wave == 3 ? 0.30f : 0.16f);
    int allDone = curTrack >= 0;
    for (int c = 0; c < NMUS; c++) if (mus[c].seq && !mus[c].done) allDone = 0;
    if (allDone && musLoop) for (int c = 0; c < NMUS; c++) if (mus[c].seq) voice_start(&mus[c], mus[c].seq);
    mix += voice_sample(&sfxv[0]) * 0.2f;
    for (int c = 1; c < NSFX; c++) mix += voice_sample(&sfxv[c]) * 0.26f * sfxGain[c];
    float v = mix * master;
    if (v > 1) v = 1;
    if (v < -1) v = -1;
    out[i] = (int16_t)(v * 30000);
  }
}
