#!/bin/sh
# Buduje przykład: build/demo (Linux, z OSMesa) i KroniX_Demo.exe (Windows, mingw-w64).
set -e
cd "$(dirname "$0")"
KX_ROOT=../..
. "$KX_ROOT/kx_sources.sh"
mkdir -p build
CFLAGS="-O2 -std=c99 -Wall -Wno-format-truncation $KX_INC"
OSM=$(ls /usr/lib/x86_64-linux-gnu/libOSMesa.so* 2>/dev/null | head -1)
gcc $CFLAGS $KX_SRC demo.c $KX_PLAT_LINUX $OSM -lm -o build/demo
if command -v x86_64-w64-mingw32-gcc >/dev/null; then
  x86_64-w64-mingw32-gcc $CFLAGS -mwindows -static $KX_SRC demo.c $KX_PLAT_WIN $KX_LIBS_WIN -o build/KroniX_Demo.exe
  x86_64-w64-mingw32-strip build/KroniX_Demo.exe
fi
echo OK
