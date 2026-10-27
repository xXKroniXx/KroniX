/* KroniX Engine — profiler klatki: czasy sekcji CPU (wygładzone), czas GPU (zapytania czasowe),
 * liczniki renderera. Nakładka F3 rysowana przez d2_prof_overlay() (gfx/ui2d.c).
 *
 *   kx_prof_begin(KXP_UPDATE); ... kx_prof_end(KXP_UPDATE);
 *   kx_prof_frame();   // raz na klatkę ekranu, po SwapBuffers
 *
 * Gra może dodać własne sekcje od KXP_USER (nazwy przez kx_prof_name). */
#ifndef KX_PROF_H
#define KX_PROF_H

enum { KXP_FRAME, KXP_UPDATE, KXP_DRAW, KXP_GPU, KXP_USER, KXP_MAX = 16 };

double kx_time_now(void);                 /* sekundy, zegar monotoniczny wysokiej rozdzielczości */
void kx_prof_name(int id, const char *name);
void kx_prof_begin(int id);
void kx_prof_end(int id);
void kx_prof_add(int id, double ms);      /* dopisz zmierzony czas (np. GPU) */
void kx_prof_frame(void);                 /* zamknij klatkę: wygładzanie, FPS */
double kx_prof_ms(int id);                /* średni czas sekcji [ms] */
double kx_prof_peak(int id);              /* najgorsza klatka z ostatniej sekundy [ms] */
const char *kx_prof_label(int id);
float kx_prof_fps(void);
extern int g_prof_show;                   /* nakładka F3 */

#endif
