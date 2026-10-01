/* KroniX Engine — minimalna „gra”: punkt startowy dla nowych projektów.
 * Pokazuje: tekstury proceduralne, siatki, światła punktowe, deszcz, poświaty, UI z czcionkami SDF,
 * muzykę MML i efekty dźwiękowe. Ruch: WASD + mysz, E — dźwięk, Esc — wyjście. */
#include "kx.h"
#include "render.h"
#include "kx_proc.h"
#include "kx_audio.h"

static GMesh scene;
static float px = 0, pz = -6, yaw = 0, pitch = 0.1f;
static long frame;

/* ---- dźwięk: jeden utwór i jeden efekt */
static const KxTrack TRACKS[] = {
  {"demo", 1, 0.4f, 0.3f, {
    "t84 S1 @9 v10 o5 e4.d8c4<a4> g2.r4 e4.d8c4<a4> a2.r4",
    "t84 S1 @3 v12 o2 a2e2 d2a2 a2e2 a2e2",
    "t84 S1 @0 v6 o3 (gb>ce)1 (fa>ce)1 (gb>ce)1 (gb>ce)1",
    "t84 S1 @4 v5 o4 [(ce)4(dg)4e4(dg)4]4", 0, 0}},
};
static const KxSfxDef SFX[] = {{"ding", 0.6f, 1, 0.02f, 0}};
static void sfx_gen(int id, int v, KxBuf *b, unsigned *r) {
  (void)id; (void)v; (void)r;
  float *d = kx_fx_new(b, 1.0f);
  kx_fx_bell(d, b->n, 0, 880, 0.5f, 5, 2.0f);
  kx_fx_bell(d, b->n, 0.08f, 1320, 0.4f, 5, 2.0f);
}

/* ---- tekstury: 0 = bruk, 1 = cegła */
static void make_textures(void) {
  kxp_begin();
  kxp_layers_begin(2);
  kxp_fill(kxp_rgb(0x5A5A60), 0.5f);
  for (int y = 0; y < KXP_N; y++)
    for (int x = 0; x < KXP_N; x++) {
      float f2; float d = kxp_worley((float)x / KXP_N, (float)y / KXP_N, 8, 3, &f2, NULL);
      float edge = f2 - d < 0.08f ? 0.55f : 1.0f;
      KXP_PX(x, y) = kxp_mul(KXP_PX(x, y), edge * (0.85f + 0.3f * kxp_fbm((float)x / KXP_N, (float)y / KXP_N, 8, 4, 1)));
      KXP_HX(x, y) = edge > 0.9f ? 1 : 0;
    }
  kxp_relief(3, 0.3f);
  kxp_layer_store(0);
  kxp_fill(kxp_rgb(0x8A3A28), 0.2f);
  for (int r = 0; r < 16; r++)
    for (int c = 0; c < 6; c++) {
      int ox = (r & 1) * 42, bw = KXP_N / 6;
      kxp_rect(c * bw + ox + 3, r * 32 + 3, bw - 6, 26, kxp_mul(kxp_rgb(0x8A3A28), 0.8f + 0.4f * kxp_rnd01(r, c, 5)), 1, 1);
    }
  kxp_relief(4, 0.4f);
  kxp_grain(0.15f, 8, 2);
  kxp_text_center(1, 64, KXP_N / 2, KXP_N / 2 + 20, "KRONIX", kxp_rgb(0xF0E6D0), 6, 0.05f);
  kxp_layer_store(1);
  r_set_world_textures(kxp_layers_finish(NULL));
  kxp_end();
}

static void make_scene(void) {
  MB b;
  mb_init(&b);
  mb_paint(0xFFFFFFFF, 0); P_.uvs = 0.5f;
  mb_quad_world(&b, v3(-20, 0, -20), v3(-20, 0, 20), v3(20, 0, 20), v3(20, 0, -20));
  mb_paint(0xFFFFFFFF, 1);
  for (int i = 0; i < 6; i++) {
    float x = (i % 3 - 1) * 5.0f, z = (i / 3) * 6.0f;
    mb_rbox(&b, v3(x - 1, 0, z - 1), v3(x + 1, 2.5f + i * 0.4f, z + 1), 0.08f);
  }
  mb_paint(0xFF2A2C30, 0);
  for (int i = 0; i < 4; i++) mb_cyl(&b, v3(-2.5f + i * 1.7f, 0, 2.5f), 0.06f, 0.05f, 2.2f, 8, 0);
  scene = gm_upload(&b);
  mb_free(&b);
}

