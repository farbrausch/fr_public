/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"

#if sConfigRender==sConfigRenderGL2 || sConfigRender==sConfigRenderGLES2

#define GL_SAMPLER_EXTERNAL_OES 0x8D66
#define GL_TEXTURE_EXTERNAL_OES 0x8D65

// extensions used:
//   GL_EXT_bgra (BGRA_EXT)
//   GL_ARB_vertex_buffer_object (MapBuffer, UnmapBuffer)
//   GL_ARB_instanced_arrays (VertexAttribDivisor, DrawElementsInstanced)
//   GL_APPLE_texture_max_level (TEXTURE_MAX_LEVEL)

#if sConfigRender==sConfigRenderGLES2
#define sGLES 1
#else
#define sGLES 0
#endif

namespace Altona2 {

/****************************************************************************/

namespace Private
{
    bool RestartFlag;
    sString<256> GlVendor;
    sString<256> GlRenderer;
    sString<256> GlVersion;
    sString<32> GlShaderVersion;

    sAdapter *DefaultAdapter;
    sScreen *DefaultScreen;
    sContext *DefaultContext;

    void sRestartGfxInit();
    void sRestartGfxExit();
    
    void sSetSysFramebuffer(unsigned int name);
    void sUpdateScreenSize(int sx,int sy);

    int GlLastTexture;
    int GlLastAttribute;
    int GlLastMask;
    int GlLastFlags;
    uint GlLastBlendCache;
    uint GlRtFrameBuffer;    
    uint GlSysFrameBuffer = 0;
    bool InsideBeginTarget;
    
    bool GlOESEGLImageExternal = false;

#if sConfigPlatform == sConfigPlatformIOS || sConfigPlatform == sConfigPlatformAndroid
    bool GLOESElementIndexUint = false; //will be check
#else
    bool GLOESElementIndexUint = true;  //always true
#endif
    int sSupportedFormats[256] = {0};    	
} // namepsace

    /****************************************************************************/

#if sConfigPlatform == sConfigPlatformWin
#include "Altona2/Libs/Base/GraphicsGlew.hpp"
namespace Private
{
    HGLRC Glrc;
    HDC Gdc;
}
#endif

#if sConfigPlatform == sConfigPlatformOSX
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#define glClearDepthf glClearDepth
#endif

#if sConfigPlatform == sConfigPlatformLinux
#include <X11/X.h>
#include <X11/Xlib.h>
#include "Altona2/Libs/Base/GraphicsGlew.hpp"

namespace Private
{
    Display                 *dpy;
    Window                  root;
    GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo             *vi;
    Colormap                cmap;
    XSetWindowAttributes    swa;
    Window                  win;
    GLXContext              glc;
    XWindowAttributes       gwa;
    XEvent                  xev;
}

#endif

#if sConfigPlatform == sConfigPlatformIOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#define GL_WRITE_ONLY GL_WRITE_ONLY_OES
#define GL_TEXTURE_MAX_LEVEL GL_TEXTURE_MAX_LEVEL_APPLE
#define glMapBuffer glMapBufferOES
#define glUnmapBuffer glUnmapBufferOES

#define GL_COMPRESSED_RGB8_ETC1	0x8d64
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278

#endif

#if sConfigPlatform == sConfigPlatformAndroid
#define GL_GLEXT_PROTOTYPES

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define GL_COMPRESSED_RGB8_ETC1	0x8d64
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278

#define GL_WRITE_ONLY GL_WRITE_ONLY_OES
//#define glMapBuffer glMapBufferOES
//#define glUnmapBuffer glUnmapBufferOES
#endif

namespace Private
{
    sArray<GLsizei> DrawRangeCount;
    sArray<GLint> DrawRangeVertex;
    sArray<GLvoid *> DrawRangeIndex;

    void InitGL();
    void ExitGL();
    void InitGLCommon();
    void ExitGLCommon();
}
using namespace Private;


#if sConfigPlatform == sConfigPlatformIOS || sConfigPlatform == sConfigPlatformAndroid || sConfigPlatform == sConfigPlatformOSX

void Private::sRestartGfxInit()
{
    sLogF("gfx","restart gfx init %dx%d",CurrentMode.SizeX,CurrentMode.SizeY);
    DefaultScreen->ScreenMode.SizeX = CurrentMode.SizeX;
    DefaultScreen->ScreenMode.SizeY = CurrentMode.SizeY;
    DefaultScreen->ColorBuffer = new sResource(DefaultAdapter,sResPara(sRBM_ColorTarget|sRU_Gpu|sRM_Texture,sRF_BGRA8888,sRES_Internal,CurrentMode.SizeX,CurrentMode.SizeY,0,0));
    DefaultScreen->DepthBuffer = new sResource(DefaultAdapter,sResPara(sRBM_DepthTarget|sRU_Gpu|sRM_Texture,sRF_D24S8,sRES_Internal,CurrentMode.SizeX,CurrentMode.SizeY,0,0));
    RestartFlag = 0;
}

void Private::sRestartGfxExit()
{
    sLogF("gfx","restart gfx exit");
    delete DefaultScreen->DepthBuffer;
    delete DefaultScreen->ColorBuffer;
}

void Private::InitGL()
{
    sLogF("gfx","enter InitGL");
    DefaultAdapter = new sAdapter();
    DefaultScreen = new sScreen(CurrentMode);
    DefaultContext = new sContext();
    DefaultAdapter->ImmediateContext = DefaultContext;
    DefaultContext->Adapter = DefaultAdapter;
    DefaultScreen->Adapter = DefaultAdapter;
    DefaultAdapter->Init();
    InitGLCommon();
}

void Private::ExitGL()
{
    ExitGLCommon();

    delete DefaultAdapter;
    delete DefaultScreen;    
    DefaultAdapter = 0;
    DefaultScreen = 0;
}

void Private::sSetSysFramebuffer(unsigned int name)
{
	GlSysFrameBuffer = name;	//Only called on IOS / Android (maybe) port
}
        
#endif

void Private::sUpdateScreenSize(int sx,int sy)
{
    CurrentMode.SizeX = sx;
    CurrentMode.SizeY = sy;
    sLogF("gfx","new screen size %d %d",sx,sy);
    RestartFlag = 1;
}


/****************************************************************************/

#if sConfigLogging

static const char *GetGLErrMsg(uint err)
{
    const char *em = 0;
    switch (err)
    {
        case GL_INVALID_ENUM :
            em = "GL_INVALID_ENUM - An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_INVALID_VALUE :
            em = "GL_INVALID_VALUE - A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_INVALID_OPERATION :
            em = "GL_INVALID_OPERATION - The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION :
            em = "GL_INVALID_FRAMEBUFFER_OPERATION - The command is trying to render to or read from the framebuffer while the currently bound framebuffer is not framebuffer complete";
            break;
        case GL_OUT_OF_MEMORY :
            em = "GL_OUT_OF_MEMORY - There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
            break;
        default:
            em = "Unknown Error";
            break;
    }
    return em;    
}
    
static void GLError(uint err,const char *file,int line)
{
    if(err!=0)
    {
      sDPrintF("%s(%d): error %08x (%d)\n",file,line,err,err&0x3fff);
      sDPrintF("%s\n",GetGLErrMsg(err));
    }
}
#define GLErr(hr) { if(!(hr)) sDPrintF("%s(%d): gl null handle\n",__FILE__,__LINE__); }
#define GLERR() { int err=glGetError(); if(err) GLError(err,__FILE__,__LINE__); }

#else

#define GLErr(hr)
#define GLERR()

#endif


/****************************************************************************/
/***                                                                      ***/
/***   Windows Message Loop Modifications                                 ***/
/***                                                                      ***/
/****************************************************************************/

#if sConfigPlatform == sConfigPlatformWin

void Private::sCreateWindow(const sScreenMode &sm)
{
    RECT r2;
    r2.left = r2.top = 0;
    r2.right = sm.SizeX; 
    r2.bottom = sm.SizeY;
    AdjustWindowRect(&r2,WS_OVERLAPPEDWINDOW,FALSE);

    uint16 titlebuffer[sFormatBuffer];
    sUTF8toUTF16(sm.WindowTitle,titlebuffer,sCOUNTOF(titlebuffer));

    Window = CreateWindowW(L"Altona2",(LPCWSTR) titlebuffer, 
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,r2.right-r2.left,r2.bottom-r2.top,
        GetDesktopWindow(), NULL, WindowClass.hInstance, NULL );
}

void Private::Render()
{
    sPreFrameHook.Call();
    {
        sZONE("render",0xff004000);
        if(RestartFlag)
        {
            sRestartGfxExit();
            sRestartGfxInit();
        }
        CurrentApp->OnFrame();
        sBeginFrameHook.Call();
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE)
            CurrentApp->OnPaint();

        sEndFrameHook.Call();
        if(sGC) sGC->CollectIfTriggered();
        {
            sZONE("flip",0xff400000);
            SwapBuffers(Gdc);
            glFlush();
        }
        sUpdateGfxStats();
    }
}

void Private::sRestartGfxInit()
{
    DefaultScreen->ScreenMode.SizeX = CurrentMode.SizeX;
    DefaultScreen->ScreenMode.SizeY = CurrentMode.SizeY;
    DefaultScreen->ColorBuffer = new sResource(DefaultAdapter,sResPara(sRBM_ColorTarget|sRU_Gpu|sRM_Texture,sRF_BGRA8888,sRES_Internal,CurrentMode.SizeX,CurrentMode.SizeY,0,0));
    DefaultScreen->DepthBuffer = new sResource(DefaultAdapter,sResPara(sRBM_DepthTarget|sRU_Gpu|sRM_Texture,sRF_D24S8,sRES_Internal,CurrentMode.SizeX,CurrentMode.SizeY,0,0));
	
	RestartFlag = 0;
}
void Private::sRestartGfxExit()
{
    sDelete(DefaultScreen->ColorBuffer);
    sDelete(DefaultScreen->DepthBuffer);
}

void Private::sRestartGfx()
{
    RestartFlag = 1;
}

void Private::InitGL()
{
    Gdc = GetDC(Window);
    GLErr(Gdc);

    PIXELFORMATDESCRIPTOR pfd;    
    sClear(pfd);
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER ;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int format;
    format = ChoosePixelFormat(Gdc,&pfd);
    GLErr(format);
    GLErr(SetPixelFormat(Gdc,format,&pfd));
    Glrc = wglCreateContext(Gdc);
    GLErr(Glrc);
    GLErr(wglMakeCurrent(Gdc,Glrc));

    if(glewInit()!=GLEW_OK)
        sFatal("glew not initialized");
    if(!GLEW_VERSION_2_1!=0)
        sFatal("require opengl 2.1 at least");

    DefaultAdapter = new sAdapter();
    DefaultScreen = 0;
    DefaultContext = new sContext();
    DefaultAdapter->ImmediateContext = DefaultContext;
    DefaultContext->Adapter = DefaultAdapter;
    DefaultAdapter->Init();

    InitGLCommon();
}

