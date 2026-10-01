/* Broń w widoku pierwszej osoby: dłonie w rękawach marynarki + Colt, Winchester, Thompson. */
#include "engine.h"

static GMesh WM[4];      /* broń + prawa dłoń */
static GMesh HANDL, FIST; /* lewa dłoń (podtrzymująca), pięść */
static GMesh FLASH;

static void sleeve_hand(MB *b, float x, float y, float z, float len) {
  mb_paint(0xFF4A423A, L_CLOTH);
  mb_rbox(b, v3(x - 0.035f, y - 0.035f, z), v3(x + 0.035f, y + 0.035f, z + len), 0.015f);
  mb_paint(0xFFE8E4DA, L_CLOTH);
  mb_rbox(b, v3(x - 0.032f, y - 0.032f, z - 0.015f), v3(x + 0.032f, y + 0.032f, z + 0.005f), 0.008f);
  mb_paint(0xFFF0C8A0, L_SKIN);
  mb_rbox(b, v3(x - 0.03f, y - 0.025f, z - 0.075f), v3(x + 0.03f, y + 0.03f, z - 0.012f), 0.014f);
}

void weapons_build(void) {
  MB b;
  /* kamera: +x prawo, +y góra, -z przód; broń w prawym dolnym rogu */
  /* pięści */
  mb_init(&b);
  sleeve_hand(&b, 0, 0, 0, 0.35f);
  FIST = gm_upload(&b);
  mb_free(&b);
  mb_init(&b);
  sleeve_hand(&b, 0, 0, 0, 0.35f);
  HANDL = gm_upload(&b);
  mb_free(&b);
  /* Colt 1911 */
  mb_init(&b);
  mb_paint(0xFF22242A, L_METAL);
  mb_rbox(&b, v3(-0.016f, 0.0f, -0.2f), v3(0.016f, 0.035f, 0.0f), 0.005f);    /* zamek */
  mb_paint(0xFF2A2C32, L_METAL);
  mb_rbox(&b, v3(-0.014f, -0.015f, -0.18f), v3(0.014f, 0.002f, -0.03f), 0.004f);
  mb_paint(0xFF4A2E1A, L_WOOD);
  mb_rbox(&b, v3(-0.017f, -0.11f, -0.035f), v3(0.017f, -0.0f, 0.005f), 0.006f); /* chwyt */
  mb_paint(0xFF101012, L_METAL);
  mb_cyl_z(&b, v3(0, 0.02f, -0.205f), 0.006f, 0.01f, 6, 3);
  mb_box(&b, v3(-0.003f, 0.035f, -0.19f), v3(0.003f, 0.043f, -0.18f));
  mb_box(&b, v3(-0.008f, 0.035f, -0.01f), v3(0.008f, 0.042f, 0.0f));
  mb_paint(0xFF22242A, L_METAL);
  mb_box(&b, v3(-0.004f, -0.035f, -0.06f), v3(0.004f, -0.012f, -0.04f));
  sleeve_hand(&b, 0.0f, -0.06f, 0.06f, 0.35f);
  WM[1] = gm_upload(&b);
  mb_free(&b);
  /* Winchester */
  mb_init(&b);
  mb_paint(0xFF22242A, L_METAL);
  mb_cyl_z(&b, v3(0, 0.02f, -0.35f), 0.014f, 0.6f, 10, 3);
  mb_cyl_z(&b, v3(0, -0.005f, -0.32f), 0.012f, 0.5f, 10, 3);
  mb_rbox(&b, v3(-0.022f, -0.03f, -0.08f), v3(0.022f, 0.035f, 0.06f), 0.006f);
  mb_paint(0xFF5A3A20, L_WOOD);
  mb_rbox(&b, v3(-0.022f, -0.035f, -0.36f), v3(0.022f, 0.002f, -0.2f), 0.01f);   /* czółenko */
  mb_rbox(&b, v3(-0.02f, -0.09f, 0.04f), v3(0.02f, 0.02f, 0.32f), 0.012f);       /* kolba */
  sleeve_hand(&b, 0.0f, -0.055f, 0.12f, 0.3f);
  WM[2] = gm_upload(&b);
  mb_free(&b);
  /* Thompson z magazynkiem bębnowym */
  mb_init(&b);
  mb_paint(0xFF22242A, L_METAL);
  mb_rbox(&b, v3(-0.022f, -0.01f, -0.18f), v3(0.022f, 0.04f, 0.06f), 0.006f);
  mb_cyl_z(&b, v3(0, 0.02f, -0.3f), 0.014f, 0.26f, 10, 3);
  mb_paint(0xFF30333A, L_METAL);
  for (int k = 0; k < 6; k++) mb_cyl_z(&b, v3(0, 0.02f, -0.24f - k * 0.018f), 0.019f, 0.008f, 10, 3);
  mb_cyl_x(&b, v3(0, -0.065f, -0.1f), 0.07f, 0.05f, 16, 3);                   /* bęben */
  mb_paint(0xFF5A3A20, L_WOOD);
  mb_rbox(&b, v3(-0.018f, -0.1f, -0.25f), v3(0.018f, -0.01f, -0.2f), 0.01f);   /* przedni chwyt */
  mb_rbox(&b, v3(-0.016f, -0.1f, 0.0f), v3(0.016f, 0.0f, 0.04f), 0.008f);       /* chwyt */
  mb_rbox(&b, v3(-0.02f, -0.06f, 0.06f), v3(0.02f, 0.03f, 0.3f), 0.012f);      /* kolba */
  sleeve_hand(&b, 0.0f, -0.07f, 0.09f, 0.3f);
  WM[3] = gm_upload(&b);
  mb_free(&b);
  /* błysk */
  mb_init(&b);
  mb_paint(0xFFFFD890, L_WHITE); P_.flags = VF_UNLIT; P_.emis = 1;
  for (int k = 0; k < 3; k++) {
    float a = k * 1.047f;
    V3 u = v3(cosf(a) * 0.06f, sinf(a) * 0.06f, 0), z1 = v3(0, 0, -0.12f);
    mb_quad(&b, v3(-u.x, -u.y, 0), v3(u.x, u.y, 0), v3(u.x * 0.3f, u.y * 0.3f, z1.z), v3(-u.x * 0.3f, -u.y * 0.3f, z1.z), 0, 0, 1, 1);
    mb_quad(&b, v3(u.x, u.y, 0), v3(-u.x, -u.y, 0), v3(-u.x * 0.3f, -u.y * 0.3f, z1.z), v3(u.x * 0.3f, u.y * 0.3f, z1.z), 0, 0, 1, 1);
  }
  FLASH = gm_upload(&b);
  mb_free(&b);
  mb_paint(0xFFFFFFFF, L_WHITE);
}

