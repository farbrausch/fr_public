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

sINITMEM(sIMF_DEBUG|sIMF_CLEAR,0);

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  Painter=0;

  CubeGeo=0;
  CubeMtrl=0;
  CubeTex=0;

  SrcGeo=0;
  SrcMtrl=0;
  SrcTex=0;

  GlareGeo=0;
  GlareMtrl=0;
  GlareTex=0;

  sImage img(256,256);

  // debug output
  Painter = new sPainter;

  // Cube
  CubeGeo = new sGeometry();
  CubeGeo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatSingle);
  sMatrix34 mat,ext;
  sVector30 dir;
  sVertexSingle *vp=0;
  sU32 col = 0xffffffff;
  CubeGeo->BeginLoadVB(4*6,sGD_STATIC,&vp);

  ext.i.x = 2;
  ext.j.y = 0.5f;
  for(sInt i=0;i<6;i++)
  {
    mat.CubeFace(i);
    mat = mat*ext;
    dir =  mat.i+mat.j+mat.k;
    vp[0].Init(dir.x,dir.y,dir.z,  col, 0,0);
    dir = -mat.i+mat.j+mat.k;
    vp[1].Init(dir.x,dir.y,dir.z,  col, 1,0);
    dir = -mat.i-mat.j+mat.k;
    vp[2].Init(dir.x,dir.y,dir.z,  col, 1,1);
    dir =  mat.i-mat.j+mat.k;
    vp[3].Init(dir.x,dir.y,dir.z,  col, 0,1);
    vp+=4;
  }
  CubeGeo->EndLoadVB();

  sU16 *ip=0;
  CubeGeo->BeginLoadIB(6*6,sGD_STATIC,&ip);
  for(sInt i=0;i<6;i++)
    sQuad(ip,i*4,0,1,2,3);
  CubeGeo->EndLoadIB();

  img.Checker(0xffff8080,0xff80ff80,32,32);
  CubeTex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  CubeMtrl = new sSimpleMaterial;
  CubeMtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  CubeMtrl->Texture[0] = CubeTex;
  CubeMtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  CubeMtrl->Prepare(sVertexFormatSingle);

  // The Source Sprites
  sRandom rnd;
  for(sInt i=0;i<SOURCECOUNT;i++)
  {
    sVector30 dir;
    dir.InitRandom(rnd);
    dir.Unit();
    Sprites[i].Pos = sVector31(dir*(20.0f+rnd.Float(30.0f)));
    Sprites[i].Occ = new sOccQuery;
  }

  SrcGeo = new sGeometry(sGF_QUADLIST,sVertexFormatSingle);

  img.Fill(0x00000000);
  img.Glow(0.5f,0.5f,0.5f,0.5f,0xffffffff,1.0f,4.0f);
  SrcTex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  SrcMtrl = new sSimpleMaterial;
  SrcMtrl->Flags = sMTRL_ZREAD | sMTRL_CULLOFF;
  SrcMtrl->Texture[0] = SrcTex;
  SrcMtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  SrcMtrl->BlendColor = sMB_ALPHA;
  SrcMtrl->Prepare(sVertexFormatSingle);

  // The Glare Sprites

  GlareGeo = new sGeometry(sGF_QUADLIST,sVertexFormatSingle);

  img.Fill(0x00000000);
  img.Glow(0.5f,0.5f,0.5f,0.5f,0xffffffff,1.0f,0.50f);
  GlareTex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  GlareMtrl = new sSimpleMaterial;
  GlareMtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF;
  GlareMtrl->Texture[0] = GlareTex;
  GlareMtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  GlareMtrl->BlendColor = sMB_ADD;
  GlareMtrl->Prepare(sVertexFormatSingle);

  // calibration material

  CalibGeo = new sGeometry(sGF_QUADLIST,sVertexFormatSingle);
  CalibMtrl = new sSimpleMaterial;
  CalibMtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF;
  CalibMtrl->BlendColor = sMBS_0|sMBO_ADD|sMBD_1;
  CalibMtrl->Prepare(sVertexFormatSingle);
  CalibQuery = new sOccQuery;
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  for(sInt i=0;i<SOURCECOUNT;i++)
    delete Sprites[i].Occ;
  delete Painter;

  delete CubeGeo;
  delete CubeMtrl;
  delete CubeTex;

  delete SrcGeo;
  delete SrcMtrl;
  delete SrcTex;

  delete GlareGeo;
  delete GlareMtrl;
  delete GlareTex;

  delete CalibGeo;
  delete CalibMtrl;
  delete CalibQuery;
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
//  sF32 px,py;
  sVertexSingle *sv;

  sSetTarget(sTargetPara(sST_CLEARALL,0xff405060));

  // get timing
  Timer.OnFrame(sGetTime());
  static sInt time;
  if(sHasWindowFocus())
    time = Timer.GetTime();

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(1.0f);
  View.Model.Init();
  View.Model.l.Init(0,0,0);
  View.Camera.EulerXYZ(time*0.00011f,time*0.00013f,time*0.00015f);
  View.Camera.l = sVector31(View.Camera.k * -4);
  View.Prepare();

  // orthogonal viewport for glare sprites (goes from -1 to 1 in x and y)
  ViewOrtho.SetTargetCurrent();
  ViewOrtho.Orthogonal = sVO_SCREEN;
  ViewOrtho.Prepare();

  // set rendertarget
  sCBuffer<sSimpleMaterialPara> cb,cbo;
  cb.Data->Set(View);
  cbo.Data->Set(ViewOrtho);

  sF32 area = 4.0;      // from -1 -1 to 1 1
  sF32 view = View.ZoomX * View.ZoomY * View.TargetSizeX *View.TargetSizeY * 0.25f;

  // calibrate the occlusion queries for Multisampling.
  sVector31 calibpos = View.Camera.l+sVector30(View.Camera.k * 5);

  CalibGeo->BeginLoadVB(4,sGD_STREAM,&sv);
  sv[0].Init(calibpos-View.Camera.i-View.Camera.j,0xffffffff,0.0f,0.0f);
  sv[1].Init(calibpos-View.Camera.i+View.Camera.j,0xffffffff,0.0f,1.0f);
  sv[2].Init(calibpos+View.Camera.i+View.Camera.j,0xffffffff,1.0f,1.0f);
  sv[3].Init(calibpos+View.Camera.i-View.Camera.j,0xffffffff,1.0f,0.0f);
  CalibGeo->EndLoadVB();
  CalibMtrl->Set(&cb);

  sVector31 tp = calibpos * View.View;
  sInt pixels = sInt(area * view / (tp.z*tp.z));
  CalibQuery->Begin(pixels);
  CalibGeo->Draw();
  CalibQuery->End();
  sF32 msaa = sRoundNear(CalibQuery->Last);

  // the cube
  CubeMtrl->Set(&cb);
  CubeGeo->Draw();

  // sources. 
  // paint them one by one, so we can query the occlusion...

  SrcMtrl->Set(&cb);

  sDrawRange ir;
  for(sInt i=0;i<SOURCECOUNT;i++)
  {
    const sVector31 &pos = Sprites[i].Pos;
    sVector31 tp = pos * View.View;
    if(tp.z>0.01f)
    {
      sInt pixels = sInt(area * msaa * view / (tp.z*tp.z));
      if(pixels>0)
      {
        Sprites[i].Occ->Begin(pixels);
        ir.Start = i*4;
        ir.End = i*4+4;

        SrcGeo->BeginLoadVB(4,sGD_STREAM,(void**)&sv);
        sv[0].Init(pos-View.Camera.i-View.Camera.j,0xffffffff,0.0f,0.0f);
        sv[1].Init(pos-View.Camera.i+View.Camera.j,0xffffffff,0.0f,1.0f);
        sv[2].Init(pos+View.Camera.i+View.Camera.j,0xffffffff,1.0f,1.0f);
        sv[3].Init(pos+View.Camera.i-View.Camera.j,0xffffffff,1.0f,0.0f);        
        SrcGeo->EndLoadVB();
        SrcGeo->Draw();

        Sprites[i].Occ->End();
      }
    }
  }


  // glare. this could all go to a shader, using a second vertex stream for the colors.
  // but just keep this sample simple...
  sVector30 dx(0.15f/View.TargetAspect,0,0);
  sVector30 dy(0,0.15f,0);
  GlareGeo->BeginLoadVB(SOURCECOUNT*4,sGD_STREAM,&sv);
  sInt n=0;
  for(sInt i=0;i<SOURCECOUNT;i++)
  {
    sVector31 pos(0,0,0.001f);
    if(View.Transform(Sprites[i].Pos,pos.x,pos.y))
    {
      sU32 col = sU32(sClamp(Sprites[i].Occ->Average,0.0f,1.0f)*0x3f);
      if(col>0)
      {
        col *= 0x10101;
        sv++->Init(pos-dx-dy,col,0,0);
        sv++->Init(pos-dx+dy,col,0,1);
        sv++->Init(pos+dx+dy,col,1,1);
        sv++->Init(pos+dx-dy,col,1,0);
        n++;
      }
    }
  }
  GlareGeo->EndLoadVB(n*4);
  if(n>0)
  {
    GlareMtrl->Set(&cbo);
    GlareGeo->Draw();
  }

  // debug output
  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000.0f/avg,avg);
  Painter->PrintF(10,30,L"multisample factor %f (%d)",CalibQuery->Last,sRoundNearInt(CalibQuery->Last));
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
  sSetWindowName(L"Occlusion Query");
  sInit(sISF_3D|sISF_CONTINUOUS|sISF_NOVSYNC|sISF_FSAA/*|sISF_FULLSCREEN*/,640,480);
  sSetApp(new MyApp());
}

/****************************************************************************/

