/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "base/windows.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"

#include "util/taskscheduler.hpp"

/****************************************************************************/

struct MandelInfo
{
  sF64 px;
  sF64 py;
  sF64 sx;
  sF64 sy;
  sInt SizeX;
  sInt SizeY;
  sInt LinesPerBatch;
  sInt Batches;

  sU32 *Data;
  sInt Pitch;

  void InitData(sU8 *data,sInt pitch,sInt sx,sInt sy,sInt batches);
  void InitRect(sF64 cx,sF64 cy,sF64 sx,sF64 sy);
  void DoBatch(sInt line);
};

sInt mandel(sF64 fx,sF64 fy,sInt depth)
{
  sInt r = 0;
  sF64 ix,iy,tx,ty;
  ix = iy = 0;
  while(r<depth && ix*ix+iy*iy<4)
  {
    tx = ix*ix-iy*iy+fx;
    ty = 2*ix*iy+fy;
    ix = tx;
    iy = ty;
    r++;
  }

  return depth==r ? 0 : r;
}


void MandelInfo::InitData(sU8 *data,sInt pitch,sInt sx,sInt sy,sInt batches)
{
  Data = (sU32 *) data;
  Pitch = pitch/4;
  SizeX = sx;
  SizeY = sy;

  Batches = batches;
  LinesPerBatch = sy/batches;
  sVERIFY(sy == LinesPerBatch*Batches);
}

void MandelInfo::InitRect(sF64 cx_,sF64 cy_,sF64 sx_,sF64 sy_)
{
  px = cx_-sx_/2;
  py = cy_-sy_/2;
  sx = sx_;
  sy = sy_;
}

void MandelInfo::DoBatch(sInt b)
{
  for(sInt y=0;y<LinesPerBatch;y++)
  {
    sInt yy = (b*LinesPerBatch+y);
    sU32 *p = yy*Pitch + Data;
    for(sInt x=0;x<SizeX;x++)
    {
      sU8 c = sMin(255,mandel(px+sx*x/SizeX,py+sy*yy/SizeY,255));
      p[x] = (c<<0)|(c<<8)|(c<<16)|0xff000000;
    }
  }
}

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  // debug output

  Painter = new sPainter;
  Timer.SetHistoryMax(4);

  // multitasking

  sAddSched();
  sSchedMon = new sStsPerfMon;

  // geometry 

  Geo = new sGeometry();
  if(1)
    Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);
  else
    Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatSingle);

  // texture

  sImage img(512,512);
  img.Checker(0xffff8080,0xff80ff80,8,8);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);
  Tex[0] = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888|sTEX_DYNAMIC|sTEX_NOMIPMAPS);
  Tex[1] = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888|sTEX_DYNAMIC|sTEX_NOMIPMAPS);
  DBuf = 0;

  // material

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Texture[0] = Tex[0];
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Prepare(Geo->GetFormat());

  // light

  sClear(Env);
  Env.AmbientColor  = 0x00202020;
  Env.LightColor[0] = 0x00ffffff;
  Env.LightDir[0].Init(0,0,1);
  Env.Fix();
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex[0];
  delete Tex[1];
}

void TaskCode(sStsManager *m,sStsThread *th,sInt start,sInt count,void *data)
{
  sSchedMon->Begin(th->GetIndex(),0x40ff40);
  sVERIFY(count==1);
  MandelInfo *mi = (MandelInfo *)data;
  mi->DoBatch(start);

  sU32 *p = mi->Data + (start * mi->LinesPerBatch * mi->Pitch);
  sInt v= th->GetIndex();
  if(m->GetThreadCount()>4)
  {
    for(sInt i=0;i<6;i++)
    {
      *p++ = (v&1)?0xffff0000 : 0xff00ff00;
      *p++ = (v&1)?0xffff0000 : 0xff00ff00;
      *p++ = (v&1)?0xffff0000 : 0xff00ff00;
      *p++ = (v&1)?0xffff0000 : 0xff00ff00;
      v = v>>1;
    }
  }
  else
  {
    static const sU32 colors[] = { 0xff0000ff,0xff00ff00,0xffff0000,0xffffff00 };
    for(sInt i=0;i<8;i++)
      *p++ = colors[v&3];
  }
  sSchedMon->End(th->GetIndex());
}

