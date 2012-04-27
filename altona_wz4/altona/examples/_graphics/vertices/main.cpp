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
#include "instshader.hpp"

/****************************************************************************/

const sInt GridX=64;
const sInt GridY=64;

struct WaterStream0
{
  sU32 c0;                        // 1 color
  sF32 u0,v0;                     // 2 uv's
};

struct WaterStream1
{
  sF32 px,py,pz;                  // 3 pos
};

/****************************************************************************/

MyApp::MyApp()
{
  sImage *img;

  Painter = new sPainter;

  // checker texture

  img = new sImage;
  img->Init(256,256);
  img->Checker(0xffffffff,0xff808080,8,8);
  Tex = sLoadTexture2D(img,sTEX_ARGB8888);
  delete img;

  CreateWater();
  CreateTorus();

  // geos

  QuadGeo = new sGeometry();
  QuadGeo->Init(sGF_QUADLIST,sVertexFormatSingle);
  InstGeo = new sGeometry();
  InstGeo->Init(sGF_TRILIST|sGF_INDEX16|sGF_INSTANCES,TorusFormat);
  MergeGeo = new sGeometry();
  MergeGeo->Init(sGF_TRILIST|sGF_INDEX16|sGF_INSTANCES,TorusFormat);

  // materials

  WaterMtrl = new sSimpleMaterial;
  WaterMtrl->Texture[0] = Tex;
  WaterMtrl->Flags = sMTRL_ZON|sMTRL_CULLOFF;
  WaterMtrl->Prepare(WaterFormat);

  TorusMtrl = new TorusShader;
  TorusMtrl->Texture[0] = Tex;
  TorusMtrl->Flags = sMTRL_ZON|sMTRL_CULLOFF;
  TorusMtrl->Prepare(TorusFormat);
}

void MyApp::CreateWater()
{
  sU16 *ip;
  WaterStream0 *vp0;
  sF32 fx,fy;

  // define a vertex format for multiple vertex streams

  static sU32 desc[] = 
  {
    sVF_STREAM0|sVF_POSITION,
    sVF_STREAM1|sVF_COLOR0,
    sVF_STREAM1|sVF_UV0,
    0
  };

  WaterFormat = sCreateVertexFormat(desc);

  // geometry with
  // - static index buffer
  // - static vertex buffer stream 0
  // - dynamic vertex buffer stream 1

  WaterGeo = new sGeometry();
  WaterGeo->Init(sGF_TRILIST|sGF_INDEX16,WaterFormat);

  // load static index buffer

  WaterGeo->BeginLoadIB((GridX-1)*(GridY-1)*6,sGD_STATIC,(void **)&ip);
  for(sInt y=0;y<GridY-1;y++)
    for(sInt x=0;x<GridX-1;x++)
      sQuad(ip,x+y*GridX,0,1,1+GridX,0+GridX);
  WaterGeo->EndLoadIB();

  // load static vertex stream 1

  WaterGeo->BeginLoadVB(GridX*GridY,sGD_STATIC,(void **)&vp0,1);
  for(sInt y=0;y<GridY;y++)
  {
    for(sInt x=0;x<GridX;x++)
    {
      fx = x/(GridX-1.0f);
      fy = y/(GridY-1.0f);
      vp0->c0 = 0xff808080;
      vp0->u0 = fx;
      vp0->v0 = fy;
      vp0++;
    }
  }
  WaterGeo->EndLoadVB(-1,1);
}

