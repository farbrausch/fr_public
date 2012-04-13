// This code is in the public domain. See LICENSE for details.

#pragma once

// ruleMouseInput.h
// simple dinput mouse code

#ifndef __RG2_RULEMOUSEINPUT_H_
#define __RG2_RULEMOUSEINPUT_H_

#include "types.h"
#define WM_RULEMOUSEINPUT WM_USER+256

extern void ruleMouseInit(void *windowHandle);
extern void ruleMouseClose();
extern sBool ruleMouseSetTarget(void *target);
extern void *ruleMouseGetTarget();
extern void ruleMousePoll(sInt &buttons, sInt &dx, sInt &dy);

#endif
