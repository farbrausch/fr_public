/*+**************************************************************************/
/***                                                                      ***/
/***   Native macOS Cocoa / OpenGL 2.1 Backend for Altona 1 / Werkkzeug 4 ***/
/***                                                                      ***/
/**************************************************************************+*/

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <Carbon/Carbon.h>
#import <CoreText/CoreText.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#include <sys/time.h>
#include <unistd.h>

#include "base/types.hpp"
#include "base/types2.hpp"
#include "base/system.hpp"
#include "base/windows.hpp"
#include "base/graphics.hpp"
#include "base/input2.hpp"
#include "base/sound.hpp"
#include "wz4lib/videoencoder.hpp"
#include "util/image.hpp"

// Forward declarations from Altona 1
extern void PreInitGFX(sInt &flags, sInt &xs, sInt &ys);
extern void InitGFX(sInt flags, sInt xs, sInt ys);
extern void ExitGFX();
extern void ResizeGFX(sInt x, sInt y);
extern void sInitGfxCommon();
extern void sExitGfxCommon();

extern sInt sExitFlag;
extern sApp *sAppPtr;
extern sU32 sKeyQual;
extern sHooks *sFrameHook;

void sLinuxFromWide(char *dest, const sChar *src, int size);
void sLinuxToWide(sChar *dest, const char *src, int size);

static sString<256> sMacWindowName;
static sInt sMacWindowWidth = 1280;
static sInt sMacWindowHeight = 720;
static sBool sMacWindowHasFocus = sTRUE;
static sString<2048> sMacDragDropFile;

@interface Altona1GLView : NSOpenGLView
{
@public
  NSOpenGLContext *GLContext;
}
- (void)drawRect:(NSRect)dirtyRect;
@end

@interface Altona1WindowDelegate : NSObject <NSWindowDelegate>
@end

static NSApplication *sMacApp = nil;
static NSWindow *sMacWindow = nil;
static Altona1GLView *sMacGLView = nil;
static Altona1WindowDelegate *sMacWinDelegate = nil;
static NSCursor *sMacCursors[sMP_MAX];

@implementation Altona1GLView

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (void)drawRect:(NSRect)dirtyRect
{
}

- (void)reshape
{
  [super reshape];
  NSRect bounds = [self bounds];
  sMacWindowWidth = (sInt)bounds.size.width;
  sMacWindowHeight = (sInt)bounds.size.height;
  ResizeGFX(sMacWindowWidth, sMacWindowHeight);
}

@end

@implementation Altona1WindowDelegate

- (void)windowDidResize:(NSNotification *)notification
{
  if (sMacGLView)
    [sMacGLView reshape];
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
  sMacWindowHasFocus = sTRUE;
}

- (void)windowDidResignKey:(NSNotification *)notification
{
  sMacWindowHasFocus = sFALSE;
}

- (BOOL)windowShouldClose:(NSWindow *)sender
{
  sExit();
  return YES;
}

@end

// OpenGL context hooks
void sMacInitGL()
{
  if (sMacGLView && sMacGLView->GLContext)
  {
    [sMacGLView->GLContext setView:sMacGLView];
    [sMacGLView->GLContext makeCurrentContext];
  }
}

void sMacExitGL()
{
  [NSOpenGLContext clearCurrentContext];
}

