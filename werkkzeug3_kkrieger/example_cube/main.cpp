// This file is distributed under a BSD license. See LICENSE.txt for details.


#include "_util.hpp"
#include "_gui.hpp"
#include "_startdx.hpp"
#include "material11.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Class for the example                                              ***/
/***                                                                      ***/
/****************************************************************************/

class MyApp_ : public sObject
{
  sF32 Time;
  sInt Texture;
  sMaterial11 *Mtrl;
  sInt GeoHandle;
public:
  MyApp_();
  ~MyApp_();
  void Tag();
  void OnPaint();
  void OnKey(sU32 key);
  void OnFrame();
};

MyApp_ *MyApp;

/****************************************************************************/
/***                                                                      ***/
/***   Event Dispatcher - the system does not define a virtual base class ***/
/***                                                                      ***/
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
    MyApp = new MyApp_;
    sBroker->AddRoot(MyApp);
    break;

  case sAPPCODE_EXIT:
    sBroker->RemRoot(MyApp);
    sBroker->Free();
    sSystem->SetSoundHandler(0,0,0);
    sBroker->Free();
    break;

  case sAPPCODE_KEY:
    MyApp->OnKey(value);
    break;

  case sAPPCODE_PAINT:
    MyApp->OnPaint();
    break;

  case sAPPCODE_TICK:
    MyApp->OnFrame();
    break;

  default:
    return sFALSE;
  }
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Actual implementation of a spinning cube...                        ***/
/***                                                                      ***/
/****************************************************************************/

MyApp_::MyApp_()
{
  sU16 *bm;
  sInt i;
  sInt ok;

  // create texture

  bm = new sU16[16*16*4];
  for(i=0;i<256;i++)
  {
    bm[i*4+0] = sGetRnd(0x7fff);
    bm[i*4+1] = sGetRnd(0x7fff);
    bm[i*4+2] = sGetRnd(0x7fff);
    bm[i*4+3] = 0x7fff;
  }
  Texture = sSystem->AddTexture(16,16,sTF_A8R8G8B8,bm);
  delete[] bm;

  // create material

  Mtrl = new sMaterial11;
  Mtrl->ShaderLevel = sPS_00;
  Mtrl->BaseFlags = sMBF_ZON|sMBF_NONORMAL;
  Mtrl->Color[0] = 0xffc0c0c0;
  Mtrl->Combiner[sMCS_TEX0] = sMCOA_SET;
  Mtrl->SetTex(0,Texture);
  ok = Mtrl->Compile();
  sVERIFY(ok);

  // create (dynamic) geometry handle

  GeoHandle = sSystem->GeoAdd(sFVF_STANDARD,sGEO_TRI|sGEO_IND32B);
}

/****************************************************************************/

MyApp_::~MyApp_()
{
  sSystem->GeoRem(GeoHandle);
  sRelease(Mtrl);
  sSystem->RemTexture(Texture);
}

/****************************************************************************/

void MyApp_::Tag()
{
}

/****************************************************************************/

void MyApp_::OnFrame()
{
  Time = sSystem->GetTime()*0.001f;
}

/****************************************************************************/

void MyApp_::OnKey(sU32 key)
{
  if((key&0x8001ffff)==sKEY_ESCAPE)
    sSystem->Exit();
}

/****************************************************************************/

void MyApp_::OnPaint()
{
  sVertexStandard *vp;
  sInt *ip;
  sInt i,j,s0,s1;
  sViewport view;
  sMaterialEnv env;

  static sF32 Planes[10][3] = 
  {
    {  1,0,0 },
    { -1,0,0 },
    { 0, 1,0 },
    { 0,-1,0 },
    { 0,0, 1 },
    { 0,0,-1 },
    {  1,0,0 },
    { -1,0,0 },
    { 0, 1,0 },
    { 0,-1,0 },
  };

// set up transform

  view.Init();

  env.Init();
  env.ModelSpace.InitEuler(Time*0.1f,Time*0.4f,Time*0.5f);
  env.CameraSpace.l.z = -5.0f;
  env.ZoomX = 1.0f;
  env.ZoomY = 1.0f*view.Window.XSize()/view.Window.YSize();

// begin drawing

  sSystem->SetViewport(view);
  sSystem->Clear(sVCF_ALL,0xff203040);
  sSystem->SetViewProject(&env);
  Mtrl->Set(env);
  sSystem->GeoBegin(GeoHandle,24,6*6,(sF32 **)&vp,(void **)&ip);

// drawing

  for(i=0;i<6;i++)
  {
    for(j=0;j<4;j++)
    {
      s0 = ((j+0)&2) ? -1 : 1;
      s1 = ((j+1)&2) ? -1 : 1;
      if(i&1) s0*=-1;
      vp->x = Planes[i][0] + s0*Planes[i+2][0] + s1*Planes[i+4][0];
      vp->y = Planes[i][1] + s0*Planes[i+2][1] + s1*Planes[i+4][1];
      vp->z = Planes[i][2] + s0*Planes[i+2][2] + s1*Planes[i+4][2];
      vp->nx = Planes[i][0];
      vp->ny = Planes[i][1];
      vp->nz = Planes[i][2];
      vp->u = ((j+0)&2) ? 1 : 0;
      vp->v = ((j+1)&2) ? 1 : 0;
      vp++;
    }
    sQuad(ip,i*4+3,i*4+2,i*4+1,i*4+0);
  }

// finish!

  sSystem->GeoEnd(GeoHandle);
  sSystem->GeoDraw(GeoHandle);
}

/****************************************************************************/
/****************************************************************************/

