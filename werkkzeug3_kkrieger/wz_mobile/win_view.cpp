// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "win_view.hpp"
#include "main_mobile.hpp"
#include "gen_bitmap_class.hpp"

/****************************************************************************/

ViewBitmapWin_::ViewBitmapWin_()
{
  Flags |= sGWF_PAINT3D;

  ShowOp = 0;

  FullSize = 0;

  ShowSizeX = 4;
  ShowSizeY = 4;
#if VARIANT==VARIANT_TEXTURE
  ShowVariant = 0;
#else
  ShowVariant = 2;
#endif

  BitmapX = 10;
  BitmapY = 10;
  BitmapTile = 0;
  BitmapAlpha = 0;
  BitmapZoom = 5;

  MtrlEnv.Init();
  MtrlEnv.Orthogonal = sMEO_PIXELS;

  MtrlTex = new sMaterial11;
  MtrlTex->ShaderLevel = sPS_00;
  MtrlTex->SetTex(0,0);
  MtrlTex->TFlags[0] = 0;
  MtrlTex->Combiner[sMCS_TEX0] = sMCOA_SET;
  MtrlTex->BaseFlags = sMBF_NONORMAL;
  MtrlTex->Compile();

  MtrlTexAlpha = new sMaterial11;
  MtrlTexAlpha->CopyFrom(MtrlTex);
  MtrlTexAlpha->BaseFlags |= sMBF_BLENDALPHA;
  MtrlTexAlpha->AlphaCombiner = sMCA_TEX0;
  MtrlTexAlpha->Compile();

  if(sSystem->GetShaderLevel()<sPS_11)
  {
    MtrlNormal = new sMaterial11;
    MtrlNormal->CopyFrom(MtrlTex);
    MtrlNormalLight = new sMaterial11;
    MtrlNormalLight->CopyFrom(MtrlTex);
  }
  else
  {
    MtrlNormal = new sMaterial11;
    MtrlNormal->CopyFrom(MtrlTex);
    MtrlNormal->ShaderLevel = sPS_11;
    MtrlNormal->Combiner[sMCS_COLOR0] = sMCOA_SET;
    MtrlNormal->Combiner[sMCS_TEX0] = sMCOA_MUL;
    MtrlNormal->Combiner[sMCS_COLOR2] = sMCOA_ADD;
    MtrlNormal->Color[0] = 0x00808080;
    MtrlNormal->Color[2] = 0x00808080;
    MtrlNormal->BaseFlags = sMBF_NONORMAL;
    MtrlNormal->Compile();

    MtrlNormalLight = new sMaterial11;
    MtrlNormalLight->ShaderLevel = sPS_11;
    MtrlNormalLight->BaseFlags = sMBF_ZOFF;
    MtrlNormalLight->LightFlags = sMLF_BUMPX;
    MtrlNormalLight->SpecPower = 24.0f;
    MtrlNormalLight->Combiner[sMCS_LIGHT] = sMCOA_SET;
    MtrlNormalLight->Combiner[sMCS_COLOR0] = sMCOA_MUL;
    MtrlNormalLight->Color[0] = 0x80c0c0c0;
    MtrlNormalLight->SetTex(1,0);
    MtrlNormalLight->TFlags[1] = 0;
    MtrlNormalLight->Compile();
  }

  {
    static sGuiMenuList ml[] = 
    {
      { sGML_COMMAND  ,'r'      ,CMD_VIEWBMP_RESET  ,0,"Reset" },
      { sGML_SPACER },
      { sGML_CHECKMARK,'t'      ,CMD_VIEWBMP_TILE      ,sOFFSET(ViewBitmapWin_,BitmapTile) ,"Tile"  },
      { sGML_CHECKMARK,'a'      ,CMD_VIEWBMP_ALPHA     ,sOFFSET(ViewBitmapWin_,BitmapAlpha),"Alpha" },
      { sGML_CHECKMARK,sKEY_TAB ,CMD_VIEWBMP_FULLSIZE  ,sOFFSET(ViewBitmapWin_,FullSize)   ,"Full size" },

      { 0 },
    };
    SetMenuList(ml);
  }

  Tools = new sToolBorder;
  Tools->AddLabel(".preview bitmap");
  Tools->AddContextMenu(CMD_VIEWBMP_POPUP);

  sControl *con;
  
  con = new sControl;
  con->EditChoice(0,&ShowSizeX,0,"16|32|64|128|256|512|1024|2048");
  con->Style &=~ sCS_FATBORDER;
  con->Style |= sCS_NOBORDER|sCS_SMALLWIDTH|sCS_DONTCLEARBACK|sCS_FAT;
  Tools->AddChild(con);

  con = new sControl;
  con->EditChoice(0,&ShowSizeY,0,"16|32|64|128|256|512|1024|2048");
  con->Style &=~ sCS_FATBORDER;
  con->Style |= sCS_NOBORDER|sCS_SMALLWIDTH|sCS_DONTCLEARBACK|sCS_FAT;
  Tools->AddChild(con);

  con = new sControl;
  con->EditChoice(0,&ShowVariant,0,"16C|16I|8C|8I");
  con->Style &=~ sCS_FATBORDER;
  con->Style |= sCS_NOBORDER|sCS_SMALLWIDTH|sCS_DONTCLEARBACK|sCS_FAT;
  Tools->AddChild(con);

  AddBorder(Tools);
}