void sMacSwapBuffers()
{
  if (sMacGLView && sMacGLView->GLContext)
  {
    [sMacGLView->GLContext flushBuffer];
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Window management functions                                        ***/
/***                                                                      ***/
/****************************************************************************/

void sSetWindowName(const sChar *name)
{
  sMacWindowName = name;
  if (sMacWindow)
  {
    char buf[512];
    sLinuxFromWide(buf, name, sizeof(buf));
    NSString *title = [NSString stringWithUTF8String:buf];
    [sMacWindow setTitle:title];
  }
}

const sChar *sGetWindowName()
{
  return sMacWindowName;
}

void sSetWindowMode(sInt mode)
{
  if (!sMacWindow) return;
  switch (mode)
  {
    case sWM_NORMAL:
      if ([sMacWindow isZoomed]) [sMacWindow zoom:nil];
      break;
    case sWM_MAXIMIZED:
      if (![sMacWindow isZoomed]) [sMacWindow zoom:nil];
      break;
    case sWM_MINIMIZED:
      [sMacWindow miniaturize:nil];
      break;
  }
}

sInt sGetWindowMode()
{
  if (!sMacWindow) return sWM_NORMAL;
  if ([sMacWindow isMiniaturized]) return sWM_MINIMIZED;
  if ([sMacWindow isZoomed]) return sWM_MAXIMIZED;
  return sWM_NORMAL;
}

sBool sHasWindowFocus()
{
  return sMacWindowHasFocus;
}

void sSetWindowSize(sInt x, sInt y)
{
  sMacWindowWidth = x;
  sMacWindowHeight = y;
  if (sMacWindow)
  {
    NSRect frame = [sMacWindow frame];
    frame.size = NSMakeSize(x, y);
    [sMacWindow setFrame:frame display:YES];
  }
}

void sSetWindowPos(sInt x, sInt y)
{
  if (sMacWindow)
  {
    NSRect screenFrame = [[NSScreen mainScreen] frame];
    NSRect frame = [sMacWindow frame];
    frame.origin.x = x;
    frame.origin.y = screenFrame.size.height - y - frame.size.height;
    [sMacWindow setFrame:frame display:YES];
  }
}

void sGetWindowPos(sInt &x, sInt &y)
{
  if (sMacWindow)
  {
    NSRect screenFrame = [[NSScreen mainScreen] frame];
    NSRect frame = [sMacWindow frame];
    x = (sInt)frame.origin.x;
    y = (sInt)(screenFrame.size.height - frame.origin.y - frame.size.height);
  }
  else
  {
    x = 0; y = 0;
  }
}

void sGetWindowSize(sInt &sx, sInt &sy)
{
  if (sMacWindow)
  {
    NSRect frame = [sMacWindow contentRectForFrameRect:[sMacWindow frame]];
    sx = (sInt)frame.size.width;
    sy = (sInt)frame.size.height;
  }
  else
  {
    sx = sMacWindowWidth;
    sy = sMacWindowHeight;
  }
}

void sUpdateWindow()
{
}

void sUpdateWindow(const sRect &)
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Mouse Cursor & Pointer                                             ***/
/***                                                                      ***/
/****************************************************************************/

void sSetMousePointer(sInt code)
{
  if (code >= 0 && code < sMP_MAX && sMacCursors[code])
  {
    [sMacCursors[code] set];
  }
  else if (code == sMP_OFF)
  {
    [NSCursor hide];
  }
}

static void InitCursors()
{
  sMacCursors[sMP_OFF] = nil;
  sMacCursors[sMP_ARROW] = [NSCursor arrowCursor];
  sMacCursors[sMP_WAIT] = [NSCursor arrowCursor];
  sMacCursors[sMP_CROSSHAIR] = [NSCursor crosshairCursor];
  sMacCursors[sMP_HAND] = [NSCursor pointingHandCursor];
  sMacCursors[sMP_TEXT] = [NSCursor IBeamCursor];
  sMacCursors[sMP_NO] = [NSCursor operationNotAllowedCursor];
  sMacCursors[sMP_SIZEALL] = [NSCursor openHandCursor];
  sMacCursors[sMP_SIZENS] = [NSCursor resizeUpDownCursor];
  sMacCursors[sMP_SIZEWE] = [NSCursor resizeLeftRightCursor];
  sMacCursors[sMP_SIZENWSE] = [NSCursor resizeUpDownCursor];
  sMacCursors[sMP_SIZENESW] = [NSCursor resizeLeftRightCursor];
}

/****************************************************************************/
/***                                                                      ***/
/***   Clipboard                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sSetClipboard(const sChar *str, sInt len)
{
  if (!str) return;
  char buf[65536];
  sLinuxFromWide(buf, str, sizeof(buf));
  NSString *nsStr = [NSString stringWithUTF8String:buf];
  NSPasteboard *pb = [NSPasteboard generalPasteboard];
  [pb clearContents];
  [pb setString:nsStr forType:NSPasteboardTypeString];
}

sChar *sGetClipboard()
{
  NSPasteboard *pb = [NSPasteboard generalPasteboard];
  NSString *nsStr = [pb stringForType:NSPasteboardTypeString];
  if (!nsStr) return 0;
  const char *utf8 = [nsStr UTF8String];
  if (!utf8) return 0;
  sInt len = (sInt)strlen(utf8);
  sChar *result = new sChar[len + 1];
  sLinuxToWide(result, utf8, len + 1);
  return result;
}

static sArray<sU8> sMacClipData;
void sSetClipboard(const sU8 *data, sDInt size, sU32 serid, sInt sermode)
{
  sMacClipData.Resize(size);
  sCopyMem(sMacClipData.GetData(), data, size);
}

sU8 *sGetClipboard(sDInt &size, sU32 serid, sInt sermode)
{
  size = sMacClipData.GetCount();
  if (size == 0) return 0;
  sU8 *p = new sU8[size];
  sCopyMem(p, sMacClipData.GetData(), size);
  return p;
}

void sEnableFileDrop(sBool enable)
{
}

const sChar *sGetDragDropFile()
{
  return sMacDragDropFile;
}

/****************************************************************************/
/***                                                                      ***/
/***   Key Translation                                                    ***/
/***                                                                      ***/
/****************************************************************************/

static sU32 TranslateKey(NSEvent *event)
{
  unsigned short code = [event keyCode];
  switch (code)
  {
    case kVK_Return:        return sKEY_ENTER;
    case kVK_Tab:           return sKEY_TAB;
    case kVK_Space:         return ' ';
    case kVK_Delete:        return sKEY_BACKSPACE;
    case kVK_Escape:        return sKEY_ESCAPE;
    case kVK_ForwardDelete: return sKEY_DELETE;
    case kVK_LeftArrow:     return sKEY_LEFT;
    case kVK_RightArrow:    return sKEY_RIGHT;
    case kVK_UpArrow:       return sKEY_UP;
    case kVK_DownArrow:     return sKEY_DOWN;
    case kVK_PageUp:        return sKEY_PAGEUP;
    case kVK_PageDown:      return sKEY_PAGEDOWN;
    case kVK_Home:          return sKEY_HOME;
    case kVK_End:           return sKEY_END;
    case kVK_F1:            return sKEY_F1;
    case kVK_F2:            return sKEY_F2;
    case kVK_F3:            return sKEY_F3;
    case kVK_F4:            return sKEY_F4;
    case kVK_F5:            return sKEY_F5;
    case kVK_F6:            return sKEY_F6;
    case kVK_F7:            return sKEY_F7;
    case kVK_F8:            return sKEY_F8;
    case kVK_F9:            return sKEY_F9;
    case kVK_F10:           return sKEY_F10;
    case kVK_F11:           return sKEY_F11;
    case kVK_F12:           return sKEY_F12;
    default:
    {
      NSString *chars = [event charactersIgnoringModifiers];
      if ([chars length] > 0)
      {
        unichar c = [chars characterAtIndex:0];
        if (c >= 32 && c < 127)
        {
          if (c >= 'A' && c <= 'Z')
            c = c - 'A' + 'a';
          return c;
        }
      }
      return 0;
    }
  }
}

static void UpdateKeyQualifiers(NSEventModifierFlags flags)
{
  sKeyQual = 0;
  if (flags & NSEventModifierFlagShift)   sKeyQual |= sKEYQ_SHIFT;
  if (flags & NSEventModifierFlagControl) sKeyQual |= sKEYQ_CTRL;
  if (flags & NSEventModifierFlagOption)  sKeyQual |= sKEYQ_ALT;
  if (flags & NSEventModifierFlagCommand) sKeyQual |= sKEYQ_CTRL; // Map Command to Ctrl for standard shortcuts
}

sU32 sGetKeyQualifier()
{
  return sKeyQual;
}

sInt sMakeUnshiftedKey(sInt ascii)
{
  ascii &= sKEYQ_MASK;
  if (ascii >= 'A' && ascii <= 'Z') return ascii - 'A' + 'a';
  if (ascii == '!') return '1';
  if (ascii == '"') return '2';
  if (ascii == 0xa7) return '3';
  if (ascii == '$') return '4';
  if (ascii == '%') return '5';
  if (ascii == '&') return '6';
  if (ascii == '/') return '7';
  if (ascii == '(') return '8';
  if (ascii == ')') return '9';
  if (ascii == '=') return '0';
  return ascii;
}

/****************************************************************************/
/***                                                                      ***/
/***   Cocoa Event Processing Loop                                        ***/
/***                                                                      ***/
/****************************************************************************/

static void ProcessCocoaEvents()
{
  NSEvent *event;
  while ((event = [sMacApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES]))
  {
    NSEventType type = [event type];
    UpdateKeyQualifiers([event modifierFlags]);

    switch (type)
    {
      case NSEventTypeKeyDown:
      {
        sU32 key = TranslateKey(event);
        if (key)
        {
          sInput2SendEvent(sInput2Event(key | sKeyQual, 0, 0));
        }
        break;
      }
      case NSEventTypeKeyUp:
      {
        sU32 key = TranslateKey(event);
        if (key)
        {
          sInput2SendEvent(sInput2Event(key | sKeyQual | sKEYQ_BREAK, 0, 0));
        }
        break;
      }
      case NSEventTypeFlagsChanged:
      {
        break;
      }
      case NSEventTypeMouseMoved:
      case NSEventTypeLeftMouseDragged:
      case NSEventTypeRightMouseDragged:
      case NSEventTypeOtherMouseDragged:
      {
        NSPoint p = [sMacGLView convertPoint:[event locationInWindow] fromView:nil];
        sInt mx = (sInt)p.x;
        sInt my = sMacWindowHeight - (sInt)p.y;
        sInput2SendEvent(sInput2Event(sKEY_MOUSEMOVE, 0, ((mx & 0xffff) << 16) | (my & 0xffff)));
        break;
      }
      case NSEventTypeLeftMouseDown:
      {
        NSPoint p = [sMacGLView convertPoint:[event locationInWindow] fromView:nil];
        sInt mx = (sInt)p.x;
        sInt my = sMacWindowHeight - (sInt)p.y;
        sInput2SendEvent(sInput2Event(sKEY_MOUSEMOVE, 0, ((mx & 0xffff) << 16) | (my & 0xffff)));
        sInput2SendEvent(sInput2Event(sKEY_LMB | sKeyQual, 0, 0));
        break;
      }
      case NSEventTypeLeftMouseUp:
      {
        NSPoint p = [sMacGLView convertPoint:[event locationInWindow] fromView:nil];
        sInt mx = (sInt)p.x;
        sInt my = sMacWindowHeight - (sInt)p.y;
        sInput2SendEvent(sInput2Event(sKEY_MOUSEMOVE, 0, ((mx & 0xffff) << 16) | (my & 0xffff)));
        sInput2SendEvent(sInput2Event(sKEY_LMB | sKeyQual | sKEYQ_BREAK, 0, 0));
        break;
      }
      case NSEventTypeRightMouseDown:
      {
        NSPoint p = [sMacGLView convertPoint:[event locationInWindow] fromView:nil];
        sInt mx = (sInt)p.x;
        sInt my = sMacWindowHeight - (sInt)p.y;
        sInput2SendEvent(sInput2Event(sKEY_MOUSEMOVE, 0, ((mx & 0xffff) << 16) | (my & 0xffff)));
        sInput2SendEvent(sInput2Event(sKEY_RMB | sKeyQual, 0, 0));
        break;
      }
      case NSEventTypeRightMouseUp:
      {
        NSPoint p = [sMacGLView convertPoint:[event locationInWindow] fromView:nil];
        sInt mx = (sInt)p.x;
        sInt my = sMacWindowHeight - (sInt)p.y;
        sInput2SendEvent(sInput2Event(sKEY_MOUSEMOVE, 0, ((mx & 0xffff) << 16) | (my & 0xffff)));
        sInput2SendEvent(sInput2Event(sKEY_RMB | sKeyQual | sKEYQ_BREAK, 0, 0));
        break;
      }
      case NSEventTypeOtherMouseDown:
      {
        NSPoint p = [sMacGLView convertPoint:[event locationInWindow] fromView:nil];
        sInt mx = (sInt)p.x;
        sInt my = sMacWindowHeight - (sInt)p.y;
        sInput2SendEvent(sInput2Event(sKEY_MOUSEMOVE, 0, ((mx & 0xffff) << 16) | (my & 0xffff)));
        sInput2SendEvent(sInput2Event(sKEY_MMB | sKeyQual, 0, 0));
        break;
      }
      case NSEventTypeOtherMouseUp:
      {
        NSPoint p = [sMacGLView convertPoint:[event locationInWindow] fromView:nil];
        sInt mx = (sInt)p.x;
        sInt my = sMacWindowHeight - (sInt)p.y;
        sInput2SendEvent(sInput2Event(sKEY_MOUSEMOVE, 0, ((mx & 0xffff) << 16) | (my & 0xffff)));
        sInput2SendEvent(sInput2Event(sKEY_MMB | sKeyQual | sKEYQ_BREAK, 0, 0));
        break;
      }
      case NSEventTypeScrollWheel:
      {
        CGFloat dy = [event scrollingDeltaY];
        if (dy > 0.1f)
        {
          sInput2SendEvent(sInput2Event(sKEY_WHEELUP | sKeyQual, 0, 0));
        }
        else if (dy < -0.1f)
        {
          sInput2SendEvent(sInput2Event(sKEY_WHEELDOWN | sKeyQual, 0, 0));
        }
        break;
      }
      default:
        break;
    }

    [sMacApp sendEvent:event];
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   2D Graphics Primitives                                             ***/
/***                                                                      ***/
/****************************************************************************/

static sU32 sMacColors[64];

void sSetColor2D(sInt colid, sU32 color)
{
  if (colid >= 0 && colid < 64)
    sMacColors[colid] = color;
}

sU32 sGetColor2D(sInt colid)
{
  if (colid >= 0 && colid < 64)
    return sMacColors[colid];
  return 0;
}

static inline void SetGLColor(sInt colid)
{
  sU32 col = sGetColor2D(colid);
  sF32 r = ((col >> 16) & 0xff) / 255.0f;
  sF32 g = ((col >> 8) & 0xff) / 255.0f;
  sF32 b = (col & 0xff) / 255.0f;
  sF32 a = ((col >> 24) & 0xff) / 255.0f;
  if (a == 0.0f) a = 1.0f;
  glColor4f(r, g, b, a);
}

void sRect2D(sInt x0, sInt y0, sInt x1, sInt y1, sInt colid)
{
  glDisable(GL_TEXTURE_2D);
  SetGLColor(colid);
  glBegin(GL_QUADS);
  glVertex2i(x0, y0);
  glVertex2i(x1, y0);
  glVertex2i(x1, y1);
  glVertex2i(x0, y1);
  glEnd();
}

void sRect2D(const sRect &r, sInt colid)
{
  sRect2D(r.x0, r.y0, r.x1, r.y1, colid);
}

void sRectInvert2D(sInt x0, sInt y0, sInt x1, sInt y1)
{
  glEnable(GL_COLOR_LOGIC_OP);
  glLogicOp(GL_INVERT);
  glBegin(GL_QUADS);
  glVertex2i(x0, y0);
  glVertex2i(x1, y0);
  glVertex2i(x1, y1);
  glVertex2i(x0, y1);
  glEnd();
  glDisable(GL_COLOR_LOGIC_OP);
}

void sRectInvert2D(const sRect &r)
{
  sRectInvert2D(r.x0, r.y0, r.x1, r.y1);
}

void sRectFrame2D(sInt x0, sInt y0, sInt x1, sInt y1, sInt colid)
{
  glDisable(GL_TEXTURE_2D);
  SetGLColor(colid);
  glBegin(GL_LINE_LOOP);
  glVertex2i(x0, y0);
  glVertex2i(x1 - 1, y0);
  glVertex2i(x1 - 1, y1 - 1);
  glVertex2i(x0, y1 - 1);
  glEnd();
}

void sRectFrame2D(const sRect &r, sInt colid)
{
  sRectFrame2D(r.x0, r.y0, r.x1, r.y1, colid);
}

void sRectHole2D(const sRect &out, const sRect &hole, sInt colid)
{
  if (hole.y0 > out.y0) sRect2D(out.x0, out.y0, out.x1, hole.y0, colid);
  if (hole.y1 < out.y1) sRect2D(out.x0, hole.y1, out.x1, out.y1, colid);
  if (hole.x0 > out.x0) sRect2D(out.x0, hole.y0, hole.x0, hole.y1, colid);
  if (hole.x1 < out.x1) sRect2D(hole.x1, hole.y0, out.x1, hole.y1, colid);
}

void sLine2D(sInt x0, sInt y0, sInt x1, sInt y1, sInt colid)
{
  glDisable(GL_TEXTURE_2D);
  SetGLColor(colid);
  glBegin(GL_LINES);
  glVertex2i(x0, y0);
  glVertex2i(x1, y1);
  glEnd();
}

void sLine2D(sInt *list, sInt count, sInt colid)
{
  glDisable(GL_TEXTURE_2D);
  SetGLColor(colid);
  glBegin(GL_LINES);
  for (sInt i = 0; i < count; i++)
  {
    glVertex2i(list[i * 4 + 0], list[i * 4 + 1]);
    glVertex2i(list[i * 4 + 2], list[i * 4 + 3]);
  }
  glEnd();
}

void sLineList2D(sInt *list, sInt count, sInt colid)
{
  glDisable(GL_TEXTURE_2D);
  SetGLColor(colid);
  glBegin(GL_LINE_STRIP);
  for (sInt i = 0; i < count; i++)
    glVertex2i(list[i * 2 + 0], list[i * 2 + 1]);
  glEnd();
}

// Image blits go through a scratch texture. This is how the editor shows every
// wz3 bitmap: the preview is a plain 2D stretch, not a 3D pass, so leaving
// these empty left the whole bitmap half of the tool blank.
static GLuint sMacBlitTex = 0;

static void sMacDrawImage(const sU32 *data,sInt width,const sRect &src,const sRect &dest)
{
  sInt sx = src.SizeX(), sy = src.SizeY();
  if(!data || sx<=0 || sy<=0 || dest.SizeX()<=0 || dest.SizeY()<=0) return;

  if(!sMacBlitTex)
    glGenTextures(1,&sMacBlitTex);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, sMacBlitTex);

  // upload just the requested sub-rectangle out of the source image
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, src.x0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, src.y0);
  // altona colours are 0xAARRGGBB, which is exactly BGRA/8888_REV
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,sx,sy,0,GL_BGRA,GL_UNSIGNED_INT_8_8_8_8_REV,data);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

  // the image's own alpha must not blend it away -- the preview is opaque
  glDisable(GL_BLEND);
  glColor4f(1,1,1,1);
  glBegin(GL_QUADS);
  glTexCoord2f(0,0); glVertex2i(dest.x0,dest.y0);
  glTexCoord2f(1,0); glVertex2i(dest.x1,dest.y0);
  glTexCoord2f(1,1); glVertex2i(dest.x1,dest.y1);
  glTexCoord2f(0,1); glVertex2i(dest.x0,dest.y1);
  glEnd();
  glEnable(GL_BLEND);

  glBindTexture(GL_TEXTURE_2D,0);
  glDisable(GL_TEXTURE_2D);
}

void sBlit2D(const sU32 *data, sInt width, const sRect &dest)
{
  sRect src(0,0,dest.SizeX(),dest.SizeY());
  sMacDrawImage(data,width,src,dest);
}

void sStretch2D(const sU32 *data, sInt width, const sRect &source, const sRect &dest)
{
  sMacDrawImage(data,width,source,dest);
}

static sArray<sRect> sMacClipStack;

// Altona works in window points; glViewport and glScissor take framebuffer
// pixels. On a retina display those differ by the backing scale factor, so
// every rect has to be scaled on the way out -- otherwise everything ends up
// squeezed into the bottom left quarter of the window. Shared with the OGL2
// renderer, which sets the 3D viewport.
sF32 sMacBackingScale()
{
  if (!sMacGLView) return 1.0f;
  sF32 s = (sF32)[sMacGLView convertRectToBacking:NSMakeRect(0, 0, 1, 1)].size.width;
  return (s > 0.0f) ? s : 1.0f;
}

static void sMacSetScissor(const sRect &r)
{
  sF32 s = sMacBackingScale();
  GLint x0 = (GLint)(r.x0 * s);
  GLint x1 = (GLint)(r.x1 * s);
  // GL counts scissor y from the bottom of the framebuffer
  GLint y0 = (GLint)((sMacWindowHeight - r.y1) * s);
  GLint y1 = (GLint)((sMacWindowHeight - r.y0) * s);
  glEnable(GL_SCISSOR_TEST);
  glScissor(x0, y0, sMax(0, (sInt)(x1 - x0)), sMax(0, (sInt)(y1 - y0)));
}

/****************************************************************************/

// Altona paints front to back: a window draws its contents, subtracts each
// finished rectangle from the clip region with sClipExclude, and only then
// fills its own background over whatever is left. A scissor rect cannot hold
// a region with holes, so the exclusions live in the stencil buffer instead:
// one bit per clip nesting level, set over the excluded rectangle, and all 2D
// drawing is masked to "no bit set". Popping a level clears its bit again.
//
// Without this the background fill at the end of, say, WinStack::OnPaint2D
// erases every operator box it was supposed to paint around.

enum { sMacClipBits = 8 };              // one stencil bit per nesting level
static sBool sMacStencilOk = sFALSE;

static sInt sMacClipBit()
{
  return sMin((sInt)sMacClipStack.GetCount(), (sInt)sMacClipBits - 1);
}

// Draw a rectangle into the stencil buffer only, setting or clearing one bit.
static void sMacStencilRect(const sRect &r, sInt bit, sBool set)
{
  if (!sMacStencilOk) return;

  glEnable(GL_STENCIL_TEST);
  glStencilMask(1 << bit);              // REPLACE only touches this one bit
  glStencilFunc(GL_ALWAYS, set ? (1 << bit) : 0, 0xff);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDisable(GL_TEXTURE_2D);

  glBegin(GL_QUADS);
  glVertex2i(r.x0, r.y0);
  glVertex2i(r.x1, r.y0);
  glVertex2i(r.x1, r.y1);
  glVertex2i(r.x0, r.y1);
  glEnd();

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilMask(0);                     // back to read-only for normal drawing
  glStencilFunc(GL_EQUAL, 0, 0xff);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

// Called once per 2D pass to arm the mask and drop last frame's exclusions.
void sMacClipInitFrame()
{
  GLint bits = 0;
  glGetIntegerv(GL_STENCIL_BITS, &bits);
  sMacStencilOk = (bits >= sMacClipBits);

  sMacClipStack.Clear();
  glDisable(GL_SCISSOR_TEST);

  if (sMacStencilOk)
  {
    glStencilMask(0xff);
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);     // scissor is off, so this covers all
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0);
  }
  else
  {
    glDisable(GL_STENCIL_TEST);
  }
}

