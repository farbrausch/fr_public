/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "doc.hpp"
//#include "gui.hpp"
#include "build.hpp"
#include "base/system.hpp"
#include "util/image.hpp"
#include "util/scanner.hpp"
#include "wz4lib/serials.hpp"
#include "wz4lib/script.hpp"
#include "wz4lib/basic_ops.hpp"
#include "wz4lib/wz4shaders.hpp"
#include "gui/color.hpp"

class wDocument *Doc;

#define LOGIT 0 //!sRELEASE

/****************************************************************************/
/***                                                                      ***/
/***   painting                                                           ***/
/***                                                                      ***/
/****************************************************************************/

wPaintInfo::wPaintInfo()
{
  Window = 0;
  Op = 0;
  Client.Init(0,0,0,0);
  Enable3D = 0;
  ViewLog = 0;

  ShowHandles = 0;
  Dragging = 0;
  HandleEnable = 0;
  DeleteSelectedHandles = 0;
  HandleMode = 0;
  HandleColor = 0;
  HandleColorIndex = 0;
  DontPaintHandles = 0;
  SelectFrame.Init(0,0,0,0);
  SelectMode = 0;

  PosX = 0;
  PosY = 0;
  Zoom2D = 0;
  Tile = 0;
  Alpha = 0;

  View = 0;
  Zoom3D = 1.7f;
  FlatMtrl = 0;
  ShadedMtrl = 0;
  DrawMtrl = 0;
  Env = 0;
  LineGeo = 0;
  LineGeo2 = 0;
  Grid = 0;
  Wireframe = 0;
  CamOverride = 0;
  BackColor = 0x00000000;
  GridColor = 0xff404040;
  SplineMode = 0;
  Lod = 2;    // high lod
  Image = 0;
  AlphaImage = 0;
  Rect.Init(0,0,0,0);
  SetCam = 0;
  SetCamZoom = 0;
  MTMFlag = 0;
  CacheWarmup = 0;
  CacheWarmupAgain = 0;

  ClearMisc();

  TexGeoVP = 0;
  TexGeoVC = 0;
  sClear(LineGeoVP);
  sClear(LineGeoVC);

  Image = new sImage;
  AlphaImage = new sImage;

  FlatMtrl = new sSimpleMaterial;
  FlatMtrl->Flags = sMTRL_ZON;
  FlatMtrl->Prepare(sVertexFormatBasic);
  ShadedMtrl = new sSimpleMaterial;
  ShadedMtrl->Flags = sMTRL_LIGHTING | sMTRL_ZON | sMTRL_CULLON;
  ShadedMtrl->Prepare(sVertexFormatStandard);
  DrawMtrl = new sSimpleMaterial;
  DrawMtrl->Flags = sMTRL_ZOFF;
  DrawMtrl->Prepare(sVertexFormatBasic);

  TexGeo = new sGeometry;
  TexGeo->Init(sGF_QUADLIST,sVertexFormatSingle);
  TexDummy = new sTexture2D(16,16,sTEX_ARGB8888,1);
  TexMtrl = new sSimpleMaterial;
  TexMtrl->Texture[0] = TexDummy;
  TexMtrl->TFlags[0] = sMTF_LEVEL0|sMTF_TILE;
  TexMtrl->Prepare(sVertexFormatSingle);

  TexAMtrl = new AlphaMtrl;
  TexAMtrl->Texture[0] = TexDummy;
  TexAMtrl->TFlags[0] = sMTF_LEVEL0|sMTF_TILE;
  TexAMtrl->Prepare(sVertexFormatSingle);

  LineGeo = new sGeometry;
  LineGeo->Init(sGF_LINELIST,sVertexFormatBasic);

  LineGeo2 = new sGeometry;
  LineGeo2->Init(sGF_LINELIST,sVertexFormatBasic);

  static sInt countdown = 5;
  if(countdown>0) 
    countdown--;
  else
    sLog(L"wz4",L"do not create wPaintInfo objects every frame. It's expensive!\n");
}

wPaintInfo::~wPaintInfo()
{
  delete Image;
  delete AlphaImage;

  delete FlatMtrl;
  delete ShadedMtrl;
  delete DrawMtrl;

  delete TexGeo;
  delete TexMtrl;
  delete TexAMtrl;
  delete TexDummy;
  delete LineGeo;
  delete LineGeo2;
}

void wPaintInfo::AddHandle(wOp *op,sInt id,const sRect &r,sInt mode,sInt *t,sF32 *x,sF32 *y,sF32 *z,sInt arrayline)
{
  sVERIFY(op);
  sVERIFY(id>0);

  sInt index = Handles.GetCount();
  sInt selindex = -1;
  sBool sel = IsSelected(op,id);
  wHandleSelectTag *tag;
  if(sel)
  {
    sFORALL(Doc->SelectedHandleTags,tag)
    {
      if(tag->Op==op && tag->Id==id)
      {
        selindex = _i;
        break;
      }
    }
  }

  wHandle *hnd = Handles.AddMany(1);
  hnd->Op = op;
  hnd->Id = id;
  hnd->Mode = mode;
  hnd->t = t;
  hnd->x = x;
  hnd->y = y;
  hnd->z = z;
  hnd->HitBox = r;
  hnd->Local = HandleTrans;
  hnd->ArrayLine = arrayline;
  hnd->Selected = sel;    
  hnd->Index = index;
  hnd->SelectIndex = selindex;

  if(sel)
  {
    SelectedHandles.AddTail(index);
  }
}
  
sBool wPaintInfo::SelectionNotEmpty()
{
  return Doc->SelectedHandleTags.GetCount()>0; 
}

void wPaintInfo::ClearHandleSelection()
{
  wHandle *hnd;
  wOp *op;

  Dragging = 0;
  sFORALL(Handles,hnd)
    if(hnd->Op) hnd->Op->HighlightArrayLine = -1;

  sFORALL(Doc->AllOps,op)
    op->SelectedHandles.Clear();

  SelectedHandles.Clear();
  Doc->SelectedHandleTags.Clear();
}

sBool wPaintInfo::IsSelected(wOp *op,sInt id) const
{
  return op->SelectedHandles[id];
}

sInt wPaintInfo::FirstSelectedId(wOp *op) const
{
  sInt max = op->SelectedHandles.GetCount();
  for(sInt i=1;i<max;i++)
  {
    if(op->SelectedHandles[i])
      return i;
  }
  return 0;
}

void wPaintInfo::SelectHandle(wHandle *hit,sInt mode)
{
  wHandleSelectTag *tag = 0;     
  wOp *op=0;
  wHandle *lasthnd = 0;
  sBool found = 0;
  switch(mode)
  {
  case 0: // clear all, then set. This mode is special, since it updates SelectedHandles immediatly!
    ClearHandleSelection();
    Doc->SelectedHandleTags.AddTail(wHandleSelectTag(hit->Op,hit->Id));
    SelectedHandles.AddTail(hit->Index);
    hit->Selected = 1;
    hit->SelectIndex = 0;
    hit->Op->SelectedHandles[hit->Id] = 1;
    hit->Op->HighlightArrayLine = hit->ArrayLine;
    Doc->AppScrollToArrayLineMsg.Code = hit->ArrayLine;
    Doc->AppScrollToArrayLineMsg.Send();
    break;

  case 1: // add to handles
    if(!IsSelected(hit->Op,hit->Id))
    {
      lasthnd = hit;
      Doc->SelectedHandleTags.AddTail(wHandleSelectTag(hit->Op,hit->Id));
    }
    sFORALL(Doc->AllOps,op)
      op->HighlightArrayLine = -1;
    if(lasthnd)
    {
      lasthnd->Op->HighlightArrayLine = lasthnd->ArrayLine;
      Doc->AppScrollToArrayLineMsg.Code = lasthnd->ArrayLine;
      Doc->AppScrollToArrayLineMsg.Send();
    }

    break;

  case 2: // rem from handles
    sFORALL(Doc->SelectedHandleTags,tag)
    {
      if(tag->Id==hit->Id && tag->Op==hit->Op)
      {
        Doc->SelectedHandleTags.RemAt(_i);
        break;
      }
    }
    sFORALL(Doc->AllOps,op)
      op->HighlightArrayLine = -1;
    break;
  case 3: // toggle
    found = 0;
    sFORALL(Doc->SelectedHandleTags,tag)
    {
      if(tag->Id==hit->Id && tag->Op==hit->Op)
      {
        Doc->SelectedHandleTags.RemAt(_i);
        found = 1;
        break;
      }
    }
    if(!found)
      Doc->SelectedHandleTags.AddTail(wHandleSelectTag(hit->Op,hit->Id));
    sFORALL(Doc->AllOps,op)
      op->HighlightArrayLine = -1;
    break;
  }
  sGui->Notify(*op);
}


void wPaintInfo::PaintHandles(const sMatrix34 *mat)
{
  if(DontPaintHandles)
    return;

  wHandleSelectTag *tag;
  wOp *op;

  // update wOp::SelectedHandles

  sFORALL(Doc->AllOps,op)
    op->SelectedHandles.Clear();
  sFORALL(Doc->SelectedHandleTags,tag)
    tag->Op->SelectedHandles[tag->Id] = 1;

  // clear all intermediate handle structures

  SelectedHandles.Clear();
  Handles.Clear();
  HandleTrans.Init();
  HandleTransI.Init();
  if(mat)
    Transform3D(*mat);
  if(ShowHandles)
  {
/*
    switch(HandleMode)
    {
      case 0:  HandleColor = 0xffffff00;  break;
      case 1:  HandleColor = 0xffff00ff;  break;
      case 2:  HandleColor = 0xff00ffff;  break;
    }
*/
    HandleColor = 0xffffff00;
    HandleColorIndex = 0;
    sSetColor2D(HandleColorIndex,HandleColor);

    if(Enable3D)
    {
      sSetTarget(sTargetPara(0,0,Spec));
      HandleView.Orthogonal = sVO_PIXELS;
      HandleView.SetTargetCurrent();
      HandleView.Prepare();
      View->SetTargetCurrent();
      View->Model.Init();
      View->Prepare();
    }
    PaintHandlesR(Op,0);
    if(SelectMode)
    {
      sRect rr;
      rr = SelectFrame;
      rr.Sort();
      if(Enable3D)
      {
        rr.x0 -= Client.x0;
        rr.y0 -= Client.y0;
        rr.x1 -= Client.x0;
        rr.y1 -= Client.y0;
        
        sU32 col = 0xffffff00;
        PaintAddLine(rr.x0-0.5f,rr.y0-0.5f,1, rr.x1-0.5f,rr.y0-0.5f,1, col,sTRUE);
        PaintAddLine(rr.x1-0.5f,rr.y0-0.5f,1, rr.x1-0.5f,rr.y1-0.5f,1, col,sTRUE);
        PaintAddLine(rr.x1-0.5f,rr.y1-0.5f,1, rr.x0-0.5f,rr.y1-0.5f,1, col,sTRUE);
        PaintAddLine(rr.x0-0.5f,rr.y1-0.5f,1, rr.x0-0.5f,rr.y0-0.5f,1, col,sTRUE);
      }
      else
      {
        sRectFrame2D(rr,sGC_YELLOW);
      }
    }

    // paint all handles
    PaintFlushRect();
    PaintFlushLine(sFALSE);
    PaintFlushLine(sTRUE);
  }
}

void wPaintInfo::PaintHandlesR(wOp *parent,sBool paint)
{
  wOp *input;
  wOpInputInfo *info;
  sMatrix34 mat;
  sMatrix34 mati;

  // don't go deeper if we switch 2D/3d mode!

  if(parent->Select && parent->Page==Doc->CurrentPage)
    paint = 1;
  switch(HandleMode)
  {
    case 0: HandleEnable = 1; break;
    case 1: HandleEnable = (parent->Select && parent->Page==Doc->CurrentPage); break;
    case 2: HandleEnable = paint; break;
  }
  if(parent->Select) HandleColor = 0xffff00ff;
  else if(paint) HandleColor = 0xff00ffff;
  else HandleColor = 0xffffff00;
  if(!Enable3D)
    sSetColor2D(HandleColorIndex,HandleColor);

  if(parent->Class->Handles)
  {
    if(Enable3D && parent->Class->OutputType->GuiSets & wTG_2D) return;
    if(!Enable3D && parent->Class->OutputType->GuiSets & wTG_3D) return;
  }

  // actually paint/add handles

  if(parent->Class->Handles)
  {
    mat  = HandleTrans;
    mati = HandleTransI;
    (*parent->Class->Handles)(*this,parent);
  }

  // recurse

  if(!(parent->Class->Flags & wCF_BLOCKHANDLES))
  {
    sFORALL(parent->Inputs,input)
      PaintHandlesR(input,paint);
    sFORALL(parent->Links,info)
      if(info->Link)
        PaintHandlesR(info->Link,paint);
  }

  // restore matrices

  if(parent->Class->Handles)
  {
    HandleTrans  = mat;
    HandleTransI = mati;
  }
}

void wPaintInfo::PaintHandle(sInt x,sInt y,sRect &r,sBool select)
{
  r.Init(x-3,y-3,x+4,y+4);
  sRect rr = r;
  if(!Enable3D)
  {
    sRectFrame2D(r,HandleColorIndex);
    if(select)
    {
      rr.Extend(-1);
      sRect2D(rr,sGC_SELECT);
    }
    sClipExclude(r);
  }
  else
  {
    rr.x0 -= Client.x0;
    rr.y0 -= Client.y0;
    rr.x1 -= Client.x0;
    rr.y1 -= Client.y0;
    
    PaintAddRect2D(rr,HandleColor);
    rr.Extend(-1);
    PaintAddRect2D(rr,sGetColor2D(select ? sGC_SELECT : sGC_BLACK));
  }
}

void wPaintInfo::PaintAddRect2D(const sRect &rr,sU32 col)
{
  static const sInt BatchSize = 16384; // verts

  if(!TexGeoVP)
  {
    TexGeo->BeginLoadVB(BatchSize,sGD_STREAM,(void**) &TexGeoVP);
    TexGeoVC = 0;
  }

  TexGeoVP[TexGeoVC+0].Init(rr.x0-0.5f,rr.y0-0.5f,1,col,0,0);
  TexGeoVP[TexGeoVC+1].Init(rr.x1-0.5f,rr.y0-0.5f,1,col,1,0);
  TexGeoVP[TexGeoVC+2].Init(rr.x1-0.5f,rr.y1-0.5f,1,col,1,1);
  TexGeoVP[TexGeoVC+3].Init(rr.x0-0.5f,rr.y1-0.5f,1,col,0,1);
  TexGeoVC += 4;

  sVERIFY(TexGeoVC <= BatchSize);
  if(TexGeoVC == BatchSize)
    PaintFlushRect();
}

void wPaintInfo::PaintFlushRect()
{
  if(!TexGeoVP)
    return;

  // unlock
  TexGeo->EndLoadVB(TexGeoVC);
  TexGeoVP = 0;

  // paint
  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(HandleView,*Env);
  DrawMtrl->Set(&cb);
  TexGeo->Draw();
}

