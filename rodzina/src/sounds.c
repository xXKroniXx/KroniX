/* KroniX: Rodzina — muzyka (aranżacje jazzowe w MML) i definicje efektów dźwiękowych.
 * Syntezator, instrumenty i narzędzia efektów są w silniku: engine/audio/kx_audio.h. */
#include "engine.h"

/* ---------------------------------------------------------------- utwory */
static const KxTrack TRACKS[] = {
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

/* ---------------------------------------------------------------- efekty (kolejność = enum SFX_*) */
static const KxSfxDef SFX[SFX_COUNT] = {
  {"blip", 0.35f, 1, 0.02f, 0}, {"ok", 0.45f, 1, 0.02f, 0}, {"cancel", 0.4f, 1, 0.02f, 0}, {"hit", 0.8f, 3, 0.02f, 0},
  {"crit", 0.9f, 1, 0.02f, 0}, {"shot", 0.9f, 1, 0.02f, 0}, {"heal", 0.45f, 1, 0.02f, 0}, {"door", 0.6f, 1, 0.02f, 0},
  {"cash", 0.6f, 1, 0.02f, 0}, {"level", 0.45f, 1, 0.02f, 0}, {"encounter", 0.6f, 1, 0.02f, 0}, {"flee", 0.5f, 1, 0.02f, 0},
  {"die", 0.7f, 1, 0.02f, 0}, {"magic", 0.45f, 1, 0.02f, 0}, {"text", 0.16f, 3, 0.02f, 1},
  {"pistol", 0.95f, 3, 0.08f, 0}, {"tommy", 0.85f, 3, 0.08f, 0}, {"shotgun", 1.0f, 1, 0.08f, 0}, {"punch", 0.75f, 3, 0.08f, 0},
  {"reload", 0.55f, 1, 0.02f, 0}, {"empty", 0.5f, 1, 0.02f, 0}, {"step", 0.3f, 3, 0.16f, 0}, {"hurt", 0.7f, 1, 0.02f, 0},
  {"horn", 0.6f, 1, 0.02f, 0}, {"phone", 0.5f, 1, 0.02f, 0}, {"glass", 0.5f, 1, 0.02f, 0}, {"bell", 0.6f, 1, 0.02f, 0},
  {"type", 0.4f, 1, 0.02f, 0}, {"paper", 0.4f, 1, 0.02f, 0}, {"car", 0.5f, 1, 0.02f, 0},
};

static void gen(int id, int v, KxBuf *b, unsigned *rp) {
  unsigned r = *rp;
  float *d;
  float jit = 1 + (v - 1) * 0.06f;
  switch (id) {
    case SFX_BLIP:
      d = kx_fx_new(b, 0.06f);
      kx_fx_click(d, b->n, 0, 1800, 0.3f, 220, &r);
      kx_fx_thump(d, b->n, 0, 900, 600, 0.5f, 90);
      break;
    case SFX_OK:
      d = kx_fx_new(b, 0.5f);
      kx_fx_bell(d, b->n, 0, 659, 0.5f, 7, 2.0f);
      kx_fx_bell(d, b->n, 0.07f, 988, 0.5f, 6, 2.0f);
      break;
    case SFX_CANCEL:
      d = kx_fx_new(b, 0.35f);
      kx_fx_bell(d, b->n, 0, 494, 0.5f, 9, 1.0f);
      kx_fx_bell(d, b->n, 0.06f, 330, 0.5f, 9, 1.0f);
      break;
    case SFX_HIT:
      d = kx_fx_new(b, 0.25f);
      kx_fx_thump(d, b->n, 0, 160 * jit, 55, 0.9f, 22);
      kx_fx_noise(d, b->n, 0, 900, 60, 0.9f, 0.001f, 30, &r);
      break;
    case SFX_CRIT:
      d = kx_fx_new(b, 0.35f);
      kx_fx_thump(d, b->n, 0, 180, 50, 1.0f, 18);
      kx_fx_noise(d, b->n, 0, 6000, 900, 0.8f, 0.0005f, 60, &r);
      kx_fx_noise(d, b->n, 0, 900, 60, 0.8f, 0.001f, 25, &r);
      break;
    case SFX_FIRE: case SFX_PISTOL:
      kx_fx_gunshot(b, 0.7f, 38 * jit, 62 * jit, 11, 0.9f, &r);
      break;
    case SFX_TOMMY:
      kx_fx_gunshot(b, 0.35f, 55 * jit, 70 * jit, 16, 0.75f, &r);
      break;
    case SFX_SHOTGUN:
      kx_fx_gunshot(b, 1.1f, 22, 48, 6, 1.3f, &r);
      d = b->d;
      kx_fx_click(d, b->n, 0.62f, 2200, 0.25f, 120, &r); /* przeładowanie pompką */
      kx_fx_thump(d, b->n, 0.62f, 300, 150, 0.08f, 60);
      kx_fx_click(d, b->n, 0.78f, 1900, 0.3f, 100, &r);
      kx_fx_thump(d, b->n, 0.78f, 280, 140, 0.08f, 60);
      break;
    case SFX_HEAL:
      d = kx_fx_new(b, 1.0f);
      kx_fx_vibe(d, b->n, 0, 523, 0.35f); kx_fx_vibe(d, b->n, 0.08f, 659, 0.35f); kx_fx_vibe(d, b->n, 0.16f, 784, 0.35f);
      break;
    case SFX_DOOR: {
      d = kx_fx_new(b, 0.95f);
      /* skrzypnięcie zawiasów */
      float ph = 0, lp = 0;
      for (int i = 0; i < (int)(0.5f * AUDIO_RATE); i++) {
        float t = i / (float)AUDIO_RATE;
        float f = 210 + 60 * sinf(t * 9) + 40 * sinf(t * 23);
        ph += f / AUDIO_RATE;
        if (ph > 1) ph -= 1;
        float x = (ph < 0.15f ? 1.0f : -0.18f) * (0.6f + 0.4f * kx_fx_sin(t * 37));
        lp += (x - lp) * 0.2f;
        d[i] += lp * sinf(3.14159f * t / 0.5f) * 0.25f;
      }
      kx_fx_click(d, b->n, 0.55f, 900, 0.5f, 90, &r);
      kx_fx_thump(d, b->n, 0.55f, 140, 70, 0.8f, 25);
      break;
    }
    case SFX_CHEST: /* kasa fiskalna */
      d = kx_fx_new(b, 1.1f);
      for (int k = 0; k < 4; k++) kx_fx_click(d, b->n, k * 0.035f, 1200 + k * 200, 0.35f, 140, &r);
      kx_fx_thump(d, b->n, 0.14f, 200, 90, 0.35f, 30);
      kx_fx_bell(d, b->n, 0.16f, 2093, 0.32f, 4, 1.41f);
      kx_fx_bell(d, b->n, 0.16f, 2637, 0.2f, 5, 1.41f);
      break;
    case SFX_LEVEL:
      d = kx_fx_new(b, 1.4f);
      kx_fx_vibe(d, b->n, 0, 523, 0.3f); kx_fx_vibe(d, b->n, 0.1f, 659, 0.3f);
      kx_fx_vibe(d, b->n, 0.2f, 784, 0.3f); kx_fx_vibe(d, b->n, 0.3f, 1047, 0.35f);
      break;
    case SFX_ENCOUNTER: {
      d = kx_fx_new(b, 1.2f);
      static const float F[4] = {196, 233, 277, 330};
      for (int k = 0; k < 4; k++) {
        float ph = 0, lp = 0;
        for (int i = 0; i < b->n; i++) {
          float t = i / (float)AUDIO_RATE;
          ph += F[k] / AUDIO_RATE; if (ph > 1) ph -= 1;
          lp += ((2 * ph - 1) - lp) * kx_fx_lp(600 + 3000 * expf(-t * 6));
          d[i] += lp * (t < 0.02f ? t / 0.02f : expf(-(t - 0.02f) * 2.5f)) * 0.3f;
        }
      }
      kx_fx_noise(d, b->n, 0, 9000, 2000, 0.5f, 0.001f, 2.5f, &r);
      kx_fx_thump(d, b->n, 0, 120, 45, 0.8f, 8);
      break;
    }
    case SFX_FLEE:
      d = kx_fx_new(b, 0.45f);
      kx_fx_noise(d, b->n, 0, 2500, 400, 0.8f, 0.2f, 6, &r);
      break;
    case SFX_DIE:
      d = kx_fx_new(b, 0.8f);
      kx_fx_grunt(d, b->n, 0, 120, 0.45f, 0.7f, 560, 950, &r);
      kx_fx_thump(d, b->n, 0.35f, 110, 45, 0.9f, 10);
      kx_fx_noise(d, b->n, 0.35f, 700, 50, 0.6f, 0.002f, 18, &r);
      break;
    case SFX_MAGIC:
      d = kx_fx_new(b, 1.0f);
      for (int k = 0; k < 6; k++) kx_fx_bell(d, b->n, k * 0.05f, 1047 * powf(1.122f, (float)k), 0.18f, 5, 3.5f);
      break;
    case SFX_TEXT: /* maszyna do pisania, bardzo cicho */
      d = kx_fx_new(b, 0.05f);
      kx_fx_click(d, b->n, 0, 2500 * jit, 0.5f, 260, &r);
      kx_fx_thump(d, b->n, 0.003f, 400, 220, 0.25f, 120);
      break;
    case SFX_PUNCH:
      d = kx_fx_new(b, 0.25f);
      kx_fx_thump(d, b->n, 0, 140 * jit, 55, 1.0f, 24);
      kx_fx_noise(d, b->n, 0, 2400, 200, 0.7f, 0.0008f, 55, &r);
      break;
    case SFX_RELOAD:
      d = kx_fx_new(b, 0.5f);
      kx_fx_click(d, b->n, 0, 2800, 0.6f, 150, &r);
      kx_fx_click(d, b->n, 0.17f, 2300, 0.5f, 120, &r);
      kx_fx_thump(d, b->n, 0.17f, 500, 300, 0.2f, 70);
      kx_fx_click(d, b->n, 0.36f, 3200, 0.7f, 160, &r);
      break;
    case SFX_EMPTY:
      d = kx_fx_new(b, 0.1f);
      kx_fx_click(d, b->n, 0, 3000, 0.7f, 180, &r);
      break;
    case SFX_STEP:
      d = kx_fx_new(b, 0.16f);
      kx_fx_noise(d, b->n, 0, 2200 * jit, 120, 0.8f, 0.004f, 38, &r);
      kx_fx_thump(d, b->n, 0, 120 * jit, 60, 0.35f, 35);
      kx_fx_noise(d, b->n, 0.03f, 4000, 1200, 0.15f, 0.002f, 60, &r); /* "szur" podeszwy */
      break;
    case SFX_HURT:
      d = kx_fx_new(b, 0.4f);
      kx_fx_thump(d, b->n, 0, 130, 55, 0.8f, 20);
      kx_fx_grunt(d, b->n, 0.02f, 135, 0.22f, 0.9f, 620, 1050, &r);
      break;
    case SFX_HORN: { /* klakson "a-u-ga" */
      d = kx_fx_new(b, 0.9f);
      float ph = 0, lp = 0;
      for (int i = 0; i < b->n; i++) {
        float t = i / (float)AUDIO_RATE;
        float f = t < 0.25f ? 160 + 200 * t / 0.25f : 360 - 90 * fminf(1, (t - 0.25f) / 0.3f);
        ph += f / AUDIO_RATE; if (ph > 1) ph -= 1;
        float x = (2 * ph - 1) + (ph < 0.3f ? 0.6f : -0.2f);
        lp += (x - lp) * kx_fx_lp(1600);
        float e = (t < 0.03f ? t / 0.03f : 1) * (t > 0.75f ? fmaxf(0, 1 - (t - 0.75f) / 0.15f) : 1);
        d[i] = lp * e;
      }
      break;
    }
    case SFX_PHONE: { /* dzwonek telefonu (dwa dzwonki młoteczka) */
      d = kx_fx_new(b, 1.4f);
      for (int k = 0; k < 24; k++) {
        kx_fx_bell(d, b->n, k * 0.04f, 1400, 0.22f, 18, 2.76f);
        kx_fx_bell(d, b->n, k * 0.04f + 0.02f, 1630, 0.22f, 18, 2.76f);
      }
      break;
    }
    case SFX_GLASS:
      d = kx_fx_new(b, 0.6f);
      kx_fx_bell(d, b->n, 0, 2600, 0.4f, 12, 2.3f);
      kx_fx_bell(d, b->n, 0, 3710, 0.3f, 15, 2.3f);
      kx_fx_bell(d, b->n, 0, 5200, 0.15f, 20, 2.3f);
      break;
    case SFX_BELL: /* dzwon kościelny */
      d = kx_fx_new(b, 4.0f);
      kx_fx_bell(d, b->n, 0, 196, 0.5f, 0.9f, 2.0f);
      kx_fx_bell(d, b->n, 0, 392 * 1.19f, 0.25f, 1.2f, 1.0f);
      kx_fx_bell(d, b->n, 0, 588, 0.18f, 1.5f, 1.4f);
      kx_fx_bell(d, b->n, 0, 98, 0.3f, 0.7f, 1.0f);
      break;
    case SFX_TYPE:
      d = kx_fx_new(b, 0.12f);
      kx_fx_click(d, b->n, 0, 2200, 0.7f, 120, &r);
      kx_fx_thump(d, b->n, 0, 600, 250, 0.4f, 60);
      break;
    case SFX_PAPER:
      d = kx_fx_new(b, 0.5f);
      for (int k = 0; k < 6; k++) kx_fx_noise(d, b->n, k * 0.06f + (kx_fx_rand(&r) * 0.5f + 0.5f) * 0.03f, 7000, 1500, 0.4f, 0.01f, 25, &r);
      break;
    case SFX_CAR: { /* silnik auta z lat 30. — odjazd */
      d = kx_fx_new(b, 2.5f);
      float ph = 0, lp = 0;
      for (int i = 0; i < b->n; i++) {
        float t = i / (float)AUDIO_RATE;
        float f = 28 + 22 * fminf(1, t / 1.2f);
        ph += f / AUDIO_RATE; if (ph > 1) ph -= 1;
        float x = (ph < 0.2f ? 1.0f : -0.25f) + kx_fx_rand(&r) * 0.3f;
        lp += (x - lp) * kx_fx_lp(500);
        d[i] = lp * (t < 0.2f ? t / 0.2f : 1) * fmaxf(0, 1 - t / 2.5f);
      }
      break;
    }
    default: d = kx_fx_new(b, 0.05f);
  }
  *rp = r;
}

void game_audio_setup(void) {
  kx_audio_tracks(TRACKS, (int)(sizeof TRACKS / sizeof TRACKS[0]));
  kx_audio_sfx(SFX, SFX_COUNT, gen);
}