void sClipFlush()
{
  sMacClipInitFrame();
}

void sClipPush()
{
  sRect cur(0, 0, sMacWindowWidth, sMacWindowHeight);
  if (sMacClipStack.GetCount() > 0)
    cur = sMacClipStack[sMacClipStack.GetCount() - 1];
  sMacClipStack.AddTail(cur);
}

void sClipPop()
{
  if (sMacClipStack.GetCount() > 0)
  {
    // release every exclusion made at this level before leaving it
    sRect full(0, 0, sMacWindowWidth, sMacWindowHeight);
    sMacStencilRect(full, sMacClipBit(), sFALSE);

    sMacClipStack.RemTail();
    if (sMacClipStack.GetCount() > 0)
      sMacSetScissor(sMacClipStack[sMacClipStack.GetCount() - 1]);
    else
      glDisable(GL_SCISSOR_TEST);
  }
}

void sClipExclude(const sRect &r)
{
  if (r.x1 > r.x0 && r.y1 > r.y0)
    sMacStencilRect(r, sMacClipBit(), sTRUE);
}

void sClipRect(const sRect &r)
{
  if (sMacClipStack.GetCount() == 0)
    sMacClipStack.AddTail(r);
  else
    sMacClipStack[sMacClipStack.GetCount() - 1].And(r);

  sMacSetScissor(sMacClipStack[sMacClipStack.GetCount() - 1]);
}

