/****************************************************************************/

#include "main.hpp"
#include "base/graphics.hpp"
#include "base/system.hpp"
#include "base/windows.hpp"
#include "util/image.hpp"

/****************************************************************************/

MyApp::MyApp()
{
  // debug output

  Painter = new sPainter;

  // upload dxt test block

  sImageData imgd;
  imgd.Init2(sTEX_DXT1|sTEX_2D|sTEX_NOMIPMAPS,1,8,8,1);
  sU8 *dp = (sU8 *) imgd.Data;

  // some platforms don't support anything smaller than 8x8, so
  // put the block into the texture 4 times
  const sU8 block[]={0x20,0x10,0x00,0x00,0xaa,0xaa,0xaa,0xaa};
  sCopyMem(dp,block,8);
  sCopyMem(dp+8,block,8);
  sCopyMem(dp+16,block,8);
  sCopyMem(dp+24,block,8);

  Tex = imgd.CreateTexture()->CastTex2D();

  // geometry 

  Geo = new sGeometry();
  Geo->Init(sGF_QUADLIST,sVertexFormatSingle);

  sVertexSingle *vp;
  Geo->BeginLoadVB(4,sGD_STATIC,&vp);
  for(sInt i=0;i<4;i++)
  {
    sInt j = i^(i>>1);
    vp->px = ((j&1) ? Tex->SizeX : 0) - 0.5f;
    vp->py = ((j&2) ? Tex->SizeY : 0) - 0.5f;
    vp->pz = 0.5f;
    vp->c0 = 0xffffffff;
    vp->u0 = ((j&1) ? 1.0f : 0.0f) + 0.0f / Tex->SizeX;
    vp->v0 = ((j&2) ? 1.0f : 0.0f) + 0.0f / Tex->SizeY;
    vp++;
  }
  Geo->EndLoadVB();

  // material

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLOFF;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Prepare(Geo->GetFormat());

  // light

  sClear(Env);
  Env.Fix();
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex;
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  // set rendertarget
  sSetRendertarget(0,sCLEAR_ALL,0xff000000);

  View.SetTargetCurrent();
  View.Orthogonal = sVO_PIXELS;
  View.Prepare();

  // set material
  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env);
  Mtrl->Set(&cb);

  // draw

  Geo->Draw();

  // grab rendertarget

  sTextureFlags flags;
  const sU8 *data;
  sS32 pitch;
  sBeginSaveRT(data,pitch,flags);
  sInt realR = data[2];
  sInt realG = data[1];
  sEndSaveRT();

  // output result
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,~0,2);
  Painter->PrintF(10,10,L"r=%d g=%d biasR=%d biasG=%d",realR,realG,realR-10,realG-2);
  Painter->End();
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  if (ie.Key==sKEY_ESCAPE) 
  {
    sExit(); 
  }
}

/****************************************************************************/

void sMain()
{
  sSetWindowName(L"dxtdecomp");
  sInit(sISF_3D/*|sISF_REFRAST|sISF_FULLSCREEN*/,640,480);
  sSetApp(new MyApp());
}

/****************************************************************************/

