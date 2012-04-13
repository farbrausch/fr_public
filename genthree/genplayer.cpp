// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "genplayer.hpp"
#include "genmesh.hpp"
#include "genmaterial.hpp"
#include "genbitmap.hpp"
#include "genmisc.hpp"
#include "genfx.hpp"
#include "cslce.hpp"
#include "cslrt.hpp"

void GenPlayerSoundHandler(sS16 *steriobuffer,sInt samples,void *user); 
GenPlayer *CurrentGenPlayer;

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   The Player Main Class                                              ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

const struct ScriptFunction  ScriptFunctions[] = 
{
	0x0001,0x020104,Mesh_Cube,
  0x0003,0x020109,Mesh_SelectCube,
  0x0004,0x020103,Mesh_Subdivide,
  0x0006,0x02010a,Mesh_Transform,
  0x000a,0x020002,Mesh_NewMesh,
  0x000b,0x020202,Mesh_Material,
	0x0010,0x020103,Mesh_CalcNormals,
	0x0013,0x020103,Mesh_Triangulate,
	0x0018,0x02010d,Mesh_Perlin,
	0x001b,0x020006,Mesh_Babe,
	0x001c,0x020100,Mesh_BeginRecord,
	0x001d,0x020102,Mesh_AnimLabel,
	0x001f,0x020104,Mesh_SelectRandom,
  0x0022,0x02010b,Mesh_ExtrudeWK,
#if !sINTRO
  0x000d,0x02010c,Mesh_TransformEx, // raus
  0x000e,0x020103,Mesh_Crease, // raus
  0x0021,0x020103,Mesh_SelectAngle, // raus
	0x0014,0x020104,Mesh_Cut,
	0x0019,0x020200,Mesh_Add,
	0x001a,0x020101,Mesh_DeleteFaces,
	0x0020,0x02010a,Mesh_Multiply,
  0x0005,0x020103,Mesh_Extrude,
	0x0015,0x020102,Mesh_ExtrudeNormal,
	0x0017,0x020105,Mesh_Bevel,
#endif
#if !sINTRO_X
  0x0002,0x020102,Mesh_SelectAll,
  0x0009,0x020102,Mesh_Cylinder,
  0x000f,0x020103,Mesh_UnCrease,
	0x0011,0x020105,Mesh_Torus,
	0x0012,0x020102,Mesh_Sphere,
	0x0016,0x020202,Mesh_Displace,
#endif

  0x0041,0x020003,Spline_New,
  0x0042,0x020100,Spline_Sort,
  0x0043,0x020102,Spline_AddKey1,
  0x0044,0x020103,Spline_AddKey2,
  0x0045,0x020104,Spline_AddKey3,
  0x0046,0x020105,Spline_AddKey4,
  0x0047,0x000101,Spline_Calc1, 
  0x0048,0x000101,Spline_Calc2,
  0x0049,0x000101,Spline_Calc3,
  0x004a,0x000101,Spline_Calc4,

  0x0055,0x020100,Sys_Copy,
  0x0058,0x020101,Sys_FindLabel,
	0x0059,0x010002,Sys_Pow,
#if !sINTRO
  0x0051,0x000001,Sys_PrintDez,
  0x0052,0x000001,Sys_PrintHex,
  0x0053,0x000001,Sys_Print,
  0x0056,0x000102,Sys_LoadSound,
  0x0057,0x000101,Sys_PlaySound, 
#endif 
#if !sINTRO
  0x0071,0x02000a,Matrix_Generate,
  0x0072,0x02010a,Matrix_Modify,
  0x0072,0x020101,Matrix_Label,
#endif

  0x0082,0x020100,Bitmap_MakeTexture,
  0x0084,0x020004,Bitmap_Flat,
  0x0085,0x020201,Bitmap_Merge,
  0x0086,0x020105,Bitmap_Color,
  0x0087,0x02010c,Bitmap_GlowRect,
  0x0091,0x020109,Bitmap_Text,
  0x0092,0x02000f,Bitmap_Perlin,
#if !sINTRO
  0x0083,0x000101,Bitmap_SetTexture,
  0x0088,0x02010a,Bitmap_Dots,
  0x0094,0x020102,Bitmap_Wavelet,
  0x0089,0x020104,Bitmap_Blur, // raus
  0x008b,0x020104,Bitmap_HSCB, // hue = 0
  0x008d,0x020202,Bitmap_Distort, // raus
  0x0093,0x020011,Bitmap_Cell, // raus
#endif
#if !sINTRO_X
  0x0081,0x000002,Bitmap_SetSize,
  0x008a,0x020300,Bitmap_Mask,
  0x008c,0x020106,Bitmap_Rotate,
  0x008e,0x020102,Bitmap_Normals,
  0x008f,0x020111,Bitmap_Light,
  0x0090,0x020217,Bitmap_Bump,
	0x0095,0x02000b,Bitmap_Gradient,
#endif
 
  0x00c1,0x020109,Scene_Mesh,
  0x00c2,0x020109,Scene_Trans,
  0x00c3,0x02010a,Scene_Multiply,
  0x00c4,0x020200,Scene_Add,
  0x00c5,0x020100,Scene_Paint,
  0x00c6,0x020101,Scene_Label,
  0x00c7,0x02001e,Scene_Light,

  0x00e0,0x020000,Material_NewMaterial,
  0x00e1,0x020108,MatPass_MatBase,
  0x00e2,0x020203,MatPass_MatTexture,
  0x00e3,0x020105,MatPass_MatFinalizer,
  0x00e4,0x020102,MatPass_MatLabel,
  0x00e5,0x02010c,MatPass_MatTexTrans,
  0x00e9,0x020118,MatPass_MatLight,

#if !sINTRO_X
  0x00e6,0x020105,MatPass_MatDX7,
  0x00e7,0x020106,MatPass_MatBlend,
  0x00e8,0x020106,MatPass_MatStencil,
#endif

  0x00f1,0x020108,MatPass_MatBase,  // raus
  0x00f2,0x020203,MatPass_MatTexture,  // raus
  0x00f3,0x020105,MatPass_MatFinalizer,  // raus
  0x00f4,0x020102,MatPass_MatLabel,  // raus
  0x00f5,0x02010c,MatPass_MatTexTrans,  // raus

	0x0100,0x020006,FXChain_New,
	0x0101,0x02010a,FXChain_BeginViewport,
	0x0102,0x000100,FXChain_Render,
	0x0103,0x020214,FXChain_Layer2D,
	0x0104,0x020209,FXChain_Merge,
	0x0105,0x02010d,FXChain_Color,
	0x0106,0x020101,FXChain_EndViewport,
	0x0107,0x020201,FXChain_PaintScene,
	0x0109,0x020114,FXChain_Blend4x,
	0x010a,0x02020e,FXChain_Viewport,
	0x010b,0x000000,FXChain_ResetAlloc,
	0x010c,0x020102,FXChain_Label,

#if !sINTRO_X
	0x00c9,0x020100,Scene_PartScene,
  0x0120,0x020107,Part_New,
  0x0121,0x02010e,Part_Emitter,
  0x0122,0x020103,Part_Rotate,
  0x0123,0x020114,Part_Life,
  0x0124,0x020202,Part_Spline,
#endif
  0,0,0
};