void sRender2DBegin()
{
  // The 3D pass runs first and leaves its own program, buffers, render target
  // and viewport bound. The gui is drawn with the fixed-function pipeline
  // against the window itself, so put all of that back first -- a leftover
  // GLSL program in particular would swallow the glOrtho below and place the
  // whole gui wherever its own vertex shader felt like.
  glUseProgram(0);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  // The 3D pass may have left a cube map or volume texture enabled on a unit.
  // Fixed-function sampling picks those over GL_TEXTURE_2D, so the gui would
  // sample the scene's cube map instead of the font atlas -- which comes out
  // as solid black text.
  for (sInt u = 0; u < 8; u++)
  {
    glActiveTexture(GL_TEXTURE0 + u);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  glActiveTexture(GL_TEXTURE0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  for (GLuint i = 0; i < 16; i++)
    glDisableVertexAttribArray(i);

  if (sMacGLView)
  {
    NSRect pix = [sMacGLView convertRectToBacking:[sMacGLView bounds]];
    glViewport(0, 0, (GLsizei)pix.size.width, (GLsizei)pix.size.height);
  }

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, sMacWindowWidth, sMacWindowHeight, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // exclusion masking is armed by sClipFlush; until then draw unmasked
  glDisable(GL_STENCIL_TEST);
  glStencilMask(0);

}

void sRender2DBegin(sInt xs, sInt ys)
{
  sRender2DBegin();
}

void sRender2DBegin(sImage2D *)
{
  sRender2DBegin();
}

void sRender2DEnd()
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_STENCIL_TEST);
  glStencilMask(0xff);                  // leave the buffer writable for 3D
}

void sRender2DSet(sU32 *data)
{
}

void sRender2DGet(sU32 *data)
{
}

/****************************************************************************/
/***                                                                      ***/
/***   sFont2D Implementation                                             ***/
/***                                                                      ***/
/****************************************************************************/

// A font is rasterised once into a glyph atlas texture and then drawn as
// textured quads, one per character. Metrics stay in window points (which is
// what the gui lays out in) while the atlas is rasterised at the backing scale
// so text stays sharp on retina displays.

enum { sMacFontGlyphs = 256 };

struct sFont2DPrivate
{
  CTFontRef Font;                 // point-sized, used for metrics
  sInt Height;
  sInt Baseline;
  sInt CharHeight;
  sInt TextColor;
  sInt BackColor;
  sInt Flags;

  GLuint Tex;                     // glyph atlas, GL_ALPHA
  sInt TexX, TexY;                // atlas size in texels
  sInt CellX, CellY;              // cell size in points
  sInt PadX, PadY;                // pen position inside a cell, in points
  sInt Advance[sMacFontGlyphs];   // advance width in points
};

// Rasterise chars 0..255 into one atlas texture.
static void sMacBuildFontAtlas(sFont2DPrivate *prv)
{
  sF32 scale = 1.0f;
  if (sMacGLView)
    scale = (sF32)[sMacGLView convertRectToBacking:NSMakeRect(0, 0, 1, 1)].size.width;
  if (scale < 1.0f) scale = 1.0f;

  CGFloat ascent = CTFontGetAscent(prv->Font);
  CGFloat descent = CTFontGetDescent(prv->Font);
  CGRect bbox = CTFontGetBoundingBox(prv->Font);

  // One uniform cell for every glyph: wide enough for the font's bounding box
  // plus a texel of slack so neighbouring cells never bleed into each other.
  prv->PadX = (sInt)ceil(sMax(0.0, -bbox.origin.x)) + 1;
  prv->PadY = (sInt)ceil(ascent) + 1;
  prv->CellX = prv->PadX + (sInt)ceil(bbox.origin.x + bbox.size.width) + 2;
  prv->CellY = prv->PadY + (sInt)ceil(descent) + 2;
  if (prv->CellX < 2) prv->CellX = 2;
  if (prv->CellY < 2) prv->CellY = 2;

  sInt cellpx = (sInt)ceil(prv->CellX * scale);
  sInt cellpy = (sInt)ceil(prv->CellY * scale);
  prv->TexX = cellpx * 16;
  prv->TexY = cellpy * 16;

  sU8 *bits = new sU8[prv->TexX * prv->TexY];
  sSetMem(bits, 0, prv->TexX * prv->TexY);

  // alpha-only bitmap: exactly the coverage mask the texture wants
  CGContextRef ctx = CGBitmapContextCreate(bits, prv->TexX, prv->TexY, 8, prv->TexX, NULL, kCGImageAlphaOnly);

  CTFontRef hifont = prv->Font;
  if (scale != 1.0f)
    hifont = CTFontCreateCopyWithAttributes(prv->Font, CTFontGetSize(prv->Font) * scale, NULL, NULL);

  if (ctx)
  {
    CGContextSetShouldAntialias(ctx, true);
    CGContextSetShouldSmoothFonts(ctx, false); // no subpixel AA: we only keep coverage

    for (sInt i = 0; i < sMacFontGlyphs; i++)
    {
      UniChar ch = (UniChar)i;
      CGGlyph glyph = 0;
      if (!CTFontGetGlyphsForCharacters(hifont, &ch, &glyph, 1))
        glyph = 0;

      CGSize adv;
      CGGlyph mglyph = 0;
      CTFontGetGlyphsForCharacters(prv->Font, &ch, &mglyph, 1);
      CTFontGetAdvancesForGlyphs(prv->Font, kCTFontOrientationHorizontal, &mglyph, &adv, 1);
      prv->Advance[i] = (sInt)(adv.width + 0.5f);

      if (!glyph || i < 32) continue;

      sInt col = i & 15, row = i >> 4;
      // CGBitmapContext rows run top-down in memory but its origin is at the
      // bottom left, so flip the row into context space when placing the pen.
      CGFloat px = col * cellpx + prv->PadX * scale;
      CGFloat py = prv->TexY - (row * cellpy + prv->PadY * scale);
      CGPoint pos = CGPointMake(px, py);
      CTFontDrawGlyphs(hifont, &glyph, &pos, 1, ctx);
    }
    CGContextRelease(ctx);
  }

  if (hifont != prv->Font)
    CFRelease(hifont);

  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, prv->TexX, prv->TexY, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bits);
  // no mipmaps are generated, so the min filter must not ask for any
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
  prv->Tex = tex;

  delete[] bits;
}

