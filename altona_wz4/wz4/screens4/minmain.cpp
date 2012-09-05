/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "util/painter.hpp"
#include "base/windows.hpp"
#include "util/shaders.hpp"

#include "config.hpp"
#include "vorbisplayer.hpp"
#include "network.hpp"
#include "playlists.hpp"

/****************************************************************************/

static Config *MyConfig;
static sScreenMode ScreenMode;

/****************************************************************************/
/****************************************************************************/

static void SetupScreenMode(Config::Resolution resolution, sBool fullscreen)
{
  sClear(ScreenMode);
  ScreenMode.Aspect = (float)resolution.Width/(float)resolution.Height;
  ScreenMode.Display = -1;
  if (fullscreen)
  {
    ScreenMode.Flags |= sSM_FULLSCREEN;
  }
  ScreenMode.Frequency = resolution.RefreshRate;
  ScreenMode.MultiLevel = -1;
  ScreenMode.OverMultiLevel = -1;
  ScreenMode.OverX = ScreenMode.ScreenX = MyConfig->DefaultResolution.Width;
  ScreenMode.OverY = ScreenMode.ScreenY = MyConfig->DefaultResolution.Height;
}

/****************************************************************************/
/****************************************************************************/

sINITMEM(0,0);

class MyApp : public sApp
{
public:

  sBool HasMusic;

  sInt StartTime;
  sInt Time;
  sPainter *Painter;
  sBool Fps;
  sInt FramesRendered;

  RPCServer *Server;
  WebServer *Web;

  PlaylistMgr PlMgr;

  sRandomMT Rnd;

  sTexture2D *Tex1, *Tex2;
  sSimpleMaterial *Mtrl1, *Mtrl2, *BarMtrl;
  sF32 TransTime, TransDurMS;
  sF32 SlideTime;
  sInt Started;

  bMusicPlayer SoundPlayer;

  SiegmeisterData *SiegData, *NextSiegData;

  MyApp()
  {
    StartTime=0;
    Time=0;
    Fps = 0;
    Painter = new sPainter();
    FramesRendered = 0;
    TransTime = -1.0f;
    SiegData = 0;
    NextSiegData = 0;

    Server = new RPCServer(PlMgr, MyConfig->Port);
    Web = new WebServer(PlMgr, MyConfig->HttpPort);
    Rnd.Seed(sGetRandomSeed());

    Tex1 = new sTexture2D(8,8,sTEX_2D|sTEX_ARGB8888);
    Tex2 = new sTexture2D(8,8,sTEX_2D|sTEX_ARGB8888);

    Mtrl1 = new sSimpleMaterial;
    Mtrl1->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
    Mtrl1->Texture[0] = Tex1;
    Mtrl1->TFlags[0] = sMTF_UV0|sMTF_CLAMP;
    Mtrl1->BlendColor = sMB_PMALPHA;
    Mtrl1->Prepare(sVertexFormatSingle);

    Mtrl2 = new sSimpleMaterial;
    Mtrl2->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
    Mtrl2->Texture[0] = Tex2;
    Mtrl2->TFlags[0] = sMTF_UV0|sMTF_CLAMP;
    Mtrl2->BlendColor = sMB_PMALPHA;
    Mtrl2->Prepare(sVertexFormatSingle);

    BarMtrl = new sSimpleMaterial;
    BarMtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
    BarMtrl->BlendColor = sMB_PMALPHA;
    BarMtrl->Prepare(sVertexFormatSingle);
  }

  ~MyApp()
  {
    sDelete(Mtrl1);
    sDelete(Mtrl2);
    sDelete(BarMtrl);
    sDelete(Tex1);
    sDelete(Tex2);
    sDelete(Server);
    sDelete(Web);
    sDelete(Painter);
    sDelete(SiegData);
    sDelete(NextSiegData);
    sDelete(MyConfig);
  }

  void MakeNextSlide(sImageData &idata)
  {
    Tex2->ReInit(idata.SizeX,idata.SizeY,idata.Format);
    Tex2->LoadAllMipmaps(idata.Data);
    
  }

