/* Modele rekwizytów (low-poly, fazowane krawędzie). Jednostka = 1 kafel ≈ 1,9 m; człowiek ≈ 0,93. */
#include "assets.h"
#include <stdlib.h>
#include <string.h>

GMesh MODELS[MD_COUNT];
MB MODEL_MB[MD_COUNT];

#define PNT(c, l) mb_paint((c), (l))
static MB *M_;
static void box(float x0, float y0, float z0, float x1, float y1, float z1) { mb_box(M_, v3(x0, y0, z0), v3(x1, y1, z1)); }
static void rbx(float x0, float y0, float z0, float x1, float y1, float z1, float e) { mb_rbox(M_, v3(x0, y0, z0), v3(x1, y1, z1), e); }
static void cyl(float x, float y, float z, float r0, float r1, float h, int seg, int caps) { mb_cyl(M_, v3(x, y, z), r0, r1, h, seg, caps); }
static void sph(float x, float y, float z, float rx, float ry, float rz, int seg) { mb_sphere(M_, v3(x, y, z), rx, ry, rz, seg); }
/* czworokąt dwustronny (kraty, liście) */
static void quad2(V3 a, V3 b, V3 c, V3 d, float u1, float v1) {
  mb_quad(M_, a, b, c, d, 0, 0, u1, v1);
  mb_quad(M_, b, a, d, c, 0, 0, u1, v1);
}