void wPaintInfo::PaintAddLine(sF32 x0,sF32 y0,sF32 z0,sF32 x1,sF32 y1,sF32 z1,sU32 col,sBool zoff)
{
  static const sInt BatchSize = 16384; // verts
  sInt ind = zoff ? 1 : 0;

  if(!LineGeoVP[ind])
  {
    sGeometry *geo = zoff ? LineGeo : LineGeo2;
    geo->BeginLoadVB(BatchSize,sGD_STREAM,(void**) &LineGeoVP[ind]);
    LineGeoVC[ind] = 0;
  }

  sVertexBasic *vp = LineGeoVP[ind];
  sInt &vc = LineGeoVC[ind];

  vp[vc+0].Init(x0,y0,z0,col);
  vp[vc+1].Init(x1,y1,z1,col);
  vc += 2;

  sVERIFY(vc <= BatchSize);
  if(vc == BatchSize)
    PaintFlushLine(zoff);
}

void wPaintInfo::PaintFlushLine(sBool zoff)
{
  sInt ind = zoff ? 1 : 0;
  sGeometry *geo = zoff ? LineGeo : LineGeo2;
  sSimpleMaterial *mtrl = zoff ? DrawMtrl : FlatMtrl;

  if(!LineGeoVP[ind])
    return;

  // unlock
  geo->EndLoadVB(LineGeoVC[ind]);
  LineGeoVP[ind] = 0;

  // paint
  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(HandleView,*Env);
  mtrl->Set(&cb);
  geo->Draw();
}

void wPaintInfo::ClearMisc()
{
  sClear(ParaI);
  sClear(ParaF);
}

void wPaintInfo::SetSizeTex2D(sInt xs,sInt ys)
{
  if(Enable3D)
  {
    Rect.x0 = PosX;
    Rect.y0 = PosY;
  }
  else
  {
    Rect.x0 = Client.x0 + PosX;
    Rect.y0 = Client.y0 + PosY;
  }
  sInt zoom = Zoom2D;     // windows can't handle larger than 32k pixels!
  while(zoom>8 && ((xs<<(zoom-8))>0x4000 || (ys<<(zoom-8))>0x4000))
    zoom--;
  Rect.x1 = Rect.x0 + ((xs<<zoom)>>8);
  Rect.y1 = Rect.y0 + ((ys<<zoom)>>8);
}

void wPaintInfo::MapTex2D(sF32 xin,sF32 yin,sInt &xout,sInt &yout)
{
  xout = sInt(xin*Rect.SizeX())+Rect.x0;
  yout = sInt(yin*Rect.SizeY())+Rect.y0;
}

void wPaintInfo::PaintTex2D(sTexture2D *tex)
{
  if(Enable3D)
  {
    sViewport view;
    sVertexSingle *vp;
    sRect r;
    sF32 s;

    sSetTarget(sTargetPara(sST_CLEARALL,sGetColor2D(sGC_BACK),Spec));

    view.Orthogonal = sVO_PIXELS;
    view.SetTargetCurrent();
    view.Prepare();

    if(Alpha)
    {
      TexAMtrl->Texture[0] = tex;
      sCBuffer<sSimpleMaterialPara> cb;
      cb.Data->Set(view);
      TexAMtrl->Set(&cb);
    }
    else
    {
      TexMtrl->Texture[0] = tex;
      sCBuffer<sSimpleMaterialPara> cb;
      cb.Data->Set(view);
      TexMtrl->Set(&cb);
    }
    TexGeo->BeginLoadVB(4,sGD_STREAM,(void **)&vp);
    if(Tile)
    {
      sInt xs = Rect.SizeX();
      sInt ys = Rect.SizeY();
      r.x0 = Rect.x0-xs;
      r.y0 = Rect.y0-ys;
      r.x1 = r.x0+xs*3;
      r.y1 = r.y0+xs*3;
      s = 3;
    }
    else
    {
      r = Rect;
      s = 1;
    }
    vp[0].Init(r.x0-0.5f,r.y0-0.5f,1,0xffffffff,0,0);
    vp[1].Init(r.x1-0.5f,r.y0-0.5f,1,0xffffffff,s,0);
    vp[2].Init(r.x1-0.5f,r.y1-0.5f,1,0xffffffff,s,s);
    vp[3].Init(r.x0-0.5f,r.y1-0.5f,1,0xffffffff,0,s);

    TexGeo->EndLoadVB();
    TexGeo->Draw();
    Grid = 0;
  }
}

void wPaintInfo::PaintTex2D(sImage *img)
{
  if(!Enable3D)
  {
    sString<256> info;

    info.PrintF(L"%d x %d",img->SizeX,img->SizeY);

    sInt xs = Rect.SizeX();
    sInt ys = Rect.SizeY();
    if(Alpha)
    {
      AlphaImage->Init(img->SizeX,img->SizeY);
      for(sInt i=0;i<img->SizeX*img->SizeY;i++)
      {
        sU32 a = img->Data[i]>>24;
        AlphaImage->Data[i] = 0xff000000 | (a<<16) | (a<<8) | a;
      }
      img = AlphaImage;
    }

    sRect all(img->SizeX,img->SizeY);
    if(Tile)
    {
      sRect r;
      r.x0 = Rect.x0-xs;
      r.y0 = Rect.y0-ys;
      r.x1 = r.x0+xs*3;
      r.y1 = r.y0+xs*3;
      sRectHole2D(Client,r,sGC_BACK);
      sGui->FixedFont->Print(0,r.x0,r.y1+6,info);
      for(sInt y=-1;y<=1;y++)
      {
        for(sInt x=-1;x<=1;x++)
        {
          r.x0 = Rect.x0+xs*x;
          r.y0 = Rect.y0+ys*y;
          r.x1 = r.x0+xs;
          r.y1 = r.y0+ys;
          sStretch2D(img->Data,img->SizeX,all,r);
        }
      }
    }
    else
    {
      sRectHole2D(Client,Rect,sGC_BACK);
      sGui->FixedFont->Print(0,Rect.x0,Rect.y1+6,info);
      sStretch2D(img->Data,img->SizeX,all,Rect);
    }
  }
}

void wPaintInfo::LineTex2D(sF32 x0,sF32 y0,sF32 x1,sF32 y1)
{
  if(!Enable3D && HandleEnable)
  {
    sInt ix0,iy0,ix1,iy1;

    MapTex2D(x0,y0,ix0,iy0);
    MapTex2D(x1,y1,ix1,iy1);
    sLine2D(ix0,iy0,ix1,iy1,HandleColorIndex);
  }
}

void wPaintInfo::HandleTex2D(wOp *op,sInt id,sF32 &x,sF32 &y,sInt arrayline)
{
  if(!Enable3D && HandleEnable)
  {
    sInt ix,iy;
    sRect r;

    MapTex2D(x,y,ix,iy);

    PaintHandle(ix,iy,r,IsSelected(op,id));
    AddHandle(op,id,r,wHM_TEX2D,0,&x,&y,0,arrayline);
  }
}

/****************************************************************************/

sBool wPaintInfo::Map3D(const sVector31 &pos,sF32 &x,sF32 &y,sF32 *zp=0)
{
  sVector4 screen;
  screen = pos * View->MVPMatrix();
  if(screen.z>0.001f)
  {
    x = View->Target.x0 + View->Target.SizeX() * (1+screen.x/screen.w)* 0.5f;
    y = View->Target.y0 + View->Target.SizeY() * (1-screen.y/screen.w)* 0.5f;
    if(zp) *zp = screen.z/screen.w;
    return 1;
  }
  else
  {
    x = 0;
    y = 0;
    return 0;
  }
}

void wPaintInfo::Handle3D(wOp *op,sInt id,sVector31 &pos,sInt mode,sInt arrayline)
{
  Handle3D(op,id,&pos.x,&pos.y,&pos.z,mode,arrayline);
}

void wPaintInfo::Handle3D(wOp *op,sInt id,sF32 *xx,sF32 *yy,sF32 *zz,sInt mode,sInt arrayline)
{
  sRect r;
  sF32 x,y;

  if(HandleEnable)
  {
    if(mode==wHM_RAY && Dragging && IsSelected(op,id))
    {
      wHandleSelectTag *tag;
      sFORALL(Doc->SelectedHandleTags,tag)
      {
        if(tag->Op==op && tag->Id==id)
        {
          *xx += tag->Drag.x;
          *yy += tag->Drag.y;
          *zz += tag->Drag.z;
        }
      }
    }
    sVector31 pos(*xx,*yy,*zz);

    if(Map3D(pos*HandleTrans,x,y))
      PaintHandle(sInt(x),sInt(y),r,IsSelected(op,id));

    AddHandle(op,id,r,mode,0,xx,yy,zz,arrayline);
  }
}

void wPaintInfo::Line3D(const sVector31 &a,const sVector31 &b,sU32 color,sBool zoff)
{
  sF32 x0,y0,z0,x1,y1,z1;

  if(HandleEnable && Map3D(a*HandleTrans,x0,y0,&z0) && Map3D(b*HandleTrans,x1,y1,&z1))
  {
    x0 -= Client.x0;
    y0 -= Client.y0;
    x1 -= Client.x0;
    y1 -= Client.y0;

    if(color==0)
      color = HandleColor;
    else if(color==0xffffffffU)
      color = 0xff000000 | ((HandleColor&0xfefefe)>>1);

    PaintAddLine(x0,y0,z0,x1,y1,z1,color,zoff);
  }
}

void wPaintInfo::Transform3D(const sMatrix34 &mat)
{
  HandleTrans = mat * HandleTrans;
  HandleTransI = HandleTrans;
  HandleTransI.Invert34();
}

/****************************************************************************/
/****************************************************************************/

void wPaintInfo::PaintMtrl()
{
  sInt ty = 24;
  sInt tx = 24;
  sF32 ro = 2;
  sF32 ri = 0.5;
  sF32 u,v,fx,fy;
  sU16 *ip;

  sGeometry *Geo = new sGeometry;     // usually i hate new in frame-loops
  Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);


  sVertexStandard *vp;
  Geo->BeginLoadVB(tx*ty,sGD_STREAM,(void **)&vp);

  for(sInt y=0;y<ty;y++)
  {
    for(sInt x=0;x<tx;x++)
    {
      u = sF32(x)/tx; fx = u*sPI2F;
      v = sF32(y)/ty; fy = v*sPI2F;
      vp->px = -sFCos(fy)*(ro+sFSin(fx)*ri);
      vp->py = -sFCos(fx)*ri;
      vp->pz = sFSin(fy)*(ro+sFSin(fx)*ri);
      vp->nx = -sFCos(fy)*sFSin(fx);
      vp->ny = -sFCos(fx);
      vp->nz = sFSin(fy)*sFSin(fx);
      vp->u0 = u;
      vp->v0 = v;
      vp++;
    }
  }
  Geo->EndLoadVB();
  
  Geo->BeginLoadIB(tx*ty*6,sGD_STREAM,(void **)&ip);
  for(sInt y=0;y<ty;y++)
  {
    for(sInt x=0;x<tx;x++)
    {
      sQuad(ip,0,
        (y+0)%ty*tx+(x+0)%tx,
        (y+1)%ty*tx+(x+0)%tx,
        (y+1)%ty*tx+(x+1)%tx,
        (y+0)%ty*tx+(x+1)%tx);
    }
  }
  Geo->EndLoadIB();

  Geo->Draw();

  delete Geo;
}


/****************************************************************************/
/***                                                                      ***/
/***   Types and classes                                                  ***/
/***                                                                      ***/
/****************************************************************************/

wType::wType()
{
  Color = 0;
  Label = L"";
  Symbol = L"";
  Flags = 0;
  GuiSets = 0;
  Parent = 0;
  Secondary = 0;
  Order = 99;
  ColumnHeaders[0] = L"generator";
  ColumnHeaders[1] = L"filter";
  ColumnHeaders[2] = L"merge";
  ColumnHeaders[3] = L"mix types";
  ColumnHeaders[30] = L"any type";
  EquivalentType = 0;
}

sBool wType::IsType(wType *type)
{
  wType *owntype = this;
  do
  {
    if(type==owntype)
      return 1;
    owntype = owntype->Parent;
  }
  while(owntype);
  return 0;
}

sBool wType::IsTypeOrConversion(wType *type)
{
  wClass *cl;

  if(IsType(type)) return 1;
  sFORALL(Doc->Conversions,cl)
    if(cl->OutputType->IsType(type) && IsType(cl->Inputs[0].Type))
      return 1;
  return 0;
}

void wType::Show(wObject *obj,wPaintInfo &pi)
{
  if(pi.Enable3D) 
    sSetTarget(sTargetPara(sST_CLEARALL,pi.BackColor,pi.Spec));
  else
    sRect2D(pi.Client,sGC_BACK);
  pi.PaintHandles();
}

/****************************************************************************/

wClass::wClass()
{
  Name = L"";
  Column = 0;
  Shortcut = 0;
  GridColumns = 0;

  Flags = 0;
  ParaWords = 0;
  ParaStrings = 0;
  HelperWords = 0;
  ArrayCount = 0;
  OutputType = 0;
  TabType = 0;
  FileInMask = 0;
  FileOutMask = 0;

  MakeGui = 0;
  Command = 0;
  Handles = 0;
  SetDefaults = 0;
  SetDefaultsArray = 0;
  Actions = 0;
  SpecialDrag = 0;
  GetDescription = 0;
  CustomEd = 0;
  BindPara = 0;
  Bind2Para = 0;
  Bind3Para = 0;
  WikiText = 0;
}