  void SetTransition(sF32 duration=1.0f)
  {
    TransTime = 0.0f;
    TransDurMS = 1000*duration;
    Started = 0;
  }

  void EndTransition()
  {
    sSwap(Tex1, Tex2);     
    sSwap(Mtrl1, Mtrl2);
    TransTime = -1.0f;
    SlideTime = 0;
    Started = 0;
    sDelete(SiegData);
    SiegData = NextSiegData;
    NextSiegData = 0;
  }


  void DrawSlide(sMaterial *mtrl, sU32 c1, SiegmeisterData *sieg)
  {
    // make screen rectangle and viewport
    sInt sx,sy;
    sGetRendertargetSize(sx,sy);
    sF32 scrnaspect=sGetRendertargetAspect();
    sViewport view;
    view.SetTargetCurrent();
    view.Orthogonal = sVO_PIXELS;
    view.Prepare();

    // init geo
    sGeometry geo;
    geo.Init(sGF_QUADLIST|sGF_INDEXOFF,sVertexFormatSingle);
    sVertexSingle *vp;
    sRect rect;

    sU32 c2 = c1 & 0xff000000;

    // fill geo  (with explicit letterbox/pillarbox for blending)
    sF32 arr=scrnaspect*sF32(mtrl->Texture[0]->SizeY)/sF32(mtrl->Texture[0]->SizeX);
    sF32 w=0.5f, h=0.5f;
    if (arr>1) 
    {
      w/=arr; 
      rect.Init(sx*(0.5f-w)+0.5f,sy*(0.5f-h)+0.5f,sx*(0.5f+w)+0.5f,sy*(0.5f+h)+0.5f);

      geo.BeginLoadVB(12,sGD_STREAM,&vp);
      (*vp++).Init(      0,rect.y0,0.5,c2,0,0);
      (*vp++).Init(rect.x0,rect.y0,0.5,c2,0,0);
      (*vp++).Init(rect.x0,rect.y1,0.5,c2,0,0);
      (*vp++).Init(      0,rect.y1,0.5,c2,0,0);

      (*vp++).Init(rect.x0,rect.y0,0.5,c1,0,0);
      (*vp++).Init(rect.x1,rect.y0,0.5,c1,1,0);
      (*vp++).Init(rect.x1,rect.y1,0.5,c1,1,1);
      (*vp++).Init(rect.x0,rect.y1,0.5,c1,0,1);

      (*vp++).Init(     sx,rect.y0,0.5,c2,0,0);
      (*vp++).Init(rect.x1,rect.y0,0.5,c2,0,0);
      (*vp++).Init(rect.x1,rect.y1,0.5,c2,0,0);
      (*vp++).Init(     sx,rect.y1,0.5,c2,0,0);
      geo.EndLoadVB();
    }
    else
    {
      h*=arr;
      rect.Init(sx*(0.5f-w)+0.5f,sy*(0.5f-h)+0.5f,sx*(0.5f+w)+0.5f,sy*(0.5f+h)+0.5f);

      geo.BeginLoadVB(12,sGD_STREAM,&vp);
      (*vp++).Init(rect.x0,      0,0.5,c2,0,0);
      (*vp++).Init(rect.x1,      0,0.5,c2,0,0);
      (*vp++).Init(rect.x1,rect.y0,0.5,c2,0,0);
      (*vp++).Init(rect.x0,rect.y0,0.5,c2,0,0);

      (*vp++).Init(rect.x0,rect.y0,0.5,c1,0,0);
      (*vp++).Init(rect.x1,rect.y0,0.5,c1,1,0);
      (*vp++).Init(rect.x1,rect.y1,0.5,c1,1,1);
      (*vp++).Init(rect.x0,rect.y1,0.5,c1,0,1);

      (*vp++).Init(rect.x0,     sy,0.5,c2,0,0);
      (*vp++).Init(rect.x1,     sy,0.5,c2,0,0);
      (*vp++).Init(rect.x1,rect.y1,0.5,c2,0,0);
      (*vp++).Init(rect.x0,rect.y1,0.5,c2,0,0);

      geo.EndLoadVB();
    }
     
    // set material
    sCBuffer<sSimpleMaterialPara> cb;
    cb.Data->Set(view);
    mtrl->Set(&cb);

    // draw
    geo.Draw();

    // siegmeister bars
    if (sieg)
    {
      sF32 time = sieg->Winners ? 1 : (Started ? sClamp(SlideTime/MyConfig->BarAnimTime,0.0f,1.0f) : 0);
      sInt bars = sieg->BarPositions.GetCount();

      geo.BeginLoadVB(4*bars,sGD_STREAM,&vp);
      for (sInt i=0; i<bars; i++)
      {
        sFRect brect = sieg->BarPositions[i];
        sU32 color = (sieg->Winners && i<3) ? sColorFade(sieg->BarBlinkColor1,sieg->BarBlinkColor2,0.5*sFCos(2*SlideTime)+0.5) : sieg->BarColor;
        color = sMulColor(sColorFade(0,color,sieg->BarAlpha),c1);
        
        sF32 phase = sFMod(brect.x1*10000,sPI2F);
        sF32 t = time + 0.25*MyConfig->BarAnimSpread *sFSin(phase)*(4*(-time*time+time));
        sF32 w = sClamp(t, 0.0f, brect.x1-brect.x0);

        brect.x1 = rect.x0+(brect.x0+w)*(rect.x1-rect.x0);
        brect.x0 = rect.x0+brect.x0*(rect.x1-rect.x0);
        brect.y0 = rect.y0+brect.y0*(rect.y1-rect.y0);
        brect.y1 = rect.y0+brect.y1*(rect.y1-rect.y0);
        
        (*vp++).Init(brect.x0,brect.y0,0.5,color,0,0);
        (*vp++).Init(brect.x1,brect.y0,0.5,color,1,0);
        (*vp++).Init(brect.x1,brect.y1,0.5,color,1,1);
        (*vp++).Init(brect.x0,brect.y1,0.5,color,0,1);
      }
      geo.EndLoadVB();

      // set material
      BarMtrl->Set(&cb);

      // draw
      geo.Draw();
    }
  }