static void m_lamp(void) {
  PNT(0xFF2A2C30, L_METAL);
  cyl(0, 0, 0, 0.09f, 0.07f, 0.12f, 10, 3);
  cyl(0, 0.12f, 0, 0.04f, 0.03f, 1.8f, 8, 0);
  cyl(0, 1.92f, 0, 0.05f, 0.05f, 0.04f, 8, 3);
  /* latarnia */
  box(-0.08f, 1.96f, -0.08f, 0.08f, 1.98f, 0.08f);
  PNT(0xFFFFE2A8, L_WHITE); P_.emis = 1.0f;
  cyl(0, 1.98f, 0, 0.06f, 0.08f, 0.2f, 6, 0);
  PNT(0xFF2A2C30, L_METAL);
  cyl(0, 2.18f, 0, 0.1f, 0.0f, 0.1f, 6, 1);
  sph(0, 2.29f, 0, 0.02f, 0.02f, 0.02f, 6);
}
static void m_hydrant(void) {
  PNT(0xFFA8281E, L_METAL);
  cyl(0, 0, 0, 0.08f, 0.08f, 0.03f, 10, 3);
  cyl(0, 0.03f, 0, 0.055f, 0.05f, 0.24f, 10, 0);
  sph(0, 0.27f, 0, 0.055f, 0.045f, 0.055f, 10);
  mb_cyl_x(M_, v3(0, 0.18f, 0), 0.025f, 0.16f, 8, 3);
  mb_cyl_z(M_, v3(0, 0.18f, 0), 0.03f, 0.13f, 8, 3);
  PNT(0xFFC8A040, L_BRASS);
  cyl(0, 0.31f, 0, 0.012f, 0.012f, 0.03f, 6, 3);
}
static void m_trash(void) {
  PNT(0xFF6A6E74, L_CORRUGATED); P_.uvs = 0.6f;
  cyl(0, 0, 0, 0.12f, 0.13f, 0.32f, 12, 1);
  PNT(0xFF55595E, L_METAL);
  cyl(0, 0.32f, 0, 0.14f, 0.13f, 0.03f, 12, 3);
  sph(0, 0.36f, 0, 0.025f, 0.015f, 0.025f, 6);
}
static void m_bench(void) {
  PNT(0xFF8A5A30, L_WOOD_LIGHT);
  for (int i = 0; i < 3; i++) rbx(-0.5f, 0.22f, -0.12f + i * 0.08f, 0.5f, 0.245f, -0.06f + i * 0.08f, 0.008f);
  for (int i = 0; i < 2; i++) rbx(-0.5f, 0.3f + i * 0.09f, 0.11f, 0.5f, 0.36f + i * 0.09f, 0.13f, 0.008f);
  PNT(0xFF22252A, L_METAL);
  for (int s = -1; s <= 1; s += 2) {
    box(s * 0.42f - 0.02f, 0, -0.12f, s * 0.42f + 0.02f, 0.22f, -0.1f);
    box(s * 0.42f - 0.02f, 0, 0.1f, s * 0.42f + 0.02f, 0.5f, 0.12f);
  }
}
static void m_newsstand(void) {
  PNT(0xFF2E4A36, L_WHITE);
  rbx(-0.3f, 0, -0.2f, 0.3f, 0.5f, 0.2f, 0.02f);
  rbx(-0.33f, 0.5f, -0.24f, 0.33f, 0.54f, 0.24f, 0.01f);
  PNT(0xFFFFFFFF, L_NEWSPAPER);
  mb_quad(M_, v3(-0.26f, 0.32f, -0.21f), v3(0.26f, 0.32f, -0.21f), v3(0.26f, 0.48f, -0.17f), v3(-0.26f, 0.48f, -0.17f), 0, 0, 1, 1);
}
static void m_table(void) {
  PNT(0xFF4A2E1A, L_WOOD);
  for (int i = 0; i < 4; i++) {
    float x = (i & 1) ? 0.2f : -0.2f, z = (i & 2) ? 0.2f : -0.2f;
    box(x - 0.02f, 0, z - 0.02f, x + 0.02f, 0.4f, z + 0.02f);
  }
  PNT(0xFFFFFFFF, L_TABLECLOTH); P_.uvs = 1.2f;
  rbx(-0.3f, 0.4f, -0.3f, 0.3f, 0.43f, 0.3f, 0.01f);
  box(-0.31f, 0.3f, -0.31f, 0.31f, 0.4f, 0.31f);
  /* butelka wina i kieliszki */
  PNT(0xFF1E3A1E, L_WHITE);
  cyl(0.08f, 0.43f, 0.02f, 0.03f, 0.03f, 0.12f, 8, 1);
  cyl(0.08f, 0.55f, 0.02f, 0.03f, 0.01f, 0.03f, 8, 0);
  cyl(0.08f, 0.58f, 0.02f, 0.01f, 0.01f, 0.04f, 6, 2);
  PNT(0xFFF0E8D8, L_WHITE); P_.emis = 0.6f;
  cyl(-0.1f, 0.43f, -0.06f, 0.012f, 0.012f, 0.07f, 6, 2);
  PNT(0xFFB8C8D0, L_WHITE);
  cyl(-0.12f, 0.43f, 0.1f, 0.025f, 0.035f, 0.07f, 8, 1);
}
static void m_chair(void) {
  PNT(0xFF5A3820, L_WOOD);
  for (int i = 0; i < 4; i++) {
    float x = (i & 1) ? 0.11f : -0.11f, z = (i & 2) ? 0.11f : -0.11f;
    box(x - 0.015f, 0, z - 0.015f, x + 0.015f, 0.23f, z + 0.015f);
  }
  rbx(-0.13f, 0.23f, -0.13f, 0.13f, 0.26f, 0.13f, 0.01f);
  box(-0.12f, 0.26f, 0.105f, -0.09f, 0.52f, 0.13f);
  box(0.09f, 0.26f, 0.105f, 0.12f, 0.52f, 0.13f);
  rbx(-0.12f, 0.42f, 0.1f, 0.12f, 0.5f, 0.13f, 0.01f);
}
static void m_bar(void) {
  PNT(0xFF4A2E1A, L_WAINSCOT); P_.uvs = 1.5f;
  box(-0.5f, 0, -0.25f, 0.5f, 0.56f, 0.2f);
  PNT(0xFF2A1A10, L_WOOD);
  rbx(-0.5f, 0.56f, -0.3f, 0.5f, 0.6f, 0.25f, 0.012f);
  PNT(0xFFD8B460, L_BRASS);
  mb_cyl_x(M_, v3(0, 0.08f, -0.33f), 0.015f, 1.0f, 8, 0);
}
static void m_stool(void) {
  PNT(0xFFB8BCC2, L_METAL);
  cyl(0, 0, 0, 0.1f, 0.08f, 0.02f, 10, 3);
  cyl(0, 0.02f, 0, 0.02f, 0.02f, 0.3f, 8, 0);
  mb_cyl(M_, v3(0, 0.12f, 0), 0.08f, 0.08f, 0.012f, 12, 3);
  PNT(0xFF7A1A1A, L_LEATHER);
  cyl(0, 0.32f, 0, 0.11f, 0.1f, 0.05f, 12, 3);
}
static void m_shelf(void) {
  PNT(0xFF3A2414, L_WOOD);
  box(-0.5f, 0, -0.15f, 0.5f, 0.08f, 0.15f);
  box(-0.5f, 1.32f, -0.15f, 0.5f, 1.4f, 0.15f);
  box(-0.5f, 0, -0.15f, -0.46f, 1.4f, 0.15f);
  box(0.46f, 0, -0.15f, 0.5f, 1.4f, 0.15f);
  PNT(0xFFFFFFFF, L_BOTTLES);
  mb_quad(M_, v3(0.46f, 0.08f, -0.05f), v3(-0.46f, 0.08f, -0.05f), v3(-0.46f, 1.32f, -0.05f), v3(0.46f, 1.32f, -0.05f), 0, 0, 1, 1);
}
static void m_piano(void) {
  PNT(0xFF181418, L_WOOD);
  rbx(-0.42f, 0, -0.14f, 0.42f, 0.62f, 0.14f, 0.015f);
  rbx(-0.44f, 0.62f, -0.16f, 0.44f, 0.66f, 0.16f, 0.01f);
  box(-0.42f, 0.3f, -0.26f, 0.42f, 0.34f, -0.14f);
  PNT(0xFFF0EAD8, L_WHITE);
  box(-0.4f, 0.34f, -0.25f, 0.4f, 0.355f, -0.16f);
  PNT(0xFF101010, L_WHITE);
  for (int k = 0; k < 24; k++) if (k % 7 != 2 && k % 7 != 6) box(-0.39f + k * 0.033f, 0.355f, -0.21f, -0.375f + k * 0.033f, 0.37f, -0.16f);
  PNT(0xFFC8A040, L_BRASS); P_.emis = 0.5f;
  cyl(-0.3f, 0.66f, 0, 0.03f, 0.04f, 0.02f, 8, 3);
  cyl(-0.3f, 0.68f, 0, 0.006f, 0.006f, 0.08f, 6, 0);
}
static void m_desk(void) {
  PNT(0xFF3E2414, L_WOOD);
  rbx(-0.55f, 0.4f, -0.3f, 0.55f, 0.45f, 0.3f, 0.015f);
  rbx(-0.53f, 0, -0.28f, -0.25f, 0.4f, 0.28f, 0.01f);
  rbx(0.25f, 0, -0.28f, 0.53f, 0.4f, 0.28f, 0.01f);
  PNT(0xFF2A4A2E, L_LEATHER);
  box(-0.3f, 0.45f, -0.18f, 0.3f, 0.452f, 0.12f);
  /* lampa bankierska */
  PNT(0xFFC8A040, L_BRASS);
  cyl(0.38f, 0.45f, 0.08f, 0.05f, 0.04f, 0.02f, 10, 3);
  cyl(0.38f, 0.47f, 0.08f, 0.008f, 0.008f, 0.12f, 6, 0);
  PNT(0xFF2A8A4A, L_WHITE); P_.emis = 0.6f;
  mb_cyl_x(M_, v3(0.38f, 0.6f, 0.06f), 0.04f, 0.18f, 10, 3);
  /* papiery i telefon */
  PNT(0xFFFFFFFF, L_NEWSPAPER);
  box(-0.2f, 0.452f, -0.1f, 0.0f, 0.456f, 0.05f);
  PNT(0xFF141414, L_METAL);
  cyl(-0.38f, 0.45f, 0.05f, 0.05f, 0.04f, 0.04f, 10, 3);
  cyl(-0.38f, 0.49f, 0.05f, 0.012f, 0.012f, 0.1f, 6, 0);
  mb_cyl_x(M_, v3(-0.38f, 0.6f, 0.05f), 0.015f, 0.14f, 6, 3);
}
static void m_bookcase(void) {
  PNT(0xFF3A2414, L_WOOD);
  box(-0.5f, 0, -0.14f, 0.5f, 0.05f, 0.14f);
  box(-0.5f, 1.35f, -0.16f, 0.5f, 1.42f, 0.16f);
  box(-0.5f, 0, -0.14f, -0.46f, 1.4f, 0.14f);
  box(0.46f, 0, -0.14f, 0.5f, 1.4f, 0.14f);
  box(-0.46f, 0, 0.1f, 0.46f, 1.4f, 0.14f);
  PNT(0xFFFFFFFF, L_BOOKS);
  mb_quad(M_, v3(0.46f, 0.05f, -0.06f), v3(-0.46f, 0.05f, -0.06f), v3(-0.46f, 1.35f, -0.06f), v3(0.46f, 1.35f, -0.06f), 0, 0, 1, 1);
}
static void m_fireplace(void) {
  PNT(0xFFFFFFFF, L_STONE); P_.uvs = 1.5f;
  box(-0.5f, 0, -0.1f, -0.3f, 0.75f, 0.2f);
  box(0.3f, 0, -0.1f, 0.5f, 0.75f, 0.2f);
  box(-0.5f, 0.55f, -0.1f, 0.5f, 0.75f, 0.2f);
  PNT(0xFF3A2414, L_WOOD);
  rbx(-0.58f, 0.75f, -0.16f, 0.58f, 0.8f, 0.22f, 0.01f);
  PNT(0xFF0E0C0C, L_WHITE);
  box(-0.3f, 0, 0.12f, 0.3f, 0.55f, 0.2f);
  box(-0.3f, 0, -0.1f, 0.3f, 0.02f, 0.2f);
  PNT(0xFFFFFFFF, L_FIRE); P_.flags = VF_FIRE;
  quad2(v3(-0.22f, 0.02f, 0.05f), v3(0.22f, 0.02f, 0.05f), v3(0.22f, 0.36f, 0.05f), v3(-0.22f, 0.36f, 0.05f), 1, 1);
  PNT(0xFF2A1A10, L_WOOD);
  mb_cyl_x(M_, v3(0, 0.05f, 0.06f), 0.04f, 0.4f, 6, 3);
  /* obraz nad kominkiem */
  PNT(0xFFFFFFFF, L_PAINTING);
  mb_quad(M_, v3(0.35f, 0.95f, -0.11f), v3(-0.35f, 0.95f, -0.11f), v3(-0.35f, 1.4f, -0.11f), v3(0.35f, 1.4f, -0.11f), 0, 0, 1, 1);
}
static void m_crate(void) {
  PNT(0xFFFFFFFF, L_CRATE); P_.uvs = 1.0f;
  float s = 0.27f;
  /* każda ściana z całą teksturą */
  V3 c[8] = {v3(-s, 0, -s), v3(s, 0, -s), v3(s, 2 * s, -s), v3(-s, 2 * s, -s), v3(-s, 0, s), v3(s, 0, s), v3(s, 2 * s, s), v3(-s, 2 * s, s)};
  mb_quad(M_, c[1], c[0], c[3], c[2], 0, 0, 1, 1);
  mb_quad(M_, c[4], c[5], c[6], c[7], 0, 0, 1, 1);
  mb_quad(M_, c[0], c[4], c[7], c[3], 0, 0, 1, 1);
  mb_quad(M_, c[5], c[1], c[2], c[6], 0, 0, 1, 1);
  mb_quad(M_, c[3], c[7], c[6], c[2], 0, 0, 1, 1);
}
static void m_crates(void) {
  MB *save = M_;
  MB t; mb_init(&t);
  M_ = &t; m_crate(); M_ = save;
  mb_append_tf(M_, &t, m4_translate(-0.14f, 0, 0.02f));
  mb_append_tf(M_, &t, m4_mul(m4_translate(0.2f, 0, -0.05f), m4_roty(0.3f)));
  mb_append_tf(M_, &t, m4_mul(m4_translate(0.0f, 0.54f, 0.0f), m4_mul(m4_roty(-0.2f), m4_scale(0.85f, 0.85f, 0.85f))));
  mb_free(&t);
}
static void m_barrel(void) {
  PNT(0xFFFFFFFF, L_BARREL); P_.uvs = 2.0f;
  cyl(0, 0, 0, 0.17f, 0.2f, 0.27f, 16, 1);
  cyl(0, 0.27f, 0, 0.2f, 0.17f, 0.27f, 16, 2);
}
static void m_vat(void) {
  PNT(0xFFB06A3A, L_BRASS);
  cyl(0, 0, 0, 0.46f, 0.46f, 1.15f, 20, 0);
  cyl(0, 1.15f, 0, 0.46f, 0.12f, 0.2f, 20, 0);
  cyl(0, 1.35f, 0, 0.1f, 0.1f, 0.12f, 10, 2);
  PNT(0xFF3A3C40, L_METAL);
  for (int k = 0; k < 3; k++) cyl(0, 0.15f + k * 0.42f, 0, 0.475f, 0.475f, 0.04f, 20, 0);
  mb_cyl_x(M_, v3(0.6f, 0.3f, 0), 0.05f, 0.4f, 8, 3);
  for (int i = 0; i < 4; i++) {
    float a = i * 1.5708f + 0.78f;
    box(cosf(a) * 0.42f - 0.03f, -0.0f, sinf(a) * 0.42f - 0.03f, cosf(a) * 0.42f + 0.03f, 0.08f, sinf(a) * 0.42f + 0.03f);
  }
}
static void m_pew(void) {
  PNT(0xFF4A2E1A, L_WOOD);
  rbx(-0.5f, 0.2f, -0.14f, 0.5f, 0.24f, 0.12f, 0.01f);
  rbx(-0.5f, 0.24f, 0.1f, 0.5f, 0.52f, 0.14f, 0.01f);
  rbx(-0.52f, 0, -0.15f, -0.46f, 0.55f, 0.15f, 0.01f);
  rbx(0.46f, 0, -0.15f, 0.52f, 0.55f, 0.15f, 0.01f);
}
static void m_altar(void) {
  PNT(0xFFFFFFFF, L_MARBLE);
  box(-0.5f, 0, -0.25f, 0.5f, 0.5f, 0.25f);
  PNT(0xFFF2EEE6, L_CLOTH);
  box(-0.52f, 0.5f, -0.27f, 0.52f, 0.52f, 0.27f);
  box(-0.52f, 0.3f, -0.272f, 0.52f, 0.5f, -0.27f);
  PNT(0xFFC8A040, L_BRASS);
  box(-0.015f, 0.52f, 0, 0.015f, 0.85f, 0.03f);
  box(-0.08f, 0.74f, 0, 0.08f, 0.77f, 0.03f);
  for (int s = -1; s <= 1; s += 2) {
    cyl(s * 0.32f, 0.52f, 0.05f, 0.03f, 0.02f, 0.12f, 8, 3);
    PNT(0xFFF0E8D8, L_WHITE); P_.emis = 0.4f;
    cyl(s * 0.32f, 0.64f, 0.05f, 0.012f, 0.012f, 0.09f, 6, 2);
    PNT(0xFFC8A040, L_BRASS);
  }
}
static void m_bed(void) {
  PNT(0xFF3E2414, L_WOOD);
  box(-0.32f, 0, -0.55f, 0.32f, 0.18f, 0.55f);
  box(-0.34f, 0, -0.58f, 0.34f, 0.45f, -0.53f);
  PNT(0xFFE8E2D4, L_CLOTH);
  rbx(-0.3f, 0.18f, -0.52f, 0.3f, 0.26f, 0.52f, 0.03f);
  rbx(-0.22f, 0.26f, -0.5f, 0.22f, 0.31f, -0.32f, 0.03f);
  PNT(0xFF6A2A2A, L_CLOTH);
  rbx(-0.32f, 0.2f, -0.2f, 0.32f, 0.28f, 0.54f, 0.02f);
}
static void m_fence(void) {
  PNT(0xFFFFFFFF, L_FENCE); P_.flags = VF_ALPHATEST;
  quad2(v3(-0.5f, 0, 0), v3(0.5f, 0, 0), v3(0.5f, 0.6f, 0), v3(-0.5f, 0.6f, 0), 1, 1);
}
static void m_bars(void) {
  PNT(0xFF3A3C42, L_METAL);
  for (int k = 0; k < 8; k++) cyl(-0.44f + k * 0.125f, 0, 0, 0.018f, 0.018f, 1.6f, 6, 0);
  box(-0.5f, 0.05f, -0.02f, 0.5f, 0.1f, 0.02f);
  box(-0.5f, 1.45f, -0.02f, 0.5f, 1.5f, 0.02f);
}
static void m_machine(void) {
  PNT(0xFF3A4A3E, L_METAL);
  rbx(-0.4f, 0, -0.3f, 0.4f, 0.7f, 0.3f, 0.03f);
  PNT(0xFF55595E, L_METAL);
  mb_cyl_x(M_, v3(0, 0.8f, 0), 0.12f, 0.7f, 12, 3);
  cyl(0.3f, 0.7f, 0.15f, 0.04f, 0.04f, 0.5f, 8, 2);
  PNT(0xFFE8E2D4, L_WHITE);
  mb_cyl_z(M_, v3(-0.2f, 0.5f, -0.31f), 0.06f, 0.02f, 12, 3);
  PNT(0xFFA02A1E, L_WHITE); P_.emis = 0.8f;
  sph(0.2f, 0.55f, -0.31f, 0.025f, 0.025f, 0.025f, 6);
}
static void m_tree(void) {
  PNT(0xFF4A3424, L_WOOD); P_.uvs = 3;
  cyl(0, 0, 0, 0.09f, 0.06f, 1.2f, 8, 0);
  cyl(0, 0.9f, 0, 0.04f, 0.02f, 0.5f, 6, 0);
  PNT(0xFF4E7A34, L_GRASS); P_.flags = VF_FOLIAGE; P_.uvs = 2;
  sph(0, 1.55f, 0, 0.6f, 0.45f, 0.6f, 9);
  sph(0.3f, 1.35f, 0.2f, 0.4f, 0.32f, 0.4f, 8);
  sph(-0.28f, 1.4f, -0.15f, 0.42f, 0.32f, 0.42f, 8);
  sph(0.05f, 1.9f, 0.1f, 0.38f, 0.3f, 0.38f, 8);
  PNT(0xFF3A3632, L_STONE);
  box(-0.3f, 0, -0.3f, 0.3f, 0.04f, 0.3f);
}
static void m_plant(void) {
  PNT(0xFF8A4A2A, L_WHITE);
  cyl(0, 0, 0, 0.1f, 0.13f, 0.2f, 10, 1);
  PNT(0xFF3E6A2A, L_GRASS); P_.flags = VF_FOLIAGE;
  for (int k = 0; k < 7; k++) {
    float a = k * 0.9f;
    sph(cosf(a) * 0.08f, 0.32f + (k % 3) * 0.06f, sinf(a) * 0.08f, 0.1f, 0.14f, 0.1f, 6);
  }
}
/* auto z lat 30. — elementy w kolorze 0xFFFFFF są przemalowywane (tint) */
static void m_car(void) {
  float L = 1.05f, Wd = 0.42f;
  PNT(0xFFFFFFFF, L_METAL);
  rbx(-L, 0.16f, -Wd, L * 0.92f, 0.46f, Wd, 0.06f);
  rbx(0.25f, 0.42f, -Wd * 0.62f, L * 0.92f, 0.54f, Wd * 0.62f, 0.05f);          /* maska */
  rbx(-0.75f, 0.44f, -Wd * 0.95f, 0.28f, 0.82f, Wd * 0.95f, 0.07f);             /* kabina */
  /* błotniki */
  for (int i = 0; i < 4; i++) {
    float x = (i & 1) ? 0.62f : -0.62f, z = (i & 2) ? Wd + 0.02f : -Wd - 0.02f;
    mb_cyl_z(M_, v3(x, 0.26f, z), 0.22f, 0.16f, 12, 3);
  }
  PNT(0xFF1E1C1A, L_METAL);
  box(-0.45f, 0.12f, -Wd - 0.12f, 0.45f, 0.15f, Wd + 0.12f);                       /* stopnie */
  /* szyby */
  PNT(0xFF1A2430, L_WINDOW);
  mb_quad(M_, v3(0.29f, 0.5f, -Wd * 0.85f), v3(0.29f, 0.5f, Wd * 0.85f), v3(0.29f, 0.78f, Wd * 0.85f), v3(0.29f, 0.78f, -Wd * 0.85f), 0, 0, 1, 1);
  mb_quad(M_, v3(-0.7f, 0.52f, -Wd * 0.96f), v3(0.24f, 0.52f, -Wd * 0.96f), v3(0.24f, 0.78f, -Wd * 0.96f), v3(-0.7f, 0.78f, -Wd * 0.96f), 0, 0, 2, 1);
  mb_quad(M_, v3(0.24f, 0.52f, Wd * 0.96f), v3(-0.7f, 0.52f, Wd * 0.96f), v3(-0.7f, 0.78f, Wd * 0.96f), v3(0.24f, 0.78f, Wd * 0.96f), 0, 0, 2, 1);
  /* koła */
  for (int i = 0; i < 4; i++) {
    float x = (i & 1) ? 0.62f : -0.62f, z = (i & 2) ? Wd - 0.02f : -Wd + 0.02f;
    PNT(0xFF141414, L_WHITE);
    mb_cyl_z(M_, v3(x, 0.17f, z), 0.17f, 0.11f, 14, 3);
    PNT(0xFFE8E4DA, L_WHITE);
    mb_cyl_z(M_, v3(x, 0.17f, z + ((i & 2) ? 0.056f : -0.056f)), 0.1f, 0.004f, 14, 3);
    PNT(0xFFB8BCC2, L_METAL);
    mb_cyl_z(M_, v3(x, 0.17f, z + ((i & 2) ? 0.06f : -0.06f)), 0.05f, 0.006f, 10, 3);
  }
  /* grill, zderzaki, reflektory */
  PNT(0xFFC8CCD2, L_METAL);
  rbx(L * 0.9f, 0.2f, -0.2f, L * 0.95f, 0.5f, 0.2f, 0.02f);
  box(L * 0.95f, 0.14f, -Wd - 0.05f, L + 0.04f, 0.19f, Wd + 0.05f);
  box(-L - 0.06f, 0.14f, -Wd - 0.05f, -L - 0.01f, 0.19f, Wd + 0.05f);
  for (int s = -1; s <= 1; s += 2) {
    PNT(0xFFC8CCD2, L_METAL);
    mb_cyl_x(M_, v3(L * 0.95f, 0.5f, s * 0.3f), 0.07f, 0.06f, 10, 3);
    PNT(0xFFE8E0C0, L_WHITE); P_.emis = 0.05f;
    mb_cyl_x(M_, v3(L * 0.985f, 0.5f, s * 0.3f), 0.055f, 0.01f, 10, 3);
    PNT(0xFF8A1A1A, L_WHITE); P_.emis = 0.05f;
    box(-L - 0.02f, 0.4f, s * 0.32f - 0.03f, -L, 0.45f, s * 0.32f + 0.03f);
  }
  /* koło zapasowe */
  PNT(0xFF141414, L_WHITE);
  mb_cyl_x(M_, v3(-L - 0.06f, 0.42f, 0), 0.16f, 0.1f, 14, 3);
}
static void m_truck(void) {
  PNT(0xFFFFFFFF, L_METAL);
  rbx(0.45f, 0.18f, -0.45f, 1.25f, 0.55f, 0.45f, 0.05f);
  rbx(0.45f, 0.5f, -0.44f, 0.85f, 0.95f, 0.44f, 0.05f);
  PNT(0xFF1A2430, L_WINDOW);
  mb_quad(M_, v3(0.86f, 0.62f, -0.36f), v3(0.86f, 0.62f, 0.36f), v3(0.86f, 0.9f, 0.36f), v3(0.86f, 0.9f, -0.36f), 0, 0, 1, 1);
  PNT(0xFF5A3A22, L_WOOD_LIGHT);
  box(-1.25f, 0.3f, -0.5f, 0.42f, 0.36f, 0.5f);
  for (int s = -1; s <= 1; s += 2) box(-1.25f, 0.36f, s * 0.5f - 0.02f, 0.42f, 0.6f, s * 0.5f + 0.02f);
  box(-1.27f, 0.36f, -0.5f, -1.23f, 0.6f, 0.5f);
  for (int i = 0; i < 6; i++) {
    float x = i < 2 ? 1.0f : i < 4 ? -0.4f : -0.95f, z = (i & 1) ? 0.42f : -0.42f;
    PNT(0xFF141414, L_WHITE);
    mb_cyl_z(M_, v3(x, 0.17f, z), 0.17f, 0.12f, 14, 3);
  }
  PNT(0xFFFFFFFF, L_BARREL);
  for (int k = 0; k < 3; k++) cyl(-1.0f + k * 0.45f, 0.36f, 0.18f, 0.16f, 0.17f, 0.4f, 12, 2);
  PNT(0xFFFFFFFF, L_CRATE);
  for (int k = 0; k < 3; k++) box(-1.15f + k * 0.45f, 0.36f, -0.42f, -0.8f + k * 0.45f, 0.7f, -0.05f);
}
static void m_chandelier(void) {
  PNT(0xFFC8A040, L_BRASS);
  cyl(0, -0.25f, 0, 0.008f, 0.008f, 0.25f, 6, 0);
  sph(0, -0.3f, 0, 0.06f, 0.05f, 0.06f, 8);
  for (int k = 0; k < 6; k++) {
    float a = k * 1.0472f;
    float x = cosf(a) * 0.2f, z = sinf(a) * 0.2f;
    PNT(0xFFC8A040, L_BRASS);
    mb_cyl_x(M_, v3(x * 0.5f, -0.32f, z * 0.5f), 0.01f, 0.01f, 4, 0);
    box(x * 0.2f - 0.005f, -0.33f, z * 0.2f - 0.005f, x + 0.005f, -0.32f, z + 0.005f);
    PNT(0xFFFFE8B0, L_WHITE); P_.emis = 1.0f;
    sph(x, -0.28f, z, 0.03f, 0.045f, 0.03f, 6);
  }
}
static void m_candle(void) {
  PNT(0xFFF0E8D8, L_WHITE);
  cyl(0, 0, 0, 0.02f, 0.02f, 0.12f, 8, 2);
  PNT(0xFFFFC870, L_WHITE); P_.emis = 1;
  sph(0, 0.14f, 0, 0.01f, 0.022f, 0.01f, 6);
}
static void m_mailbox(void) {
  PNT(0xFF1E3A6A, L_METAL);
  for (int s = -1; s <= 1; s += 2) box(s * 0.1f - 0.015f, 0, -0.1f, s * 0.1f + 0.015f, 0.18f, 0.1f);
  rbx(-0.14f, 0.18f, -0.13f, 0.14f, 0.48f, 0.13f, 0.02f);
  mb_cyl_x(M_, v3(0, 0.48f, 0), 0.13f, 0.28f, 12, 3);
  PNT(0xFFE8E2D4, L_WHITE);
  box(-0.08f, 0.32f, -0.135f, 0.08f, 0.36f, -0.13f);
}
static void m_phone(void) {
  PNT(0xFF3A2414, L_WOOD);
  rbx(-0.25f, 0, -0.25f, 0.25f, 1.2f, 0.25f, 0.03f);
  PNT(0xFF2A3040, L_WINDOW);
  mb_quad(M_, v3(-0.2f, 0.4f, -0.255f), v3(0.2f, 0.4f, -0.255f), v3(0.2f, 1.05f, -0.255f), v3(-0.2f, 1.05f, -0.255f), 0, 0, 1, 1);
  PNT(0xFFE8E2D4, L_WHITE); P_.emis = 0.8f;
  box(-0.2f, 1.08f, -0.26f, 0.2f, 1.16f, -0.25f);
}
static void m_radio(void) {
  PNT(0xFF5A3A1E, L_WOOD_LIGHT);
  rbx(-0.12f, 0, -0.07f, 0.12f, 0.22f, 0.07f, 0.02f);
  sph(0, 0.22f, 0, 0.12f, 0.08f, 0.07f, 10);
  PNT(0xFFD8B460, L_CLOTH); P_.emis = 0.2f;
  mb_quad(M_, v3(0.08f, 0.06f, -0.075f), v3(-0.08f, 0.06f, -0.075f), v3(-0.08f, 0.22f, -0.075f), v3(0.08f, 0.22f, -0.075f), 0, 0, 1, 1);
}
static void m_gramophone(void) {
  PNT(0xFF3A2414, L_WOOD);
  rbx(-0.14f, 0, -0.14f, 0.14f, 0.1f, 0.14f, 0.01f);
  PNT(0xFF141414, L_WHITE);
  cyl(0, 0.1f, 0, 0.11f, 0.11f, 0.01f, 16, 3);
  PNT(0xFFC8A040, L_BRASS);
  cyl(0.08f, 0.1f, 0.08f, 0.012f, 0.012f, 0.15f, 6, 0);
  cyl(0.08f, 0.25f, 0.08f, 0.02f, 0.16f, 0.18f, 12, 0);
}
static void m_counter(void) {
  PNT(0xFF5A3A22, L_WOOD_LIGHT);
  box(-0.5f, 0, -0.2f, 0.5f, 0.52f, 0.2f);
  PNT(0xFF2A1A10, L_WOOD);
  rbx(-0.52f, 0.52f, -0.22f, 0.52f, 0.56f, 0.22f, 0.01f);
  PNT(0xFFC8A040, L_BRASS);
  rbx(0.1f, 0.56f, -0.1f, 0.38f, 0.74f, 0.12f, 0.02f);
}
static void m_safe(void) {
  PNT(0xFF2A2E34, L_METAL);
  rbx(-0.25f, 0, -0.22f, 0.25f, 0.6f, 0.22f, 0.03f);
  PNT(0xFFC8A040, L_BRASS);
  mb_cyl_z(M_, v3(0, 0.35f, -0.23f), 0.06f, 0.02f, 14, 3);
  box(0.12f, 0.25f, -0.235f, 0.16f, 0.4f, -0.22f);
}
static void m_ring(void) {
  PNT(0xFF8A1A1A, L_METAL);
  cyl(0, 0, 0, 0.04f, 0.04f, 0.8f, 8, 2);
  PNT(0xFFE8E2D4, L_CLOTH);
  for (int k = 0; k < 3; k++) {
    float y = 0.35f + k * 0.17f;
    box(0, y, -0.015f, 1.0f, y + 0.03f, 0.015f);
    box(-0.015f, y, 0, 0.015f, y + 0.03f, 1.0f);
  }
}
static void m_tank(void) {
  PNT(0xFF4A3A2A, L_BARREL); P_.uvs = 1.0f;
  cyl(0, 0.6f, 0, 0.6f, 0.6f, 0.9f, 16, 0);
  PNT(0xFF2A2420, L_ROOF);
  cyl(0, 1.5f, 0, 0.65f, 0.0f, 0.4f, 16, 1);
  PNT(0xFF22252A, L_METAL);
  for (int i = 0; i < 4; i++) {
    float a = i * 1.5708f + 0.78f;
    box(cosf(a) * 0.45f - 0.03f, 0, sinf(a) * 0.45f - 0.03f, cosf(a) * 0.45f + 0.03f, 0.62f, sinf(a) * 0.45f + 0.03f);
  }
}
static void m_ceillamp(void) {
  PNT(0xFF22252A, L_METAL);
  cyl(0, -0.3f, 0, 0.006f, 0.006f, 0.3f, 4, 0);
  PNT(0xFF2A6A4A, L_METAL);
  cyl(0, -0.38f, 0, 0.16f, 0.03f, 0.08f, 12, 2);
  PNT(0xFFFFE8B0, L_WHITE); P_.emis = 1.0f;
  sph(0, -0.39f, 0, 0.05f, 0.03f, 0.05f, 8);
}
static void m_sofa(void) {
  PNT(0xFF4A2418, L_LEATHER);
  rbx(-0.55f, 0.08f, -0.25f, 0.55f, 0.26f, 0.22f, 0.05f);
  rbx(-0.55f, 0.08f, 0.12f, 0.55f, 0.5f, 0.26f, 0.05f);
  rbx(-0.62f, 0.08f, -0.25f, -0.48f, 0.4f, 0.26f, 0.05f);
  rbx(0.48f, 0.08f, -0.25f, 0.62f, 0.4f, 0.26f, 0.05f);
  PNT(0xFF2A1A10, L_WOOD);
  for (int i = 0; i < 4; i++) box((i & 1) ? 0.5f : -0.56f, 0, (i & 2) ? 0.18f : -0.22f, (i & 1) ? 0.56f : -0.5f, 0.08f, (i & 2) ? 0.24f : -0.16f);
}
static void m_boat(void) {
  PNT(0xFF5A3A22, L_WOOD_LIGHT);
  rbx(-0.9f, -0.1f, -0.3f, 0.9f, 0.18f, 0.3f, 0.12f);
  PNT(0xFF2A1A10, L_WOOD);
  box(-0.3f, 0.12f, -0.28f, -0.2f, 0.2f, 0.28f);
  box(0.3f, 0.12f, -0.28f, 0.4f, 0.2f, 0.28f);
}

