/* KroniX Engine — profiler klatki (patrz kx_prof.h). */
#ifndef _WIN32
#define _POSIX_C_SOURCE 199309L /* clock_gettime w trybie -std=c99 */
#endif
#include "kx_prof.h"
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

int g_prof_show;

static struct {
  const char *name;
  double t0, acc, avg, peak, peakAcc;
  int open;
} P[KXP_MAX] = {{"klatka"}, {"logika"}, {"rysowanie CPU"}, {"GPU"}};
static double lastFrame, fpsT;
static int fpsN;
static float fps;

double kx_time_now(void) {
#ifdef _WIN32
  static LARGE_INTEGER f;
  LARGE_INTEGER c;
  if (!f.QuadPart) QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&c);
  return (double)c.QuadPart / (double)f.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
#endif
}

void kx_prof_name(int id, const char *name) { if (id >= 0 && id < KXP_MAX) P[id].name = name; }
void kx_prof_begin(int id) { if (id >= 0 && id < KXP_MAX) { P[id].t0 = kx_time_now(); P[id].open = 1; } }
void kx_prof_end(int id) {
  if (id < 0 || id >= KXP_MAX || !P[id].open) return;
  P[id].acc += (kx_time_now() - P[id].t0) * 1000.0;
  P[id].open = 0;
}
void kx_prof_add(int id, double ms) { if (id >= 0 && id < KXP_MAX) P[id].acc += ms; }

void kx_prof_frame(void) {
  double now = kx_time_now();
  if (lastFrame > 0) P[KXP_FRAME].acc = (now - lastFrame) * 1000.0;
  lastFrame = now;
  for (int i = 0; i < KXP_MAX; i++) {
    /* wygładzanie wykładnicze (~0,5 s) i szczyt z ostatniej sekundy */
    P[i].avg += (P[i].acc - P[i].avg) * 0.05;
    if (P[i].acc > P[i].peakAcc) P[i].peakAcc = P[i].acc;
    P[i].acc = 0;
  }
  fpsN++;
  if (now - fpsT >= 1.0) {
    fps = (float)(fpsN / (now - fpsT));
    fpsN = 0;
    fpsT = now;
    for (int i = 0; i < KXP_MAX; i++) { P[i].peak = P[i].peakAcc; P[i].peakAcc = 0; }
  }
}

double kx_prof_ms(int id) { return id >= 0 && id < KXP_MAX ? P[id].avg : 0; }
double kx_prof_peak(int id) { return id >= 0 && id < KXP_MAX ? P[id].peak : 0; }
const char *kx_prof_label(int id) { return id >= 0 && id < KXP_MAX ? P[id].name : NULL; }
float kx_prof_fps(void) { return fps; }
