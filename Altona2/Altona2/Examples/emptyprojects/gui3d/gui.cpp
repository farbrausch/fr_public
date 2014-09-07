/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "altona2/libs/base/base.hpp"
#include "altona2/libs/util/graphicshelper.hpp"
#include "shaders.hpp"
#include "gui.hpp"
#include "doc.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

MainWindow::MainWindow()
{
  Doc = new wDocument;
  ParaWin = 0;
  RenderWin = 0;
}

void MainWindow::OnInit()
{
  ParaWin = new sGridFrame();
  RenderWin = new RenderWindow();

  sVSplitterFrame *vbox = new sVSplitterFrame;

  sToolBorder *tb = new sToolBorder;
  tb->AddChild(new sButtonControl(sGuiMsg(this,&MainWindow::CmdEdit),"Edit",sBS_Menu));

  vbox->AddChild(ParaWin);
  vbox->AddChild(RenderWin);
  vbox->SetSize(0,400);
  AddChild(vbox);
  AddBorder(tb);

  AddKey("Exit",sKEY_Escape|sKEYQ_Shift,sGuiMsg(this,&sWindow::CmdExit));

  CmdSetPara();
  CmdUpdate();
}

MainWindow::~MainWindow()
{
  delete Doc;
}

void MainWindow::CmdEdit()
{
  sMenuFrame *mf = new sMenuFrame(this);
  mf->AddItem(sGuiMsg(this,&MainWindow::CmdUpdate),0,"Update");
  mf->AddSpacer();
  mf->AddItem(sGuiMsg(this,&MainWindow::CmdExit),0,"Exit");
  mf->StartMenu();
}

void MainWindow::CmdUpdate()
{
  Doc->Update();
}

void MainWindow::CmdSetPara()
{
  ParaWin->Clear();
  ParaWin->GridX = 8;
  sGridFrameHelper gh(ParaWin);

  gh.Left = 2;
  gh.Normal = 2;
  gh.Wide = 2;
  gh.Right = 8;
  gh.ChangeMsg = sGuiMsg(this,&MainWindow::CmdUpdate);
  
  gh.Group("Animation");
  gh.Label("Speed");
  gh.BeginLink();
  gh.Float(RenderWin->Speed.x,0,100,0.1f);
  gh.Float(RenderWin->Speed.y,0,100,0.1f);
  gh.Float(RenderWin->Speed.z,0,100,0.1f);
  gh.EndLink();

  gh.ChangeMsg = sGuiMsg();

  gh.Group("Rendering");
  gh.Label("BackColor");
  gh.Color(RenderWin->BackColor);
  gh.Label("Flags");
  gh.Choice(RenderWin->Fps,"-|FPS");
  gh.Label("Multisample");
  gh.ChangeMsg = sGuiMsg(this,&MainWindow::CmdSetPara);

  sString<2048> buffer = "0 off";
  for(sInt i=1;i<4;i++)
    if(RenderWin->GetMsQuality(1<<i)>0)
      buffer.AddF("|%d %d",1<<i,1<<i);
  gh.Choice(RenderWin->MsCount,sPoolString(buffer));

  sInt ms = RenderWin->GetMsQuality(RenderWin->MsCount);
  buffer = "0 0";
  for(sInt i=1;i<ms;i++)
    buffer.AddF("|%d %d",i,i);
  gh.Choice(RenderWin->MsQuality,sPoolString(buffer));
}


/****************************************************************************/
/****************************************************************************/

RenderWindow::RenderWindow()
{
  Mtrl = 0;
  Tex = 0;
  Geo = 0;
  cbv0 = 0;

  Rot.Set(0,0,0);
  Speed.Set(14,16,19);
  LastTime = 0;
}