void wClass::Tag()
{
  wClassInputInfo *in;
  sFORALL(Inputs,in)
  {
    in->Type->Need();
    in->DefaultClass->Need();
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   pages                                                              ***/
/***                                                                      ***/
/****************************************************************************/

wPage::wPage()
{
  IsTree = 0;
  ScrollX = 0;
  ScrollY = 0;
  Include = 0;
  Temp = 0;
  ManualWriteProtect = 0;
}

void wPage::Tag()
{
  sNeed(Ops);
  sNeed(Tree);
}

void wPage::DefaultName()
{
  sInt max = Doc->Pages.GetCount()+2;
  sU8 *mask = new sU8[max];
  sSetMem(mask,0,max);
  sInt val;
  wPage *page;

  Name=L"new page";

  sFORALL(Doc->Pages,page)
  {
    if(sCmpMem(page->Name,L"new page ",sizeof(sChar)*9)==0)
    {
      const sChar *s = page->Name+9;
      if(sScanInt(s,val))
      {
        if(val<max && val>=0)
          mask[val] = 1;
      }
    }
  }
  for(sInt i=1;i<max;i++)
  {
    if(mask[i]==0)
    {
      sSPrintF(Name,L"new page %d",i);
      break;
    }
  }

  delete[] mask;
}

sBool wPage::CheckDest(wOp *op0,sInt x,sInt y,sInt w,sInt h,sBool move)
{
  wStackOp *op;
  sRect r0,r1;

  if(x<0 || x+w>=wPAGEXS) return 0;
  if(y<0 || y+h>=wPAGEYS) return 0;

  if(op0 && op0->Class->Flags & wCF_COMMENT)
    return 1;

  r0.Init(x,y,x+w,y+h);
  sFORALL(Ops,op)
  {
    if(!(op->Class->Flags & wCF_COMMENT))
    {
      if(!move || !op->Select)
      {
        r1.Init(op->PosX,op->PosY,op->PosX+op->SizeX,op->PosY+op->SizeY);
        if(r0.IsInside(r1))
          return 0;
      }
    }
  }
  return 1;
}

sBool wPage::CheckMove(sInt dx,sInt dy,sInt dw,sInt dh,sBool move)
{
  wStackOp *op0;

  sFORALL(Ops,op0)
  {
    if(op0->Select)
    {
      if(!CheckDest(op0,
         op0->PosX+dx,
         op0->PosY+dy,
         sClamp(op0->SizeX+dw,1,wPAGEXS-op0->SizeX-dw),
         sClamp(op0->SizeY+dh,1,wPAGEYS-op0->SizeY-dh),
         move))
        return 0;
    }
  }
  return 1;
}

void wPage::Rem(wOp *op)
{
  if(IsTree)
    Tree.RemOrder((wTreeOp *)op);
  else
    Ops.Rem((wStackOp *)op);
}

template <class streamer> void wPage::Serialize_(streamer &s)
{
  wStackOp *sop;
  wTreeOp *top;

  sInt version = s.Header(sSerId::wPage,1);
  if(version>0)
  {
    s | Name | IsTree | ScrollX | ScrollY;

    s.ArrayNew(Ops);
    sFORALL(Ops,sop)
      s | sop;

    s.ArrayNew(Tree);
    sFORALL(Tree,top)
      s  | top;

    s.Footer();
  }
}

void wPage::Serialize(sWriter &s) { Serialize_(s); }
void wPage::Serialize(sReader &s) { Serialize_(s); }

/****************************************************************************/
/***                                                                      ***/
/***   ops                                                                ***/
/***                                                                      ***/
/****************************************************************************/

sInt OpsTotal = 0;

wOp::wOp() : SelectedHandles(0)
{
  OpsTotal++;
  EditStringCount = 0;
  EditString = 0;
  EditData = 0;
  HelperData = 0;
  Cache = 0;
  BuilderNodeCallId = 0;
  BuilderNodeCallerId = 0;
  WeakCache = 0;
  CalcTemp = 0;
  BuilderNode = 0;
  Strobe = 0;
  ArrayGroupMode = 0;
  CacheLRU = 0;
  BlockedChange = 0;
  ConversionOrExtractionUsed = 0;

  ScriptShow = 0;
  ScriptSourceOffset = 0;
  ScriptSourceValid = 0;
  Script = 0;
  
  Name = L"";
  Class = 0;
  Page = 0;
  ConnectError = 0;
  ConnectErrorString = L"not checked";
  CalcErrorString = 0;
  CycleCheck = 0;
  ConnectionMask = 0;
  SlowSkipFlag = 0;

  Select = 0;
  ImportantOp = 0;
  BlinkTimer = 0;
  HighlightArrayLine = -1;
  Bypass = 0;

  FeedbackObject = 0;
  GCParent = 0;
  GCObj = 0;
  RefParent = 0;
  RefObj = 0;

}


void wOp::Init(wClass *cl)
{
  sVERIFY(cl);

  wClassInputInfo *info;
  Class = cl;

  // during loading, default ops get initialized twice!

  if(EditString)
    for(sInt i=0;i<EditStringCount;i++)
      delete EditString[i];
  delete[] EditString;
  delete[] (sU32 *)EditData;   EditData = 0;
  delete[] (sU32 *)HelperData;  HelperData = 0;
  EditStringCount = 0;
  EditString = 0;
  Links.Clear();
  sDeleteAll(ArrayData);
  Conversions.Clear();
  Extractions.Clear();
  delete Script;
  Temp = 0;

  // now initialize

  if(Class->ParaWords>0)
  {
    EditData = new sU32[Class->ParaWords];
    sSetMem(EditData,0,Class->ParaWords*sizeof(sU32));
  }

  if(Class->HelperWords>0)
  {
    HelperData = new sU32[Class->HelperWords];
    sSetMem(HelperData,0,Class->HelperWords*sizeof(sU32));
  }

  if(Class->ParaStrings>0)
  {
    EditStringCount = Class->ParaStrings;
    EditString = new sTextBuffer *[Class->ParaStrings];
    for(sInt i=0;i<EditStringCount;i++)
      EditString[i] = new sTextBuffer;
  }

  sFORALL(Class->Inputs,info)
  {
    wOpInputInfo *link = Links.AddMany(1);
    link->Link = 0;
    link->LinkName = L"";
    link->Select = 0;
    switch(info->Flags & wCIF_METHODMASK)
    {
      case wCIF_METHODLINK:   link->Select = 1; break;
      case wCIF_METHODBOTH:   link->Select = 0; break;
      case wCIF_METHODCHOOSE: link->Select = 2; break;
      case wCIF_METHODANIM:   link->Select = 1; break;
    }
    link->Default = 0;
    link->DefaultUsed = 0;
    if(info->DefaultClass)
    {
      link->Default = new wOp;
      link->Default->Init(info->DefaultClass);
    }
  }

  (*Class->SetDefaults)(this);
	/*
  if((Class->Flags & wCF_LOAD) && Links.GetCount()==1)
    Links[0].LinkName = Doc->LastName;
  if(Class->Flags & wCF_STORE)
    Name = Doc->LastName;
		*/
/*
  if(Class->ArrayCount)
  {
    for(sInt i=0;i<2;i++)
    {
      sU32 *data = new sU32[Class->ArrayCount];
      sSetMem(data,0,Class->ArrayCount*4);
      ArrayData.AddTail(data);
    }
  }
  */
}


wOp::~wOp()
{
  OpsTotal--;
  for(sInt i=0;i<EditStringCount;i++)
    delete EditString[i];
  delete[] EditString;
  delete[] (sU32 *)EditData;
  delete[] (sU32 *)HelperData;
  sDeleteAll(ArrayData);
  delete Script;

  RefParent->Release();
  RefObj->Release();
  sRelease(Cache);
  sRelease(WeakCache);
}

void wOp::Finalize()
{
  sRelease(Cache);
  sRelease(WeakCache);
}

void wOp::Tag()
{
  wOpInputInfo *info;
  sNeed(Inputs);
  sNeed(Outputs);
  sNeed(WeakOutputs);
  sNeed(OldInputs);
  sNeed(Conversions);
  sNeed(Extractions);
  sFORALL(Links,info)
  {
    info->Link->Need();
    info->Default->Need();
  }
  Class->Need();
  GCParent->Need();
  GCObj->Need();
  FeedbackObject->Need();
  Page->Need();
}

void wOp::CopyFrom(wOp *src)
{
  sVERIFY(Class == 0);

  Init(src->Class);
  Name = src->Name;
  Bypass = src->Bypass;

  sCopyMem(EditData,src->EditData,sizeof(sU32)*Class->ParaWords);

  for(sInt i=0;i<Class->ParaStrings;i++)
  {
    EditString[i]->Clear();
    EditString[i]->Print(src->EditString[i]->Get());
  }

  for(sInt i=0;i<Class->Inputs.GetCount();i++)
  {
    Links[i].Select = src->Links[i].Select;
    Links[i].LinkName = src->Links[i].LinkName;
    Links[i].Link = 0;
    Links[i].Default = 0;
    if(src->Links[i].Default)
    {
      Links[i].Default = new wOp();
      Links[i].Default->CopyFrom(src->Links[i].Default);
    }
  }

  void *as,*ad;
  sFORALL(src->ArrayData,as)
  {
    ad = new sU32[Class->ArrayCount];
    sCopyMem(ad,as,Class->ArrayCount*4);
    ArrayData.AddTail(ad);
  }

  ScriptShow = src->ScriptShow;
  ScriptSource.Print(src->ScriptSource.Get());
  ScriptParas = src->ScriptParas;
  UpdateScript();
}

wType *wOp::OutputType()
{
  wOp *cur = this;

  // go up from this op while the type isn't determined.
  while(cur->Class->Flags & (wCF_TYPEFROMINPUT|wCF_LOAD))
  {
    if(cur->Class->Flags & wCF_LOAD) // load: continue to link #0
      cur = cur->Links[0].Link;
    else if(cur->Class->Flags & wCF_TYPEFROMINPUT)
      cur = cur->Inputs.GetCount() ? cur->Inputs[0] : 0;

    if(!cur) // dead link or no input
      return 0; // no type found
  }

  return cur->Class->OutputType;
}

wOp *wOp::MakeConversionTo(wType *type,sInt callid)
{
  wClass *cl;
  wOp *convop;
    
  sFORALL(Doc->Conversions,cl)
    if(cl->OutputType->IsType(type) && Class->OutputType->IsType(cl->Inputs[0].Type))
      goto found;

  return 0;
found:
  sFORALL(Conversions,convop)
    if(convop->Class==cl && callid==convop->BuilderNodeCallId)
      return convop;

  convop = new wOp;
  convop->Init(cl);
  convop->BuilderNodeCallId = callid;
  Conversions.AddTail(convop);

  return convop;
}

wOp *wOp::MakeExtractionTo(const sChar *filter)
{
  sInt len;
  wClass *cl;
  wOp *op;
  
  len = sFindFirstChar(filter,'.');
  if(len==-1)
    len = sGetStringLen(filter);

  // find existing conversion

  sFORALL(Extractions,op)
  {
    if(sCmpString(filter,op->EditString[0]->Get())==0)
    {
      if(op->Inputs.GetCount()==0)
        op->Inputs.AddTail(this);
      return op;
    }
  }

  // find required conversion class, create new conversion op
    
  sFORALL(Doc->Extractions,cl)
  {
    wType *outType = OutputType();

    if(outType && outType->IsType(cl->Inputs[0].Type) // conversion can accept me
      && sCmpStringLen(cl->Extract,filter,len)==0)
    {
      op = new wOp;
      op->Init(cl);
      *op->EditString[0] = filter;
      Extractions.AddTail(op);
      if(op->Inputs.GetCount()==0)
        op->Inputs.AddTail(this);
      return op;
    }
  }

  // no conversion available

  return 0;
}


void *wOp::AddArray(sInt pos)
{
  HighlightArrayLine = -1;
  if(pos==-1)
    pos = ArrayData.GetCount();
  if(Class->ArrayCount>0 && pos>=0 && pos<=ArrayData.GetCount())
  {
    sU32 *mem = new sU32[Class->ArrayCount];
    sSetMem(mem,0,Class->ArrayCount*4);
    if(Class->SetDefaultsArray)
      (*Class->SetDefaultsArray)(this,pos,mem);
    ArrayData.AddBefore(mem,pos);
    return mem;
  }
  else
  {
    return 0;
  }
}

void wOp::RemArray(sInt pos)
{
  HighlightArrayLine = -1;
  if(Class->ArrayCount>=0 && pos>=0 && pos<ArrayData.GetCount())
  {
    void *mem = ArrayData[pos];
    ArrayData.RemAtOrder(pos);
    delete[] (sU8*)mem;
  }
}

void wOp::UpdateScript()
{
  ScriptSourceValid = 0;
  ScriptSourceOffset = 0;
  ScriptSourceLine = 0;

  const sChar *source = ScriptSource.Get();

  while(sIsSpace(*source)) source++;

  sInt lastmax = ScriptParas.GetCount();
  ScriptParas.Clear();
  if(*source)
  {
    sScanner Scan;
    Scan.Init();
    Scan.DefaultTokens();
    Scan.Start(ScriptSource.Get());

    ScriptParas.Clear();
    sBool HasGlobal = 0;

    if(Scan.IfName(L"gui"))
    {
      Scan.Match('{');
      while(!Scan.Errors && Scan.Token!='}')
      {
        if(Scan.IfName(L"group"))
        {
          wScriptParaInfo *wi = ScriptParas.AddMany(1);
          sClear(*wi);
          wi->StringVal = L"";
          wi->Type = 0;
          Scan.ScanString(wi->Name);
          Scan.Match(';');
        }
        else
        {
          sInt type = -1;
          sInt extra = 0;
          sInt count = -1;
          sBool global = 0;
          sInt guiflags = 0;

          // keywords

keywords:
          if(Scan.IfName(L"global"))
          {
            global = 1;
            HasGlobal = 1;
            goto keywords;
          }
          if(Scan.IfName(L"nolabel"))
          {
            guiflags |= 1;
            goto keywords;
          }

          // type

          if(Scan.IfName(L"float"))
            type = ScriptTypeFloat;
          if(Scan.IfName(L"float30"))
          {
            type = ScriptTypeFloat;
            count = 3;
          }
          if(Scan.IfName(L"float31"))
          {
            type = ScriptTypeFloat;
            count = 3;
          }
          if(Scan.IfName(L"float4"))
          {
            type = ScriptTypeFloat;
            count = 4;
          }
          else if(Scan.IfName(L"int"))
            type = ScriptTypeInt;
          else if(Scan.IfName(L"string"))
            type = ScriptTypeString;
          else if(Scan.IfName(L"color"))
            type = ScriptTypeColor;
          else if(Scan.IfName(L"fileout"))
          {
            type = ScriptTypeString;
            extra = 1;
          }
          else if(Scan.IfName(L"filein"))
          {
            type = ScriptTypeString;
            extra = 2;
          }
          else if(Scan.IfName(L"flags"))
          {
            type = ScriptTypeInt;
            guiflags |= 2;
          }

          // create type

          if(type!=-1)
          {
            wScriptParaInfo *wi = ScriptParas.AddMany(1);
            wi->Type = type;
            wi->Min = -1024;
            wi->Max = 1024;
            wi->Default = 0;
            wi->Global = global;
            wi->Count = 1;
            wi->Step = (type == ScriptTypeFloat) ? 0.01f : 0.25f;
            wi->RStep = 0.25;
            wi->GuiExtras = extra;
            wi->GuiFlags = guiflags;
            sBool setdefault = 0;
            if(ScriptParas.GetCount()>lastmax)
            {
              setdefault = 1;
              sClear(wi->IntVal);
              wi->StringVal = L"";
            }
            Scan.ScanName(wi->Name);

            // modifications

            if(type==ScriptTypeInt || type==ScriptTypeFloat)
            {
              if(count==-1)
              {
                if(Scan.IfToken('['))
                {
                  sInt n = Scan.ScanInt();
                  if(n<1)
                    Scan.Error(L"dimension of array < 1");
                  else if(n>4)
                    Scan.Error(L"dimension of array > 4");
                  else
                    wi->Count = n;
                  Scan.Match(']');
                }
              }
              else
              {
                wi->Count = count;
              }
              if(Scan.IfToken('('))
              {
                if(wi->GuiFlags & 2)
                {
                  Scan.ScanString(wi->GuiChoices);
                }
                else
                {
                  wi->Min = Scan.ScanFloat();
                  Scan.Match(sTOK_ELLIPSES);
                  wi->Max = Scan.ScanFloat();
  loopintflags:
                  if(Scan.IfName(L"step"))
                  {
                    wi->Step = Scan.ScanFloat();
                    if(Scan.IfToken(','))
                      wi->RStep = Scan.ScanFloat();
                    goto loopintflags;
                  }
                  else if(Scan.IfName(L"logstep"))
                  {
                    wi->Step = -Scan.ScanFloat();
                    goto loopintflags;
                  }
                  else if(Scan.IfName(L"hex"))
                  {
                    wi->GuiExtras = 4;
                    if(Scan.Token==sTOK_INT)
                      wi->GuiExtras = sClamp(Scan.ScanInt(),0,8);
                    goto loopintflags;
                  }
                }
                Scan.Match(')');
              }
              if(wi->GuiChoices.IsEmpty() && (wi->GuiFlags & 2))
                wi->GuiChoices = L"-|on";
              if(Scan.IfToken('='))
              {
                wi->Default = Scan.ScanFloat();
                if(setdefault)
                {
                  wi->FloatVal[0] = wi->FloatVal[1] = wi->FloatVal[2] = wi->FloatVal[3] = wi->Default;
                }
              }
            }
            else if(type==ScriptTypeColor)
            {
              if(Scan.IfToken('('))
              {
                sPoolString rgb;
                Scan.ScanString(rgb);
                if(rgb==L"rgb")
                  wi->GuiExtras = 1;
                else if(rgb==L"rgba")
                  wi->GuiExtras = 0;
                else
                  Scan.Error(L"colors must be 'rgb' or 'rgba");
                Scan.Match(')');
              }
              if(Scan.IfToken('='))
              {
                sU32 def = Scan.ScanInt();
                if(setdefault)
                  wi->ColorVal = def;
              }
            }
            else if(type==ScriptTypeString)
            {
              if(Scan.IfToken('='))
              {
                if(Scan.IfToken(sTOK_STRING))
                {
                  sPoolString str;
                  Scan.ScanString(str);
                  if(setdefault)
                    wi->StringVal = str;
                }
              }
            }
            Scan.Match(';');
          }
          else
          {
            Scan.Error(L"syntax");
          }
        }
      }
      if(Scan.Token=='}')
      {
        ScriptSourceOffset = Scan.Stream->ScanPtr - ScriptSource.Get();
        ScriptSourceLine = Scan.Stream->Line;
      }
      Scan.Match('}');
      if(Scan.Errors)
        CalcErrorString = sPoolString(Scan.ErrorMsg);
    }
    ScriptSourceValid = (Scan.Token!=sTOK_END) || HasGlobal;
  }
}

void wOp::MakeSource(sTextBuffer &tb)
{
  wScriptParaInfo *para;
  tb.Clear();
  if(!ScriptSourceValid)
    return;

  sString<256> opname;
  
  // pretty printed operator name for #line directive

  if(Page)
  {
    if(Page->IsTree)
    {
      opname.PrintF(L"page '%s'",Page->Name);
    }
    else
    {
      wStackOp *sop = (wStackOp *) this;
      opname.PrintF(L"page '%s' pos %d %d",Page->Name,sop->PosX,sop->PosY);
    }
  }
  else
  {
    opname.PrintF(L"unknown");
  }

  // insert parameters

  tb.PrintF(L"#line 1 \"%s, generated\"\n",opname);
  sFORALL(ScriptParas,para)
  {
    if(para->Type!=0)         // type 0 is group label
    {
      if(para->Global)
        tb.Print(L"global ");
      static const sChar *tname[] = { L"???",L"int",L"float",L"string",L"color" };
      tb.PrintF(L"%s : %s",para->Name,tname[para->Type]);
      sVERIFY(para->Count>0);
      if(para->Count>1)
        tb.PrintF(L"[%d] = [ ",para->Count);
      else
        tb.Print(L" = ");
      for(sInt i=0;i<para->Count;i++)
      {
        if(i>0)
          tb.Print(L",");
        switch(para->Type)
        {
        case ScriptTypeInt:
          tb.PrintF(L"%d",para->IntVal[i]);
          break;
        case ScriptTypeFloat:
          tb.PrintF(L"%f",para->FloatVal[i]);
          break;
        case ScriptTypeColor:
          sVERIFY(i==0);
          tb.PrintF(L"[%d/255.0,%d/255.0,%d/255.0,%d/255.0]",(para->ColorVal>>16)&255,(para->ColorVal>>8)&255,(para->ColorVal>>0)&255,(para->ColorVal>>24)&255);
          break;
        case ScriptTypeString:
          sVERIFY(i==0);
          tb.PrintF(L"%q",para->StringVal);
          break;
        default:
          sVERIFYFALSE;
        }
      }
      if(para->Count>1)
        tb.Print(L" ]");
      tb.Print(L";\n");
    }
  }

  // insert imports

  if(Class->Bind3Para)
    (*Class->Bind3Para)(this,tb);

  // add code

  tb.PrintF(L"#line %d \"%s\"\n",ScriptSourceLine,opname);
  tb.Print(ScriptSource.Get()+ScriptSourceOffset);
}

ScriptContext *wOp::GetScript()
{
  if(!Script)
  {
    Script = new ScriptContext;

    UpdateScript();
    sTextBuffer tb;
    MakeSource(tb);
    Script->Compile(tb.Get());
  }
  return Script;
}


wOp *wOp::FirstInputOrLink() 
{
  if(Class->Flags & (wCF_CALL))
  {
    if(Links.IsIndexValid(0) && Links[0].Link) 
      return Links[0].Link; 
    if(Inputs.IsIndexValid(0) && Inputs[0])
      return Inputs[0]; 
  }
  else
  {
    if(Inputs.IsIndexValid(0) && Inputs[0])
      return Inputs[0]; 
    if(Links.IsIndexValid(0) && Links[0].Link) 
      return Links[0].Link; 
  }
  return 0;
}

sBool wOp::CheckShellSwitch()
{
  if(!(Class->Flags & wCF_SHELLSWITCH))   // not a shell switch, always good
    return 1;
  
  sInt f = EditU()[0];

  sInt bit = Doc->ShellSwitches & (1<<(f&31)) ? 1 : 0;
  if(f&0x100)                             // negative flag check
    bit = !bit;
  return bit;
}

/****************************************************************************/

template <class streamer> void wOp::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::Werkkzeug4Op,12);
  sInt dummy=0;

  if(version)
  {
    wDocName classname,typenam,opname;
    if(s.IsReading())
    {
      s | opname;
      s | classname;
      s | typenam;
      wClass *cl = Doc->FindClass(classname,typenam);
      if(!cl)
      {
        sDPrintF(L"try to load unknown classname <%s> type <%s>\n",classname,typenam);
        cl = Doc->FindClass(L"UnknownOp",L"AnyType");
        Doc->UnknownOps++;
        sVERIFY(cl);
      }
      Init(cl);
      Name = opname;
    }
    else
    {
      classname = Class->Name;
      typenam = Class->OutputType->Symbol;
      s | Name;
      s | classname;
      s | typenam;
    }

    if(version>=12)
      s | dummy;

    sInt words = Class->ParaWords;
    s | words;

    s.ArrayU32(EditU(),sMin(words,Class->ParaWords));
    if(words>Class->ParaWords)
      s.Skip((words-Class->ParaWords)*4);

    sInt strings = Class->ParaStrings;
    s | strings;
    if(s.IsWriting())
    {
      sVERIFY(strings<=Class->ParaStrings);
      for(sInt i=0;i<strings;i++)
        s | EditString[i];
    }
    else
    {
      sInt iend = sMin(Class->ParaStrings,strings);
      for(sInt i=0;i<iend;i++)
        s | EditString[i];
      // skip unused string
      sTextBuffer discard;
      for(sInt i=iend;i<strings;i++)
        s | &discard;
    }

    if(version>=3)
    {
      sInt count = Links.GetCount();
      s | count;
      for(sInt i=0;i<count;i++)
      {
        wOpInputInfo in;
        if(s.IsReading())
        {
          s | in.LinkName | in.Select;
          if(i<Links.GetCount())      // ist das nicht immer TRUE?
          {
            Links[i].LinkName = in.LinkName;
            sInt method = Class->Inputs[i].Flags & wCIF_METHODMASK;
            if(method==wCIF_METHODBOTH || method==wCIF_METHODCHOOSE)
              Links[i].Select = in.Select;
          }
          if(version>=4)
          {
            sInt test=0;
            s | test;
            if(test)
            {
              if(i<Links.GetCount() && Links[i].Default)        // normal case
              {
                s | Links[i].Default;
              }
              else                        // unknown ops
              {
                wOp *def = new wOp;
                s | def;
              }
            }
          }
        }
        else
        {
          s | Links[i].LinkName | Links[i].Select;
          if(version>=4)
          {
            sInt test;
            if(Links[i].Default)
            {
              test = 1;
              s | test | Links[i].Default;
            }
            else
            {
              test = 0;
              s | test;
            }
          }
        }
      }
    }
    if(version>=5)
    {
      if(s.IsReading())                               // delete default data before reading
        sDeleteAll(ArrayData);

      sInt words = Class->ArrayCount;                 // words per array entry
      s | words;
      sInt skips = words - Class->ArrayCount;
      words = sMin(words,Class->ArrayCount);

      s.Array(ArrayData);                             // number of array entries

      if(s.IsReading())                               // when reading, allocate and clear the array entries
      {
        for(sInt i=0;i<ArrayData.GetCount();i++)
        {
          ArrayData[i] = new sU32[Class->ArrayCount];
          sSetMem(ArrayData[i],0,Class->ArrayCount*sizeof(sU32));
        }
      }

      for(sInt i=0;i<ArrayData.GetCount();i++)        // entry data
      {
        s.ArrayU32((sU32 *)ArrayData[i],words);
        if(s.IsReading() && skips>0)
          s.Skip(skips*4);
      }
    }

    if(version==6) 
    {
      sInt animcount=0;
      sU32 id=0;
      s | animcount;
      if(animcount)
      {
        if(s.IsReading())
        {
          for(sInt i=0;i<animcount;i++)
          {
            s | id;
            sVERIFY(id==0);
          }
        }
      }
    }
    if(version>=8)
    {
      s | ScriptShow;
      ScriptSource.Serialize(s);
      UpdateScript();
    }
    if(version>=9)
    {
      wScriptParaInfo *sv;
      if(s.IsReading())
        ScriptParas.Clear();
      s.Array(ScriptParas);
      sFORALL(ScriptParas,sv)
        for(sInt i=0;i<4;i++)
          s | sv->IntVal[i];
    }
    if(version>=10)
    {
      wScriptParaInfo *sv;
      sFORALL(ScriptParas,sv)
        s | sv->StringVal;
    }
  }

  if(version>=11)
    s.Footer();
}
void wOp::Serialize(sWriter &stream) { Serialize_(stream); }
void wOp::Serialize(sReader &stream) { Serialize_(stream); }