sFont2D::sFont2D(const sChar *name, sInt size, sInt flags, sInt width)
{
  prv = 0;
  Init(name, size, flags, width);
}

sFont2D::~sFont2D()
{
  Exit();
}

void sFont2D::Exit()
{
  if (prv)
  {
    if (prv->Tex) { GLuint t = prv->Tex; glDeleteTextures(1, &t); prv->Tex = 0; }
    if (prv->Font) { CFRelease(prv->Font); prv->Font = 0; }
    delete prv;
    prv = 0;
  }
}

void sFont2D::Init(const sChar *name, sInt size, sInt flags, sInt width)
{
  Exit();
  prv = new sFont2DPrivate;
  sClear(*prv);

  char buf[256];
  sLinuxFromWide(buf, (name && name[0]) ? name : L"Helvetica", sizeof(buf));
  CFStringRef fontName = CFStringCreateWithCString(NULL, buf, kCFStringEncodingUTF8);
  CGFloat fontSize = (size > 0) ? (CGFloat)size : 12.0f;
  prv->Font = CTFontCreateWithName(fontName, fontSize, NULL);
  CFRelease(fontName);

  if (!prv->Font)
    prv->Font = CTFontCreateWithName(CFSTR("Helvetica"), fontSize, NULL);

  if (flags & (sF2C_BOLD | sF2C_ITALICS))
  {
    CTFontSymbolicTraits traits = 0;
    if (flags & sF2C_BOLD) traits |= kCTFontTraitBold;
    if (flags & sF2C_ITALICS) traits |= kCTFontTraitItalic;
    CTFontRef styled = CTFontCreateCopyWithSymbolicTraits(prv->Font, fontSize, NULL, traits, traits);
    if (styled) { CFRelease(prv->Font); prv->Font = styled; }
  }

  CGFloat ascent = CTFontGetAscent(prv->Font);
  CGFloat descent = CTFontGetDescent(prv->Font);
  CGFloat leading = CTFontGetLeading(prv->Font);
  CGFloat capHeight = CTFontGetCapHeight(prv->Font);

  prv->Baseline = (sInt)ceil(ascent);
  prv->Height = (sInt)ceil(ascent + descent + leading);
  prv->CharHeight = (sInt)ceil(capHeight);
  prv->TextColor = 4;                       // sGC_TEXT, defined a layer up in gui/
  prv->BackColor = 1;                       // sGC_BACK
  prv->Flags = flags;

  sMacBuildFontAtlas(prv);
}

sInt sFont2D::GetHeight()
{
  return prv ? prv->Height : 12;
}

sInt sFont2D::GetBaseline()
{
  return prv ? prv->Baseline : 10;
}

sInt sFont2D::GetCharHeight()
{
  return prv ? prv->CharHeight : 10;
}

static sInt sMacAdvance(sFont2DPrivate *prv, sChar c)
{
  if (!prv) return 8;
  if (c == '\t') return prv->Advance[' '] * 4;
  if ((sInt)c < sMacFontGlyphs) return prv->Advance[c];
  return prv->Advance['?'];      // outside the atlas: use a stand-in of equal width
}

sInt sFont2D::GetWidth(const sChar *text, sInt len)
{
  if (!text || !prv) return 0;
  if (len < 0) len = sGetStringLen(text);

  sInt w = 0;
  for (sInt i = 0; i < len; i++)
    w += sMacAdvance(prv, text[i]);
  return w;
}

sInt sFont2D::GetAdvance(const sChar *text, sInt len)
{
  return GetWidth(text, len);
}

sInt sFont2D::GetCharCountFromWidth(sInt width, const sChar *text, sInt len)
{
  if (!text || !prv) return 0;
  if (len < 0) len = sGetStringLen(text);

  sInt w = 0;
  for (sInt i = 0; i < len; i++)
  {
    w += sMacAdvance(prv, text[i]);
    if (w > width) return i;
  }
  return len;
}

sFont2D::sLetterDimensions sFont2D::sGetLetterDimensions(const sChar letter)
{
  sLetterDimensions dim;
  sClear(dim);
  dim.Advance = sMacAdvance(prv, letter);
  dim.Cell = dim.Advance;
  dim.Height = GetHeight();
  dim.OriginY = GetBaseline();
  return dim;
}

sBool sFont2D::LetterExists(sChar letter)
{
  return sTRUE;
}

void sFont2D::AddResource(const sChar *filename)
{
}

void sFont2D::SetColor(sInt text, sInt back)
{
  if (prv)
  {
    prv->TextColor = text;
    prv->BackColor = back;
  }
}

// Draw one run of text with the baseline at y, returning the end pen position.
static sInt sMacDrawText(sFont2DPrivate *prv, sInt x, sInt y, const sChar *text, sInt len, sInt colid)
{
  if (!prv || !prv->Tex || len <= 0) return x;

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, prv->Tex);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  SetGLColor(colid);

  const sF32 du = 1.0f / 16.0f;
  const sF32 dv = 1.0f / 16.0f;
  const sF32 ex = 1.0f / prv->TexX;   // half-texel-ish inset, keeps cells apart
  const sF32 ey = 1.0f / prv->TexY;

  glBegin(GL_QUADS);
  for (sInt i = 0; i < len; i++)
  {
    sChar c = text[i];
    sInt adv = sMacAdvance(prv, c);
    sInt g = ((sInt)c < sMacFontGlyphs) ? (sInt)c : '?';
    if (c == '\t') { x += adv; continue; }
    if (g > 32)
    {
      sF32 u0 = (g & 15) * du, v0 = (g >> 4) * dv;
      sInt qx = x - prv->PadX;
      sInt qy = y - prv->PadY;
      glTexCoord2f(u0 + ex, v0 + ey);           glVertex2i(qx, qy);
      glTexCoord2f(u0 + du - ex, v0 + ey);      glVertex2i(qx + prv->CellX, qy);
      glTexCoord2f(u0 + du - ex, v0 + dv - ey); glVertex2i(qx + prv->CellX, qy + prv->CellY);
      glTexCoord2f(u0 + ex, v0 + dv - ey);      glVertex2i(qx, qy + prv->CellY);
    }
    x += adv;
  }
  glEnd();

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
  return x;
}

void sFont2D::PrintBasic(sInt flags, const sRect *r, sInt x, sInt y, const sChar *text, sInt len)
{
  if (!prv) return;
  if (len < 0) len = sGetStringLen(text);

  if ((flags & sF2P_OPAQUE) && r)
    sRect2D(*r, prv->BackColor);

  sMacDrawText(prv, x, y + prv->Baseline, text, len, prv->TextColor);
}

