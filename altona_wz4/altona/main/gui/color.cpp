/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/color.hpp"
#include "gui/frames.hpp"
#include "util/image.hpp"
#include "base/serialize.hpp"

sF32 sColorPickerWindow::PaletteColors[32][4] = 
{
  { 0.0f,0.0f,0.0f,1.0f },
  { 1.0f,0.0f,0.0f,1.0f },
  { 0.0f,1.0f,0.0f,1.0f },
  { 1.0f,1.0f,0.0f,1.0f },
  { 0.0f,0.0f,1.0f,1.0f },
  { 1.0f,0.0f,1.0f,1.0f },
  { 0.0f,1.0f,1.0f,1.0f },
  { 1.0f,1.0f,1.0f,1.0f },

  { 0.0f,0.0f,0.0f,1.0f },
  { 0.5f,0.0f,0.0f,1.0f },
  { 0.0f,0.5f,0.0f,1.0f },
  { 0.5f,0.5f,0.0f,1.0f },
  { 0.0f,0.0f,0.5f,1.0f },
  { 0.5f,0.0f,0.5f,1.0f },
  { 0.0f,0.5f,0.5f,1.0f },
  { 0.5f,0.5f,0.5f,1.0f },

  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },

  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
  { 0.0f,0.0f,0.0f,1.0f },
};

/****************************************************************************/
/***                                                                      ***/
/***   Gradient                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sColorGradient::sColorGradient()
{
  Clear();
}

sColorGradient::~sColorGradient()
{
}

void sColorGradient::Clear()
{
  Gamma = 1;
  Flags = 0;
  
  Keys.Clear();
  AddKey(0,0xff000000);
  AddKey(1,0xffffffff);
}

template <class streamer> void sColorGradient::Serialize_(streamer &s)
{
  s.Header(sSerId::sColorGradient,1);
  s.Array(Keys);
  s | Flags | Gamma;
  s.Skip(4);

  sColorGradientKey *key;
  sFORALL(Keys,key)
    s | key->Time | key->Color;

  s.Footer();
}

void sColorGradient::Serialize(sReader &stream) { Serialize_(stream); }
void sColorGradient::Serialize(sWriter &stream) { Serialize_(stream); }

/****************************************************************************/

void sColorGradient::AddKey(sF32 time,sU32 col)
{
  sColorGradientKey *key = Keys.AddMany(1);
  key->Time = time;
  key->Color.InitColor(col);
}

void sColorGradient::AddKey(sF32 time,const sVector4 &col)
{
  sColorGradientKey *key = Keys.AddMany(1);
  key->Time = time;
  key->Color = col;
}

void sColorGradient::Sort()
{
  sSortUp(Keys,&sColorGradientKey::Time);
}

void sColorGradient::Calc(sF32 time,sVector4 &col)
{
  if(Gamma<0.0001f)
    col = Keys[Keys.GetCount()-1].Color;
  else
    CalcUnwarped(sFPow(time,Gamma),col);
}