/****************************************************************************/
/****************************************************************************/

wTreeOp::wTreeOp()
{
  Select = 0;
}

void wTreeOp::Tag()
{
  wOp::Tag();
  TreeInfo.Need();
}

template <class streamer> void wTreeOp::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::wTreeOp,1);
  if(version)
  {
    wOp::Serialize(s);
    s | TreeInfo.Level;
    s | TreeInfo.Flags;
    s.Footer();
  }
}

void wTreeOp::Serialize(sWriter &s) { Serialize_(s); }
void wTreeOp::Serialize(sReader &s) { Serialize_(s); }

/****************************************************************************/

wStackOp::wStackOp()
{
  PosX = 0;
  PosY = 0;
  SizeX = 3;
  SizeY = 1;
  Select = 0;
  Page = 0;
  Hide = 0;
}

void wStackOp::Tag()
{
  wOp::Tag();
  Page->Need();
}

void wStackOp::CopyFrom(wStackOp *src)
{
  wOp::CopyFrom(src);
  Hide = src->Hide;
}

template <class streamer> void wStackOp::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::wStackOp,1);
  if(version)
  {
    wOp::Serialize(s);
    s | PosX | PosY | SizeX | SizeY | Bypass | Hide;
    s.Footer();
  }
}

void wStackOp::Serialize(sWriter &s) { Serialize_(s); }
void wStackOp::Serialize(sReader &s) { Serialize_(s); }

/****************************************************************************/

wOpData::wOpData()
{
  Valid = 0;
  ElementBytes = 0;
}
wOpData::~wOpData()
{
  Clear();
}

void wOpData::CopyFrom(wOp *op)
{
  Clear();
  if(op)
  {
    Data.Resize(op->Class->ParaWords);
    Strings.Resize(op->Class->ParaStrings);
    sInt links = op->Links.GetCount();
    LinkNames.Resize(links);
    LinkFlags.Resize(links);
    ElementBytes = op->Class->ArrayCount*4;
    ArrayData.Resize(op->ArrayData.GetCount());

    for(sInt i=0;i<op->Class->ParaWords;i++)
      Data[i] = op->EditU()[i];

    for(sInt i=0;i<op->Class->ParaStrings;i++)
      if(op->EditString[i])
        Strings[i] = op->EditString[i]->Get();

    for(sInt i=0;i<links;i++)
    {
      LinkNames[i] = op->Links[i].LinkName;
      LinkFlags[i] = op->Links[i].Select;
    }

    for(sInt i=0;i<op->ArrayData.GetCount();i++)
    {
      ArrayData[i] = new sU8[ElementBytes];
      sCopyMem(ArrayData[i],op->ArrayData[i],ElementBytes);
    }
  }

  Valid = op;
}

void wOpData::Clear()
{
  Valid = 0;
  sDeleteAll(ArrayData);
  LinkNames.Clear();
  LinkFlags.Clear();
  Strings.Clear();
  Data.Clear();
}

void wOpData::CopyTo(wOp *op)
{
  if(IsValid(op))
  {
    sCopyMem(op->EditData,Data.GetData(),sizeof(sU32)*op->Class->ParaWords);
    for(sInt i=0;i<Strings.GetCount();i++)
    {
      sVERIFY(op->EditString[i]);
      op->EditString[i]->Clear();
      op->EditString[i]->Print(Strings[i]);
    }
    for(sInt i=0;i<LinkNames.GetCount();i++)
    {
      op->Links[i].LinkName = LinkNames[i];
      op->Links[i].Select = LinkFlags[i];
    }
    sDeleteAll(op->ArrayData);
    for(sInt i=0;i<ArrayData.GetCount();i++)
    {
      op->AddArray(op->GetArrayCount());
      sCopyMem(op->ArrayData[i],ArrayData[i],ElementBytes);
    }
  }
}

