/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "chaos_font.hpp"
#include "chaos_font_ops.hpp"

/****************************************************************************/

ChaosFont::ChaosFont()
{
  Type = ChaosFontType;
  Font = 0;
  Image = 0;
  Geo = 0;
  Tex = 0;
  Mtrl = 0;
  CursorX = 0;
  CursorY = 0;
  PrintBuffer.HintSize(256);
}

ChaosFont::~ChaosFont()
{
  delete Font;
  delete Image;
  delete Mtrl;
  delete Geo;
  delete Tex;
}

void ChaosFont::CopyFrom(ChaosFont *src)
{
  if(src->Image)
  {
    delete Image;
    Image = new sImage;
    Image->Copy(src->Image);
  }
  Letters = src->Letters;
  Safety = src->Safety;
  Outline = src->Outline;
  Baseline = src->Baseline;
  Height = src->Height;
  LineFeed = src->LineFeed;
  CursorX = src->CursorX;
  CursorY = src->CursorY;
}

void ChaosFont::CopyTo(sImage *dest)
{
  if(Image)
    dest->Copy(Image);
  else
  {
    dest->Init(16,16);
    dest->Fill(0xffff0000);
  }
}

void ChaosFont::CopyTo(sImageI16 *dest)
{
  if(Image)
    dest->CopyFrom(Image);
  else
  {
    dest->Init(16,16);
    dest->Fill(0xffff);
  }
}

void ChaosFont::CopyTo(sImageData *dest,sInt format)
{
  if(!Image)
  {
    Image = new sImage;
    Image->Init(16,16);
    Image->Fill(0xffff0000);
  }
  dest->Init2(format|sTEX_2D,0,Image->SizeX,Image->SizeY,1);
  dest->ConvertFrom(Image);
}

template <class streamer> void ChaosFont::Serialize_(streamer &s)
{    
  ChaosFontLetter *let;

  sInt version = s.Header(sSerId::Wz4ChaosFont,1);
  if(version)
  {
    // important constants

    s | Baseline | Height | LineFeed;

    // most likely useless constants

    s | Safety | Outline | CursorX | CursorY;

    // letters

    s.Array(Letters);
    sFORALL(Letters,let)
    {
      s.U16(let->Char);
      s.U16(let->StartX);
      s.U16(let->StartY);
      s.S16(let->Cell);
      s.S16(let->Pre);
      s.S16(let->Post);
    }

    // image

    if(!Image)
      Image = new sImage();
    Image->Serialize(s);

    // done

    s.Footer();
  }
}

void ChaosFont::Serialize(sWriter &s) { Serialize_(s); }
void ChaosFont::Serialize(sReader &s) { Serialize_(s); }

/****************************************************************************/

void ChaosFont::Init(sInt sizex,sInt sizey)
{
  delete Image;
  Image = new sImage(sizex,sizey);
}

void ChaosFont::InitFont(const sChar *name,sInt height,sInt width,sInt style,sInt safety,sInt outline)
{
  delete Font;

  sInt flags = 0;
  if(style & 1) flags |= sF2C_BOLD;
  if(style & 2) flags |= sF2C_ITALICS;
  if(style & 4) flags |= sF2C_UNDERLINE;
  if(style & 8) flags |= sF2C_STRIKEOUT;
  if(style & 16) flags |= sF2C_SYMBOLS;
  Font = new sFont2D(name,height,flags,width);
  Safety = safety;
  Outline = outline;
  Height = height;//Font->GetHeight();
  Baseline = Font->GetBaseline();
  LineFeed = 0;

  CursorX = Outline+Safety;
  CursorY = Outline+Safety;

  sRender2DBegin(Image->SizeX,Image->SizeY);
  sRect2D(0,0,Image->SizeX,Image->SizeY,sGC_BLACK);
  Font->SetColor(sGC_WHITE,sGC_BLACK);
}

void ChaosFont::Letter(sInt c)
{
  sChar text[2];
  ChaosFontLetter *letter = Letters.AddMany(1);

  sFont2D::sLetterDimensions dim = Font->sGetLetterDimensions(c);
  letter->Char = c;
  letter->Pre = dim.Pre;
  letter->Post = dim.Post;
  letter->Cell = dim.Cell;
  if(CursorX + letter->Cell + Safety + Outline > Image->SizeX)
  {
    CursorY += Height + Safety + Outline*2;
    CursorX = Outline + Safety;
  }
  letter->StartX = CursorX;
  letter->StartY = CursorY;

  text[0] = c;
  text[1] = 0;

  Font->Print(0,CursorX-letter->Pre,CursorY,text,1);
  CursorX += letter->Cell + Outline*2 + Safety;
}

