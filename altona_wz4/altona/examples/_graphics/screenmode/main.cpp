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
#include "base/windows.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"

sInt Settings[LINE_MAX];    // must be static, to survive restart

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  // debug output

  Painter = new sPainter;

  // geometry 

  Geo = new sGeometry();
  Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);
  GeoQuad = new sGeometry();
  GeoQuad->Init(sGF_QUADLIST,sVertexFormatSingle);

  // texture
/*
  sImage img;
  img.Init(64,64);
  img.Checker(0xffff8080,0xff80ff80,8,8);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);

  Tex = sLoadTexture2D(&img,sTEX_ARGB8888|sTEX_2D);
*/

  Tex = new sTexture2D(256,256,sTEX_ARGB8888|sTEX_2D|sTEX_RENDERTARGET|sTEX_AUTOMIPMAP);
  sCreateZBufferRT(256,256);

  // material

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Prepare(sVertexFormatStandard);

  White = new sSimpleMaterial;
  White->Flags = sMTRL_ZON | sMTRL_CULLOFF;
  White->Prepare(sVertexFormatSingle);

  // light

  sClear(Env);
  Env.AmbientColor  = 0xff404040;
  Env.LightColor[0] = 0x00c00000;
  Env.LightColor[1] = 0x0000c000;
  Env.LightColor[2] = 0x000000c0;
  Env.LightColor[3] = 0x00000000;
  Env.LightDir[0].Init(1,0,0);
  Env.LightDir[1].Init(0,1,0);
  Env.LightDir[2].Init(0,0,1);
  Env.LightDir[3].Init(0,0,0);
  Env.Fix();

  // Gui

  InitGui();
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete GeoQuad;
  delete Mtrl;
  delete White;
  delete Tex;
}

/****************************************************************************/

void MyApp::OnPaint3D()
{ 
  // set rendertarget


  // get timing

  Timer.OnFrame(sGetTime());
  sInt time = Timer.GetTime();

  // draw

  PaintRT(time);
  sSetRendertarget(0,sCLEAR_ALL,0xff405060);
  PaintCube(time);
  PaintSamplePattern(time);

  // debug output

  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,~0,1);
  PrintGui();
   
  Painter->End();
}

void MyApp::PaintRT(sInt time)
{
  sSetTarget(sTargetPara(sST_CLEARALL,0xffffffff,0,Tex,sGetRTDepthBuffer()));

  sViewport View;
  View.SetTargetCurrent();
  View.SetZoom(1.0f);
  View.Camera.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.Camera.l = sVector31(View.Camera.k*-3);
  View.Prepare();

  sCBuffer<sSimpleMaterialPara> cb;
  cb.Modify();
  cb.Data->Set(View);
  White->Set(&cb);

  sMatrix34 mat;
  sRandom rnd;
  sSRT srt;
  sVertexSingle *vp;
  const sInt count = 10;
  GeoQuad->BeginLoadVB(count*4,sGD_STREAM,(void **)&vp);
  for(sInt i=0;i<count;i++)
  {
    srt.Rotate.x = rnd.Float(sPI2F);
    srt.Rotate.y = rnd.Float(sPI2F);
    srt.Rotate.z = rnd.Float(sPI2F);
    srt.Scale.x = rnd.Float(1)+1;
    srt.Scale.y = rnd.Float(0.125f)+0.125f;
    srt.Scale.z = 1;
    srt.Translate.x = rnd.Float(2)-1;
    srt.Translate.y = rnd.Float(2)-1;
    srt.Translate.z = rnd.Float(2)-1;
    srt.MakeMatrix(mat);
    sU32 col = rnd.Int32()|0xff000000;

    vp[0].Init(mat.l+mat.i+mat.j,col,0,0);
    vp[1].Init(mat.l-mat.i+mat.j,col,0,0);
    vp[2].Init(mat.l-mat.i-mat.j,col,0,0);
    vp[3].Init(mat.l+mat.i-mat.j,col,0,0);
    vp+=4;
  }
  GeoQuad->EndLoadVB();
  GeoQuad->Draw();

}

