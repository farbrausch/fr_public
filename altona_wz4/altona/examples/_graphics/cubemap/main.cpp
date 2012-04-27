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
/***   Draw a cube using a custom shader.                                 ***/
/***                                                                      ***/
/***   the "altona shader compiler" does most of the ugly work.           ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "base/windows.hpp"
#include "shader.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"

/****************************************************************************/

// constructor

MyApp::MyApp()
{
  Painter = new sPainter;
  sRandom rnd;
  rnd.Seed(5);
  Env.AmbientColor = 0x00404040;
  Env.LightDir[0].Init(1,-1,1);
  Env.LightColor[0] = 0x00c0c0c0;
  Env.LightDir[1].Init(-1,1,-1);
  Env.LightColor[1] = 0x00000040;
  Env.Fix();

  // a lot of cubes

  sImage img;
  img.Init(32,32);
  img.Checker(0xffff4040,0xff40ff40,4,4);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffc0c0c0,1.0f,4.0f);

  CubeTex = sLoadTexture2D(&img,sTEX_ARGB8888);

  CubeMtrl = new sSimpleMaterial;
  CubeMtrl->Texture[0] = CubeTex;
  CubeMtrl->Flags = sMTRL_ZON|sMTRL_CULLON|sMTRL_LIGHTING;
  CubeMtrl->TFlags[0] = sMTF_CLAMP|sMTF_LEVEL2;
  CubeMtrl->Prepare(sVertexFormatStandard);

  CubeGeo = new sGeometry(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);
  sF32 s = 0.5f;
  CubeGeo->LoadCube(-1,s,s,s);

  CubeMats.AddMany(20);
  sMatrix34 *mat;
  sFORALL(CubeMats,mat)
  {
    mat->EulerXYZ(rnd.Float(sPI2F),rnd.Float(sPI2F),rnd.Float(sPI2F));
    sVector30 dir;
    dir.InitRandom(rnd);
    dir.Unit();

    mat->l = sVector31(dir*5);
  }

  // a torus

//  TorusCubemap = new sTextureCube(512,sTEX_CUBE|sTEX_ARGB8888|sTEX_RENDERTARGET|sTEX_AUTOMIPMAP);
  TorusCubemap = new sTextureCube(512,sTEX_CUBE|sTEX_ARGB8888|sTEX_RENDERTARGET|sTEX_NOMIPMAPS);
  TorusDepth = new sTexture2D(512,512,sTEX_2D|sTEX_DEPTH24NOREAD|sTEX_RENDERTARGET|sTEX_NOMIPMAPS);

  TorusMtrl = new TorusShader;
  TorusMtrl->Texture[0] = TorusCubemap;
  TorusMtrl->Flags = sMTRL_ZON|sMTRL_CULLON;
  TorusMtrl->TFlags[0] = sMTF_CLAMP|sMTF_LEVEL2;
  TorusMtrl->LodBias[0] = -1.0f;
  TorusMtrl->Prepare(sVertexFormatStandard);

  sInt tx = 64;
  sInt ty = 48;
  sF32 ro = 2.0f;
  sF32 ri = 0.85f;
  sVertexStandard *vp;
  sU16 *ip;
  TorusGeo = new sGeometry(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);
  TorusGeo->BeginLoadVB((tx+1)*(ty+1),sGD_STATIC,&vp);
  for(sInt y=0;y<=ty;y++)
  {
    for(sInt x=0;x<=tx;x++)
    {
      sF32 px = x * sPI2F / tx;
      sF32 py = y * sPI2F / ty;
      vp->px = -sFCos(px)*(ro+sFCos(py)*ri);
      vp->py = -              sFSin(py)*ri;
      vp->pz =  sFSin(px)*(ro+sFCos(py)*ri);
      vp->nx = -sFCos(px)*sFCos(py);
      vp->ny = -          sFSin(py);
      vp->nz =  sFSin(px)*sFCos(py);
      vp->u0 = sF32(x)/tx;
      vp->v0 = sF32(y)/ty;
      vp++;
    }
  }
  TorusGeo->EndLoadVB();
  TorusGeo->BeginLoadIB(tx*ty*6,sGD_STATIC,&ip);
  for(sInt y=0;y<ty;y++)
    for(sInt x=0;x<tx;x++)
      sQuad(ip,y*(tx+1)+x,1,0,tx+1,tx+2);
  TorusGeo->EndLoadIB();
}

/****************************************************************************/

// destructor

MyApp::~MyApp()
{
  delete Painter;
  delete CubeGeo;
  delete CubeMtrl;
  delete CubeTex;

  delete TorusGeo;
  delete TorusMtrl;
  delete TorusCubemap;
  delete TorusDepth;
}

/****************************************************************************/

// painting

void MyApp::OnPaint3D()
{
  // timing

  Timer.OnFrame(sGetTime());
  time = Timer.GetTime();

  // Render

  RenderCubemap();
  RenderScene();

  // debug out: framerate.
  sF32 avg = Timer.GetAverageDelta();
  Painter->Begin();
  Painter->SetPrint(0,~0,1);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000/avg,avg);
  Painter->End();
}

void MyApp::RenderCubemap()
{
  for(sInt i=0;i<6;i++)
  {
    sTargetPara tp(sST_CLEARALL,0xff808080,0,TorusCubemap,TorusDepth);
    tp.Cubeface = i;
    sSetTarget(tp);

    View.SetTargetCurrent();
    View.SetZoom(1);
    View.Model.Init();
    View.Camera.CubeFace(i);
    View.Prepare();

    PaintCubes();
  }
}

void MyApp::RenderScene()
{
  // clear screen

  sSetTarget(sTargetPara(sCLEAR_ALL,0xff405060));

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(1.7f);
  View.Model.Init();
  View.Camera.Init();
  View.Camera.l = sVector31(View.Camera.k*-6);
  View.Prepare();

  // Paint Cubes

  PaintCubes();

  // Paint Torus

  sCBuffer<TorusShaderPara> cbt;
  View.Model.Init();
  View.Model.EulerXYZ(time*0.00011f,time*0.0003f,time*0.00012f);
  View.Prepare();
  cbt.Modify();
  cbt.Data->Set(View,Env);
  TorusMtrl->Set(&cbt);
  TorusGeo->Draw();
  
}

void MyApp::PaintCubes()
{
  // if we were going for speed, we would use instances here...
  sMatrix34 mrot;
  mrot.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0012f);

  sCBuffer<sSimpleMaterialEnvPara> cb;
  sMatrix34 *mat;
  sFORALL(CubeMats,mat)
  {
    View.UpdateModelMatrix(mrot**mat);
    cb.Modify();
    cb.Data->Set(View,Env);
    CubeMtrl->Set(&cb);
    CubeGeo->Draw();
  }
}

/****************************************************************************/

// onkey: check for escape

void MyApp::OnInput(const sInput2Event &ie)
{
  if((ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
}

/****************************************************************************/

// main: initialize screen and app

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS,1280,720);
  sSetWindowName(L"cubemap");
  sSetApp(new MyApp());
}

/****************************************************************************/