void MyApp::Update(sTexture2D *tex)
{
  sU8 *data;
  sInt pitch;
  sVERIFY(tex->BitsPerPixel == 32);

  static sInt time=0;
  sU32 qual = sGetKeyQualifier();

  if(!(qual & sKEYQ_CTRL))
    time++;
  sF64 sx = 0.02f/(((time & 0xff)+1)/sF32(0xff));
  sF32 sy = sx;
  sF64 cx = -1.24090;
  sF64 cy = -0.15743;
  tex->BeginLoad(data,pitch,0);

  MandelInfo mi;
  mi.InitData(data,pitch,tex->SizeX,tex->SizeY,tex->SizeY);
  mi.InitRect(cx,cy,sx,sy);

  sStsWorkload *wl = sSched->BeginWorkload();

  sStsTask *task = wl->NewTask(TaskCode,&mi,tex->SizeY,0);
  wl->AddTask(task);
  sSchedMon->Begin(0,0x008000);
  sSched->StartWorkload(wl);
/*
  if(qual & sKEYQ_SHIFT)
    sSched->StartSingle();
  else
    sSched->Start();
  sSched->Finish();
  */
  tb.PrintF(L"%s",wl->PrintStat());
  sSched->SyncWorkload(wl);
  sSched->EndWorkload(wl);
  sSchedMon->End(0);

  tex->EndLoad();
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  // set rendertarget

  sSchedMon->Begin(0,0xff0000);
  sSetRendertarget(0,sCLEAR_ALL,0xff405060);
  // get timing

  Timer.OnFrame(sGetTime());
  static sInt time;
  if(sHasWindowFocus())
    time = Timer.GetTime();

  sF32 avg = Timer.GetAverageDelta();
  tb.Clear();
  tb.PrintF(L"%5.2ffps %5.3fms\n",1000/avg,avg);

  // update texture

  Mtrl->Texture[0] = Tex[DBuf];
  DBuf = (DBuf+1)&1;
  Update(Tex[DBuf]);

  // set camera

  time = time*0.01f;
  View.SetTargetCurrent();
  View.SetZoom(2.0f);
//  View.Model.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.Model.EulerXYZ(0,-0.25f*sPI2F,0);
  View.Model.l.Init(0,0,0);
  View.Camera.l.Init(0,0,-3.2f);
  View.Prepare();
 
  // set material

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env);
  Mtrl->Set(&cb);

  // load vertices and indices

  if(Geo->GetFormat()==sVertexFormatSingle)
  {
    sMatrix34 mat;
    sVector30 dir;
    sVertexSingle *vp=0;
    sU32 col = 0xffffffff;
    Geo->BeginLoadVB(4*6,sGD_STREAM,&vp);
    for(sInt i=0;i<6;i++)
    {
      mat.CubeFace(i);
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
    Geo->EndLoadVB();

    sU16 *ip=0;
    Geo->BeginLoadIB(6*6,sGD_STREAM,&ip);
    for(sInt i=0;i<6;i++)
      sQuad(ip,i*4,0,1,2,3);
    Geo->EndLoadIB();
  }
  else
  {
    sVertexStandard *vp=0;

    Geo->BeginLoadVB(24,sGD_STREAM,&vp);
    vp->Init(-1, 1,-1,  0, 0,-1, 1,0); vp++; // 3
    vp->Init( 1, 1,-1,  0, 0,-1, 1,1); vp++; // 2
    vp->Init( 1,-1,-1,  0, 0,-1, 0,1); vp++; // 1
    vp->Init(-1,-1,-1,  0, 0,-1, 0,0); vp++; // 0

    vp->Init(-1,-1, 1,  0, 0, 1, 1,0); vp++; // 4
    vp->Init( 1,-1, 1,  0, 0, 1, 1,1); vp++; // 5
    vp->Init( 1, 1, 1,  0, 0, 1, 0,1); vp++; // 6
    vp->Init(-1, 1, 1,  0, 0, 1, 0,0); vp++; // 7

    vp->Init(-1,-1,-1,  0,-1, 0, 1,0); vp++; // 0
    vp->Init( 1,-1,-1,  0,-1, 0, 1,1); vp++; // 1
    vp->Init( 1,-1, 1,  0,-1, 0, 0,1); vp++; // 5
    vp->Init(-1,-1, 1,  0,-1, 0, 0,0); vp++; // 4

    vp->Init( 1,-1,-1,  1, 0, 0, 1,0); vp++; // 1
    vp->Init( 1, 1,-1,  1, 0, 0, 1,1); vp++; // 2
    vp->Init( 1, 1, 1,  1, 0, 0, 0,1); vp++; // 6
    vp->Init( 1,-1, 1,  1, 0, 0, 0,0); vp++; // 5

    vp->Init( 1, 1,-1,  0, 1, 0, 1,0); vp++; // 2
    vp->Init(-1, 1,-1,  0, 1, 0, 1,1); vp++; // 3
    vp->Init(-1, 1, 1,  0, 1, 0, 0,1); vp++; // 7
    vp->Init( 1, 1, 1,  0, 1, 0, 0,0); vp++; // 6

    vp->Init(-1, 1,-1, -1, 0, 0, 1,0); vp++; // 3
    vp->Init(-1,-1,-1, -1, 0, 0, 1,1); vp++; // 0
    vp->Init(-1,-1, 1, -1, 0, 0, 0,1); vp++; // 4
    vp->Init(-1, 1, 1, -1, 0, 0, 0,0); vp++; // 7
    Geo->EndLoadVB();

    sU16 *ip=0;
    Geo->BeginLoadIB(6*6,sGD_STREAM,&ip);
    sQuad(ip,0, 0, 1, 2, 3);
    sQuad(ip,0, 4, 5, 6, 7);
    sQuad(ip,0, 8, 9,10,11);
    sQuad(ip,0,12,13,14,15);
    sQuad(ip,0,16,17,18,19);
    sQuad(ip,0,20,21,22,23);
    Geo->EndLoadIB();
  }

  // draw

  Geo->Draw();

  // debug output

  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->Print(10,10,tb.Get());
  Painter->End();

//  sSchedMon->Paint(sTargetSpec());
  sSchedMon->Scale = 19;
  sSchedMon->End(0);
  sSchedMon->FlipFrame();
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  sU32 key = ie.Key;
  if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key & sKEYQ_CTRL ) key |= sKEYQ_CTRL;
  switch(ie.Key)
  {
  case sKEY_ESCAPE:
  case sKEY_ESCAPE|sKEYQ_SHIFT:
    sExit();
    break;
  }
}

/****************************************************************************/

// register application class

#if sPLATFORM==sPLAT_WINDOWS
sINITMEM(0,0);
#endif

void sMain()
{
  sSetWindowName(L"Multithreaded Mandelbrot");

  sInit(sISF_3D|sISF_CONTINUOUS|sISF_FSAA/*|sISF_NOVSYNC*//*|sISF_FULLSCREEN*/,1280,720);
  sSetApp(new MyApp());
}

/****************************************************************************/


/****************************************************************************/

