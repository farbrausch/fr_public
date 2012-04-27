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
/***                                                                      ***/
/***   Demonstrate                                                        ***/
/***   - multiple vertex streams                                          ***/
/***   - instances                                                        ***/
/***                                                                      ***/
/****************************************************************************/

#define sPEDANTIC_OBSOLETE 1
#define sPEDANTIC_WARN 1

#include "main.hpp"
#include "base/windows.hpp"
#include "util/shaders.hpp"
#include "util/image.hpp"
#include "shader.hpp"

/****************************************************************************/

MyApp::MyApp()
{
  sImage *img;

  Painter = new sPainter;

  // perlin texture

  sInt xs = 256;
  sInt ys = 256;
  img = new sImage;
  img->Init(xs,ys);
  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 p = sPerlin2D(x*(0x100000/xs),y*(0x100000/ys),3,0)*2.0f
             + sPerlin2D(x*(0x200000/xs),y*(0x200000/ys),7,0)*1.5f
             + sPerlin2D(x*(0x400000/xs),y*(0x400000/ys),15,0)*1.0f
             ;
      sInt c = sInt(128+127*sClamp(p,-1.0f,1.0f));
      img->Data[y*xs+x] = c|(c<<8)|(c<<16)|0xff000000;
    }
  }
  Tex = sLoadTexture2D(img,sTEX_ARGB8888);
  delete img;

  CreateTorus();

  // materials

  TorusMtrl = new TorusShader;
  TorusMtrl->Texture[0] = Tex;
  TorusMtrl->Texture[1] = Tex;
  TorusMtrl->TBind[1] = sMTB_VS|0;
  TorusMtrl->Flags = sMTRL_ZON|sMTRL_CULLOFF;
  TorusMtrl->Prepare(TorusFormat);
}

void MyApp::CreateTorus()
{
  const sInt TessX = 128;
  const sInt TessY = 128;
  const sF32 ri = 0.25f;
  const sF32 ro = 1.0f;
  sU16 *ip;
  sVertexStandard *vp;

  TorusFormat = sVertexFormatStandard;

  TorusGeo = new sGeometry;
  TorusGeo->Init(sGF_TRILIST|sGF_INDEX16|sGF_INSTANCES,TorusFormat);

  TorusGeo->BeginLoadIB(TessX*TessY*6,sGD_STATIC,(void **)&ip);
  for(sInt y=0;y<TessY;y++)
  {
    for(sInt x=0;x<TessX;x++)
    {
      sQuad(ip,0,
        (x+0)+((y+0))*(TessX+1),
        (x+0)+((y+1))*(TessX+1),
        (x+1)+((y+1))*(TessX+1),
        (x+1)+((y+0))*(TessX+1));
    }
  }
  TorusGeo->EndLoadIB();

  TorusGeo->BeginLoadVB((TessX+1)*(TessY+1),sGD_STATIC,(void **)&vp);
  for(sInt y=0;y<=TessY;y++)
  {
    for(sInt x=0;x<=TessX;x++)
    {
      sF32 fx = x*sPI2F/TessX;
      sF32 fy = y*sPI2F/TessY;
      sF32 px = (x==TessX) ? 0 : fx;
      sF32 py = (y==TessY) ? 0 : fy;

      vp->px = sFSin(py)*(ro+sFSin(px)*ri);
      vp->py = sFCos(px)*ri;
      vp->pz = sFCos(py)*(ro+sFSin(px)*ri);
      vp->nx = sFSin(py)*(ro+sFSin(px)*ri) - sFSin(py)*(ro+sFSin(px)*(ri+1));
      vp->ny = sFCos(px)*ri - sFCos(px)*(ri+1);
      vp->nz = sFCos(py)*(ro+sFSin(px)*ri) - sFCos(py)*(ro+sFSin(px)*(ri+1));
      vp->u0 = sF32(x)/TessX/4;
      vp->v0 = sF32(y)/TessY;
      vp++;
    }
  }
  TorusGeo->EndLoadVB();
}


/****************************************************************************/

MyApp::~MyApp()
{
  delete Painter;
  delete TorusGeo;
  delete TorusMtrl;
  delete Tex;
}

/****************************************************************************/

void MyApp::OnPaint3D()
{
  sVector30 n;
  sVector31 p;
  sMatrix34 mat;
  sMatrix44 mat4;

  // timer

  Timer.OnFrame(sGetTime());
  sInt time = Timer.GetTime();

  // init viewport

  View.Camera.EulerXYZ(0.3,time*0.0001f,0);
  View.Camera.l = sVector31(View.Camera.k * -3);
  View.Model.Init();
  View.SetTargetCurrent();
  View.SetZoom(1.5f);
  View.Prepare();

  // start painting

  sSetTarget(sTargetPara(sST_CLEARALL,0xff405060));
  sCBuffer<TorusPara> cb;
  cb.Data->mvp = View.ModelScreen;
  cb.Data->scale.x = sFSin(time*0.004f)*0.125f;

  // torus: paint a ring of rings

  TorusMtrl->Set(&cb);
  TorusGeo->Draw();

  // paint debug info

  sF32 avg = Timer.GetAverageDelta();
  Painter->Begin();
  Painter->SetPrint(0,~0,1);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000/avg,avg);
  Painter->End();
}

/****************************************************************************/

void MyApp::OnInput(const sInput2Event &ie)
{
  if((ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
}

/****************************************************************************/

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS,640,480);
  sSetApp(new MyApp());
  sSetWindowName(L"Vertex Texture");
}

/****************************************************************************/