sBool wOpData::IsSame(wOp *op)
{
  if(!IsValid(op)) return 0;
  if(sCmpMem(Data.GetData(),op->EditData,sizeof(sU32)*op->Class->ParaWords)!=0) return 0;
  for(sInt i=0;i<op->Class->ParaStrings;i++)
  {
    if(Strings[i].IsEmpty())
    {
      if(op->EditString[i]!=0)
        if(op->EditString[i]->GetCount()!=0)
          return 0;
    }
    else
    {
      if(op->EditString[i]==0) return 0;
      if(sCmpString(op->EditString[i]->Get(),Strings[i])!=0) return 0;
    }
  }
  for(sInt i=0;i<LinkNames.GetCount();i++)
  {
    if(LinkNames[i]!=op->Links[i].LinkName) return 0;
    if(LinkFlags[i]!=op->Links[i].Select) return 0;
  }
  if(op->ArrayData.GetCount()!=ArrayData.GetCount()) return 0;
  if(op->Class->ArrayCount*4!=ElementBytes) return 0;
  for(sInt i=0;i<op->ArrayData.GetCount();i++)
  {
    if(sCmpMem(op->ArrayData[i],ArrayData[i],ElementBytes)!=0) return 0;
  }

  return 1;
}

sBool wOpData::IsValid(wOp *op)
{
  if(!Valid) return 0;
  if(Valid!=op) return 0;
  if(Data.GetCount()!=op->Class->ParaWords) return 0;
  if(Strings.GetCount()!=op->Class->ParaStrings) return 0;
  if(LinkNames.GetCount()!=op->Links.GetCount()) return 0;
  if(ElementBytes != op->Class->ArrayCount*4) return 0;
  return 1;
}

/****************************************************************************/
/*
wImportOp::wImportOp()
{
}

wImportOp::~wImportOp()
{
}

void wImportOp::Tag()
{
  wOp::Tag();
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   options                                                            ***/
/***                                                                      ***/
/****************************************************************************/

void wEditOptions::Init()
{
  File = L"";
  Screen = 0;
  BackColor = 0xff303030;
  GridColor = 0xffffffff;
  AmbientColor = 0xffffffff;
  SplineMode = 0;
  Flags = 0;
  Volume = 100;
  ClipNear = 0.125f;
  ClipFar = 16384;
  MoveSpeed = 1;
  AutosavePeriod = 90;
  MultiSample = 0;
  ZoomFont = 1;
  MemLimit = 0;
  ExpensiveIPPQuality = 2; // default to high
  Theme = TH_DEFAULT;
  CustomTheme = sGuiThemeDefault;
  DefaultCamSpeed = 0;
}

template <class streamer> void wEditOptions::Serialize_(streamer &s)
{
  sPoolString dummy;
  sInt version = s.Header(sSerId::Wz4EditOptions,18);
  sInt dummyi = 32;

  if(version)
  {
    s | File | dummy | Screen | BackColor;
    if(version>=2)  s | GridColor;
    if(version>=3)  s | SplineMode;
    if(version>=4)  s | Flags;
    if(version>=5)  s | AmbientColor;
    if(version>=6)  s | Volume;
    if(version>=7)  s | ClipNear | ClipFar | MoveSpeed;
    if(version>=8)  s | AutosavePeriod;
    if(version>=9)  s | MultiSample;
    if(version>=10 && version<11) s | dummyi;
    if(version>=12) s | ZoomFont;
    if(version>=13) s | MemLimit;
    if(version>=14) s | ExpensiveIPPQuality;
    if(version>=15) s | Theme | &CustomTheme;
    if(version>=16) s | DefaultCamSpeed;
    if(version==17) s | dummy;
    s.Footer();
  }
}

void wEditOptions::Serialize(sWriter &stream) { Serialize_(stream); }
void wEditOptions::Serialize(sReader &stream) { Serialize_(stream); }

void wEditOptions::ApplyTheme()
{
  const sGuiTheme *gt=0;
  switch (Theme)
  {
  case TH_DEFAULT: gt=&sGuiThemeDefault; break;
  case TH_DARKER: gt=&sGuiThemeDarker; break;
  case TH_CUSTOM: gt=&CustomTheme; break;
  }
  sVERIFY(gt);
  sGui->SetTheme(*gt);
}


/****************************************************************************/

void wDocOptions::Init()
{
  ProjectPath = L"";
  ProjectName = L"";
  ProjectId = 0;
  SiteId = 0;
  TextureQuality = 1;
  LevelOfDetail = 2;
  BeatsPerSecond = 0x10000;
  BeatStart = 0;
  Beats = 32;
  ScreenX = 800;
  ScreenY = 600;
  SampleRate = 44100;
  DialogFlags = wDODF_Benchmark;
  Infinite = 0;

  VariableBpm = 0;
  BpmSegments.Clear();
  HiddenParts.Clear();

  UpdateTiming();
}

void wDocOptions::UpdateTiming()
{
  BpmSegment *s;
  sFORALL(BpmSegments,s)
  {
    s->Bps = sInt(s->Bpm/60*0x10000);
    s->Milliseconds = sMulDiv(s->Beats*0x10000,1000,s->Bps);
    s->Samples = sMulDiv(s->Beats*0x10000,SampleRate,s->Bps);
  }
}

template <class streamer> void wDocOptions::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::Wz4DocOptions,16);
  sVERIFY(sCOUNTOF(sColorPickerWindow::PaletteColors)==32);
  if(version)
  {
    s | ProjectPath;
    if(version>=2)  s | ProjectName;
    if(version>=11) s | ProjectId | SiteId;
    if(version>=3)  s | TextureQuality;
    if(version>=4)  s | LevelOfDetail;
    if(version>=5)  s | BeatsPerSecond | MusicFile;
    if(version>=10) s | BeatStart;
    if(version>=9)  s | Beats;
    if(version>=6)  s | ScreenX | ScreenY;
    if(version>=7)  s | PageName;
    if(version>=8)  s | Packfiles;
    if(version>=12)
    {
      for(sInt i=0;i<32;i++)
      {
        s | sColorPickerWindow::PaletteColors[i][0];
        s | sColorPickerWindow::PaletteColors[i][1];
        s | sColorPickerWindow::PaletteColors[i][2];
        s | sColorPickerWindow::PaletteColors[i][3];
      }
    }

    if(version>=13)
    {
      if(s.IsReading())
        BpmSegments.Clear();
      s | SampleRate | VariableBpm;
      s.Array(BpmSegments);
      BpmSegment *e;
      sFORALL(BpmSegments,e)
        s | e->Beats | e->Bpm;
    }
    if(version>=14)
      s | DialogFlags;

    if(version>=15)
    {
      if(s.IsReading())
        HiddenParts.Clear();
      s.Array(HiddenParts);
      HiddenPart *hp;
      sFORALL(HiddenParts,hp)
      {
        s | hp->Code | hp->Store | hp->Bpm | hp->LastBeat | hp->Flags;
        if(version>=16)
          s | hp->Song;
      }
    }

    s.Footer();
  }

  UpdateTiming();
}

void wDocOptions::Serialize(sWriter &stream) { Serialize_(stream); }
void wDocOptions::Serialize(sReader &stream) { Serialize_(stream); }

/****************************************************************************/

