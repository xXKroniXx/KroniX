/* Platforma Windows: okno z kontekstem OpenGL 3.3 core (WGL), klawiatura + mysz (raw input)
 * + pad XInput, dźwięk waveOut. Logika w stałym kroku 60 Hz, rysowanie raz na klatkę ekranu. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#undef RGB
#include "engine.h"
#include "glapi.h"

static HWND hwnd;
static HDC hdc;
static HGLRC hglrc;
static int keys[256];
static int fullscreen;
static WINDOWPLACEMENT prevPlacement = {sizeof(WINDOWPLACEMENT)};
static int mouseBtn[3], wheelPulse, captured;
static int resized, clientW = 1280, clientH = 720;
static float padLookX, padLookY;

/* ---------------------------------------------------------------- OpenGL (WGL) */
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x0001
typedef HGLRC(WINAPI *CreateCtxAttribsFn)(HDC, HGLRC, const int *);
typedef BOOL(WINAPI *SwapIntervalFn)(int);
static SwapIntervalFn swapInterval;
static HMODULE glLib;

static void *win_getproc(const char *name) {
  void *p = (void *)wglGetProcAddress(name);
  if (p == NULL || p == (void *)1 || p == (void *)2 || p == (void *)3 || p == (void *)-1)
    p = (void *)GetProcAddress(glLib, name);
  return p;
}

static void fatal(const char *msg) {
  MessageBoxA(NULL, msg, "KroniX: Rodzina", MB_ICONERROR | MB_OK);
  ExitProcess(1);
}

static void gl_create(void) {
  hdc = GetDC(hwnd);
  PIXELFORMATDESCRIPTOR pfd;
  memset(&pfd, 0, sizeof pfd);
  pfd.nSize = sizeof pfd;
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cAlphaBits = 8;
  pfd.cDepthBits = 24;
  pfd.cStencilBits = 8;
  pfd.iLayerType = PFD_MAIN_PLANE;
  int pf = ChoosePixelFormat(hdc, &pfd);
  if (!pf || !SetPixelFormat(hdc, pf, &pfd)) fatal("Nie udało się ustawić formatu pikseli OpenGL.");
  HGLRC tmp = wglCreateContext(hdc);
  if (!tmp || !wglMakeCurrent(hdc, tmp)) fatal("Nie udało się utworzyć kontekstu OpenGL.\nZaktualizuj sterowniki karty graficznej.");
  glLib = LoadLibraryA("opengl32.dll");
  CreateCtxAttribsFn createAttribs = (CreateCtxAttribsFn)(void *)wglGetProcAddress("wglCreateContextAttribsARB");
  hglrc = tmp;
  if (createAttribs) {
    int at[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 3,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};
    HGLRC core = createAttribs(hdc, NULL, at);
    if (core && wglMakeCurrent(hdc, core)) { wglDeleteContext(tmp); hglrc = core; }
    else wglMakeCurrent(hdc, tmp); /* zgodność: kontekst domyślny (często i tak 3.3+) */
  }
  const char *miss = NULL;
  if (gl_load(win_getproc, &miss)) {
    char b[512];
    snprintf(b, sizeof b, "Karta graficzna nie obsługuje OpenGL 3.3 (brak funkcji %s).\n"
             "Zaktualizuj sterowniki karty graficznej.", miss ? miss : "?");
    fatal(b);
  }
  swapInterval = (SwapIntervalFn)(void *)wglGetProcAddress("wglSwapIntervalEXT");
  if (swapInterval) swapInterval(g_cfg.vsync ? 1 : 0);
}

/* ---------------------------------------------------------------- pad XInput (ładowany dynamicznie) */
typedef struct { WORD wButtons; BYTE bLeftTrigger, bRightTrigger; SHORT sThumbLX, sThumbLY, sThumbRX, sThumbRY; } XPad;
typedef struct { DWORD dwPacketNumber; XPad Gamepad; } XState;
typedef DWORD(WINAPI *XInputGetStateFn)(DWORD, XState *);
static XInputGetStateFn xinputGetState;
static int padConnected, padRetry;
static int padBtn[BTN_COUNT];

