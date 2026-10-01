/* Matematyka 3D i budowanie siatek (prymitywy low-poly w stylu indie). */
#include "render.h"
#include "glapi.h"
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------- macierze */
M4 m4_identity(void) {
  M4 r;
  memset(&r, 0, sizeof r);
  r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1;
  return r;
}
M4 m4_mul(M4 a, M4 b) {
  M4 r;
  for (int c = 0; c < 4; c++)
    for (int rr = 0; rr < 4; rr++) {
      float s = 0;
      for (int k = 0; k < 4; k++) s += a.m[k * 4 + rr] * b.m[c * 4 + k];
      r.m[c * 4 + rr] = s;
    }
  return r;
}
M4 m4_translate(float x, float y, float z) { M4 r = m4_identity(); r.m[12] = x; r.m[13] = y; r.m[14] = z; return r; }
M4 m4_scale(float x, float y, float z) { M4 r = m4_identity(); r.m[0] = x; r.m[5] = y; r.m[10] = z; return r; }
M4 m4_rotx(float a) { M4 r = m4_identity(); float c = cosf(a), s = sinf(a); r.m[5] = c; r.m[6] = s; r.m[9] = -s; r.m[10] = c; return r; }
M4 m4_roty(float a) { M4 r = m4_identity(); float c = cosf(a), s = sinf(a); r.m[0] = c; r.m[2] = -s; r.m[8] = s; r.m[10] = c; return r; }
M4 m4_rotz(float a) { M4 r = m4_identity(); float c = cosf(a), s = sinf(a); r.m[0] = c; r.m[1] = s; r.m[4] = -s; r.m[5] = c; return r; }
M4 m4_perspective(float fovy, float aspect, float zn, float zf) {
  M4 r;
  memset(&r, 0, sizeof r);
  float f = 1.0f / tanf(fovy * 0.5f);
  r.m[0] = f / aspect;
  r.m[5] = f;
  r.m[10] = (zf + zn) / (zn - zf);
  r.m[11] = -1;
  r.m[14] = 2 * zf * zn / (zn - zf);
  return r;
}
M4 m4_ortho(float l, float rgt, float b, float t, float n, float f) {
  M4 r = m4_identity();
  r.m[0] = 2 / (rgt - l); r.m[5] = 2 / (t - b); r.m[10] = -2 / (f - n);
  r.m[12] = -(rgt + l) / (rgt - l); r.m[13] = -(t + b) / (t - b); r.m[14] = -(f + n) / (f - n);
  return r;
}
V3 m4_point(M4 m, V3 p) {
  return v3(m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12],
            m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13],
            m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14]);
}
V3 m4_dir(M4 m, V3 d) {
  return v3(m.m[0] * d.x + m.m[4] * d.y + m.m[8] * d.z,
            m.m[1] * d.x + m.m[5] * d.y + m.m[9] * d.z,
            m.m[2] * d.x + m.m[6] * d.y + m.m[10] * d.z);
}

/* ---------------------------------------------------------------- budowniczy */
Paint P_ = {0xFFFFFFFFu, 0, 0, 0, 0, 1.0f};

void mb_init(MB *b) { b->v = NULL; b->n = b->cap = 0; }
void mb_free(MB *b) { free(b->v); b->v = NULL; b->n = b->cap = 0; }
void mb_reset(MB *b) { b->n = 0; }
Vert *mb_push(MB *b, int n) {
  if (b->n + n > b->cap) {
    int nc = b->cap ? b->cap * 2 : 1024;
    while (nc < b->n + n) nc *= 2;
    b->v = (Vert *)realloc(b->v, sizeof(Vert) * nc);
    b->cap = nc;
  }
  Vert *v = b->v + b->n;
  b->n += n;
  return v;
}
void mb_paint(uint32_t col, int layer) {
  P_.col = col; P_.layer = layer; P_.emis = 0; P_.flags = 0; P_.uvs = 1.0f;
}

