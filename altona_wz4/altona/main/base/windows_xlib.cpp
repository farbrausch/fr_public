/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "base/system.hpp"
#include "base/windows.hpp"

#if !sCOMMANDLINE

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>
#include <string.h>

extern Colormap sXColMap;
extern Window sXWnd;
extern Visual *sXVisual;
extern GC sXGC;
extern sInt sXScreen;

/****************************************************************************/

extern void sLinuxFromWide(char *dest,const sChar *src,int size);
extern char *sLinuxFromWide(const sChar *str);
extern void sLinuxToWide(sChar *dest,const char *src,int size);
extern Display *sXDisplay();

#define MAX_COLORS  32
#define MAX_CLIPS   64

static Cursor Cursors[sMP_MAX];
static sU32 Color[MAX_COLORS][2]; // [0]=X color index, [1]=RGB color
static XftColor ColorXFT[MAX_COLORS];
static Region EmptyRegion,UpdateRegion;
static sBool UpdateRegionGC,UpdateRegionXft,UpdateRegionXR;
static Region ClipStack[MAX_CLIPS];
static sInt ClipIndex;
static sRect ClipBounds;
static sString<256> WindowName;

static sInt ForegroundCol;
static XRenderPictFormat *XFmtARGB;
static Picture XPict;
static XftDraw *XDraw;

#if sCONFIG_LE
static const FcEndian Endianess = FcEndianLittle;
#elif sCONFIG_BE
static const FcEndian Endianess = FcEndianBig;
#endif

static void InitXLib()
{
  for(sInt i=0;i<MAX_CLIPS;i++)
    ClipStack[i] = XCreateRegion();
  
  EmptyRegion = XCreateRegion();
  UpdateRegion = XCreateRegion();
  
  static const int shapeInd[] =
  {
    0,
    XC_left_ptr,
    XC_watch,
    XC_crosshair,
    XC_hand2,
    XC_xterm,
    XC_X_cursor,
    XC_fleur,
    XC_sb_v_double_arrow,
    XC_sb_h_double_arrow,
    XC_bottom_right_corner, // poor match!
    XC_bottom_left_corner, // poor match!
  };
  Cursors[0] = 0;
  for(sInt i=1;i<sMP_MAX;i++)
    Cursors[i] = XCreateFontCursor(sXDisplay(),shapeInd[i]);
  
  if(sGetSystemFlags() & sISF_2D)
  {
    XRenderColor rc;
    XftColor blackColor;
    sClear(rc);
    rc.alpha = 0xffff;
    XftColorAllocValue(sXDisplay(),sXVisual,sXColMap,&rc,&blackColor);
    
    for(sInt i=0;i<MAX_COLORS;i++)
    {
      Color[i][0] = Color[i][1] = 0;
      ColorXFT[i] = blackColor;
    }

    XDraw = XftDrawCreate(sXDisplay(),sXWnd,DefaultVisual(sXDisplay(),sXScreen),sXColMap);
    if(!XDraw)
      sFatal(L"XftDrawCreate failed!");
    
    // initialize x render extension
    int major,minor;
    XRenderQueryVersion(sXDisplay(),&major,&minor);
    sLogF(L"xlib",L"XRender %d.%d found\n",major,minor);
    
    XRenderPictFormat *fmt = XRenderFindVisualFormat(sXDisplay(),sXVisual);
    if(!fmt)
      sFatal(L"XRenderFindVisualFormat failed on default visual!");
    
    XRenderPictureAttributes attr;
    attr.graphics_exposures = False;
    XPict = XRenderCreatePicture(sXDisplay(),sXWnd,fmt,CPGraphicsExposure,&attr);
    if(!XPict)
      sFatal(L"XRenderCreatePicture for main picture failed!");
    
    XFmtARGB = XRenderFindStandardFormat(sXDisplay(),PictStandardARGB32);
    if(!XFmtARGB)
      sFatal(L"XRenderFindStandardFormat failed on display!");
  }
  
  ForegroundCol = -1;
}

static void ExitXLib()
{
  if(sGetSystemFlags() & sISF_2D)
  {
    XRenderFreePicture(sXDisplay(),XPict);
    XftDrawDestroy(XDraw);
    
    for(sInt i=0;i<MAX_COLORS;i++)
      XftColorFree(sXDisplay(),sXVisual,sXColMap,&ColorXFT[i]);
    
    for(sInt i=0;i<MAX_CLIPS;i++)
      XDestroyRegion(ClipStack[i]);
    
    XDestroyRegion(EmptyRegion);
    
    for(sInt i=0;i<sMP_MAX;i++)
      XFreeCursor(sXDisplay(),Cursors[i]);
  }
}

