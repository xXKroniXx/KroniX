/* Platforma Windows: okno GDI, klawiatura + mysz (raw input) + pad XInput, dźwięk waveOut, 60 FPS. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#undef RGB
#include "engine.h"

static HWND hwnd;
static BITMAPINFO bmi;
static int keys[256];
static int fullscreen;
static WINDOWPLACEMENT prevPlacement = {sizeof(WINDOWPLACEMENT)};
static int mouseBtn[3], wheelPulse, captured;
static int vpX, vpY, vpW = SCREEN_W, vpH = SCREEN_H; /* obszar obrazu w oknie */
static float padLookX, padLookY;

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
#define NBUF 4
#define BUFSAMP 1024
static HWAVEOUT waveOut;
static WAVEHDR hdr[NBUF];
static int16_t abuf[NBUF][BUFSAMP];
static int audioOk;

static void audio_open(void) {
  WAVEFORMATEX wf;
  memset(&wf, 0, sizeof wf);
  wf.wFormatTag = WAVE_FORMAT_PCM;
  wf.nChannels = 1;
  wf.nSamplesPerSec = AUDIO_RATE;
  wf.wBitsPerSample = 16;
  wf.nBlockAlign = 2;
  wf.nAvgBytesPerSec = AUDIO_RATE * 2;
  if (waveOutOpen(&waveOut, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) return;
  for (int i = 0; i < NBUF; i++) {
    memset(&hdr[i], 0, sizeof hdr[i]);
    hdr[i].lpData = (LPSTR)abuf[i];
    hdr[i].dwBufferLength = BUFSAMP * 2;
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

static void present(HDC dc) {
  RECT rc;
  GetClientRect(hwnd, &rc);
  int cw = rc.right, ch = rc.bottom;
  if (cw <= 0 || ch <= 0) return;
  int scale = cw / SCREEN_W < ch / SCREEN_H ? cw / SCREEN_W : ch / SCREEN_H;
  int dw, dh;
  if (scale >= 1) { dw = SCREEN_W * scale; dh = SCREEN_H * scale; }
  else if (cw * SCREEN_H < ch * SCREEN_W) { dw = cw; dh = cw * SCREEN_H / SCREEN_W; }
  else { dh = ch; dw = ch * SCREEN_W / SCREEN_H; }
  int ox = (cw - dw) / 2, oy = (ch - dh) / 2;
  vpX = ox; vpY = oy; vpW = dw; vpH = dh;
  if (ox > 0) { PatBlt(dc, 0, 0, ox, ch, BLACKNESS); PatBlt(dc, ox + dw, 0, cw - ox - dw, ch, BLACKNESS); }
  if (oy > 0) { PatBlt(dc, 0, 0, cw, oy, BLACKNESS); PatBlt(dc, 0, oy + dh, cw, ch - oy - dh, BLACKNESS); }
  SetStretchBltMode(dc, COLORONCOLOR);
  StretchDIBits(dc, ox, oy, dw, dh, 0, 0, SCREEN_W, SCREEN_H, fb, &bmi, DIB_RGB_COLORS, SRCCOPY);
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
      if (vpW > 0 && vpH > 0 && !captured) {
        g_mouse_x = (x - vpX) * SCREEN_W / vpW;
        g_mouse_y = (y - vpY) * SCREEN_H / vpH;
        if (g_mouse_x < 0 || g_mouse_y < 0 || g_mouse_x >= SCREEN_W || g_mouse_y >= SCREEN_H) g_mouse_x = g_mouse_y = -1;
      }
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
      HDC dc = BeginPaint(h, &ps);
      present(dc);
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

  game_init(__argc, __argv);

  WNDCLASSW wc;
  memset(&wc, 0, sizeof wc);
  wc.lpfnWndProc = wndproc;
  wc.hInstance = inst;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
  wc.lpszClassName = L"KroniXRodzina";
  RegisterClassW(&wc);

  RECT wa;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0);
  int scale = 1;
  while (SCREEN_W * (scale + 1) + 40 <= wa.right - wa.left && SCREEN_H * (scale + 1) + 80 <= wa.bottom - wa.top) scale++;
  RECT r = {0, 0, SCREEN_W * scale, SCREEN_H * scale};
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
  int ww = r.right - r.left, wh = r.bottom - r.top;
  hwnd = CreateWindowW(L"KroniXRodzina", L"KroniX: Rodzina", WS_OVERLAPPEDWINDOW,
                       wa.left + (wa.right - wa.left - ww) / 2, wa.top + (wa.bottom - wa.top - wh) / 2, ww, wh,
                       NULL, NULL, inst, NULL);
  ShowWindow(hwnd, show);

  memset(&bmi, 0, sizeof bmi);
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = SCREEN_W;
  bmi.bmiHeader.biHeight = -SCREEN_H;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  RAWINPUTDEVICE rid;
  rid.usUsagePage = 0x01; rid.usUsage = 0x02; rid.dwFlags = 0; rid.hwndTarget = hwnd;
  RegisterRawInputDevices(&rid, 1, sizeof rid);
  pad_init();
  audio_open();
  timeBeginPeriod(1);

  LARGE_INTEGER freq, now, next;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&next);
  LONGLONG frameTicks = freq.QuadPart / 60;

  while (!g_quit) {
    MSG m;
    while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE)) {
      if (m.message == WM_QUIT) g_quit = 1;
      TranslateMessage(&m);
      DispatchMessageW(&m);
    }
    if (g_quit) break;
    map_input();
    game_frame();
    if (g_fullscreen_toggle) { g_fullscreen_toggle = 0; toggle_fullscreen(); }
    HDC dc = GetDC(hwnd);
    present(dc);
    ReleaseDC(hwnd, dc);
    audio_pump();

    next.QuadPart += frameTicks;
    QueryPerformanceCounter(&now);
    if (now.QuadPart > next.QuadPart + frameTicks * 4) next = now; /* za duże opóźnienie: nie nadrabiaj */
    while (now.QuadPart < next.QuadPart) {
      LONGLONG left = next.QuadPart - now.QuadPart;
      if (left * 1000 / freq.QuadPart > 2) Sleep(1);
      QueryPerformanceCounter(&now);
      audio_pump();
    }
  }
  ClipCursor(NULL);
  timeEndPeriod(1);
  if (audioOk) { waveOutReset(waveOut); waveOutClose(waveOut); }
  DestroyWindow(hwnd);
  return 0;
}