static void setv(Vert *v, V3 p, V3 n, float u, float vv) {
  v->p[0] = p.x; v->p[1] = p.y; v->p[2] = p.z;
  v->n[0] = n.x; v->n[1] = n.y; v->n[2] = n.z;
  v->uv[0] = u; v->uv[1] = vv; v->uv[2] = (float)P_.layer;
  v->c[0] = (P_.col >> 16) & 255; v->c[1] = (P_.col >> 8) & 255; v->c[2] = P_.col & 255;
  float e = P_.emis < 0 ? 0 : P_.emis > 1 ? 1 : P_.emis;
  v->c[3] = (uint8_t)(e * 255);
  v->bone = (uint8_t)P_.bone; v->flags = (uint8_t)P_.flags; v->pad[0] = v->pad[1] = 0;
}

void mb_tri(MB *b, V3 a, V3 c, V3 d, const float *uva, const float *uvc, const float *uvd) {
  V3 n = v3norm(v3cross(v3sub(c, a), v3sub(d, a)));
  Vert *v = mb_push(b, 3);
  setv(v, a, n, uva[0], uva[1]);
  setv(v + 1, c, n, uvc[0], uvc[1]);
  setv(v + 2, d, n, uvd[0], uvd[1]);
}

void mb_quad(MB *b, V3 a, V3 c, V3 d, V3 e, float u0, float v0, float u1, float v1) {
  V3 n = v3norm(v3cross(v3sub(c, a), v3sub(e, a)));
  Vert *v = mb_push(b, 6);
  setv(v, a, n, u0, v1); setv(v + 1, c, n, u1, v1); setv(v + 2, d, n, u1, v0);
  setv(v + 3, a, n, u0, v1); setv(v + 4, d, n, u1, v0); setv(v + 5, e, n, u0, v0);
}

/* uv z rzutu na płaszczyznę prostopadłą do dominującej osi normalnej */
static void world_uv(V3 p, V3 n, float *u, float *v) {
  float ax = fabsf(n.x), ay = fabsf(n.y), az = fabsf(n.z);
  if (ay >= ax && ay >= az) { *u = p.x; *v = p.z; }
  else if (ax >= az) { *u = n.x > 0 ? -p.z : p.z; *v = -p.y; }
  else { *u = n.z > 0 ? p.x : -p.x; *v = -p.y; }
  *u *= P_.uvs; *v *= P_.uvs;
}

void mb_quad_world(MB *b, V3 a, V3 c, V3 d, V3 e) {
  V3 n = v3norm(v3cross(v3sub(c, a), v3sub(e, a)));
  Vert *v = mb_push(b, 6);
  V3 ps[6] = {a, c, d, a, d, e};
  for (int i = 0; i < 6; i++) { float u, vv; world_uv(ps[i], n, &u, &vv); setv(v + i, ps[i], n, u, vv); }
}

void mb_box_faces(MB *b, V3 mn, V3 mx, int m) {
  V3 p000 = v3(mn.x, mn.y, mn.z), p100 = v3(mx.x, mn.y, mn.z), p010 = v3(mn.x, mx.y, mn.z), p110 = v3(mx.x, mx.y, mn.z);
  V3 p001 = v3(mn.x, mn.y, mx.z), p101 = v3(mx.x, mn.y, mx.z), p011 = v3(mn.x, mx.y, mx.z), p111 = v3(mx.x, mx.y, mx.z);
  if (m & 1) mb_quad_world(b, p000, p001, p011, p010);  /* -x */
  if (m & 2) mb_quad_world(b, p101, p100, p110, p111);  /* +x */
  if (m & 4) mb_quad_world(b, p000, p100, p101, p001);  /* -y */
  if (m & 8) mb_quad_world(b, p010, p011, p111, p110);  /* +y */
  if (m & 16) mb_quad_world(b, p100, p000, p010, p110); /* -z */
  if (m & 32) mb_quad_world(b, p001, p101, p111, p011); /* +z */
}
void mb_box(MB *b, V3 mn, V3 mx) { mb_box_faces(b, mn, mx, 63); }

