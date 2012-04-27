/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui.hpp"
#include "base/graphics.hpp"

MainWindow *App;

/****************************************************************************/

MainWindow::MainWindow()
{
  App = this;
  Doc = new Document;

  if(sGetShellParameter(0,0))
    Doc->Scan(sGetShellParameter(0,0));
  else
    Doc->Scan(L".");

  Coder = 0;
  Decoder = 0;
  Decoder2 = 2;
  Format = 0;
  DiffMode = 0;
  DiffBoost = 4;

  BrightError[0] = 0;
  LinearError[0] = 0;
  SquareError[0] = 0;
  BrightError[1] = 0;
  LinearError[1] = 0;
  SquareError[1] = 0;
  CompressionTime = 0;

  for(sInt i=0;i<MAX_CODECS;i++)
  {
    if(Doc->Codecs[i])
    {
      if(i!=0)
        CodecString.Add(L"|");
      if(Doc->Codecs[i])
        CodecString.Add(Doc->Codecs[i]->GetName());
      else
        CodecString.Add(L"???");
    }
  }

  ViewWin = new WinView;
  ListWin = new sSingleListWindow<DocImage>(&Doc->Images);
  ListWin->SetArray(&Doc->Images);
  ListWin->AddField(L"Filename",sLWF_SORT,300,sMEMBERPTR(DocImage,Name));
  ListWin->SelectMsg = sMessage(this,&MainWindow::CmdSet);
  GridWin = new sWireGridFrame();

  AddChild(sWire = new sWireMasterWindow);
  sWire->AddWindow(L"main",this);
  ViewWin->InitWire(L"view");
  ListWin->InitWire(L"list");
  GridWin->InitWire(L"para");

  sGridFrameHelper gh(GridWin);
  gh.DoneMsg = sMessage(this,&MainWindow::CmdSet);
  //gh.PushButton(L"Calculate all", gh.DoneMsg);
  gh.Label(L"Coder");
  gh.Choice(&Coder,CodecString)->Style = sChoiceControl::sCBS_DROPDOWN;
  gh.Label(L"Decoder");
  gh.Choice(&Decoder,CodecString)->Style = sChoiceControl::sCBS_DROPDOWN;
  gh.Label(L"Format");
  gh.ControlWidth = 4;
  gh.Choice(&Format,L"DXT1 without alpha|DXT1 with alpha|DXT3|DXT5")->Style = sChoiceControl::sCBS_DROPDOWN;
  gh.ControlWidth = 2;
  gh.Label(L"Difference");
  gh.Flags(&DiffMode,L"Absolute|Relative:*1coder-decoder|decoder-decoder");
  gh.Choice(&DiffBoost,L"x1|x2|x4|x8|x16|x32|x64|x128")->Style = sChoiceControl::sCBS_DROPDOWN;
  gh.Choice(&Decoder2,CodecString)->Style = sChoiceControl::sCBS_DROPDOWN;
  gh.Group(L"Error");
  gh.Label(L"Brightness");
  gh.Float(&App->BrightError[0],-9999,9999,0)->Style |= sSCS_STATIC;
  gh.Float(&App->BrightError[1],-9999,9999,0)->Style |= sSCS_STATIC;
  gh.Label(L"Linear Error");
  gh.Float(&App->LinearError[0],-9999,9999,0)->Style |= sSCS_STATIC;
  gh.Float(&App->LinearError[1],-9999,9999,0)->Style |= sSCS_STATIC;
  gh.Label(L"Square Error");
  gh.Float(&App->SquareError[0],-9999,9999,0)->Style |= sSCS_STATIC;
  gh.Float(&App->SquareError[1],-9999,9999,0)->Style |= sSCS_STATIC;
  gh.Label(L"Compression Time");
  gh.Int(&App->CompressionTime,-9999,9999,0)->Style |= sSCS_STATIC;

  sWire->ProcessFile(L"dxt.wire.txt");
  sWire->ProcessEnd();
}

MainWindow::~MainWindow()
{
}

void MainWindow::Tag()
{
  sWindow::Tag();
  Doc->Need();
  ViewWin->Need();
  ListWin->Need();
}

void MainWindow::CmdSet()
{
  DocImage* selected = ListWin->GetSelected();
  if(selected)
    ViewWin->SetImage(selected);
  else
  {
    sArray<DocImage*> *temp = &Doc->Images;
    sInt count = temp->GetCount();
    sDPrintF(L"%d\n", count);
    App->CompressionTime = 0;
    App->BrightError[0] = 0;
    App->LinearError[0] = 0;
    App->SquareError[0] = 0;
    App->BrightError[1] = 0;
    App->LinearError[1] = 0;
    App->SquareError[1] = 0;
    for(sInt i=0; i<count; i++)
    {
      sDPrintF(L"%d:   ", i);
      ViewWin->SetImage(temp[0][i], true);
    }
    sDPrintF(L"Calculation Complete \n\n\n");
  }
}