static void m_ledger(void) {
  PNT(0xFF4A1E14, L_LEATHER);
  rbx(-0.09f, 0, -0.065f, 0.09f, 0.03f, 0.065f, 0.006f);
  PNT(0xFFE8DCC0, L_WHITE);
  box(-0.085f, 0.004f, -0.06f, 0.088f, 0.026f, 0.06f);
  PNT(0xFFD8B460, L_BRASS);
  box(-0.02f, 0.03f, -0.04f, 0.02f, 0.032f, 0.04f);
}
static void m_photo(void) {
  PNT(0xFFC8A040, L_BRASS);
  rbx(-0.06f, 0, -0.012f, 0.06f, 0.14f, 0.012f, 0.004f);
  PNT(0xFFB0A898, L_NEWSPAPER);
  mb_quad(M_, v3(-0.05f, 0.012f, 0.013f), v3(0.05f, 0.012f, 0.013f), v3(0.05f, 0.128f, 0.013f), v3(-0.05f, 0.128f, 0.013f), 0.3f, 0.3f, 0.5f, 0.6f);
  PNT(0xFF3A2414, L_WOOD);
  box(-0.01f, 0, -0.06f, 0.01f, 0.08f, -0.012f);
}
static void m_money(void) {
  PNT(0xFF6A8A5A, L_NEWSPAPER);
  for (int k = 0; k < 3; k++) rbx(-0.06f + k * 0.03f, k * 0.022f, -0.03f, 0.06f + k * 0.03f, 0.02f + k * 0.022f, 0.03f, 0.003f);
  PNT(0xFFE8DCC0, L_WHITE);
  for (int k = 0; k < 3; k++) box(-0.012f + k * 0.03f, k * 0.022f, -0.031f, 0.012f + k * 0.03f, 0.021f + k * 0.022f, 0.031f);
}
static void m_bottle(void) {
  PNT(0xFF3A5A2A, L_WHITE);
  cyl(0, 0, 0, 0.035f, 0.035f, 0.12f, 10, 1);
  cyl(0, 0.12f, 0, 0.035f, 0.012f, 0.04f, 10, 0);
  cyl(0, 0.16f, 0, 0.012f, 0.012f, 0.05f, 6, 2);
  PNT(0xFFE8DCC0, L_NEWSPAPER);
  cyl(0, 0.03f, 0, 0.036f, 0.036f, 0.05f, 10, 0);
}
static void m_briefcase(void) {
  PNT(0xFF3A1E12, L_LEATHER);
  rbx(-0.13f, 0, -0.04f, 0.13f, 0.18f, 0.04f, 0.015f);
  PNT(0xFFD8B460, L_BRASS);
  box(-0.04f, 0.18f, -0.008f, 0.04f, 0.2f, 0.008f);
}
static void m_sconce(void) {
  PNT(0xFFC8A040, L_BRASS);
  rbx(-0.03f, -0.06f, 0, 0.03f, 0.06f, 0.02f, 0.006f);
  box(-0.006f, -0.01f, 0.0f, 0.006f, 0.01f, 0.08f);
  PNT(0xFFFFE0A0, L_WHITE); P_.emis = 1.0f;
  cyl(0, 0.0f, 0.08f, 0.035f, 0.05f, 0.08f, 8, 3);
}