void mb_cyl(MB *b, V3 base, float r0, float r1, float h, int seg, int caps) {
  for (int i = 0; i < seg; i++) {
    float a0 = 2 * 3.14159265f * i / seg, a1 = 2 * 3.14159265f * (i + 1) / seg;
    float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
    V3 b0 = v3(base.x + c0 * r0, base.y, base.z + s0 * r0), b1 = v3(base.x + c1 * r0, base.y, base.z + s1 * r0);
    V3 t0 = v3(base.x + c0 * r1, base.y + h, base.z + s0 * r1), t1 = v3(base.x + c1 * r1, base.y + h, base.z + s1 * r1);
    float slope = (r0 - r1) / (h != 0 ? h : 1);
    V3 n0 = v3norm(v3(c0, slope, s0)), n1 = v3norm(v3(c1, slope, s1));
    Vert *v = mb_push(b, 6);
    float u0 = (float)i / seg * 2 * 3.14159265f * (r0 > r1 ? r0 : r1) * P_.uvs, u1 = (float)(i + 1) / seg * 2 * 3.14159265f * (r0 > r1 ? r0 : r1) * P_.uvs;
    float vv = h * P_.uvs;
    setv(v, b0, n0, u0, vv); setv(v + 1, t1, n1, u1, 0); setv(v + 2, b1, n1, u1, vv);
    setv(v + 3, b0, n0, u0, vv); setv(v + 4, t0, n0, u0, 0); setv(v + 5, t1, n1, u1, 0);
    if (caps & 1 && r0 > 0) {
      Vert *c = mb_push(b, 3);
      V3 dn = v3(0, -1, 0);
      setv(c, base, dn, 0.5f, 0.5f); setv(c + 1, b0, dn, 0.5f + c0 * 0.5f, 0.5f + s0 * 0.5f); setv(c + 2, b1, dn, 0.5f + c1 * 0.5f, 0.5f + s1 * 0.5f);
    }
    if (caps & 2 && r1 > 0) {
      Vert *c = mb_push(b, 3);
      V3 up = v3(0, 1, 0), top = v3(base.x, base.y + h, base.z);
      setv(c, top, up, 0.5f, 0.5f); setv(c + 1, t1, up, 0.5f + c1 * 0.5f, 0.5f + s1 * 0.5f); setv(c + 2, t0, up, 0.5f + c0 * 0.5f, 0.5f + s0 * 0.5f);
    }
  }
}

static void mb_cyl_axis(MB *b, V3 c, float r, float len, int seg, int caps, int axis) {
  MB t;
  mb_init(&t);
  mb_cyl(&t, v3(0, -len * 0.5f, 0), r, r, len, seg, caps);
  M4 tf = axis == 0 ? m4_mul(m4_translate(c.x, c.y, c.z), m4_rotz(-1.5707963f)) : m4_mul(m4_translate(c.x, c.y, c.z), m4_rotx(1.5707963f));
  mb_append_tf(b, &t, tf);
  mb_free(&t);
}
void mb_cyl_x(MB *b, V3 c, float r, float len, int seg, int caps) { mb_cyl_axis(b, c, r, len, seg, caps, 0); }
void mb_cyl_z(MB *b, V3 c, float r, float len, int seg, int caps) { mb_cyl_axis(b, c, r, len, seg, caps, 2); }

void mb_sphere(MB *b, V3 c, float rx, float ry, float rz, int seg) {
  int rings = seg / 2 < 3 ? 3 : seg / 2;
  for (int j = 0; j < rings; j++) {
    float t0 = 3.14159265f * j / rings, t1 = 3.14159265f * (j + 1) / rings;
    for (int i = 0; i < seg; i++) {
      float p0 = 2 * 3.14159265f * i / seg, p1 = 2 * 3.14159265f * (i + 1) / seg;
      V3 d[4] = {
        v3(sinf(t0) * cosf(p0), cosf(t0), sinf(t0) * sinf(p0)), v3(sinf(t0) * cosf(p1), cosf(t0), sinf(t0) * sinf(p1)),
        v3(sinf(t1) * cosf(p1), cosf(t1), sinf(t1) * sinf(p1)), v3(sinf(t1) * cosf(p0), cosf(t1), sinf(t1) * sinf(p0))};
      V3 p[4];
      for (int k = 0; k < 4; k++) p[k] = v3(c.x + d[k].x * rx, c.y + d[k].y * ry, c.z + d[k].z * rz);
      float uv[4][2] = {{(float)i / seg, (float)j / rings}, {(float)(i + 1) / seg, (float)j / rings},
                        {(float)(i + 1) / seg, (float)(j + 1) / rings}, {(float)i / seg, (float)(j + 1) / rings}};
      int idx[6] = {0, 1, 2, 0, 2, 3};
      Vert *v = mb_push(b, 6);
      for (int k = 0; k < 6; k++) {
        V3 n = v3norm(v3(d[idx[k]].x / rx, d[idx[k]].y / ry, d[idx[k]].z / rz));
        setv(v + k, p[idx[k]], n, uv[idx[k]][0], uv[idx[k]][1]);
      }
      /* trójkąty muszą być CCW patrząc z zewnątrz */
      Vert tmp = v[1]; v[1] = v[2]; v[2] = tmp;
      tmp = v[4]; v[4] = v[5]; v[5] = tmp;
    }
  }
}