void RenderWindow::OnInit()
{
  s3dWindow::OnInit();

  const sInt sx=64;
  const sInt sy=64;
  sU32 tex[sy][sx];
  for(sInt y=0;y<sy;y++)
    for(sInt x=0;x<sx;x++)
      tex[y][x] = ((x^y)&8) ? 0xffff8080 : 0xff80ff80;
  Tex = new sResource(Adapter(),sResTexPara(sRF_BGRA8888,sx,sy,1),&tex[0][0],sizeof(tex));


  Mtrl = new sMaterial(Adapter());
  Mtrl->SetShaders(CubeShader.Get(0));
  Mtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
  Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
  Mtrl->Prepare(Adapter()->FormatPNT);


  const sInt ic = 6*6;
  const sInt vc = 24;
  static const sU16 ib[ic] =
  {
     0, 1, 2, 0, 2, 3,
     4, 5, 6, 4, 6, 7,
     8, 9,10, 8,10,11,

    12,13,14,12,14,15,
    16,17,18,16,18,19,
    20,21,22,20,22,23,
  };
  struct vert
  {
    sF32 px,py,pz;
    sF32 nx,ny,nz;
    sF32 u0,v0;
  };
  static const vert vb[vc] =
  {
    { -1, 1,-1,   0, 0,-1, 0,0, },
    {  1, 1,-1,   0, 0,-1, 1,0, },
    {  1,-1,-1,   0, 0,-1, 1,1, },
    { -1,-1,-1,   0, 0,-1, 0,1, },

    { -1,-1, 1,   0, 0, 1, 0,0, },
    {  1,-1, 1,   0, 0, 1, 1,0, },
    {  1, 1, 1,   0, 0, 1, 1,1, },
    { -1, 1, 1,   0, 0, 1, 0,1, },

    { -1,-1,-1,   0,-1, 0, 0,0, },
    {  1,-1,-1,   0,-1, 0, 1,0, },
    {  1,-1, 1,   0,-1, 0, 1,1, },
    { -1,-1, 1,   0,-1, 0, 0,1, },

    {  1,-1,-1,   1, 0, 0, 0,0, },
    {  1, 1,-1,   1, 0, 0, 1,0, },
    {  1, 1, 1,   1, 0, 0, 1,1, },
    {  1,-1, 1,   1, 0, 0, 0,1, },

    {  1, 1,-1,   0, 1, 0, 0,0, },
    { -1, 1,-1,   0, 1, 0, 1,0, },
    { -1, 1, 1,   0, 1, 0, 1,1, },
    {  1, 1, 1,   0, 1, 0, 0,1, },

    { -1, 1,-1,  -1, 0, 0, 0,0, },
    { -1,-1,-1,  -1, 0, 0, 1,0, },
    { -1,-1, 1,  -1, 0, 0, 1,1, },
    { -1, 1, 1,  -1, 0, 0, 0,1, },
  };

  Geo = new sGeometry(Adapter());
  Geo->SetIndex(sResBufferPara(sRBM_Index|sRU_Static,sizeof(sU16),ic),ib);
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(vert),vc),vb);
  Geo->Prepare(Adapter()->FormatPNT,sGMP_Tris|sGMF_Index16);

  cbv0 = new sCBuffer<CubeShader_cbvs>(Adapter(),sST_Vertex,0);
}

RenderWindow::~RenderWindow()
{
  delete Mtrl;
  delete Tex;
  delete Geo;
  delete cbv0;
}

void RenderWindow::Render(const sTargetPara &tp)
{
  sU32 utime = sGetTimeMS();

  sF32 time = (utime-LastTime)*1.0f;
  if(LastTime==0)
    time = 0;
  LastTime = utime;

  Rot = Rot + Speed*time;

  sViewport view;
  view.Camera.k.w = -4;
  view.Model = sEulerXYZ(Rot*s2Pi*0.00001f);
  view.ZoomX = 1/tp.Aspect;
  view.ZoomY = 1;
  view.Prepare(tp);

  cbv0->Map();
  cbv0->Data->mat = view.MS2SS;
  cbv0->Data->ldir.Set(-view.MS2CS.k.x,-view.MS2CS.k.y,-view.MS2CS.k.z);
  cbv0->Unmap();

  Adapter()->ImmediateContext->Draw(sDrawPara(Geo,Mtrl,cbv0));
}

/****************************************************************************/
