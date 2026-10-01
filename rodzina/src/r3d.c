/* Programowy renderer 3D: wielokąty z teksturami i z-buforem (perspektywa poprawna),
 * obcinanie do płaszczyzny bliskiej, oświetlenie Gourauda, mgła, billboardy, niebo. */
#include "engine.h"

float zbuf[SCREEN_W * SCREEN_H];
float r3d_focal;

#define NEAR_Z 0.05f

static struct {
  float cx, cy, cz, cyaw, syaw, cp, sp, pitch;
  int fogR, fogG, fogB;
  float fogS, fogE, fogInv;
  float flash;
} R;

Tex *tex_new(int lw, int lh) {
  Tex *t = (Tex *)calloc(1, sizeof(Tex));
  t->lw = lw; t->lh = lh; t->w = 1 << lw; t->h = 1 << lh;
  t->px = (u32 *)calloc((size_t)t->w * t->h, sizeof(u32));
  return t;
}

void r3d_fog(u32 c, float s, float e) {
  R.fogR = (c >> 16) & 255; R.fogG = (c >> 8) & 255; R.fogB = c & 255;
  R.fogS = s; R.fogE = e; R.fogInv = 1.0f / (e - s);
}

void r3d_ambient_flash(float add) { R.flash = add; }

void r3d_begin(const Cam *c) {
  r3d_focal = (SCREEN_W * 0.5f) / tanf(37.5f * PI_F / 180.0f);
  R.cx = c->x; R.cy = c->y; R.cz = c->z;
  R.cyaw = cosf(c->yaw); R.syaw = sinf(c->yaw);
  R.cp = cosf(c->pitch); R.sp = sinf(c->pitch);
  R.pitch = c->pitch;
  for (int i = 0; i < SCREEN_W * SCREEN_H; i++) zbuf[i] = 0.0f;
}

/* świat -> kamera: przód f=(sin yaw,0,cos yaw), prawo r=(-cos yaw,0,sin yaw) */
static inline void to_cam(float x, float y, float z, float *ox, float *oy, float *oz) {
  float dx = x - R.cx, dy = y - R.cy, dz = z - R.cz;
  float rx = -dx * R.cyaw + dz * R.syaw;
  float fz = dx * R.syaw + dz * R.cyaw;
  *ox = rx;
  *oy = dy * R.cp - fz * R.sp;
  *oz = dy * R.sp + fz * R.cp;
}

int r3d_project(float x, float y, float z, float *sx, float *sy, float *depth) {
  float ox, oy, oz;
  to_cam(x, y, z, &ox, &oy, &oz);
  if (oz < NEAR_Z) return 0;
  *sx = SCREEN_W * 0.5f + ox * r3d_focal / oz;
  *sy = SCREEN_H * 0.5f - oy * r3d_focal / oz;
  *depth = oz;
  return 1;
}

float r3d_horizon(void) { return SCREEN_H * 0.5f + tanf(R.pitch) * r3d_focal; }

/* ---------------------------------------------------------------- rasteryzacja */
typedef struct { float x, y, iz, uz, vz, l; } SV; /* wierzchołek ekranowy */

static inline u32 texel_shade(u32 t, int li, float oz, int flags) {
  int r = (t >> 16) & 255, g = (t >> 8) & 255, b = t & 255;
  if (!(flags & RF_FULLBRIGHT)) {
    r = r * li >> 8; g = g * li >> 8; b = b * li >> 8;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
  }
  if (!(flags & RF_NOFOG) && oz > R.fogS) {
    int f = (int)((oz - R.fogS) * R.fogInv * 256.0f);
    if (f > 256) f = 256;
    r += (R.fogR - r) * f >> 8; g += (R.fogG - g) * f >> 8; b += (R.fogB - b) * f >> 8;
  }
  return 0xFF000000u | (u32)(r << 16) | (u32)(g << 8) | (u32)b;
}

