// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "appbabewatch.hpp"
#include "_xsi.hpp"
#include "genbitmap.hpp"

/****************************************************************************/

sBabewatchApp::sBabewatchApp()
{
  sTexInfo ti;
  sInt i;
  sBitmap *bm;
  GenBitmap *gbm;
  GenMatPass *gmp;
  sU32 *p,*s;
  sXSILoader *load;
  sU8 *compact,*cs;
  sInt4 col0,col1,col2;

// window

  Status = new sStatusBorder;
  AddBorder(Status);
  Status->SetTab(0,Stat1,250);
  Status->SetTab(1,Stat2,250);
  Status->SetTab(2,Stat3,250);
  Status->SetTab(3,Stat4,0);

// engine-stuf

  Flags |= sGWF_PAINT3D;
  Camera.Init();
  Camera.CamPos.l.Init(0,0,-15,-5);

  bm = new sBitmap;
  bm->Init(64,64);
  for(i=0;i<64*64;i++)
    bm->Data[i] = sGetRnd(0xffffff)|0xff000000;
  ti.Init(bm);
  Texture = sSystem->AddTexture(ti);

  s=p=Material;
  sSystem->StateBase(p,sMBF_ZON|sMBF_BLENDOFF,sMBM_TEX,0);
  sSystem->StateTex(p,0,sMTF_FILTER);
  sSystem->StateEnd(p,s,sizeof(Material));

  Mesh = new GenMesh;
  Mesh->Init(sGMF_POS,64*1024);
//  Mesh->Ring(4,0,1,0);

// generate mesh

  load = new sXSILoader;
  load->LoadXSI("c:/nxn/genthree/babe/babe_sprung.xsi");
//  load->LoadXSI("c:/nxn/genthree/babe/babe_all_anims.xsi");
//  load->LoadXSI("c:/nxn/genthree/babe/babe_spack.xsi");
  load->Optimise();
  Mesh->ImportXSI(load);
  delete load;

// save

  compact = new sU8[1024*1024];
  cs = compact;
  Mesh->WriteCompact(cs);
  sSystem->SaveFile("c:/nxn/genthree/babe.mesh",compact,cs-compact);
  delete Mesh;

// load;

  Mesh = new GenMesh;
  cs = compact;
  Mesh->ReadCompact(cs,sGMF_POS|sGMF_NORMAL|sGMF_UV0);

// update status

  sSPrintF(Stat1,sizeof(Stat1),"Bones %d  Keys %d  Curves %d",Mesh->BoneMatrix.Count,Mesh->KeyCount,Mesh->CurveCount);
  sSPrintF(Stat2,sizeof(Stat2),"Vert %d  Face %d  Edge %d",Mesh->Vert.Count,Mesh->Face.Count,Mesh->Edge.Count);
  sSPrintF(Stat3,sizeof(Stat3),"");
  sSPrintF(Stat4,sizeof(Stat4),"Size: %d",cs-compact);

// make material

  col0.Init(0xffff,0x0000,0x0000,0x0000);
  col1.Init(0x0000,0xffff,0x0000,0x0000);
  col2.Init(0xffff,0xffff,0xffff,0x0000);

  Bitmap_SetSize(8<<16,8<<16);
  gbm = Bitmap_Cell(col0,col1,col2,50,0,0x10000,0x02000,7);

  Mesh->Mtrl[1].Material = new GenMaterial;
  Mesh->Mtrl[1].Material->Default();
  gmp = Mesh->Mtrl[1].Material->Passes[0];
  gmp->Programm = MPP_DYNAMIC;
  gmp->Texture[0] = gbm;
	sSystem->StateBase(gmp->StatePtr,sMBF_ZON|sMBF_LIGHT,sMBM_TEX,0);
  sSystem->StateTex(gmp->StatePtr,0,sMTF_FILTER|sMTF_MIPMAP);

// record

  sMatrix pmat;
  sVector pvec;
  pmat.InitEuler(0,0,0);
  pvec.Init(0.3f,0.3f,0.3f);

  Mesh->RecStoreMode();
  BoneIndex = Mesh->Bones(0.3f);
  Mesh->All2Mask(0x00010101,0);
  Mesh->Mask2Sel(0x00000100);
//  Mesh->Perlin(pmat,pvec);
  Mesh->Subdivide(1);
  Mesh->Subdivide(1);
  Mesh->All2Sel();
  Mesh->CalcNormals();

  Mesh->Prepare();

  delete[] compact;
}

sBabewatchApp::~sBabewatchApp()
{
  sSystem->RemTexture(Texture);
}

void sBabewatchApp::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(Mesh);
}

void sBabewatchApp::OnPaint3d(sViewport &view)
{
  sLightInfo light;

  static sF32 time;
  time+=0.0025f;
  time = Time*0.05f;

  Rot.Init3(0,Time*0.5,0);
  Frame.InitEuler(Rot.x,Rot.y,Rot.z);
  Frame.l.Init(0,-15,0,1);
//  Frame.l.Init(0,-14,0,1);

  view.ClearColor = 0xff000000;
  view.ClearFlags = sVCF_ALL;

  Camera.AspectX = view.Window.XSize();
  Camera.AspectY = view.Window.YSize();
//  Camera.CamPos.l.Init(0,0,-10,1);
  Camera.CamPos.l.Init(0,0,-15,1);
  sSystem->BeginViewport(view);
  sSystem->SetCamera(Camera);
  sSystem->SetMatrix(Frame);
//  sSystem->SetTexture(0,Texture,0);
//  sSystem->SetStates(Material);

  light.Init();
  light.Type = sLI_DIR;
  light.Diffuse = 0xffffff;
  light.Dir.Init(0,0,1,0);

  sSystem->ClearLights();
  sSystem->AddLight(light);
  sSystem->EnableLights(1);

  Mesh->BonesModify(BoneIndex,time*2);
  Mesh->RecReplay();
//  Mesh->PaintWire();
  Mesh->Paint();
//  Mesh->PaintSolid();

  sSystem->EndViewport();
}

void sBabewatchApp::OnKey(sU32 key)
{
  if(key==sKEY_APPCLOSE)
    Parent->RemChild(this);
}

void sBabewatchApp::OnDrag(sDragData &)
{
}

void sBabewatchApp::OnFrame()
{
  Time = sSystem->GetTime()*0.001f;
}

/****************************************************************************/