  void DoPaint()
  {
    sSetTarget(sTargetPara(sST_CLEARALL|sST_NOMSAA,sRELEASE?0:0xffff007f));

    // current slide
    DrawSlide(Mtrl1,0xffffffff,SiegData);

    // next slide
    if (TransTime>=0)
      DrawSlide(Mtrl2, 0x1010101 * int(255*sClamp(TransTime,0.0f,1.0f)), NextSiegData);
  }

  void OnPaint3D()
  {
    sInt tdelta;
    if (!StartTime) 
    {
        StartTime=sGetTime();
        Time = 0;
        tdelta = 0;
    }
    else
    {
      sInt newtime = sGetTime()-StartTime;
      tdelta = newtime-Time;
      Time = newtime;
    }

    NewSlideData *newslide = PlMgr.OnFrame(tdelta/1000.0f);

    if (newslide)
    {
      if (!newslide->Error)
      {
        if (TransTime>=0)
          EndTransition();
       
        MakeNextSlide(*newslide->ImgData);
        NextSiegData = newslide->SiegData;
        newslide->SiegData = 0;

        if (newslide->TransitionTime>0)
            SetTransition(newslide->TransitionTime);
        else
          EndTransition();
      }
      else sDPrintF(L"skipping faulty slide\n");
      delete newslide;
    }

    sTextBuffer Log;
    static sTiming time;

    time.OnFrame(sGetTime());
    if(Fps)
    {
      sF32 ms = time.GetAverageDelta();
      Log.PrintF(L"'%f' ms %f fps\n",ms,1000/ms);
      Log.PrintF(L"Time %dm%2ds  Frames %d\n",Time/60000,(Time/1000)%60,FramesRendered);
    }
   
    DoPaint();
    FramesRendered++;

    SlideTime += tdelta/1000.0f;
    if (TransTime>=0)
    {
      TransTime += tdelta/TransDurMS;
      if (TransTime>=1.0f)
        EndTransition();
    }

    if (SiegData && Started==1 && !SiegData->Winners && SlideTime>=MyConfig->BarAnimTime)
    {
      PlMgr.Next(sTRUE, sTRUE);
      Started = 2;
    }

    sEnableGraphicsStats(0);
    sSetTarget(sTargetPara(0,0));
    Painter->SetTarget();
    Painter->Begin();
    Painter->SetPrint(0,0xff000000,1,0xff000000);
    Painter->Print(10,11,Log.Get());
    Painter->Print(11,10,Log.Get());
    Painter->Print(12,11,Log.Get());
    Painter->Print(11,12,Log.Get());
    Painter->SetPrint(0,0xffc0c0c0,1,0xffffffff);
    Painter->Print(11,11,Log.Get());
    Painter->End();
  }