#if !sINTRO

GenPlayer::GenPlayer()
{
	const ScriptFunction *f;

#if !sPLAYER
  SC = new ScriptCompiler;
#endif
  SR = new ScriptRuntime;
  MP = 0;
  MPName[0] = 0;
  SR->Player = this;

	for(f=ScriptFunctions;f->id;f++)
	{
    sVERIFY(f->params!=0xf0000);
		SR->AddCode(f->id,f->params,f->func);
	}

  BitmapQueue = 0;
  Status = 0;
  Viewport.Init();
  Matrix.Init();
  Camera.Init();
  BeatMax = 512;
  TimeSpeed = 60*44100/140;
  TimeNow = 0;
  TimePlay = 0;
  TimeLoop = 0;
  BeatNow = 0;
  BeatLoopStart = 0x00000;
  BeatLoopEnd = 0x100000;
  TotalSample = 0;
  Show = 0;
  ShowTexture = 0;
  LastTime = 0;
  TotalFrame = 0;

  sSystem->SetSoundHandler(GenPlayerSoundHandler,64,this);
}

/****************************************************************************/

GenPlayer::~GenPlayer()
{
  sSystem->SetSoundHandler(0,0,0);
  if(ShowTexture)
    sSystem->RemTexture(ShowTexture);
}

/****************************************************************************/

void GenPlayer::Tag()
{
#if !sPLAYER
  sBroker->Need(SC);
#endif
  sBroker->Need(SR);
  sBroker->Need(Show);
  sBroker->Need(MP);
}

/****************************************************************************/
/****************************************************************************/

void GenPlayer::Compile(sChar *source)
{
#if !sPLAYER
  sChar *text;
  sU32 *code;

  SC->Reset();
  
  text = sSystem->LoadText("system.txt");
  code = 0;

  Status = 0;
  if(text && SC->Parse(text,"system.txt"))
  {
    delete[] text;
    text = source;//sSystem->LoadText("source2.txt");
    if(text && SC->Parse(text,"source2.txt"))
    {
      SC->Optimise();
      code = SC->Generate();
      if(code)
      {
        SR->Load(code);
        Status = 1;
      }
    }
  }
#endif
//  delete[] text;
}