void MyApp::PaintCube(sInt time)
{
  sViewport View;

  View.SetTargetCurrent();
  if(Settings[LINE_ASPECT]==0)
  {
    View.SetZoom(1.0f);
  }
  else
  {
    Resolution a = ScreenAspects[Settings[LINE_ASPECT]];
    View.SetZoom(sF32(a.XSize)/a.YSize,1.0f);
  }
  View.Camera.EulerXYZ(time*0.000011f,time*0.000013f,time*0.000015f);
  View.Camera.l = sVector31(View.Camera.k*-10);

  // load vertices and indices

  const sF32 s=0.3f;
  sVertexStandard *vp;
  sMatrix34 mat;
  Geo->BeginLoadVB(24,sGD_STREAM,(void **)&vp);
  for(sInt i=0;i<6;i++)
  {
    mat.CubeFace(i);
    vp[0].Init(sVector31((mat.k+mat.i+mat.j)*s),mat.k,0,0);
    vp[1].Init(sVector31((mat.k-mat.i+mat.j)*s),mat.k,1,0);
    vp[2].Init(sVector31((mat.k-mat.i-mat.j)*s),mat.k,1,1);
    vp[3].Init(sVector31((mat.k+mat.i-mat.j)*s),mat.k,0,1);
    vp+=4;
  }
  Geo->EndLoadVB();

  sU16 *ip;
  Geo->BeginLoadIB(6*6,sGD_STREAM,(void **)&ip);
  sQuad(ip, 0,0,1,2,3);
  sQuad(ip, 4,0,1,2,3);
  sQuad(ip, 8,0,1,2,3);
  sQuad(ip,12,0,1,2,3);
  sQuad(ip,16,0,1,2,3);
  sQuad(ip,20,0,1,2,3);
  Geo->EndLoadIB();

  // draw

  sVector30 dirs[6];
  dirs[0].Init(-1,0,0);
  dirs[1].Init( 1,0,0);
  dirs[2].Init(0,-1,0);
  dirs[3].Init(0, 1,0);
  dirs[4].Init(0,0,-1);
  dirs[5].Init(0,0, 1);

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->la.Init(0.25f,0.25f,0.25f,0);
  cb.Data->lc[0].Init(0.75f,0.75f,0.75f,0);
  cb.Data->ld[0].x = -View.Camera.k.x;
  cb.Data->ld[1].x = -View.Camera.k.y;
  cb.Data->ld[2].x = -View.Camera.k.z;
  sRandom rnd;
  for(sInt i=0;i<6;i++)
  {
    for(sInt j=1;j<20;j++)
    {
      View.Model.EulerXYZ(rnd.Float(sPI2F),rnd.Float(sPI2F),rnd.Float(sPI2F));
      View.Model.l = sVector31(dirs[i]*j);
      View.Prepare();
      cb.Modify();
      cb.Data->ld[0].x = -View.ModelView.i.z;
      cb.Data->ld[1].x = -View.ModelView.j.z;
      cb.Data->ld[2].x = -View.ModelView.k.z;
      cb.Data->mvp = View.ModelScreen;
      Mtrl->Set(&cb);

      Geo->Draw();
    }
  }
}

void MyApp::PaintSamplePattern(sInt time)
{
  sViewport View;
  sF32 f0,f1,f,x0,y0,x1,y1;
  sInt xs,ys;
  sGetScreenSize(xs,ys);
  const sInt n = 16;
  f = 1.0f/n;
  f0 = 0;// + (time&0xfff)/4096.0f;
  f1 = f + f0;
  x0 = 4;//xs-n*4-4;
  y0 = 4;//ys-n*4-4;
  x1 = x0+n*4;
  y1 = y0+n*4;

  View.SetTargetCurrent();
  View.Orthogonal = sVO_PIXELS;
  View.Prepare();

  sCBuffer<sSimpleMaterialPara> cb;
  cb.Data->Set(View);
  White->Set(&cb);

  sVertexSingle *vp;
  GeoQuad->BeginLoadVB((n*n*16+1)*4,sGD_STREAM,(void **)&vp);

  vp[0].Init(x0,y0,0,0,0,0);
  vp[1].Init(x1,y0,0,0,0,0);
  vp[2].Init(x1,y1,0,0,0,0);
  vp[3].Init(x0,y1,0,0,0,0);
  vp+=4;
  for(sInt x=0;x<n;x++)
  {
    for(sInt y=0;y<n;y++)
    {
      for(sInt xx=0;xx<4;xx++)
      {
        for(sInt yy=0;yy<4;yy++)
        {
          sF32 xp = x0+x*4+xx+x*f;
          sF32 yp = y0+y*4+yy+y*f;
          vp[0].Init(xp+f0,yp+f0,0,~0,0,0);
          vp[1].Init(xp+f1,yp+f0,0,~0,0,0);
          vp[2].Init(xp+f1,yp+f1,0,~0,0,0);
          vp[3].Init(xp+f0,yp+f1,0,~0,0,0);
          vp+=4;
        }
      }
    }
  }
  GeoQuad->EndLoadVB();

  GeoQuad->Draw();
}

/****************************************************************************/