void Private::ExitGL()
{
    ExitGLCommon();

    sDelete(DefaultScreen->DepthBuffer);
    sDelete(DefaultScreen->ColorBuffer);
    sDelete(DefaultScreen);
    sDelete(DefaultContext);
    sDelete(DefaultAdapter);

    wglMakeCurrent(0,0);
    wglDeleteContext(Glrc);
    ReleaseDC(Window,Gdc);
}

#endif

/****************************************************************************/

sScreenPrivate::sScreenPrivate(const sScreenMode &sm)
{
//    sASSERT(DefaultScreen==0);
//    sASSERT(DefaultAdapter!=0);
    if (!DefaultScreen) DefaultScreen = (sScreen *)this;

    ColorBuffer = 0;
    DepthBuffer = 0;
    Adapter = DefaultAdapter;
    ScreenWindow = 0;
    ScreenMode = sm;
    if(!ScreenMode.ColorFormat) ScreenMode.ColorFormat = sRF_BGRA8888;
    if(!ScreenMode.DepthFormat) ScreenMode.DepthFormat = sRF_D24S8;
}

sScreenPrivate::~sScreenPrivate()
{
//    sASSERT(DefaultScreen==this);
    if (DefaultScreen==this) DefaultScreen = 0;
}


void sScreen::WindowControl(WindowControlEnum mode)
{
#if sConfigPlatform == sConfigPlatformWin
    switch(mode)
    {
    case sWC_Hide:
        ShowWindow((HWND) ScreenWindow,SW_HIDE);
        break;
    case sWC_Normal:
        ShowWindow((HWND) ScreenWindow,SW_SHOWNORMAL);
        break;
    case sWC_Minimize:
        ShowWindow((HWND) ScreenWindow,SW_MINIMIZE);
        break;
    case sWC_Maximize:
        ShowWindow((HWND) ScreenWindow,SW_MAXIMIZE);
        break;
    }
#endif
}
  
void sScreen::SetTitle(const char *title)
{
}

sResource *sScreen::GetScreenColor()
{
  return ColorBuffer;
}

sResource *sScreen::GetScreenDepth()
{
  return DepthBuffer;
}

/****************************************************************************/

int sGetDisplayCount()
{
    return 1;
}

void sGetDisplayInfo(int display,sDisplayInfo &info)
{
  
  info.DisplayNumber = 0;
  info.AdapterNumber = 0;
  info.DisplayInAdapterNumber = 0;
  info.AdapterName = "Main";
  info.MonitorName = "Asus";
  info.SizeX = 1920;
  info.SizeY = 1080;      
}

/****************************************************************************/

#if sConfigPlatform == sConfigPlatformLinux

const static uint XKeys[][2] = 
{
    XK_Tab, sKEY_Tab, 
    XK_Return, sKEY_Enter,
    XK_Pause, sKEY_Pause,
    XK_Scroll_Lock, sKEY_Scroll,
    XK_Escape, sKEY_Escape,
    XK_Home, sKEY_Home,
    XK_End, sKEY_End,
    XK_Page_Up, sKEY_PageUp,
    XK_Page_Down, sKEY_PageDown,
    XK_Left, sKEY_Left,
    XK_Right, sKEY_Right,
    XK_Up, sKEY_Up,
    XK_Down, sKEY_Down,
    XK_Print, sKEY_Print,
    XK_Insert, sKEY_Insert,
    XK_Menu, sKEY_WinM,
    XK_Num_Lock, sKEY_Numlock,
    XK_F1, sKEY_F1,
    XK_F2, sKEY_F2,
    XK_F3, sKEY_F3,
    XK_F4, sKEY_F4,
    XK_F5, sKEY_F5,
    XK_F6, sKEY_F6,
    XK_F7, sKEY_F7,
    XK_F8, sKEY_F8,
    XK_F9, sKEY_F9,
    XK_F10, sKEY_F10,
    XK_F11, sKEY_F11,
    XK_F12, sKEY_F12,
    XK_Shift_L, sKEY_Shift,
    XK_Shift_R, sKEY_Shift,
    XK_Control_L, sKEY_Ctrl,
    XK_Control_R, sKEY_Ctrl,
    //  XK_Alt_L, sKEY_Alt,
    //  XK_Alt_R, sKEY_AltGr,
    XK_Super_L, sKEY_WinL,
    XK_Super_R, sKEY_WinR,
    XK_Delete, sKEY_Delete,
    0 ,0

    //  XK_ , sKEY_,
};

void Private::InitGL()
{
    dpy = XOpenDisplay(NULL);
    if(!dpy) sFatal("cannot connect to X server");
    root = DefaultRootWindow(dpy);
    vi = glXChooseVisual(dpy, 0, att);
    if(!vi) sFatal("no appropriate visual found");
    cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PointerMotionMask;
    win = XCreateWindow(dpy, root, 0, 0, CurrentMode.SizeX, CurrentMode.SizeY, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "altona2");
    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);

    if(glewInit()!=GLEW_OK)
        sFatal("glew not initialized");
    if(!GLEW_VERSION_2_1!=0)
        sFatal("require opengl 2.1 at least");

    DefaultAdapter = new sAdapter();
    DefaultScreen = 0;
    DefaultContext = new sContext();
    DefaultAdapter->ImmediateContext = DefaultContext;
    DefaultContext->Adapter = DefaultAdapter;
    DefaultAdapter->Init();

    InitGLCommon();
}

void sRenderGL();

void XRender()
{
    sRenderGL();

    //  glFlush();
    glXSwapBuffers(dpy, win); 
    sUpdateGfxStats();
    // sDPrintF(".\n");
}

void XMainLoop()
{
    sLogF("sys","start main loop");

    static const int mbkey[] = 
    { 0,sKEY_LMB,sKEY_MMB,sKEY_RMB,sKEY_WheelUp,sKEY_WheelDown,0,0,sKEY_Extra1,sKEY_Extra2 };
    static const uint mbbit[] = 
    { 0,sMB_Left,sMB_Middle,sMB_Right,0,0,0,0,sMB_Extra1,sMB_Extra2 };


    while(!ExitFlag) 
    {
        while(XPending(dpy)>0)
        {
            XNextEvent(dpy, &xev);

            uint isbreak = 0;
            switch(xev.type)
            {
            case Expose:
                XRender();
                break;

            case ConfigureNotify:
                if(xev.xconfigure.width  != CurrentMode.SizeX ||
                    xev.xconfigure.height != CurrentMode.SizeY)
                {
                    RestartFlag = 1;
                }
                break;

            case MotionNotify:
                NewMouseX = xev.xmotion.x;
                NewMouseY = xev.xmotion.y;
                HandleMouse(xev.xmotion.time,0,sGetScreen());
                break;

            case ButtonRelease:
                isbreak = sKEYQ_Break;
            case ButtonPress:
                {
                    int button = xev.xbutton.button;
                    NewMouseX = xev.xmotion.x;
                    NewMouseY = xev.xmotion.y;
                    if(button>=0 && button<sCOUNTOF(mbkey))
                    {
                        if(isbreak)
                            NewMouseButtons &= ~mbbit[button];
                        else
                            NewMouseButtons |= mbbit[button];

                        if(mbkey[button])
                        {
                            sKeyData kd;
                            kd.Key = mbkey[button] | KeyQual | isbreak;
                            kd.MouseX = NewMouseX;
                            kd.MouseY = NewMouseY;
                            kd.Timestamp = xev.xbutton.time;
                            CurrentApp->OnKey(kd);            
                        }
                    }

                    HandleMouse(xev.xbutton.time,0,sGetScreen());
                }  
                break;

            case KeyRelease:
                isbreak = sKEYQ_Break;
            case KeyPress:
                {
                    static XComposeStatus state[2];
                    uint mask = isbreak ? 0 : ~0U;
                    char buffer[10];
                    KeySym sym;

                    xev.xkey.state &= ~ControlMask;

                    int count = XLookupString(&xev.xkey,buffer,sCOUNTOF(buffer)-1,&sym,&state[isbreak?1:0]);
                    buffer[count] = 0;

                    switch(sym)
                    {
                    case XK_Shift_L:   KeyQual &= ~sKEYQ_ShiftL; KeyQual |= sKEYQ_ShiftL & mask; break;
                    case XK_Shift_R:   KeyQual &= ~sKEYQ_ShiftR; KeyQual |= sKEYQ_ShiftR & mask; break;
                    case XK_Control_L: KeyQual &= ~sKEYQ_CtrlL;  KeyQual |= sKEYQ_CtrlL  & mask; break;
                    case XK_Control_R: KeyQual &= ~sKEYQ_CtrlR;  KeyQual |= sKEYQ_CtrlR  & mask; break;
                    case XK_Alt_L:     KeyQual &= ~sKEYQ_Alt;    KeyQual |= sKEYQ_Alt    & mask; break;
                    case XK_Alt_R:     KeyQual &= ~sKEYQ_AltGr;  KeyQual |= sKEYQ_AltGr  & mask; break;
                    }

                    sKeyData kd;
                    kd.MouseX = NewMouseX;
                    kd.MouseY = NewMouseY;
                    kd.Timestamp = xev.xkey.time;
                    bool found = 0;
                    for(int i=0;XKeys[i][0];i++)
                    {
                        if(XKeys[i][0]==sym)
                        {
                            found = 1;
                            kd.Key = XKeys[i][1] | KeyQual | isbreak;            
                            CurrentApp->OnKey(kd);
                            break;
                        }
                    }
                    if(!found)
                    {
                        const char *s = buffer;
                        while(*s)
                        {
                            // i thought this is utf, but its not!
                            kd.Key = (*s++)&0xff /*sReadUTF8(s)*/ | KeyQual | isbreak;
                            CurrentApp->OnKey(kd);
                        }
                    }
                }
                break;
            }    
        }
        XRender();
    } 
    sLogF("sys","end main loop");
}