static void raster_tri(const SV *a, const SV *b, const SV *c, const Tex *t, int flags) {
  /* sortowanie po y */
  const SV *v[3] = {a, b, c};
  for (int i = 0; i < 2; i++)
    for (int j = i + 1; j < 3; j++)
      if (v[j]->y < v[i]->y) { const SV *tmp = v[i]; v[i] = v[j]; v[j] = tmp; }
  const SV *p0 = v[0], *p1 = v[1], *p2 = v[2];
  if (p2->y - p0->y < 0.0001f) return;
  int y0 = (int)ceilf(p0->y - 0.5f), y2 = (int)ceilf(p2->y - 0.5f);
  if (y0 < 0) y0 = 0;
  if (y2 > SCREEN_H) y2 = SCREEN_H;
  int wmask = t->w - 1, hmask = t->h - 1, lw = t->lw;
  float tw = (float)t->w, th = (float)t->h;
  for (int y = y0; y < y2; y++) {
    float fy = y + 0.5f;
    /* długa krawędź p0->p2 */
    float ta = (fy - p0->y) / (p2->y - p0->y);
    SV A, B;
    A.x = p0->x + (p2->x - p0->x) * ta; A.iz = p0->iz + (p2->iz - p0->iz) * ta;
    A.uz = p0->uz + (p2->uz - p0->uz) * ta; A.vz = p0->vz + (p2->vz - p0->vz) * ta;
    A.l = p0->l + (p2->l - p0->l) * ta;
    const SV *s0, *s1;
    if (fy < p1->y) { s0 = p0; s1 = p1; } else { s0 = p1; s1 = p2; }
    float dy = s1->y - s0->y;
    float tb = dy > 0.0001f ? (fy - s0->y) / dy : 0.0f;
    B.x = s0->x + (s1->x - s0->x) * tb; B.iz = s0->iz + (s1->iz - s0->iz) * tb;
    B.uz = s0->uz + (s1->uz - s0->uz) * tb; B.vz = s0->vz + (s1->vz - s0->vz) * tb;
    B.l = s0->l + (s1->l - s0->l) * tb;
    if (A.x > B.x) { SV tmp = A; A = B; B = tmp; }
    int x0 = (int)ceilf(A.x - 0.5f), x1 = (int)ceilf(B.x - 0.5f);
    if (x1 <= x0) continue;
    float span = B.x - A.x;
    if (span < 0.0001f) span = 0.0001f;
    float diz = (B.iz - A.iz) / span, duz = (B.uz - A.uz) / span, dvz = (B.vz - A.vz) / span, dl = (B.l - A.l) / span;
    float pre = x0 + 0.5f - A.x;
    float iz = A.iz + diz * pre, uz = A.uz + duz * pre, vz = A.vz + dvz * pre, l = A.l + dl * pre;
    if (x0 < 0) { float s = (float)-x0; iz += diz * s; uz += duz * s; vz += dvz * s; l += dl * s; x0 = 0; }
    if (x1 > SCREEN_W) x1 = SCREEN_W;
    u32 *row = fb + y * SCREEN_W;
    float *zrow = zbuf + y * SCREEN_W;
    for (int x = x0; x < x1; x++, iz += diz, uz += duz, vz += dvz, l += dl) {
      if (iz <= zrow[x]) continue;
      float oz = 1.0f / iz;
      int tu = (int)floorf(uz * oz * tw) & wmask, tv = (int)floorf(vz * oz * th) & hmask;
      u32 tx = t->px[(tv << lw) + tu];
      if ((flags & RF_ALPHA) && !(tx >> 24)) continue;
      float lf = l + R.flash;
      int li = (int)(lf * 256.0f);
      if (li < 0) li = 0;
      u32 col = texel_shade(tx, li, oz, flags);
      if (flags & RF_ADD) {
        u32 d = row[x];
        int r = ((d >> 16) & 255) + ((col >> 16) & 255), g = ((d >> 8) & 255) + ((col >> 8) & 255), b = (d & 255) + (col & 255);
        row[x] = RGB(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
      } else row[x] = col;
      if (!(flags & RF_NOZWRITE)) zrow[x] = iz;
    }
  }
}

void r3d_poly(const Vtx *v, int n, const Tex *t, int flags) {
  if (n < 3 || n > 8 || !t) return;
  /* do przestrzeni kamery */
  float cx[16], cy[16], cz[16], cu[16], cv[16], cl[16];
  int m = 0, allBehind = 1, allL = 1, allR = 1, allU = 1, allD = 1;
  for (int i = 0; i < n; i++) {
    to_cam(v[i].x, v[i].y, v[i].z, &cx[i], &cy[i], &cz[i]);
    cu[i] = v[i].u; cv[i] = v[i].v; cl[i] = v[i].l;
    if (cz[i] >= NEAR_Z) allBehind = 0;
    /* szybkie odrzucanie poza stożkiem widzenia */
    float lim = cz[i] * (SCREEN_W * 0.5f / r3d_focal) * 1.02f + 0.01f;
    float limy = cz[i] * (SCREEN_H * 0.5f / r3d_focal) * 1.02f + 0.01f;
    if (cx[i] > -lim) allL = 0;
    if (cx[i] < lim) allR = 0;
    if (cy[i] > -limy) allD = 0;
    if (cy[i] < limy) allU = 0;
  }
  if (allBehind || allL || allR || allU || allD) return;
  /* obcinanie płaszczyzną bliską (Sutherland-Hodgman) */
  float ox[16], oy[16], oz[16], ou[16], ov[16], ol[16];
  for (int i = 0; i < n; i++) {
    int j = (i + 1) % n;
    int in_i = cz[i] >= NEAR_Z, in_j = cz[j] >= NEAR_Z;
    if (in_i) { ox[m] = cx[i]; oy[m] = cy[i]; oz[m] = cz[i]; ou[m] = cu[i]; ov[m] = cv[i]; ol[m] = cl[i]; m++; }
    if (in_i != in_j) {
      float tt = (NEAR_Z - cz[i]) / (cz[j] - cz[i]);
      ox[m] = cx[i] + (cx[j] - cx[i]) * tt; oy[m] = cy[i] + (cy[j] - cy[i]) * tt; oz[m] = NEAR_Z;
      ou[m] = cu[i] + (cu[j] - cu[i]) * tt; ov[m] = cv[i] + (cv[j] - cv[i]) * tt; ol[m] = cl[i] + (cl[j] - cl[i]) * tt;
      m++;
    }
  }
  if (m < 3) return;
  SV s[16];
  float hw = SCREEN_W * 0.5f, hh = SCREEN_H * 0.5f;
  for (int i = 0; i < m; i++) {
    float iz = 1.0f / oz[i];
    s[i].x = hw + ox[i] * r3d_focal * iz;
    s[i].y = hh - oy[i] * r3d_focal * iz;
    s[i].iz = iz; s[i].uz = ou[i] * iz; s[i].vz = ov[i] * iz; s[i].l = ol[i];
  }
  for (int i = 1; i + 1 < m; i++) raster_tri(&s[0], &s[i], &s[i + 1], t, flags);
}

/* Billboard: postać/obiekt zawsze zwrócony do kamery, podstawa na wysokości y. */
void r3d_billboard(float x, float y, float z, float w, float h, const Sprite *spr, int flip, float light, int flags) {
  if (!spr) return;
  float bx, by, bz, tx, ty, tz;
  to_cam(x, y, z, &bx, &by, &bz);
  to_cam(x, y + h, z, &tx, &ty, &tz);
  if (bz < NEAR_Z || tz < NEAR_Z) return;
  float hw = SCREEN_W * 0.5f, hh = SCREEN_H * 0.5f;
  float sxb = hw + bx * r3d_focal / bz, syb = hh - by * r3d_focal / bz;
  float syt = hh - ty * r3d_focal / tz, sxt = hw + tx * r3d_focal / tz;
  float pw = w * r3d_focal / bz;
  float ph = syb - syt;
  if (ph < 1 || pw < 1) return;
  int x0 = (int)(sxb - pw * 0.5f), x1 = (int)(sxb + pw * 0.5f);
  int y0 = (int)syt, y1 = (int)syb;
  if (x1 < 0 || x0 >= SCREEN_W || y1 < 0 || y0 >= SCREEN_H) return;
  float iz = 1.0f / bz;
  int li = (int)((light + R.flash) * 256.0f);
  (void)sxt;
  for (int y = y0 < 0 ? 0 : y0; y < y1 && y < SCREEN_H; y++) {
    int sy = (int)((y - syt) * spr->h / ph);
    if (sy < 0 || sy >= spr->h) continue;
    int off = 0;
    for (int xx = x0; xx < x1; xx++) {
      int X = xx + off;
      if (X < 0 || X >= SCREEN_W) continue;
      int i = y * SCREEN_W + X;
      if (iz <= zbuf[i]) continue;
      int sx = (int)((xx - (sxb - pw * 0.5f)) * spr->w / pw);
      if (sx < 0 || sx >= spr->w) continue;
      u32 c = spr->px[sy * spr->w + (flip ? spr->w - 1 - sx : sx)];
      if (!(c >> 24)) continue;
      if (flags & RF_ADD) {
        u32 d = fb[i];
        int r = ((d >> 16) & 255) + ((c >> 16) & 255), g = ((d >> 8) & 255) + ((c >> 8) & 255), b = (d & 255) + (c & 255);
        fb[i] = RGB(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
        continue;
      }
      fb[i] = texel_shade(c, li, bz, flags);
      if (!(flags & RF_NOZWRITE)) zbuf[i] = iz;
    }
  }
}

/* Niebo: gradient, gwiazdy, księżyc i panorama miasta przesuwająca się z obrotem. */
void r3d_sky(int night, int t) {
  float hz = r3d_horizon();
  u32 top = night ? RGB(0x05, 0x06, 0x12) : RGB(0x5a, 0x7a, 0xa8);
  u32 bot = night ? RGB(0x24, 0x22, 0x3a) : RGB(0xb8, 0xc0, 0xc8);
  for (int y = 0; y < SCREEN_H; y++) {
    float k = (y - (hz - 140)) / 140.0f;
    if (k < 0) k = 0;
    if (k > 1) k = 1;
    u32 c = blend(top, bot, (int)(k * 255));
    u32 *row = fb + y * SCREEN_W;
    for (int x = 0; x < SCREEN_W; x++) row[x] = c;
  }
  float yaw = atan2f(R.syaw, R.cyaw);
  const float PANO = 2048.0f;
  if (night) {
    for (int i = 0; i < 160; i++) {
      u32 h = (u32)i * 2654435761u;
      float ang = (h % 3600) / 3600.0f * 2 * PI_F;
      float elev = ((h >> 12) % 1000) / 1000.0f;
      float da = ang - yaw;
      while (da > PI_F) da -= 2 * PI_F;
      while (da < -PI_F) da += 2 * PI_F;
      if (fabsf(da) > 1.0f) continue;
      int sx = (int)(SCREEN_W * 0.5f - tanf(da) * r3d_focal);
      int sy = (int)(hz - 30 - elev * 160);
      if (((h >> 20) + t / 30) % 17 == 0) continue;
      gfx_pset(sx, sy, (h & 3) ? pal('s') : pal('w'));
    }
    float mda = 0.6f - yaw;
    while (mda > PI_F) mda -= 2 * PI_F;
    while (mda < -PI_F) mda += 2 * PI_F;
    if (fabsf(mda) < 1.2f) {
      int mx = (int)(SCREEN_W * 0.5f - tanf(mda) * r3d_focal), my = (int)(hz - 120);
      gfx_circle(mx, my, 14, RGB(0xe8, 0xe4, 0xd0), 240);
      gfx_circle(mx, my, 28, RGB(0xe8, 0xe4, 0xd0), 30);
    }
  }
  /* panorama drapaczy chmur */
  for (int x = 0; x < SCREEN_W; x++) {
    float a = yaw - atanf((x - SCREEN_W * 0.5f) / r3d_focal);
    float fu = a / (2 * PI_F) * PANO;
    int u = ((int)floorf(fu) % (int)PANO + (int)PANO) % (int)PANO;
    int seg = u / 22;
    u32 hs = (u32)seg * 2246822519u;
    int hgt = 30 + (int)(hs % 70);
    if (seg % 7 == 3) hgt += 40;
    u32 col = night ? RGB(0x12, 0x10, 0x1c) : RGB(0x7a, 0x86, 0x96);
    for (int y = (int)hz - hgt; y < (int)hz + 2 && y < SCREEN_H; y++) {
      if (y < 0) continue;
      u32 c = col;
      if (night && ((u % 22) % 5 == 2) && (((y - (int)hz) / 4) % 2 == 0) && ((hs >> ((y & 15))) & 3) == 0) c = RGB(0xc8, 0x9c, 0x50);
      fb[y * SCREEN_W + x] = c;
    }
  }
}