/****************************************************************************/

WinView::WinView()
{
  Current = 0;
  Flags = sWF_CLIENTCLIPPING;
  Decoded = new sImage;
  Difference = new sImage;
  AlphaMode = 0;
  Zoom = 1;
}

WinView::~WinView()
{
  delete Decoded;
  delete Difference;
}

void WinView::Tag()
{
  sWindow::Tag();
  Current->Need();
}

void WinView::InitWire(const sChar *name)
{
  sWire->AddWindow(name,this);
  sWire->AddDrag(name,L"Scroll",sMessage(this,&WinView::DragScroll));
  sWire->AddKey(name,L"ToggleAlpha",sMessage(this,&WinView::CmdToggleAlpha));
  sWire->AddKey(name,L"ZoomIn",sMessage(this,&WinView::CmdZoom,1));
  sWire->AddKey(name,L"ZoomOut",sMessage(this,&WinView::CmdZoom,-1));
}

void WinView::SetImage(DocImage *i, sBool loop)
{
  static sInt format[4] = { sTEX_DXT1,sTEX_DXT1A,sTEX_DXT3,sTEX_DXT5 };
  sU8 *a,*b,*d;
  sInt xs,ys;
  sInt diff;
  sImage *Decoded2=0;
  Current = i;
  sF32 BrightError[2], LinearError[2], SquareError[2];

  if(Current)
  {
    xs = Current->Image->SizeX;
    ys = Current->Image->SizeY;

    Decoded->Init(xs,ys);
    Difference->Init(xs,ys);

    Current->Calc(App->Coder,format[App->Format]);

    Doc->Codecs[App->Decoder]->Unpack(Decoded,Current->Dxt[App->Coder],format[App->Format]);

    sVERIFY(Difference->SizeX == Current->Image->SizeX);
    sVERIFY(Decoded->SizeX == Current->Image->SizeX);
    sVERIFY(Difference->SizeY == Current->Image->SizeY);
    sVERIFY(Decoded->SizeY == Current->Image->SizeY);

    BrightError[0] = 0;
    LinearError[0] = 0;
    SquareError[0] = 0;
    BrightError[1] = 0;
    LinearError[1] = 0;
    SquareError[1] = 0;

    a = (sU8 *) Current->Image->Data;
    b = (sU8 *) Decoded->Data;
    d = (sU8 *) Difference->Data;

    if(App->DiffMode&2)
    {
      Decoded2 = new sImage;
      Decoded2->Init(xs,ys);
      Doc->Codecs[App->Decoder2]->Unpack(Decoded2,Current->Dxt[App->Coder],format[App->Format]);
      a = (sU8 *) Decoded2->Data;
    }

    for(sInt i=0;i<xs*ys*4;i+=4)
    {
      diff = a[0]-b[0];
      d[0] = sClamp( (App->DiffMode&1) ? ((diff<<App->DiffBoost)+0x80) : (sAbs(diff)<<App->DiffBoost),0,255);
      BrightError[0] += diff;
      LinearError[0] += sFAbs(diff);
      SquareError[0] += diff*diff;

      diff = a[1]-b[1];
      d[1] = sClamp( (App->DiffMode&1) ? ((diff<<App->DiffBoost)+0x80) : (sAbs(diff)<<App->DiffBoost),0,255);
      BrightError[0] += diff;
      LinearError[0] += sFAbs(diff);
      SquareError[0] += diff*diff;

      diff = a[2]-b[2];
      d[2] = sClamp( (App->DiffMode&1) ? ((diff<<App->DiffBoost)+0x80) : (sAbs(diff)<<App->DiffBoost),0,255);
      BrightError[0] += diff;
      LinearError[0] += sFAbs(diff);
      SquareError[0] += diff*diff;

      diff = a[3]-b[3];
      d[3] = sClamp( (App->DiffMode&1) ? ((diff<<App->DiffBoost)+0x80) : (sAbs(diff)<<App->DiffBoost),0,255);
      BrightError[1] += diff;
      LinearError[1] += sFAbs(diff);
      SquareError[1] += diff*diff;

// this helps testing the DXT1 case      
//      if(a[3]==0 || b[3]==0)
//        d[0]=d[1]=d[2]= ((App->DiffMode&1) ? 0x80 : 0x0);

      a+=4;
      b+=4;
      d+=4;
    }

    BrightError[0] /= xs*ys*4;
    LinearError[0] /= xs*ys*4;
    SquareError[0] = sFSqrt(SquareError[0]/(xs*ys*4));
    BrightError[1] /= xs*ys*4;
    LinearError[1] /= xs*ys*4;
    SquareError[1] = sFSqrt(SquareError[1]/(xs*ys*4));

  }

  Update();
  if(loop)
  {
    App->BrightError[0] += BrightError[0];
    App->LinearError[0] += LinearError[0];
    App->SquareError[0] += SquareError[0];
    App->BrightError[1] += BrightError[1];
    App->LinearError[1] += LinearError[1];
    App->SquareError[1] += SquareError[1];
    App->CompressionTime += Current->CompressionTime[App->Coder][App->Format];
     sDPrintF(L"Time: %d   Bright: %f   Linear: %f   Square: %f\n", App->CompressionTime, App->BrightError[0], App->LinearError[0], App->SquareError[0]);
  }
  else
  {
    App->BrightError[0] = BrightError[0];
    App->LinearError[0] = LinearError[0];
    App->SquareError[0] = SquareError[0];
    App->BrightError[1] = BrightError[1];
    App->LinearError[1] = LinearError[1];
    App->SquareError[1] = SquareError[1];
    App->CompressionTime = Current->CompressionTime[App->Coder][App->Format];
  }
     
  sGui->Notify(App->BrightError);
  sGui->Notify(App->LinearError);
  sGui->Notify(App->SquareError);
  delete Decoded2;
}