void Private::ExitGL()
{
    ExitGLCommon();

    sDelete(DefaultScreen->DepthBuffer);
    sDelete(DefaultScreen->ColorBuffer);
    sDelete(DefaultScreen);
    sDelete(DefaultContext);
    sDelete(DefaultAdapter);
    
    glXMakeCurrent(dpy, None, NULL);
    glXDestroyContext(dpy, glc);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

void Private::sRestartGfxInit()
{
    XGetWindowAttributes(dpy, win, &gwa);
    CurrentMode.SizeX = gwa.width;
    CurrentMode.SizeY = gwa.height;

    sLogF("gfx","restart gfx %dx%d",CurrentMode.SizeX,CurrentMode.SizeY);

    DefaultScreen->ColorBuffer = new sResource(DefaultAdapter,sResPara(sRBM_ColorTarget|sRU_Gpu|sRM_Texture,sRF_BGRA8888,sRES_Internal,CurrentMode.SizeX,CurrentMode.SizeY,0,0));
    DefaultScreen->DepthBuffer = new sResource(DefaultAdapter,sResPara(sRBM_DepthTarget|sRU_Gpu|sRM_Texture,sRF_D24S8,sRES_Internal,CurrentMode.SizeX,CurrentMode.SizeY,0,0));
    RestartFlag = 0;
}

void Private::sRestartGfxExit()
{
    sDelete(DefaultScreen->DepthBuffer);
    sDelete(DefaultScreen->ColorBuffer);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Platform Independent                                               ***/
/***                                                                      ***/
/****************************************************************************/

void Private::InitGLCommon()
{
	glGenFramebuffers(1,&GlRtFrameBuffer);
}

void Private::ExitGLCommon()
{
    glDeleteFramebuffers(1,&GlRtFrameBuffer);
}

void sRenderGL()
{
    sPreFrameHook.Call();
    {
        sZONE("render",0xff00ff00);
        if(RestartFlag)
        {
            sRestartGfxExit();
            sRestartGfxInit();
        }

        GlLastTexture = sGfxMaxTexture;
        GlLastAttribute = sGfxMaxVSAttrib;
        GlLastFlags = 0;
        GlLastMask = 0;
        GlLastBlendCache = 0xffffffff;
        glEnable(GL_SCISSOR_TEST);
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        //  glDepthFunc(rs->GLDepthFunc);
        //  glBlendEquationSeparate(rs->GL_BOC,rs->GL_BOA);
        //  glBlendFuncSeparate(rs->GL_BSC,rs->GL_BDC,rs->GL_BSA,rs->GL_BDA);
        //  glBlendColor(dp.BlendFactor.x,dp.BlendFactor.y,dp.BlendFactor.z,dp.BlendFactor.w);
        glColorMask(0,0,0,0);
        glDepthMask(0);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CCW);


        CurrentApp->OnFrame();
        sBeginFrameHook.Call();
        CurrentApp->OnPaint();
        sEndFrameHook.Call();
        if(sGC) sGC->CollectIfTriggered();
        sUpdateGfxStats();
    }
}

/****************************************************************************/

bool sIsSupportedFormat(int format)
{
	int i=0;
	for (;sSupportedFormats[i]!=0 && sSupportedFormats[i]!=format;i++);
    return sSupportedFormats[i] == format;
}

/****************************************************************************/

bool sIsElementIndexUintSupported()
{
    return Private::GLOESElementIndexUint;
}

/****************************************************************************/

class sGL2PreSubsystem : public sSubsystem
{
public:
    sGL2PreSubsystem() : sSubsystem("opengl_pre",0x40) {}
    void Init()
    {
        CurrentMode.ColorFormat = sRF_BGRA8888;
        CurrentMode.DepthFormat = sRF_D24S8;
    }
};

class sGL2Subsystem : public sSubsystem
{
public:
    sGL2Subsystem() : sSubsystem("opengl",0xc0) {}
    void Init()
    {
        RestartFlag = 0;

        InitGL();

        // extensions

        GlVendor   = (const char *)glGetString(GL_VENDOR);
        GlRenderer = (const char *)glGetString(GL_RENDERER);
        GlVersion  = (const char *)glGetString(GL_VERSION);
        const char *ext = (const char *) glGetString(GL_EXTENSIONS);
#if sConfigPlatform==sConfigPlatformWin
        const char *ext2 = wglGetExtensionsStringARB(Gdc);
#endif
        GlShaderVersion = (const char *) glGetString(GL_SHADING_LANGUAGE_VERSION);


        sLogF("gfx","GL Vendor   <%s>",GlVendor);
        sLogF("gfx","GL Renderer <%s>",GlRenderer);
        sLogF("gfx","GL Version  <%s>",GlVersion);
        sLogF("gfx","GL Shaders  <%s>",GlShaderVersion);

        sString<256> buffer;
        sLogF("gfx","GL Extensions:");

        int depth = 0;

        while(*ext!=0)
        {
            int i=0;
            while(*ext==' ') ext++;
            while(*ext!=' ' && *ext!=0)
                buffer[i++] = *ext++;
            while(*ext==' ') ext++;
            buffer[i++] = 0;


            if (buffer=="GL_OES_depth24") depth++;
            if (buffer=="GL_OES_depth_texture") depth++;
            if (buffer=="GL_OES_packed_depth_stencil") depth++;

            if (buffer=="GL_OES_EGL_image_external") Private::GlOESEGLImageExternal=true;    

            sLogF("gfx","  <%s>",buffer);

#if sConfigPlatform == sConfigPlatformAndroid || sConfigPlatform == sConfigPlatformIOS
            if (buffer=="GL_OES_element_index_uint") Private::GLOESElementIndexUint=true;            
#endif
#if sConfigPlatform==sConfigPlatformWin
            if(*ext==0)
            {
                ext = ext2;
                ext2 = "";
            }
#endif
        }
        sLogF("gfx",".. end of extension list");

#if sConfigPlatform == sConfigPlatformAndroid
        if (depth==3)
        {
            Private::DefaultRTDepthFormat = sRF_D24S8;            
            Private::DefaultRTDepthIsUsableInShader = true;
            sLogF("gfx","depth texture available");
        }
#endif


        int *sf = sSupportedFormats;
        *sf++ = sRF_RGBA8888;
        *sf++ = sRF_BGRA8888;
        *sf++ = sRF_BGRA4444;
        *sf++ = sRF_BGRA5551;
        *sf++ = sRF_BGR565;
        *sf++ = sRF_A8;
        *sf++ = sRF_D32;
        *sf++ = sRF_X32;
        *sf++ = sRF_D16;
        *sf++ = sRF_D24;
        *sf++ = sRF_D24S8;

		GLint params;
		GLint ta[32];
        glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS,&params);
		glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS,ta);
        
        for (int i=0;i<params;i++)
        {
        	switch (ta[i])
            {
                case 0x83f3 :
                    *sf++ = sRF_BC3;
                    sLog("GL","COMPRESSED_RGBA_S3TC_DXT5_EXT");
                break;
                case 0x83f2 :
                    *sf++ = sRF_BC2;
                    sLog("GL","COMPRESSED_RGBA_S3TC_DXT3_EXT");
                break;
                case 0x83f0 :
                    *sf++ = sRF_BC1;
                    sLog("GL","COMPRESSED_RGB_S3TC_DXT1_EXT");
                break;
                case 0x8d64 :
                    *sf++ = sRF_ETC1;
                    sLog("GL","COMPRESSED_RGB8_ETC1");
                break;
                case 0x9274 :
                    *sf++ = sRF_ETC2;
                    sLog("GL","COMPRESSED_RGB8_ETC2");
                break;
                case 0x9278 :
                    *sf++ = sRF_ETC2A;
                    sLog("GL","COMPRESSED_RGBA8_ETC2_EAC");
                break;
                case 0x8c00 :
                    *sf++ = sRF_PVR4;
                    sLog("GL","COMPRESSED_RGB_PVRTC_4BPPV1_IMG");
                break;
                case 0x8c01 :
                    *sf++ = sRF_PVR2;
                    sLog("GL","COMPRESSED_RGB_PVRTC_2BPPV1_IMG");
                break;
                case 0x8c02 :
                    *sf++ = sRF_PVR4A;
                    sLog("GL","COMPRESSED_RGBA_PVRTC_4BPPV1_IMG");
                break;
                case 0x8c03 :
                    *sf++ = sRF_PVR2A;
                    sLog("GL","COMPRESSED_RGBA_PVRTC_2BPPV1_IMG");                                        
                break;
                default :
                	sLogF("GL","Unknown compression %d",ta[i]);
                break;
            }
        }
        *sf = 0;

        RestartFlag = 1;
    }
    void Exit()
    {
        sRestartGfxExit();
        ExitGL();

        DrawRangeCount.FreeMemory();
        DrawRangeIndex.FreeMemory();
        DrawRangeVertex.FreeMemory();
    }
} sGL2Subsystem_;

