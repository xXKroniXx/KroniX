/* KroniX Engine — Platforma bez okna (Linux): testy automatyczne, walidacja fabuły i — z --render —
 * rysowanie przez OSMesa (OpenGL programowy) do zrzutów ekranu. */
#include "kx.h"
#include "kx_prof.h"
#include "render.h"
#include "kx_audio.h"
#include "glapi.h"
#include <math.h>
#include <time.h>

typedef struct osmesa_context *OSMesaContext;
extern OSMesaContext OSMesaCreateContextAttribs(const int *attribList, OSMesaContext sharelist);
extern unsigned char OSMesaMakeCurrent(OSMesaContext ctx, void *buffer, unsigned type, int width, int height);
extern void *OSMesaGetProcAddress(const char *funcName);
static void *gp(const char *n) { return OSMesaGetProcAddress(n); }

int main(int argc, char **argv) {
  snprintf(g_data_dir, sizeof g_data_dir, "./");
  long maxFrames = 60L * 60 * 30, shotAt = -1;
  const char *shotPath = NULL;
  int w = 1280, h = 720;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--frames") && i + 1 < argc) maxFrames = atol(argv[++i]);
    else if (!strcmp(argv[i], "--render")) g_render = 1;
    else if (!strcmp(argv[i], "--shot") && i + 2 < argc) { shotAt = atol(argv[++i]); shotPath = argv[++i]; } /* zrzut po N klatkach */
    else if (!strcmp(argv[i], "--size") && i + 2 < argc) { w = atoi(argv[++i]); h = atoi(argv[++i]); }
  }
  if (g_render) {
    int at[] = {0x22, 0x1908, 0x30, 24, 0x33, 0x34, 0x36, 3, 0x37, 3, 0};
    OSMesaContext c = OSMesaCreateContextAttribs(at, NULL);
    static void *buf;
    buf = malloc((size_t)w * h * 4);
    const char *miss = NULL;
    if (!c || !OSMesaMakeCurrent(c, buf, 0x1401, w, h) || gl_load(gp, &miss)) {
      fprintf(stderr, "OSMesa/GL niedostępne (%s) — bez renderingu\n", miss ? miss : "?");
      g_render = 0;
    }
    g_winW = w; g_winH = h;
  }
  kx_game_init(argc, argv);
  g_cfg.dynres = 0; /* testy i zrzuty: zawsze pełna, stała rozdzielczość */
  for (int i = 1; i + 3 < argc; i++)
    if (!strcmp(argv[i], "--wav")) { /* --wav utwór|sfx:nazwa|amb:N sekundy plik.wav */
      const char *what = argv[i + 1];
      int sec = atoi(argv[i + 2]), frames = sec * AUDIO_RATE;
      if (!strncmp(what, "sfx:", 4)) { sfx_play(sfx_find(what + 4)); music_play(NULL); }
      else if (!strncmp(what, "amb:", 4)) { music_play(NULL); audio_ambience(atoi(what + 4), what[4] == '1' ? 1.0f : 0.0f, 0); }
      else music_play(what);
      int16_t *pcm = malloc((size_t)frames * 4);
      clock_t c0 = clock();
      for (int f = 0; f < frames; f += 1024) audio_render(pcm + f * 2, frames - f < 1024 ? frames - f : 1024);
      double el = (double)(clock() - c0) / CLOCKS_PER_SEC;
      long peak = 0; double rms = 0;
      for (int k = 0; k < frames * 2; k++) { long v = labs((long)pcm[k]); if (v > peak) peak = v; rms += (double)pcm[k] * pcm[k]; }
      printf("%s: %d s, CPU %.2f s (%.1f%%), szczyt %ld, RMS %.0f\n", what, sec, el, el * 100 / sec, peak, sqrt(rms / (frames * 2.0)));
      FILE *f = fopen(argv[i + 3], "wb");
      unsigned hdr[11] = {0x46464952, 36 + frames * 4, 0x45564157, 0x20746d66, 16, 0x00020001, AUDIO_RATE, AUDIO_RATE * 4, 0x00100004, 0x61746164, frames * 4};
      fwrite(hdr, 4, 11, f); fwrite(pcm, 4, frames, f); fclose(f);
      exit(0);
    }
  static int16_t audio[AUDIO_RATE / 60 * 2 + 16];
  /* KX_BENCH=1: rysuj każdą klatkę i wypisz uśrednione czasy sekcji (profil CPU) */
  int bench = getenv("KX_BENCH") != NULL;
  for (long f = 0; f < maxFrames && !g_quit; f++) {
    g_mouse_dx = g_mouse_dy = 0;
    kx_prof_begin(KXP_UPDATE);
    kx_game_frame();
    kx_prof_end(KXP_UPDATE);
    if (bench && g_render && !g_quit) { kx_prof_begin(KXP_DRAW); kx_game_draw(); kx_prof_end(KXP_DRAW); }
    if (f == shotAt && g_render && shotPath) { kx_game_draw(); r_screenshot(shotPath); g_quit = 1; }
    kx_prof_begin(KXP_MAX - 1);
    audio_render(audio, AUDIO_RATE / 60);
    kx_prof_end(KXP_MAX - 1);
    kx_prof_frame();
    if (bench && f % 120 == 119) {
      int d, t, sc;
      r_stats(&d, &t, &sc);
      fprintf(stderr, "bench skala %d%% f=%ld logika %.2f ms  rysowanie %.2f ms  dźwięk %.2f ms  draws %d  tris %dk\n", sc, f + 1,
              kx_prof_ms(KXP_UPDATE), kx_prof_ms(KXP_DRAW), kx_prof_ms(KXP_MAX - 1), d, t / 1000);
    }
  }
  return 0;
}
