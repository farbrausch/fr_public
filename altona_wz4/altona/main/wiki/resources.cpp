/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "resources.hpp"
#include "base/system.hpp"
#include "util/image.hpp"

sResourceManager_ *sResourceManager;

/****************************************************************************/

void sExitResourceManager()
{
  sDelete(sResourceManager);
}

void sAddResourceManager()
{
  if(!sResourceManager)
  {
    sResourceManager = new sResourceManager_;
    sAddSubsystem(L"sResourceManager",0x40,0,sExitResourceManager);
  }
}

/****************************************************************************/

sResourceManager_::sResourceManager_()
{
  for(sInt i=0;i<sRFF_MAX;i++)
    SetLogFont(i,L"Arial",16);

  SetLogFont(sRFF_FIXED,L"Courier New",17);
  SetLogFont(sRFF_SYMBOL,L"Webdings",16);
}

sResourceManager_::~sResourceManager_()
{
  Flush();
}

void sResourceManager_::Flush()
{
  for(sInt i=0;i<Images.GetCount();)
  {
    sImageResource *r = Images[i];
    if(r->RefCount==0)
      delete r;
    else
      i++;
  }
  for(sInt i=0;i<Fonts.GetCount();)
  {
    sFontResource *r = Fonts[i];
    if(r->RefCount==0)
      delete r;
    else
      i++;
  }
  for(sInt i=0;i<Texts.GetCount();)
  {
    sTextResource *r = Texts[i];
    if(r->RefCount==0)
      delete r;
    else
      i++;
  }
}

void sResourceManager_::ClearTemp()
{
  sImageResource *ir;
  sFontResource *fr;
  sTextResource *tr;

  sFORALL(Images,ir)
    ir->Temp = 0;
  sFORALL(Fonts,fr)
    fr->Temp = 0;
  sFORALL(Texts,tr)
    tr->Temp = 0;
}

/****************************************************************************/

sFontResource *sResourceManager_::NewFont(const sChar *name,sInt size,sInt style)
{
  sFontResource *f;
  sFORALL(Fonts,f)
  {
    if(sCmpStringI(f->Name,name)==0 && f->Size==size && f->Style==style)
    {
      f->AddRef();
      return f;
    }
  }

  f = new sFontResource;
  f->Font = new sFont2D(name,size,style);
  f->Name = name;
  f->Size = size;
  f->Height = f->Font->GetHeight();
  f->Style = style;
  f->LogSize = sRFS_NORMAL;

  sDPrintF(L"flush resources %d img %d fonts %d text\n",Images.GetCount(),Fonts.GetCount(),Texts.GetCount());
  return f;
}

sFontResource *sResourceManager_::NewLogFont(sInt family,sInt logsize,sInt style)
{
  const sChar *name = FontFamilies[family].Name;
  sInt size = FontFamilies[family].Sizes[logsize];
  sFontResource *f = NewFont(name,size,style);
  f->LogSize = logsize;
  return f;
}

void sResourceManager_::SetLogFont(sInt family,const sChar *name,sInt basesize)
{
  FontFamilies[family].Name = name;
  FontFamilies[family].Sizes[0] = sInt(basesize*0.70f+0.5f); 
  FontFamilies[family].Sizes[1] = sInt(basesize*0.80f+0.5f); 
  FontFamilies[family].Sizes[2] = basesize; 
  FontFamilies[family].Sizes[3] = sInt(basesize*1.20f+0.5f); 
  FontFamilies[family].Sizes[4] = sInt(basesize*1.40f+0.5f); 
  FontFamilies[family].Sizes[5] = sInt(basesize*1.80f+0.5f); 
  FontFamilies[family].Sizes[6] = sInt(basesize*2.50f+0.5f); 
}

/****************************************************************************/

sImageResource *sResourceManager_::NewImage(const sChar *name)
{
  sImageResource *ir;

  sFORALL(Images,ir)
  {
    if(sCmpStringP(name,ir->Name)==0)
    {
      ir->AddRef();
      return ir;
    }
  }

  sImage *img = new sImage;
  if(!img->Load(name))
  {
    img->Init(16,16);
    img->Fill(0x00ff0000);
  }
  ir = new sImageResource;
  ir->Name = name;
  ir->Image = new sImage2D(img->SizeX,img->SizeY,img->Data);
  delete img;

  return ir;
}

/****************************************************************************/

sTextResource *sResourceManager_::NewText(const sChar *name)
{
  sTextResource *tr;

  sFORALL(Texts,tr)
  {
    if(sCmpStringP(name,tr->Name)==0)
    {
      tr->AddRef();
      return tr;
    }
  }

  const sChar *text = sLoadText(name);
  if(!text)
  {
    sChar *t = new sChar[1];
    *t = 0;
    text = t;
  }
  tr = new sTextResource;
  tr->Name = name;
  tr->Text = text;

  return tr;
}

/****************************************************************************/