void wTestOptions::Init()
{
  Mode = wTOM_Compare;
  Compare = wTOP_DirectX11;
  FailRed = 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   document                                                           ***/
/***                                                                      ***/
/****************************************************************************/

void RegisterWZ4Classes();

wDocument::wDocument()
{
  wClass *cl;

  Doc = this;
  ViewLog = 0;
  EditOptions.Init();
  DocOptions.Init();
  TestOptions.Init();
  PostLoadAction = 0;
  FinalizeAction = 0;
  CacheLRU = 1;
  IsPlayer = 0;
  CurrentPage = 0;
  LowQuality = 0;
  IsCacheWarmup = 0;
  BlockedChanges = 0;

  ShellSwitches = 0;
  for(sInt i=0;i<wSWITCHES;i++)
    ShellSwitchOptions[i].PrintF(L"%d",i);

  sSetRunlevel(0x120);
  RegisterWZ4Classes();

  sSortUp(Types,&wType::Order);

  sFORALL(Classes,cl)
  {
    if(cl->Flags & wCF_CONVERSION)
    {
      sVERIFY(cl->Inputs.GetCount()==1);
      sVERIFY(cl->Inputs[0].Type);
      Conversions.AddTail(cl);
    }
    if(!cl->Extract.IsEmpty())
    {
      sVERIFY(cl->Inputs.GetCount()==1);
      sVERIFY(cl->Inputs[0].Type);
      sVERIFY(cl->ParaStrings==1);
      Extractions.AddTail(cl);
    }
  }

  Exe = new wExecutive;
  Builder = new wBuilder;

  sSortUp(Classes,&wClass::Label);

  DefaultDoc();

  Rnd.Seed(sGetRandomSeed());
}

wDocument::~wDocument()
{
  delete Exe;
  delete Builder;
}

void wDocument::Finalize()
{
  if(FinalizeAction)
    (*FinalizeAction)();
}

void wDocument::Tag()
{
  sNeed(Pages);
  sNeed(Classes);
  sNeed(Types);
  sNeed(Conversions);  
  sNeed(Extractions);  
  sNeed(DirtyWeakOps);
  sNeed(AllOps);
  sNeed(Includes);

  wHandleSelectTag *tag;
  sFORALL(Doc->SelectedHandleTags,tag)
    tag->Op->Need();
  CurrentPage->Need();
}

void wDocument::New()
{
  Pages.Clear();
  Connect();
  Filename = L"";
  DocOptions.Init();
  DocChanged = sFALSE;
}

sBool wDocument::Load(const sChar *filename)
{
  wDocInclude *inc;

  sBool ok = sLoadObject(filename,this);
  if(ok)
    Filename = filename;
  sFORALL(Includes,inc)
    if(ok)
      if(!inc->Load())
        ok = 0;

  // sort pages for includes

  sPoolString *str;
  sArray<wPage *> oldpages;
  oldpages = Pages;
  Pages.Clear();
  sFORALL(PageNames,str)
  {
    wPage *page = sFind(oldpages,&wPage::Name,*str);
    if(page)
    {
      oldpages.Rem(page);
      Pages.AddTail(page);
    }
  }
  Pages.Add(oldpages);

  // connect

  Connect();
  Doc->UnblockChange();

  return ok;
}

sBool wDocument::Save(const sChar *filename)
{
  wDocInclude *inc;

  sBool ok = sSaveObject(filename,this);
  sFORALL(Includes,inc)
    if(ok)
      if(!inc->Save())
        ok = 0;

  if(ok)
    Filename = filename;
  return ok;
}

void wDocument::DefaultDoc()
{
  wPage *page;
  Pages.AddTail(page = new wPage); page->DefaultName();
  Connect();
}

void wDocument::RemoveType(wType *type)
{
  sRemEqual(Classes,&wClass::OutputType,type);
  Types.RemOrder(type);
}

void wDocument::ConnectError(wOp *op,const sChar *text)
{
  if(!op->ConnectErrorString)
    op->ConnectErrorString = text;
}

static sBool isname(const sChar *s)
{
  for(;;)
  {
    if(*s!='-' && !sIsLetter(*s++)) return 0;
    while(*s=='-' || sIsLetter(*s) || sIsDigit(*s)) s++;
    if(*s==0) return 1;
    if(*s!=':' && *s!='.') return 0;
    s++;
  }
}

static void RootConnectR(wOp *op)
{
  if(op && !op->ConnectedToRoot)
  {
    wOp *c;
    wOpInputInfo *l;
    op->ConnectedToRoot = 1;
    sFORALL(op->Inputs,c)
      RootConnectR(c);
    sFORALL(op->Links,l)
      RootConnectR(l->Link);
  }
}

void wDocument::Connect()
{
  wPage *page;
  wOp *op,*in;
  wOpInputInfo *link;

//  if(!ReconnectFlag)
//    return;

  AllOps.Clear();
  Stores.Clear();

  // make list of ALL ops

  sFORALL(Pages,page)
  {
    sFORALL(page->Ops,op)
      op->Page = page;
    sFORALL(page->Tree,op)
      op->Page = page;
    if(page->IsTree)
    {
      AllOps.Add((sArray<wOp *>&)page->Tree);
      sVERIFY(page->Ops.IsEmpty());
    }
    else
    {
      AllOps.Add((sArray<wOp *>&)page->Ops);
      sVERIFY(page->Tree.IsEmpty());
    }
  }

  sInt max = AllOps.GetCount();
  for(sInt i=0;i<max;i++)
  {
    op = AllOps[i];
    sFORALL(op->Links,link)
    {
      link->DefaultUsed = 0;
      if(link->Default)
        AllOps.AddTail(link->Default);
    }
    AllOps.Add(op->Conversions);
    AllOps.Add(op->Extractions);
  }

  // clear all ops, find stores

  sFORALL(AllOps,op)
  {
    op->BuilderNode = 0;
    op->ConnectErrorString = 0;
    op->ConnectError = 0;
    op->CalcErrorString = 0;
    op->CycleCheck = 0;
    op->CheckedByBuild = 0;
    op->ConnectedToRoot = 0;
    op->OldInputs.Add(op->Inputs);
    op->Inputs.Clear();
    op->Outputs.Clear();
    op->WeakOutputs.Clear();
    if(op->Name[0] && op->Name[0]!=';' && !(op->Class->Flags & wCF_COMMENT))
    {
      if(isname(op->Name))
      {
        Stores.AddTail(op);
      }
      else
      {
        ConnectError(op,L"store name must be c-style name. '-' is allowed as a character. names starting with ';' are ignored");
        op->ConnectError = 1;
      }
    }
  }

  // sort stores, find doubles

  sHeapSortUp(Stores,&wOp::Name);
  for(sInt i=0;i<Stores.GetCount()-1;i++)
  {
    if(sCmpString(Stores[i]->Name,Stores[i+1]->Name)==0)
    {
      ConnectError(Stores[i],L"store name used twice");
      Stores[i]->ConnectError = 1;
      ConnectError(Stores[i+1],L"store name used twice");
      Stores[i+1]->ConnectError = 1;
    }
  }

  // reconnect 

  sFORALL(Pages,page)
  {
    if(page->IsTree)
      ConnectTree(page->Tree);
    else
      ConnectStack(page);
  }

  // check connection & links

  sFORALL(AllOps,op)
  {

    // find links

    sFORALL(op->Links,link)
    {
      link->Link = 0;
      if(link->LinkName[0] && link->Select==1)
      {
        link->Link = FindStore(link->LinkName);

        if(!link->Link)
        {
          ConnectError(op,L"link/load name not found");
          op->ConnectError = 1;
        }
      }
    }
  }

  // link all outputs.
  // this must be done before realizing connection changes

  sFORALL(AllOps,op)
  {
    sFORALL(op->Inputs,in)
      if(in)
        in->Outputs.AddTail(op);
    sFORALL(op->Links,link)
      if(link->Link)
        link->Link->Outputs.AddTail(op);
  }

  // do checking using the builder. this also identifies weak outputs

  sFORALL(AllOps,op)
  {
    if(op->Outputs.GetCount()==0 && !op->CheckedByBuild)
      Builder->Check(op);
  }

  // finalize

  sFORALL(AllOps,op)
    op->Temp = 1;


  sFORALL(AllOps,op)
  {
    // check if connection has changed

    sBool differ = 0;
    if(op->Inputs.GetCount()!=op->OldInputs.GetCount())
    {
      differ = 1;
    }
    else
    {
      for(sInt i=0;i<op->Inputs.GetCount();i++)
        if(op->Inputs[i]!=op->OldInputs[i])
          differ = 1;
    }
    if(differ)
      ChangeR(op,0,0,0);

    // clear temporary arrays.

    op->OldInputs.Clear();
  }

  // notify

  if(sGui)
    sGui->Notify(*this);

  // who is connected to root?

  op = FindStore(L"root");
  RootConnectR(op);

  // stats

  if(0)
  {
    sInt n = 0;
    sFORALL(AllOps,op)
      n += op->Conversions.GetCount();

    sDPrintF(L"conversions %d - ",n);
    sDPrintF(L"ops total %d\n",OpsTotal);
  }
}

void wDocument::ConnectTree(sArray<wTreeOp *> &tree)
{
  wTreeOp *obj;
  wTreeOp *parents[sLW_MAXTREENEST];
  wTreeOp *last;
  sInt parentlevel;

  sClear(parents);

//  sInt max = Tree.GetCount();

  last = 0;
  parentlevel = 0;
  parents[0] = last;
    
  sFORALL(tree,obj)
  {
    sInt level = sClamp(obj->TreeInfo.Level,0,sLW_MAXTREENEST-1);

    if(level > parentlevel)
    {
      while(parentlevel <= level)
      {
        parentlevel++;
        parents[parentlevel] = last;
      }
    }
    if(parents[level] && obj->CheckShellSwitch())
    {
      parents[level]->Inputs.AddTail(obj);
    }
    parentlevel = level;
    last = obj;
  }
}

void wDocument::ConnectStack(wPage *page)
{
  wStackOp *op0,*op1;

  // copy ops and remove comments and disabled shellswitch operators

  sArray<wStackOp *> ops = page->Ops;
  sInt max = ops.GetCount();
  for(sInt i=0;i<max;)
  {
    if(ops[i]->Class->Flags & wCF_COMMENT)
      ops[i] = ops[--max];
    else if(!ops[i]->CheckShellSwitch())
      ops[i] = ops[--max];
    else
      i++;
  }
  ops.Resize(max);

  sSortUp(ops,&wStackOp::PosY);

  // connection

  max = ops.GetCount();
  sFORALL(ops,op0)
  {
    sInt j = _i+1;
    while(j<max && ops[j]->PosY <  op0->PosY+op0->SizeY) j++;
    while(j<max && ops[j]->PosY == op0->PosY+op0->SizeY)
    {
      op1 = ops[j];

      if(op0->PosX < op1->PosX+op1->SizeX && op1->PosX < op0->PosX+op0->SizeX)
      {
        op1->Inputs.AddTail(op0);
      }

      j++;
    }
  }

  // hide

  sFORALL(ops,op0)
  {
    sInt max = op0->Inputs.GetCount();
    for(sInt i=0;i<max;)
    {
      op1 = (wStackOp *) op0->Inputs[i];
      if(op1->Hide)
      {
        op0->Inputs.RemAt(i);
        max--;
      }
      else
      {
        i++;
      }
    }
  }

  // sort left<->right

  sFORALL(ops,op0)
  {
    sInt max = op0->Inputs.GetCount();
    for(sInt i=0;i<max-1;i++)
    {
      for(sInt j=i+1;j<max;j++)
      {
        wStackOp *a = (wStackOp *)op0->Inputs[i];
        wStackOp *b = (wStackOp *)op0->Inputs[j];
        if(a->PosX > b->PosX)
          op0->Inputs.Swap(i,j);
      }
    }
  }

  // bypass

  sFORALL(ops,op0)
  {
    sInt max = op0->Inputs.GetCount();
    for(sInt i=0;i<max;i++)
    {
      op1 = (wStackOp *) op0->Inputs[i];
      if(op1->Bypass)
      {
        if(op1->Inputs.GetCount()>=1)
        {
          op0->Inputs[i] = op1->Inputs[0];
        }
        else if(op1->Inputs.GetCount()==0)
        {
          op0->Inputs.RemAtOrder(i);
          i--;
          max--;
        }
        else
        {
          op1->Bypass = 0;
        }
      }
    }
  }

}

void wDocument::ChangeDefaults(wOp *op)
{
  wOpInputInfo *link;
  sFORALL(op->Links,link)
  {
    if(link->Default && link->Default->Cache)
      sRelease(link->Default->Cache);
  }
}

void wDocument::Change(wOp *op,sBool ignoreweak,sBool dontnotify)
{
  wOp *clr;
  sFORALL(AllOps,clr)
    clr->Temp = 1;
  ChangeR(op,ignoreweak,dontnotify,0);
}

void wDocument::ChangeR(wOp *op,sBool ignoreweak,sBool dontnotify,wOp *from)
{
  if(op->Temp)
  {
    if(from && op->Class->Flags & wCF_BLOCKCHANGE)
    {
      op->BlockedChange = 1;
      BlockedChanges = 1;
      return;
    }
    if(sGui && !dontnotify)
      sGui->Notify(*op);
    op->CycleCheck++;

    op->Temp = 0;
    wOp *out;
    sFORALL(op->Outputs,out)
    {
      if(out->CycleCheck==0)
      {
        if(!ignoreweak && sFindPtr(op->WeakOutputs,out))    // weak linked ops do not propagate changes, but are entered into a list so they can be calculated when needed.
          DirtyWeakOps.AddTail(op);
        else
          ChangeR(out,0,sTRUE,op);
      }
    }
    op->Conversions.Clear();
    op->Extractions.Clear();
    sDelete(op->Script);
    sRelease(op->Cache);

 //   if(!((op->Class->Flags & wCF_CALL) && (op->Links.GetCount()>=1) && op->Links[0].Link==from))
      op->BuilderNodeCallerId = 0;
 
    op->CycleCheck--;
  }
}

void wDocument::FlushCaches()
{
  wOp *op;
  Connect();

  sFORALL(AllOps,op)
  {
    op->Conversions.Clear();
    op->Extractions.Clear();

    op->GCObj = 0;
    op->GCParent = 0;
    sRelease(op->RefParent);
    sRelease(op->RefObj);
    sRelease(op->Cache);
    sRelease(op->WeakCache);

    op->BuilderNodeCallerId = 0;
  }

  Connect();
}

void wDocument::ChargeCaches()
{
  wOp *rootop = FindStore(L"root");

  if(rootop)
  {
    wObject *root = CalcOp(rootop);
    
    if(root)
    {
      wPaintInfo pi;
      sViewport view;
      sRender3DBegin();
      sTexture2D *tex = new sTexture2D(256,256,sTEX_2D|sTEX_ARGB8888|sTEX_NOMIPMAPS|sTEX_RENDERTARGET);
      sEnlargeRTDepthBuffer(256,256);
      sTargetSpec spec(tex,sGetRTDepthBuffer());
      sSetTarget(sTargetPara(sST_CLEARALL,0,spec));

      for(sInt i=0;i<DocOptions.Beats;i+=16)
      {
        ProgressPaintFunc(i,DocOptions.Beats);
        pi.TimeBeat = i<<16;
        pi.TimeMS = sMulDiv(i<<16,1000,DocOptions.BeatsPerSecond);

        view.SetTargetCurrent();
        view.Prepare();
        pi.View=&view;

        root->Type->BeforeShow(root,pi);
        LastView = *pi.View;
        pi.SetCam = 0;
        Show(root,pi);
      }
      delete tex;
      sRender3DEnd(0);
    }

    root->Release();
  }
}

void wDocument::UnblockChange()
{
  wOp *op;
  sFORALL(AllOps,op)
  {
    if(op->BlockedChange)
    {
      op->BlockedChange = 0;
      Change(op);
    }
  }
  BlockedChanges = 0;
}

wType *wDocument::FindType(const sChar *name)
{
  wType *type;
  sFORALL(Types,type)
    if(sCmpString(type->Symbol,name)==0)
      return type;
  return 0;
}

wOp *wDocument::FindStore(const sChar *name)
{
  wOp *op;
  wDocName cutname;

  const sChar *filter = 0;
  sInt pos = sFindFirstChar(name,':');
  if(pos>=0)
  {
    cutname = name;
    cutname[pos] = 0;
    filter = name+pos+1;
    name = cutname;
  }

  op = FindStoreNoExtr(name);

  if(filter && op)
    op = op->MakeExtractionTo(filter);

  return op;
}

wOp *wDocument::FindStoreNoExtr(const sChar *name)
{
  sInt min = 0;
  sInt max = Stores.GetCount()-1;
  if(max==-1)
    return 0;

  while(max-min>1)
  {
    sInt mid = min + (max-min)/2;
    sInt cmd = sCmpString(name,Stores[mid]->Name);
    if(cmd==0)
      return Stores[mid];
    if(cmd<0)
      max=mid-1;
    if(cmd>0)
      min=mid+1;
  }
  if(sCmpString(Stores[min]->Name,name)==0)
    return Stores[min];
  if(min!=max && sCmpString(Stores[max]->Name,name)==0)
    return Stores[max];
  return 0;
}

wClass *wDocument::FindClass(const sChar *name,const sChar *tname)
{
  wType *type = 0;
  wClass *cl;

  if(tname)
  {
    type = FindType(tname);
    if(type==0)
      return 0;
  }

  sFORALL(Classes,cl)
    if((type==0 || type==cl->OutputType || type==cl->OutputType->EquivalentType) && sCmpString(cl->Name,name)==0)
      return cl;
  return 0;
}

sBool wDocument::IsOp(wOp *op)
{
  if(!op) return 0;
  return sFindPtr(AllOps,op);
}


void wDocument::SerializeOptions(sReader &s,wDocOptions &options)
{
  sInt version = s.Header(sSerId::Werkkzeug4Doc,VERSION);
  if (version<12)
  {
    s.Fail();
    return;
  }

  s | &options;
}


template <class streamer> void wDocument::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::Werkkzeug4Doc,VERSION);
  wPage *page;
  sArray<wPage *> mypages;
  wStackOp *po;
  wTreeOp *to;
  sArray<wTreeOp *> dummytree;

  // NOTE: doc options first so we can fake-load them without setting up the whole document
  if(version>=12)
    s | &DocOptions;

  if(version)
  {
    // set up arrays

    if(s.IsWriting())
    {
      sFORALL(Pages,page)
        if(page->Include==0)
          mypages.AddTail(page);
    }    

    Doc->UnknownOps = 0;
    s.ArrayNew(mypages);
    sFORALL(mypages,page)
    {
      s.ArrayNew(page->Ops);
      sFORALL(page->Ops,po)
        s.RegisterPtr(po);
      if(version>=6)
      {
        s.ArrayNew(page->Tree);
        sFORALL(page->Tree,to)
          s.RegisterPtr(to);
      }
      if(version>=8)
        s | page->TreeInfo.Level | page->TreeInfo.Flags;
      if(version>=10)
        s | page->ManualWriteProtect;
    }
    if(version<6)
    {
      s.ArrayNew(dummytree);
      sFORALL(dummytree,to)
        s.RegisterPtr(to);
    }

    sInt importcount=0;   // obsolete
    s | importcount;
    
    // doc options

    if(version>=4 && version<12)
      s | &DocOptions;

    // pages

    sFORALL(mypages,page)
    {
      s | page->Name;
      if(version>=6)
        s | page->IsTree;
      if(version>=7)
        s | page->ScrollX | page->ScrollY;
      sFORALL(page->Ops,po)
      {
        s | po->PosX | po->PosY | po->SizeX | po->SizeY;
        if(version>=5)
          s | po->Bypass | po->Hide;
        s | (wOp *)po;
      }
      sFORALL(page->Tree,to)
      {
        s | to->TreeInfo.Level;
        s | to->TreeInfo.Flags;
        s | (wOp *)to;
      }
    }

    // tree

    if(version<6)
    {
      sFORALL(dummytree,to)
      {
        s | to->TreeInfo.Level;
        s | to->TreeInfo.Flags;
        s | (wOp *)to;
      }
      dummytree.Clear();
    }

    // imports

    if(version>=2)
    {
      sString<256> str;
      sInt ena=0;
      for(sInt i=0;i<importcount;i++)
      {
        s | str;
        s | str;
        if(version>=3)
          s | ena;
      }
    }

    // write pages

    if(s.IsReading())
      Pages.Add(mypages);

    // includes

    if(version>=9)
    {
      s.ArrayNew(Includes);
      wDocInclude *inc;
      sFORALL(Includes,inc)
      {
        s | inc->Filename;
      }
    }

    // page order for sorting includes

    if(version>=11)
    {
      sPoolString *str;
      PageNames.Clear();
      if(s.IsWriting())
      {
        sFORALL(Pages,page)
          PageNames.AddTail((const sChar *)page->Name);
      }
      s.Array(PageNames);
      sFORALL(PageNames,str)
        s | *str;
    }
  }

  DocChanged = 0;
  if(s.IsReading() && !Doc->IsPlayer && !Doc->DocOptions.ProjectPath.IsEmpty())
    sChangeDir(Doc->DocOptions.ProjectPath);
  if(s.IsReading() && PostLoadAction)
   (*PostLoadAction)();
}
void wDocument::Serialize(sWriter &stream) { Connect(); Serialize_(stream); }
void wDocument::Serialize(sReader &stream) { Pages.Clear(); Includes.Clear(); Serialize_(stream); Connect(); }

wObject *wDocument::CalcOp(wOp *op,sBool honorslow)
{
  sArray<wOp *> failed;
  wOp *weak;

  // first calc all weak linked ops.

  sFORALL(DirtyWeakOps,weak)
  {
    wObject *obj = Builder->Execute(*Exe,weak,honorslow,1);
    if(!obj)
      failed.AddTail(weak);
    else
      obj->Release();
  }
  DirtyWeakOps.Clear();

  // if some weak linked op failed, ignore the whole weak linking system

  if(failed.GetCount()>0)
  {
    sFORALL(failed,weak)
      Change(weak,1);
  }

  // now try to calc the real op.

  wObject *res = Builder->Execute(*Exe,op,honorslow,1);
  if(!res)
  {
    PropagateCalcError(op);
  }

  return res;
}

