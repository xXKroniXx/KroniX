/* KroniX Engine — kolizje statyczne (patrz kx_coll.h). */
#include "kx_coll.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct { float x, z, hx, hz, c, s, r; unsigned mask; int box; } Shape;
typedef struct { int shape, next; } Link;
struct KxColl {
  int w, h;
  int *head;          /* pierwszy link w kubełku */
  Shape *sh; int nsh, csh;
  Link *ln; int nln, cln;
};

KxColl *kxc_create(int w, int h) {
  KxColl *c = calloc(1, sizeof *c);
  if (!c) return NULL;
  c->w = w > 1 ? w : 1; c->h = h > 1 ? h : 1;
  c->head = malloc(sizeof(int) * c->w * c->h);
  kxc_clear(c);
  return c;
}
void kxc_free(KxColl *c) {
  if (!c) return;
  free(c->head); free(c->sh); free(c->ln); free(c);
}
void kxc_clear(KxColl *c) {
  if (!c) return;
  for (int i = 0; i < c->w * c->h; i++) c->head[i] = -1;
  c->nsh = c->nln = 0;
}
int kxc_count(const KxColl *c) { return c ? c->nsh : 0; }

static void insert(KxColl *c, int id, float x0, float z0, float x1, float z1) {
  int ix0 = (int)floorf(x0), iz0 = (int)floorf(z0), ix1 = (int)floorf(x1), iz1 = (int)floorf(z1);
  if (ix0 < 0) ix0 = 0;
  if (iz0 < 0) iz0 = 0;
  if (ix1 >= c->w) ix1 = c->w - 1;
  if (iz1 >= c->h) iz1 = c->h - 1;
  for (int z = iz0; z <= iz1; z++)
    for (int x = ix0; x <= ix1; x++) {
      if (c->nln == c->cln) { c->cln = c->cln ? c->cln * 2 : 256; c->ln = realloc(c->ln, sizeof(Link) * c->cln); }
      Link *l = &c->ln[c->nln];
      l->shape = id; l->next = c->head[z * c->w + x];
      c->head[z * c->w + x] = c->nln++;
    }
}
static int add(KxColl *c, Shape s) {
  if (c->nsh == c->csh) { c->csh = c->csh ? c->csh * 2 : 128; c->sh = realloc(c->sh, sizeof(Shape) * c->csh); }
  c->sh[c->nsh] = s;
  return c->nsh++;
}
void kxc_circle(KxColl *c, float x, float z, float r, unsigned mask) {
  if (!c) return;
  Shape s = {x, z, r, r, 1, 0, r, mask, 0};
  int id = add(c, s);
  insert(c, id, x - r, z - r, x + r, z + r);
}
void kxc_box(KxColl *c, float cx, float cz, float hx, float hz, float yaw, unsigned mask) {
  if (!c) return;
  Shape s = {cx, cz, hx, hz, cosf(yaw), sinf(yaw), sqrtf(hx * hx + hz * hz), mask, 1};
  int id = add(c, s);
  insert(c, id, cx - s.r, cz - s.r, cx + s.r, cz + s.r);
}

/* wektor wypchnięcia koła (x,z,r) z kształtu; 0 = brak kolizji */
static int push(const Shape *s, float x, float z, float r, float *ox, float *oz) {
  float dx = x - s->x, dz = z - s->z;
  if (!s->box) {
    float d2 = dx * dx + dz * dz, rr = r + s->r;
    if (d2 >= rr * rr) return 0;
    float d = sqrtf(d2);
    if (d < 1e-5f) { *ox = rr; *oz = 0; return 1; }
    *ox = dx / d * (rr - d); *oz = dz / d * (rr - d);
    return 1;
  }
  /* do układu prostokąta (odwrotność obrotu m4_roty) */
  float lx = dx * s->c - dz * s->s, lz = dx * s->s + dz * s->c;
  float qx = lx < -s->hx ? -s->hx : lx > s->hx ? s->hx : lx;
  float qz = lz < -s->hz ? -s->hz : lz > s->hz ? s->hz : lz;
  float ex = lx - qx, ez = lz - qz, e2 = ex * ex + ez * ez;
  float px, pz;
  if (e2 > 1e-10f) {
    if (e2 >= r * r) return 0;
    float e = sqrtf(e2);
    px = ex / e * (r - e); pz = ez / e * (r - e);
  } else {
    /* środek koła w prostokącie: wypchnij najkrótszą drogą */
    float mx = s->hx - fabsf(lx), mz = s->hz - fabsf(lz);
    if (mx < mz) { px = (lx < 0 ? -1 : 1) * (mx + r); pz = 0; }
    else { px = 0; pz = (lz < 0 ? -1 : 1) * (mz + r); }
  }
  /* z powrotem do świata */
  *ox = px * s->c + pz * s->s; *oz = -px * s->s + pz * s->c;
  return 1;
}

int kxc_overlap(const KxColl *c, float x, float z, float r, unsigned mask) {
  if (!c) return 0;
  int ix0 = (int)floorf(x - r), iz0 = (int)floorf(z - r), ix1 = (int)floorf(x + r), iz1 = (int)floorf(z + r);
  for (int iz = iz0; iz <= iz1; iz++)
    for (int ix = ix0; ix <= ix1; ix++) {
      if (ix < 0 || iz < 0 || ix >= c->w || iz >= c->h) continue;
      for (int l = c->head[iz * c->w + ix]; l >= 0; l = c->ln[l].next) {
        const Shape *s = &c->sh[c->ln[l].shape];
        float ox, oz;
        if ((s->mask & mask) && push(s, x, z, r, &ox, &oz)) return 1;
      }
    }
  return 0;
}

int kxc_resolve(const KxColl *c, float *x, float *z, float r, unsigned mask) {
  if (!c) return 0;
  int moved = 0;
  for (int it = 0; it < 3; it++) {
    int any = 0;
    int ix0 = (int)floorf(*x - r), iz0 = (int)floorf(*z - r), ix1 = (int)floorf(*x + r), iz1 = (int)floorf(*z + r);
    for (int iz = iz0; iz <= iz1; iz++)
      for (int ix = ix0; ix <= ix1; ix++) {
        if (ix < 0 || iz < 0 || ix >= c->w || iz >= c->h) continue;
        for (int l = c->head[iz * c->w + ix]; l >= 0; l = c->ln[l].next) {
          const Shape *s = &c->sh[c->ln[l].shape];
          float ox, oz;
          if (!(s->mask & mask) || !push(s, *x, *z, r, &ox, &oz)) continue;
          *x += ox * 1.001f; *z += oz * 1.001f;
          any = moved = 1;
        }
      }
    if (!any) break;
  }
  return moved;
}
