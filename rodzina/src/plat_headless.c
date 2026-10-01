/* Platforma bez okna (Linux): testy automatyczne, walidacja fabuły i — z --render —
 * rysowanie przez OSMesa (OpenGL programowy) do zrzutów ekranu. */
#include "engine.h"
#include "glapi.h"

typedef struct osmesa_context *OSMesaContext;
extern OSMesaContext OSMesaCreateContextAttribs(const int *attribList, OSMesaContext sharelist);
extern unsigned char OSMesaMakeCurrent(OSMesaContext ctx, void *buffer, unsigned type, int width, int height);
extern void *OSMesaGetProcAddress(const char *funcName);
static void *gp(const char *n) { return OSMesaGetProcAddress(n); }

int main(int argc, char **argv) {
  snprintf(g_data_dir, sizeof g_data_dir, "./");
  long maxFrames = 60L * 60 * 30;
  int w = 1280, h = 720;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--frames") && i + 1 < argc) maxFrames = atol(argv[++i]);
    else if (!strcmp(argv[i], "--render")) g_render = 1;
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
  game_init(argc, argv);
  int16_t audio[400];
  for (long f = 0; f < maxFrames && !g_quit; f++) {
    g_mouse_dx = g_mouse_dy = 0;
    game_frame();
    audio_render(audio, AUDIO_RATE / 60);
  }
  return 0;
}