void kx_game_init(int argc, char **argv) {
  (void)argc; (void)argv;
  if (g_render) {
    r_resize(g_winW, g_winH);
    r_init();
    make_textures();
    make_scene();
  }
  kx_audio_tracks(TRACKS, 1);
  kx_audio_sfx(SFX, 1, sfx_gen);
  audio_init();
  music_play("demo");
  audio_ambience(AMB_STREET, 1.0f, 0);
}

void kx_game_look(void) {
  float s = g_mouse_sens / 5.0f;
  yaw -= g_mouse_dx * s * 0.0032f; pitch -= g_mouse_dy * s * 0.0032f;
  g_mouse_dx = g_mouse_dy = 0;
  pitch = CLAMP(pitch, -1.2f, 1.2f);
}

void kx_game_frame(void) {
  input_update();
  g_want_mouse_capture = 1;
  yaw -= in.mdx * 0.0032f; pitch = CLAMP(pitch - in.mdy * 0.0032f, -1.2f, 1.2f);
  float fx = sinf(yaw), fz = cosf(yaw), sp = in.held[BTN_RUN] ? 0.09f : 0.05f;
  if (in.held[BTN_UP]) { px += fx * sp; pz += fz * sp; }
  if (in.held[BTN_DOWN]) { px -= fx * sp; pz -= fz * sp; }
  if (in.held[BTN_SL]) { px += fz * sp; pz -= fx * sp; }
  if (in.held[BTN_SR]) { px -= fz * sp; pz += fx * sp; }
  if (in.pressed[BTN_A]) sfx_play(sfx_find("ding"));
  if (in.pressed[BTN_B]) g_quit = 1;
  frame++;
}

void kx_game_draw(void) {
  if (!g_render) return;
  REnv e;
  memset(&e, 0, sizeof e);
  float top[3] = {0.012f, 0.016f, 0.035f}, hor[3] = {0.11f, 0.08f, 0.12f}, glow[3] = {0.22f, 0.11f, 0.05f};
  memcpy(e.skyTop, top, 12); memcpy(e.skyHorizon, hor, 12); memcpy(e.skyGlow, glow, 12);
  e.fogCol[0] = 0.075f; e.fogCol[1] = 0.062f; e.fogCol[2] = 0.085f; e.fogDensity = 0.05f;
  e.ambSky[0] = 0.12f; e.ambSky[1] = 0.125f; e.ambSky[2] = 0.17f;
  e.ambGround[0] = 0.06f; e.ambGround[1] = 0.05f; e.ambGround[2] = 0.045f;
  e.sunDir[0] = -0.4f; e.sunDir[1] = 0.55f; e.sunDir[2] = 0.6f;
  e.sunCol[0] = 0.1f; e.sunCol[1] = 0.12f; e.sunCol[2] = 0.2f;
  e.wet = 1; e.rain = 1; e.exposure = 1; e.moon = 0.3f; e.stars = 0.2f;
  RCam cam = {v3(px, 0.9f, pz), yaw, pitch, 75};
  r_frame_begin(&cam, &e);
  float t = frame / 60.0f;
  for (int i = 0; i < 4; i++) {
    float r = i & 1 ? 2.4f : 0.6f, g = 1.4f, b = i & 1 ? 0.6f : 2.6f;
    r_light(-2.5f + i * 1.7f, 2.3f, 2.5f, r, g, b, 6);
  }
  r_light(sinf(t) * 4, 1.2f, 6 + cosf(t) * 3, 2.5f, 0.4f, 0.3f, 5);
  r_lights_commit();
  r_draw(&scene, NULL, 0);
  for (int i = 0; i < 4; i++) r_glow(-2.5f + i * 1.7f, 2.3f, 2.5f, 0.5f, i & 1 ? 1.0f : 0.3f, 0.6f, i & 1 ? 0.3f : 1.0f);
  r_glow(sinf(t) * 4, 1.2f, 6 + cosf(t) * 3, 0.6f, 1.0f, 0.2f, 0.15f);
  r_glow_flush();
  r_rain(0.6f);
  r_grade(1.0f, 1.0f, 1.0f, 0);
  r_frame_end();
  d2_begin();
  d2_rrect(28, 24, 470, 96, 12, 0xC8121016);
  d2_text(FONT_SERIF, 34, 46, 34, "KroniX Engine " KX_VERSION, 0xFFE8B84A);
  d2_text(FONT_SANS, 18, 46, 82, "WASD + mysz — ruch, E — dźwięk, Esc — wyjście", 0xFFF0E6D0);
  d2_end();
}