ViewBitmapWin_::~ViewBitmapWin_()
{
  sRelease(MtrlTex);
  sRelease(MtrlTexAlpha);
  sRelease(MtrlNormalLight);
  sRelease(MtrlNormal);
}

/****************************************************************************/

void ViewBitmapWin_::OnPaint()
{
  if(Flags & sGWF_CHILDFOCUS)
  {
    App->StatusMouse = "m: scroll [menu]   r:zoom";
    App->StatusWindow = "...";
  }
  if(!ShowOp)
    ClearBack(1);
}

void ViewBitmapWin_::OnPaint3d(sViewport &view)
{
//  sU32 states[256],*p;
  sVertexTSpace3 *vp;
  sMatrix mat;
  sMaterial11 *mtrl;
  sU16 *ip;
  sInt x,y,xs,ys;
  sInt handle;
  GenBitmap *bm_flat;
  static sInt var[] = 
  {
    GenBitmapTile::Pixel16C::Variant,
    GenBitmapTile::Pixel16I::Variant,
    GenBitmapTile::Pixel8C::Variant,
    GenBitmapTile::Pixel8I::Variant,
  };

  bm_flat = 0;
  if(ShowOp)
  {
    Doc->Connect();
    if(!ShowOp->Error)
    {
      bm_flat = CalcBitmap(ShowOp,16<<ShowSizeX,16<<ShowSizeY,var[ShowVariant]);
    }
  }

  if(bm_flat)
  {
    if(bm_flat->Texture==sINVALID)
    {
      bm_flat->MakeTexture(bm_flat->Format);
    }

    if(App->CurrentViewWin == this)
      sSPrintF(App->StatusObject,"Bitmap: %d x %d @ %d bits, %dkb",bm_flat->XSize,bm_flat->YSize,bm_flat->BytesPerPixel*8,(bm_flat->BytesTotal+1023)/1024);

//    view.ClearColor = sGui->Palette[sGC_BACK] & 0xffffff;
    mat.Init();

//    p = states;

    sSystem->SetViewport(view);
    sSystem->Clear(sVCF_ALL,sGui->Palette[sGC_BACK] & 0xffffff);
    MtrlTexAlpha->SetTex(0,bm_flat->Texture);
    MtrlTex->SetTex(0,bm_flat->Texture);
    MtrlNormal->SetTex(0,bm_flat->Texture);
    MtrlNormalLight->SetTex(1,bm_flat->Texture);

    if(bm_flat->XSize >= bm_flat->YSize)
    {
      xs = 256 * (1<<BitmapZoom)/16;
      ys = sMulDiv(xs,bm_flat->YSize,bm_flat->XSize);
    }
    else
    {
      ys = 256 * (1<<BitmapZoom)/16;
      xs = sMulDiv(ys,bm_flat->XSize,bm_flat->YSize);
    }

    x = BitmapX;
    y = BitmapY;

    if(bm_flat->Format == sTF_Q8W8V8U8) // normal map
    {
      mtrl = MtrlNormalLight;

      MtrlEnv.LightAmplify = 1.0f;
      MtrlEnv.LightColor.Init(1,1,1,1);

      sInt r = sMax(xs/2,ys/2);
      sVector dir;

      dir.Init(1.0f * LightPosX / r,1.0f * LightPosY / r,0.0f,0.0f);
      dir.z = 1.0 - dir.x * dir.x - dir.y*dir.y;
      dir.z = (dir.z < 0.3f * 0.3f) ? 0.3f : sFSqrt(dir.z);
      dir.Unit3();

      MtrlEnv.LightPos.Init(r*dir.x,r*dir.y,-r*dir.z);
      MtrlEnv.LightRange = r * 32.0f;
      MtrlEnv.CenterX = -1.0f * (x + xs / 2);
      MtrlEnv.CenterY = -1.0f * (y + ys / 2);
      x = -xs / 2;
      y = -ys / 2;
    }
    else
      mtrl = BitmapAlpha ? MtrlTexAlpha : MtrlTex;

    sSystem->SetViewProject(&MtrlEnv);
    mtrl->Set(MtrlEnv);

    handle = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRISTRIP|sGEO_DYNAMIC);
    sSystem->GeoBegin(handle,4,4,(sF32 **)&vp,(void **)&ip);    
    *ip++ = 3;
    *ip++ = 2;
    *ip++ = 1;
    *ip++ = 0;
    if(BitmapTile)
    {
      vp->Init(x-xs  ,y-ys  ,0.5f,~0,-1,-1); vp++;
      vp->Init(x+xs*2,y-ys  ,0.5f,~0, 2,-1); vp++;
      vp->Init(x-xs  ,y+ys*2,0.5f,~0,-1, 2); vp++;
      vp->Init(x+xs*2,y+ys*2,0.5f,~0, 2, 2); vp++;
    }
    else
    {
      vp->Init( 0+x, 0+y,0.5f,~0,0,0); vp++;
      vp->Init(xs+x, 0+y,0.5f,~0,1,0); vp++;
      vp->Init( 0+x,ys+y,0.5f,~0,0,1); vp++;
      vp->Init(xs+x,ys+y,0.5f,~0,1,1); vp++;
    }
    sSystem->GeoEnd(handle);
    sSystem->GeoDraw(handle);
    sSystem->GeoRem(handle);

    if(bm_flat->Format == sTF_Q8W8V8U8)
    {
//      GenOverlayManager->FXQuad(GENOVER_ADDDESTALPHA);
      MtrlEnv.CenterX = MtrlEnv.CenterY = 0.0f;
    }
  }
}

