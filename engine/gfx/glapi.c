/* KroniX Engine — Ładowanie wskaźników funkcji OpenGL. */
#include "glapi.h"

#define GLDEF(ret, name, args) PFN_##name name;
GL_FUNCS(GLDEF)
#undef GLDEF

int gl_load(void *(*getproc)(const char *), const char **missing) {
#define GLLOAD(ret, name, args) \
  name = (PFN_##name)getproc(#name); \
  if (!name) { if (missing) *missing = #name; return 1; }
  GL_FUNCS(GLLOAD)
#undef GLLOAD
  return 0;
}