static void pad_init(void) {
  const char *dlls[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
  for (int i = 0; i < 3 && !xinputGetState; i++) {
    HMODULE m = LoadLibraryA(dlls[i]);
    if (m) xinputGetState = (XInputGetStateFn)(void *)GetProcAddress(m, "XInputGetState");
  }
}

static void pad_poll(void) {
  memset(padBtn, 0, sizeof padBtn);
  if (!xinputGetState) return;
  if (!padConnected && --padRetry > 0) return;
  XState st;
  memset(&st, 0, sizeof st);
  if (xinputGetState(0, &st) != 0) { padConnected = 0; padRetry = 120; return; }
  padConnected = 1;
  WORD b = st.Gamepad.wButtons;
  int dz = 12000;
  padBtn[BTN_UP] = (b & 0x0001) || st.Gamepad.sThumbLY > dz;
  padBtn[BTN_DOWN] = (b & 0x0002) || st.Gamepad.sThumbLY < -dz;
  padBtn[BTN_LEFT] = (b & 0x0004) || st.Gamepad.sThumbLX < -dz;
  padBtn[BTN_RIGHT] = (b & 0x0008) || st.Gamepad.sThumbLX > dz;
  padBtn[BTN_TL] = (b & 0x0004) != 0;
  padBtn[BTN_TR] = (b & 0x0008) != 0;
  padBtn[BTN_A] = (b & 0x1000) != 0;
  padBtn[BTN_B] = (b & 0x2000) || (b & 0x0010);
  padBtn[BTN_SL] = st.Gamepad.sThumbLX < -dz;
  padBtn[BTN_SR] = st.Gamepad.sThumbLX > dz;
  padBtn[BTN_RUN] = (b & 0x0040) != 0;                 /* wciśnięcie lewej gałki */
  padBtn[BTN_FIRE] = st.Gamepad.bRightTrigger > 60;
  padBtn[BTN_RELOAD] = (b & 0x4000) != 0;              /* X */
  padBtn[BTN_WNEXT] = (b & 0x8000) != 0;               /* Y */
  padBtn[BTN_TAB] = (b & 0x0020) != 0;                 /* Back */
  padBtn[BTN_N] = (b & 0x0200) != 0;                   /* RB */
  int rx = st.Gamepad.sThumbRX, ry = st.Gamepad.sThumbRY;
  padLookX = (rx > 8000 || rx < -8000) ? rx / 32768.0f * 9.0f : 0;
  padLookY = (ry > 8000 || ry < -8000) ? -ry / 32768.0f * 6.0f : 0;
}

/* ---------------------------------------------------------------- dźwięk */
#define NBUF 5
#define BUFSAMP 1024 /* ramek stereo */
static HWAVEOUT waveOut;
static WAVEHDR hdr[NBUF];
static int16_t abuf[NBUF][BUFSAMP * 2];
static int audioOk;

static void audio_open(void) {
  WAVEFORMATEX wf;
  memset(&wf, 0, sizeof wf);
  wf.wFormatTag = WAVE_FORMAT_PCM;
  wf.nChannels = 2;
  wf.nSamplesPerSec = AUDIO_RATE;
  wf.wBitsPerSample = 16;
  wf.nBlockAlign = 4;
  wf.nAvgBytesPerSec = AUDIO_RATE * 4;
  if (waveOutOpen(&waveOut, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) return;
  for (int i = 0; i < NBUF; i++) {
    memset(&hdr[i], 0, sizeof hdr[i]);
    hdr[i].lpData = (LPSTR)abuf[i];
    hdr[i].dwBufferLength = BUFSAMP * 4;
    waveOutPrepareHeader(waveOut, &hdr[i], sizeof(WAVEHDR));
    hdr[i].dwFlags |= WHDR_DONE;
  }
  audioOk = 1;
}

static void audio_pump(void) {
  if (!audioOk) return;
  for (int i = 0; i < NBUF; i++)
    if (hdr[i].dwFlags & WHDR_DONE) {
      audio_render(abuf[i], BUFSAMP);
      waveOutWrite(waveOut, &hdr[i], sizeof(WAVEHDR));
    }
}

/* ---------------------------------------------------------------- okno */
static void toggle_fullscreen(void) {
  DWORD style = (DWORD)GetWindowLongA(hwnd, GWL_STYLE);
  if (!fullscreen) {
    MONITORINFO mi = {sizeof(mi)};
    if (GetWindowPlacement(hwnd, &prevPlacement) && GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
      SetWindowLongA(hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
      SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                   mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
  } else {
    SetWindowLongA(hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(hwnd, &prevPlacement);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
  fullscreen = !fullscreen;
}

static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_CLOSE: g_quit = 1; return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_KILLFOCUS: memset(keys, 0, sizeof keys); memset(mouseBtn, 0, sizeof mouseBtn); break;
    case WM_INPUT: {
      RAWINPUT ri;
      UINT sz = sizeof ri;
      if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, &ri, &sz, sizeof(RAWINPUTHEADER)) != (UINT)-1 && ri.header.dwType == RIM_TYPEMOUSE) {
        if (!(ri.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) && captured) {
          g_mouse_dx += (float)ri.data.mouse.lLastX;
          g_mouse_dy += (float)ri.data.mouse.lLastY;
        }
      }
      break;
    }
    case WM_MOUSEMOVE: {
      int x = (short)LOWORD(lp), y = (short)HIWORD(lp);
      if (g_uiScale > 0 && !captured) {
        g_mouse_x = (int)((x - g_uiOffX) / g_uiScale);
        g_mouse_y = (int)((y - g_uiOffY) / g_uiScale);
        if (g_mouse_x < 0 || g_mouse_y < 0 || g_mouse_x >= UI_W || g_mouse_y >= UI_H) g_mouse_x = g_mouse_y = -1;
      }
      return 0;
    }
    case WM_SIZE: {
      int w = LOWORD(lp), h = HIWORD(lp);
      if (w > 0 && h > 0 && (w != clientW || h != clientH)) { clientW = w; clientH = h; resized = 1; }
      return 0;
    }
    case WM_LBUTTONDOWN: mouseBtn[0] = 1; SetCapture(h); return 0;
    case WM_LBUTTONUP: mouseBtn[0] = 0; ReleaseCapture(); return 0;
    case WM_RBUTTONDOWN: mouseBtn[1] = 1; return 0;
    case WM_RBUTTONUP: mouseBtn[1] = 0; return 0;
    case WM_MOUSEWHEEL: wheelPulse = 2; return 0;
    case WM_SETCURSOR:
      if (LOWORD(lp) == HTCLIENT) { SetCursor(NULL); return TRUE; }
      break;
    case WM_SYSKEYDOWN:
      if (wp == VK_RETURN && (lp & (1 << 29))) { toggle_fullscreen(); return 0; }
      if (wp == VK_F4) { g_quit = 1; return 0; }
      if (wp == VK_F10) return 0;
      break;
    case WM_KEYDOWN:
      if (wp == VK_F11) { if (!(lp & (1 << 30))) toggle_fullscreen(); return 0; }
      if (wp < 256) keys[wp] = 1;
      return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
      if (wp < 256) keys[wp] = 0;
      if (msg == WM_SYSKEYUP) break;
      return 0;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      BeginPaint(h, &ps);
      EndPaint(h, &ps);
      return 0;
    }
    case WM_ERASEBKGND: return 1;
  }
  return DefWindowProcW(h, msg, wp, lp);
}

/* przechwytywanie myszy w trybie FPP: kursor ukryty i zamknięty w oknie */
static void update_capture(void) {
  int want = g_want_mouse_capture && GetForegroundWindow() == hwnd;
  if (want) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    POINT a = {rc.left, rc.top}, b = {rc.right, rc.bottom};
    ClientToScreen(hwnd, &a); ClientToScreen(hwnd, &b);
    RECT clip = {a.x, a.y, b.x, b.y};
    ClipCursor(&clip);
    SetCursorPos((a.x + b.x) / 2, (a.y + b.y) / 2);
    if (!captured) { g_mouse_dx = g_mouse_dy = 0; }
    captured = 1;
    g_mouse_x = g_mouse_y = -1;
  } else if (captured) {
    ClipCursor(NULL);
    captured = 0;
  }
}

