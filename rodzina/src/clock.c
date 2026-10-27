/* Pora dnia i pogoda: zegar gry (1 sekunda = 1 minuta), płynne przejścia zmierzch/świt,
 * deszcz przychodzący falami, godziny otwarcia sklepów, sen do wybranej godziny. */
#include "engine.h"

float g_nightK = 1.0f, g_rainK = 0.0f, g_duskK = 0.0f;
float g_rainForce = -1.0f; /* debug/testy: wymuszona pogoda (<0 = naturalna) */
static int sub;

static float smooth01(float a, float b, float x) {
  float t = (x - a) / (b - a);
  t = t < 0 ? 0 : t > 1 ? 1 : t;
  return t * t * (3 - 2 * t);
}
/* 0 w dzień (7:00–17:30), 1 w nocy (20:30–5:30), płynnie o świcie i zmierzchu */
float clock_night(int m) {
  float h = m / 60.0f;
  if (h < 12) return 1.0f - smooth01(5.5f, 7.0f, h);
  return smooth01(17.5f, 20.5f, h);
}
/* złota godzina: maksimum ok. 18:45 i 6:15 */
static float clock_dusk(int m) {
  float h = m / 60.0f;
  float a = 1.0f - fabsf(h - 18.75f) / 1.6f, b = 1.0f - fabsf(h - 6.25f) / 1.0f;
  return fmaxf(0, fmaxf(a, b));
}
static float hash1(int n) { unsigned h = (unsigned)n * 2654435761u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; return (h & 0xFFFF) / 65535.0f; }
/* deszcz: wartość szumu w blokach 3-godzinnych, wygładzona */
static float rain_at(long absMin) {
  long blk = absMin / 180;
  float f = (absMin % 180) / 180.0f;
  float a = hash1((int)blk), b = hash1((int)blk + 1);
  float v = a + (b - a) * (f * f * (3 - 2 * f));
  return smooth01(0.52f, 0.68f, v);
}
static long abs_min(void) { return (long)H.day * 1440 + H.clock; }

void clock_update_env(void) {
  g_nightK = clock_night(H.clock);
  g_duskK = clock_dusk(H.clock);
  g_rainK = g_rainForce >= 0 ? g_rainForce : rain_at(abs_min());
}

void clock_tick(void) {
  if (++sub < 60) return;
  sub = 0;
  if (++H.clock >= 1440) { H.clock = 0; H.day++; }
}

int clock_hour(void) { return H.clock / 60; }
const char *clock_text(void) {
  static char b[16];
  snprintf(b, sizeof b, "%02d:%02d", H.clock / 60, H.clock % 60);
  return b;
}
/* przewiń do najbliższego wystąpienia godziny h:00; zwraca liczbę przespanych minut */
int clock_sleep_until(int h) {
  int target = ((h % 24) + 24) % 24 * 60;
  int d = target - H.clock;
  if (d <= 30) d += 1440;
  H.clock += d;
  while (H.clock >= 1440) { H.clock -= 1440; H.day++; }
  sub = 0;
  clock_update_env();
  return d;
}

/* godziny otwarcia wg rodzaju lokalu (szyld SIGN_TEXT); przedział może przechodzić przez północ */
static const struct { int sign, open, close; } HOURS[] = {
  {0, 6, 18},   /* piekarnia */
  {1, 8, 20},   /* delikatesy */
  {3, 10, 19},  /* lombard */
  {6, 7, 21},   /* trafika */
  {7, 8, 20},   /* apteka */
  {9, 15, 3},   /* pub */
  {10, 11, 24}, /* restauracja hotelowa */
  {12, 7, 20},  /* sklep spożywczy */
  {13, 11, 2},  /* chop suey */
  {14, 9, 21},  /* herbaciarnia */
  {15, 9, 18},  /* narzędzia Gina */
};
int shop_hours(int sign, int *open, int *close) {
  for (unsigned i = 0; i < sizeof HOURS / sizeof HOURS[0]; i++)
    if (HOURS[i].sign == sign) { *open = HOURS[i].open; *close = HOURS[i].close; return 1; }
  *open = 0; *close = 24;
  return 0;
}
int shop_is_open(int sign) {
  int o, c, h = clock_hour();
  shop_hours(sign, &o, &c);
  if (c > 24) c -= 24;
  if (o < c) return h >= o && h < c;
  return h >= o || h < c; /* przez północ */
}