void MyApp::CreateTorus()
{
  const sInt TessX = 16;
  const sInt TessY = 16;
  const sF32 ri = 0.1f;
  const sF32 ro = 1.0f;
  sU16 *ip;
  sVertexSingle *vp;

  static sU32 desc[] = 
  {
    sVF_STREAM0|sVF_POSITION,
    sVF_STREAM0|sVF_COLOR0,
    sVF_STREAM0|sVF_UV0,
    sVF_STREAM1|sVF_UV1|sVF_F4|sVF_INSTANCEDATA,
    sVF_STREAM1|sVF_UV2|sVF_F4|sVF_INSTANCEDATA,
    sVF_STREAM1|sVF_UV3|sVF_F4|sVF_INSTANCEDATA,
    0
  };

  TorusFormat = sCreateVertexFormat(desc);

  TorusGeo = new sGeometry;
  TorusGeo->Init(sGF_TRILIST|sGF_INDEX16|sGF_INSTANCES,TorusFormat);

  TorusGeo->BeginLoadIB(TessX*TessY*6,sGD_STATIC,(void **)&ip);
  for(sInt y=0;y<TessY;y++)
  {
    for(sInt x=0;x<TessX;x++)
    {
      sQuad(ip,0,
        (x+0)%TessX+((y+0)%TessY)*TessX,
        (x+0)%TessX+((y+1)%TessY)*TessX,
        (x+1)%TessX+((y+1)%TessY)*TessX,
        (x+1)%TessX+((y+0)%TessY)*TessX);
    }
  }
  TorusGeo->EndLoadIB();

  TorusGeo->BeginLoadVB(TessX*TessY,sGD_STATIC,(void **)&vp);
  for(sInt y=0;y<TessY;y++)
  {
    for(sInt x=0;x<TessX;x++)
    {
      sF32 fx = x*sPI2F/TessX;
      sF32 fy = y*sPI2F/TessY;

      vp->px = sFSin(fy)*(ro+sFSin(fx)*ri);
      vp->py = sFCos(fx)*ri;
      vp->pz = sFCos(fy)*(ro+sFSin(fx)*ri);
      vp->c0 = 0xff6080c0;
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
  delete WaterGeo;
  delete TorusGeo;
  delete QuadGeo;
  delete WaterMtrl;
  delete TorusMtrl;
  delete Tex;
  delete InstGeo;
  delete MergeGeo;
}

/****************************************************************************/

void MyApp::OnPaint3D()
{
  sVector30 n;
  sVector31 p;
  sF32 fx,fy,ax,ay;
  const sF32 s = 32;
  WaterStream1 *vp1;
  sMatrix34 mat;
  sMatrix44 mat4;
  sVector4 *vpi;

  // timer

  Timer.OnFrame(sGetTime());
  sInt time = Timer.GetTime();

  // init viewport

  View.Camera.EulerXYZ(0.3f,time*0.0001f,0);
  View.Camera.l = sVector31(View.Camera.k * -25);
  View.Model.Init();
  View.SetTargetCurrent();
  View.SetZoom(1.5f);
  View.Prepare();

  // start painting

  sSetTarget(sTargetPara(sST_CLEARALL,0xff405060));
  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env);

  // water: update only stream1


  if(1)
  {
    WaterMtrl->Set(&cb);
    WaterGeo->BeginLoadVB(GridX*GridY,sGD_STREAM,(void **)&vp1,0);
    for(sInt y=0;y<GridY;y++)
    {
      for(sInt x=0;x<GridX;x++)
      {
        fx = x/(GridX-1.0f);
        fy = y/(GridY-1.0f);
        ax = fx*10+time*0.002f;
        ay = fy*12+time*0.003f;
        vp1->px = (fx-0.5f + sFCos(ax)*0.03f)*s;
        vp1->py = sFSin(ax)+sFSin(ay);
        vp1->pz = (fy-0.5f + sFCos(ay)*0.03f)*s;
        vp1++;
      }
    }
    WaterGeo->EndLoadVB(-1,0);
    WaterGeo->Draw();
  }

  // quads: i want to see more than 0x2000 quads to test quad splitting

  if(1)
  {
//    const sInt QuadXY=192;      // stress test: more than 0x2000 quads
//    const sF32 qs = 0.03f;
    const sInt QuadXY=16;       // easy test.
    const sF32 qs = 0.33f;
    sMatrix34 mat;
    mat = View.ModelView;
    mat.Trans3();
    sVertexSingle *vp;
    sVector31 v;

    WaterMtrl->Set(&cb);
    QuadGeo->BeginLoadVB(QuadXY*QuadXY*4,sGD_STREAM,(void **)&vp);
    for(sInt y=0;y<QuadXY;y++)
    {
      for(sInt x=0;x<QuadXY;x++)
      {
        fx = x/(QuadXY-1.0f);
        fy = y/(QuadXY-1.0f);
        ax = fx*10+time*0.002f;
        ay = fy*12+time*0.003f;
        v.x = (fx-0.5f + sFCos(ax)*0.03f)*s;
        v.y = sFSin(ax)+sFSin(ay);
        v.z = (fy-0.5f + sFCos(ay)*0.03f)*s;
        vp[0].Init(v-mat.i*qs-mat.j*0   ,0xffa08060,0     ,0);
        vp[1].Init(v-mat.i*qs+mat.j*qs*4,0xffa08060,0.125f,0);
        vp[2].Init(v+mat.i*qs+mat.j*qs*4,0xffa08060,0.125f,0.125f);
        vp[3].Init(v+mat.i*qs-mat.j*0   ,0xffa08060,0     ,0.125f);
        vp+=4;
      }
    }
    QuadGeo->EndLoadVB();
    QuadGeo->Draw();
  }

  // torus: paint a ring of rings

  if(1)
  {
    const sInt instcount = 250;
    const sF32 instscale = 0.01f;
    TorusGeo->BeginLoadVB(instcount,sGD_STREAM,(void **)&vpi,1);
    sF32 ftime = time*0.01f;
    for(sInt i=0;i<instcount;i++)
    {
      mat.EulerXYZ(ftime*0.11f,ftime*0.12f,ftime*0.13f);
      mat.Scale(instscale*(i+4),instscale*(i+4),instscale*(i+4));
      mat.l.y = 5;
      mat4 = mat;
      mat4.Trans4();

      *vpi++ = mat4.i;
      *vpi++ = mat4.j;
      *vpi++ = mat4.k;

      ftime += 1.0f;
    }
    TorusGeo->EndLoadVB(-1,1);

    TorusMtrl->Set(&cb);
    TorusGeo->Draw(0,0,instcount);
  }

  // another way to do it: merge geometries

  if(1)
  {
    // some times, different streams need to be combined
    // here we have the instance-matrices in a different stream as the vertices

    const sInt instcount = 20;
    InstGeo->BeginLoadVB(instcount,sGD_STREAM,(void **)&vpi,1);
    for(sInt i=0;i<instcount;i++)
    {
      mat.EulerXYZ(0,time*0.001f,0);
      mat.Scale(4,1,4);
      mat.l.y = 1+i*0.5f;
      mat4 = mat;
      mat4.Trans4();

      *vpi++ = mat4.i;
      *vpi++ = mat4.j;
      *vpi++ = mat4.k;
    }
    InstGeo->EndLoadVB(-1,1);

    // we can merge them. this is a fast operation. no format checks are done, so merge only what fits.
    MergeGeo->Merge(TorusGeo,InstGeo);

    // now paint the merged gemometry
    TorusMtrl->Set(&cb);
    MergeGeo->Draw(0,0,instcount);
  }

  // paint debug info

  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
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
  sSetWindowName(L"Vertex Buffer Tricks");
}

/****************************************************************************/

