/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_SYSTEM_PRIVATE_HPP
#define FILE_ALTONA2_LIBS_BASE_SYSTEM_PRIVATE_HPP

#include "Altona2/Libs/Base/Base.hpp"
#if sConfigPlatform==sConfigPlatformWin
#include <windows.h>
#endif

namespace Altona2 {
namespace Private {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

extern Altona2::sScreenMode CurrentMode;
extern uint KeyQual;
extern sApp *CurrentApp;
extern sDragData DragData;
extern int OldMouseButtons;
extern int NewMouseButtons;
extern int NewMouseX;
extern int NewMouseY;
extern int MouseWheelAkku;
extern float RawMouseX;
extern float RawMouseY;
extern float RawMouseStartX;
extern float RawMouseStartY;

#if sConfigPlatform==sConfigPlatformWin
extern HWND Window;

extern HCURSOR MouseCursor;
extern WNDCLASSEXW WindowClass;
extern bool WindowActive;

#if sConfigLogging
void DXError(uint err,const char *file,int line);
#define DXErr(hr) { uint err=hr; if(FAILED(err)) DXError(err,__FILE__,__LINE__); }
#else
void DXError(uint err);
#define DXErr(hr) { uint err=hr; if(FAILED(err)) DXError(err); }
#endif

#endif  // sConfigPlatformWin

#if sConfigPlatform==sConfigPlatformLinux || sConfigPlatform==sConfigPlatformIOS|| sConfigPlatform==sConfigPlatformOSX
extern bool ExitFlag;
#endif  // sConfigPlatformLinix || sConfigPlatformIOS

#if sConfigPlatform==sConfigPlatformOSX
extern  int sArgC;
extern  const char **sArgV;
#endif
  
extern sArray<sScreen *> OpenScreens;

void Render();
void sCreateWindow(const sScreenMode &);
void sRestartGfx();
void HandleMouse(int msgtime,int dbl,sScreen *scr);
void RestartGfxInit();
void RestartGfxExit();
void ExitGfx();
void DestroyAllWindows();

/****************************************************************************/
}} // namepsace

#endif  // FILE_ALTONA2_LIBS_BASE_SYSTEM_PRIVATE_HPP

