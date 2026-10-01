/* KroniX Engine — rdzeń: wspólny stan wejścia i drobne narzędzia. */
#include "kx.h"

int g_btn_raw[BTN_COUNT];
float g_mouse_dx, g_mouse_dy;
int g_mouse_x = -1, g_mouse_y = -1, g_want_mouse_capture;
int g_quit, g_fullscreen_toggle;
char g_data_dir[512];
int g_render;
float g_alpha = 1.0f;
int g_mouse_sens = 5;
Input in;

void input_update(void) {
  for (int b = 0; b < BTN_COUNT; b++) {
    int h = g_btn_raw[b] != 0;
    in.pressed[b] = h && !in.held[b];
    in.holdT[b] = h ? in.holdT[b] + 1 : 0;
    in.held[b] = h;
    in.rep[b] = in.pressed[b] || (in.holdT[b] > 18 && (in.holdT[b] - 18) % 5 == 0);
  }
  float sens = g_mouse_sens / 5.0f;
  in.mdx = g_mouse_dx * sens;
  in.mdy = g_mouse_dy * sens;
  g_mouse_dx = g_mouse_dy = 0;
  in.click = in.pressed[BTN_FIRE];
  in.mx = g_mouse_x; in.my = g_mouse_y;
}

void input_clear(void) {
  for (int b = 0; b < BTN_COUNT; b++) in.pressed[b] = in.rep[b] = 0;
  in.click = 0;
}

int utf8_next(const char **p) {
  const unsigned char *s = (const unsigned char *)*p;
  int c = *s;
  if (c < 0x80) { *p += 1; return c; }
  if ((c & 0xE0) == 0xC0 && s[1]) { *p += 2; return ((c & 0x1F) << 6) | (s[1] & 0x3F); }
  if ((c & 0xF0) == 0xE0 && s[1] && s[2]) { *p += 3; return ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); }
  *p += 1;
  return '?';
}