static void map_input(void) {
  pad_poll();
  update_capture();
  g_btn_raw[BTN_UP] = keys[VK_UP] || keys['W'] || padBtn[BTN_UP];
  g_btn_raw[BTN_DOWN] = keys[VK_DOWN] || keys['S'] || padBtn[BTN_DOWN];
  g_btn_raw[BTN_LEFT] = keys[VK_LEFT] || keys['A'] || padBtn[BTN_LEFT];
  g_btn_raw[BTN_RIGHT] = keys[VK_RIGHT] || keys['D'] || padBtn[BTN_RIGHT];
  g_btn_raw[BTN_SL] = keys['A'] || padBtn[BTN_SL];
  g_btn_raw[BTN_SR] = keys['D'] || padBtn[BTN_SR];
  g_btn_raw[BTN_TL] = keys[VK_LEFT] || padBtn[BTN_TL];
  g_btn_raw[BTN_TR] = keys[VK_RIGHT] || padBtn[BTN_TR];
  g_btn_raw[BTN_A] = keys[VK_RETURN] || keys[VK_SPACE] || keys['E'] || padBtn[BTN_A];
  g_btn_raw[BTN_B] = keys[VK_ESCAPE] || keys[VK_BACK] || padBtn[BTN_B] || mouseBtn[1];
  g_btn_raw[BTN_RUN] = keys[VK_SHIFT] || padBtn[BTN_RUN];
  g_btn_raw[BTN_FIRE] = mouseBtn[0] || keys[VK_CONTROL] || padBtn[BTN_FIRE];
  g_btn_raw[BTN_RELOAD] = keys['R'] || padBtn[BTN_RELOAD];
  g_btn_raw[BTN_TAB] = keys[VK_TAB] || padBtn[BTN_TAB];
  g_btn_raw[BTN_W1] = keys['1'];
  g_btn_raw[BTN_W2] = keys['2'];
  g_btn_raw[BTN_W3] = keys['3'];
  g_btn_raw[BTN_W4] = keys['4'];
  g_btn_raw[BTN_WNEXT] = wheelPulse > 0 || keys['Q'] || padBtn[BTN_WNEXT];
  if (wheelPulse > 0) wheelPulse--;
  g_btn_raw[BTN_N] = keys['N'] || padBtn[BTN_N];
  if (padLookX != 0 || padLookY != 0) { g_mouse_dx += padLookX; g_mouse_dy += padLookY; }
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
  (void)prev; (void)cmd;
  SetProcessDPIAware();

  char exe[MAX_PATH];
  DWORD n = GetModuleFileNameA(NULL, exe, MAX_PATH);
  if (n > 0 && n < MAX_PATH) {
    char *slash = strrchr(exe, '\\');
    if (slash) slash[1] = 0;
    snprintf(g_data_dir, sizeof g_data_dir, "%s", exe);
  }

  WNDCLASSW wc;
  memset(&wc, 0, sizeof wc);
  wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = wndproc;
  wc.hInstance = inst;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszClassName = L"KroniXRodzina";
  RegisterClassW(&wc);

  /* okno ~80% obszaru roboczego, proporcje 16:9 */
  RECT wa;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0);
  int aw = wa.right - wa.left, ah = wa.bottom - wa.top;
  int cw = aw * 85 / 100, ch = cw * 9 / 16;
  if (ch > ah * 85 / 100) { ch = ah * 85 / 100; cw = ch * 16 / 9; }
  RECT r = {0, 0, cw, ch};
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
  int ww = r.right - r.left, wh = r.bottom - r.top;
  hwnd = CreateWindowW(L"KroniXRodzina", L"KroniX: Rodzina", WS_OVERLAPPEDWINDOW,
                       wa.left + (aw - ww) / 2, wa.top + (ah - wh) / 2, ww, wh, NULL, NULL, inst, NULL);
  if (!hwnd) fatal("Nie udało się utworzyć okna.");
  gl_create();
  RECT cr;
  GetClientRect(hwnd, &cr);
  clientW = cr.right > 0 ? cr.right : cw;
  clientH = cr.bottom > 0 ? cr.bottom : ch;
  g_winW = clientW; g_winH = clientH;

  /* ekran ładowania (generowanie tekstur i modeli trwa chwilę) */
  ShowWindow(hwnd, show);
  UpdateWindow(hwnd);
  glViewport(0, 0, clientW, clientH);
  glClearColor(0.02f, 0.018f, 0.015f, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  SwapBuffers(hdc);

  g_render = 1;
  game_init(__argc, __argv);
  if (g_glError[0]) {
    char b[1400];
    snprintf(b, sizeof b, "Błąd shaderów karty graficznej:\n\n%s", g_glError);
    fatal(b);
  }
  if (swapInterval) swapInterval(g_cfg.vsync ? 1 : 0);

  RAWINPUTDEVICE rid;
  rid.usUsagePage = 0x01; rid.usUsage = 0x02; rid.dwFlags = 0; rid.hwndTarget = hwnd;
  RegisterRawInputDevices(&rid, 1, sizeof rid);
  pad_init();
  audio_open();
  timeBeginPeriod(1);

  LARGE_INTEGER freq, now, last;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&last);
  const double step = 1.0 / 60.0;
  double acc = step;

  while (!g_quit) {
    MSG m;
    while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE)) {
      if (m.message == WM_QUIT) g_quit = 1;
      TranslateMessage(&m);
      DispatchMessageW(&m);
    }
    if (g_quit) break;
    if (IsIconic(hwnd)) { Sleep(30); QueryPerformanceCounter(&last); audio_pump(); continue; }
    if (resized) { resized = 0; r_resize(clientW, clientH); }

    QueryPerformanceCounter(&now);
    acc += (double)(now.QuadPart - last.QuadPart) / (double)freq.QuadPart;
    last = now;
    if (acc > step * 6) acc = step * 6; /* po zacięciu nie nadrabiaj w nieskończoność */
    int steps = 0;
    while (acc >= step) {
      map_input();
      game_frame();
      acc -= step;
      steps++;
      if (g_fullscreen_toggle) { g_fullscreen_toggle = 0; toggle_fullscreen(); }
    }
    audio_pump();
    if (captured) game_look(); /* obrót myszą w każdej klatce ekranu — bez szarpania */
    g_alpha = (float)(acc / step);
    (void)steps;
    game_draw();   /* z vsync SwapBuffers czeka na odświeżenie ekranu: rysujemy w tempie monitora */
    SwapBuffers(hdc);
    if (!g_cfg.vsync) {
      /* bez vsync: ogranicz do ~144 FPS, żeby nie palić procesora */
      QueryPerformanceCounter(&now);
      double spent = (double)(now.QuadPart - last.QuadPart) / (double)freq.QuadPart;
      if (spent < 1.0 / 144) Sleep((DWORD)((1.0 / 144 - spent) * 1000));
    }
    audio_pump();
  }
  ClipCursor(NULL);
  timeEndPeriod(1);
  if (audioOk) { waveOutReset(waveOut); waveOutClose(waveOut); }
  wglMakeCurrent(NULL, NULL);
  if (hglrc) wglDeleteContext(hglrc);
  DestroyWindow(hwnd);
  return 0;
}
