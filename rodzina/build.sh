#!/bin/sh
# Buduje grę: KroniX_Rodzina.exe (Windows) oraz kronix_test (Linux, testy).
set -e
cd "$(dirname "$0")"
mkdir -p build
python3 tools/embed.py build/story_data.c data/*.txt
SRC="src/glapi.c src/mesh.c src/render.c src/ui2d.c src/font_data.c src/tex.c src/models.c src/human.c src/weapons.c src/city.c src/audio.c src/story.c src/script.c src/world3d.c src/combat.c src/tycoon.c src/ui.c src/game.c build/story_data.c"
CFLAGS="-O2 -std=c99 -Wall -Wno-format-truncation -Wno-parentheses -Wno-misleading-indentation -Isrc"
if [ "$1" != "win" ]; then
  OSM=$(ls /usr/lib/x86_64-linux-gnu/libOSMesa.so* 2>/dev/null | head -1)
  gcc $CFLAGS -g $SRC src/plat_headless.c $OSM -lm -o build/kronix_test
fi
if [ "$1" != "linux" ] && command -v x86_64-w64-mingw32-gcc >/dev/null; then
  x86_64-w64-mingw32-windres res/kronix.rc -O coff -o build/kronix_res.o
  x86_64-w64-mingw32-gcc $CFLAGS -mwindows -static $SRC src/plat_win32.c build/kronix_res.o \
    -lopengl32 -lgdi32 -lwinmm -luser32 -o build/KroniX_Rodzina.exe
  x86_64-w64-mingw32-strip build/KroniX_Rodzina.exe
  cp build/KroniX_Rodzina.exe KroniX_Rodzina.exe
fi
echo "OK"