/****************************************************************************/
/***                                                                      ***/
/***   sResource, Textures, Buffers                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sFindFormat(int format,int &intr,int &extr,int &type)
{
    switch(format)
    {
        default: sFatalF("unknown pixel format %08x",format);

        case sRF_D16 : intr = GL_DEPTH_COMPONENT16; extr = GL_UNSIGNED_SHORT; type = 0; break;
#if !sGLES
        case sRF_D24S8 : intr = GL_DEPTH24_STENCIL8_EXT; extr = GL_DEPTH_STENCIL_EXT;     type = GL_UNSIGNED_INT_24_8_EXT; break;
        case sRF_RGBA8888: intr = GL_RGBA; extr = GL_RGBA;     type = GL_UNSIGNED_BYTE; break;
#else
        case sRF_D24S8 : intr = GL_DEPTH_STENCIL_OES; extr = GL_DEPTH_STENCIL_OES;     type = GL_UNSIGNED_INT_24_8_OES; break;
        case sRF_RGBA8888: intr = GL_RGBA; extr = GL_RGBA;     type = GL_UNSIGNED_BYTE; break;
#endif
            
#if sConfigPlatform!=sConfigPlatformAndroid
        case sRF_BGRA8888: intr = GL_RGBA; extr = GL_BGRA_EXT; type = GL_UNSIGNED_BYTE; break;
#else    
        case sRF_BGRA8888: intr = GL_BGRA_EXT; extr = GL_BGRA_EXT; type = GL_UNSIGNED_BYTE; break;
#endif
        // case sRF_BGRA4444: intr = GL_RGBA; extr = GL_BGRA_EXT; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
        case sRF_BGR565  : intr = GL_RGB ; extr = GL_RGB ;     type = GL_UNSIGNED_SHORT_5_6_5; break;
        case sRF_A8      : intr = GL_ALPHA; extr = GL_ALPHA;   type = GL_UNSIGNED_BYTE; break;
#if !sGLES
        case sRF_BC1     : intr = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_BC2     : intr = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_BC3     : intr = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_ETC1    : intr = 0x8d64; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_ETC2    : intr = 0x9274; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_ETC2A   : intr = 0x9278; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRFC_R|sRFB_8|sRFT_UNorm: intr = GL_RED; extr = GL_RED; type = GL_UNSIGNED_BYTE; break;
        case sRFC_R|sRFB_16|sRFT_UNorm: intr = GL_RED; extr = GL_RED; type = GL_UNSIGNED_SHORT; break;
        case sRFC_R|sRFB_32|sRFT_Float: intr = GL_RED; extr = GL_RED; type = GL_FLOAT; break;

//        case sRFC_R|sRFB_8|sRFT_UNorm: intr = GL_LUMINANCE; extr = GL_LUMINANCE; type = GL_UNSIGNED_BYTE; break;
//        case sRFC_R|sRFB_32|sRFT_Float: intr = GL_LUMINANCE; extr = GL_LUMINANCE; type = GL_FLOAT; break;

#else
        case sRF_BC1     : intr = 0x83F0; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_BC2     : intr = 0x83F2; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_BC3     : intr = 0x83F3; extr = 0; type = GL_UNSIGNED_BYTE; break;

        case sRF_PVR2  : intr = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_PVR2A : intr = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_PVR4  : intr = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_PVR4A : intr = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_ETC1  : intr = GL_COMPRESSED_RGB8_ETC1; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_ETC2  : intr = GL_COMPRESSED_RGB8_ETC2; extr = 0; type = GL_UNSIGNED_BYTE; break;
        case sRF_ETC2A : intr = GL_COMPRESSED_RGBA8_ETC2_EAC; extr = 0; type = GL_UNSIGNED_BYTE; break;

        case sRFC_R|sRFB_8|sRFT_UNorm: intr = GL_LUMINANCE; extr = GL_LUMINANCE; type = GL_UNSIGNED_BYTE; break;
        case sRFC_R|sRFB_32|sRFT_Float: intr = GL_LUMINANCE; extr = GL_LUMINANCE; type = GL_FLOAT; break;
        case sRFC_R|sRFB_16|sRFT_UNorm: intr = GL_LUMINANCE; extr = GL_LUMINANCE; type = GL_UNSIGNED_SHORT; break;
#endif

    }
    return;
}

void sAdapter::GetBestMultisampling(const sResPara &para,int &count,int &quality)
{
    count = 1;
    quality = 0;
}

int sAdapter::GetMultisampleQuality(const sResPara &para,int count)
{
    return 0;
}

#if sConfigRender==sConfigRenderGLES2
sResource::sResource(sAdapter *adapter,const sResPara &para_,uint glName,bool externalOES)
{
    Adapter = adapter;
    Loading = 0;
    LoadBuffer = 0;
    LoadMipmap = -1;
    MainBind = -1;
    GLName = glName;
    GLInternalFormat = 0;
    Para = para_;
    Usage = 0;
    SharedHandle = 0;
    ExternalOES = externalOES;
    External = true;

    // usage mode for buffers

    switch(Para.Mode & sRU_Mask)
    {
        case sRU_Static:
            Usage = GL_STATIC_DRAW;
            break;
        case sRU_Gpu:
            Usage = GL_DYNAMIC_DRAW;
            break;
        case sRU_MapWrite:
            Usage = GL_STREAM_DRAW;
            break;
        default:
            sASSERT0();
            break;
    }

    // sanitize mipmaps

    if(Para.Flags & sRES_NoMipmaps)
        Para.Mipmaps = 1;

    if(Para.Mipmaps == 0)
        Para.Mipmaps = Para.GetMaxMipmaps();
    else
        Para.Mipmaps = sMin(Para.Mipmaps,Para.GetMaxMipmaps());

    if(Para.Mode & (sRBM_ColorTarget|sRBM_DepthTarget))
        Para.Mipmaps = 1;
    
    // get element size

    int bitsperpixel = sGetBitsPerPixel(Para.Format);
    if(Para.BitsPerElement == 0) 
        Para.BitsPerElement = bitsperpixel;
    else 
        if(bitsperpixel!=0) 
            sASSERT(Para.BitsPerElement==bitsperpixel);

    // create specialized resources
    if(Para.Mode & sRBM_Vertex)
    {
        MainBind = sRBM_Vertex;
        TotalSize = Para.BitsPerElement/8 * Para.SizeX;
        sASSERT(Para.SizeY==0);
        sASSERT(Para.SizeZ==0);
        sASSERT(Para.SizeA==0);
    }
    else if(Para.Mode & sRBM_Index)
    {
        MainBind = sRBM_Vertex;
        TotalSize = Para.BitsPerElement/8 * Para.SizeX;
        sASSERT(Para.SizeY==0);
        sASSERT(Para.SizeZ==0);
        sASSERT(Para.SizeA==0);
    }    
    else if(!(Para.Mode&sRM_Texture) && Para.Mode&(sRBM_ColorTarget|sRBM_DepthTarget))
    {
        MainBind = Para.Mode & (sRBM_ColorTarget|sRBM_DepthTarget);
    }    
    else if(Para.Mode & sRBM_Shader)
    {
        sASSERT(Para.SizeZ==0);
        TotalSize = 0;
        if(Para.SizeA==6 && (Para.Mode & sRM_Cubemap))
        {
            sASSERT(Para.SizeX==Para.SizeY);
            sFatal("cubemaps not yet supported");
        }
        else
        {
            MainBind = sRBM_Shader;
            sASSERT(Para.SizeA==0);

            int intr,extr,type;
            sFindFormat(Para.Format,intr,extr,type);
            GLInternalFormat = intr;
        }
    }
    else
    {
        TotalSize = 0;
    }    
}
#endif


sResource::sResource(sAdapter *adapter,const sResPara &para_,const void *data,uptr size)
{
    Adapter = adapter;
    Loading = 0;
    LoadBuffer = 0;
    LoadMipmap = -1;
    MainBind = -1;
    GLName = 0;
    GLInternalFormat = 0;
    Para = para_;
    Usage = 0;
    SharedHandle = 0;
    ExternalOES = false;
    External = false;

    // usage mode for buffers

    switch(Para.Mode & sRU_Mask)
    {
        case sRU_Static:
            Usage = GL_STATIC_DRAW;
            break;
        case sRU_Gpu:
            Usage = GL_DYNAMIC_DRAW;
            break;
        case sRU_MapWrite:
            Usage = GL_STREAM_DRAW;
            break;
        case sRU_SystemMem:
            Usage = GL_STATIC_DRAW;
            break;
        case sRU_MapRead:
            Usage = GL_DYNAMIC_DRAW;
            break;
        default:
            sASSERT0();
            break;
    }

    // sanitize mipmaps

    if(Para.Flags & sRES_NoMipmaps)
        Para.Mipmaps = 1;

    if(Para.Mipmaps == 0)
        Para.Mipmaps = Para.GetMaxMipmaps();
    else
        Para.Mipmaps = sMin(Para.Mipmaps,Para.GetMaxMipmaps());

    if(Para.Mode & (sRBM_ColorTarget|sRBM_DepthTarget))
        Para.Mipmaps = 1;
    
    // get element size

    int bitsperpixel = sGetBitsPerPixel(Para.Format);
    if(Para.BitsPerElement == 0) 
        Para.BitsPerElement = bitsperpixel;
    else 
        if(bitsperpixel!=0) 
            sASSERT(Para.BitsPerElement==bitsperpixel);

    // create specialized resources
    if(Para.Mode & sRBM_Vertex)
    {
        MainBind = sRBM_Vertex;
        TotalSize = Para.BitsPerElement/8 * Para.SizeX;
        sASSERT(Para.SizeY==0);
        sASSERT(Para.SizeZ==0);
        sASSERT(Para.SizeA==0);

        glGenBuffers(1,&GLName);
        GLERR();
        glBindBuffer(GL_ARRAY_BUFFER,GLName);
        GLERR();
        if(data) sASSERT(size==TotalSize);
        glBufferData(GL_ARRAY_BUFFER,TotalSize,data,Usage);
        GLERR();
        glBindBuffer(GL_ARRAY_BUFFER,0);
        GLERR();
    }
    else if(Para.Mode & sRBM_Index)
    {
        MainBind = sRBM_Vertex;
        TotalSize = Para.BitsPerElement/8 * Para.SizeX;
        sASSERT(Para.SizeY==0);
        sASSERT(Para.SizeZ==0);
        sASSERT(Para.SizeA==0);

        glGenBuffers(1,&GLName);
        GLERR();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,GLName);
        GLERR();
        if(data) sASSERT(size==TotalSize);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,TotalSize,data,Usage);
        GLERR();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
        GLERR();
    }    
    else if(!(Para.Mode&sRM_Texture) && Para.Mode&(sRBM_ColorTarget|sRBM_DepthTarget))
    {
        MainBind = Para.Mode & (sRBM_ColorTarget|sRBM_DepthTarget);
        if(!(Para.Flags & sRES_Internal))
        {
            int intr,extr,type;
            sFindFormat(Para.Format,intr,extr,type);
            glGenRenderbuffers(1,&GLName);
            glBindRenderbuffer(GL_RENDERBUFFER,GLName);
            glRenderbufferStorage(GL_RENDERBUFFER,intr,Para.SizeX,Para.SizeY);
            glBindRenderbuffer(GL_RENDERBUFFER,0);
        }
    }    
    else if(Para.Mode & sRBM_Shader)
    {
        sASSERT(Para.SizeZ==0);
        TotalSize = 0;
        if(Para.SizeA==6 && (Para.Mode & sRM_Cubemap))
        {
            sASSERT(Para.SizeX==Para.SizeY);
            sFatal("cubemaps not yet supported");
        }
        else
        {
            MainBind = sRBM_Shader;
            sASSERT(Para.SizeA==0);

            glActiveTexture(GL_TEXTURE0);
            GLERR();
            glGenTextures(1,&GLName);
            GLERR();
            glBindTexture(GL_TEXTURE_2D,GLName);
            GLERR();                    
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            GLERR();
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            GLERR();
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            GLERR();
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,Para.Mipmaps==1 ? GL_NEAREST : GL_NEAREST_MIPMAP_NEAREST);
            GLERR();
#if sConfigPlatform!=sConfigPlatformAndroid
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,Para.Mipmaps-1);
            GLERR();
#endif
            int intr,extr,type;
            sFindFormat(Para.Format,intr,extr,type);
            GLInternalFormat = intr;

            if(extr==0) // compression
            {
                if((Para.Mode & sRU_Mask)==sRU_Static)
                {
                    sASSERT(data);
                    const uint8 *d = (const uint8 *)data;
                    for(int i=0;i<Para.Mipmaps;i++)
                    {
                        uptr n = Para.GetTextureSize(i);
                        sASSERT(size>=n);
                        glCompressedTexImage2D(GL_TEXTURE_2D,i,intr,Para.SizeX>>i,Para.SizeY>>i,0,(GLsizei)n,d);
                        GLERR();
                        d += n;
                        size -= n;
                    }
                    sASSERT(size==0);
                }
                else
                {
                    sASSERT(data==0);
                    for(int i=0;i<Para.Mipmaps;i++)
                    {
                        uptr n = Para.GetTextureSize(i);
                        glCompressedTexImage2D(GL_TEXTURE_2D,i,intr,Para.SizeX>>i,Para.SizeY>>i,0,(GLsizei)n,0);
                        GLERR();
                    }
                }
            }
            else
            {
                if((Para.Mode & sRU_Mask)==sRU_Static)
                {
                    sASSERT(data);
                    const uint8 *d = (const uint8 *)data;
                    for(int i=0;i<Para.Mipmaps;i++)
                    {
                        uptr n = Para.GetTextureSize(i);
                        sASSERT(size>=n);
                        glTexImage2D(GL_TEXTURE_2D,i,intr,Para.SizeX>>i,Para.SizeY>>i,0,extr,type,d);
                        GLERR();
                        d += n;
                        size -= n;
                    }
                    sASSERT(size==0);
                }
                else
                {
                    sASSERT(data==0);                            
                    for(int i=0;i<Para.Mipmaps;i++)
                    {
                        glTexImage2D(GL_TEXTURE_2D,i,intr,Para.SizeX>>i,Para.SizeY>>i,0,extr,type,0);                        
                        GLERR();
                    }
                }
            }
            glBindTexture(GL_TEXTURE_2D,0);
            GLERR();
        }                
    }
    else
    {
        TotalSize = 0;
    }    
}

sResource::~sResource()
{
    if(GLName && !External)
    {
        switch(MainBind)
        {
        case sRBM_Shader:
            glDeleteTextures(1,&GLName);	//??Tron Check
            GLERR();
            break;
        case sRBM_ColorTarget:
        case sRBM_DepthTarget:
            glDeleteRenderbuffers(1,&GLName);
            GLERR();
            break;
        case sRBM_Index:
        case sRBM_Vertex:
            glDeleteBuffers(1,&GLName);
            GLERR();
            break;
        default:
            sASSERT0();
            break;
        }
    }
    sASSERT(LoadBuffer == 0);
}

#if sConfigPlatform==sConfigPlatformAndroid
static char *bufData = 0;
static GLint bufSize = 0;
static GLint bufSizeTotal = 0;
#endif

void sResource::MapBuffer0(sContext *ctx,void **data,sResourceMapMode mode)
{
#if sConfigPlatform==sConfigPlatformAndroid
    sASSERT(!Loading);
    Loading = 1;
    if(MainBind==sRBM_Vertex)
        Loading = GL_ARRAY_BUFFER;
    if(MainBind==sRBM_Index)
        Loading = GL_ELEMENT_ARRAY_BUFFER;
    sASSERT(Loading);

    glBindBuffer(Loading,GLName);
    GLERR();

    glGetBufferParameteriv(Loading,  GL_BUFFER_SIZE, &bufSize);
    GLERR();

    if (bufSizeTotal<bufSize)
    {
        bufData = new char[bufSize];
        bufSizeTotal = bufSize;
    }

    *data = bufData;
    glBindBuffer(Loading,0);
    GLERR();
#else
    sASSERT(!Loading);
    Loading = 1;
    if(MainBind==sRBM_Vertex)
        Loading = GL_ARRAY_BUFFER;
    if(MainBind==sRBM_Index)
        Loading = GL_ELEMENT_ARRAY_BUFFER;
    sASSERT(Loading);
    //  sDPrintF("map %d\n",GLName);
    glBindBuffer(Loading,GLName);
    GLERR();
    //  if(!(flags & sRMF_NoOverwrite))
    //    glBufferData(Loading,TotalSize,0,Usage);   // discard
    *data = glMapBuffer(Loading,GL_WRITE_ONLY);
    GLERR();
    glBindBuffer(Loading,0);
    GLERR();
#endif
}

void sResource::MapTexture0(sContext *ctx,int mipmap,int index,void **data,int *pitch,int *pitch2,sResourceMapMode mode)
{
    sASSERT(Loading==0);
    sASSERT(MainBind==sRBM_Shader);
    sASSERT(index==0);
    sASSERT(mipmap>=0 && mipmap<Para.Mipmaps);
    int p = (Para.SizeX>>mipmap) * Para.BitsPerElement / 8;
    int size = (Para.SizeY>>mipmap) * p;
    LoadBuffer = new uint8[size];
    LoadMipmap = mipmap;
    *data = LoadBuffer;
    *pitch = p;
    Loading = 1;
}

void sResource::Unmap(sContext *ctx)
{
    sASSERT(Loading);
  
    if(LoadBuffer)    // texture
    {
        sASSERT(MainBind==sRBM_Shader);
        int intr,extr,type;
        sFindFormat(Para.Format,intr,extr,type);
        glBindTexture(GL_TEXTURE_2D,GLName);
        GLERR();
        glTexSubImage2D(GL_TEXTURE_2D,LoadMipmap,0,0,
            Para.SizeX>>LoadMipmap,Para.SizeY>>LoadMipmap,
            extr,type,LoadBuffer);
        GLERR();
        glBindTexture(GL_TEXTURE_2D,0);
        GLERR();
        delete[] LoadBuffer;
        LoadBuffer = 0;
        LoadMipmap = -1;
    }
    else              // vertex / index
    {
      int bind = 0;
      switch(MainBind)
      {
        case sRBM_Vertex:
          bind = GL_ARRAY_BUFFER;
          break;
        case sRBM_Index:
          bind = GL_ELEMENT_ARRAY_BUFFER;
          break;
        default:
          sASSERT0();
          break;
      }

      glBindBuffer(bind,GLName);
      GLERR();
#if sConfigPlatform==sConfigPlatformAndroid
      glBufferData(bind, bufSize, bufData, GL_STATIC_DRAW);
      GLERR();
#else
      glUnmapBuffer(Loading);
      GLERR();
#endif
      glBindBuffer(bind,0);
      GLERR();
    }
    Loading = 0;
}

void sResource::UpdateBuffer(void *data,int startbyte,int bytesize)
{
    sFatal("sResource::UpdateBuffer - not implemented");
}

void sResource::UpdateTexture(void *data,int miplevel,int arrayindex)
{
    int intr,extr,type;
    sFindFormat(Para.Format,intr,extr,type);
    glBindTexture(GL_TEXTURE_2D,GLName);
    GLERR();
    glTexImage2D(GL_TEXTURE_2D,miplevel,intr,Para.SizeX>>miplevel,Para.SizeY>>miplevel,0,extr,type,data);
    GLERR();
   // sFatal("sResource::UpdateTexture - not implemented");
}

#if sConfigRender==sConfigRenderGLES2 || sConfigRender==sConfigRenderGL2

void sResource::UpdateTexture(void *data, int sizex, int sizey,int miplevel)
{
    Para.SizeX = sizex;
    Para.SizeY = sizey;
    UpdateTexture(data, miplevel, 0);
}

#endif


void sResource::ReadTexture(void *data,uptr size)
{
    sASSERT(!InsideBeginTarget);
    int intr,extr,type;
    sFindFormat(Para.Format,intr,extr,type);
    sASSERT(size==(Para.SizeX*sGetBitsPerPixel(Para.Format))/8*Para.SizeY);

    Adapter->ImmediateContext->BeginTarget(sTargetPara(0,0,this,0));
    glReadPixels(0,0,Para.SizeX,Para.SizeY,extr,type,data);
    Adapter->ImmediateContext->EndTarget();
}

/****************************************************************************/
/***                                                                      ***/
/***   Vertex Formats & Geometry                                          ***/
/***                                                                      ***/
/****************************************************************************/

