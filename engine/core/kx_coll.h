/* KroniX Engine — kolizje statyczne w płaszczyźnie XZ: koła i obrócone prostokąty w siatce
 * kubełków 1x1 (szybkie zapytania niezależnie od liczby rekwizytów).
 *
 *   KxColl *c = kxc_create(szer, wys);          // obszar [0,szer)x[0,wys)
 *   kxc_circle(c, x, z, 0.12f, KXC_SOLID);       // latarnia
 *   kxc_box(c, x, z, 0.5f, 0.15f, yaw, KXC_SOLID); // ławka: połówki wymiarów + obrót (jak m4_roty)
 *   kxc_resolve(c, &px, &pz, 0.25f, KXC_SOLID);  // wypchnij postać z przeszkód (ślizganie)
 *
 * Maski pozwalają mieć przeszkody tylko dla pieszych, tylko dla aut itd. */
#ifndef KX_COLL_H
#define KX_COLL_H

enum { KXC_SOLID = 1, KXC_CAR = 2, KXC_BULLET = 4 };

typedef struct KxColl KxColl;
KxColl *kxc_create(int w, int h);
void kxc_free(KxColl *c);
void kxc_clear(KxColl *c);
void kxc_circle(KxColl *c, float x, float z, float r, unsigned mask);
void kxc_box(KxColl *c, float cx, float cz, float hx, float hz, float yaw, unsigned mask);
int kxc_overlap(const KxColl *c, float x, float z, float r, unsigned mask);    /* czy koło koliduje */
int kxc_resolve(const KxColl *c, float *x, float *z, float r, unsigned mask);  /* 1 = przesunięto */
int kxc_count(const KxColl *c);

#endif