/* czworokąt skierowany „na zewnątrz” względem punktu ctr */
static void quad_out(MB *b, V3 a, V3 c, V3 d, V3 f, V3 ctr) {
  V3 n = v3cross(v3sub(c, a), v3sub(f, a));
  V3 mid = v3mul(v3add(v3add(a, c), v3add(d, f)), 0.25f);
  if (v3dot(n, v3sub(mid, ctr)) >= 0) mb_quad_world(b, a, c, d, f);
  else mb_quad_world(b, a, f, d, c);
}

/* fazowany prostopadłościan (miękkie krawędzie, wygląd „indie low-poly”) */
void mb_rbox(MB *b, V3 mn, V3 mx, float e) {
  float sx = mx.x - mn.x, sy = mx.y - mn.y, sz = mx.z - mn.z;
  float lim = (sx < sy ? (sx < sz ? sx : sz) : (sy < sz ? sy : sz)) * 0.45f;
  if (e > lim) e = lim;
  if (e <= 0.0005f) { mb_box(b, mn, mx); return; }
  V3 C = v3((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f, (mn.z + mx.z) * 0.5f);
  float X[2] = {mn.x, mx.x}, Y[2] = {mn.y, mx.y}, Z[2] = {mn.z, mx.z};
  float sg[2] = {1, -1}; /* kierunek do środka */
  /* ściany */
  for (int i = 0; i < 2; i++) {
    quad_out(b, v3(X[i], mn.y + e, mn.z + e), v3(X[i], mn.y + e, mx.z - e), v3(X[i], mx.y - e, mx.z - e), v3(X[i], mx.y - e, mn.z + e), C);
    quad_out(b, v3(mn.x + e, Y[i], mn.z + e), v3(mx.x - e, Y[i], mn.z + e), v3(mx.x - e, Y[i], mx.z - e), v3(mn.x + e, Y[i], mx.z - e), C);
    quad_out(b, v3(mn.x + e, mn.y + e, Z[i]), v3(mx.x - e, mn.y + e, Z[i]), v3(mx.x - e, mx.y - e, Z[i]), v3(mn.x + e, mx.y - e, Z[i]), C);
  }
  /* krawędzie */
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++) {
      float xi = X[i] + sg[i] * e, yj = Y[j] + sg[j] * e, zj = Z[j] + sg[j] * e, yi = Y[i] + sg[i] * e;
      quad_out(b, v3(X[i], yj, mn.z + e), v3(X[i], yj, mx.z - e), v3(xi, Y[j], mx.z - e), v3(xi, Y[j], mn.z + e), C);
      quad_out(b, v3(X[i], mn.y + e, zj), v3(X[i], mx.y - e, zj), v3(xi, mx.y - e, Z[j]), v3(xi, mn.y + e, Z[j]), C);
      quad_out(b, v3(mn.x + e, Y[i], zj), v3(mx.x - e, Y[i], zj), v3(mx.x - e, yi, Z[j]), v3(mn.x + e, yi, Z[j]), C);
    }
  /* narożniki */
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++)
      for (int k = 0; k < 2; k++) {
        V3 a = v3(X[i], Y[j] + sg[j] * e, Z[k] + sg[k] * e);
        V3 c = v3(X[i] + sg[i] * e, Y[j], Z[k] + sg[k] * e);
        V3 d = v3(X[i] + sg[i] * e, Y[j] + sg[j] * e, Z[k]);
        float uv0[2] = {0, 0}, uv1[2] = {0.1f, 0}, uv2[2] = {0, 0.1f};
        V3 n = v3cross(v3sub(c, a), v3sub(d, a));
        if (v3dot(n, v3sub(a, C)) > 0) mb_tri(b, a, c, d, uv0, uv1, uv2); else mb_tri(b, a, d, c, uv0, uv2, uv1);
      }
}