void ChaosFont::Symbol(sInt c,const sRect &r,const sImage *img,sInt yo)
{
  ChaosFontLetter *letter = Letters.AddMany(1);

  letter->Char = c;
  letter->Pre = 0;
  letter->Post = 0;
  letter->Cell = r.SizeX();
  if(CursorX + letter->Cell + Safety + Outline > Image->SizeX)
  {
    CursorY += Height + Safety + Outline*2;
    CursorX = Outline + Safety;
  }
  letter->StartX = CursorX;
  letter->StartY = CursorY;

  Image->BlitFrom(img,r.x0,r.y0,CursorX,CursorY+Baseline-r.SizeY()+yo,r.SizeX(),r.SizeY());


  CursorX += r.SizeX() + Outline*2 + Safety;
}

void ChaosFont::Finish()
{
  sRender2DGet(Image->Data);
  for(sInt i=0;i<Image->SizeX*Image->SizeY;i++)
    Image->Data[i] = (Image->Data[i]<<24)|(Image->Data[i]&0xffffff);
  sRender2DEnd();
}

/****************************************************************************/

void ChaosFont::PreparePrint()
{
  if(!Tex || !Mtrl || !Geo)
  {
    delete Tex;
    delete Mtrl;
    delete Geo;

    sInt xs = Image->SizeX;
    sInt ys = Image->SizeY;
    for(sInt y=ys-4;y<ys;y++)
      for(sInt x=xs-4;x<xs;x++)
        Image->Data[y*xs+x] = ~0;

    Tex = sLoadTexture2D(Image,sTEX_2D|sTEX_ARGB8888);

    Mtrl = new sSimpleMaterial;
    Mtrl->Texture[0] = Tex;
    Mtrl->BlendColor = sMB_PMALPHA;
    Mtrl->Flags = sMTRL_CULLOFF|sMTRL_ZOFF;
    Mtrl->TFlags[0] = sMTF_CLAMP|sMTF_LEVEL2;
    Mtrl->Prepare(sVertexFormatSingle);
    
    Geo = new sGeometry(sGF_QUADLIST,sVertexFormatSingle);
  }
}

void ChaosFont::Letter(sVertexSingle *vp,sInt c,sF32 &x,sF32 y,sF32 s,sU32 col,sF32 sign)
{
  ChaosFontLetter *let;
  sFRect r;
  sFRect uv;
  sF32 tx = 1.0f/Image->SizeX;
  sF32 ty = 1.0f/Image->SizeY;
  sF32 ox = -tx*0.5f;
  sF32 oy = -ty*0.5f;

  let = sFind(Letters,&ChaosFontLetter::Char,c);
  if(let==0)
  let = &Letters[0];

  x += (let->Pre-1)*s;
  r.x0 = x;
  r.y0 = y;
  x += (let->Cell+2)*s;
  r.x1 = x;
  r.y1 = y + sign*Height*s;
  x += (let->Post-1)*s;
  uv.x0 = ox+(let->StartX-1)*tx;
  uv.y0 = oy+let->StartY*ty;
  uv.x1 = ox+(let->StartX + let->Cell+2)*tx;
  uv.y1 = oy+(let->StartY + Height)*ty;

  vp[0].Init(r.x0,r.y0,0,col,uv.x0,uv.y0);
  vp[1].Init(r.x1,r.y0,0,col,uv.x1,uv.y0);
  vp[2].Init(r.x1,r.y1,0,col,uv.x1,uv.y1);
  vp[3].Init(r.x0,r.y1,0,col,uv.x0,uv.y1);
}

void ChaosFont::Print(const sViewport &view,const sChar *text,sU32 col,sF32 s)
{
  // geo

  sVertexSingle *vp;
  sInt vc = 0;
  if(Letters.GetCount()==0)
    return;
  sF32 x = 0;
  sF32 y = 0;

  Geo->BeginLoadVB(sGetStringLen(text)*4,sGD_FRAME,&vp);

  while(*text)
  {
    if(*text=='\n')
    {
      y -= (Height+LineFeed)*s;
      x = 0;
    }
    else
    {
      Letter(vp,*text,x,y,s,col,-1);

      vc += 4;
      vp += 4;
    }
    text++;
  }

//  sDPrintF(L"vc %d\n",vc);
  Geo->EndLoadVB(vc,0);

  // draw it...

  sCBuffer<sSimpleMaterialPara> cb;
  cb.Data->Set(view);

  Mtrl->Set(&cb);
  Geo->Draw();
}

/****************************************************************************/

void ChaosFont::Print(sInt x,sInt y,const sChar *text,sU32 col,sF32 scale,sInt len)
{
  if(this==0) return;

  if(len==-1)
    len = sGetStringLen(text);
  ChaosFontPrint *p = PrintBuffer.AddMany(1);

  p->Rect.Init(x,y,x,y);
  p->Color = col;
  p->Text = text;
  p->TextLen = len;
  p->Scale = scale;
}


