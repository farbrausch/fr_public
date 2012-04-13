// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "appcube.hpp"

/****************************************************************************/

sCubeApp::sCubeApp()
{
  sTexInfo ti;
  sInt i;
  sBitmap *bm;
  sU32 *p,*s;

  Flags |= sGWF_PAINT3D;
  Camera.Init();
  Camera.CamPos.l.z = -5;

  bm = new sBitmap;
  bm->Init(16,16);
  for(i=0;i<256;i++)
    bm->Data[i] = sGetRnd(0xffffff)|0xff000000;
  ti.Init(bm);
  Texture = sSystem->AddTexture(ti);

  s=p=Material;
  sSystem->StateBase(p,sMBF_ZON|sMBF_BLENDOFF,sMBM_TEX,0);
  sSystem->StateTex(p,0,sMTF_CLAMP);
  sSystem->StateEnd(p,s,sizeof(Material));
}

sCubeApp::~sCubeApp()
{
  sSystem->RemTexture(Texture);
}

void sCubeApp::Tag()
{
  sGuiWindow::Tag();
}

void sCubeApp::OnPaint3d(sViewport &view)
{
  sVertex3d *vp;
  sU16 *ip;
  sInt i;
  sInt handle;

  Rot.Init3(Time*0.1,Time*0.4,Time*0.5);
  Frame.InitEuler(Rot.x,Rot.y,Rot.z);

  view.ClearColor = 0xff000000;
  view.ClearFlags = sVCF_ALL;

  Camera.AspectX = view.Window.XSize();
  Camera.AspectY = view.Window.YSize();

  sSystem->BeginViewport(view);
  sSystem->SetCamera(Camera);
  sSystem->SetMatrix(Frame);
  sSystem->SetTexture(0,Texture,0);
  sSystem->SetStates(Material);

  handle = sSystem->GeoAdd(sFVF_DEFAULT3D,sGEO_TRI);
  sSystem->GeoBegin(handle,8,6*6,(sF32 **)&vp,&ip);
  for(i=0;i<8;i++)
  {
    vp->x  = ((i+0)&2) ? -1 : 1;
    vp->y  = ((i+1)&2) ? -1 : 1;
    vp->z  = ((i+0)&4) ? -1 : 1;
    vp->nx = 0;
    vp->ny = 0;
    vp->nz = 0;
    vp->u  = ((i+0)&2) ? 1 : 0;
    vp->v  = ((i+1)&2) ? 1 : 0;
    vp++;
  }
  sQuad(ip,3,2,1,0);
  sQuad(ip,4,5,6,7);
  sQuad(ip,0,1,5,4);
  sQuad(ip,1,2,6,5);
  sQuad(ip,2,3,7,6);
  sQuad(ip,3,0,4,7);
  sSystem->GeoEnd(handle);
  sSystem->GeoDraw(handle);
  sSystem->GeoRem(handle);

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