sVertexFormat::sVertexFormat(sAdapter *adapter,const uint *desc_,int count_,const sChecksumMD5 &hash_)
{
    Adapter = adapter;
    Hash = hash_;
    Count = count_;
    Desc = desc_;

    int i,b[sGfxMaxStream];
    int stream;

    for(i=0;i<sGfxMaxStream;i++)
      b[i] = 0;

    //  bool dontcreate=0;
    i = 0;
    int j = 0;

    sClear(Attribs);

    while(Desc[i])
    {
      stream = (Desc[i]&sVF_StreamMask)>>sVF_StreamShift;

      Attribs[i].Stream = stream;
      Attribs[i].Offset = b[stream];

      sASSERT(i<sGfxMaxVSAttrib);

      // actually, we ignore the use-mask. just depend on correct ordering!
      // semantics are too oldschool.

      switch(Desc[i]&sVF_TypeMask)
      {
        case sVF_F1:  Attribs[j].Type = GL_FLOAT; Attribs[j].Size=1; Attribs[j].Normalized=0; b[stream]+=1*4;  break;
        case sVF_F2:  Attribs[j].Type = GL_FLOAT; Attribs[j].Size=2; Attribs[j].Normalized=0; b[stream]+=2*4;  break;
        case sVF_F3:  Attribs[j].Type = GL_FLOAT; Attribs[j].Size=3; Attribs[j].Normalized=0; b[stream]+=3*4;  break;
        case sVF_F4:  Attribs[j].Type = GL_FLOAT; Attribs[j].Size=4; Attribs[j].Normalized=0; b[stream]+=4*4;  break;
        case sVF_I4:  Attribs[j].Type = GL_UNSIGNED_BYTE; Attribs[j].Size=4; Attribs[j].Normalized=0; b[stream]+=4*1;  break;
        case sVF_C4:  Attribs[j].Type = GL_UNSIGNED_BYTE; Attribs[j].Size=4; Attribs[j].Normalized=1; b[stream]+=4*1;  break;
        case sVF_S2:  Attribs[j].Type = GL_SHORT; Attribs[j].Size=2; Attribs[j].Normalized=1; b[stream]+=2*2;  break;
        case sVF_S4:  Attribs[j].Type = GL_SHORT; Attribs[j].Size=4; Attribs[j].Normalized=1; b[stream]+=4*2;  break;
            //    case sVF_H2:  Attrib[j].Type = GL_FLOAT; Attribs[j].Size=2; Attribs[j].Normalized=0;  b[stream]+=1*4;  break;
            //    case sVF_H4:  Attrib[j].Type = GL_FLOAT; Attribs[j].Size=2; Attribs[j].Normalized=0;  b[stream]+=2*4;  break;

        default: sASSERT0(); break;
        }

        Attribs[j].Instanced = (Desc[i] & sVF_InstanceData) ? 1 : 0;

        i++;
        j++;
    }

    Streams = 0;
    for(int i=0;i<j;i++)
    {
        Attribs[i].Pitch = b[Attribs[i].Stream];
        Streams = sMax(Streams,Attribs[i].Stream+1);
    }
    for(int i=0;i<sGfxMaxStream;i++)
    {
        VertexSize[i] = b[i];
    }
}