void ChaosFont::PrintC(sInt x,sInt y,const sChar *text,sU32 col,sF32 scale,sInt len)
{
  if(this==0) return;

  ChaosFontLetter *let;
  sInt offset = 0;
  if(len==-1)
    len = sGetStringLen(text);
  for(sInt i=0;i<len;i++)
  {
    let = sFind(Letters,&ChaosFontLetter::Char,text[i]);
    if(let==0)
      let = &Letters[0];
    offset += let->Pre+let->Cell+let->Post;
  }

  Print(x-sInt(offset*scale/2/Height),y,text,col,scale,len);
}

void ChaosFont::PrintR(sInt x0,sInt x1,sInt y,const sChar *text,sU32 col,sF32 scale)
{
  if(this==0) return;
  sInt max = sInt((x1-x0)/scale*Height);
  ChaosFontLetter *let;

  while(*text)
  {
    while(*text=='\n')
    {
      text++;
      y += scale*0.5f;
    }
    while(*text==' ') text++;

    const sChar *save = text+1;
    const sChar *start = text;

    sInt width = 0;

    while(*text && *text!='\n' && width<max)
    {
      if(*text==' ')
        save = text;
      if(*text==0x3001 || *text==0x3002 || *text==0x30fb)  // japanese
        save = text+1;

      let = sFind(Letters,&ChaosFontLetter::Char,*text);
      if(let==0)
        let = &Letters[0];

      width += let->Pre+let->Cell+let->Post;
      text++;
    }
    if(*text==0 || *text=='\n')
      save = text;
    Print(x0,y,start,col,scale,save-start);
    y += scale;
    text = save;
    if(*text=='\n')
      text++;
  }
}

/*
void ChaosFont::PrintR(sInt x0,sInt x1,sInt y,const sChar *text,sU32 col,sF32 scale)
{
  if(this==0) return;
  sInt max = x1-x0;
  ChaosFontLetter *let;

  while(*text)
  {
    while(*text=='\n')
    {
      text++;
      y += scale*0.5f;
    }
    while(*text==' ') text++;

    const sChar *save = text+1;
    const sChar *start = text;

    sInt width = 0;

    while(*text && *text!='\n' && width<max)
    {
      if(*text==' ')
        save = text;

      let = sFind(Letters,&ChaosFontLetter::Char,*text);
      if(let==0)
        let = &Letters[0];

      width += let->Pre+let->Cell+let->Post;
      text++;
    }
    if(*text==0 || *text=='\n')
      save = text;
    PrintC(x0,y,start,col,scale,save-start);
    y += scale;
    text = save;
    if(*text=='\n') 
      text++;
  }
}
*/

void ChaosFont::Rect(const sRect &r,sU32 col)
{
  if(this==0) return;

  ChaosFontPrint *p = PrintBuffer.AddMany(1);

  p->Rect = r;
  p->Color = col;
  p->Text = 0;
  p->Scale = 1;
}

void ChaosFont::Paint()
{
  ChaosFontPrint *p;
  sInt max = 0;
  sVertexSingle *vp;
  sInt vc = 0;
  if(Letters.GetCount()==0)
    return;
  if(PrintBuffer.GetCount()==0)
    return;
  sF32 x = 0;
  sF32 y = 0;
  sF32 ur = 1.0 - 2.0f/Image->SizeX;
  sF32 vr = 1.0 - 2.0f/Image->SizeY;


  sFORALL(PrintBuffer,p)
  {
    if(p->Text)
      max += p->TextLen;
    else
      max ++;
  }
  if(max==0)
    return;


  Geo->BeginLoadVB(max*4,sGD_STREAM,&vp);

  sFORALL(PrintBuffer,p)
  {
    if(p->Text)
    {
      x = p->Rect.x0;
      y = p->Rect.y0;
      for(sInt i=0;i<p->TextLen;i++)
      {
        Letter(vp,p->Text[i],x,y,p->Scale/Height,p->Color,1);
        vp+=4;
        vc+=4;
      }
    }
    else
    {
      vp[0].Init(p->Rect.x0,p->Rect.y0,0,p->Color,ur,vr);
      vp[1].Init(p->Rect.x1,p->Rect.y0,0,p->Color,ur,vr);
      vp[2].Init(p->Rect.x1,p->Rect.y1,0,p->Color,ur,vr);
      vp[3].Init(p->Rect.x0,p->Rect.y1,0,p->Color,ur,vr);
      vp+=4;
      vc+=4;
    }
  }

  sVERIFY(vc>0);
  Geo->EndLoadVB(vc,0);

  PrintBuffer.Clear();

  // draw it...

  if(vc>0)
  {
    sViewport view;
    view.Orthogonal = sVO_PIXELS;
    view.Prepare();
    sCBuffer<sSimpleMaterialPara> cb;
    cb.Data->Set(view);

    Mtrl->Set(&cb);
    Geo->Draw();
  }
}

/****************************************************************************/
