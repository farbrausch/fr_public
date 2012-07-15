#include "scope.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <gl/GL.h>
#include <vector>

static const int MAXSCOPES = 16;

namespace
{
  class Scope
  {
    std::vector<float> samples;
    size_t pos;

    CRITICAL_SECTION csec;

    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;

  public:
    Scope(int size, const char *title, int w, int h)
    {
      samples.resize(size);
      pos = 0;

      InitializeCriticalSection(&csec);
      init_window(title, w, h);
    }

    ~Scope()
    {
      DeleteCriticalSection(&csec);
      wglDeleteContext(hglrc);
      ReleaseDC(hwnd, hdc);
      DestroyWindow(hwnd);
    }

    void submit(const float *data, size_t count)
    {
      EnterCriticalSection(&csec);
      size_t cur = 0;
      while (cur < count)
      {
        size_t block = min(count - cur, samples.size() - pos);
        memcpy(&samples[pos], &data[cur], block * sizeof(float));
        cur += block;
        pos = (pos + block) % samples.size();
      }
      LeaveCriticalSection(&csec);
    }

    void msgloop()
    {
      MSG msg;
      while (PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
      }
    }

    void render()
    {
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      glColor3ub(0, 255, 0);
      glEnable(GL_LINE_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
      glEnable(GL_BLEND);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0.0f, (float)samples.size() - 1.0f, -1.0f, 1.0f, -0.5f, 0.5f);

      EnterCriticalSection(&csec);
      glBegin(GL_LINE_STRIP);
      for (size_t i=0; i < samples.size(); i++)
        glVertex2f((float)i, samples[i]);
      glEnd();
      LeaveCriticalSection(&csec);

      SwapBuffers(hdc);
    }

  private:
    static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
      Scope *me = (Scope *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

      switch (msg)
      {
      case WM_CREATE:
        {
          CREATESTRUCT *cs = (CREATESTRUCT *)lparam;
          SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        }
        break;

      case WM_ERASEBKGND:
        return 0;

      case WM_PAINT:
        me->render();
        ValidateRect(hwnd, 0);
        return 0;
      }

      return DefWindowProcA(hwnd, msg, wparam, lparam);
    }

    void init_window(const char *title, int w, int h)
    {
      WNDCLASSA wc = { };
      wc.style = CS_OWNDC;
      wc.lpfnWndProc = wndproc;
      wc.hInstance = (HINSTANCE)GetModuleHandle(0);
      wc.hCursor = LoadCursor(0, IDC_ARROW);
      wc.lpszClassName = "debug.win.scope";
      RegisterClass(&wc);

      DWORD style = WS_CAPTION;
      RECT r = { 0, 0, w, h };
      AdjustWindowRect(&r, style, FALSE);

      hwnd = CreateWindowA("debug.win.scope", title, style, CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top, 0, 0, wc.hInstance, this);
      hdc = 0;
      hglrc = 0;

      if (hwnd)
      {
        static const PIXELFORMATDESCRIPTOR pfd = {
          sizeof(PIXELFORMATDESCRIPTOR), 1,
          PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
          PFD_TYPE_RGBA,
          32,
          0, 0, 0, 0, 0, 0, 0, 0, // RGBA bit layout ignored
          0, 0, 0, 0, 0, // accum format ignored
          0, 0, // depth/stencil ignored
          0, // aux ignored
          PFD_MAIN_PLANE,
          0, 0, 0, 0
        };

        hdc = GetDC(hwnd);
        SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd);
        hglrc = wglCreateContext(hdc);
        wglMakeCurrent(hdc, hglrc);

        ShowWindow(hwnd, SW_SHOW);
      }
    }
  };
}

static Scope *scopes[MAXSCOPES];

static bool index_valid(int index)
{
  return index >= 0 && index < MAXSCOPES;
}

void scopeOpen(int index, const char *title, int nsamples, int w, int h)
{
  if (!index_valid(index))
    return;

  scopes[index] = new Scope(nsamples, title, w, h);
}

void scopeClose(int index)
{
  if (index_valid(index) && scopes[index])
  {
    delete scopes[index];
    scopes[index] = 0;
  }
}

void scopeSubmit(int index, const float *data, int nsamples)
{
  if (index_valid(index) && scopes[index])
    scopes[index]->submit(data, nsamples);
}

void scopeUpdateAll()
{
  for (int i=0; i < MAXSCOPES; i++)
  {
    if (!scopes[i])
      continue;

    scopes[i]->msgloop();
    scopes[i]->render();
  }
}