void GenPlayer::Run()
{
  if(Status==1)
  {
    Status = 2;
//    sDPrintF("@[1]");
//    sDPrintF("start of initialisation:\n");

    SR->SetGlobal(sGPG_BPM,140*0x10000);
    SR->SetGlobal(sGPG_BEATMAX,512*0x10000);
    SR->SetGlobal(sGPG_PLAYER,(sInt)this);

    FXMaster->ResetAlloc();
    CurrentGenPlayer = this;
    TotalFrame++;
		SR->Execute(4);
    SR->Execute(1);
#if !sINTRO
    CurrentGenPlayer = 0;
#endif

    TimeSpeed = sMulDiv(60*44100,0x10000,SR->GetGlobal(sGPG_BPM));
    BeatMax = SR->GetGlobal(sGPG_BEATMAX)/0x10000;


//    sDPrintF("ok.\n");
//    sDPrintF("@[0]");
  }
}

void GenPlayer::Stop()
{
  if(Status==2)
    Status = 1;
}

/****************************************************************************/

void GenPlayer::OnPaint(sRect *r)
{
  sInt time;
  sInt beatnow;
#if !sINTRO
  sInt mx,my;
  sScreenInfo screen;
  sSystem->GetScreenInfo(0,screen);
  sSystem->GetWinMouse(mx,my);
#endif

  if(Status==2)
  {
    time = sSystem->GetTime();

    Viewport.Init();
    if(r)
      Viewport.Window = *r;
    //sSystem->BeginViewport(Viewport);

    Camera.Init(Viewport);
    Camera.CamPos.l.Init4(0,0,-2,1);
    Matrix.Init();

    sSystem->SetCamera(Camera);
    sSystem->SetMatrix(Matrix);

    if(TimePlay)
    {
      LastTime = TimeNow - TotalSample + sSystem->GetCurrentSample();
      if(LastTime<0)
        LastTime = 0;
    }
    beatnow = sMulDiv(LastTime,0x10000,TimeSpeed);
#if sINTRO
    beatnow *= 3;
#endif
    SR->SetGlobal(sGPG_BEAT,beatnow);
    SR->SetGlobal(sGPG_TIME,LastTime);
    SR->SetGlobal(sGPG_TICKS,1);
    SR->SetGlobal(sGPG_PLAYER,(sInt)this);
#if !sINTRO
    SR->SetGlobal(sGPG_MOUSEX,mx*0x10000/screen.XSize);
    SR->SetGlobal(sGPG_MOUSEY,my*0x10000/screen.YSize);
#endif

//    sDPrintF("@[2]");
//    sDPrintF("start of frame:\n");
    FXMaster->ResetAlloc();
    CurrentGenPlayer = this;
    TotalFrame++;
    SR->Execute(2);
#if !sINTRO
    CurrentGenPlayer = 0;
#endif
    sBroker->Free();
//    sDPrintF("ok.\n");
//    sDPrintF("@[0]");

    //sSystem->EndViewport();
  }
}

void GenPlayer::OnShow(sRect *r,sInt funcid)
{
  Show = 0;
  if(ShowTexture)
  {
    sSystem->RemTexture(ShowTexture);
    ShowTexture = 0;
  }
  if(Status==2)
  {
    Viewport.Init();
    if(r)
      Viewport.Window = *r;
    Viewport.ClearFlags = sVCF_NONE;
    sSystem->BeginViewport(Viewport);

    Camera.Init(Viewport);
    Camera.CamPos.l.Init4(0,0,-2,1);
    Matrix.Init();

    sSystem->SetCamera(Camera);
    sSystem->SetMatrix(Matrix);

    SR->SetGlobal(sGPG_BEAT,0);
    SR->SetGlobal(sGPG_TIME,0);
    SR->SetGlobal(sGPG_TICKS,1);
    SR->SetGlobal(sGPG_PLAYER,(sInt)this);

//    sDPrintF("@[2]");
//    sDPrintF("start of frame (show):\n");
    FXMaster->ResetAlloc();
    CurrentGenPlayer = this;
    TotalFrame++;
    SR->Execute(funcid);
#if !sINTRO
    CurrentGenPlayer = 0;
#endif
    sBroker->Free();
//    sDPrintF("ok.\n");
//    sDPrintF("@[0]");

    sSystem->EndViewport();
  }
}

/****************************************************************************/

void GenPlayerSoundHandler(sS16 *steriobuffer,sInt samples,void *user)
{
#if sINTRO
  if(CurrentGenPlayer)
    CurrentGenPlayer->SoundHandler(steriobuffer,samples);
#else
  ((GenPlayer *)user)->SoundHandler(steriobuffer,samples);
#endif
}

