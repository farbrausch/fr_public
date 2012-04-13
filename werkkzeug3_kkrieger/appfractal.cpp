// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "appfractal.hpp"
#include "material11.hpp"

/****************************************************************************/

sFractalApp::sFractalApp()
{
  sTexInfo ti;
  sInt i,cr,cg,cb;

  Bitmap = new sBitmap;
  Bitmap->Init(512,512);
  ti.Init(Bitmap);
  Texture = sSystem->AddTexture(ti);

  PaintHandle = sPainter->LoadMaterial(new sSimpleMaterial(Texture,0,0,0));
  sVERIFY(PaintHandle!=sINVALID);

  CalcNum = 0;

  Flags |= sGWF_SETSIZE;
  History[0].Init(-2,-2,2,2);
  HistCount=0;

  for(i=0;i<64;i++)
  {
    cr = (sFSin( (i+  0  )*(sPI2F/64) )+1)*127;
    cg = (sFSin( (i+ 64/3)*(sPI2F/64) )+1)*127;
    cb = (sFSin( (i+128/3)*(sPI2F/64) )+1)*127;
    Palette[i] = 0xff000000 | (cr<<16) | (cg<<8) | cb;
  }
}

sFractalApp::~sFractalApp()
{
  if(Texture)
    sSystem->RemTexture(Texture);
}

void sFractalApp::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(Bitmap);
}

void sFractalApp::OnCalcSize()
{
  SizeX = Bitmap->XSize;
  SizeY = Bitmap->XSize;
}

void sFractalApp::OnPaint()
{
  sRect r;
  sPainter->Paint(PaintHandle,Client,0xffffffff);
  Flags &= ~sGWF_SETSIZE;
  if(DragMode)
  {
    sPainter->Flush();
    r = DragRect;
    r.Sort();
    sGui->RectHL(r,1,0xffffffff,0xffffffff);
  }
}

void sFractalApp::OnKey(sU32 key)
{
  switch(key&0x8001ffff)
  {
  case sKEY_APPCLOSE:
    Parent->RemChild(this);
    break;
  case sKEY_BACKSPACE:
  case sKEY_DELETE:
    if(HistCount>0)
    {
      HistCount--;
      CalcNum = 0;
    }
    break;
  }
}

void sFractalApp::OnFrame()
{
  sInt time;

  if(CalcNum<256)
  {
    time = sSystem->GetTime();
    while(sSystem->GetTime()<time+100 && CalcNum<256)
      Calc();
    sSystem->UpdateTexture(Texture,Bitmap);
  }
}

void sFractalApp::OnDrag(sDragData &dd)
{
  sFRect fr;

  switch(dd.Mode)
  {
  case sDD_START:
    DragRect.x0 = dd.MouseX;
    DragRect.y0 = dd.MouseY;
    DragRect.x1 = DragRect.x0;
    DragRect.y1 = DragRect.y0;
    DragMode = 1;
    break;
  case sDD_DRAG:
    DragRect.x1 = DragRect.x0+(dd.DeltaX+dd.DeltaY)/2;
    DragRect.y1 = DragRect.y0+(dd.DeltaX+dd.DeltaY)/2;
    break;
  case sDD_STOP:
    DragMode = 0;
    if(Client.x0<Client.x1 && Client.y0<Client.y1)
    {
      if(HistCount>=256)
      {
        sCopyMem(History+1,History+17,sizeof(sFRect)*239);
        HistCount-=16;
      }
      DragRect.Sort();
      fr = History[HistCount++];
      History[HistCount].x0 = fr.x0 + (fr.x1-fr.x0)*(DragRect.x0-Client.x0)/(Client.x1-Client.x0);
      History[HistCount].y0 = fr.y0 + (fr.y1-fr.y0)*(DragRect.y0-Client.y0)/(Client.y1-Client.y0);
      History[HistCount].x1 = fr.x0 + (fr.x1-fr.x0)*(DragRect.x1-Client.x0)/(Client.x1-Client.x0);
      History[HistCount].y1 = fr.y0 + (fr.y1-fr.y0)*(DragRect.y1-Client.y0)/(Client.y1-Client.y0);
      CalcNum = 0;
    }
    break;
  }
}

void sFractalApp::Calc()
{
  sRect r;
  sFRect fr;
  sInt x,y,i;
  sF64 fx,fy,cx,cy,ox;
  sInt bpr;

  if(CalcNum<256)
  {
    r.x0 = (CalcNum&15)*Bitmap->XSize/16;
    r.y0 = ((CalcNum&240)>>4)*Bitmap->YSize/16;
    r.x1 = r.x0+Bitmap->XSize/16;
    r.y1 = r.y0+Bitmap->YSize/16;
    bpr = Bitmap->XSize;
    fr = History[HistCount];

    for(y=r.y0;y<r.y1;y++)
    {
      for(x=r.x0;x<r.x1;x++)
      {
        fx = fr.x0+(fr.x1-fr.x0)*x/Bitmap->XSize;
        fy = fr.y0+(fr.y1-fr.y0)*y/Bitmap->YSize;
        cx = 0;
        cy = 0;
        i = 0;
        while(cx*cx+cy*cy<4 && i<256)
        {
          i++;
          ox = cx;
          cx = cx*cx-cy*cy+fx;
          cy = 2*ox*cy+fy;
        }

        if(i>255)
          Bitmap->Data[y*bpr+x] = 0xff000000;
        else
          Bitmap->Data[y*bpr+x] = Palette[i&63];
      }
    }
    CalcNum++;
  }
}


/****************************************************************************/

