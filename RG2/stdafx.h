// This code is in the public domain. See LICENSE for details.

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__FA483D3E_DDEB_498A_A510_8E13F91865F1__INCLUDED_)
#define AFX_STDAFX_H__FA483D3E_DDEB_498A_A510_8E13F91865F1__INCLUDED_

// Change these values to use different versions
#define WINVER		0x0500
#define _WIN32_IE	0x0500
#define _RICHEDIT_VER	0x0200


#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <atlmisc.h>
#include <atlddx.h>
#include <atlgdi.h>
#include <atlctrls.h>
#include <atlctrlw.h>
#include <atlctrlx.h>
#include <atlscrl.h>
#include <atldlgs.h>

#include <list>
#include <vector>
#include <map>
#include <stack>
#include <set>

#include "viewcomm.h"
#include "frControl.h"
#include "ruleMouseInput.h"

#include "types.h"
#include "directx.h"
#include "stream.h"
#include "tool.h"
#include "debug.h"
#include "math3d_2.h"
#include "sync.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__FA483D3E_DDEB_498A_A510_8E13F91865F1__INCLUDED_)
