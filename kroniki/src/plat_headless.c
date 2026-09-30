/* Platforma bez okna (Linux): do testów automatycznych i walidacji fabuły. */
#include "engine.h"

int main(int argc, char **argv) {
  snprintf(g_data_dir, sizeof g_data_dir, "./");
  long maxFrames = 60L * 60 * 30;
  for (int i = 1; i < argc; i++)
    if (!strcmp(argv[i], "--frames") && i + 1 < argc) maxFrames = atol(argv[++i]);
  game_init(argc, argv);
  int16_t audio[400];
  for (long f = 0; f < maxFrames && !g_quit; f++) {
    game_frame();
    audio_render(audio, AUDIO_RATE / 60);
  }
  return 0;
}
