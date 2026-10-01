#!/bin/sh
# Buduje grę: KroniX_Rodzina.exe (Windows) oraz kronix_test (Linux, testy).
# Silnik KroniX Engine jest w ../engine (wspólny dla gier KroniX).
set -e
cd "$(dirname "$0")"
KX_ROOT=../engine
. "$KX_ROOT/kx_sources.sh"
mkdir -p build
python3 tools/embed.py build/story_data.c data/*.txt
SRC="$KX_SRC src/sounds.c src/tex.c src/models.c src/human.c src/weapons.c src/city.c src/vehicle.c src/story.c src/script.c src/world3d.c src/combat.c src/tycoon.c src/ui.c src/game.c build/story_data.c"
CFLAGS="-O2 -std=c99 -Wall -Wno-format-truncation -Wno-parentheses -Wno-misleading-indentation -Isrc $KX_INC"
if [ "$1" != "win" ]; then
  OSM=$(ls /usr/lib/x86_64-linux-gnu/libOSMesa.so* 2>/dev/null | head -1)
  gcc $CFLAGS -g $SRC $KX_PLAT_LINUX $OSM -lm -o build/kronix_test
fi
if [ "$1" != "linux" ] && command -v x86_64-w64-mingw32-gcc >/dev/null; then
  x86_64-w64-mingw32-windres res/kronix.rc -O coff -o build/kronix_res.o
  x86_64-w64-mingw32-gcc $CFLAGS -mwindows -static $SRC $KX_PLAT_WIN build/kronix_res.o $KX_LIBS_WIN -o build/KroniX_Rodzina.exe
  x86_64-w64-mingw32-strip build/KroniX_Rodzina.exe
  cp build/KroniX_Rodzina.exe KroniX_Rodzina.exe
fi
echo "OK"