sVertexFormat::~sVertexFormat()
{
    delete[] Desc;
}

int sVertexFormat::GetStreamCount()
{
    return Streams;
}

int sVertexFormat::GetStreamSize(int stream)
{
    return VertexSize[stream];
}

/****************************************************************************/

void sGeometry::PrivateInit()
{
    Topology = 0; 

    const static int top[16] = 
    {
        0,GL_POINTS,GL_LINES,GL_TRIANGLES
    };

    // quads in GL have different vertex order than in Altona2

    Topology = top[(Mode & sGMP_Mask)>>sGMP_Shift];
    sASSERT(Topology!=0);
}

void sGeometry::PrivateExit()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Shaders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sShaderBinary *sCompileShader(sShaderTypeEnum type,const char *profile,const char *source,sTextLog *errors)
{
    sShaderBinary *sh = new sShaderBinary;
//    sTextLog log; log.PrintWithLineNumbers(source); sDPrint(log.Get());
    sh->Type = type;
    sh->Profile = profile;
    sh->Size = sGetStringLen(source)+1;
    sh->Hash.Calc((const uint8 *)source,sh->Size);
    uint8 *data = new uint8[sh->Size];
    sh->Data = data;
    sh->DeleteData = 1;
    sCopyMem(data,source,sh->Size);

    return sh;
}

sShader::sShader(sAdapter *adapter,sShaderBinary *bin)
{
    Adapter = adapter;
    Hash = bin->Hash;
    Type = bin->Type;

    int shadertype = 0;
    switch(Type)
    {
    case sST_Vertex: shadertype = GL_VERTEX_SHADER; break;
    case sST_Pixel:  shadertype = GL_FRAGMENT_SHADER; break;
    default: sASSERT0();
    }

    GLName = glCreateShader(shadertype);
    GLERR();

    glShaderSource(GLName,1,(const char **)&bin->Data,0);
    GLERR();

    glCompileShader(GLName);
    GLERR();

    int ok=0;
    glGetShaderiv(GLName,GL_COMPILE_STATUS,&ok);
    GLERR();

    if(!ok)
    {
        sString<sFormatBuffer> error;
        glGetShaderInfoLog(GLName,sFormatBuffer,0,(char *)error.Get());
        sDPrint(error);
        sLog("gfx","compile shader");
        sTextLog tl;
        tl.PrintWithLineNumbers((const char *)bin->Data);
        sDPrint(tl.Get());
        sFatal("shader compiler fail");
    }
}

sShader::~sShader()
{
    if(GLName)
    {
        glDeleteShader(GLName);
        GLERR();
    }
}


sCBufferBase::sCBufferBase(sAdapter *adapter,int size,sShaderTypeEnum type,int slot)
{
    Adapter = adapter;
    LoadBuffer = new uint8[size];
    Size = size;
    Type = 1<<type;
    Slot = slot;
}

sCBufferBase::sCBufferBase(sAdapter *adapter,int size,int typesmask,int slot)
{
  sFatal("sCBufferBase::sCBufferBase(sAdapter *adapter,int size,int typesmask,int slot) - not implemented");
}

sCBufferBase::~sCBufferBase()
{
    delete[] LoadBuffer;
}

void sCBufferBase::Map(sContext *ctx,void **ptr)
{
    *ptr = LoadBuffer;
}

void sCBufferBase::Unmap(sContext *ctx)
{
}

/****************************************************************************/
/***                                                                      ***/
/***   States                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sRenderState::sRenderState(sAdapter *adapter,const sRenderStatePara &para,const sChecksumMD5 &hash)
{
    Adapter = adapter;
    Hash = hash;

    static const int func[16] =
    {
        0,
        GL_NEVER,GL_LESS,GL_EQUAL,GL_LEQUAL,
        GL_GREATER,GL_NOTEQUAL,GL_GEQUAL,GL_ALWAYS,
    };
    static const int blend[64] =
    {
        0,
        GL_ZERO,
        GL_ONE,
        GL_SRC_COLOR,

        GL_ONE_MINUS_SRC_COLOR,
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA,
        GL_DST_ALPHA,

        GL_ONE_MINUS_DST_ALPHA,
        GL_DST_COLOR,
        GL_ONE_MINUS_DST_COLOR,
        GL_SRC_ALPHA_SATURATE,

        0, // 0x0c
        0, // 0x0d,
        GL_CONSTANT_COLOR,
        GL_ONE_MINUS_CONSTANT_COLOR,
    };
    static const int op[16] =
    {
        0,
        GL_FUNC_ADD,
        GL_FUNC_SUBTRACT,
        GL_FUNC_REVERSE_SUBTRACT,
    };

    // pixel

    GLFlags = 0;
    if(!(para.Flags & sMTRL_ScissorDisable)) GLFlags |= 1;

    if(para.Flags & sMTRL_DepthRead) GLFlags |= 4;
    if(para.BlendAlpha[0]!=sMB_Off || para.BlendColor[0]!=sMB_Off) GLFlags |= 8;

    GLDepthFunc = func[para.DepthFunc&15];

    // blend

    int bc = para.BlendColor[0];
    int ba = para.BlendAlpha[0];
    if(bc==0) bc = sMBS_1 | sMBO_Add | sMBD_0;
    if(ba==0) ba = sMBS_1 | sMBO_Add | sMBD_0;

    GL_BSC = blend[(bc>> 0)&63];
    GL_BOC = op   [(bc>>12)&15];
    GL_BDC = blend[(bc>> 6)&63];

    GL_BSA = blend[(ba>> 0)&63];
    GL_BOA = op   [(ba>>12)&15];
    GL_BDA = blend[(ba>> 6)&63];

    GL_BlendCache = bc | (ba<<16);

    GLMask = (~para.TargetWriteMask[0])&15;
    if(para.Flags & sMTRL_DepthWrite)
        GLMask |= 0x10;

    // polygon

    if(para.Flags & sMTRL_CullMask) GLFlags |= 16;
    GLCull = ((para.Flags & sMTRL_CullMask)==sMTRL_CullInv) ? GL_BACK : GL_FRONT;
}

sRenderState::~sRenderState()
{
}

/****************************************************************************/

sSamplerState::sSamplerState(sAdapter *adapter,const sSamplerStatePara &para,const sChecksumMD5 &hash)
{
    Adapter = adapter;
    Hash = hash;

    static const int addr[16] =
    {
        GL_REPEAT,
        GL_CLAMP_TO_EDGE,
        0,
        GL_MIRRORED_REPEAT,
        0,
    };
    static const int filtmin[8] =
    {
        GL_NEAREST_MIPMAP_NEAREST,
        GL_LINEAR_MIPMAP_NEAREST,
        GL_NEAREST_MIPMAP_NEAREST,
        GL_LINEAR_MIPMAP_NEAREST,

        GL_NEAREST_MIPMAP_LINEAR,
        GL_LINEAR_MIPMAP_LINEAR,
        GL_NEAREST_MIPMAP_LINEAR,
        GL_LINEAR_MIPMAP_LINEAR,
    };
    static const int filtminnm[2] =
    {
        GL_NEAREST,
        GL_LINEAR,
    };

    GLWrapS = addr[(para.Flags&sTF_UMask)>>16];
    GLWrapT = addr[(para.Flags&sTF_VMask)>>20];
    if((para.Flags & sTF_FilterMask)==sTF_Aniso)
    {
        GLMinFilter = GL_LINEAR_MIPMAP_LINEAR;
        GLMinFilterNoMipmap = GL_LINEAR;
        GLMagFilter = GL_LINEAR;
    }
    else
    {
        GLMinFilter = filtmin[para.Flags&7];
        GLMinFilterNoMipmap = filtminnm[para.Flags&1];
        GLMagFilter = (para.Flags&2) ? GL_LINEAR : GL_NEAREST;
    }
}

sSamplerState::~sSamplerState()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Material                                                           ***/
/***                                                                      ***/
/****************************************************************************/

int sFindEndNumber(const char *x)
{
    int val = 0;
    int fac = 1;
    int len = (int)sGetStringLen(x)-1;
    while(len>0 && sIsDigit(x[len]))
    {
        val += fac*(x[len]-'0');
        fac = fac*10;
        len--;
    }
    return val;
}

sMaterialPrivate::~sMaterialPrivate()
{
    glDeleteProgram(GLName);
}

void sMaterial::PrivateSetBase(sAdapter *adapter)
{
    GLName = glCreateProgram();
    GLERR();
    glAttachShader(GLName,Shaders[sST_Vertex]->GLName);
    GLERR();
    glAttachShader(GLName,Shaders[sST_Pixel]->GLName);
    GLERR();
    glLinkProgram(GLName);
    GLERR();
    int ok;
    glGetProgramiv(GLName,GL_LINK_STATUS,&ok);
    if(!ok)
    {
        sString<sFormatBuffer> error;
        glGetProgramInfoLog(GLName,sFormatBuffer,0,(char *)error.Get());
        sDPrint(error);
        sFatal("shader link fail");
    }

    // locate attributes
    int attrcount = 0;
    glGetProgramiv(GLName,GL_ACTIVE_ATTRIBUTES,&attrcount);
    GLERR();
    
    for(int i=0;i<sGfxMaxVSAttrib;i++)
        AttribMap[i] = -1;
    for(int i=0;i<attrcount;i++)
    {
        sString<64> name;
        GLenum type;
        GLint count;
        GLsizei len;
        glGetActiveAttrib(GLName,i,64,&len,&count,&type,name);
        GLERR();
        int n = sFindEndNumber(name);
        int loc = glGetAttribLocation(GLName,name);
        GLERR();
        //    sLogF("gfx","shader attribute %s:%04x %d",name,type,loc);
        sASSERT(loc>=0 && loc<sGfxMaxVSAttrib);
        sASSERT(AttribMap[loc]==-1);
        AttribMap[loc] = n;
    }

    // locate textures and cbuffers

    int unicount = 0;
    glGetProgramiv(GLName,GL_ACTIVE_UNIFORMS,&unicount);
    GLERR();
    
    CBInfos.HintSize(unicount);
    SRInfos.HintSize(unicount);
    CBInfos.Clear();
    SRInfos.Clear();
    for(int i=0;i<unicount;i++)
    {
        sString<64> name;
        GLenum type;
        GLint count;
        GLsizei len;
        glGetActiveUniform(GLName,i,64,&len,&count,&type,name);
        GLERR();
        int n = sFindEndNumber(name);
        int loc = glGetUniformLocation(GLName,name);
        GLERR();
        //    sLogF("gfx","shader uniform %s %d:%04x [%d]",name,loc,type,count);
        if(type==GL_FLOAT_VEC4)   // constant buffer?
        {
            CBInfo cb;
            cb.Slot = n;
            cb.Loc = loc;
            cb.Count = count;
            sASSERT(name[1]=='c' && sIsDigit(name[2]));
            if(name[0]=='v') cb.Type = sST_Vertex;
            else if(name[0]=='p') cb.Type = sST_Pixel;
            else sASSERT0();

            CBInfos.Add(cb);
        }
        else if(type==GL_SAMPLER_2D || type==GL_SAMPLER_EXTERNAL_OES)
        {
            sASSERT(n>=0 && n<sGfxMaxTexture);
            SRInfo sr;
            sr.Loc = loc;
            sr.Slot = n;
            SRInfos.Add(sr);
        }
        else
        {
            sASSERT0();
        }
    }    
}


