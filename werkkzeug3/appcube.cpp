// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "appcube.hpp"
#include "_startdx.hpp"

/****************************************************************************/

sCubeApp::sCubeApp()
{
  sU16 *bm;
  sInt i;

  Flags |= sGWF_PAINT3D;

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

// load vertex and pixel shader and create material setup
  sU32 *vs = (sU32 *) sSystem->LoadFile("test.vso");
  sU32 *ps = (sU32 *) sSystem->LoadFile("test.pso");
  static const sU32 states[] = {
    // general setup
    sD3DRS_ZENABLE,                   sD3DZB_TRUE,
    sD3DRS_CULLMODE,                  sD3DCULL_CCW,
    sD3DRS_CLIPPING,                  1,
    sD3DRS_ALPHABLENDENABLE,          0,

    // texture sampler
    sD3DSAMP_0|sD3DSAMP_MINFILTER,    sD3DTEXF_POINT,
    sD3DSAMP_0|sD3DSAMP_MAGFILTER,    sD3DTEXF_POINT,
    sD3DSAMP_0|sD3DSAMP_MIPFILTER,    sD3DTEXF_LINEAR,
    sD3DSAMP_0|sD3DSAMP_ADDRESSU,     sD3DTADDRESS_WRAP,
    sD3DSAMP_0|sD3DSAMP_ADDRESSV,     sD3DTADDRESS_WRAP,

    // texture stage
    sD3DTSS_0|sD3DTSS_TEXCOORDINDEX,  0,

    // end
    ~0U, ~0U
  };

  MtrlSetup = sSystem->MtrlAddSetup(states,vs,ps);
  sVERIFY(MtrlSetup != sINVALID);

  delete[] vs;
  delete[] ps;

// set up material instance
  Inst.NumTextures = 1;
  Inst.Textures[0] = Texture;
  Inst.NumVSConstants = 4;
  Inst.VSConstants = vc;
  Inst.NumPSConstants = 1;
  Inst.PSConstants = pc;
}

sCubeApp::~sCubeApp()
{
  sSystem->MtrlRemSetup(MtrlSetup);
  sSystem->RemTexture(Texture);
}

void sCubeApp::Tag()
{
  sGuiWindow::Tag();
}

void sCubeApp::OnPaint()
{
}

void sCubeApp::OnPaint3d(sViewport &view)
{
  sVertexStandard *vp;
  sU16 *ip;
  sInt i,j,s0,s1;
  sInt handle;
//  sVector c[4];
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

  Rot.Init3(Time*0.1,Time*0.4,Time*0.5);
  Frame.InitEuler(Rot.x,Rot.y,Rot.z);

// set up transform
  sMatrix m1,m2,m3;
  m1 = Frame;
  m1.l.z += 5.0f;
  m2.Init();
  m2.j.y = 1.0f*view.Window.XSize()/view.Window.YSize();
  m2.k.w = 1.0f;
  m2.l.z = -0.125f;
  m2.l.w = 0.0f;

  m3.Mul4(m1,m2);
  m3.Trans4();

  vc[0] = m3.i;
  vc[1] = m3.j;
  vc[2] = m3.k;
  vc[3] = m3.l;
  pc[0].Init(1,1,1,1);

  view.ClearColor = 0xff203040;
  view.ClearFlags = sVCF_ALL;

  sSystem->BeginViewport(view);

  sSystem->MtrlSetSetup(MtrlSetup);
  sSystem->MtrlSetInstance(Inst);
  handle = sSystem->GeoAdd(sFVF_STANDARD,sGEO_TRI);
  sSystem->GeoBegin(handle,24,6*6,(sF32 **)&vp,&ip);
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

  sSystem->GeoEnd(handle);
  sSystem->GeoDraw(handle);
  sSystem->GeoRem(handle);

  /*MtrlEnv.ZoomY = 1.0f*view.Window.XSize()/view.Window.YSize();
  MtrlEnv.ModelSpace = Frame;

  sSystem->SetViewProject(&MtrlEnv);
  sSystem->MtrlSet(MtrlHandle,&MtrlInfo,&MtrlEnv);  
  handle = sSystem->GeoAdd(sFVF_STANDARD,sGEO_TRI);
  sSystem->GeoBegin(handle,24,6*6,(sF32 **)&vp,&ip);
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

  sSystem->GeoEnd(handle);
  sSystem->GeoDraw(handle);
  sSystem->GeoRem(handle);*/

  sSystem->EndViewport();
}

void sCubeApp::OnKey(sU32 key)
{
  if(key==sKEY_APPCLOSE)
    Parent->RemChild(this);
}

void sCubeApp::OnDrag(sDragData &)
{
}

void sCubeApp::OnFrame()
{
  Time = sSystem->GetTime()*0.001f;
}

/****************************************************************************/