void weapon_draw_fpp(int w, float bobT, float kick, float reload, float swap, float punch, int flash) {
  M4 cw = r_cam_to_world();
  float bx = sinf(bobT) * 0.012f, by = -fabsf(cosf(bobT)) * 0.01f;
  float drop = reload * 0.18f + swap * 0.25f;
  float sway = sinf(g_time * 1.3f) * 0.003f;
  if (w == WPN_FISTS) {
    M4 l = m4_mul(m4_translate(-0.2f + bx, -0.2f + by + sway, -0.38f), m4_mul(m4_rotx(0.15f), m4_roty(0.15f)));
    M4 r = m4_mul(m4_translate(0.2f + bx - punch * 0.08f, -0.2f + by + punch * 0.06f, -0.38f - punch * 0.25f), m4_mul(m4_rotx(0.15f + punch * 0.2f), m4_roty(-0.15f)));
    M4 ml = m4_mul(cw, l), mr = m4_mul(cw, r);
    r_draw(&HANDL, &ml, 0);
    r_draw(&FIST, &mr, 0);
    return;
  }
  float kz = kick * (w == WPN_SHOTGUN ? 0.09f : 0.04f), kr = kick * (w == WPN_SHOTGUN ? 0.25f : 0.12f);
  M4 local = m4_mul(m4_translate(0.15f + bx, -0.17f + by - drop + sway, -0.33f + kz), m4_mul(m4_rotx(kr - reload * 0.5f), m4_mul(m4_roty(0.04f), m4_rotz(reload * 0.4f))));
  M4 m = m4_mul(cw, local);
  r_draw(&WM[w], &m, 0);
  if (w != WPN_PISTOL) {
    /* lewa dłoń na łożu */
    float zf = w == WPN_TOMMY ? -0.22f : -0.28f;
    M4 lh = m4_mul(local, m4_mul(m4_translate(-0.05f, -0.05f, zf + 0.33f), m4_roty(0.5f)));
    M4 ml = m4_mul(cw, lh);
    r_draw(&HANDL, &ml, 0);
  }
  if (flash) {
    float z = w == WPN_PISTOL ? -0.21f : w == WPN_SHOTGUN ? -0.66f : -0.44f;
    M4 f = m4_mul(local, m4_mul(m4_translate(0, 0.02f, z), m4_rotz(g_time * 40)));
    M4 mf = m4_mul(cw, f);
    r_draw(&FLASH, &mf, 0);
    V3 p = m4_point(mf, v3(0, 0, -0.05f));
    r_glow(p.x, p.y, p.z, 0.12f, 4.0f, 2.6f, 1.0f);
  }
}
