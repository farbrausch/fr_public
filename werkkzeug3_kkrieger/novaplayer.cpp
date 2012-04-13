// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#if !sPLAYER

#include "novaplayer.hpp"
#include "engine.hpp"
#include "genmesh.hpp"
#include "genoverlay.hpp"
#if !sPLAYER
#include "werkkzeug.hpp"
#include "winnova.hpp"
#include "script.hpp"
#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Effect                                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

void KEnvironment2::Init()
{
  Events.Init();
}

void KEnvironment2::Exit()
{
  Events.Exit();
}

/****************************************************************************/

void KEvent2::Init(const KEffect2 *ef)
{
  sSetMem(this,0,sizeof(*this));
  Splines.Init();
  Code.Init();
  SceneNodes.Init();

  Start = 0x00000;
  End = 0x10000;
  LoopEnd = 0;
  Effect = ef;

  if(ef->EditSize>0)
  {
    Edit = new sF32[(ef->EditSize+3)/4];
    sSetMem(Edit,0,ef->EditSize);
  }

  if(ef->ParaSize>0)
  {
    Para = new sF32[(ef->ParaSize+3)/4];
    sSetMem(Para,0,ef->ParaSize);
  }

}

void KEvent2::Exit()
{
  if(Edit) delete[] Edit;
  if(Para) delete[] Para;
  Code.Exit();
  Splines.Exit();
  SceneNodes.Exit();
}

/****************************************************************************/

KSceneNode2::KSceneNode2()
{
  ClassId = KC_SCENE2;

  Matrix.Init();
  Bounds.Min.Init(0,0,0,1);
  Bounds.Max.Init(0,0,0,1);
  Next = 0;
  First = 0;
  Light = 0;
  Mesh = 0;
  BackLink = 0;
}

KSceneNode2::~KSceneNode2()
{
}

void KSceneNode2::Copy(KObject *o)
{
  KSceneNode2 *no;

  sVERIFY(o->ClassId==KC_SCENE2);
  no = (KSceneNode2 *) o;

  Matrix = no->Matrix;
}

void NovaEngine(WerkDoc2 *doc2,WerkDoc *)
{
  sInt i;
  KEvent2 *ev;
  KEnvironment2 *kenv;
  sF32 time;

  kenv = &doc2->Env2;
  kenv->InScene = 0;
  kenv->Env.Init();

  doc2->Build();

  GenOverlayManager->SetMasterViewport(kenv->View);  

  doc2->VM->InitFrame();
  for(i=0;i<kenv->Events.Count;i++)
  {
    ev = kenv->Events[i];

    if(kenv->BeatTime>=ev->Start && kenv->BeatTime<ev->LoopEnd && ev->Effect->OnPaint)
    {
      doc2->VM->InitEvent(ev);    // init BEFORE alias
      if(ev->Alias)
      {
        time = doc2->VM->GetTime();   // hardcoded alias onpaint
        time = sFMod((time*ev->Para[0])+ev->Para[1],1.0f);
        doc2->VM->SetTime(time);
        ev = ev->Alias;
      }
      doc2->VM->Execute(&ev->Code);
      if(kenv->InScene && !(ev->Effect->Flags&KEF_NEEDBEGINSCENE))
      {
        //sSystem->EndViewport();
        kenv->InScene = 0;
      }
      if(!kenv->InScene && (ev->Effect->Flags&KEF_NEEDBEGINSCENE))
      {
        sSystem->SetViewport(kenv->View);
        //sSystem->BeginViewport(kenv->View);
        kenv->InScene = 1;
      }

      kenv->EffectBeatTime = kenv->BeatTime - ev->Start;
      ev->Effect->OnPaint(ev,kenv);
    }
  }

  /*if(kenv->InScene)
    sSystem->EndViewport();*/

  kenv->InScene = 0;
}

/****************************************************************************/