void GenPlayer::SoundHandler(sS16 *buffer,sInt samples)
{
  sInt time=0;
  sInt i,j;
  sInt a;
  static sInt ao;

  if(TimePlay)
  {
    if(MP)
    {
      MP->PlayPos = TimeNow;
      MP->Handler(buffer,samples);
      TimeNow+=samples;
    }
    else
    {
      for(i=0;i<samples;i+=64)
      {
        time = TimeNow;
        a = 0x7000-(BeatNow&0xffff);
        if(a<0) a = 0;
        for(j=0;j<64;j++)
        {
          buffer[0] = sFSin((time+j)*0.0501f)*((a*j+ao*(63-j))/64);
          buffer[1] = sFSin((time+j)*0.0503f)*((a*j+ao*(63-j))/64);
          buffer+=2;
        }
        ao = a;
        TimeNow += 64;
      }
    }
    BeatNow = sMulDiv(TimeNow,0x10000,TimeSpeed);
    if(TimeLoop && BeatNow > BeatLoopEnd)
    {
      BeatNow = BeatNow - (BeatLoopEnd-BeatLoopStart);
      if(BeatNow<0)
        BeatNow = 0;
      TimeNow = sMulDiv(BeatNow,TimeSpeed,0x10000);
    }
  }
  else
  {
    sSetMem4((sU32 *)buffer,0,samples);
  }
  TotalSample+=samples;
}

/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***    sScriptTimeBorder                                                 ***/
/***                                                                      ***/
/****************************************************************************/

#if sLINK_GUI

GenTimeBorder::GenTimeBorder()
{
  Height = 21;
  Player = 0;
}

void GenTimeBorder::Tag()
{
  sBroker->Need(Player);
  sGuiWindow::Tag();
}

void GenTimeBorder::OnCalcSize()
{
  SizeY = Height;
  SizeX = 0;
}

void GenTimeBorder::OnSubBorder()
{
  Parent->Client.y1 -= Height;
}

void GenTimeBorder::OnPaint()
{
  sRect r;
  sU32 col;
  sInt i;
  sInt x,y,x0,x1;

  col = sGui->Palette[sGC_DRAW];
  r = Client;
  r.y0 = r.y1 - Height;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
  r.Extend(-2);
  sGui->RectHL(r,1,col,col);

  if(Player)
  {
    if(Player->BeatLoopEnd > Player->BeatLoopStart)
    {
      x0 = r.x0+sMulDiv(Player->BeatLoopStart,r.XSize(),Player->BeatMax*0x10000);
      x1 = r.x0+sMulDiv(Player->BeatLoopEnd  ,r.XSize(),Player->BeatMax*0x10000);
      sPainter->Paint(sGui->FlatMat,x0,r.y0,x1-x0,r.y1-r.y0,sGui->Palette[Player->TimeLoop?sGC_SELBACK:sGC_BACK]);
    }
    for(i=0;i<Player->BeatMax;i+=4)
    {
      y = (i&15)?(r.y0+r.y1*2)/3:r.y0+4;
      x = r.x0+sMulDiv(i,r.XSize(),Player->BeatMax);
      sPainter->Paint(sGui->FlatMat,x,y,1,r.y1-y,col);
    }
    x = r.x0+sMulDiv(Player->BeatNow,r.XSize(),Player->BeatMax*0x10000);
    sPainter->Paint(sGui->FlatMat,x-1,r.y0,2,r.y1-r.y0,0xffff0000);
  }

  Flags &= ~sGWF_PASSRMB;
}

void GenTimeBorder::OnDrag(sDragData &dd)
{
  sRect r;
  sInt x;

  r = Client;
  r.y0 = r.y1 - Height;
  //sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
  r.Extend(-2);
  x = sMulDiv(dd.MouseX-r.x0,Player->BeatMax*0x10000,r.XSize());

  switch(dd.Mode)
  {
  case sDD_START:
    if(dd.Buttons&1)
    {
      DragStart = x;
      DragMode = 1;
    }
    else if(dd.Buttons&2)
    {
      DragStart = x;
      DragMode = 2;
    }
    break;
  case sDD_DRAG:
    if(DragMode == 1 && Player)
    {
      if(x<0) x = 0;
      Player->BeatNow = x;
      Player->TimeNow = sMulDiv(Player->BeatNow,Player->TimeSpeed,0x10000);
      Player->LastTime = Player->TimeNow;
    }
    if(DragMode == 2 && Player)
    {
      Player->BeatLoopStart = (DragStart+0x20000)/0x40000*0x40000;
      Player->BeatLoopEnd   = (x        +0x20000)/0x40000*0x40000;
      if(Player->BeatLoopStart > Player->BeatLoopEnd)
        sSwap(Player->BeatLoopStart,Player->BeatLoopEnd);
    }
    break;
  case sDD_STOP:
    DragMode = 0;
    break;
  }

}

#endif // !sINTRO
#endif
