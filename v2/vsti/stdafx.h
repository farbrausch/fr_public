// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__213CE34F_CC77_11D4_9322_00A0CC20D58C__INCLUDED_)
#define AFX_STDAFX_H__213CE34F_CC77_11D4_9322_00A0CC20D58C__INCLUDED_

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NON_CONFORMING_SWPRINTFS

// Change these values to use different versions
#define WINVER		0x0400
#define _WIN32_IE	0x0400
#define _RICHEDIT_VER	0x0100
#define _WTL_NO_CSTRING // we use CString from atlstr.h instead (can tokenize, see MSDN)
//#define _ATL_NO_COMMODULE

#include <windows.h>
#include <mmsystem.h>

#include <atlbase.h>
#include <atlstr.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

#include <atlcrack.h>
#include <atlctrls.h>
#include <atlctrlw.h>
#include <atlctrlx.h>
#include <atldlgs.h>
#include <atlframe.h>
#include <atlgdi.h>
#include <atlres.h>
#include <atlscrl.h>
#include <atlsplit.h>
#include <gdiplus.h>
#include <atlmisc.h>

// don't we love precompiled headers?
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>

#include <string>
#include <windows.h>
#include "console.h"
#include "vst2/audioeffectx.h"
#include "vst2/AEffEditor.hpp"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__213CE34F_CC77_11D4_9322_00A0CC20D58C__INCLUDED_)