void sMaterial::SetTexture(sShaderTypeEnum shader,int index,sResource *tex,sSamplerState *sam,int slicestart,int slicecount)
{
    sASSERT(slicestart==0);
    sASSERT(slicecount==1);

    if(shader==sST_Pixel)
    {
        SamplerPS.SetAndGrow(index,sam);
        TexturePS.SetAndGrow(index,tex);
    }
    else if(shader==sST_Vertex)
    {
        SamplerVS.SetAndGrow(index,sam);
        TextureVS.SetAndGrow(index,tex);
    }
    else
    {
        sASSERT0();
    }
}

void sMaterial::UpdateTexture(sShaderTypeEnum shader,int index,sResource *tex,int slicestart,int slicecount)
{
    sASSERT(slicestart==0);
    sASSERT(slicecount==1);

    if(shader==sST_Pixel)
    {
        TexturePS[index] = tex;
    }
    else if(shader==sST_Vertex)
    {
        TextureVS[index] = tex;
    }
    else
    {
        sASSERT0();
    }
}

sResource *sMaterial::GetTexture(sShaderTypeEnum shader,int index)
{
    if(shader==sST_Vertex && TextureVS.IsIndex(index))
        return TextureVS[index];
    if(shader==sST_Pixel && TexturePS.IsIndex(index))
        return TexturePS[index];
    return 0;
}

void sMaterial::SetUav(sShaderTypeEnum shader,int index,sResource *uav)
{
    sASSERT0();
}

void sMaterial::ClearTextureSamplerUav()
{
    SamplerVS.Clear();
    SamplerPS.Clear();
    TextureVS.Clear();
    TexturePS.Clear();
}

/****************************************************************************/
/***                                                                      ***/
/***   Functions                                                          ***/
/***                                                                      ***/
/****************************************************************************/


void sContext::Draw(const sDrawPara &dp)
{
    sUpdateGfxStats(dp);

    sGeometry *geo = 0;
    sVertexFormat *fmt = 0;
    uint topology = 0;
    int ic=0;
    int vc=0;
    int is=0;
    const uint8 *ip=0;
    const uint8 *vp=0;



    if(dp.Flags & sDF_Arrays)
    {
        sASSERT(!(dp.Flags & sDF_Ranges))
        topology = GL_TRIANGLES;
        fmt = dp.VertexArrayFormat;
        vc = dp.VertexArrayCount;
        vp = (const uint8 *) dp.VertexArray;
        ic = dp.IndexArrayCount;
        ip = (const uint8 *) dp.IndexArray;
        if(dp.IndexArraySize==2)
        	 is = GL_UNSIGNED_SHORT;
        if(dp.IndexArraySize==4)
        		is = GL_UNSIGNED_INT;
    }
    else
    {
        geo = dp.Geo;
        topology = geo->Topology;
        fmt = geo->Format;
        vc = geo->VertexCount;
        if(geo->Index)
        {
            ic = geo->IndexCount;
            ip = (const uint8 *)(geo->IndexFirst * 2);
        }
        if((dp.Geo->Mode & sGMF_IndexMask)==sGMF_Index16)
        		is = GL_UNSIGNED_SHORT;
        if((dp.Geo->Mode & sGMF_IndexMask)==sGMF_Index32)
        	is = GL_UNSIGNED_INT;
    }
    sASSERT((ic==0) == (is==0));


    // set shaders

    glUseProgram(dp.Mtrl->GLName);
    GLERR();

    for(auto &cbi : dp.Mtrl->CBInfos)
    {
        for(int i=0;i<dp.CBCount;i++)
        {
            if((dp.CBs[i]->Type&(1<<cbi.Type)) && dp.CBs[i]->Slot==cbi.Slot)
            {
                glUniform4fv(cbi.Loc,cbi.Count,(float *)dp.CBs[i]->LoadBuffer);
                GLERR();
                break;
            }
        }
    }

    for(auto &sri : dp.Mtrl->SRInfos)
    {
        glUniform1i(sri.Loc,sri.Slot);
        GLERR();
    }
    // set textures

    for(int i=0;i<dp.Mtrl->TexturePS.GetCount();i++)
    {
        glActiveTexture(GL_TEXTURE0+i);
        GLERR();
        sResource *tex = dp.Mtrl->TexturePS[i];
        sSamplerState *sam = dp.Mtrl->SamplerPS.IsIndex(i) ? dp.Mtrl->SamplerPS[i] : 0;
        if(tex)
        {
            if (tex->ExternalOES)
                glBindTexture(GL_TEXTURE_EXTERNAL_OES,tex->GLName);
            else
                glBindTexture(GL_TEXTURE_2D,tex->GLName);
            GLERR();
            if(sam)
            {
#if sConfigPlatform!=sConfigPlatformAndroid
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,sMin(sam->GLMaxLevel,tex->Para.Mipmaps-1));
                GLERR();
#endif
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,tex->Para.Mipmaps!=1 ? sam->GLMinFilter : sam->GLMinFilterNoMipmap);
                GLERR();
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,sam->GLMagFilter);
                GLERR();
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,sam->GLWrapS);
                GLERR();
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,sam->GLWrapT);
                GLERR();
            }
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D,0);
            GLERR();
        }
    }
    for(int i=dp.Mtrl->TexturePS.GetCount();i<GlLastTexture;i++)
    {
        glActiveTexture(GL_TEXTURE0+i);
        GLERR();
        glBindTexture(GL_TEXTURE_2D,0);
        GLERR();
    }
    GlLastTexture = dp.Mtrl->TexturePS.GetCount();

    // set state

    if(1)
    {
        const sRenderState *rs = dp.Mtrl->RenderState;
        uint flags = rs->GLFlags;
        uint change = flags ^ GlLastFlags;
        /*
        if(change & 0x00000001) {
        		if(flags & 0x00000001)
                	glEnable(GL_SCISSOR_TEST);
                else
                	glDisable(GL_SCISSOR_TEST); }
                    */
        //    if(change & 0x00000002) { if(flags & 0x00000002) glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE) else glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE); }
        if(change & 0x00000004) { if(flags & 0x00000004) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST); }
        if(change & 0x00000008) { if(flags & 0x00000008) glEnable(GL_BLEND); else glDisable(GL_BLEND); }
        if(change & 0x00000010) { if(flags & 0x00000010) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE); }

        glDepthFunc(rs->GLDepthFunc);
        if(flags & 0x00000008)
        {
            if(GlLastBlendCache!=rs->GL_BlendCache)
            {
                glBlendFuncSeparate(rs->GL_BSC,rs->GL_BDC,rs->GL_BSA,rs->GL_BDA);
                GlLastBlendCache=rs->GL_BlendCache;
            }
            glBlendEquationSeparate(rs->GL_BOC,rs->GL_BOA);
            if(dp.Flags & sDF_BlendFactor)
                glBlendColor(dp.BlendFactor.x,dp.BlendFactor.y,dp.BlendFactor.z,dp.BlendFactor.w);
            else
                glBlendColor(rs->GLBlendColor.x,rs->GLBlendColor.y,rs->GLBlendColor.z,rs->GLBlendColor.w);
        }
        if(rs->GLMask != GlLastMask)
        {
            glColorMask((rs->GLMask>>0)&1,(rs->GLMask>>1)&1,(rs->GLMask>>2)&1,(rs->GLMask>>3)&1);
            glDepthMask((rs->GLMask>>4)&1);
        }
        glCullFace(rs->GLCull);
        //    glFrontFace(GL_CW);

        GlLastFlags = flags;
        GlLastMask = rs->GLMask;
        GLERR();
    }

    // set vertex buffer

    bool instanced = (dp.Flags & sDF_Instances)?1:0;

#if sGLES
    if(instanced)
        sFatal("glDrawElementsInstanced not supported");
#endif

    // vertex buffer

    int newalimit = 0;
    if(!geo)
        glBindBuffer(GL_ARRAY_BUFFER,0);
    for(int i=0;i<sGfxMaxVSAttrib;i++)
    {
        if(dp.Mtrl->AttribMap[i]>=0 && fmt->Attribs[dp.Mtrl->AttribMap[i]].Size>0)
        {
            newalimit = i+1;
            const sVertexFormatPrivate::attrib *at = &fmt->Attribs[dp.Mtrl->AttribMap[i]];
            const void *data = 0;
            if(geo)
            {
                glBindBuffer(GL_ARRAY_BUFFER,geo ? geo->Vertex[at->Stream]->GLName : 0);
                GLERR();
            }
            glEnableVertexAttribArray(i);
            GLERR();
            glVertexAttribPointer(i,
                at->Size,
                at->Type,
                at->Normalized,
                at->Pitch,
                (const void *)uptr(vp + at->Offset + dp.VertexOffset[at->Stream]) );
            GLERR();
#if !sGLES
            glVertexAttribDivisorARB(i,at->Instanced && instanced);
            GLERR();
#endif
        }
        else
        {
            if(i<GlLastAttribute)
            {
                glDisableVertexAttribArray(i);
                GLERR();
            }
        }
    }
    GlLastAttribute = newalimit;

    // set index buffer and kick it