void wDocument::PropagateCalcError(wOp *op)
{
  wOp *o2;
  wOpInputInfo *link;
  if(!op->CycleCheck)
  {
    op->CycleCheck++;

    sBool error = 0;
    sFORALL(op->Inputs,o2)
    {
      PropagateCalcError(o2);
      if(o2->CalcErrorString)
        error = 1;
    }
    sFORALL(op->Links,link)
    {
      if(link->Link)
      {
        PropagateCalcError(link->Link);
        if(link->Link->CalcErrorString)
          error = 1;
      }
    }

    if(!op->CalcErrorString && error)
      op->CalcErrorString = L"...";

    op->CycleCheck--;
  }
}

const sChar *wDocument::CreateRandomString()
{
  const sInt size=8;
  static sChar buffer[size+1];
  for(sInt i=0;i<size;i++)
    buffer[i] = Rnd.Int('z'-'a'+1)+'a';
  buffer[size]=0;
  return buffer;
}

void wDocument::ClearSlowFlags()
{
  wOp *op;
  sFORALL(AllOps,op)
    op->SlowSkipFlag = 0;
}

void wDocument::Show(wObject *obj,wPaintInfo &pi)
{
  pi.DeleteHandlesList.Clear();

  wType *type;
  Spec = pi.Spec;
  ViewLog = pi.ViewLog;
  sFORALL(Types,type)
    type->BeginShow(pi);
  obj->Type->Show(obj,pi);
  sFORALL(Types,type)
    type->EndShow(pi);
  ViewLog = 0;

  if(!pi.DontPaintHandles)
  {
    if(pi.DeleteSelectedHandles)
      pi.ClearHandleSelection();

    if(pi.DeleteHandlesList.GetCount()>0)
    {
      wOp *op;
      sFORALL(pi.DeleteHandlesList,op)
      {
        Change(op);
        op->Page->Rem(op);
      }
      Connect();
    }
  }
}

sInt wDocument::SecondsToBeats(sF32 t)
{
  if(!Doc->DocOptions.VariableBpm)
  {
    return sInt(t * DocOptions.BeatsPerSecond)+DocOptions.BeatStart;
  }
  else
  {
    wDocOptions::BpmSegment *s;
    sInt b = DocOptions.BeatStart;
    sFORALL(DocOptions.BpmSegments,s)
    {
      if(t<s->Milliseconds*0.001f)
      {
        b += sMulDiv(t,s->Bps,1000);
        return b;
      }
      else
      {
        t -= s->Milliseconds*0.001f;
        b += s->Beats*0x10000;
      }
    }
    return b;
  }
}

sInt wDocument::MilliSecondsToBeats(sInt t)
{
  if(!Doc->DocOptions.VariableBpm)
  {
    return sMulDiv(t,DocOptions.BeatsPerSecond,1000)+DocOptions.BeatStart;
  }
  else
  {
    wDocOptions::BpmSegment *s;
    sInt b = DocOptions.BeatStart;
    sFORALL(DocOptions.BpmSegments,s)
    {
      if(t<s->Milliseconds)
      {
        b += sMulDiv(t,s->Bps,1000);
        return b;
      }
      else
      {
        t -= s->Milliseconds;
        b += s->Beats*0x10000;
      }    
    }
  
    // extrapolate last segment after the end
    if (t>0)
    {
      sInt bps = DocOptions.BpmSegments.IsEmpty()?(2<<16):DocOptions.BpmSegments.GetTail().Bps;
      b += sMulDiv(t,bps,1000);
    }

    return b;
  }
}

sInt wDocument::SampleToBeats(sInt t)
{
  if(!Doc->DocOptions.VariableBpm)
  {
    return sMulDiv(t,Doc->DocOptions.BeatsPerSecond,DocOptions.SampleRate)+Doc->DocOptions.BeatStart;
  }
  else
  {
    wDocOptions::BpmSegment *s;
    sInt b = DocOptions.BeatStart;
    sFORALL(DocOptions.BpmSegments,s)
    {
      if(t<s->Samples)
      {
        b += sMulDiv(t,s->Bps,DocOptions.SampleRate);
        return b;
      }
      else
      {
        t -= s->Samples;
        b += s->Beats*0x10000;
      }
    }
    return b;
  }
}


sInt wDocument::BeatsToMilliseconds(sInt b)
{
  if(!Doc->DocOptions.VariableBpm)
  {
    return sMulDiv(b-DocOptions.BeatStart,1000,DocOptions.BeatsPerSecond);
  }
  else
  {
    wDocOptions::BpmSegment *s;
    b -= DocOptions.BeatStart;
    if(b<=0)
      return 0;
    sInt t = 0;
    sFORALL(DocOptions.BpmSegments,s)
    {
      if(b<s->Beats*0x10000)
      {
        t += sMulDiv(b,1000,s->Bps);
        return t;
      }
      else
      {
        b -= s->Beats*0x10000;
        t += s->Milliseconds;
      }
    }
    return t;
  }
}

sInt wDocument::BeatsToSample(sInt b)
{
  if(!Doc->DocOptions.VariableBpm)
  {
    return sMax(0,sMulDiv(b-DocOptions.BeatStart,DocOptions.SampleRate,DocOptions.BeatsPerSecond));
  }
  else
  {
    wDocOptions::BpmSegment *s;
    b -= DocOptions.BeatStart;
    if(b<=0)
      return 0;
    sInt t = 0;
    sFORALL(DocOptions.BpmSegments,s)
    {
      if(b<s->Beats*0x10000)
      {
        t += sMulDiv(b,DocOptions.SampleRate,s->Bps);
        return t;
      }
      else
      {
        b -= s->Beats*0x10000;
        t += s->Samples;
      }
    }
    return t;
  }
}


sBool wDocument::RenameAllOps(const sChar *from,const sChar *to)
{
  Connect();
  wOp *op;
  wOpInputInfo *info;
  sFORALL(AllOps,op)
  {
    if(sCmpString(op->Name,to)==0)
      return 0;
  }
  sFORALL(AllOps,op)
  {
    if(sCmpString(op->Name,from)==0)
      op->Name = to;
    sFORALL(op->Links,info)
      if(sCmpString(info->LinkName,from)==0)
        info->LinkName = to;
  }
  Connect();
  return 1;
}

sBool wDocument::UnCacheLRU()
{
  wOp *best0 = 0;
  wOp *best1 = 0;
  sU32 lru0 = CacheLRU;
  sU32 lru1 = CacheLRU;
  wOp *op;

  sFORALL(AllOps,op)
  {
    if(op->Cache && (op->Cache->Type->Flags & wTF_UNCACHE))
    {
      if(op->CacheLRU<lru0)
      {
        best0 = op;
        lru0 = op->CacheLRU;
      }
      if(op->CacheLRU<lru1 && op->Cache->RefCount==1)
      {
        best1 = op;
        lru1 = op->CacheLRU;
      }
    }
  }

  if(best1)                       // best op with only one ref left
  {
    sRelease(best1->Cache);
    if(LOGIT)
      sDPrintF(L" *");
    return 1;
  }
  if(best0)                       // best op with more than one ref left
  {
    sRelease(best0->Cache);
    if(LOGIT)
      sDPrintF(L" *");
    return 1;
  }
  return 0;
}

/****************************************************************************/

void wDocument::GlobalAction(const sChar *name)
{
  wOp *op;
  wClass *cl;
  wClassActionInfo *ac;
  Connect();

  sFORALL(Classes,cl)
  {
    cl->Temp = -1;
    sFORALL(cl->ActionIds,ac)
      if(sCmpStringI(ac->Name,name)==0)
        cl->Temp = ac->Id;
  }

  sFORALL(AllOps,op)
  {
    if(op->Class->Temp>=0)
    {
      (*op->Class->Actions)(op,op->Class->Temp,0);
    }
  }
} 


