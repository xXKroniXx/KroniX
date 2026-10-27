@echo off
setlocal
rem ======================================================================
rem  KroniX: Rodzina - budowanie KroniX_Rodzina.exe na Windows (dwuklik)
rem
rem  Potrzebne (jednorazowo):
rem   1) w64devkit - kompilator C: https://github.com/skeeto/w64devkit/releases
rem      pobierz w64devkit-x64-*.exe, rozpakuj do C:\w64devkit
rem   2) Python 3 - https://www.python.org/downloads/
rem      przy instalacji zaznacz "Add python.exe to PATH"
rem
rem  Lista plikow musi sie zgadzac z ..\engine\kx_sources.sh i build.sh.
rem ======================================================================
cd /d "%~dp0"
if exist C:\w64devkit\bin set "PATH=C:\w64devkit\bin;%PATH%"
gcc --version >nul 2>nul
if errorlevel 1 goto brakgcc
python --version >nul 2>nul
if errorlevel 1 goto brakpy
if not exist build mkdir build

echo [1/4] Mapy (tools\maps.py -^> data\10_mapy.txt)...
python tools\maps.py
if errorlevel 1 goto blad
echo [2/4] Fabula i dane (data\*.txt -^> build\story_data.c)...
python tools\embed.py build\story_data.c "data\*.txt"
if errorlevel 1 goto blad
echo [3/4] Ikona...
windres --include-dir=res res\kronix.rc -O coff -o build\kronix_res.o
if errorlevel 1 goto blad
echo [4/4] Kompilacja (to potrwa chwile)...
set "E=..\engine"
set "SRC=%E%\core\kx_core.c %E%\core\kx_prof.c %E%\core\kx_coll.c %E%\gfx\glapi.c %E%\gfx\mesh.c %E%\gfx\render.c %E%\gfx\ui2d.c %E%\gfx\font_data.c %E%\gfx\kx_proc.c %E%\audio\kx_audio.c src\sounds.c src\tex.c src\models.c src\human.c src\weapons.c src\city.c src\vehicle.c src\streetlife.c src\clock.c src\story.c src\script.c src\world3d.c src\combat.c src\tycoon.c src\ui.c src\game.c build\story_data.c %E%\platform\win32.c"
set "CFLAGS=-O2 -std=c99 -Wall -Wno-format-truncation -Wno-parentheses -Wno-misleading-indentation -Isrc -I%E% -I%E%\core -I%E%\gfx -I%E%\audio"
gcc %CFLAGS% -mwindows -static %SRC% build\kronix_res.o -lopengl32 -lgdi32 -lwinmm -luser32 -o KroniX_Rodzina.exe
if errorlevel 1 goto blad
strip KroniX_Rodzina.exe
echo.
echo Gotowe: KroniX_Rodzina.exe
pause
exit /b 0

:blad
echo.
echo [!] Blad budowania - przeczytaj komunikaty powyzej.
pause
exit /b 1

:brakgcc
echo [!] Brak kompilatora gcc. Pobierz w64devkit i rozpakuj do C:\w64devkit
pause
exit /b 1

:brakpy
echo [!] Brak Pythona. Zainstaluj Python 3 z opcja "Add python.exe to PATH"
pause
exit /b 1
