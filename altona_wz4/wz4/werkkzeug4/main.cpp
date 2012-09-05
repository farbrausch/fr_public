/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "wz4lib/gui.hpp"
#include "wz4frlib/wz4_gui.hpp"
#include "gui/gui.hpp"
#include "base/windows.hpp"
#include "base/devices.hpp"
#include "util/taskscheduler.hpp"
#include "extra/blobheap.hpp"

/****************************************************************************/


void RegisterWZ4Classes()
{
  for(sInt i=0;i<2;i++)
  {
    sREGOPS(basic,0);
    sREGOPS(poc,1);
//    sREGOPS(minmesh,1);           // obsolete wz3
//    sREGOPS(genspline,1);         // obsolete wz3

    sREGOPS(chaos_font,0);        // should go away soon (detuned)
    sREGOPS(wz3_bitmap,0);
    sREGOPS(wz4_anim,0);
//    sREGOPS(wz4_bitmap,1);        // bitmap system based on new image libarary, never finished
//    sREGOPS(wz4_cubemap,0);       // bitmap system based on new image libarary, never finished
    sREGOPS(wz4_mtrl,1);
    sREGOPS(chaosmesh,1);         // should go away soon (detuned)
//    sREGOPS(wz4_demo,1);          // old demo system, never worked
    sREGOPS(wz4_demo2,0);
    sREGOPS(wz4_mesh,0);
    sREGOPS(chaosfx,0);
    sREGOPS(easter,0);
    sREGOPS(wz4_bsp,0);
    sREGOPS(wz4_mtrl2,0);
//    sREGOPS(wz4_rovemtrl,0);      // was never finished
    sREGOPS(tron,1);
    sREGOPS(wz4_ipp,0);
    sREGOPS(wz4_audio,0);         // audio to image -> not usefull (yet)
    //sREGOPS(wz4_ssao,0);
    sREGOPS(fxparticle,0);
    sREGOPS(wz4_modmtrl,0);
    sREGOPS(wz4_modmtrlmod,0);

//    sREGOPS(fr033,0);   // bp invitation
    sREGOPS(fr062,0);   // the cube
    sREGOPS(fr063_chaos,0);   // chaos+tron
    sREGOPS(fr063_mandelbulb,0);   // chaos+tron
    sREGOPS(fr063_sph,0);   // chaos+tron
    sREGOPS(fr067,0);
//    sREGOPS(fr070_tron,0);

//    sREGOPS(fr070,0);

    sREGOPS(fr063_tron,1);   // chaos+tron
    sREGOPS(adf,1);     
    sREGOPS(pdf,1);

    sREGOPS(screens4,1);
  }

  Doc->FindType(L"Scene")->Secondary = 1;
  Doc->FindType(L"GenBitmap")->Order = 2;
  Doc->FindType(L"Wz4Mesh")->Order = 3;
  Doc->FindType(L"Wz4Render")->Order = 4;
  Doc->FindType(L"Wz4Mtrl")->Order = 5;
//  Doc->FindType(L"Wz4Cubemap")->Order = 6;

  wClass *cl = Doc->FindClass(L"MakeTexture2",L"Texture2D");
  sVERIFY(cl);
  Doc->Classes.RemOrder(cl);
  Doc->Classes.AddHead(cl);   // order preserving!

  Doc->FindType(L"ModShaderSampler")->EquivalentType = Doc->FindType(L"ModShader");
}

//sINITMEM(sIMF_DEBUG|sIMF_CLEAR|sIMF_NORTL,1500*1024*1024);

void sMain()
{
//  sBreakOnAllocation(27005);
//  sAddMidi();
  sAddSched();
  sAddGlobalBlobHeap();
  sEnlargeRTDepthBuffer(1024,1024);
  sInit(sISF_2D|sISF_NOVSYNC|sISF_3D,1280,768);
  sInitGui();
//  sCreateZBufferRT(1600,1200);
  sGui->AddBackWindow(new Wz4FrMainWindow);
  App->WikiPath = L"c:/svn/farbrausch/wz4frwiki";
  App->UnitTestPath = L"c:/svn/farbrausch/werkkzeug4/unittest";

//  App->WikiCheckout = L"svn checkout svn://dns49878.dyndns.org:27730/wz4frwiki c:/svn/farbrausch/wz4frwiki --username wiki --password qwer12";
  App->WikiCheckout = L"svn checkout svn://192.168.20.40:27730/farbrausch/wz4frwiki c:/svn/farbrausch/wz4frwiki --username wiki --password qwer12";
  App->MainInit();
  sSetWindowName(L"werkkzeug4");
#if sRENDERER!=sRENDER_DX11
  sSetWindowMode(sWM_MAXIMIZED);
#endif
}

/****************************************************************************/

void Wz4FrMainWindow::ExtendWireRegister()
{
  sWire->AddKey(L"main",L"TimelineEditor",sMessage(this,&Wz4FrMainWindow::CmdCustomEditor));
  sWire->AddKey(L"main",L"BeatEditor",sMessage(this,&Wz4FrMainWindow::CmdCustomEditor2));
}

void Wz4FrMainWindow::ExtendWireProcess()
{
  sWire->ProcessText(
"menu menu_nav \"Navigation\"\n"
"{\n"
"  item main.TimelineEditor \"Timeline Editor\" F3|GLOBAL;\n"
"  item main.BeatEditor \"Beat Editor\" F4|GLOBAL;\n"
"}\n"
  );
}

void Wz4FrMainWindow::CmdCustomEditor()
{
  StartCustomEditor(new Wz4TimelineCed(0));
}

void Wz4FrMainWindow::CmdCustomEditor2()
{
  StartCustomEditor(new Wz4BeatCed(0));
}

/****************************************************************************/
