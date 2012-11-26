#include "scope.h"
#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <gl/GL.h>
#include <vector>

#pragma comment(lib, "opengl32.lib")

static const int MAXSCOPES = 16;

namespace
{
  class Scope
  {
    std::vector<float> samples;
    size_t pos;

    DWORD orig_thread_id;

    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;

    char window_title[256];
    int window_w, window_h;

  public:
    const void *unique_id;

    Scope(const void *uniq, int size, const char *title, int w, int h)
    {
      unique_id = uniq;
      samples.resize(size);
      pos = 0;
      
      orig_thread_id = 0;
      hwnd = 0;
      hdc = 0;
      hglrc = 0;

      strcpy_s(window_title, title);
      window_w = w;
      window_h = h;
    }

    ~Scope()
    {
      wglMakeCurrent(hdc, 0);
      wglDeleteContext(hglrc);
      ReleaseDC(hwnd, hdc);
      DestroyWindow(hwnd);
    }

    void submit(const float *data, size_t stride, size_t count)
    {
      delayed_init();
      assert(this_is_the_right_thread());

      size_t cur = 0;
      while (cur < count)
      {
        size_t block = min(count - cur, samples.size() - pos);
        for (size_t i=0; i < block; i++)
          samples[pos+i] = data[(cur+i) * stride];
        cur += block;
        pos = (pos + block) % samples.size();
      }
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
      delayed_init();
      assert(this_is_the_right_thread());

      wglMakeCurrent(hdc, hglrc);

      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      glEnable(GL_LINE_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glEnable(GL_BLEND);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0.0f, (float)samples.size() - 1.0f, -1.05f, 1.05f, -0.5f, 0.5f);

      glColor3ub(0, 255, 0);

      glBegin(GL_LINE_STRIP);
      for (size_t i=0; i < samples.size(); i++)
        glVertex2f((float)i, samples[i]);
      glEnd();

      glColor3ub(255, 255, 255);
      glBegin(GL_LINES);
      glVertex2f((float)pos, -100.0f);
      glVertex2f((float)pos,  100.0f);
      glEnd();

      SwapBuffers(hdc);
    }

  private:
    bool this_is_the_right_thread()
    {
      return GetCurrentThreadId() == orig_thread_id;
    }

    void delayed_init()
    {
      if (!orig_thread_id)
      {
        orig_thread_id = GetCurrentThreadId();
        init_window(window_title, window_w, window_h);
        msgloop();
      }
    }

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
        return 1;

      case WM_PAINT:
        assert(me != 0);
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
      RegisterClassA(&wc);

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

static Scope *get_scope(const void *unique_id)
{
  for (int i=0; i < MAXSCOPES; i++)
    if (scopes[i] && scopes[i]->unique_id == unique_id)
      return scopes[i];

  return 0;
}

void scopeOpen(const void *unique_id, const char *title, int nsamples, int w, int h)
{
  if (get_scope(unique_id))
    scopeClose(unique_id);

  for (int i=0; i < MAXSCOPES; i++)
  {
    if (!scopes[i])
    {
      scopes[i] = new Scope(unique_id, nsamples, title, w, h);
      break;
    }
  }
}

void scopeClose(const void *unique_id)
{
  for (int i=0; i < MAXSCOPES; i++)
  {
    if (scopes[i] && scopes[i]->unique_id == unique_id)
    {
      delete scopes[i];
      scopes[i] = 0;
    }
  }
}

bool scopeIsOpen(const void *unique_id)
{
    return get_scope(unique_id) != 0;
}

void scopeSubmit(const void *unique_id, const float *data, int nsamples)
{
  if (Scope *s = get_scope(unique_id))
    s->submit(data, 1, nsamples);
}

void scopeSubmitStrided(const void *unique_id, const float *data, int stride, int nsamples)
{
  if (Scope *s = get_scope(unique_id))
    s->submit(data, stride, nsamples);
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