void wDocument::CheckShellSwitches()
{
  ShellSwitchChoice[0]=0;
  for(sInt i=0;i<wSWITCHES;i++)
  {
    if(i>0)
      ShellSwitchChoice.Add(L"|");
    ShellSwitchChoice.Add(L" ");
    ShellSwitchChoice.Add(ShellSwitchOptions[i]);
  }
  for(sInt i=0;;i++)
  {
    const sChar *o = sGetShellParameter(L"s",i);
    if(o==0)
      break;
    sBool found = 0;
    for(sInt j=0;j<wSWITCHES && !found;j++)
    {
      if(sCmpStringI(o,ShellSwitchOptions[j])==0)
      {
        found = 1;
        Doc->ShellSwitches |= 1<<j;
      }
    }
    if(!found)
      sDPrintF(L"unknown switch option \"-s %s\".\n",o);
  }

  // define script string
  for(sInt j=1;j<4;j++)
  {
    static const sChar *optname[] = { 0,L"ds",L"dd",L"df" };
    for(sInt i=0;;i++)
    {
      sBool ok = 1;
      const sChar *d = sGetShellParameter(optname[j],i);
      if(d==0)
        break;
      const sChar *ptr = d;

      wScriptDefine sd;
      sd.Mode = j;
      sd.IntValue = 0;
      sd.FloatValue = 0;
      sd.StringValue = L"";

      while(*ptr!=0 && *ptr!='=')
        ptr++;
      if(*ptr!='=')
      {
        ok = 0;
      }
      sString<sMAXPATH> buffer;
      buffer.Init(d,ptr-d);
      sMakeLower(buffer);
      sd.Name = buffer;

      ptr++;

      switch(j)
      {
      case 1:
        sd.StringValue = ptr;
        break;
      case 2:
        ok = sScanInt(ptr,sd.IntValue);
        break;
      case 3:
        ok = sScanFloat(ptr,sd.FloatValue);
        break;
      }

      if(ok)
        *ScriptDefines.AddMany(1) = sd;
      else
        sPrintF(L"error in option -%s %s\n",optname[j],d);
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   includes                                                           ***/
/***                                                                      ***/
/****************************************************************************/

wDocInclude::wDocInclude()
{
  Active = 0; 
  Protected = 0;
  Filename = L"new.wz4i";
}

wDocInclude::~wDocInclude()
{
}

void wDocInclude::Tag() 
{
  sObject::Tag();
}

void wDocInclude::Clear()
{
  wPage *page;
  sFORALL(Doc->Pages,page)
    page->Temp = page->Include==this;
  sRemTrue(Doc->Pages,&wPage::Temp);
  Active = 0;
  Protected = 1;
}

void wDocInclude::New()
{
  Clear();
  
  wPage *page = new wPage;
  page->DefaultName();
  page->Include = this;
  Doc->Pages.AddTail(page);

  Active = 1;
  Protected = 0;
  sBool prot = 0;
  if(sGetFileWriteProtect(Filename,prot))
    Protected = prot;
}

sBool wDocInclude::Load()
{
  sBool ok = 0;
  Clear();
  if(!Active)
  {
    ok = sLoadObject(Filename,this);
    if(ok)
    {
      Active = 1;
      sBool prot = 1;
      sGetFileWriteProtect(Filename,prot);
      Protected = prot;
    }
  }
  return ok;
}

sBool wDocInclude::Save()
{
  if(Protected)
    return 1;
  if(!Active)
    return 1;
  return sSaveObject(Filename,this);
}

/****************************************************************************/

template <class streamer> void wDocInclude::Serialize_(streamer &s)
{
  sArray<wPage *> mypages;
  wPage *page;
  wStackOp *po;
  wTreeOp *to;

  sInt version = s.Header(sSerId::Werkkzeug4DocInclude,3);
  if(version)
  {
    if(s.IsWriting())
    {
      sFORALL(Doc->Pages,page)
        if(page->Include == this)
          mypages.AddTail(page);
    }

    s.ArrayNew(mypages);
    sFORALL(mypages,page)
    {
      s.ArrayNew(page->Ops);
      sFORALL(page->Ops,po)
        s.RegisterPtr(po);
      s.ArrayNew(page->Tree);
      sFORALL(page->Tree,to)
        s.RegisterPtr(to);
      s | page->TreeInfo.Level | page->TreeInfo.Flags;
      if(version>=2)
        s | page->ManualWriteProtect;
    }

    sFORALL(mypages,page)
    {
      s | page->Name;
      s | page->IsTree;
      s | page->ScrollX | page->ScrollY;
      sFORALL(page->Ops,po)
      {
        if(version<3)
        {
          s | po->PosX | po->PosY | po->SizeX | po->SizeY;
          s | po->Bypass | po->Hide;
          s | po;
          //s | (wOp *)po;    // use this line instead to load files saved before wStackOp got own serialize and wOp::Serialize was used
        }
        if(version>=3)
          s | po;
      }
      sFORALL(page->Tree,to)
      {
        if(version<3)
        {
          s | to->TreeInfo.Level;
          s | to->TreeInfo.Flags;
          s | to;
          //s | (wOp *)to;    // use this line instead to load files saved before wStackOp got own serialize and wOp::Serialize was used
        }
        if(version>=3)
          s | to;

      }
    }

    if(s.IsReading())
    {
      sFORALL(mypages,page)
        page->Include = this;
      Doc->Pages.Add(mypages);
    }
  }
}

void wDocInclude::Serialize(sWriter &s) { Serialize_(s); }
void wDocInclude::Serialize(sReader &s) { Serialize_(s); }

/****************************************************************************/
/***                                                                      ***/
/***   execution                                                          ***/
/***                                                                      ***/
/****************************************************************************/

wObject::wObject()
{
  Type = 0;
  RefCount = 1;
  CallId = 0;
}

wObject::~wObject()
{
}

/****************************************************************************/

wExecutive::wExecutive()
{
  MemPool = new sMemoryPool(0x10000);
}

wExecutive::~wExecutive()
{
  delete MemPool;
}


void ProgressPaint(sInt count,sInt max)
{
  static sInt lasttick = 0;
  sInt tick = sGetTime();
  if(tick<lasttick+500)
    return;
  lasttick = tick;
  sRect r,ro,ri;
  sInt sx,sy;

  sGetWindowSize(sx,sy);

  r.x0 = 20;
  r.x1 = sx-20;
  r.y0 = sy/2-15;
  r.y1 = sy/2+15;

  count = sClamp(count,0,max);

  sRender2DBegin();

  ro = r;
  r.Extend(-2);
  ri = r;
  sRectHole2D(ro,ri,sGC_WHITE);

  ro = r;
  r.Extend(-2);
  ri = r;
  sRectHole2D(ro,ri,sGC_BLACK);

  ro = ri = r;
  ri.x1 = ro.x0 = r.x0+sMulDiv(count,r.SizeX(),max);
  sRect2D(ro,sGC_BLACK);
  sRect2D(ri,sGC_WHITE);

  sRender2DEnd();
}

void (*ProgressPaintFunc)(sInt count, sInt max) = ProgressPaint;

wObject *wExecutive::Execute(sBool progress,sBool depend)
{
  wCommand *cmd;
  sBool allok = 1;
  sBool logging = 0;
  wObject *result = 0;
  sBool ok;
  sInt cmdcount = Commands.GetCount();
  sInt ProgressTimer = sGetTime()+500;
  sInt ProgressEnable = 0;
  sBool Fail=0;

  Doc->CacheWarmupBeat.Clear();
  sCheckBreakKey();   // throw away any break key in queue
  sPtr memlimit = sPtr(Doc->EditOptions.MemLimit)*1024*1024;

  if(cmdcount>0)
  {
    sFORALL(Commands,cmd)
      if(cmd->Op)
        cmd->Op->CalcErrorString = 0;
    if(LOGIT)
      sDPrintF(L"Calc:");

    sFORALL(Commands,cmd)
    {
      if(!Fail && sCheckBreakKey())
        Fail = 1;
      if(LOGIT)
        if(cmd->Op)
          sDPrintF(L" %s",cmd->Op->Class->Name);
      if(cmd->Op && (cmd->Op->Class->Flags & wCF_LOGGING))
      {
        if(!logging)
        {
          logging = 1;
          BeginLogging();
          ProgressTimer = 0;
        }
      }
      if(progress && !ProgressEnable && sGetTime()>ProgressTimer)
        ProgressEnable = 1;
      if(ProgressEnable && ProgressPaintFunc)
        ProgressPaintFunc(_i+1,Commands.GetCount());
      ok = 1;
      if(allok)
      {
        sVERIFY(cmd->Output==0);
        if(cmd->Op && cmd->Op->WeakCache)
        {
          cmd->Output = cmd->Op->WeakCache;
          cmd->Output->Reuse();
          cmd->Output->AddRef();
        }

        if(cmd->PassInput>=0)
        {
          wObject *in = cmd->GetInput<wObject *>(cmd->PassInput);
          if(in && in->RefCount==1)
          {
            cmd->Output = in;
            cmd->Inputs[cmd->PassInput]->Output=0;
          }
        }

        // script

        sBool vars_from_context = 0;
        if(cmd->Script)
        {
          vars_from_context = 1;
          cmd->Script->PushGlobal();
          cmd->Script->ClearImports();

          for(sInt i=0;i<cmd->FakeInputCount;i++)
          {
            if(cmd->Inputs[i])
            {
              for(sInt j=0;j<cmd->Inputs[i]->OutputVarCount;j++)
              {
                wScriptVar *var = cmd->Inputs[i]->OutputVars+j;
                cmd->Script->AddImport(var->Name,var->Type,var->Count,var->IntVal);
              }
            }
          }
          if(!cmd->LoopName.IsEmpty())
          {
            ScriptValue *val = cmd->Script->MakeFloat(1);
            val->FloatPtr[0] = cmd->LoopValue;
            cmd->Script->BindGlobal(cmd->Script->AddSymbol(cmd->LoopName),val);
          }

          if(cmd->ScriptBind2)
            (*cmd->ScriptBind2)(cmd,cmd->Script);

          if(cmd->ScriptSource)
          {
            cmd->Script->AddImport(L"lowquality",ScriptTypeInt,1,&Doc->LowQuality);
            wScriptDefine *sd;
            sFORALL(Doc->ScriptDefines,sd)
            {
              if(sd->Mode==1)
                cmd->Script->AddImport(sd->Name,ScriptTypeString,1,&sd->StringValue);
              if(sd->Mode==2)
                cmd->Script->AddImport(sd->Name,ScriptTypeInt,1,&sd->IntValue);
              if(sd->Mode==3)
                cmd->Script->AddImport(sd->Name,ScriptTypeFloat,1,&sd->FloatValue);
            }
            ScriptCode code(cmd->ScriptSource,0);
            const sChar *error = cmd->Script->Run();
            if(error)
            {
              ok = 0;
              cmd->SetError(sPoolString(error));
              sDPrintF(L"\n%s\n",error);
            }
          }
        }

        // execute

        if(depend)    // don't really execute, just determine dependencies
        {
          cmd->Output = new AnyType;
          cmd->Output->CallId = cmd->CallId;

          if(cmd->Op)
          {
            for(sInt i=0;i<cmd->Op->Class->ParaStrings;i++)
            {
              if((cmd->Op->Class->FileInMask & (1<<i)) && sCmpString(cmd->Strings[i],L"")!=0)
                if(!sMatchWildcard(L"*.kd",cmd->Strings[i],0))
                  sPrintF(L"in_execute \"%p\";\n",cmd->Strings[i]);
              if((cmd->Op->Class->FileOutMask & (1<<i)) && sCmpString(cmd->Strings[i],L"")!=0)
                if(!sMatchWildcard(L"*.kd",cmd->Strings[i],0))
                  sPrintF(L"out \"%p\";\n",cmd->Strings[i]);
            }
          }
        }
        else          // really execute
        {
          if(cmd->Code)
          {
            if(cmd->Op)
              sPushMemLeakDesc(cmd->Op->Class->OutputType->Symbol);
            else
              sPushMemLeakDesc(L"unknown op");
            if(Fail)
              ok = 0;
            if(ok)
              if(!(*cmd->Code)(this,cmd))
                ok = 0;
            if(ok && cmd->Output)
              cmd->Output->CallId = cmd->CallId;
            sPopMemLeakDesc();
          }
          else
          {
            if(cmd->InputCount>0 && cmd->Inputs[0] && cmd->Inputs[0]->Output)
            {
              cmd->Output = cmd->Inputs[0]->Output;
              cmd->Output->AddRef();
            }
            else      // fake op, just generate variables 
            {
              cmd->Output = new wObject;
            }
            ok = 1;
          }

          if(allok && !ok)
          {
            sDPrintF(L" (FAIL)");
            if(cmd->Op)
            {
              sPrintF(L"operator class %q failed\n",cmd->Op->Class->Label);
              sDPrintF(L"operator class %q failed\n",cmd->Op->Class->Label);
              wPage *page;
              sFORALL(Doc->Pages,page)
              {
                if(sFindPtr(page->Ops,cmd->Op))
                {
                  wStackOp *op = (wStackOp *)cmd->Op;
                  sPrintF(L"location page %q, x=%d, y=%d\n",page->Name,op->PosX,op->PosY);
                  sDPrintF(L"location page %q, x=%d, y=%d\n",page->Name,op->PosX,op->PosY);
                }
              }
              for(sInt i=0;i<cmd->Op->EditStringCount;i++)
                sPrintF(L"string %d:%q\n",i,cmd->Op->EditString[i]->Get());
              wOpInputInfo *info;
              sFORALL(cmd->Op->Links,info)
                if(!info->LinkName.IsEmpty())
                  sPrintF(L"link %d:%q\n",_i,info->LinkName);
            }
          }
          allok &= ok;
          if(cmd->Op && cmd->Op->WeakOutputs.GetCount())
          {
            cmd->Op->WeakCache->Release();
            if(ok)
            {
              cmd->Op->WeakCache = cmd->Output;
              cmd->Op->WeakCache->AddRef();
            }
          }
        }

        // gather globals (inputs + script)

        if(allok)
        {
          cmd->OutputVarCount = 0;
          sInt count = 0;
          for(sInt i=0;i<cmd->FakeInputCount;i++)
            if(cmd->Inputs[i])
              count += cmd->Inputs[i]->OutputVarCount;
          if(!cmd->LoopName.IsEmpty())
            count++;

          if(vars_from_context)
          {
            cmd->Script->FlushLocal();
            ScriptValue *val = cmd->Script->GetFirstFromScope();
            while(val)
            {
              val = val->ScopeLink;
              count++;
            }
          }

          cmd->OutputVars = MemPool->Alloc<wScriptVar>(count);
          if(!cmd->LoopName.IsEmpty())
          {
            wScriptVar var;
            var.Count = 1;
            var.Name = cmd->LoopName;
            var.Type = ScriptTypeFloat;
            var.FloatVal[0] = cmd->LoopValue;
            cmd->AddOutputVar(var);
          }
          for(sInt i=0;i<cmd->FakeInputCount;i++)
          {
            if(cmd->Inputs[i])
            {
              for(sInt j=0;j<cmd->Inputs[i]->OutputVarCount;j++)
                cmd->AddOutputVar(cmd->Inputs[i]->OutputVars[j]);
            }
          }

          if(vars_from_context)
          {
            ScriptValue *val = cmd->Script->GetFirstFromScope();
            while(val)
            {
              if(val->Symbol && val->Count<4 && (val->Type==ScriptTypeInt || val->Type==ScriptTypeFloat || val->Type==ScriptTypeString || val->Type==ScriptTypeColor))
              {
                wScriptVar var;
                var.Name = val->Symbol->Name;
                var.Type = val->Type;
                var.Count = val->Count;
                if(var.Type==ScriptTypeString)
                {
                  for(sInt i=0;i<var.Count;i++)
                    var.StringVal[i] = val->StringPtr[i];
                }
                else
                {
                  for(sInt i=0;i<var.Count;i++)
                    var.IntVal[i] = val->IntPtr[i];
                }
                cmd->AddOutputVar(var);
              }
              val = val->ScopeLink;
            }
            cmd->Script->PopGlobal();
          }
        }
      }

      if(!allok /*&& cmd->CallId==0*/)
      {
        sInt error = 0;
        for(sInt i=0;i<cmd->FakeInputCount;i++)
          if(cmd->Inputs[i])
            error |= cmd->Inputs[i]->ErrorFlag;
        if(error)
          cmd->SetError(L"....");
      }

      if(cmd->Output)
      {
        if(!cmd->Output->Type && cmd->Op)
          sFatal(L"forgot to initialize Type field in wObject constructor of\n"
                 L"operator %s %s(...)",cmd->Op->Class->OutputType->Label,cmd->Op->Class->Label);
        
        cmd->Output->RefCount += cmd->OutputRefs;
      }
      if(ok && cmd->Op)
        cmd->Op->Strobe = 0;
      if(!ok)
        cmd->SetError(L"calculation error");
      for(sInt i=0;i<cmd->InputCount;i++)
        if(cmd->Inputs[i])
          cmd->Inputs[i]->Output->Release();
      if(cmd->StoreCacheOp && allok)
      {
        if(cmd->StoreCacheOp->Cache)
        {
          // this should only happen in a subroutine that is evaluated multiple times!
          cmd->StoreCacheOp->Cache->Release();
        }
        cmd->StoreCacheOp->Cache = cmd->Output;
        cmd->StoreCacheOp->CacheLRU = Doc->CacheLRU++;
        cmd->StoreCacheOp->CacheVars.Clear();
        cmd->StoreCacheOp->CacheVars.Resize(cmd->OutputVarCount);
        for(sInt i=0;i<cmd->OutputVarCount;i++)
          cmd->StoreCacheOp->CacheVars[i] = cmd->OutputVars[i];
        cmd->Output->AddRef();
      }
      if(_i==cmdcount-1 && allok)
        result = cmd->Output;
      else
        cmd->Output->Release();

      // memorymanagement

      while(allok && memlimit>0 && sMemoryUsed>memlimit)
      {
        if(!Doc->UnCacheLRU())
          break;
      }
    }

    if(LOGIT)
      sDPrintF(L"\n");
  }
  if(logging)
    EndLogging();

  if(ProgressEnable && ProgressPaintFunc)
  {
    ProgressPaintFunc(Commands.GetCount(),Commands.GetCount());
    sUpdateWindow();
  }

//  sGetMemoryLeakTracker()->DumpLeaks(L"execution",0,1);

  return result;
}


void wCommand::AddOutputVar(wScriptVar &var)
{
  for(sInt i=0;i<OutputVarCount;i++)
  {
    if(OutputVars[i].Name==var.Name)
    {
      OutputVars[i] = var;
      return;
    }
  }
  OutputVars[OutputVarCount++] = var;
}


static sRect LogRect;
static sTextBuffer *LogBuffer;
static sThreadLock *LogLock;
static sInt LogScroll;

void OpPrint(const sChar *text)
{
#if !sCOMMANDLINE
  if(LogLock && LogLock->TryLock())
  {
    LogBuffer->Print(text);
    sRect r;
    
    r = LogRect;
    sRender2DBegin();
    sRectFrame2D(r,sGC_DRAW);
    r.Extend(-1);
    sGui->FixedFont->SetColor(sGC_TEXT,sGC_BACK);
    sClipPush();
    sClipRect(r);
    sInt y = sGui->FixedFont->Print(sF2P_MULTILINE|sF2P_LEFT|sF2P_BOTTOM|sF2P_OPAQUE,r,LogBuffer->Get(),-1,4,0,-LogScroll);
    LogScroll = y+sGui->FixedFont->GetHeight()-r.y1 + LogScroll;
    sClipPop();
    sRender2DEnd();
    LogLock->Unlock();
  }
#endif
}

void wExecutive::BeginLogging()
{
#if !sCOMMANDLINE
  sInt xs,ys;
  sGetScreenSize(xs,ys);
  LogBuffer = new sTextBuffer;
  LogLock = new sThreadLock;
  LogRect.Init(xs*2/10,ys*2/10,xs*8/10,ys*8/10);

  OpPrint(L"");
  sRedirectStdOut = OpPrint;
#endif
}

void wExecutive::EndLogging()
{
#if !sCOMMANDLINE
  sRedirectStdOut = 0;
  sDelete(LogBuffer);
  sDelete(LogLock);
  sGui->Update(LogRect);
#endif
}

void ViewPrint(const sChar *text)
{
  if(Doc && Doc->ViewLog)
    Doc->ViewLog->Print(text);
}

/****************************************************************************/

void NXNCheckout(const sChar *filename)
{
  sDirEntry de;

  if (sGetFileInfo(filename, &de) && (de.Flags & sDEF_WRITEPROTECT))
  {
    if(sCmpStringPLen(filename,L"c:/nxn/",7)==0)
    {
      const sChar* s = filename;
      s+=7;
      sString<2048> project;
      sChar* d = project;
      while(*s!='/' && *s!='\\' && *s!=0)
        *d++ = *s++;
      *d++ = 0;

      if(*s=='/' || *s=='\\')
      {
        sString<2048> buffer;
        sBool filecheckedout = 0;
        sString<2048> name = s+1;
        sString<sMAXPATH> path;
        path = project;
        path.AddPath(name);

        sSPrintF(buffer,
          L"alienbrainconsole command='checkout' "
          L"namespacepath='/workspace/%s' "
          L"username='chaos' "
          L"password='qwer12'",
          path);

        sPrintF(L"%s\n",buffer);
        sExecuteShell(buffer);

        if(sGetFileInfo(filename,&de) && (de.Flags & sDEF_WRITEPROTECT))
        {
          sSPrintF(buffer, L"attrib -r \"%s\"", filename);
          sPrintF(L"%s\n",buffer);
          sExecuteShell(buffer);

          if(sGetFileInfo(filename,&de) && !(de.Flags & sDEF_WRITEPROTECT))
          {
            filecheckedout = 1;
          }
        }

        if(!filecheckedout)
        {
          sPrintF(L"could not unlock file <%s>\n", filename);
        }
      }
      else
      {
        sPrintF(L"could not map file <%s> to NxN\n", filename);
      }
    }
    else
    {
      sPrintF(L"could not map file <%s> to NxN\n", filename);
    }
  }
}

/****************************************************************************/
