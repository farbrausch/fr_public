#include "types.h"
#include "dsio.h"

#include <Windows.h>
#include <dsound.h>

#pragma comment(lib, "dsound.lib")

static const int BUFFERLEN = 0x10000;  // bytes
//static const int BUFFERLEN = (44100/10)*4; // bytes

static struct DSData
{
  IDirectSound *dssd;             // DirectSound instance
  IDirectSoundBuffer *pbuf;       // primary buffer
  IDirectSoundBuffer *sbuf;       // secondary buffer
  DSIOCALLBACK *callback;         // callback function
  void *cbparm;                   // user parameter to callback
  HANDLE tickev;
  HANDLE exitev;
  HANDLE thndl;
  CRITICAL_SECTION crsec;
  sF32 vol;
  sU32 lastpos;                   // last pos we got
  sS32 bufcnt;                    // filled buffer count
  sS32 ltg;                       // last time got
  sF32 mixbuffer[BUFFERLEN/2];    // 32bit stereo mixing buffer
} g_dsound;

static void clamp(void *dest, const sF32 *src, sInt nsamples)
{
  sS16 *dests = (sS16 *)dest;

  for (sInt i=0; i < nsamples; i++)
  {
    sF32 v = src[i] * g_dsound.vol;
    if (v >  32600.0f) v =  32600.0f;
    if (v < -32600.0f) v = -32600.0f;
    dests[i] = (sS16)v;
  }
}

static DWORD WINAPI threadfunc(void *param)
{
  HANDLE handles[2] = { g_dsound.tickev, g_dsound.exitev };

  while (WaitForMultipleObjects(2, handles, FALSE, BUFFERLEN/3000) != WAIT_OBJECT_0 + 1) // while not exit
  {
    EnterCriticalSection(&g_dsound.crsec);

    void *buf1, *buf2;
    DWORD len1, len2;

    // fetch current buffer pos
    DWORD curpos;
    sInt nwrite = 0;
    for (;;)
    {
      HRESULT hr = g_dsound.sbuf->GetCurrentPosition(&curpos, 0);
      if (hr == S_OK)
      {
        // find out how many bytes to write
        curpos &= ~31u;
        if (curpos == g_dsound.lastpos)
          goto done; // still the same

        nwrite = curpos - g_dsound.lastpos;
        if (nwrite < 0)
          nwrite += BUFFERLEN;

        hr = g_dsound.sbuf->Lock(g_dsound.lastpos, nwrite, &buf1, &len1, &buf2, &len2, 0);
      }

      if (hr == S_OK)
        break;
      else if (hr == DSERR_BUFFERLOST)
        g_dsound.sbuf->Restore();
      else
        goto done;
    }

    // we got the lock
    g_dsound.lastpos = curpos;
    g_dsound.bufcnt += nwrite;

    // render to mix buffer
    g_dsound.callback(g_dsound.cbparm, g_dsound.mixbuffer, nwrite / 4);

    // float->int, clamp
    if (buf1)
      clamp(buf1, g_dsound.mixbuffer, len1/2);
    if (buf2)
      clamp(buf2, g_dsound.mixbuffer + len1/2, len2/2);

    g_dsound.sbuf->Unlock(buf1, len1, buf2, len2);

  done:
    LeaveCriticalSection(&g_dsound.crsec);
  }

  return 0;
}

