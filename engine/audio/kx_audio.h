/* KroniX Engine — dźwięk: syntezator muzyki (MML, instrumenty akustyczne), efekty
 * syntetyzowane do buforów, tło dźwiękowe miejsc, pogłos, głos silnika auta. Stereo 44,1 kHz.
 *
 * Gra:
 *   1. kx_audio_tracks(tabela utworów)              — przed audio_init
 *   2. kx_audio_sfx(tabela efektów, generator)        — przed audio_init
 *   3. audio_init()                                   — kompiluje utwory i syntetyzuje efekty
 *   4. music_play / sfx_play / audio_ambience ...     — w trakcie gry
 * Platforma woła audio_render() do wypełniania bufora karty dźwiękowej. */
#ifndef KX_AUDIO_H
#define KX_AUDIO_H
#include <stdint.h>

#define AUDIO_RATE 44100 /* stereo, próbki przeplatane L,P */
#define KX_MUS_CH 6
#define KX_MAX_SFX 64
#define KX_SFX_VARIANTS 3

/* utwór: do 6 linii MML (opis składni w kx_audio.c) */
typedef struct { const char *name; int loop; float reverb, vinyl; const char *ch[KX_MUS_CH]; } KxTrack;

/* efekt: nazwa (dla skryptów), głośność, liczba wariantów, rozrzut wysokości przy odtwarzaniu,
 * exclusive = nowy egzemplarz ucina poprzedni (np. stukot maszyny do pisania) */
typedef struct { const char *name; float gain; int variants; float pitchJitter; int exclusive; } KxSfxDef;
typedef struct { float *d; int n; } KxBuf;
/* generator: wypełnia bufor b dla efektu id i wariantu v (r — ziarno losowe) */
typedef void (*KxSfxGen)(int id, int v, KxBuf *b, unsigned *r);

void kx_audio_tracks(const KxTrack *tracks, int n);
void kx_audio_sfx(const KxSfxDef *defs, int n, KxSfxGen gen);
void audio_init(void);
void audio_render(int16_t *out, int frames);

void music_play(const char *name); /* NULL = cisza (wybrzmiewanie) */
int music_exists(const char *name);
int sfx_find(const char *name);
void sfx_play(int id);
void sfx_play_at(int id, float dist);
void sfx_play_pan(int id, float gain, float pan);
extern int g_volume; /* 0..10 */

/* tło dźwiękowe: rodzaj miejsca, deszcz 0..1, akustyka 0 plener / 1 pokój / 2 hala.
 * Wydarzenia tła używają efektów o nazwach: horn, car, bell, text, glass (jeśli są). */
enum { AMB_NONE, AMB_STREET, AMB_HARBOR, AMB_ROOM, AMB_CROWD, AMB_OFFICE, AMB_CHURCH, AMB_WAREHOUSE };
void audio_ambience(int kind, float rain, float space);
void audio_engine(float rpm, float gain); /* silnik pojazdu: obroty 0..1, głośność 0..1 */

/* narzędzia do syntezy efektów (dla generatorów gry) */
float *kx_fx_new(KxBuf *b, float sec);
void kx_fx_norm(KxBuf *b, float peak);
void kx_fx_softclip(KxBuf *b, float drive);
void kx_fx_bell(float *d, int n, float t0, float f, float amp, float dec, float ratio);   /* FM dzwonek */
void kx_fx_vibe(float *d, int n, float t0, float f, float amp);                           /* wibrafon */
void kx_fx_click(float *d, int n, float t0, float hz, float amp, float dec, unsigned *r); /* klik mechaniczny */
void kx_fx_thump(float *d, int n, float t0, float f0, float f1, float amp, float dec);    /* uderzenie basowe */
void kx_fx_noise(float *d, int n, float t0, float lpHz, float hpHz, float amp, float att, float dec, unsigned *r);
void kx_fx_grunt(float *d, int n, float t0, float f0, float dur, float amp, float F1, float F2, unsigned *r); /* głos */
void kx_fx_gunshot(KxBuf *b, float sec, float crackDec, float boomHz, float boomDec, float body, unsigned *r);
float kx_fx_rand(unsigned *r);  /* -1..1 */
float kx_fx_lp(float hz);       /* współczynnik filtra jednobiegunowego */
float kx_fx_sin(float phase);   /* sinus z tablicy, faza w cyklach */

#endif