void WinView::CmdToggleAlpha()
{
  AlphaMode = !AlphaMode;
  Update();
}

void WinView::CmdZoom(sDInt dir)
{
  Zoom = sClamp<sInt>(Zoom+dir,1,4);
  Update();
}

void WinView::OnCalcSize()
{
  if(Current && Current->Image)
  {
    ReqSizeX = (Current->Image->SizeX + 10) * 3 * Zoom;
    ReqSizeY = (Current->Image->SizeY + 10) * Zoom;
  }
  else
    ReqSizeX = ReqSizeY = 0;
}

void WinView::OnPaint2D()
{
  sRect r,h;
  sInt Split1 = (Current && Current->Image) ? Current->Image->SizeX*Zoom + 10: 0;
  sInt Split2 = Split1*2;
  if(Current && Current->Image)
  {
    r = Client;
    r.x1 = Client.x0 + Split1;
    h.x0 = r.x0 + 10;
    h.y0 = r.y0 + 10;
    h.x1 = h.x0 + Current->Image->SizeX*Zoom;
    h.y1 = h.y0 + Current->Image->SizeY*Zoom;
    sRectHole2D(r,h,sGC_BACK);
    Blit(Current->Image,h);

    r = Client;
    r.x0 = Client.x0 + Split1;
    r.x1 = Client.x0 + Split2;
    h.x0 = r.x0 + 10;
    h.y0 = r.y0 + 10;
    h.x1 = h.x0 + Current->Image->SizeX*Zoom;
    h.y1 = h.y0 + Current->Image->SizeY*Zoom;
    sRectHole2D(r,h,sGC_BACK);
    Blit(Decoded,h);

    sSetColor2D(0,App->DiffMode ? 0xff808080 : 0xff000000);
    r = Client;
    r.x0 = Client.x0 + Split2;
    h.x0 = r.x0 + 10;
    h.y0 = r.y0 + 10;
    h.x1 = h.x0 + Current->Image->SizeX*Zoom;
    h.y1 = h.y0 + Current->Image->SizeY*Zoom;
    sRectHole2D(r,h,sGC_BACK);
    Blit(Difference,h);
  }
  else
  {
    sRect2D(Client,sGC_BACK);
  }
}

void WinView::Blit(const sImage *img,const sRect &r)
{
  if(!AlphaMode)
  {
    if(r.SizeX() == img->SizeX && r.SizeY() == img->SizeY)
      sBlit2D(img->Data,img->SizeX,r);
    else
      sStretch2D(img->Data,img->SizeX,sRect(img->SizeX,img->SizeY),r);
  }
  else
  {
    sU32 *p = new sU32[img->SizeX*img->SizeY];
    sU32 *pp = p;
    const sU32 *data = img->Data;
    sInt width = img->SizeX;
    sInt c;

    for(sInt y=0;y<img->SizeY;y++)
    {
      for(sInt x=0;x<img->SizeX;x++)
      {
        c =  data[width*y+x]>>24;
        *pp++ = 0xff000000 | (c<<16) | (c<<8) | c;
      }
    }

    if(r.SizeX() == img->SizeX && r.SizeY() == img->SizeY)
      sBlit2D(p,img->SizeX,r);
    else
      sStretch2D(p,img->SizeX,sRect(img->SizeX,img->SizeY),r);
    delete[] p;
  }
}

/****************************************************************************/