sU32 __stdcall dsInit(DSIOCALLBACK *callback, void *parm, void *hwnd)
{
  static const WAVEFORMATEX wfxprimary = { WAVE_FORMAT_PCM, 2, 44100, 44100*2*2, 2*2, 16, 0 };
  //static const WAVEFORMATEX wfxprimary = { WAVE_FORMAT_PCM, 2, 48000, 48000*2*2, 2*2, 16, 0 };
  static const DSBUFFERDESC primdesc = { sizeof(DSBUFFERDESC), DSBCAPS_PRIMARYBUFFER, 0, 0, 0 };
  static const DSBUFFERDESC streamdesc = { sizeof(DSBUFFERDESC), DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS, BUFFERLEN, 0, (WAVEFORMATEX*)&wfxprimary };

  ZeroMemory(&g_dsound, sizeof(g_dsound));
  g_dsound.callback = callback;
  g_dsound.cbparm = parm;

  void *buf1, *buf2;
  DWORD len1, len2;

  if (DirectSoundCreate(0, &g_dsound.dssd, 0) != S_OK || // create DSound instance
    g_dsound.dssd->SetCooperativeLevel((HWND)hwnd, DSSCL_PRIORITY) != S_OK || // cooperative level
    g_dsound.dssd->CreateSoundBuffer(&primdesc, &g_dsound.pbuf, 0) != S_OK || // primary buffer
    g_dsound.dssd->CreateSoundBuffer(&streamdesc, &g_dsound.sbuf, 0) != S_OK || // secondary buffer
    g_dsound.pbuf->SetFormat(&wfxprimary) != S_OK || // set primary buf format
    g_dsound.sbuf->Lock(0, 0, &buf1, &len1, &buf2, &len2, DSBLOCK_ENTIREBUFFER) != S_OK) // lock secondary buf
    goto bad;

  // clear secondary buffer
  memset(buf1, 0, len1);
  memset(buf2, 0, len2);
  if (g_dsound.sbuf->Unlock(buf1, len1, buf2, len2) != S_OK)
    goto bad;

  g_dsound.bufcnt = -BUFFERLEN;
  g_dsound.ltg = -BUFFERLEN;

  g_dsound.tickev = CreateEvent(0, FALSE, FALSE, 0);
  g_dsound.exitev = CreateEvent(0, FALSE, FALSE, 0);
  InitializeCriticalSection(&g_dsound.crsec);

  if (g_dsound.sbuf->Play(0, 0, DSBPLAY_LOOPING) != S_OK)
    goto bad;

  dsSetVolume(1.0f);

  // start sound thread
  g_dsound.thndl = CreateThread(0, 0, threadfunc, 0, 0, &len1);
  SetThreadPriority(g_dsound.thndl, THREAD_PRIORITY_ABOVE_NORMAL);

  return 0;

bad:
  dsClose();
  return ~0u;
}

static void release(IUnknown **pcom)
{
  if (*pcom)
  {
    (*pcom)->Release();
    *pcom = 0;
  }
}

void __stdcall dsClose()
{
  if (g_dsound.thndl)
  {
    // set exit request
    SetEvent(g_dsound.exitev);
  
    // wait for thread to finish
    WaitForSingleObject(g_dsound.thndl, INFINITE);
  }

  CloseHandle(g_dsound.tickev);
  CloseHandle(g_dsound.exitev);
  CloseHandle(g_dsound.thndl);
  DeleteCriticalSection(&g_dsound.crsec);
  release((IUnknown **)&g_dsound.sbuf);
  release((IUnknown **)&g_dsound.pbuf);
  release((IUnknown **)&g_dsound.dssd);
}

sS32 __stdcall dsGetCurSmp()
{
  DWORD gppos;

  EnterCriticalSection(&g_dsound.crsec);
  g_dsound.sbuf->GetCurrentPosition(&gppos, 0);

  sInt ndiff = gppos - g_dsound.lastpos;
  if (ndiff < 0)
    ndiff += BUFFERLEN;

  if (ndiff < BUFFERLEN/4)
    g_dsound.ltg = g_dsound.bufcnt + ndiff;

  LeaveCriticalSection(&g_dsound.crsec);
  return g_dsound.ltg;
}

void __stdcall dsSetVolume(float vol)
{
  g_dsound.vol = vol * 32768.0f;
}

void __stdcall dsTick()
{
  SetEvent(g_dsound.tickev);
}

void __stdcall dsLock()
{
  EnterCriticalSection(&g_dsound.crsec);
}

void __stdcall dsUnlock()
{
  LeaveCriticalSection(&g_dsound.crsec);
}