void MyApp::OnPrepareFrame()
{
  UpdateGui();
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  sU32 key = ie.Key;
  if(key&sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL)  key |= sKEYQ_CTRL;
  key &= ~sKEYQ_REPEAT;
  switch(key) 
  {
  case sKEY_ESCAPE:
    sExit();
    break;

  case sKEY_UP:
    Cursor = sClamp(Cursor-1,0,LINE_MAX-1);
    break;
  case sKEY_DOWN:
    Cursor = sClamp(Cursor+1,0,LINE_MAX-1);
    break;
  case sKEY_LEFT:
    Settings[Cursor] = sClamp(Settings[Cursor]-1,0,SetMax[Cursor]-1);
    UpdateGui();
    break;
  case sKEY_LEFT|sKEYQ_SHIFT:
    Settings[Cursor] = 0;
    UpdateGui();
    break;
  case sKEY_RIGHT:
    Settings[Cursor] = sClamp(Settings[Cursor]+1,0,SetMax[Cursor]-1);
    UpdateGui();
    break;
  case sKEY_RIGHT|sKEYQ_SHIFT:
    Settings[Cursor] = SetMax[Cursor]-1;
    UpdateGui();
    break;
  }
}

/****************************************************************************/
/****************************************************************************/

void MyApp::InitGui()
{
  Cursor = 0;
  for(sInt i=0;i<LINE_MAX;i++)
  {
    SetMax[i] = 2;
    SetStrobe[i] = 1;
    SetString[i] = L"???";
    SetLabel[i] = L"???";
  }

  SetLabel[LINE_SCREEN]  = L"Resolution";
  SetLabel[LINE_DISPLAY] = L"Display";
  SetLabel[LINE_WINDOW]  = L"Windowed";
  SetLabel[LINE_REFRESH] = L"Refresh Rate";
  SetLabel[LINE_FSAA]    = L"FSAA";
  SetLabel[LINE_ASPECT]  = L"Aspect";
  SetLabel[LINE_OVERSIZE]= L"Oversize";
  SetLabel[LINE_OVERFSAA]= L"Oversize FSAA";
  SetLabel[LINE_RTSIZE]  = L"Rendertarget";

  OverSizes.AddMany(1)->Init(   0,   0,L"off");
  OverSizes.AddMany(1)->Init( 400, 300,L"%d x %d");
  OverSizes.AddMany(1)->Init( 800, 600,L"%d x %d");
  OverSizes.AddMany(1)->Init(1600,1200,L"%d x %d");
  OverSizes.AddMany(1)->Init(2000,1500,L"%d x %d");
  OverSizes.AddMany(1)->Init(4000,3000,L"%d x %d");
  OverSizes.AddMany(1)->Init(8000,6000,L"%d x %d");
  RTSizes  .AddMany(1)->Init(  16,  16,L"%d x %d");
  RTSizes  .AddMany(1)->Init(  64,  64,L"%d x %d");
  RTSizes  .AddMany(1)->Init( 256, 256,L"%d x %d");
  RTSizes  .AddMany(1)->Init(1024,1024,L"%d x %d");
  RTSizes  .AddMany(1)->Init(2048,2048,L"%d x %d");
  RTSizes  .AddMany(1)->Init(4096,4096,L"%d x %d");

  SetMax[LINE_SCREEN] = ScreenSizes.GetCount();
  SetMax[LINE_FSAA] = MultisampleModes.GetCount();
  SetMax[LINE_ASPECT] = ScreenAspects.GetCount();
  SetMax[LINE_OVERSIZE] = OverSizes.GetCount();
  SetMax[LINE_RTSIZE] = RTSizes.GetCount();
  SetMax[LINE_OVERFSAA] = MultisampleModes.GetCount();

  UpdateGui();
}

void MyApp::ExitGui()
{
}