#if !sGLES
    if(instanced)
    {
        sASSERT(!(dp.Flags & sDF_Ranges));
        if(ic==0)
        {
            glDrawArraysInstanced(topology,geo ? geo->VertexOffset : 0,vc,dp.InstanceCount);
            GLERR();
        }
        else
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,geo ? geo->Index->GLName : 0);
            GLERR();
            glDrawElementsInstanced(topology,ic,is,(const void *)uptr(ic + dp.IndexOffset),dp.InstanceCount);
            GLERR();
        }
    }
    else
#endif
    {
        if(dp.Flags & sDF_Ranges)
        {
            sASSERT(geo);       // can't combine arrays and ranges.
            if(sGLES)
            {
                if(geo->Index==0)
                {
                    for(int i=0;i<dp.RangeCount;i++)
                    {
                        int i0 = geo->VertexOffset + dp.Ranges[i].Start;
                        int ic = dp.Ranges[i].End-dp.Ranges[i].Start;
                        glDrawArrays(geo->Topology,i0,ic);
                        GLERR();
                    }
                }
                else
                {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,geo->Index->GLName);
                    GLERR();
                    for(int i=0;i<dp.RangeCount;i++)
                    {
                        int i0 = geo->VertexOffset + dp.Ranges[i].Start;
                        int ic = dp.Ranges[i].End-dp.Ranges[i].Start;
                        glDrawElements(geo->Topology,ic,is,(const void *)uptr(i0*2+dp.IndexOffset));
                        GLERR();
                    }
                }        
            }
            else
            {
#if !sGLES
                if(geo->Index==0)
                {
                    DrawRangeCount.SetSize(dp.RangeCount);
                    DrawRangeVertex.SetSize(dp.RangeCount);
                    for(int i=0;i<dp.RangeCount;i++)
                    {
                        DrawRangeVertex[i] = dp.Ranges[i].Start;
                        DrawRangeCount[i] = dp.Ranges[i].End-dp.Ranges[i].Start;
                    }
                    glMultiDrawArrays(geo->Topology,DrawRangeVertex.GetData(),DrawRangeCount.GetData(),dp.RangeCount);
                    GLERR();
                }
                else
                {
                    DrawRangeCount.SetSize(dp.RangeCount);
                    DrawRangeIndex.SetSize(dp.RangeCount);
                    for(int i=0;i<dp.RangeCount;i++)
                    {
                        DrawRangeIndex[i] = (GLvoid *)uptr(dp.Ranges[i].Start*2+dp.IndexOffset);
                        DrawRangeCount[i] = dp.Ranges[i].End-dp.Ranges[i].Start;
                    }
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,geo->Index->GLName);
                    GLERR();
                    GLsizei *counts = DrawRangeCount.GetData();
                    GLvoid **offsets = DrawRangeIndex.GetData();
                    glMultiDrawElements(geo->Topology,counts,is,(const GLvoid **)offsets,dp.RangeCount);
                    GLERR();
                }
#endif
            }
        }
        else
        {
            if(ic==0)
            {
                glDrawArrays(topology,geo ? geo->VertexOffset : 0,vc);
                GLERR();
            }
            else
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,geo ? geo->Index->GLName : 0);
                GLERR();
                glDrawElements(topology,ic,is,(const void *)uptr(ip + dp.IndexOffset));
                GLERR();
            }
        }
    }
    // 
}

/****************************************************************************/
/***                                                                      ***/
/***   Targets                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sContextPrivate::sContextPrivate()
{
    BlitMtrl = 0;
    BlitCB = 0;
}

sContextPrivate::~sContextPrivate()
{
    delete BlitMtrl;
    auto cb = (sCBuffer<sFixedMaterialVC> *) BlitCB;
    delete cb;
}

void sContext::BeginTarget(const sTargetPara &para)
{
    bool intd = para.Depth && (para.Depth->Para.Flags & sRES_Internal);
    bool intc = para.Target[0] && (para.Target[0]->Para.Flags & sRES_Internal);
    sASSERT(!InsideBeginTarget);
    InsideBeginTarget = true;
    if(intd && intc)
    {
        glBindFramebuffer(GL_FRAMEBUFFER,GlSysFrameBuffer);
        GLERR();
    }
    else
    {
        if(intd || intc)
            sFatal("can not mix internal and user created depth / color targets");

        glBindFramebuffer(GL_FRAMEBUFFER,GlRtFrameBuffer);
        
        if(para.Target[0])
        {
            sASSERT(para.Target[0]->Para.Mode & sRBM_ColorTarget);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,para.Target[0]->GLName,0);
            GLERR();
        }
        else
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,0,0);
            GLERR();
        }
        if(para.Depth)
        {
            sASSERT(para.Depth->Para.Mode & sRBM_DepthTarget);

            if (para.Depth->MainBind==sRBM_DepthTarget)
            {
                glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,para.Depth->GLName);            
            }
            else
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,para.Depth->GLName,0);
                glFramebufferTexture2D(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,GL_TEXTURE_2D,para.Depth->GLName,0);            
            }
            GLERR();
        }
        else
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
            GLERR();
        }
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            sFatal("Render to Texture error");
        }
    }

    glEnable(GL_SCISSOR_TEST);
    GLERR();
    glColorMask(1,1,1,1);    
    GLERR();
    glViewport(para.Window.x0,para.SizeY-para.Window.y1,para.Window.SizeX(),para.Window.SizeY());
    GLERR();
    glScissor(para.Window.x0,para.SizeY-para.Window.y1,para.Window.SizeX(),para.Window.SizeY());    
    GLERR();
    glClearColor(para.ClearColor[0].x,para.ClearColor[0].y,para.ClearColor[0].z,para.ClearColor[0].w);
    GLERR();
#if sConfigPlatform == sConfigPlatformWin   //Hack!! For some reason on some Intel OpenGL Driver this function is 0
    if (glClearDepthf!=0)
#endif
        glClearDepthf(para.ClearZ);
    GLERR();
    glDepthMask(1);
    GLERR();
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    GLERR();

    int cf = 0;
    if(para.Flags & sTAR_ClearColor) cf |= GL_COLOR_BUFFER_BIT;
    if(para.Flags & sTAR_ClearDepth) cf |= GL_DEPTH_BUFFER_BIT;
    if(cf)
    {
        glClear(cf);
        GLERR();
    }
}

void sContext::EndTarget()
{
    sASSERT(InsideBeginTarget);
    InsideBeginTarget = false;
    glBindFramebuffer(GL_FRAMEBUFFER,GlSysFrameBuffer);
}

void sContext::SetScissor(bool enable,const sRect &r)
{
}

void sContext::Resolve(sResource *dest,sResource *src)
{
    // multisampling not implemented, just copy
    // issues: not working on some Android devices
    
    sASSERT(src->Para.SizeX == dest->Para.SizeX);
    sASSERT(src->Para.SizeY == dest->Para.SizeY);
    sASSERT(src->Para.Mode & sRBM_Shader);
    sASSERT(dest->Para.Mode & sRBM_Shader);
    
    glBindFramebuffer(GL_FRAMEBUFFER,GlRtFrameBuffer);
    GLERR();
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,src->GLName,0);
    GLERR();
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
    GLERR();

    glBindTexture(GL_TEXTURE_2D,dest->GLName);
    GLERR();
    glCopyTexImage2D(GL_TEXTURE_2D,0,dest->GLInternalFormat,0,0,dest->Para.SizeX,dest->Para.SizeY,0);
    GLERR();

    glBindTexture(GL_TEXTURE_2D,0);
    GLERR();
    glBindFramebuffer(GL_FRAMEBUFFER,GlSysFrameBuffer);
    GLERR(); 
}

void sContext::Copy(const sCopyTexturePara &cp)
{
    auto src = cp.Source;
    auto dest = cp.Dest;

    // when blitting to framebuffer or stretching, use real blit

    if(dest->GLName==0 || cp.SourceRect.SizeX()!=cp.DestRect.SizeX() || cp.SourceRect.SizeY()!=cp.DestRect.SizeY())
    {
        if(BlitMtrl==0)
        {
            BlitMtrl = new sFixedMaterial(Adapter);
            BlitMtrl->SetTexturePS(0,0,sSamplerStatePara(sTF_Clamp|sTF_Point,0));
            BlitMtrl->SetState(sRenderStatePara(sMTRL_CullOff|sMTRL_DepthOff,sMB_Off));
            BlitMtrl->Prepare(Adapter->FormatPT);

            BlitCB = new sCBuffer<sFixedMaterialVC>(Adapter,sST_Vertex,0);
        }
        auto cb = (sCBuffer<sFixedMaterialVC> *) BlitCB;
        cb->Map();
        cb->Data->MS2SS.SetViewportPixels(dest->Para.SizeX,dest->Para.SizeY);
        cb->Unmap();
        sVertexPT p0,p1;
        p0.Set(float(cp.DestRect.x0),float(cp.DestRect.y0),0,0,0);
        p1.Set(float(cp.DestRect.x1),float(cp.DestRect.y1),0,1,1);
        BlitMtrl->UpdateTexturePS(0,src);
        Adapter->DynamicGeometry->Quad(BlitMtrl,p0,p1,cb);
    }
    else
    {
//        sFatal("not tested!");
        glBindFramebuffer(GL_FRAMEBUFFER,GlRtFrameBuffer);
        GLERR();
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,src->GLName,0);
        GLERR();
        glBindTexture(GL_TEXTURE_2D,dest->GLName);
        GLERR();

        glCopyTexImage2D(GL_TEXTURE_2D,0,dest->GLInternalFormat,0,0,dest->Para.SizeX,dest->Para.SizeY,0);
        GLERR();

        glBindFramebuffer(GL_FRAMEBUFFER,0);
        GLERR();
        glBindTexture(GL_TEXTURE_2D,0);
        GLERR();
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Queries (not implemented)                                          ***/
/***                                                                      ***/
/****************************************************************************/

sGpuQuery::sGpuQuery(sAdapter *adapter,sGpuQueryKind kind)
{
    Adapter = adapter;
    Kind = kind;
}

sGpuQuery::~sGpuQuery()
{
}

void sGpuQuery::Begin(sContext *ctx)
{
}

void sGpuQuery::End(sContext *ctx)
{
}

void sGpuQuery::Issue(sContext *ctx)
{
}

bool sGpuQuery::GetData(uint &data)
{
    data = 1;
    return 1;
}

bool sGpuQuery::GetData(uint64 &data)
{
    data = 1;
    return 1;
}

bool sGpuQuery::GetData(sPipelineStats &stat)
{
    sASSERT0();
    return 1;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

#else

int ALTONA2_LIBS_BASE_GRAPHICS_GL2_CPP_STUB;

#endif // gl2 | gles2

}

//#endif // apple disable


