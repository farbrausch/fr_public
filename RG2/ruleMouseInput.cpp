// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "ruleMouseInput.h"
#define DIRECTINPUT_VERSION 0x800

// hack to get rid of dxguid.lib
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#include <dinput.h>
#include "sync.h"

#pragma comment(lib, "dinput8.lib")

static IDirectInput8 *dinput=0;
static IDirectInputDevice8 *dmouse=0;
static HANDLE event[3]={0, 0, 0};
static fr::cSection inputSync;

static volatile void *messageTarget=0;

static sInt mouseButtonState;
static sS32 mouseDeltaX, mouseDeltaY;
static sBool gottaPoll;

static sU32 lastPollTick=0;

static void __cdecl ruleMouseThread(void *)
{
  sBool exit=sFALSE;

  do
  {
    DWORD state = WaitForMultipleObjects(2, event, FALSE, 20);
    if (dinput && dmouse)
      dmouse->Acquire();

    switch (state)
    {
    case WAIT_OBJECT_0 + 0: // input event
      if (dinput && dmouse)
      {
        inputSync.enter();

        DIMOUSESTATE state;
        HRESULT hr = dmouse->GetDeviceState(sizeof(state), &state);

        if (SUCCEEDED(hr))
        {
          if (messageTarget)
          {
            sInt buttonState = 0;

            for (sInt i = 0; i < 4; i++)
            {
              if (state.rgbButtons[i] & 0x80)
                buttonState |= 1 << i;
            }

            mouseButtonState = buttonState;

            if (buttonState)
            {
              mouseDeltaX += state.lX;
              mouseDeltaY += state.lY;
            }

            sS32 tdif = (sS32) (GetTickCount() - lastPollTick);

            if (!gottaPoll || tdif > 75)
            {
              ::PostMessage((HWND) messageTarget, WM_RULEMOUSEINPUT, 0, 0);
              gottaPoll = sTRUE;
            }
          }
        }

        inputSync.leave();
      }
      break;

    case WAIT_OBJECT_0 + 1: // close event
      exit = sTRUE;
      break;
    }
  } while (!exit);

  SetEvent(event[2]);
}

void ruleMouseInit(void *windowHandle)
{
  if (FAILED(DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8A, (void **) &dinput, 0)))
    return;

  if (FAILED(dinput->CreateDevice(GUID_SysMouse, &dmouse, 0)))
  {
    ruleMouseClose();
    return;
  }

  if (FAILED(dmouse->SetDataFormat(&c_dfDIMouse)))
  {
    ruleMouseClose();
    return;
  }

  if (FAILED(dmouse->SetCooperativeLevel((HWND) windowHandle, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE)))
  {
    ruleMouseClose();
    return;
  }

  event[0]=CreateEvent(0, 0, 0, 0);
  event[1]=CreateEvent(0, 0, 0, 0);
  event[2]=CreateEvent(0, 0, 0, 0);

  messageTarget=0;
  mouseButtonState=0;
  mouseDeltaX=0;
  mouseDeltaY=0;
  gottaPoll=sFALSE;

  if (FAILED(dmouse->SetEventNotification(event[0])))
  {
    ruleMouseClose();
    return;
  }

  _beginthread(ruleMouseThread, 0x1000, 0);
}

void ruleMouseClose()
{
  if (dinput && dmouse && event[1] && event[2])
  {
    SetEvent(event[1]);
    WaitForSingleObject(event[2], 250);
  }

  if (event[0])
  {
    CloseHandle(event[0]);
    event[0]=0;
  }

  if (event[1])
  {
    CloseHandle(event[1]);
    event[1]=0;
  }

  if (event[2])
  {
    CloseHandle(event[2]);
    event[2]=0;
  }

  if (dmouse)
  {
		dmouse->Unacquire();
    dmouse->Release();
    dmouse=0;
  }

  if (dinput)
  {
    dinput->Release();
    dinput=0;
  }

  messageTarget=0;
}

sBool ruleMouseSetTarget(void *target)
{
  inputSync.enter();
  messageTarget=target;
  inputSync.leave();

  return dmouse != 0;
}

void *ruleMouseGetTarget()
{
  return (void *) messageTarget;
}

void ruleMousePoll(sInt &buttons, sInt &dx, sInt &dy)
{
  inputSync.enter();

  buttons=mouseButtonState;
  dx=mouseDeltaX;
  dy=mouseDeltaY;

  mouseDeltaX=0;
  mouseDeltaY=0;
  gottaPoll=sFALSE;

  lastPollTick=GetTickCount();

  inputSync.leave();
}
