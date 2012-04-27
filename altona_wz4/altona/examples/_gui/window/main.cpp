/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "gui/textwindow.hpp"
#include "util/image.hpp"
#include "util/rasterizer.hpp"

/****************************************************************************/

void MyWindow::OnPaint2D()
{
  //sRect2D(Client,sGC_BACK);
  //sLine2D(Client.x0,Client.y0,Client.x1-1,Client.y1-1,sGC_DRAW);
  //sLine2D(Client.x0,Client.y1-1,Client.x1-1,Client.y0,sGC_DRAW);
  
  sF32 width = 1.5f;

  sInt sx = Client.SizeX();
  sInt sy = Client.SizeY();

  sImage img(sx,sy);
  sVectorRasterizer raster(&img);

  img.Fill(sGetColor2D(sGC_BACK));

  // thick line 1
  raster.MoveTo(sVector2(0,0));
  raster.EdgeTo(sVector2(width,0));
  raster.EdgeTo(sVector2(sx,sy-width));
  raster.EdgeTo(sVector2(sx,sy));
  raster.EdgeTo(sVector2(sx-width,sy));
  raster.EdgeTo(sVector2(0,width));
  raster.EdgeTo(sVector2(0,0));

  // thick line 2
  raster.MoveTo(sVector2(sx,0));
  raster.EdgeTo(sVector2(sx,width));
  raster.EdgeTo(sVector2(width,sy));
  raster.EdgeTo(sVector2(0,sy));
  raster.EdgeTo(sVector2(0,sy-width));
  raster.EdgeTo(sVector2(sx-width,0));
  raster.EdgeTo(sVector2(sx,0));

  raster.RasterizeAll(sGetColor2D(sGC_DRAW),sVRFC_NONZERO_WINDING);
  sBlit2D(img.Data,img.SizeX,Client);
}

/****************************************************************************/

MyParentWin::MyParentWin()
{
  sVSplitFrame *v;
  sToolBorder *tb;

  for(sInt i=0;i<4;i++)
  {
    v = new sVSplitFrame;
    for(sInt j=0;j<4;j++)
    {
      sWindow *w;
      if(i==2 && j==2)
      {
        sTextWindow *t = new sTextWindow;
        t->SetText(&Text);
        Text.PrintF(L"lk jsalk dsf fasd fdsfdkl asjd\n123456789\nabcdefghijklmnop\nnlhdlhefhdsjf hdkfh sldjhf");
        t->TextFlags |= sF2P_MULTILINE;
        w = t;
      }
      else
      {
        w = new MyWindow;
      }
      w->AddBorder(new sFocusBorder);
      v->AddChild(w);
    }
    v->AddBorder(new sFocusBorder);
    AddChild(v);
  }
  tb = new sToolBorder;
  tb->AddMenu(L"File",sMessage(this,&MyParentWin::MakeMenu));
  tb->AddMenu(L"View",sMessage());
  AddBorder(tb);
}

void MyParentWin::MakeMenu()
{
  sMenuFrame *mf;
  mf = new sMenuFrame(this);
  mf->AddItem(L"first",sMessage(),0);
  mf->AddItem(L"second",sMessage(),0);
  mf->AddSpacer();
  mf->AddItem(L"last",sMessage(),0);
  sGui->AddPulldownWindow(mf);
}


void sMain()
{
  sInit(sISF_2D,800,600);
  sInitGui();
  sGui->AddBackWindow(new MyParentWin);
}

/****************************************************************************/