void ViewBitmapWin_::OnKey(sU32 key)
{
  MenuListKey(key);
}

void ViewBitmapWin_::OnDrag(sDragData &dd)
{
  sInt mode;
  sU32 qual;

  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 0;
    mode = 0;
    if(dd.Buttons& 1) mode = 1;   // lmb
    if(dd.Buttons& 2) mode = 2;   // rmb
    if(dd.Buttons& 4) mode = 3;   // mmb
    if(dd.Buttons& 8) mode = 4;   // xmb
    if(dd.Buttons&16) mode = 5;   // ymb
    qual = sSystem->GetKeyboardShiftState();
    if(qual&sKEYQ_CTRL)
      mode |= 16;
    else if(qual&sKEYQ_ALT)
      mode |= 32;
    else if(qual&sKEYQ_SHIFT)
      mode |= 64;

    switch(mode&15)
    {
    case 1:
      DragMode = DMB_LIGHT;
      DragStartX = LightPosX;
      DragStartY = LightPosY;
      break;
    case 2:
      DragMode = DMB_ZOOM;
      DragStartY = BitmapZoom;
      break;
    default:
      DragMode = DMB_SCROLL;
      DragStartX = BitmapX-dd.MouseX;
      DragStartY = BitmapY-dd.MouseY;
      break;
    }
    break;

  case sDD_DRAG:
    switch(DragMode)
    {
    case DMB_SCROLL:   // scroll bitmap
      BitmapX = DragStartX + dd.MouseX;
      BitmapY = DragStartY + dd.MouseY;
      break;
    case DMB_ZOOM:   // zoom bitmap
      BitmapZoom = sRange<sInt>(DragStartY - (-dd.DeltaX-dd.DeltaY)*0.02,8,0);
      break;
    case DMB_LIGHT:  // orbit light
      LightPosX = DragStartX + dd.DeltaX;
      LightPosY = DragStartY + dd.DeltaY;
      break;
    }
    break;

  case sDD_STOP:
    DragMode = 0;
    break;
  }
}

sBool ViewBitmapWin_::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case CMD_VIEWBMP_RESET:
    BitmapX = 10;
    BitmapY = 10;
    BitmapTile = 0;
    BitmapAlpha = 0;
    BitmapZoom = 5;
    LightPosX = 0;
    LightPosY = 0;
    break;
  case CMD_VIEWBMP_TILE:
    BitmapTile = !BitmapTile;
    break;
  case CMD_VIEWBMP_ALPHA:
    BitmapAlpha = !BitmapAlpha;
    break;
  case CMD_VIEWBMP_FULLSIZE:
    FullSize = !FullSize;
    SetFullsize(FullSize);
    break;
  case CMD_VIEWBMP_POPUP:
    PopupMenuList();
    break;
  default:
    return sFALSE;
  }
  return sTRUE;
}

/****************************************************************************/

void ViewBitmapWin_::SetOp(GenOp *op)
{
  ShowOp = op;
}

/****************************************************************************/