void sFont2D::PrintMarked(sInt flags, const sRect *r, sInt x, sInt y, const sChar *text, sInt len, sPrintInfo *pi)
{
  if (!prv) return;
  if (len < 0) len = sGetStringLen(text);

  if ((flags & sF2P_OPAQUE) && r)
    sRect2D(*r, prv->BackColor);

  sInt baseline = y + prv->Baseline;

  if (pi && pi->HasSelection())
  {
    // paint the selected run on its own background, then the two plain runs
    sInt s0 = sClamp(pi->SelectStart, 0, len);
    sInt s1 = sClamp(pi->SelectEnd, 0, len);
    sInt xa = x + GetWidth(text, s0);
    sInt xb = xa + GetWidth(text + s0, s1 - s0);
    sInt back = (pi->SelectBackColor == (sU32)-1) ? prv->TextColor : (sInt)pi->SelectBackColor;
    sInt front = (pi->SelectBackColor == (sU32)-1) ? prv->BackColor : prv->TextColor;

    sRect2D(xa, y, xb, y + prv->Height, back);
    sMacDrawText(prv, x, baseline, text, s0, prv->TextColor);
    sMacDrawText(prv, xa, baseline, text + s0, s1 - s0, front);
    sMacDrawText(prv, xb, baseline, text + s1, len - s1, prv->TextColor);
  }
  else
  {
    sMacDrawText(prv, x, baseline, text, len, prv->TextColor);
  }

  if (pi && pi->CursorPos >= 0 && pi->CursorPos <= len)
  {
    sInt cx = x + GetWidth(text, pi->CursorPos);
    sInt cw = pi->Overwrite ? sMacAdvance(prv, pi->CursorPos < len ? text[pi->CursorPos] : ' ') : 2;
    sRectInvert2D(cx, y, cx + cw, y + prv->Height);
  }
}

void sFont2D::Print(sInt flags, sInt x, sInt y, const sChar *text, sInt len)
{
  if (!prv) return;
  if (len < 0) len = sGetStringLen(text);
  sRect r(x, y, x + GetWidth(text, len), y + prv->Height);
  PrintBasic(flags, &r, x, y, text, len);
}

sInt sFont2D::Print(sInt flags, const sRect &r, const sChar *text, sInt len, sInt margin, sInt xo, sInt yo, sPrintInfo *pi)
{
  if (!prv) return 12;
  if (len < 0) len = sGetStringLen(text);

  if (flags & sF2P_SPACE)
    margin += prv->Advance[' '];

  if ((flags & sF2P_OPAQUE) && (!pi || pi->Mode == sPIM_PRINT))
    sRect2D(r, prv->BackColor);

  sRect inner = r;
  inner.Extend(-margin);
  sInt boxw = sMax(0, inner.SizeX());

  // split into lines: explicit newlines always break, word wrap only when asked
  struct Line { sInt Start, Count, Width; };
  sStaticArray<Line> lines;
  lines.HintSize(64);

  sInt pos = 0;
  while (pos <= len)
  {
    sInt start = pos;
    sInt end = pos;
    while (end < len && text[end] != '\n')
      end++;

    if (flags & sF2P_MULTILINE)
    {
      // greedy word wrap inside [start,end)
      while (start < end)
      {
        sInt w = 0, last = start, i = start;
        while (i < end)
        {
          w += sMacAdvance(prv, text[i]);
          if (w > boxw && last > start) break;
          i++;
          if (i < end && text[i] == ' ') last = i;
        }
        sInt cut = (i >= end) ? end : ((last > start) ? last : i);
        Line l; l.Start = start; l.Count = cut - start; l.Width = GetWidth(text + start, l.Count);
        lines.AddTail(l);
        start = cut;
        while (start < end && text[start] == ' ') start++;
      }
      if (start == end && (end == pos || lines.GetCount() == 0))
      {
        Line l; l.Start = start; l.Count = 0; l.Width = 0;
        lines.AddTail(l);
      }
    }
    else
    {
      Line l; l.Start = start; l.Count = end - start; l.Width = GetWidth(text + start, l.Count);
      lines.AddTail(l);
    }

    if (end >= len) break;
    pos = end + 1;
  }

  sInt lh = prv->Height;
  sInt total = lines.GetCount() * lh;

  if (pi && pi->Mode == sPIM_GETHEIGHT)
    return total;

  sInt y;
  if (flags & sF2P_TOP)         y = inner.y0;
  else if (flags & sF2P_BOTTOM) y = inner.y1 - total;
  else                          y = inner.y0 + (inner.SizeY() - total) / 2;
  y += yo;

  Line *l;
  sFORALL(lines, l)
  {
    sInt x;
    if (flags & sF2P_LEFT)       x = inner.x0;
    else if (flags & sF2P_RIGHT) x = inner.x1 - l->Width;
    else                         x = inner.x0 + (boxw - l->Width) / 2;
    x += xo;

    if (pi && pi->Mode == sPIM_POINT2POS)
    {
      if (pi->QueryY >= y && pi->QueryY < y + lh)
      {
        sInt n = GetCharCountFromWidth(pi->QueryX - x + prv->Advance[' '] / 2, text + l->Start, l->Count);
        pi->QueryPos = text + l->Start + n;
        pi->Mode = sPIM_QUERYDONE;
      }
    }
    else if (pi && pi->Mode == sPIM_POS2POINT)
    {
      const sChar *q = pi->QueryPos;
      if (q >= text + l->Start && q <= text + l->Start + l->Count)
      {
        pi->QueryX = x + GetWidth(text + l->Start, (sInt)(q - (text + l->Start)));
        pi->QueryY = y;
      }
    }
    else
    {
      if (y + lh > r.y0 && y < r.y1)
      {
        if (pi)
        {
          sPrintInfo lpi = *pi;
          lpi.CursorPos = (pi->CursorPos >= l->Start && pi->CursorPos <= l->Start + l->Count) ? pi->CursorPos - l->Start : -1;
          lpi.SelectStart = pi->SelectStart - l->Start;
          lpi.SelectEnd = pi->SelectEnd - l->Start;
          PrintMarked(flags & ~sF2P_OPAQUE, 0, x, y, text + l->Start, l->Count, &lpi);
        }
        else
        {
          sMacDrawText(prv, x, y + prv->Baseline, text + l->Start, l->Count, prv->TextColor);
        }
      }
    }
    y += lh;
  }

  if (pi && pi->HintLine)
  {
    sInt hx = inner.x0 + xo + prv->Advance[' '] * pi->HintLine;
    if (hx > r.x0 && hx < r.x1)
      sLine2D(hx, r.y0, hx, r.y1, pi->HintLineColor);
  }

  return total;
}

void sPrintInfo::Init()
{
  Mode = 0;
  CursorPos = -1;
  Overwrite = 0;
  SelectStart = -1;
  SelectEnd = -1;
  SelectBackColor = 0;
  HintLine = 0;
  HintLineColor = 0;
  CullRect.Init(0,0,0,0);
  QueryX = 0;
  QueryY = 0;
  QueryPos = 0;
}

void sSetMouse(sInt x, sInt y)
{
}

void sSetMouseCenter()
{
}

void sPreventPaint()
{
}

void sTriggerEvent(sInt event)
{
}

sBool sCheckBreakKey()
{
  return sFALSE;
}

void sSetTimerEvent(sInt rate, sInt id)
{
}

sBool sGetUserName(const sStringDesc &dest, sInt joypadId)
{
  sCopyString(dest, L"wangxin");
  return sTRUE;
}