void NovaSceneWalk(KSceneNode2 *node,const sMatrix &pmat)
{
  KSceneNode2 *next;

  node->WorldMatrix.MulA(node->Matrix,pmat);
  next = node->First;
  while(next)
  {
    NovaSceneWalk(next,node->WorldMatrix);
    next = next->Next;                    // continue sibling
  }
  if(node->Mesh)
  {
    Engine->AddPaintJob(node->Mesh,node->WorldMatrix,0.0f); // paint
  }
#if !sPLAYER
  if(node->BackLink)
    node->BackLink->Visible = 1;
#endif
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Parameter Info                                                     ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/


void KEffectInfo::Init()
{
  sSetMem(this,0,sizeof(*this));
  ParaInfo.Init();
  Exports.Init();
}

void KEffectInfo::Exit()
{
  ParaInfo.Exit();
  Exports.Exit();
}

void KEffectInfo::Clear()
{
  sSetMem(DefaultPara,0,sizeof(DefaultPara));
  sSetMem(DefaultEdit,0,sizeof(DefaultEdit));
  ParaInfo.Count = 0;
  Exports.Count = 0;
}

void KEffectInfo::AddSymbol(const sChar *name,sInt id,sInt type)
{
  KEffect2Symbol *ex;
  const sChar *s;
  sChar *d;
  sInt i;

  ex = Exports.Add();
  s = name;
  d = ex->Name;
  i = 0;
  while(*s && i<KK_NAME-1)
  {
    if(*s!=' ')
    {
      *d++ = *s;
      i++;
    }
    s++;
  }
  *d++ = 0;
  ex->Id = id;
  ex->Type = type;
}

KEffect2ParaInfo2 *KEffectInfo::AddPara(const sChar *name,sInt offset,sInt type)
{
  KEffect2ParaInfo2 *info;

  info = ParaInfo.Add();
  sSetMem(info,0,sizeof(*info));

  info->Name = name;
  info->Offset = offset;
  info->Type = type;

  return info;
}

KEffect2ParaInfo2 *KEffectInfo::AddLink(const sChar *name,sInt index,sInt kc_type) 
{
  KEffect2ParaInfo2 *info;

  info = AddPara(name,index,EPT_LINK);
  info->KCType = kc_type;

  return info;
}

KEffect2ParaInfo2 *KEffectInfo::AddAlias(const sChar *name) 
{
  KEffect2ParaInfo2 *info;

  info = AddPara(name,0,EPT_ALIAS);

  return info;
}

KEffect2ParaInfo2 *KEffectInfo::AddGroup(const sChar *name)
{
  KEffect2ParaInfo2 *info;

  info = AddPara(name,0,EPT_GROUP);

  return info;
}

KEffect2ParaInfo2 *KEffectInfo::AddChoice(const sChar *name,sInt offset,const sChar *choice)
{
  KEffect2ParaInfo2 *info;

  offset = offset/4;
  info = AddPara(name,offset,EPT_CHOICE);
  info->Choice = choice;

  return info;
}

KEffect2ParaInfo2 *KEffectInfo::AddFlags(const sChar *name,sInt offset,const sChar *choice)
{
  KEffect2ParaInfo2 *info;

  offset = offset/4;
  info = AddPara(name,offset,EPT_FLAGS);
  info->Choice = choice;

  return info;
}

KEffect2ParaInfo2 *KEffectInfo::AddInt(const sChar *name,sInt offset,sF32 min,sF32 max,sF32 step,sInt count)
{
  KEffect2ParaInfo2 *info;

  offset = offset/4;
  info = AddPara(name,offset,EPT_INT);
  info->Min = min;
  info->Max = max;
  info->Step = step;
  info->Count = count;

  return info;
}

KEffect2ParaInfo2 *KEffectInfo::AddFloat(const sChar *name,sInt offset,sF32 min,sF32 max,sF32 step,sInt count)
{
  KEffect2ParaInfo2 *info;
  sInt anim = 0;

  offset = offset/4;
  if(*name=='*')
  {
    name++;
    if(count==1) { AddSymbol(name,offset,sST_SCALAR); anim = EAT_SCALAR; }
    if(count==4) { AddSymbol(name,offset,sST_VECTOR); anim = EAT_VECTOR; }
  }

  info = AddPara(name,offset,EPT_FLOAT);
  info->Min = min;
  info->Max = max;
  info->Step = step;
  info->Count = count;
  info->AnimType = anim;

  return info;
}

KEffect2ParaInfo2 *KEffectInfo::AddColor(const sChar *name,sInt offset)
{
  KEffect2ParaInfo2 *info;
  sInt anim = 0;

  offset = offset/4;
  if(*name=='*')
  {
    name++;
    AddSymbol(name,offset,sST_VECTOR);
    anim = EAT_COLOR;
  }

  info = AddPara(name,offset,EPT_COLOR);
  info->AnimType = anim;

  return info;
}

KEffect2ParaInfo2 *KEffectInfo::AddMatrix(const sChar *name,sInt offset,sInt editoffset)
{
  KEffect2ParaInfo2 *info;
  sInt anim = 0;

  offset = offset/4;
  if(*name=='*')
  {
    name++;
    AddSymbol(name,offset,sST_MATRIX);
    anim = EAT_EULER|EAT_SRT|EAT_TARGET;
  }

  info = AddPara(name,offset,EPT_MATRIX);
  info->EditOffset = editoffset/4;
  info->AnimType = anim;

  return info;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Operators                                                          ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

void KE_Default_OnSet(KEvent2 *event,KEnvironment2 *env,sInt id,sF32 *values,sU32 mask)
{
  sF32 *dest;

  dest = ((sF32 *)event->Para)+id;
  while(mask!=0)
  {
    sVERIFY(id<event->Effect->ParaSize); id++;
    if(mask&1) 
      *(sU32 *)dest = *(sU32 *)values;     // copy as integer, because there might be pointers!
    dest++;
    values++;
    mask = mask>>1;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Camera                                                             ***/
/***                                                                      ***/
/****************************************************************************/

struct KE_Camera_Para
{
  sMatrix Matrix;
  sF32 ClipNear;
  sF32 ClipFar;
  sVector ClearColor;
  sInt Flags;
  sF32 CenterX,CenterY;
  sF32 ZoomX,ZoomY;
  sVector FogColor;
  sF32 FogStart;
  sF32 FogEnd;
};

struct KE_Camera_Edit
{
  sMatrixEdit Matrix;
};

void KE_Camera_OnPaint(KEvent2 *ev,KEnvironment2 *kenv)
{
  KE_Camera_Para *para;
  para = (KE_Camera_Para *) ev->Para;

  // FIXME: need to get clears into the current architecture somehow
  /*kenv->View.ClearColor = para->ClearColor.GetColor();
  kenv->View.ClearFlags = para->Flags&3;*/

  kenv->Env.Init();
  kenv->Env.FarClip       = para->ClipFar;
  kenv->Env.NearClip      = para->ClipNear;
  kenv->Env.CameraSpace   = para->Matrix;
  kenv->Env.ZoomX         = para->ZoomX;
  kenv->Env.ZoomY         = para->ZoomY*kenv->View.Window.XSize()/kenv->View.Window.YSize();
  kenv->Env.CenterX       = para->CenterX;
  kenv->Env.CenterY       = para->CenterY;
  kenv->Env.FogColor      = para->FogColor.GetColor();
  kenv->Env.FogStart      = para->FogStart;
  kenv->Env.FogEnd        = para->FogEnd;
}

#if !sPLAYER
void KE_Camera_OnPara(KEffectInfo *win)
{
  KE_Camera_Para *para = (KE_Camera_Para *) win->DefaultPara;

  win->AddFloat ("Clip Near"    ,sOFFSET(KE_Camera_Para,ClipNear  ),0,0x10000,0.01f);
  win->AddFloat ("Clip Far"     ,sOFFSET(KE_Camera_Para,ClipFar   ),0,0x10000,16.0f);
  win->AddColor ("*Clear Color" ,sOFFSET(KE_Camera_Para,ClearColor));
  win->AddFlags ("Flags"        ,sOFFSET(KE_Camera_Para,Flags     ),"*0|color:*1|zbuffer");

  win->AddFloat ("Center"       ,sOFFSET(KE_Camera_Para,CenterX   ),  -4,    4,0.002f,2);
  win->AddFloat ("Zoom"         ,sOFFSET(KE_Camera_Para,ZoomX     ),   0,  256,0.020f,2);
  win->AddColor ("*Fog Color"   ,sOFFSET(KE_Camera_Para,FogColor  ));
  win->AddFloat ("*Fog Start"   ,sOFFSET(KE_Camera_Para,FogStart  ), 0,65536,1.000f);
  win->AddFloat ("*Fog End"     ,sOFFSET(KE_Camera_Para,FogEnd    ), 0,65536,1.000f);

  win->AddGroup ("Matrix");
  win->AddMatrix("*Matrix"      ,sOFFSET(KE_Camera_Para,Matrix    ),sOFFSET(KE_Camera_Edit,Matrix));

  para->Matrix.Init();
  para->ClipNear = 0.125f;
  para->ClipFar = 4096;
  para->Flags = 3;
  para->ClearColor.InitColor(0xff204060);
  para->ZoomX = 1.0;
  para->ZoomY = 1.0;
  para->FogColor.InitColor(0xffffffff);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Mesh (erste polys)                                                 ***/
/***                                                                      ***/
/****************************************************************************/

struct KE_Mesh_Para
{
  sMatrix Matrix;
};

struct KE_Mesh_Edit
{
  sMatrixEdit Matrix;
};


void KE_Mesh_OnPaint(KEvent2 *ev,KEnvironment2 *kenv)
{
  KE_Mesh_Para *para;
  GenMesh *mesh;
  KEnvironment kenv1;

  para = (KE_Mesh_Para *) ev->Para;
  mesh = (GenMesh *) ev->Links[0];
  if(!kenv->InScene && mesh && mesh->ClassId==KC_MESH)
  {
    mesh->Prepare();

    Engine->SetViewProject(kenv->Env);
    Engine->StartFrame();
    Engine->AddPaintJob(mesh->PreparedMesh,para->Matrix,0.0f);
    sSystem->SetViewport(kenv->View);

    // add default light
    EngLight light;
    light.Position = kenv->Env.CameraSpace.l;
    light.Flags = 0;
    light.Color = 0x78787878;
    light.Amplify = 1.0f;
    light.Range = 32.0f;
    light.Event = 0;
    light.Id = 0;
    Engine->AddLightJob(light);

    kenv1.InitView();
    kenv1.InitFrame(0,0);
    Engine->Paint(&kenv1);
    kenv1.ExitFrame();
  }
}

#if !sPLAYER
void KE_Mesh_OnPara(KEffectInfo *win)
{
  KE_Mesh_Para *para = (KE_Mesh_Para *) win->DefaultPara;
  KE_Mesh_Edit *edit = (KE_Mesh_Edit *) win->DefaultEdit;

  win->AddLink  ("Mesh"      ,0,KC_MESH);
  win->AddGroup ("Matrix");
  win->AddMatrix("*Matrix"   ,sOFFSET(KE_Mesh_Para,Matrix    ),sOFFSET(KE_Mesh_Edit,Matrix));

  para->Matrix.Init();
  edit->Matrix.Init();
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Glare (ipp-test)                                                   ***/
/***                                                                      ***/
/****************************************************************************/

struct KE_Glare_Para
{
  sInt Size;
  sVector SubColor;
  sVector Amplify1;
  sF32 Range;
  sF32 Amplify2;
  sU32 Flags;
  sInt MaxStages;
  sInt MergeMode;
  sVector MergeColor;
};

struct KE_Glare_Edit
{
};

enum KE_Glare_Para_Flags
{
  GPF_DIAMOND       = 0x0001,
  GPF_OVERBRIGHT    = 0x0002,
  GPF_BYPASSSUB     = 0x0004,
  GPF_BYPASSBLUR    = 0x0008,
};


void KE_Glare_OnPaint(KEvent2 *ev,KEnvironment2 *kenv)
{
  KE_Glare_Para *para;
  GenOverlayRT *input,*screen,*render0,*render1;
  sViewport view;
  sInt mode;
  sU32 flags;
  sFRect rc[4];
  static sInt kernel[] = {
    -1,-1,  1,-1, -1,1, 1,1, // square
     0,-1, -1, 0,  1,0, 0,1, // diamond
  };
  sInt i,count;
  sF32 radius;

  para = (KE_Glare_Para *) ev->Para;
  flags = para->Flags;
  if((flags & 12)==12) return;

  render1 = GenOverlayManager->Get(para->Size ,0);
  render0 = GenOverlayManager->Get(para->Size ,1);
  screen = GenOverlayManager->Get(RTSIZE_SCREEN,0);

  input = render1;
  GenOverlayManager->GrabScreen(input);

// subtract

  if(!(flags & 4))
  {
    mode = GENOVER_COLOR2;
    GenOverlayManager->PrepareViewport(render0,view);
    sSystem->SetViewport(view);

    GenOverlayManager->Mtrl[mode]->SetTex(0,render1->Bitmap->Texture);
    GenOverlayManager->Mtrl[mode]->Color[0] = para->SubColor.GetColor();
    GenOverlayManager->Mtrl[mode]->Color[2] = para->Amplify1.GetColor();
    GenOverlayManager->FXQuad(mode,render1->ScaleUV);

    sSwap(render1,render0);
    input = render1;
  }

// glare

  if(!(flags & 8))
  {
    count = 0;
    radius = para->Range;
    do
    {
      radius *= 0.5f;
      count++;
    }
    while(count<para->MaxStages && radius>0.5f);

    while(count--)
    {
      for(i=0;i<4;i++)
      {
        rc[i].x0 = rc[i].x1 = kernel[(flags&1)*8+i*2+0] * radius;
        rc[i].y0 = rc[i].y1 = kernel[(flags&1)*8+i*2+1] * radius * 0.5f;
      }

      if(count)
        GenOverlayManager->Blend4x(input,render0,rc,1,(flags&2)?para->Amplify2:1.0f);
      else
        GenOverlayManager->Blend4x(input,render0,rc,1,para->Amplify2);
      radius *= 2.0f;
      sSwap(render1,render0);
      input = render1;
    }
    GenOverlayManager->Copy(render1,screen,para->MergeMode,para->MergeColor.GetColor(),1);
  }
}

#if !sPLAYER
void KE_Glare_OnPara(KEffectInfo *win)
{
  KE_Glare_Para *para = (KE_Glare_Para *) win->DefaultPara;
  KE_Glare_Edit *edit = (KE_Glare_Edit *) win->DefaultEdit;

  win->AddChoice ("Size"      ,sOFFSET(KE_Glare_Para,Size     ),"Large|Medium|Small|Full|Screen");
  win->AddFlags  ("Flags"     ,sOFFSET(KE_Glare_Para,Flags    ),"*2|Bypass Sub:*3|Bypass Glare");
  win->AddGroup  ("hightlights");
  win->AddColor  ("*Color"    ,sOFFSET(KE_Glare_Para,SubColor ));
  win->AddColor  ("*Amplify1" ,sOFFSET(KE_Glare_Para,Amplify1 ));
  win->AddGroup  ("blur");
  win->AddFloat  ("*Radius"   ,sOFFSET(KE_Glare_Para,Range    ), 0.0f,100.0f,0.05f);
  win->AddFloat  ("*Amplify2" ,sOFFSET(KE_Glare_Para,Amplify2 ), 0.0f,4.0f,0.01f);
  win->AddFlags  ("Flags"     ,sOFFSET(KE_Glare_Para,Flags    ), "Square|Diamond:*1Amp Once|Overbright");
  win->AddInt    ("Max Stages",sOFFSET(KE_Glare_Para,MaxStages), 1,16,0.25f);
  win->AddGroup  ("Merge");
  win->AddColor  ("*Amplify3" ,sOFFSET(KE_Glare_Para,MergeColor));
  win->AddFlags  ("Mode"      ,sOFFSET(KE_Glare_Para,MergeMode),"copy|add|mul|mul2|addsmooth");

  para->Size = 2;
  para->SubColor.InitColor(0xff404040);
  para->Amplify1.InitColor(0xff404040);
  para->Range = 16.0f;
  para->Amplify2 = 1.0f;
  para->Flags = 0;
  para->MaxStages = 2;
  para->MergeMode = 1;
  para->MergeColor.InitColor(0xffffffff);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Scene (erste polys)                                                ***/
/***                                                                      ***/
/****************************************************************************/

struct KE_Scene_Para
{
//  sMatrix Matrix;
};

struct KE_Scene_Edit
{
//  sMatrixEdit Matrix;
};

void KE_Scene_OnInit(KEvent2 *ev,KEnvironment2 *kenv)
{
}

void KE_Scene_OnExit(KEvent2 *ev,KEnvironment2 *kenv)
{
}

void KE_Scene_OnPaint(KEvent2 *ev,KEnvironment2 *kenv)
{
  KEnvironment kenv1;
  sMatrix mat;
  KSceneNode2 *scene;

  scene = (KSceneNode2 *) ev->Links[0];
  if(!scene) return;
  sVERIFY(scene->ClassId == KC_SCENE2);

  // prepare drawing

  Engine->SetViewProject(kenv->Env);
  Engine->StartFrame();
  sSystem->SetViewport(kenv->View);

  // walk scene

  mat.Init();
  NovaSceneWalk(scene,mat);

  // add a light

  EngLight light;
  light.Position = kenv->Env.CameraSpace.l;
  light.Flags = 0;
  light.Color = 0x78787878;
  light.Amplify = 1.0f;
  light.Range = 32.0f;
  light.Event = 0;
  light.Id = 0;
  Engine->AddLightJob(light);

  // draw..

  kenv1.InitView();
  kenv1.InitFrame(0,0);
  Engine->Paint(&kenv1);
  kenv1.ExitFrame();
}

void KE_Scene_OnSet(KEvent2 *event,KEnvironment2 *env,sInt id,sF32 *values,sU32 mask)
{
  KSceneNode2 *node;
  sF32 *dest;
  sVERIFY(id>=0 && id/16<event->SceneNodes.Count);
  node = event->SceneNodes[id/16];
  dest = (&node->Matrix.i.x)+(id&15);
  while(mask)
  {
    if(mask&1)
      *dest = *values;
    dest++;
    values++;
    mask = mask>>1;
  }
}

#if !sPLAYER
void KE_Scene_OnPara(KEffectInfo *win)
{
  win->AddLink("Scene",0,KC_SCENE2);
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Alias                                                              ***/
/***                                                                      ***/
/****************************************************************************/

struct KE_Alias_Para
{
  sF32 Zoom;
  sF32 Offset;
};

void KE_Alias_OnPaint(KEvent2 *ev,KEnvironment2 *kenv)
{
  // hardcoded in execution engine
}

#if !sPLAYER
void KE_Alias_OnPara(KEffectInfo *win)
{
  KE_Alias_Para *para = (KE_Alias_Para *) win->DefaultPara;
  para->Zoom = 1.0f;
  para->Offset = 0.0f;
  win->AddAlias  ("Alias-Event");
  win->AddFloat  ("Zoom"      ,sOFFSET(KE_Alias_Para,Zoom     ), 0.0f,1024.0f,0.05f);
  win->AddFloat  ("Offset"    ,sOFFSET(KE_Alias_Para,Offset   ), 0.0f,1024.0f,0.05f);
}
#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Effect-List                                                        ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if !sPLAYER
KEffect2 EffectList[] =
{
  { 
    "camera"  ,0         ,0x01,0,
    sizeof(KE_Camera_Para),
    sizeof(KE_Camera_Edit),
    0,0,KE_Camera_OnPaint,KE_Default_OnSet,
    KE_Camera_OnPara,
    { 0 },
  },
  { 
    "mesh"    ,0xffffc0ff,0x02,0,
    sizeof(KE_Mesh_Para),
    sizeof(KE_Mesh_Edit),
    0,0,KE_Mesh_OnPaint  ,KE_Default_OnSet,
    KE_Mesh_OnPara,
    { KC_MESH },
  },
  { 
    "scene"   ,0xffffffa0,0x03,0,
    sizeof(KE_Scene_Para),
    sizeof(KE_Scene_Edit),
    KE_Scene_OnInit,KE_Scene_OnExit,KE_Scene_OnPaint ,KE_Scene_OnSet,
    KE_Scene_OnPara,
    { KC_SCENE2 },
  },
  { 
    "alias"    ,0        ,0x04,0,
    sizeof(KE_Alias_Para),
    0,
    0,0,KE_Alias_OnPaint ,KE_Default_OnSet,
    KE_Alias_OnPara,
    { KC_SCENE2 },
  },  


  { 
    "glare"   ,0xffffa070,0x20,0,
    sizeof(KE_Glare_Para),
    sizeof(KE_Glare_Edit),
    0,0,KE_Glare_OnPaint ,KE_Default_OnSet,
    KE_Glare_OnPara,
    { 0 },
  },


  { 
    0 
  }
};
#endif

/****************************************************************************/
/****************************************************************************/

#endif

