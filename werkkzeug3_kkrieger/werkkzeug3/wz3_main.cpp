// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_start.hpp"
#include "_startdx.hpp"
#include "_util.hpp"
#include "_gui.hpp"

#include "rtmanager.hpp"

#include "appbrowser.hpp"
#include "apptext.hpp"
#include "werkkzeug.hpp"
#include "genoverlay.hpp"
#include "geneffect.hpp"
#include "engine.hpp"

#include <stdio.h>
#include "_bsplines.hpp"

/****************************************************************************/
/****************************************************************************/

sGuiManager *sGui;
sGuiPainter *sPainter;
sDiskItem *sDiskRoot;

class BSplineTestWin : public sGuiWindow
{
  struct Key
  {
    sF32 t;
    sF32 v;
  };

  static const sInt nSamples = 129;

  sToolBorder *Border;
  sBSpline Spline;
  sCurveSamplePoint TargetFunction[nSamples];
  sF32 DerivPoint;

public:
  BSplineTestWin()
    : Spline(3)
  {
    AddTitle("B-Spline-Test");

#if 0
    static Key keys[] = {
      0.0f, 0.0f,
      0.0f, 0.0f,
      0.0f, 0.0f,
      0.0f, 0.0f,
      0.25f, 0.0f,
      0.375f, 0.0f,
      0.5f, 0.0f,
      0.625f, 0.0f,
      0.75f, 0.0f,
      1.0f, 0.0f,
      1.0f, 0.0f,
      1.0f, 0.0f,
      1.0f, 0.0f,
    };
    static const sInt nKeys = sizeof(keys)/sizeof(keys[0]);

    Spline.Knots.Resize(nKeys);
    Spline.Values.Resize(nKeys);

    Flags |= sGWF_LAYOUT;
    Border = new sToolBorder;
    AddBorder(Border);

    for(sInt i=0;i<nKeys;i++)
    {
      Spline.Knots[i] = keys[i].t;
      Spline.Values[i] = keys[i].v;

      sControl *con = new sControl;
      con->EditFloat(0,&Spline.Values[i],"");
      con->InitNum(-1.0f,1.0f,0.001f,0.0f);
      con->Style |= sCS_SMALLWIDTH;
      con->Position.Init(0,0,96,16);
      Border->AddChild(con);
    }
#endif

    // Try to perform a least-squares fit to a simple target function
    for(sInt i=0;i<nSamples;i++)
    {
      sF32 t = 1.0f * i / (nSamples - 1);
      TargetFunction[i].Time = t;
      TargetFunction[i].Value = sin(t * sPI2F) * 0.5f + 0.125f * (i / (nSamples / 4));
      TargetFunction[i].DerivLeft = sPI2F * cos(t * sPI2F) * 0.5f;
      TargetFunction[i].DerivRight = TargetFunction[i].DerivLeft;
      TargetFunction[i].Flags = 3;
    }

    sTickTimer timer;
    timer.Start();
    Spline.FitToCurve(TargetFunction,nSamples,0.1f,0.001f);
    timer.Stop();

    sF64 time = timer.GetTotal() / (2400.0 * 1000.0);
    sDPrintF("%f ms\n",time);

    DerivPoint = 0.5f;
  }

  void OnDrag(sDragData &dd)
  {
    if(dd.Buttons & sDDB_LEFT)
      DerivPoint = 1.0f * (dd.MouseX - Client.x0) / Client.XSize();
  }