sADDSUBSYSTEM(XLib,0x88,InitXLib,ExitXLib);

/****************************************************************************/

static void sGetClientArea(sRect &area)
{
  Window root;
  int xp,yp;
  unsigned int width,height,border,depth;
  
  XGetGeometry(sXDisplay(),sXWnd,&root,&xp,&yp,&width,&height,&border,&depth);
  area.Init(0,0,width,height);
}

static Region sRectToRegion(const sRect &r)
{
  Region newRegion = XCreateRegion();
  
  XRectangle rect;
  rect.x = r.x0;
  rect.y = r.y0;
  rect.width = r.x1-r.x0;
  rect.height = r.y1-r.y0;
  XUnionRectWithRegion(&rect,EmptyRegion,newRegion);
  
  return newRegion;
}

static void sRegionBoundingRect(Region region,sRect &r)
{
  XRectangle rect;
  XClipBox(region,&rect);
  r.Init(rect.x,rect.y,rect.x+rect.width,rect.y+rect.height);
}

static sBool sRectsIntersect(const sRect &a,const sRect &b)
{
  return sMax(a.x0,b.x0) < sMin(a.x1,b.x1) && sMax(a.y0,b.y0) < sMin(a.y1,b.y1);
}

/****************************************************************************/

void sXClearUpdate()
{
  XUnionRegion(EmptyRegion,EmptyRegion,UpdateRegion);
}

void sXGetUpdateBoundingRect(sRect &r)
{
  sRegionBoundingRect(UpdateRegion,r);
}

void sUpdateWindow()
{
  sRect client;
  sGetClientArea(client);
  sUpdateWindow(client);
}

void sUpdateWindow(const sRect &r)
{
  if(r.x1 > r.x0 && r.y1 > r.y0)
  {
    XRectangle rect;
    rect.x = r.x0;
    rect.y = r.y0;
    rect.width = r.x1-r.x0;
    rect.height = r.y1-r.y0;
    XUnionRectWithRegion(&rect,UpdateRegion,UpdateRegion);
  }
}

void sSetMousePointer(sInt code)
{
  static sInt oldcursor = -1;
  sVERIFY(code>=0 && code<sMP_MAX);
  if(code != oldcursor)
  {
    XDefineCursor(sXDisplay(),sXWnd,Cursors[code]);
    oldcursor = code;
  }
}

const sChar *sGetWindowName()
{
  if(WindowName.IsEmpty())
    return L"altona application";
  else
    return WindowName;
}

void sSetWindowName(const sChar *name)
{
  WindowName = name;
  if(sXWnd) XStoreName(sXDisplay(),sXWnd,sLinuxFromWide(name));
}

void sSetWindowMode(sInt mode)
{
  sLogF(L"xlib",L"sSetWindowMode\n");
}

void sSetWindowSize(sInt xs,sInt ys)
{
  sLogF(L"xlib",L"sSetWindowSize\n");
}

void sSetWindowPos(sInt x,sInt y)
{
  sLogF(L"xlib",L"sSetWindowPos\n");
}

void sGetWindowPos(sInt &x,sInt &y)
{
  sLogF(L"xlib",L"sGetWindowPos\n");
  x = 0;
  y = 0;
}

void sGetWindowSize(sInt &sx,sInt &sy)
{
  sLogF(L"xlib",L"sGetWindowSize\n");
  sx = 0;
  sy = 0;
}

sInt sGetWindowMode()
{
  sLogF(L"xlib",L"sGetWindowMode\n");
  return sWM_NORMAL;
}

sBool sHasWindowFocus()
{
  Window focusWin;
  int focusState;
  XGetInputFocus(sXDisplay(),&focusWin,&focusState);

  return focusWin == sXWnd;
}

void sSetClipboard(const sChar *text,sInt len)
{
  if(len==-1) len=sGetStringLen(text);

  char *dest = new char[(len*4)+1];
  sU32 *convBuffer = new sU32[len+1];
  for(sInt i=0;i<len;i++) // fake-wchar16-to-wchar32 (argh!)
    convBuffer[i] = text[i];
  
  convBuffer[len] = 0;
  wcstombs(dest,(wchar_t *)convBuffer,len*4+1);
  dest[len*4] = 0;
  
  XStoreBytes(sXDisplay(),dest,strlen(dest));
  delete[] dest;
  delete[] convBuffer;
}

