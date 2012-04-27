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
//#define sPEDANTIC_WARN 1

#include "gui.hpp"
#include "doc.hpp"
#include "gui/gui.hpp"
#include "util/shaders.hpp"

MainWindow *App;

/****************************************************************************/

MainWindow::MainWindow()
{
  App = this;
  new Document();

  AddChild(sWire = new sWireMasterWindow);
  sWire->AddWindow(L"main",this);
  sWire->AddWindow(L"Bla",BlaWin = new sWindow);

  Cube1Win = new WinCube;
  Cube1Win->InitWire(L"Cube1");
  Cube2Win = new WinCube;
  Cube2Win->InitWire(L"Cube2");

  sWire->AddKey(L"main",L"New",sMessage(this,&MainWindow::CmdNew));
  sWire->AddKey(L"main",L"Open",sMessage(this,&MainWindow::CmdOpen));
  sWire->AddKey(L"main",L"Save",sMessage(this,&MainWindow::CmdSave));
  sWire->AddKey(L"main",L"SaveAs",sMessage(this,&MainWindow::CmdSaveAs));
  sWire->AddKey(L"main",L"Exit",sMessage(this,&MainWindow::CmdExit));

  sWire->ProcessFile(L"wireapp.wire.txt");
  sWire->ProcessEnd();

  if(sGetShellParameter(0,0))
  {
    Doc->Filename = sGetShellParameter(0,0);
    Doc->Load();
  }
}

MainWindow::~MainWindow()
{
}

void MainWindow::Tag()
{
  sWindow::Tag();
  BlaWin->Need();
  Doc->Need();
}

void MainWindow::ResetWindows()
{
}

/****************************************************************************/

void MainWindow::CmdNew()
{
  Doc->New();
  ResetWindows();
}

void MainWindow::CmdOpen()
{
  sOpenFileDialog(L"open",L"bla",sSOF_LOAD,Doc->Filename,sMessage(this,&MainWindow::CmdOpen2),sMessage());
}

void MainWindow::CmdOpen2()
{
  Doc->Load();
  ResetWindows();
}

void MainWindow::CmdSaveAs()
{
  sOpenFileDialog(L"save as",L"bla",sSOF_SAVE,Doc->Filename,sMessage(this,&MainWindow::CmdSave),sMessage());
}

void MainWindow::CmdSave()
{
  Doc->Save();
}

void MainWindow::CmdExit()
{
  sExit();
}

/****************************************************************************/
/****************************************************************************/

WinCube::WinCube()
{
  Painter = new sPainter;
  Geo = new sGeometry();
  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_LIGHTING | sMTRL_ZON;
  Mtrl->Prepare(sVertexFormatStandard);
}

WinCube::~WinCube()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
}

void WinCube::Paint(sViewport &view,const sTargetSpec &spec)
{
#if !sCONFIG_SYSTEM_LINUX

  sSetTarget(sTargetPara(sST_CLEARALL|sST_SCISSOR,0xff405060,spec));

  Env.AmbientColor  = 0xff404040;
  Env.LightColor[0] = 0x00c00000;
  Env.LightColor[1] = 0x0000c000;
  Env.LightColor[2] = 0x000000c0;
  Env.LightDir[0].Init(1,0,0);
  Env.LightDir[1].Init(0,1,0);
  Env.LightDir[2].Init(0,0,1);

  Timer.OnFrame(sGetTime());
//  sInt time = Timer.GetTime();

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(view,Env);
  Mtrl->Set(&cb);

  sVertexStandard *vp;
  sU16 *ip;
  Geo->BeginLoad(sVertexFormatStandard,sGF_TRILIST|sGF_INDEX16,sGD_STREAM,8,6*6,(void **)&vp,(void **)&ip);
  vp[0].Init(-1,-1,-1, -1,-1,-1, 0,0);
  vp[1].Init( 1,-1,-1,  1,-1,-1, 1,0);
  vp[2].Init( 1, 1,-1,  1, 1,-1, 1,1);
  vp[3].Init(-1, 1,-1, -1, 1,-1, 0,1);
  vp[4].Init(-1,-1, 1, -1,-1, 1, 0,0);
  vp[5].Init( 1,-1, 1,  1,-1, 1, 1,0);
  vp[6].Init( 1, 1, 1,  1, 1, 1, 1,1);
  vp[7].Init(-1, 1, 1, -1, 1, 1, 0,1);
  sQuad(ip,0,3,2,1,0);
  sQuad(ip,0,4,5,6,7);
  sQuad(ip,0,0,1,5,4);
  sQuad(ip,0,1,2,6,5);
  sQuad(ip,0,2,3,7,6);
  sQuad(ip,0,3,0,4,7);
  Geo->EndLoad();
  Geo->Draw();

  PaintGrid();

  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget(Client);
  Painter->Begin();
  Painter->SetPrint(0,~0,1);
  Painter->PrintF(30,30,L"%5.2ffps %5.3fms",1000/avg,avg);
  Painter->End();
#endif
}

/****************************************************************************/
/****************************************************************************/