  void OnInput(const sInput2Event &ie)
  {
    if (PlMgr.OnInput(ie))
      return;

    sU32 key = ie.Key;
    key &= ~sKEYQ_CAPS;
    if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
    if(key & sKEYQ_CTRL ) key |= sKEYQ_CTRL;
    if(key & sKEYQ_ALT ) key |= sKEYQ_ALT;

    Config::KeyEvent *kev;
    sFORALL(MyConfig->Keys,kev) if (key == kev->Key) switch (kev->Type)
    {
    case Config::PLAYSOUND:
      SoundPlayer.Init(kev->ParaStr);
      SoundPlayer.Play();
      return;
    case Config::STOPSOUND:
      SoundPlayer.Exit();
      return;
    case Config::FULLSCREEN:
      ScreenMode.Flags ^= sSM_FULLSCREEN;
      sSetScreenMode(ScreenMode);
      return;
    case Config::SETRESOLUTION:
      SetupScreenMode(kev->ParaRes,ScreenMode.Flags&sSM_FULLSCREEN);
      sSetScreenMode(ScreenMode);
      return;
    }

    switch(key)
    {
    case sKEY_ESCAPE|sKEYQ_SHIFT:
    case sKEY_F4|sKEYQ_ALT:
      sExit();
      break;

    case 'F'|sKEYQ_SHIFT:
      Fps = !Fps;
      break;

    case ' ':
      if (TransTime<0 && !Started)
      {
        Started=1;
        SlideTime = 0;
      }
      break;
    }
  }
};


/****************************************************************************/
/****************************************************************************/

void sMain()
{
  sGetMemHandler(sAMF_HEAP)->MakeThreadSafe();

  sString<256> title=L"screens4minimal";
  if(sCONFIG_64BIT)
    title.PrintAddF(L" (64Bit)");
  sSetWindowName(title);

  MyConfig = new Config;
  if (!MyConfig->Read(L"config.txt"))
  {
    sFatal(MyConfig->GetError());
  }

  // open selector and set screen mode
  sInt flags=sISF_3D|sISF_CONTINUOUS; 

  sInt refresh = MyConfig->DefaultResolution.RefreshRate;
  if (MyConfig->DefaultResolution.Width==0 || MyConfig->DefaultResolution.Height==0)
  {
    sScreenInfo si;
    sGetScreenInfo(si);
    MyConfig->DefaultResolution.Width = si.CurrentXSize;
    MyConfig->DefaultResolution.Height = si.CurrentYSize;
  }

  SetupScreenMode(MyConfig->DefaultResolution,MyConfig->DefaultFullscreen);
  sSetScreenMode(ScreenMode);
  if (ScreenMode.Flags & sSM_FULLSCREEN) flags|=sISF_FULLSCREEN;
  sInit(flags,ScreenMode.ScreenX,ScreenMode.ScreenY);
 

  // .. and go!
  sSetApp(new MyApp);
}

/****************************************************************************/