sBool sSetFileTime(const sChar *name, sU64 time)
{
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Audio Subsystem                                                    ***/
/***                                                                      ***/
/****************************************************************************/

// wz4 does not just play music through this -- it clocks its timeline off the
// audio callback (MainWindow::SoundHandler advances TimelineWin->Time). With
// the handler never called, time stays at zero forever and a demo renders its
// first frame and nothing else. So this is what makes the timeline run.

static AudioUnit sMacAudioUnit = 0;
static sSoundHandler sMacSoundHandler = 0;
static sInt sMacSoundFreq = 44100;
static volatile sInt sMacSamplePos = 0;

static OSStatus sMacAudioRender(void *ref,AudioUnitRenderActionFlags *flags,
                                const AudioTimeStamp *ts,UInt32 bus,UInt32 frames,
                                AudioBufferList *data)
{
  sSoundHandler h = sMacSoundHandler;
  for(UInt32 b=0;b<data->mNumberBuffers;b++)
  {
    sS16 *out = (sS16 *)data->mBuffers[b].mData;
    if(!out) continue;
    if(h)
      h(out,(sInt)frames);                  // 1 count = one stereo frame
    else
      sSetMem(out,0,frames*4);
  }
  sMacSamplePos += (sInt)frames;
  (void)ref; (void)flags; (void)ts; (void)bus;
  return noErr;
}

sBool sSetSoundHandler(sInt freq, sSoundHandler handler, sInt latency, sInt flags)
{
  sClearSoundHandler();
  if(!handler) return sTRUE;

  AudioComponentDescription desc;
  sSetMem(&desc,0,sizeof(desc));
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;

  AudioComponent comp = AudioComponentFindNext(0,&desc);
  if(!comp) { sLog(L"sys",L"no default audio output\n"); return sFALSE; }

  AudioUnit unit = 0;
  if(AudioComponentInstanceNew(comp,&unit)!=noErr || !unit)
  { sLog(L"sys",L"could not open audio output\n"); return sFALSE; }

  AudioStreamBasicDescription fmt;
  sSetMem(&fmt,0,sizeof(fmt));
  fmt.mSampleRate = freq;
  fmt.mFormatID = kAudioFormatLinearPCM;
  fmt.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
  fmt.mChannelsPerFrame = 2;
  fmt.mBitsPerChannel = 16;
  fmt.mFramesPerPacket = 1;
  fmt.mBytesPerFrame = 4;                   // interleaved stereo, 16 bit
  fmt.mBytesPerPacket = 4;

  if(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,
       kAudioUnitScope_Input,0,&fmt,sizeof(fmt))!=noErr)
  {
    sLog(L"sys",L"audio output rejected 16 bit stereo\n");
    AudioComponentInstanceDispose(unit);
    return sFALSE;
  }

  AURenderCallbackStruct cb;
  cb.inputProc = sMacAudioRender;
  cb.inputProcRefCon = 0;
  AudioUnitSetProperty(unit,kAudioUnitProperty_SetRenderCallback,
    kAudioUnitScope_Input,0,&cb,sizeof(cb));

  sMacSoundHandler = handler;
  sMacSoundFreq = freq;
  sMacSamplePos = 0;

  if(AudioUnitInitialize(unit)!=noErr || AudioOutputUnitStart(unit)!=noErr)
  {
    sLog(L"sys",L"could not start audio output\n");
    sMacSoundHandler = 0;
    AudioComponentInstanceDispose(unit);
    return sFALSE;
  }

  sMacAudioUnit = unit;
  sLogF(L"sys",L"audio output running at %d Hz\n",freq);
  (void)latency; (void)flags;
  return sTRUE;
}

void sClearSoundHandler()
{
  if(sMacAudioUnit)
  {
    AudioOutputUnitStop(sMacAudioUnit);
    AudioUnitUninitialize(sMacAudioUnit);
    AudioComponentInstanceDispose(sMacAudioUnit);
    sMacAudioUnit = 0;
  }
  sMacSoundHandler = 0;
}

sInt sGetCurrentSample()
{
  return sMacSamplePos;
}

/****************************************************************************/
/***                                                                      ***/
/***   sImage2D Implementation                                            ***/
/***                                                                      ***/
/****************************************************************************/

struct sImage2DPrivate
{
  sInt SizeX;
  sInt SizeY;
  sU32 *Data;
};

sImage2D::sImage2D(sInt xs, sInt ys, sU32 *data)
{
  prv = new sImage2DPrivate;
  prv->SizeX = xs;
  prv->SizeY = ys;
  prv->Data = new sU32[xs * ys];
  if (data)
    sCopyMem(prv->Data, data, xs * ys * 4);
  else
    sSetMem(prv->Data, 0, xs * ys * 4);
}

sImage2D::~sImage2D()
{
  if (prv)
  {
    delete[] prv->Data;
    delete prv;
  }
}

void sImage2D::Update(sU32 *data)
{
  if (prv && data)
    sCopyMem(prv->Data, data, prv->SizeX * prv->SizeY * 4);
}

void sImage2D::Paint(const sRect &source, sInt x, sInt y)
{
}

void sImage2D::Paint(sInt x, sInt y)
{
}

void sImage2D::Stretch(const sRect &source, const sRect &dest)
{
}

sInt sImage2D::GetSizeX()
{
  return prv ? prv->SizeX : 0;
}

