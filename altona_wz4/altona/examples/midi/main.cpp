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

#include "main.hpp"
#include "gui/textwindow.hpp"
#include "base/devices.hpp"

/****************************************************************************/

MainWindow::MainWindow()
{
  sString<64> str;

  ScreenIndex = 0;
  sClear(Screen);

  sMidiHandler->InputMsg = sMessage(this,&MainWindow::CmdMidi);

  for(sInt i=0;;i++)
  {
    const sChar *n = sMidiHandler->GetDeviceName(0,i);
    if(n==0)
      break;
    str.PrintF(L"in  %d = %q",i,n);
    Print(str);
  }
  for(sInt i=0;;i++)
  {
    const sChar *n = sMidiHandler->GetDeviceName(1,i);
    if(n==0)
      break;
    str.PrintF(L"out %d = %q",i,n);
    Print(str);
  }

  
  Print(L"ready...");
}

MainWindow::~MainWindow()
{
}

void MainWindow::Print(const sChar *str)
{
  Screen[ScreenIndex] = str;
  ScreenIndex = (ScreenIndex+1) % sCOUNTOF(Screen);
}

void MainWindow::OnPaint2D()
{
  sRect r;
  sInt h = sGui->FixedFont->GetHeight()+1;
  for(sInt i=0;i<sCOUNTOF(Screen);i++)
  {
    r = Client;
    r.y0 += h*i;
    r.y1 = r.y0+h;
    sGui->FixedFont->Print(sF2P_LEFT|sF2P_TOP|sF2P_OPAQUE,r,Screen[(i+ScreenIndex) % sCOUNTOF(Screen)]);
  }
  r = Client;
  r.y0 = sCOUNTOF(Screen)*h;
  sRect2D(r,sGC_BACK);
}

sBool MainWindow::OnKey(sU32 key)
{
  sInt val;
  switch(key & (sKEYQ_MASK|sKEYQ_BREAK))
  {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
    val = sClamp<sInt>((key&15)*128,0,999);
    for(sInt i=0;i<8;i++)
    {
      sMidiHandler->Output(0,0xb0,0x10+i,(val>>7)&0x7f);
      sMidiHandler->Output(0,0xb0,0x30+i,val & 0x7f);
    }
    break;
  }


//  sString<64> str;
//  str.PrintF(L"%08x",key);
//  Print(str);
//  Update();

  return 1;
}

void MainWindow::CmdMidi()
{
  sMidiEvent ev;
  while(sMidiHandler->GetInput(ev))
  {
    sString<64> str;
    str.PrintF(L"%02x:%02x %02x %02x",ev.Device,ev.Status,ev.Value1,ev.Value2);
    Print(str);
  }
  Update();
}

/****************************************************************************/

void sMain()
{
  sAddMidi();
  sSetWindowName(L"Midi Example");

  sInit(sISF_2D,800,600);
  sInitGui();
  sGui->AddBackWindow(new MainWindow);
}

/****************************************************************************/