int objmodel_find(const char *n) {
  static const struct { const char *name; int md; } T[] = {
    {"ledger", MD_LEDGER}, {"papers", MD_PHOTO}, {"photo", MD_PHOTO}, {"money", MD_MONEY}, {"bottle", MD_BOTTLE},
    {"briefcase", MD_BRIEFCASE}, {"safe", MD_SAFE}, {"radio", MD_RADIO}, {"phone", MD_PHONE}, {"crate", MD_CRATE},
    {"gramophone", MD_GRAMOPHONE}, {"note", MD_LEDGER}};
  for (unsigned i = 0; i < sizeof T / sizeof T[0]; i++) if (!strcmp(T[i].name, n)) return T[i].md;
  return -1;
}

void models_build(void) {
  void (*fn[MD_COUNT])(void) = {
    m_lamp, m_hydrant, m_trash, m_bench, m_newsstand, m_table, m_chair, m_bar, m_stool, m_shelf,
    m_piano, m_desk, m_bookcase, m_fireplace, m_crate, m_crates, m_barrel, m_vat, m_pew, m_altar,
    m_bed, m_fence, m_bars, m_machine, m_tree, m_plant, m_car, m_truck, m_chandelier, m_candle,
    m_mailbox, m_phone, m_radio, m_gramophone, m_counter, m_safe, m_ring, m_tank, m_ceillamp,
    m_sofa, m_boat, m_ledger, m_photo, m_money, m_bottle, m_briefcase, m_sconce};
  for (int i = 0; i < MD_COUNT; i++) {
    mb_init(&MODEL_MB[i]);
    M_ = &MODEL_MB[i];
    mb_paint(0xFFFFFFFF, L_WHITE);
    fn[i]();
    MODELS[i] = gm_upload(&MODEL_MB[i]);
  }
  mb_paint(0xFFFFFFFF, L_WHITE);
}

/* wtapia model w siatkę mapy: obrót wokół Y, skala, przemalowanie białych elementów */
void model_add(MB *dst, int model, float x, float y, float z, float yaw, float scale, uint32_t tint) {
  if (model < 0 || model >= MD_COUNT) return;
  M4 tf = m4_mul(m4_translate(x, y, z), m4_mul(m4_roty(yaw), m4_scale(scale, scale, scale)));
  int start = dst->n;
  mb_append_tf(dst, &MODEL_MB[model], tf);
  if (tint && tint != 0xFFFFFFFFu) {
    uint8_t tr = (tint >> 16) & 255, tg = (tint >> 8) & 255, tb = tint & 255;
    for (int i = start; i < dst->n; i++) {
      Vert *v = &dst->v[i];
      if (v->c[0] == 255 && v->c[1] == 255 && v->c[2] == 255 && v->uv[2] == (float)L_METAL) { v->c[0] = tr; v->c[1] = tg; v->c[2] = tb; }
    }
  }
}
