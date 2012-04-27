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

#define sPEDANTIC_OBSOLETE 1
#define sPEDANTIC_WARN 1

#include "main.hpp"
#include "base/windows.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"
#include "shader.hpp"

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  Painter = 0;
  Geo = 0;
  Mtrl = 0;
  Tex2D = 0;
  Tex3D = 0;

  // debug output

  Painter = new sPainter;

  // geometry 

  Geo = new sGeometry();
  Geo->Init(sGF_QUADLIST,sVertexFormatStandard);

  // texture 2d (just for comparison)

  sImage *img = new sImage(64,64);
  img->Checker(0xffff8080,0xff80ff80,8,8);
  img->Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);
  Tex2D = sLoadTexture2D(img,sTEX_2D|sTEX_ARGB8888);
  delete img;

  // texture 3d

  sInt size = 64;
  sInt rp,sp;
  sU8 *ptr;
  sU32 *d;
  Tex3D = new sTexture3D(size,size,size,sTEX_3D|sTEX_ARGB8888|sTEX_NOMIPMAPS);

  Tex3D->BeginLoad(ptr,rp,sp,0);
  for(sInt z=0;z<size;z++)
  {
    for(sInt y=0;y<size;y++)
    {
      d = (sU32 *) (ptr + y*rp + z*sp);
      for(sInt x=0;x<size;x++)
      {
        d[x] = (z&1) ? ((x^y^z)&1)? 0xffc08080 : 0xff80c080
                     : ((x^y^z)&1)? 0xffffc0ff : 0xffc0ffff;
      }
    }
  }
  Tex3D->EndLoad();

  // material

  Mtrl = new sVolumeMtrl;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  Mtrl->Texture[0] = Tex3D;
  Mtrl->TFlags[0] = sMTF_LEVEL0|sMTF_TILE;
  Mtrl->Prepare(sVertexFormatStandard);
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex2D;
  delete Tex3D;
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  // set rendertarget

  sSetTarget(sTargetPara(sST_CLEARALL,0xff405060));

  // get timing

  Timer.OnFrame(sGetTime());
  sInt time = Timer.GetTime();

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(1.0f);
  View.Model.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.Model.l.Init(0,0,0);
  View.Camera.l.Init(0,0,-4);
  View.Prepare();
 
  // set material

  sCBuffer<sVolumeVS> cb;
  cb.Data->mvp = View.ModelScreen;
  cb.Data->mv = View.ModelView;
  sF32 s = 0.125f;
  cb.Data->scaleuv.Init(s,s,s,0);
  Mtrl->Set(&cb);

  // load vertices and indices

  sVertexStandard *vp=0;
  Geo->BeginLoadVB(24,sGD_STREAM,&vp);

  vp->Init(-1, 1,-1,  0, 0,-1, 1,0); vp++; // 3
  vp->Init( 1, 1,-1,  0, 0,-1, 1,1); vp++; // 2
  vp->Init( 1,-1,-1,  0, 0,-1, 0,1); vp++; // 1
  vp->Init(-1,-1,-1,  0, 0,-1, 0,0); vp++; // 0

  vp->Init(-1,-1, 1,  0, 0, 1, 0,1); vp++; // 4
  vp->Init( 1,-1, 1,  0, 0, 1, 1,1); vp++; // 5
  vp->Init( 1, 1, 1,  0, 0, 1, 1,0); vp++; // 6
  vp->Init(-1, 1, 1,  0, 0, 1, 0,0); vp++; // 7

  vp->Init(-1,-1,-1,  0,-1, 0, 0,0); vp++; // 0
  vp->Init( 1,-1,-1,  0,-1, 0, 0,1); vp++; // 1
  vp->Init( 1,-1, 1,  0,-1, 0, 1,1); vp++; // 5
  vp->Init(-1,-1, 1,  0,-1, 0, 0,1); vp++; // 4

  vp->Init( 1,-1,-1,  1, 0, 0, 0,1); vp++; // 1
  vp->Init( 1, 1,-1,  1, 0, 0, 1,1); vp++; // 2
  vp->Init( 1, 1, 1,  1, 0, 0, 1,0); vp++; // 6
  vp->Init( 1,-1, 1,  1, 0, 0, 1,1); vp++; // 5

  vp->Init( 1, 1,-1,  0, 1, 0, 1,1); vp++; // 2
  vp->Init(-1, 1,-1,  0, 1, 0, 1,0); vp++; // 3
  vp->Init(-1, 1, 1,  0, 1, 0, 1,1); vp++; // 7
  vp->Init( 1, 1, 1,  0, 1, 0, 1,0); vp++; // 6

  vp->Init(-1, 1,-1, -1, 0, 0, 0,1); vp++; // 3
  vp->Init(-1,-1,-1, -1, 0, 0, 0,0); vp++; // 0
  vp->Init(-1,-1, 1, -1, 0, 0, 1,0); vp++; // 4
  vp->Init(-1, 1, 1, -1, 0, 0, 0,0); vp++; // 7

  Geo->EndLoadVB();

  // draw

  Geo->Draw();

  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000/avg,avg);
  Painter->End();
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  if((ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
}

/****************************************************************************/

// register application class

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS|sISF_FSAA/*|sISF_FULLSCREEN*/,640,480);
  sSetApp(new MyApp());
  sSetWindowName(L"Volume Texture");
}

/****************************************************************************/