void sColorGradient::CalcUnwarped(sF32 time,sVector4 &col)
{
  sInt max = Keys.GetCount();
  sInt pos;
  sVERIFY(max>=2);

  if(time<=Keys[0].Time)
  {
    col = Keys[0].Color;
  }
  else if(time>=Keys[max-1].Time)
  {
    col = Keys[max-1].Color;
  }
  else 
  {
    for(pos=0;pos<max-1;pos++)
      if(time<=Keys[pos+1].Time)
        break; 
    sVERIFY(pos<max-1);

    sVector4 dif(Keys[pos+1].Color-Keys[pos].Color);
    sVector4 d0,d1;
    sF32 tdif = Keys[pos+1].Time-Keys[pos].Time;
    sF32 t = (time-Keys[pos].Time)/tdif;

    if(pos==0)
    {
      if(Flags & 1)
        d0.Init(0,0,0,0);
      else
        d0 = dif;
    }
    else
    {
      d0 = tdif * (Keys[pos+1].Color - Keys[pos-1].Color)/(Keys[pos+1].Time-Keys[pos-1].Time);
    }

    if(pos==max-2)
    {
      if(Flags & 2)
        d1.Init(0,0,0,0);
      else
        d1 = dif;
    }
    else
    {
      d1 = tdif * (Keys[pos+2].Color - Keys[pos+0].Color)/(Keys[pos+2].Time-Keys[pos+0].Time);
    }

    col = (((d0 + d1 - 2*dif) * t + 3*dif - 2*d0 - d1)*t + d0)*t + Keys[pos].Color;

    col.x = sClamp(col.x,0.0f,1.0f);
    col.y = sClamp(col.y,0.0f,1.0f);
    col.z = sClamp(col.z,0.0f,1.0f);
    col.w = sClamp(col.w,0.0f,1.0f);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Control                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sColorGradientControl::sColorGradientControl(sColorGradient *g,sBool alpha)
{
  Gradient  = g;
  AlphaEnable = alpha;
  Bar = new sImage();
  Bar->Init(1,1);
  AddNotify(*Gradient);
}

sColorGradientControl::~sColorGradientControl()
{
  delete Bar;
}

void sColorGradientControl::Tag()
{
  Gradient->Need();
}

void sColorGradientControl::OnPaint2D()
{
  sRect cl = Client;

  sGui->RectHL(cl,sGC_DRAW,sGC_DRAW);
  cl.Extend(-1);

  sInt xs = cl.SizeX();
  if(xs!=Bar->SizeX)
    Bar->Init(xs,1);
  sU32 *d = Bar->Data;
  sVector4 col;
  for(sInt i=0;i<xs;i++)
  {
    Gradient->Calc(sF32(i)/(xs-1),col);
    *d++ = col.GetColor();
  }

  sRect r(0,0,xs,1);
  
  sStretch2D(Bar->Data,xs,r,cl);
}

void sColorGradientControl::OnDrag(const sWindowDrag &dd)
{
  if(dd.Mode==sDD_START && dd.Buttons==1)
  {
    sColorPickerWindow *cp = new sColorPickerWindow;
    cp->Init(Gradient,AlphaEnable);
    cp->Flags = sWF_AUTOKILL;
    cp->ChangeMsg = sMessage(this,&sColorGradientControl::Update);
    sGui->AddFloatingWindow(cp,L"gradient");
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Gui                                                                ***/
/***                                                                      ***/
/****************************************************************************/

sF32 Mod1(sF32 f)
{

  f = sFMod(f,1);
  if(f<0) f+=1;
  return f;
}


/****************************************************************************/

sColorPickerWindow::sColorPickerWindow()
{
  FPtr = 0;
  UPtr = 0;
  Red = 0;
  Green = 0;
  Blue = 0;
  Hue = 0;
  Sat = 0;
  Value = 0;

  PickImage = new sImage;
  PickImage->Init(1,1);
  PickSat = -1;
  PickHue = -1;

  GradientImage= new sImage;
  GradientImage->Init(1,1);
  GradientChanged = 1;

  WarpImage= new sImage;
  WarpImage->Init(1,1);

  Gradient = 0;
  DragMode = 0;
  TagRef = 0;
  DragKey = 0;

  AddChild(Grid = new sGridFrame());
}

sColorPickerWindow::~sColorPickerWindow()
{
  delete PickImage;
  delete GradientImage;
  delete WarpImage;
}

void sColorPickerWindow::Tag()
{
  sWindow::Tag();
  Grid->Need();
  TagRef->Need();
}

/****************************************************************************/

void sColorPickerWindow::Init(sF32 *f,sObject *tagref,sBool alpha)
{
  TagRef = tagref;
  FPtr = f;
  Red   = FPtr[0];
  Green = FPtr[1];
  Blue  = FPtr[2];
  if(alpha)
  {
    AlphaEnable = 1;
    Alpha = FPtr[3];
  }
  ChangeRGB();
  MakeGui();
}

void sColorPickerWindow::Init(sU32 *u,sObject *tagref,sBool alpha)
{
  TagRef = tagref;
  UPtr = (sU8 *)u;
  Red   = UPtr[2]/255.0f;
  Green = UPtr[1]/255.0f;
  Blue  = UPtr[0]/255.0f;
  if(alpha)
  {
    AlphaEnable = 1;
    Alpha = UPtr[3]/255.0f;
  }
  ChangeRGB();
  MakeGui();
}

void sColorPickerWindow::Init(sColorGradient *grad,sBool alpha)
{
  sVERIFY(grad->Keys.GetCount()>=2);
  Gradient = grad;

  if(0)   // test serialisation
  {
    sSaveObject(L"c:/temp/test.gradient",Gradient);
    Gradient->Clear();

    if(sLoadObject(L"c:/temp/test.gradient",Gradient))
      sFatal(L"readtest failed");
  }

  DragKey = 1;
  Init(&grad->Keys[DragKey].Color.x,0,alpha);
}

void sColorPickerWindow::MakeGui()
{
  sGridFrameHelper gh(Grid);
  Grid->Columns = 4;
  gh.LabelWidth = 2;
  const sF32 step = 0.002f;

  if(Gradient)
  {
    gh.ChangeMsg = sMessage(this,&sColorPickerWindow::ChangeGradient);
    gh.DoneMsg = sMessage(this,&sColorPickerWindow::ChangeGradient);
    gh.Group(L"Gradient");
    gh.Label(L"Gamma Warp");
    gh.Float(&Gradient->Gamma,0,16,0.01f);
    gh.Label(L"Start");
    gh.Choice(&Gradient->Flags,L"*0free|linear");
    gh.Label(L"End");
    gh.Choice(&Gradient->Flags,L"*1free|linear");
    gh.DoneMsg = sMessage();
  }

  gh.ChangeMsg = sMessage(this,&sColorPickerWindow::ChangeRGB);
  gh.Group(L"RGB");
  gh.Label(L"Red");         gh.Float(&Red  ,0,1,step);
  gh.Label(L"Green");       gh.Float(&Green,0,1,step);
  gh.Label(L"Blue");        gh.Float(&Blue ,0,1,step);
  if(AlphaEnable)
  {
    gh.ChangeMsg = sMessage(this,&sColorPickerWindow::ChangeRGB);
    gh.Label(L"Alpha");       gh.Float(&Alpha,0,1,step);
  }
  gh.Group(L"HSV");
  gh.ChangeMsg = sMessage(this,&sColorPickerWindow::ChangeHSV);
  gh.Label(L"Hue");         gh.Float(&Hue  ,-360,720,1.0f);
  gh.Label(L"Saturation");  gh.Float(&Sat  ,0,1,step);
  gh.Label(L"Value");       gh.Float(&Value,0,1,step);

  if(!Gradient)
  {
    gh.Group(L"Shift-Click in palette to set");
  }
}

void sColorPickerWindow::ChangeGradient()
{
  GradientChanged = 1;
  ChangeMsg.Post();
  Update();
}

void sColorPickerWindow::ChangeRGB()
{
	sF32 min,max,delta;

  min = sMin(Red,sMin(Green,Blue));
	max = sMax(Red,sMax(Green,Blue));
  Value = max;

	delta = max - min;
	if(max!=0 && delta!=0)
  {
		Sat = delta / max;

	  if(Red == max)
		  Hue= (Green - Blue) / delta;
	  else if(Green == max)
		  Hue= 2 + (Blue - Red) / delta;
	  else
		  Hue = 4 + (Red - Green) / delta;
  }
	else 
  {
		Sat = 0;
		Hue = 0;
	}

	Hue *= 60;
	if(Hue < 0)
		Hue += 360;

  sGui->Notify(&Hue,3*sizeof(sF32));
  GradientChanged = 1;

  ChangeMsg.Post();
  UpdateOutput();
}

void sColorPickerWindow::ChangeHSV()
{
	sInt i;
	sF32 f, p, q, t;
  sF32 *r = &Red;
  sF32 *g = &Green;
  sF32 *b = &Blue;

	if(Sat == 0) 
  {
		*r = *g = *b = Value;
	}
  else
  {
	  sF32 h = Mod1(Hue/360.0f)*6;
	  i = sInt(sFFloor(h));
	  f = h - i;
	  p = Value * (1 - Sat);
	  q = Value * (1 - Sat * f );
	  t = Value * (1 - Sat * ( 1 - f ) );

	  switch( i ) 
    {
	  case 0:
		  *r = Value;
		  *g = t;
		  *b = p;
		  break;
	  case 1:
		  *r = q;
		  *g = Value;
		  *b = p;
		  break;
    case 2:
		  *r = p;
		  *g = Value;
		  *b = t;
		  break;
	  case 3:
		  *r = p;
		  *g = q;
		  *b = Value;
		  break;
	  case 4:
		  *r = t;
		  *g = p;
		  *b = Value;
		  break;
    default:
		  *r = Value;
		  *g = p;
		  *b = q;
		  break;
    }
	}

  sGui->Notify(&Red,3*sizeof(sF32));
  GradientChanged = 1;

  ChangeMsg.Post();
  UpdateOutput();
}

void IntHSV(sU8 *dest,sInt h,sInt s,sInt v)
{
	sInt i;
	sInt f, p, q, t;
  sU8 *r = dest+2;
  sU8 *g = dest+1;
  sU8 *b = dest+0;

	if(s == 0) 
  {
		*r = *g = *b = v;
	}
  else
  {
	  i = h>>8;
	  f = h&255;
	  p = (v * (256 - s))/256;
	  q = (v * (0x10000 - s * f ))/(0x10000);
	  t = (v * (0x10000 - s * ( 0x100 - f ) ))/(0x10000);

	  switch( i ) 
    {
	  case 0:
		  *r = v;
		  *g = t;
		  *b = p;
		  break;
	  case 1:
		  *r = q;
		  *g = v;
		  *b = p;
		  break;
    case 2:
		  *r = p;
		  *g = v;
		  *b = t;
		  break;
	  case 3:
		  *r = p;
		  *g = q;
		  *b = v;
		  break;
	  case 4:
		  *r = t;
		  *g = p;
		  *b = v;
		  break;
    default:
		  *r = v;
		  *g = p;
		  *b = q;
		  break;
    }
	}
}


void sColorPickerWindow::UpdateOutput()
{
  if(FPtr)
  {
    sGui->Notify(FPtr,sizeof(sF32)*(3+AlphaEnable));
    FPtr[0] = sClamp(Red  ,0.0f,1.0f);
    FPtr[1] = sClamp(Green,0.0f,1.0f);
    FPtr[2] = sClamp(Blue ,0.0f,1.0f);
    if(AlphaEnable)
      FPtr[3] = sClamp(Alpha,0.0f,1.0f);
  }
  if(UPtr)
  {
    sGui->Notify(UPtr,sizeof(sU8)*(3+AlphaEnable));
    UPtr[2] = sClamp(sInt(Red  *255),0,255);
    UPtr[1] = sClamp(sInt(Green*255),0,255);
    UPtr[0] = sClamp(sInt(Blue *255),0,255);
    if(AlphaEnable)
      UPtr[3] = sClamp(sInt(Alpha*255),0,255);
  }
  Update();
}

/****************************************************************************/

void sColorPickerWindow::OnPaint2D()
{

  // prepare color picker image

  if(PickImage->SizeX!=PickRect.SizeX() || PickImage->SizeY!=PickRect.SizeY())
  {
    PickImage->Init(PickRect.SizeX(),PickRect.SizeY());
    PickSat = -1;
  }
  if(PickSat!=Sat || PickHue!=Hue)
  {
    sU8 *d=(sU8 *)PickImage->Data;
    sInt v=sClamp(sInt(Value*255),0,255);
    for(sInt s=0;s<PickImage->SizeY;s++)
    {
      for(sInt h=0;h<PickImage->SizeX;h++)
      {
        IntHSV(d,h*6*256/PickImage->SizeX,s*256/PickImage->SizeY,v);
        d[3]=255;
        d+=4;
      }
    }
  }

  // prapare gradient image

  if(GradientImage->SizeX!=BarRect.SizeX())
  {
    WarpImage->Init(BarRect.SizeX(),1);
    WarpImage->Fill(0x0ffff0000);
    GradientImage->Init(BarRect.SizeX(),1);
    GradientImage->Fill(0x0ffff0000);
    GradientChanged = 1;
  }
  if(GradientChanged && Gradient && GradientImage->SizeX>1)
  {
    GradientChanged = 0;
    Gradient->Sort();
    sVector4 col;
    sU32 *d=GradientImage->Data;
    sU32 *w=WarpImage->Data;
    for(sInt x=0;x<GradientImage->SizeX;x++)
    {
      Gradient->CalcUnwarped(sF32(x)/(GradientImage->SizeX-1),col);
      *d++ =col.GetColor();
      Gradient->Calc(sF32(x)/(GradientImage->SizeX-1),col);
      *w++ =col.GetColor();
    }
  }

  // paint rects

  sInt cr = sClamp(sInt(Red  *255),0,255);
  sInt cg = sClamp(sInt(Green*255),0,255);
  sInt cb = sClamp(sInt(Blue *255),0,255);

  sRect2D(Client.x0,BarRect.y1,Client.x1,BarRect.y1+1,sGC_DRAW);
  sRect2D(PickRect.x0-1,PickRect.y0,PickRect.x0,PickRect.y1,sGC_DRAW);

  if(!Gradient)
  {
    sSetColor2D(0,0xff000000|(cr<<16)|(cg<<8)|cb);
    sRect2D(BarRect,0);

    sRect r(PaletteRect);
    sInt h = 20;
    sInt w = r.SizeX()+1;
    sRect2D(PaletteRect,sGC_RED);
    for(sInt i=0;i<4;i++)
      sRect2D(r.x0,r.y0+i*h,r.x1,r.y0+i*h+1,sGC_DRAW);
    for(sInt i=1;i<8;i++)
      sRect2D(r.x0+w*i/8-1,r.y0,r.x0+w*i/8,r.y1,sGC_DRAW);

    for(sInt y=0;y<4;y++)
    {
      for(sInt x=0;x<8;x++)
      {
        sInt i = y*8+x;
        sVector4 v(PaletteColors[i][0],PaletteColors[i][1],PaletteColors[i][2],0);
        sU32 col = v.GetColor();
        sSetColor2D(sGC_MAX,col);
        PaletteBoxes[i].Init(r.x0+w*x/8,r.y0+1+y*h,r.x0+w*(x+1)/8-1,r.y0+(y+1)*h);
        sRect2D(PaletteBoxes[i],sGC_MAX);
      }
    }
  }
  else
  {
    sRect r;
    sColorGradientKey *key;

    sRect2D(Client.x0,WarpRect.y1,Client.x1,WarpRect.y1+1,sGC_DRAW);

    r.Init(0,0,WarpImage->SizeX,1);
    sStretch2D(WarpImage->Data,WarpImage->SizeX,r,WarpRect);
    sStretch2D(GradientImage->Data,GradientImage->SizeX,r,BarRect);

    r = BarRect;
    r.y0 = r.y1-6;
    sRect2D(r,sGC_BACK);

    sClipPush();
    sClipRect(BarRect);
    sFORALL(Gradient->Keys,key)
    {
      sInt x = r.x0 + sInt(sClamp(key->Time,0.0f,1.0f)*(r.SizeX()-1));
      sInt y = r.y0;
      sInt col = (_i==DragKey) ? sGC_SELECT : sGC_DRAW;

      sRect2D(x,BarRect.y0,x+1,y,col);
      sLine2D(x  ,y  ,x-4,y+4,col);
      sLine2D(x-4,y+4,x+4,y+4,col);
      sLine2D(x+4,y+4,x  ,y  ,col);
    }
    sClipPop();
  }

  // paint colour picker handle

  sInt x = sInt(Mod1(Hue/360)*PickRect.SizeX())+PickRect.x0;
  sInt y = sInt(Sat*PickRect.SizeY())+PickRect.y0;
  sInt n = 3;
  sInt color = Value>0.5 ? sGC_BLACK : sGC_WHITE;

  sClipPush();
  sClipRect(PickRect);
  sBlit2D(PickImage->Data,PickImage->SizeX,PickRect);

  sLine2D(x-n,y  ,x  ,y-n,color);
  sLine2D(x  ,y-n,x+n,y  ,color);
  sLine2D(x+n,y  ,x  ,y+n,color);
  sLine2D(x  ,y+n,x-n,y  ,color);

  sClipPop();
}

void sColorPickerWindow::OnCalcSize()
{
  ReqSizeX = 320;
  ReqSizeY = Grid->DecoratedSizeY + 20;
  if(Gradient)
    ReqSizeY += 20;
  else
    ReqSizeY += 20*4;
}

void sColorPickerWindow::OnLayout()
{
  sInt x = Client.x0+150;
  sInt y = Client.y0+20;//sMax(Client.y0,Client.y1 - Grid->DecoratedSizeY);
  if(Gradient)
    y += 20;

  PaletteRect = WarpRect = BarRect = Client;
  PickRect = Client;
  Grid->Outer = Client;

  Grid->Outer.y0 = y;
  BarRect.y1 = y-1;
  Grid->Outer.x1 = x-1;
  PickRect.x0 = x;
  PickRect.y0 = y;

  if(Gradient)
  {
    WarpRect.y1 = BarRect.y0 = BarRect.y0+14;
    WarpRect.y1--;
    PaletteRect.y0 = PaletteRect.y1;
  }
  else
  {
    PaletteRect.y0 = PaletteRect.y1-20*4;
    Grid->Outer.y1 = PaletteRect.y0;
    PickRect.y1 = PaletteRect.y0;
    PaletteRect.y0;
  }
}

void sColorPickerWindow::SelectKey(sInt nr)
{
  sColorGradientKey *key;
  DragKey = nr;
  key = &Gradient->Keys[nr];
  DragStart = key->Time;
  FPtr = &key->Color.x;
  Red = key->Color.x;
  Green = key->Color.y;
  Blue = key->Color.z;
  Alpha = key->Color.w;
  ChangeRGB();
  Update();
}

void sColorPickerWindow::OnDrag(const sWindowDrag &dd)
{
  sU32 qual = sGetKeyQualifier();
  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 0;
    if(PickRect.Hit(dd.MouseX,dd.MouseY))
      DragMode = 1;
    if(Gradient && BarRect.Hit(dd.MouseX,dd.MouseY))
    {
      if(dd.Buttons == 1)
      {
        sColorGradientKey *key;
        sInt best = 5;
        sInt bestindex = -1;

        sFORALL(Gradient->Keys,key)
        {
          sInt dist = sAbs(Client.x0 + sInt((key->Time*(Client.SizeX()-1))) - dd.MouseX);
          if(dist<best)
          {
            bestindex = _i;
            best = dist;
          }
        }
        if(bestindex!=-1)
        {
          SelectKey(bestindex);
          DragMode = 2;
        }
      }
      else if(dd.Buttons == 2)
      {
        sColorGradientKey *key;
        sF32 time = sF32(dd.MouseX-Client.x0)/(Client.SizeX()-1);
        sVector4 col;
        Gradient->Sort();
        Gradient->Calc(time,col);
        Gradient->AddKey(time,col);
        Gradient->Sort();
        GradientChanged = 1;
        SelectKey(0);
        sGui->Notify(*Gradient);
        sFORALL(Gradient->Keys,key)
        {
          if(key->Time==time)
          {
            SelectKey(_i);
            DragMode = 2;
          }
        }
      }
    }
    if(PaletteRect.Hit(dd.MouseX,dd.MouseY))
    {
      for(sInt i=0;i<32;i++)
      {
        if(PaletteBoxes[i].Hit(dd.MouseX,dd.MouseY))
        {
          if(dd.Buttons==1 && !(qual&sKEYQ_SHIFT))
          {
            Red   = PaletteColors[i][0];
            Green = PaletteColors[i][1];
            Blue  = PaletteColors[i][2];
            Alpha = PaletteColors[i][3];
            ChangeRGB();
          }
          if(dd.Buttons==1 && (qual&sKEYQ_SHIFT))
          {
            PaletteColors[i][0] = Red;
            PaletteColors[i][1] = Green;
            PaletteColors[i][2] = Blue;
            PaletteColors[i][3] = Alpha;
          }
          Update();
        }
      }
    }
    break;
  case sDD_DRAG:
    if(DragMode==1)
    {
      Hue = sFMod((dd.MouseX-PickRect.x0)*360.0f/PickRect.SizeX(),360);
      Sat = sClamp<sF32>((dd.MouseY-PickRect.y0)*1.0f/PickRect.SizeY(),0.0f,1.0f);
      sGui->Notify(Hue);
      sGui->Notify(Sat);
      ChangeHSV();
    }
    if(Gradient && DragMode==2)
    {
      sF32 time = sClamp(DragStart + sF32(dd.DeltaX)/(Client.SizeX()-1),0.0f,1.0f);
      if(Gradient->Keys[DragKey].Time!=time)
      {
        Gradient->Keys[DragKey].Time = time;      
        GradientChanged = 1;
        Update();
        sGui->Notify(*Gradient);
      }
    }
    break;
  case sDD_STOP:
    DragMode = 0;
    break;
  }
}

sBool sColorPickerWindow::OnKey(sU32 key)
{
  switch(key & (sKEYQ_BREAK|sKEYQ_MASK))
  {
  case sKEY_ESCAPE:
    Parent->Childs.Rem(this);
    sGui->Layout();
    break;
  case sKEY_DELETE:
  case sKEY_BACKSPACE:
    CmdDelete();
  default:
    return 0;
  }
  return 1;
}

void sColorPickerWindow::CmdDelete()
{
  if(Gradient && Gradient->Keys.GetCount()>2)
  {
    Gradient->Keys.RemAtOrder(DragKey);
    GradientChanged = 1;
    DragKey = 0;
    Update();
    sGui->Notify(*Gradient);
  }
}

/****************************************************************************/