sInt sImage2D::GetSizeY()
{
  return prv ? prv->SizeY : 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   System Dialogs & Threads                                           ***/
/***                                                                      ***/
/****************************************************************************/

// Cocoa speaks utf-8; altona's sChar is a 16 bit wide character.
static NSString *sMacNSStringFromWide(const sChar *str)
{
  if (!str || !*str)
    return @"";
  char buf[sMAXPATH * 3 + 4];                  // utf-8 is up to 3 bytes per sChar
  sLinuxFromWide(buf, str, sizeof(buf));
  NSString *s = [NSString stringWithUTF8String:buf];
  return s ? s : @"";
}

// Altona describes the files it wants with a '|' separated list of bare
// extensions -- "wz4", "xsi|wz4mesh|lwo|obj" -- and with "", "*" or "*.*"
// when it wants anything at all. Cocoa wants an array of extensions, and no
// array when nothing should be filtered out.
static NSArray *sMacFileTypes(const sChar *extensions)
{
  NSMutableArray *types = [NSMutableArray array];

  for (NSString *part in [sMacNSStringFromWide(extensions) componentsSeparatedByString:@"|"])
  {
    NSString *ext = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    if ([ext hasPrefix:@"*."])
      ext = [ext substringFromIndex:2];
    if ([ext length] == 0 || [ext isEqualToString:@"*"])
      continue;
    [types addObject:ext];
  }

  return [types count] ? types : nil;
}

// The buffer the caller hands in is where the dialog should start: a file, a
// directory, or nothing. Werkkzeug keeps most of its paths relative to the
// working directory, and the ones it read out of a project file may still
// carry windows separators.
static void sMacSplitDialogPath(const sChar *path, NSString **dir, NSString **name)
{
  *dir = nil;
  *name = nil;

  NSString *p = [sMacNSStringFromWide(path) stringByReplacingOccurrencesOfString:@"\\" withString:@"/"];
  if ([p length] == 0)
    return;

  if (![p isAbsolutePath])
    p = [[[NSFileManager defaultManager] currentDirectoryPath] stringByAppendingPathComponent:p];
  p = [p stringByStandardizingPath];

  BOOL isdir = NO;
  if ([[NSFileManager defaultManager] fileExistsAtPath:p isDirectory:&isdir] && isdir)
  {
    *dir = p;
  }
  else
  {
    *dir = [p stringByDeletingLastPathComponent];
    *name = [p lastPathComponent];
  }
}

sBool sSystemOpenFileDialog(const sChar *label, const sChar *extensions, sInt flags, const sStringDesc &buffer)
{
  sBool result = sFALSE;

  @autoreleasepool {
    NSArray *types = sMacFileTypes(extensions);
    NSString *dir = nil;
    NSString *name = nil;
    sMacSplitDialogPath(buffer.Buffer, &dir, &name);

    NSSavePanel *panel = nil;
    if ((flags & 3) == sSOF_SAVE)
    {
      NSSavePanel *save = [NSSavePanel savePanel];
      if (types)
        [save setAllowedFileTypes:types];        // also supplies the default extension
      [save setAllowsOtherFileTypes:YES];
      [save setCanCreateDirectories:YES];
      panel = save;
    }
    else
    {
      sBool dironly = ((flags & 3) == sSOF_DIR);
      NSOpenPanel *open = [NSOpenPanel openPanel];
      [open setCanChooseFiles:dironly ? NO : YES];
      [open setCanChooseDirectories:dironly ? YES : NO];
      [open setCanCreateDirectories:dironly ? YES : NO];
      // one path in, one path out: sSOF_MULTISELECT has no caller here, and
      // the buffer has nowhere to put a second name.
      [open setAllowsMultipleSelection:NO];
      if (types && !dironly)
        [open setAllowedFileTypes:types];
      panel = open;
    }

    if (label && *label)
    {
      NSString *title = sMacNSStringFromWide(label);
      [panel setTitle:title];
      [panel setMessage:title];                  // modern panels do not show the title
    }
    if (dir)
      [panel setDirectoryURL:[NSURL fileURLWithPath:dir]];
    if (name)
      [panel setNameFieldStringValue:name];

    // The demo window is the only window this app has, and it never yields.
    // Without this the panel can come up behind it.
    [NSApp activateIgnoringOtherApps:YES];

    if ([panel runModal] == NSModalResponseOK)
    {
      NSURL *url = [panel isKindOfClass:[NSOpenPanel class]]
        ? [[(NSOpenPanel *)panel URLs] firstObject]
        : [panel URL];
      const char *path = url ? [url fileSystemRepresentation] : 0;
      if (path && *path)
      {
        sLinuxToWide(buffer.Buffer, path, buffer.Size);
        result = sTRUE;
      }
    }

    // hand the keyboard and the gl context back to the demo window
    [sMacWindow makeKeyAndOrderFront:nil];
    if (sMacGLView && sMacGLView->GLContext)
      [sMacGLView->GLContext makeCurrentContext];
  }

  return result;
}

sBool sSystemMessageDialog(const sChar *label, sInt flags)
{
  return sTRUE;
}

sBool sSystemMessageDialog(const sChar *label, sInt flags, const sChar *title)
{
  return sTRUE;
}

void sThread::SetHomeCore(sInt core)
{
}

sVideoEncoder *sCreateVideoEncoder(const sChar *filename, sF32 fps, sVideoEncoder::CodecInfo *info)
{
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   sInit & Main Loop Implementation                                   ***/
/***                                                                      ***/
/****************************************************************************/

void sInit(sInt flags, sInt xs, sInt ys)
{
  extern sInt sSystemFlags;
  sSystemFlags = flags;

  sMacWindowWidth = (xs > 0) ? xs : 1280;
  sMacWindowHeight = (ys > 0) ? ys : 720;

  @autoreleasepool {
    sMacApp = [NSApplication sharedApplication];
    [sMacApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Setup Main Menu
    NSMenu *mainMenu = [[NSMenu alloc] init];
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:appMenuItem];
    [sMacApp setMainMenu:mainMenu];

    NSMenu *appMenu = [[NSMenu alloc] init];
    NSString *appName = [[NSProcessInfo processInfo] processName];
    NSString *quitTitle = [NSString stringWithFormat:@"Quit %@", appName];
    NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];

    // Create Window
    NSRect contentRect = NSMakeRect(100, 100, sMacWindowWidth, sMacWindowHeight);
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    sMacWindow = [[NSWindow alloc] initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
    [sMacWindow setTitle:@"Werkkzeug 4 (macOS Native)"];
    [sMacWindow center];

    sMacWinDelegate = [[Altona1WindowDelegate alloc] init];
    [sMacWindow setDelegate:sMacWinDelegate];
    // the gui tracks hover and drives menus off plain mouse movement, which
    // cocoa only delivers when the window asks for it
    [sMacWindow setAcceptsMouseMovedEvents:YES];

    // Pixel format for OpenGL 2.1 Metal context
    NSOpenGLPixelFormatAttribute attrs[] = {
      NSOpenGLPFADoubleBuffer,
      NSOpenGLPFAColorSize, 24,
      NSOpenGLPFAAlphaSize, 8,
      NSOpenGLPFADepthSize, 24,
      NSOpenGLPFAStencilSize, 8,
      NSOpenGLPFAAccelerated,
      0
    };
    NSOpenGLPixelFormat *pixFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];

    sMacGLView = [[Altona1GLView alloc] initWithFrame:contentRect pixelFormat:pixFormat];
    sMacGLView->GLContext = [sMacGLView openGLContext];
    [sMacWindow setContentView:sMacGLView];
    [sMacWindow makeKeyAndOrderFront:nil];
    [sMacApp activateIgnoringOtherApps:YES];
    [sMacApp finishLaunching];

    InitCursors();

    if (flags & sISF_3D)
    {
      PreInitGFX(flags, sMacWindowWidth, sMacWindowHeight);
      InitGFX(flags, sMacWindowWidth, sMacWindowHeight);
    }
  }
}

/****************************************************************************/

// Dump the finished frame to a file when asked. Screenshotting the window from
// outside depends on which app happens to be in front and which desktop the
// window opened on, so it is no use for checking what this port actually
// draws; reading the framebuffer back does not care about either.
//
//   WZ4_GRABDIR=<dir>   write <dir>/frame.bmp, overwritten as the app runs
//   WZ4_GRABEVERY=<n>   how many frames apart, 60 by default
//
// Off unless WZ4_GRABDIR is set.

static sString<sMAXPATH> sMacGrabDir;
static sInt sMacGrabEvery = 0;                  // 0 = not looked up yet, -1 = off
static sInt sMacGrabCount = 0;
static sTexture2D *sMacGrabTex = 0;

static void sMacGrabFrame()
{
  if(sMacGrabEvery==0)
  {
    const char *dir = getenv("WZ4_GRABDIR");
    if(!dir || !*dir)
    {
      sMacGrabEvery = -1;
      return;
    }
    sInt i=0;
    while(dir[i] && i<sMAXPATH-1) { sMacGrabDir[i] = (sChar)dir[i]; i++; }
    sMacGrabDir[i] = 0;

    const char *every = getenv("WZ4_GRABEVERY");
    sMacGrabEvery = every ? sMax(1,(sInt)atoi(every)) : 60;
  }
  if(sMacGrabEvery<0) return;

  if(++sMacGrabCount < sMacGrabEvery) return;
  sMacGrabCount = 0;

  // sGrabScreen copies out of whatever is bound for reading, which here is the
  // back buffer the frame was just drawn into. The destination has to be a
  // rendertarget: a plain texture gets its storage from BeginLoad/EndLoad and
  // has none yet at this point.
  sInt xs,ys,zs;
  if(sMacGrabTex)
  {
    sMacGrabTex->GetSize(xs,ys,zs);
    if(xs!=sMacWindowWidth || ys!=sMacWindowHeight)
      sDelete(sMacGrabTex);
  }
  if(!sMacGrabTex)
    sMacGrabTex = new sTexture2D(sMacWindowWidth,sMacWindowHeight,
      sTEX_2D|sTEX_ARGB8888|sTEX_NOMIPMAPS|sTEX_RENDERTARGET,1);

  sGrabScreen(sMacGrabTex,sGFF_NONE,0,0);

  // WZ4_GRABSEQ numbers the files instead of overwriting one, which is what
  // turns a WZ4_TIMESTEP sweep into a strip of frames to pick from.
  sString<sMAXPATH> name(sMacGrabDir);
  if(getenv("WZ4_GRABSEQ"))
  {
    static sInt seq = 0;
    sString<64> leaf;
    leaf.PrintF(L"frame_%04d.bmp",seq++);
    sAppendPath(name,leaf);
  }
  else
  {
    sAppendPath(name,L"frame.bmp");
  }
  sSaveRT(name,sMacGrabTex);
}

/****************************************************************************/

// Master execution loop for GUI applications
void sRunMacApp()
{
  while (!sExitFlag)
  {
    @autoreleasepool {
      ProcessCocoaEvents();

      if (sFrameHook)
        sFrameHook->Call();

      sBool app_fullpaint = sFALSE;
      if (sGetApp())
      {
        app_fullpaint = sGetApp()->OnPaint();
        if (!app_fullpaint)
          sGetApp()->OnPrepareFrame();
      }

      // Unlike X11, where the 2D gui goes to an Xlib drawable and the 3D scene
      // to a separate GLX surface, both share one GL framebuffer here. The 3D
      // pass clears whatever it renders over, so it has to run *before* the
      // gui, which then composites on top. sRender3DEnd must not flip here:
      // the frame is only complete after the 2D pass.
      if (!app_fullpaint && (sGetSystemFlags() & sISF_3D) && !sExitFlag)
      {
        if (sRender3DBegin())
        {
          if (sGetApp())
            sGetApp()->OnPaint3D();
          sRender3DEnd(0);
        }
      }

      if ((sGetSystemFlags() & sISF_2D) && !sExitFlag)
      {
        sRender2DBegin();
        sRect client(0, 0, sMacWindowWidth, sMacWindowHeight);
        sRect update = client;
        if (sAppPtr)
          sAppPtr->OnPaint2D(client, update);
        sRender2DEnd();
      }

      sMacGrabFrame();
      sMacSwapBuffers();
      // WZ4_FRAMESLEEP overrides the pacing, in microseconds. Effects that
      // accumulate from frame to frame come out brighter the more frames they
      // get, so being able to slow the loop down is how that gets measured.
      {
        static sInt sleepus = -1;
        if(sleepus<0)
        {
          const char *s = getenv("WZ4_FRAMESLEEP");
          sleepus = (s && *s) ? atoi(s) : 8000;
        }
        usleep(sleepus); // 8000 = ~120 fps max cadence
      }
    }
  }

  ExitGFX();
}