sChar *sGetClipboard()
{
  sChar *result = 0;
  int nbytes;
  
  char *buffer = XFetchBytes(sXDisplay(),&nbytes);
  if(buffer)
  {
    char *tempBuf = sALLOCSTACK(char,nbytes+1);
    sCopyMem(tempBuf,buffer,nbytes);
    tempBuf[nbytes] = 0;
    XFree(buffer);
    
    result = new sChar[nbytes+1];
    sLinuxToWide(result,tempBuf,nbytes+1);
  }
  
  return result;
}

void sEnableFileDrop(sBool enable)
{
}

const sChar *sGetDragDropFile()
{
  static const sChar nofile[] = L"";
  return nofile;
}

/****************************************************************************/

sBool sSystemOpenFileDialog(const sChar *label,const sChar *extensions,sInt flags,const sStringDesc &buffer)
{
  sLogF(L"xlib",L"sSystemOpenFileDialog\n");
  return sFALSE;
}

/****************************************************************************/

sBool sSystemMessageDialog(const sChar *label,sInt flags)
{
  sLogF(L"xlib",L"sSystemMessageDialog\n");
  return sFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Clipping                                                           ***/
/***                                                                      ***/
/****************************************************************************/

static void sChangedRegions()
{
  sRegionBoundingRect(ClipStack[ClipIndex],ClipBounds);
  
  UpdateRegionGC = sTRUE;
  UpdateRegionXft = sTRUE;
  UpdateRegionXR = sTRUE;
}

static void sSetGCRegion()
{
  if(UpdateRegionGC)
  {
    XSetRegion(sXDisplay(),sXGC,ClipStack[ClipIndex]);
    UpdateRegionGC = sFALSE;
  }
}

static void sSetXRRegion()
{
  if(UpdateRegionXR)
  {
    XRenderSetPictureClipRegion(sXDisplay(),XPict,ClipStack[ClipIndex]);
    UpdateRegionXR = sFALSE;
  }
}

void sClipFlush()
{
  sVERIFY(ClipIndex <= 1);
}

void sClipPush()
{
  sVERIFY(ClipIndex<MAX_CLIPS);
  if(ClipIndex)
    XUnionRegion(ClipStack[ClipIndex],EmptyRegion,ClipStack[ClipIndex+1]);
  else
  {
    sRect client;
    XRectangle rect;
    sGetClientArea(client);
    
    rect.x = 0;
    rect.y = 0;
    rect.width = client.x1;
    rect.height = client.y1;
    XUnionRectWithRegion(&rect,EmptyRegion,ClipStack[ClipIndex+1]);
  }
  
  ClipIndex++;
  sChangedRegions();
}

sBool sXUpdateEmpty()
{
  return XEmptyRegion(UpdateRegion) != 0;
}

void sXClipPushUpdate()
{
  sVERIFY(ClipIndex<MAX_CLIPS);
  ClipIndex++;
  XUnionRegion(UpdateRegion,EmptyRegion,ClipStack[ClipIndex]);
  sChangedRegions();
}

void sClipPop()
{
  sVERIFY(ClipIndex>0);
  ClipIndex--;
  sChangedRegions();
}

void sClipExclude(const sRect &r)
{
  sVERIFY(ClipIndex != 0);
  Region temp = sRectToRegion(r);
  XSubtractRegion(ClipStack[ClipIndex],temp,ClipStack[ClipIndex]);
  XDestroyRegion(temp);
  sChangedRegions();
}

void sClipRect(const sRect &r)
{
  sVERIFY(ClipIndex != 0);
  Region temp = sRectToRegion(r);
  XIntersectRegion(ClipStack[ClipIndex],temp,ClipStack[ClipIndex]);
  XDestroyRegion(temp);
  sChangedRegions();
}

/****************************************************************************/
/***                                                                      ***/
/***   Painting                                                           ***/
/***                                                                      ***/
/****************************************************************************/

static void sForeground(sInt colid)
{
  sVERIFY(colid>=0 && colid<MAX_COLORS);
  if(ForegroundCol != colid)
  {
    XSetForeground(sXDisplay(),sXGC,Color[colid][0]);
    ForegroundCol = colid;
  }
}

void sSetColor2D(sInt colid,sU32 color)
{
  sVERIFY(colid>=0 && colid<MAX_COLORS);
  
  if(Color[colid][1] != color)
  {
    XColor col;
    col.red   = ((color>>16) & 0xff)*0x101;
    col.green = ((color>> 8) & 0xff)*0x101;
    col.blue  = ((color>> 0) & 0xff)*0x101;
    col.flags = DoRed|DoGreen|DoBlue;
    if(!XAllocColor(sXDisplay(),sXColMap,&col))
      sFatal(L"XAllocColor failed");
    
    Color[colid][0] = col.pixel;
    Color[colid][1] = color;
    
    XRenderColor rc;
    rc.red = col.red;
    rc.green = col.green;
    rc.blue = col.blue;
    rc.alpha = 0xffff;
    
    if(!XftColorAllocValue(sXDisplay(),sXVisual,sXColMap,&rc,&ColorXFT[colid]))
      sFatal(L"XftColorAllocValue failed");
  }
}

sU32 sGetColor2D(sInt colid)
{
  sVERIFY(colid>=0 && colid<MAX_COLORS);
  return Color[colid][1];
}

void sRect2D(sInt x0,sInt y0,sInt x1,sInt y1,sInt colid)
{
  sRect r;
  r.Init(x0,y0,x1,y1);
  sRect2D(r,colid);
}

void sRect2D(const sRect &r,sInt colid)
{
  if(sRectsIntersect(ClipBounds,r))
  {
    sSetGCRegion();
    sForeground(colid);
    XFillRectangle(sXDisplay(),sXWnd,sXGC,r.x0,r.y0,r.x1-r.x0,r.y1-r.y0);
  }
}

void sRectHole2D(const sRect &out,const sRect &hole,sInt colid)
{
  sRect2D(out .x0,out .y0,out .x1,hole.y0,colid);
  sRect2D(out .x0,hole.y0,hole.x0,hole.y1,colid);
  sRect2D(hole.x1,hole.y0,out .x1,hole.y1,colid);
  sRect2D(out .x0,hole.y1,out .x1,out .y1,colid);
}

void sRectInvert2D(sInt x0,sInt y0,sInt x1,sInt y1)
{
  sRect r;
  r.Init(x0,y0,x1,y1);
  sRectInvert2D(r);
}

void sRectInvert2D(const sRect &r)
{
  if(sRectsIntersect(ClipBounds,r))
  {
    XSetFunction(sXDisplay(),sXGC,GXinvert);
    XFillRectangle(sXDisplay(),sXWnd,sXGC,r.x0,r.y0,r.x1-r.x0,r.y1-r.y0);
    XSetFunction(sXDisplay(),sXGC,GXcopy);
  }
}

void sRectFrame2D(const sRect &r,sInt colid)
{
  sRectFrame2D(r.x0,r.y0,r.x1,r.y1,colid);
}

void sRectFrame2D(sInt x0,sInt y0,sInt x1,sInt y1,sInt colid)
{
  sRect2D(x0  ,y0  ,x1  ,y0+1,colid);
  sRect2D(x0  ,y1-1,x1  ,y1  ,colid);
  sRect2D(x0  ,y0+1,x0+1,y1-1,colid);
  sRect2D(x1-1,y0+1,x1  ,y1-1,colid);
}

void sLine2D(sInt x0,sInt y0,sInt x1,sInt y1,sInt colid)
{
  sSetGCRegion();
  sForeground(colid);
  XDrawLine(sXDisplay(),sXWnd,sXGC,x0,y0,x1,y1);
}

void sLine2D(sInt *list,sInt count,sInt colid)
{
  sSetGCRegion();
  sForeground(colid);
  for(sInt i=1;i<count;i++)
    XDrawLine(sXDisplay(),sXWnd,sXGC,list[i*2-2],list[i*2-1],list[i*2+0],list[i*2+1]);
}

void sLineList2D(sInt *list,sInt count,sInt colid)
{
  sSetGCRegion();
  sForeground(colid);
  for(sInt i=0;i<count;i++)
    XDrawLine(sXDisplay(),sXWnd,sXGC,list[i*4+0],list[i*4+1],list[i*4+2],list[i*4+3]);
}

static void sPicturePixmapFromRawData(Picture &pic,Pixmap &pixmap,const sU32 *data,sInt w,sInt h,sInt stride)
{
  XRenderPictureAttributes attr;
  XGCValues vals;

  attr.repeat = RepeatPad;
  
  // create a pixmap and corresponding picture
  pixmap = XCreatePixmap(sXDisplay(),sXWnd,w,h,32);
  pic = XRenderCreatePicture(sXDisplay(),pixmap,XFmtARGB,CPRepeat,&attr);
  GC gc = XCreateGC(sXDisplay(),pixmap,0,&vals);
  
  // upload data
  XImage *image = XCreateImage(sXDisplay(),sXVisual,32,ZPixmap,0,(char*)data,w,h,32,stride);
  XPutImage(sXDisplay(),pixmap,gc,image,0,0,0,0,w,h);
  
  // cleanup
  image->data = 0;
  XDestroyImage(image);
  XFreeGC(sXDisplay(),gc);
}

void sBlit2D(const sU32 *data,sInt width,const sRect &dest)
{
  Pixmap pixmap;
  Picture pict;
  sInt w = dest.SizeX(), h = dest.SizeY();

  if(sRectsIntersect(ClipBounds,dest))
  {
    sPicturePixmapFromRawData(pict,pixmap,data,w,h,width*4);
    
    sSetXRRegion();
    XRenderComposite(sXDisplay(),PictOpSrc,pict,None,XPict,0,0,0,0,dest.x0,dest.y0,w,h);
    
    XRenderFreePicture(sXDisplay(),pict);
    XFreePixmap(sXDisplay(),pixmap);
  }
}

void sStretch2D(const sU32 *data,sInt width,const sRect &source,const sRect &dest)
{
  Pixmap pixmap;
  Picture pict;
  sInt w = source.SizeX(), h = source.SizeY();

  if(sRectsIntersect(ClipBounds,dest))
  {
    sPicturePixmapFromRawData(pict,pixmap,data,w,h,width*4);
    
    XTransform trafo;
    sClear(trafo);
    trafo.matrix[0][0] = sDivShift(w,dest.SizeX());
    trafo.matrix[1][1] = sDivShift(h,dest.SizeY());
    trafo.matrix[2][2] = 65536;
    sSetXRRegion();
    XRenderSetPictureTransform(sXDisplay(),pict,&trafo);
    XRenderSetPictureFilter(sXDisplay(),pict,FilterGood,0,0);
    XRenderComposite(sXDisplay(),PictOpSrc,pict,None,XPict,0,0,0,0,dest.x0,dest.y0,dest.SizeX(),dest.SizeY());
    
    XRenderFreePicture(sXDisplay(),pict);
    XFreePixmap(sXDisplay(),pixmap);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Offscreen Rendering                                                ***/
/***                                                                      ***/
/****************************************************************************/

void sRender2DBegin(sInt xs,sInt ys)
{
  sLogF(L"xlib",L"sRender2DBegin\n");
}

void sRender2DBegin()
{
  sLogF(L"xlib",L"sRender2DBegin\n");
}

void sRender2DBegin(sImage2D *)
{
	sLogF(L"xlib",L"sRender2DBegin\n");
}

void sRender2DEnd()
{
  sLogF(L"xlib",L"sRender2DEnd\n");
}

void sRender2DSet(sU32 *data)
{
  sLogF(L"xlib",L"sRender2DSet\n");
}

void sRender2DGet(sU32 *data)
{
  sLogF(L"xlib",L"sRender2DGet\n");
}

/****************************************************************************/
/***                                                                      ***/
/***   2D Blitting                                                        ***/
/***                                                                      ***/
/****************************************************************************/

struct sImage2DPrivate
{
  sInt xs,ys;
  Pixmap pixmap;
  Picture pict;
};

sImage2D::sImage2D(sInt xs,sInt ys,sU32 *data)
{
  prv = new sImage2DPrivate;
  prv->xs = xs;
  prv->ys = ys;
  sPicturePixmapFromRawData(prv->pict,prv->pixmap,data,xs,ys,xs*4);

  XRenderSetPictureFilter(sXDisplay(),prv->pict,FilterFast,0,0);
}

sImage2D::~sImage2D()
{
  XRenderFreePicture(sXDisplay(),prv->pict);
  XFreePixmap(sXDisplay(),prv->pixmap);
  delete prv;
}

sInt sImage2D::GetSizeX()
{
  return prv->xs;
}

sInt sImage2D::GetSizeY()
{
  return prv->ys;
}

void sImage2D::Update(sU32 *data)
{
  sLogF(L"xlib",L"sImage2D::Update\n");
}

void sImage2D::Paint(sInt x,sInt y)
{
  sRect r;
  r.Init(x,y,x+prv->xs,y+prv->ys);
  
  if(sRectsIntersect(ClipBounds,r))
  {
    sSetXRRegion();
    XRenderComposite(sXDisplay(),PictOpSrc,prv->pict,None,XPict,0,0,0,0,x,y,prv->xs,prv->ys);
  }
}

void sImage2D::Paint(const sRect &source,sInt x,sInt y)
{
  sRect r;
  r.Init(x,y,x+source.SizeX(),y+source.SizeY());
  
  if(sRectsIntersect(ClipBounds,r))
  {
    sSetXRRegion();
    XRenderComposite(sXDisplay(),PictOpSrc,prv->pict,None,XPict,source.x0,source.y0,0,0,x,y,source.SizeX(),source.SizeY());
  }
}

void sImage2D::Stretch(const sRect &source,const sRect &dest)
{
  if(sRectsIntersect(ClipBounds,dest))
  {
    XTransform trafo;
    sClear(trafo);
    trafo.matrix[0][0] = sDivShift(source.SizeX(),dest.SizeX());
    trafo.matrix[1][1] = sDivShift(source.SizeY(),dest.SizeY());
    trafo.matrix[2][2] = 65536;
    sSetXRRegion();
    XRenderSetPictureTransform(sXDisplay(),prv->pict,&trafo);
    XRenderComposite(sXDisplay(),PictOpSrc,prv->pict,None,XPict,source.x0,source.y0,0,0,dest.x0,dest.y0,dest.SizeX(),dest.SizeY());

    trafo.matrix[0][0] = 65536;
    trafo.matrix[1][1] = 65536;
    trafo.matrix[2][2] = 65536;
    XRenderSetPictureTransform(sXDisplay(),prv->pict,&trafo);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Fonts                                                              ***/
/***                                                                      ***/
/****************************************************************************/

struct sFont2DPrivate
{
  XftFont *Font;
  sBool Underline,Strikeout;
  sInt TextColor,BackColor;
};

sFont2D::sFont2D(const sChar *name,sInt size,sInt flags,sInt width)
{
  prv = new sFont2DPrivate;
  prv->Font = 0;
  Init(name,size,flags,width);
}

sFont2D::~sFont2D()
{
  if(prv->Font) XftFontClose(sXDisplay(),prv->Font);
  delete prv;
}

void sFont2D::AddResource(const sChar *filename)
{
}

void sFont2D::Init(const sChar *name,sInt size,sInt flags,sInt width)
{
  if(prv->Font) XftFontClose(sXDisplay(),prv->Font);

  prv->Font = XftFontOpen(sXDisplay(), sXScreen,
    FC_FAMILY, FcTypeString, sLinuxFromWide(name),
    FC_WEIGHT, FcTypeInteger, (flags & sF2C_BOLD) ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR,
    FC_SLANT, FcTypeInteger, (flags & sF2C_ITALICS) ? FC_SLANT_ITALIC : FC_SLANT_ROMAN,
    FC_PIXEL_SIZE, FcTypeInteger, size,
    NULL);
  if(!prv->Font)
    sLogF(L"xlib",L"sFont2D: XftFontOpen for font \"%s\" failed\n",name);
  
  prv->Underline = (flags & sF2C_UNDERLINE) != 0;
  prv->Strikeout = (flags & sF2C_STRIKEOUT) != 0;
  
  // TODO: implement width, underline, strokeout

  prv->BackColor = 1;
  prv->TextColor = 0;
}

sInt sFont2D::GetHeight()
{
  return prv->Font->height;
}

sInt sFont2D::GetBaseline()
{
  return prv->Font->ascent;
}

sInt sFont2D::GetCharHeight()
{
  return prv->Font->height;
}

sInt sFont2D::GetWidth(const sChar *text,sInt len)
{
  if(len==-1) len=sGetStringLen(text);
  XGlyphInfo extents;
  XftTextExtentsUtf16(sXDisplay(),prv->Font,(const FcChar8*) text,Endianess,len*2,&extents);
  return extents.xOff;
}

sInt sFont2D::GetAdvance(const sChar *text,sInt len)
{
  if(len==-1) len=sGetStringLen(text);
  XGlyphInfo extents;
  XftTextExtentsUtf16(sXDisplay(),prv->Font,(const FcChar8*) text,Endianess,len*2,&extents);
  return extents.width;
}

sInt sFont2D::GetCharCountFromWidth(sInt width,const sChar *text,sInt len)
{
  if(len==-1) len = sGetStringLen(text);
  
  // binary search (yeah, not terribly efficient)
  sInt l=0,r=len;
  while(l<r)
  {
    sInt x = (l+r)/2;
    sInt w = GetWidth(text,x);
    
    if(width < w)
      r = x;
    else
      l = x+1;
  }
  
  if(l>0)
  {
    sInt w = GetWidth(text,l-1);
    sInt wCh = GetAdvance(&text[l],1);
    if(width - w <= wCh/2)
      l--;
  }
  
  return l;
}

sFont2D::sLetterDimensions sFont2D::sGetLetterDimensions(const sChar letter)
{
  sLetterDimensions result;
  sClear(result);
  
  sLogF(L"xlib",L"sFont2D::sGetLetterDimensions\n");
  return result;
}

sBool sFont2D::LetterExists(sChar letter)
{
  sLogF(L"xlib",L"sFont2D::LetterExists\n");
  return sFALSE;
}

void sFont2D::SetColor(sInt text,sInt back)
{
  sVERIFY(text>=0 && text<MAX_COLORS);
  sVERIFY(back>=0 && back<MAX_COLORS);
  prv->BackColor = back;
  prv->TextColor = text;
}

void sFont2D::PrintMarked(sInt flags,const sRect *rc,sInt x,sInt y,const sChar *text,sInt len,sPrintInfo *pi)
{
  if(len==-1) len=sGetStringLen(text);
  sVERIFY(rc);
  sVERIFY(pi);
  sRect r(*rc);
  const sChar *ot = text;
  sInt ol = len;
  sInt ox = x;

  switch(pi->Mode)
  {
  case sPIM_PRINT:
    if(pi->SelectStart<len && pi->SelectEnd>0 && pi->SelectEnd>pi->SelectStart)
    {
      sInt i = pi->SelectStart;
      if(i>0)
      {
        r.x1 = x + GetWidth(text,i);
        PrintBasic(flags,&r,x,y,text,i);
        r.x0 = x = r.x1;

        len -= i;
        pi->SelectEnd -= i;
        pi->SelectStart -= i;
        text += i;
      }

      sU32 oldbackcolor=prv->BackColor;
      if (pi->SelectBackColor!=~0u)
        prv->BackColor=pi->SelectBackColor;
      else
        sSwap(prv->TextColor,prv->BackColor);

      i = sMin(pi->SelectEnd,len);
      r.x1 = x + GetWidth(text,i);
      PrintBasic(flags,&r,x,y,text,i);
      r.x0 = x = r.x1;

      len -= i;
      pi->SelectEnd -= i;
      pi->SelectStart -= i;
      text += i;

      if (pi->SelectBackColor!=~0u)
        prv->BackColor=oldbackcolor;
      else
        sSwap(prv->TextColor,prv->BackColor);
    }
    r.x1 = x + GetWidth(text,len);
    PrintBasic(flags,&r,x,y,text,len);
    if(flags & sF2P_OPAQUE)
    {
      r.x0 = x = r.x1;
      r.x1 = rc->x1;
      if(pi->HintLine && pi->HintLine>=r.x0 && pi->HintLine+2<=r.x1)
      {
        r.x1 = pi->HintLine;
        sRect2D(r,prv->BackColor);
        r.x0 = r.x1;
        r.x1 = r.x0+2;
        sRect2D(r,pi->HintLineColor);
        r.x0 = r.x1;
        r.x1 = rc->x1;
      }
      sRect2D(r,prv->BackColor);
    }

    if(pi->CursorPos>=0 && pi->CursorPos<=ol && pi->Mode==sPIM_PRINT)
    {
      sInt t = GetWidth(ot,pi->CursorPos);
      sInt w = 2;
      if(pi->Overwrite)
      {
        if(pi->CursorPos<ol)
          w = GetWidth(ot+pi->CursorPos,1);
        else
          w = GetWidth(L" ",1);
      }
      sRectInvert2D(ox+t,rc->y0,ox+t+w,rc->y1);
    }
    break;

  case sPIM_POINT2POS:
    if(pi->QueryY>=rc->y0 && pi->QueryY<rc->y1)
    {
      x = ox;
	  sInt i;
      for(i=0;i<len;i++)
      {
        x += GetWidth(ot+i,1);
        if(pi->QueryX < x)
          break;
      }
      pi->QueryPos = ot+i;
      pi->Mode = sPIM_QUERYDONE;
    }
    break;

  case sPIM_POS2POINT:
    if(pi->QueryPos>=ot && pi->QueryPos<=ot+len)
    {
      pi->QueryY = rc->y0;
      pi->QueryX = ox+GetWidth(ot,sInt(pi->QueryPos-ot));
    }
    break;
  }
}

void sFont2D::PrintBasic(sInt flags,const sRect *r,sInt x,sInt y,const sChar *text,sInt len)
{
  if(len==-1) len = sGetStringLen(text);
  
  if(x >= ClipBounds.x1 || y >= ClipBounds.y1)
    return;
  
  if(r)
  {
    if(!sRectsIntersect(ClipBounds,*r))
      return;
    
    sClipPush();
    sClipRect(*r);
  }
  
  if(UpdateRegionXft)
  {
    XftDrawSetClip(XDraw,ClipStack[ClipIndex]);
    UpdateRegionXft = sFALSE;
  }
  
  if((flags & sF2P_OPAQUE) && r)
    XftDrawRect(XDraw,&ColorXFT[prv->BackColor],r->x0,r->y0,r->SizeX(),r->SizeY());
  
  XftDrawStringUtf16(XDraw,&ColorXFT[prv->TextColor],prv->Font,x,y+prv->Font->ascent,(const FcChar8 *)text,Endianess,len*2);
  if(r)
    sClipPop();
}

sInt sFont2D::Print(sInt flags,const sRect &r,const sChar *text,sInt len,sInt margin,sInt xo,sInt yo,sPrintInfo *pi)
{
  if(len==-1) len=sGetStringLen(text);
  sInt x,y;
  sPrintInfo pil;
  const sChar *textstart=text;
  sInt result;

  sRect r2(r);

  pil.Init();
  if(pi)
  {
    pil = *pi;
  }
  pil.CursorPos = -1;
  pil.Overwrite = 0;
  pil.SelectStart = -1;
  pil.SelectEnd = -1;

  r2.Extend(-margin);
  x = r2.x0 + xo;
  y = r2.y0 + yo;
  sInt xs = r2.SizeX();
  sInt h = GetHeight();
  sRect rs(r);
  result = rs.y0;

  if(pil.Mode==sPIM_POINT2POS && pil.QueryY<y)
  {
    pi->Mode = sPIM_QUERYDONE;
    pi->QueryPos = text;
    return result;
  }

  if(flags & sF2P_MULTILINE)
  {
    const sChar *textend = text+len;
    for(;;)
    {
      sInt i=0;
      sInt best = 0;
      if(pi)
      {
        pil.CursorPos   = pi->CursorPos   - sInt(text-textstart);
        pil.Overwrite   = pi->Overwrite;
        pil.SelectStart = pi->SelectStart - sInt(text-textstart);
        pil.SelectEnd   = pi->SelectEnd   - sInt(text-textstart);
      }
      for(;;)
      {
        while(text+i<textend && text[i]!=' ' && text[i]!='\n')
          i++;
        if(GetWidth(text,i)>xs)
          break;
        best = i;
        if(text[i]=='\n')
          break;
        if(text+i>=textend)
          break;
        i++;
      }
      if(best==0)
      {
        i = 0;
        while(text+i<textend && text[i]!=' ' && text[i]!='\n' && GetWidth(text,i)<=xs)
          i++;
        best = i-1;
        if(best<1 && text<textend && text[0]!=' ' && text[0]!='\n')
          best = 1;
        if(best<0)
          best = 0;
        else if(pil.CursorPos>=best)
          pil.CursorPos = -1;
      }

      rs.y1 = y + h;
      PrintMarked(flags,&rs,x,y,text,best,&pil);
      rs.y0 = rs.y1;
      y+= h;
      text+=best;
      if(text>=textend) break;
      if(text[0]==' ' || text[0]=='\n')
        text++;
    }

    result = y+margin;
    rs.y1 = r.y1;
    if(rs.y1>rs.y0 && pil.Mode==sPIM_PRINT && (flags & sF2P_OPAQUE))
    {
      sRect r(rs);
      if(pil.HintLine && pil.HintLine>=r.x0 && pil.HintLine+2<=r.x1)
      {
        r.x1 = pil.HintLine;
        sRect2D(r,prv->BackColor);
        r.x0 = r.x1;
        r.x1 = r.x0+2;
        sRect2D(r,pil.HintLineColor);
        r.x0 = r.x1;
        r.x1 = rs.x1;
      }
      sRect2D(r,prv->BackColor);
    }

    if(pi)
    {
      pi->QueryPos = pil.QueryPos;
      pi->QueryX = pil.QueryX;
      pi->QueryY = pil.QueryY;
      pi->Mode = pil.Mode;
    }
  }
  else
  {
    sInt space=0;
    if(flags & sF2P_SPACE)
      space = GetWidth(L" ");
    if(flags & sF2P_LEFT)
    {
      x = r2.x0 + space;
    }
    else if(flags & sF2P_RIGHT)
    {
      x = r2.x1 - GetWidth(text,len) - space;
    }
    else
    {
      x = (r2.x0 + r2.x1 - GetWidth(text,len))/2;
    }
    if(flags & sF2P_TOP)
    {
      y = r2.y0;
    }
    else if(flags & sF2P_BOTTOM)
    {
      y = r2.y1 - h;
    }
    else
    {
      y = (r2.y0 + r2.y1 - h)/2;
    }

    if(pi)
    {
      sPrintInfo pi2 = *pi;
      PrintMarked(flags,&r,x+xo,y+yo,text,len,&pi2);
      if(pi->Mode)
      {
        pi->QueryPos = pi2.QueryPos;
        pi->QueryX = pi2.QueryX;
        pi->QueryY = pi2.QueryY;
      }
    }
    else
    {
      PrintBasic(flags,&r,x,y,text,len);
    }

    result = y+h;
  }

  return result;
}

#endif // sCOMMANDLINE

/****************************************************************************/

