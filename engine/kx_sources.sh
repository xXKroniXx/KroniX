# KroniX Engine — lista źródeł i flag dla skryptów budujących gry.
# Użycie w build.sh gry:   KX_ROOT=../engine; . "$KX_ROOT/kx_sources.sh"
KX_SRC="$KX_ROOT/core/kx_core.c $KX_ROOT/gfx/glapi.c $KX_ROOT/gfx/mesh.c $KX_ROOT/gfx/render.c $KX_ROOT/gfx/ui2d.c $KX_ROOT/gfx/font_data.c $KX_ROOT/gfx/kx_proc.c $KX_ROOT/audio/kx_audio.c"
KX_INC="-I$KX_ROOT -I$KX_ROOT/gfx -I$KX_ROOT/audio"
KX_PLAT_LINUX="$KX_ROOT/platform/headless.c"
KX_PLAT_WIN="$KX_ROOT/platform/win32.c"
KX_LIBS_WIN="-lopengl32 -lgdi32 -lwinmm -luser32"