  void OnPaint()
  {
    // clear background
    sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BACK]);

    // draw the target function
    sInt x0,y0,x1,y1;

    x0 = y0 = x1 = y1 = 0;
    for(sInt t=0;t<nSamples;t++)
    {
      sF32 tm = TargetFunction[t].Time;

      x0 = x1;
      y0 = y1;
      x1 = tm * (Client.XSize() - 1) + Client.x0;
      y1 = (1.0f - TargetFunction[t].Value) * 0.5f * Client.YSize() + Client.y0;

      if(t)
        sPainter->Line(x0,y0,x1,y1,0xff00ff00);
    }

    // draw the spline
    x0 = y0 = x1 = y1 = 0;
    sF32 error = 0.0f;
    
    for(sInt t=0;t<nSamples;t++)
    {
      sF32 tm = TargetFunction[t].Time;

      x0 = x1;
      y0 = y1;
      x1 = tm * (Client.XSize() - 1) + Client.x0;
      y1 = (1.0f - Spline.Evaluate(tm)) * 0.5f * Client.YSize() + Client.y0;
      
      error = sMax<sF32>(error,TargetFunction[t].Value - Spline.Evaluate(tm));

      if(t)
        sPainter->Line(x0,y0,x1,y1,0xffff0000);
    }

    sInt fs = sPainter->GetHeight(sGui->PropFont);

    sChar buffer[128];
    sSPrintF(buffer,sizeof(buffer),"%d knots, max error %f (%d pixels)\n",Spline.Knots.Count,error,sInt(error*0.5f*Client.YSize()));
    sPainter->Print(sGui->PropFont,Client.x0+3,Client.y0,buffer,0xff000000);

    sSPrintF(buffer,sizeof(buffer),"derivative at %f is %f\n",DerivPoint,Spline.EvaluateDeriv(DerivPoint));
    sPainter->Print(sGui->PropFont,Client.x0+3,Client.y0+fs+2,buffer,0xff000000);
  }
};

/****************************************************************************/
/****************************************************************************/

void InitGui()
{
  sOverlappedFrame *ov;
  sGuiWindow *win;
  sInt i;
  sScreenInfo si;

  for(i=0;i<sGUI_MAXROOTS;i++)
  {
    if(sSystem->GetScreenInfo(i,si))
    {
      ov = new sOverlappedFrame;
      ov->RightClickCmd = 4;
      sGui->SetRoot(ov,i);
    }
  }

  sGui->SetStyle(sGui->GetStyle()|sGS_SMALLFONTS);

  win = new WerkkzeugApp;
  win->Position.Init(0,0,300,200);
  sGui->AddApp(win);

  sGui->GetRoot()->GetChild(0)->FindTitle()->Maximise(1);

  /*win = new BSplineTestWin;
  win->Position.Init(0,0,640,480);
  sGui->AddApp(win);*/
}

/****************************************************************************/
/****************************************************************************/


sBool sAppHandler(sInt code,sDInt value)
{
  switch(code)
  {
  case sAPPCODE_CONFIG:
    sSetConfig(sSF_DIRECT3D,0,0);
    break;

  case sAPPCODE_INIT:
    sInitPerlin();
    GenOverlayInit();
    MemManagerInit();

    RenderTargetManager = new RenderTargetManager_;
    Engine = new Engine_;

    sDiskRoot  = new sDIRoot;
    sBroker->AddRoot(sDiskRoot);
    sPainter = new sGuiPainter;
    sPainter->Init();
    sBroker->AddRoot(sPainter);
    sGui = new sGuiManager;
    sBroker->AddRoot(sGui);
    InitGui();
    sBroker->Free();
    break;

  case sAPPCODE_EXIT:
    delete Engine;
    delete RenderTargetManager;

    GenOverlayExit();
    sSystem->SetSoundHandler(0,0,0);
    sBroker->RemRoot(sPainter);
    sBroker->RemRoot(sGui); sGui = 0;
    sBroker->RemRoot(sDiskRoot);
    sBroker->Free();
    MemManagerExit(); // must be last because garbagecollected objects may rely on it
    FreeParticles();
    break;

  case sAPPCODE_KEY:
    switch(value)
    {
    case sKEY_ESCAPE|sKEYQ_SHIFTL:
    case sKEY_ESCAPE|sKEYQ_SHIFTR:
    case sKEY_CLOSE:
      sSystem->Exit();
      break;
    default:
      sGui->OnKey(value);
      break;
    }
    break;
    
  case sAPPCODE_PAINT:
    {
      sInt i;
      sBool ok;

      sGui->OnFrame();
      sGui->OnPaint();
      ok = sFALSE;
      for(i=0;i<sGUI_MAXROOTS;i++)
        if(sGui->GetRoot(i) && sGui->GetRoot(i)->GetChildCount()!=0)
          ok = sTRUE;
      if(!ok) 
        sSystem->Exit();
    }
    break;

  case sAPPCODE_TICK:
    sGui->OnTick(value);
    break;

  /*case sAPPCODE_LAYOUT:
    break;*/

  case sAPPCODE_DEBUGPRINT:
    if(sGui)
      return sGui->OnDebugPrint((sChar *)value);
    return sFALSE;

  default:
    return sFALSE;
  }
  return sTRUE;
}

/****************************************************************************/