void mb_append_tf(MB *dst, const MB *src, M4 tf) {
  Vert *v = mb_push(dst, src->n);
  for (int i = 0; i < src->n; i++) {
    v[i] = src->v[i];
    V3 p = m4_point(tf, v3(src->v[i].p[0], src->v[i].p[1], src->v[i].p[2]));
    V3 n = v3norm(m4_dir(tf, v3(src->v[i].n[0], src->v[i].n[1], src->v[i].n[2])));
    v[i].p[0] = p.x; v[i].p[1] = p.y; v[i].p[2] = p.z;
    v[i].n[0] = n.x; v[i].n[1] = n.y; v[i].n[2] = n.z;
  }
}

void mb_colorize_ao(MB *b, float (*ao)(float x, float y, float z)) {
  for (int i = 0; i < b->n; i++) {
    float a = ao(b->v[i].p[0], b->v[i].p[1], b->v[i].p[2]);
    for (int k = 0; k < 3; k++) b->v[i].c[k] = (uint8_t)(b->v[i].c[k] * a);
  }
}

/* ---------------------------------------------------------------- GPU */
static void setup_attribs(void) {
  glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void *)offsetof(Vert, p));
  glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void *)offsetof(Vert, n));
  glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void *)offsetof(Vert, uv));
  glEnableVertexAttribArray(3); glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vert), (void *)offsetof(Vert, c));
  glEnableVertexAttribArray(4); glVertexAttribPointer(4, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vert), (void *)offsetof(Vert, bone));
}

static void bounds(GMesh *g, const MB *b) {
  g->bmin[0] = g->bmin[1] = g->bmin[2] = 1e9f;
  g->bmax[0] = g->bmax[1] = g->bmax[2] = -1e9f;
  for (int i = 0; i < b->n; i++)
    for (int k = 0; k < 3; k++) {
      if (b->v[i].p[k] < g->bmin[k]) g->bmin[k] = b->v[i].p[k];
      if (b->v[i].p[k] > g->bmax[k]) g->bmax[k] = b->v[i].p[k];
    }
}

GMesh gm_upload(const MB *b) {
  GMesh g;
  memset(&g, 0, sizeof g);
  glGenVertexArrays(1, &g.vao);
  glGenBuffers(1, &g.vbo);
  glBindVertexArray(g.vao);
  glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(Vert) * (b->n ? b->n : 1), b->n ? b->v : NULL, GL_STATIC_DRAW);
  setup_attribs();
  glBindVertexArray(0);
  g.n = b->n; g.cap = b->n;
  bounds(&g, b);
  return g;
}

void gm_update(GMesh *g, const MB *b) {
  if (!g->vao) {
    glGenVertexArrays(1, &g->vao);
    glGenBuffers(1, &g->vbo);
    glBindVertexArray(g->vao);
    glBindBuffer(GL_ARRAY_BUFFER, g->vbo);
    setup_attribs();
    glBindVertexArray(0);
  }
  glBindBuffer(GL_ARRAY_BUFFER, g->vbo);
  if (b->n > g->cap) {
    g->cap = b->n + b->n / 2 + 64;
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(Vert) * g->cap, NULL, GL_DYNAMIC_DRAW);
  }
  if (b->n) glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)sizeof(Vert) * b->n, b->v);
  g->n = b->n;
}

void gm_free(GMesh *g) {
  if (g->vbo) glDeleteBuffers(1, &g->vbo);
  if (g->vao) glDeleteVertexArrays(1, &g->vao);
  memset(g, 0, sizeof *g);
}