void MyApp::PrintGui()
{
  sInt y = 10;
  sF32 avg = Timer.GetAverageDelta();
  Painter->PrintF(80,y,L"%5.2ffps %5.3fms",1000/avg,avg); y+=10;
  for(sInt i=0;i<LINE_MAX;i++)
  {
    Painter->PrintF(80,y,L"%s%-12s(%d/%d): %s"
      ,Cursor==i ? L"-> " : L"   ",SetLabel[i]
      ,Settings[i],SetMax[i],SetString[i]); 
    y+=10;
  }
}
void MyApp::UpdateGui()
{
  sScreenInfo si;
  sScreenInfoXY *xy;
  sInt *ip;
  sGetScreenInfo(si,0,Settings[LINE_DISPLAY]);

  SetMax[LINE_DISPLAY] = sGetDisplayCount();

  SetMax[LINE_SCREEN] = si.Resolutions.GetCount();
  ScreenSizes.Clear();
  sFORALL(si.Resolutions,xy)
    ScreenSizes.AddMany(1)->Init(xy->x,xy->y,L"%dx%d");

  SetMax[LINE_FSAA] = si.MultisampleLevels+1;
  SetMax[LINE_OVERFSAA] = si.MultisampleLevels+1;
  MultisampleModes.Clear();
  MultisampleModes.AddMany(1)->Init(0,0,L"off");
  for(sInt i=0;i<si.MultisampleLevels;i++)
    MultisampleModes.AddMany(1)->Init(i,1,L"level %d");

  SetMax[LINE_ASPECT] = si.AspectRatios.GetCount()+1;
  ScreenAspects.Clear();
  ScreenAspects.AddMany(1)->Init(0,0,L"auto");
  sFORALL(si.AspectRatios,xy)
    ScreenAspects.AddMany(1)->Init(xy->x,xy->y,L"%d:%d");

  SetMax[LINE_REFRESH] = si.RefreshRates.GetCount()+1;
  RefreshRates.Clear();
  RefreshRates.AddMany(1)->Init(0,0,L"default");
  sFORALL(si.RefreshRates,ip)
    RefreshRates.AddMany(1)->Init(*ip,0,L"%dHz");

  for(sInt i=0;i<LINE_MAX;i++)
    Settings[i] = sClamp(Settings[i],0,SetMax[i]-1);

  SetString[LINE_DISPLAY].PrintF(L"Monitor %d",Settings[LINE_DISPLAY]+1);
  SetString[LINE_SCREEN]  = ScreenSizes[Settings[LINE_SCREEN]].Name;
  SetString[LINE_WINDOW]  = Settings[LINE_WINDOW] ? L"Fullscreen" : L"Windowed";
  SetString[LINE_REFRESH] = RefreshRates[Settings[LINE_REFRESH]].Name;
  SetString[LINE_FSAA  ]  = MultisampleModes[Settings[LINE_FSAA  ]].Name;
  SetString[LINE_ASPECT]  = ScreenAspects[Settings[LINE_ASPECT]].Name;
  SetString[LINE_OVERSIZE]= OverSizes[Settings[LINE_OVERSIZE]].Name;
  SetString[LINE_OVERFSAA]= MultisampleModes[Settings[LINE_OVERFSAA]].Name;
  SetString[LINE_RTSIZE]  = RTSizes[Settings[LINE_RTSIZE]].Name;

  // update screenmode;

  sScreenMode sm;
  sGetScreenMode(sm);
  sm.Flags = sSM_VALID|sSM_NOVSYNC;
  sm.Flags |= Settings[LINE_WINDOW] ? sSM_FULLSCREEN : 0;
  sm.ScreenX = ScreenSizes[Settings[LINE_SCREEN]].XSize;
  sm.ScreenY = ScreenSizes[Settings[LINE_SCREEN]].YSize;
  sm.MultiLevel = Settings[LINE_FSAA]-1;
  sm.RTZBufferX = RTSizes[Settings[LINE_RTSIZE]].XSize;
  sm.RTZBufferY = RTSizes[Settings[LINE_RTSIZE]].YSize;
  sm.OverX = OverSizes[Settings[LINE_OVERSIZE]].XSize;
  sm.OverY = OverSizes[Settings[LINE_OVERSIZE]].YSize;
  sm.OverMultiLevel = Settings[LINE_OVERFSAA]-1;
  if(Settings[LINE_OVERSIZE]>0)  sm.Flags |= sSM_OVERSIZE;

  if(sm.Display==-1) sm.Display = 0;
  if(sm.Display!=Settings[LINE_DISPLAY])     
  {
    sm.Display = Settings[LINE_DISPLAY];
#if sRENDERER!=sRENDER_DX11
    sRestart();   // can't change display without drastic measures: recreate the device
#endif
  }
  if(sm.RTZBufferX!=Tex->SizeX || sm.RTZBufferY!=Tex->SizeY)
  {
    delete Tex;
    Tex = new sTexture2D(sm.RTZBufferX,sm.RTZBufferY,sTEX_ARGB8888|sTEX_2D|sTEX_RENDERTARGET|sTEX_AUTOMIPMAP);
    Mtrl->Texture[0] = Tex;
  }
  if(sCmpMem(&sm,&LastMode,sizeof(sScreenMode))!=0)
  {
    sSetScreenMode(sm);
    LastMode = sm;
  }
}

/****************************************************************************/
/****************************************************************************/

// register application class

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS/*|sISF_FULLSCREEN*/,640,480);
  sSetApp(new MyApp());
  sSetWindowName(L"Cube");
}

/****************************************************************************/

