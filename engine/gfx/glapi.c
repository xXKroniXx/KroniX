/* KroniX Engine — Ładowanie wskaźników funkcji OpenGL. */
#include "glapi.h"

#define GLDEF(ret, name, args) PFN_##name name;
GL_FUNCS(GLDEF)
GL_FUNCS_OPT(GLDEF)
#undef GLDEF

int gl_load(void *(*getproc)(const char *), const char **missing) {
#define GLLOAD(ret, name, args) \
  name = (PFN_##name)getproc(#name); \
  if (!name) { if (missing) *missing = #name; return 1; }
  GL_FUNCS(GLLOAD)
#undef GLLOAD
#define GLOPT(ret, name, args) name = (PFN_##name)getproc(#name);
  GL_FUNCS_OPT(GLOPT)
#undef GLOPT
  return 0;
}
