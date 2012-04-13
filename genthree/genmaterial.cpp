// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "genmaterial.hpp"
#include "genmesh.hpp"
#include "genmisc.hpp"
#include "genbitmap.hpp"
#include "genplayer.hpp"
#include "cslrt.hpp"

#include "_startdx.hpp"


sMAKEZONE(MeshCount,"MeshCount",0xff000040);
sMAKEZONE(MeshFinal,"MeshFinal",0xff0000c0);

/****************************************************************************/
/***                                                                      ***/
/***   GenScene                                                           ***/
/***                                                                      ***/
/****************************************************************************/

GenScene::GenScene()
{
  sSetMem(SRT,0,sOFFSET(GenScene,Childs)-sOFFSET(GenScene,SRT));
  Childs.Init(4);

  SRT[0] = 1.0f;
  SRT[1] = 1.0f;
  SRT[2] = 1.0f;
  Multiply = 1;
}

GenScene::~GenScene()
{
  Childs.Exit();
}

void GenScene::Tag()
{
  sInt i;
  for(i=0;i<Childs.Count;i++)
    sBroker->Need(Childs[i]);
  sBroker->Need(Mesh);
  sBroker->Need(Light);
  sBroker->Need(Part);
}

#if !sINTRO
void GenScene::Copy(sObject *o)
{
  GenScene *os;

  sVERIFY(o->GetClass()==sCID_GENSCENE)
  os = (GenScene *) o;

  Childs.Copy(os->Childs);
  sCopyMem(SRT,os->SRT,sOFFSET(GenScene,Childs)-sOFFSET(GenScene,SRT));
  _Label = os->_Label;
}
#endif

/****************************************************************************/

void GenScene::Paint(sMatrix &mat)
{
  sSystem->ClearLights();
  PaintR(mat,0);

#if sINTRO
//  sVERIFY(sSystem->GetLightCount());
//  if(sSystem->GetLightCount()==0)
//    sDPrintF("lightcount==0!\n");
#else
  sMatrix cam;
  if(sSystem->GetLightCount()==0)
  {
    sLightInfo li;
    li.Init();
    li.Type = sLI_DIR;
    li.Ambient = 0xff404040;
    li.Diffuse = 0xffffffff;
    li.Dir.Add3(cam.i,cam.j);
    li.Dir.Unit3();
    li.Mask = ~0;
    sSystem->AddLight(li);
  }
#endif

  PaintR(mat,1);
//  PaintR(mat,2);
#if sINTRO
  sSystem->ClearLights();
#endif
}

void GenScene::PaintR(sMatrix &mat,sInt action)
{
  sInt i,max,j;
  sMatrix newmat,trans,mx;

  newmat = mat;
  for(j=0;j<Multiply;j++)
  {
    trans.InitSRT(SRT);

    mx.Mul4(trans,newmat);
    newmat = mx;

    max = Childs.Count;
    for(i=0;i<max;i++)
      Childs[i]->PaintR(mx,action);

    if(action==0)
    {
      if(Light)
        Light->Set(mx);
    }
    else
    {
      if(action==1)
      {
        sSystem->SetMatrix(mx);
        if(Mesh)
          Mesh->Paint();
      }
#if !sINTRO_X
      else
      {
        if(Part)
          Part->Paint(mx,1);
      }
#endif
    }
  }
}

/****************************************************************************/

GenLight::GenLight()
{
  sSetMem(&Diffuse,0,sOFFSET(GenLight,Ordinal)+4-sOFFSET(GenLight,Diffuse));
/*
  Mode = 0;
  Mask = 0;
  Ordinal = 0;
  Range = 0x10000;
  RangeCut = 0x01000;
  Amplify = 0x10000;
  SpotFalloff = 0x08000;
  SpotInner = 0x00000;
  SpotOuter = 0x04000;
  Diffuse.Init(0x8000,0x8000,0x8000,0xffff);
  Ambient.Init(0x4000,0x4000,0x4000,0xffff);
  Specular.Init(0xffff,0xffff,0xffff,0xffff);
  */
}

#if !sINTRO
void GenLight::Copy(sObject *o)
{
  GenLight *lo;

  sVERIFY(o->GetClass()==sCID_GENLIGHT);
  lo = (GenLight *)o;

  sCopyMem(&Diffuse,&lo->Diffuse,sOFFSET(GenLight,Ordinal)+4-sOFFSET(GenLight,Diffuse));
  _Label = lo->_Label;
/*
  Range = lo->Range;
  RangeCut = lo->RangeCut;
  Amplify = lo->Amplify;
  SpotFalloff = lo->SpotFalloff;
  SpotInner = lo->SpotInner;
  SpotOuter = lo->SpotOuter;
  Diffuse = lo->Diffuse;
  Ambient = lo->Ambient;
  Specular = lo->Specular;
  Mode = lo->Mode;
  Mask = lo->Mask;
  sCopyString(Label,lo->Label,sizeof(Label));
  Ordinal = lo->Ordinal;
  */
}
#endif

void GenLight::Set(sMatrix &mat)
{
  sMatrix m2,m3;
  sMatrix *mp;

  mp = &mat;

  if(Mode & 8)
  {
    sSystem->GetTransform(sGT_MODELVIEW,m2);
    m2.TransR();
    m3.Mul3(mat,m2);
    mp = &m3;
  }

#if sINTRO
  sD3DLight light;
  sF32 amp;

  amp = Amplify/(65536.0f);

  light.Type = Mode&7;
  light.Diffuse.x = Diffuse.z*amp;
  light.Diffuse.y = Diffuse.y*amp;
  light.Diffuse.z = Diffuse.x*amp;
  light.Diffuse.w = 1.0f;//Diffuse.w*amp;
  light.Specular.x = Specular.z*amp;
  light.Specular.y = Specular.y*amp;
  light.Specular.z = Specular.x*amp;
  light.Specular.w = 1.0f;//Specular.w*amp;
  light.Ambient.x = Ambient.z*amp;
  light.Ambient.y = Ambient.y*amp;
  light.Ambient.z = Ambient.x*amp;
  light.Ambient.w = 1.0f;//Ambient.w*amp;
  light.Position[0] = mp->l.x;
  light.Position[1] = mp->l.y;
  light.Position[2] = mp->l.z;
  light.Direction[0] = mp->k.x;
  light.Direction[1] = mp->k.y;
  light.Direction[2] = mp->k.z;
  light.Range = Range;
  light.Falloff = SpotFalloff;
  light.Attenuation[0] = 1;
  light.Attenuation[1] = 0;
  light.Attenuation[2] = 0;
  light.Theta = SpotInner;
  light.Phi = SpotOuter;
  sSystem->AddLight(light,Mask);
#else
  sLightInfo light;

  light.Pos = mp->l;
  light.Dir = mp->k;
  light.Type = Mode&7;
  light.Mask = Mask;
  light.Range = Range;
  light.RangeCut = RangeCut;
  light.Diffuse = GetColor32(Diffuse);
  light.Ambient = GetColor32(Ambient);
  light.Specular = GetColor32(Specular);
  light.Amplify = Amplify;
  light.Gamma = SpotFalloff;
  light.Inner = SpotInner;
  light.Outer = SpotOuter;
  Ordinal = sSystem->AddLight(light);
#endif
}

/****************************************************************************/
/***                                                                      ***/
/***   GenMatPass                                                         ***/
/***                                                                      ***/
/****************************************************************************/

GenMatPass::GenMatPass()
{
  sSetMem(&Aspect,0,sOFFSET(GenMatPass,Optimised)+4-sOFFSET(GenMatPass,Aspect));

  Programm = MPP_STATIC;
  LightMask = 0x0000ffff;
  StatePtr = States;
//  sSystem->StateBase(StatePtr,0,0,0);
#if !sINTRO
  Aspect = 1.0;
  Enlarge = 0.0f;
  Size = 0.125;
  Color.Init(255,255,255,255);
  Flags = sMBF_ZON;
#endif

#if sINTRO
  MatLightSet = 0;
  MatLight.Ambient.Init(1.0f,1.0f,1.0f,1.0f);
  MatLight.Diffuse.Init(1.0f,1.0f,1.0f,1.0f);
  MatLight.Emissive.Init(0.0f,0.0f,0.0f,0.0f);
  MatLight.Specular.Init(1.0f,1.0f,1.0f,1.0f);
  MatLight.Power = 1.0f;
#endif
}

#if !sINTRO
void GenMatPass::Copy(sObject *o)
{
  GenMatPass *op;

  sVERIFY(o->GetClass()==sCID_GENMATPASS);
  op = (GenMatPass *) o;

  sCopyMem(&Aspect,&op->Aspect,sOFFSET(GenMatPass,Optimised)+4-sOFFSET(GenMatPass,Aspect));
  _Label = op->_Label;
}
#endif

void GenMatPass::Tag()
{
  sInt i;

  for(i=0;i<GENMAT_MAXTEX;i++)
  {
    sBroker->Need(Texture[i]);
    sBroker->Need(TexTrans[i]);
  }
}

/****************************************************************************/

void GenMatPass::Set()
{
  sInt i;
  sMatrix ttm,m2,m3;

  if(!Optimised)
  {
#if !sINTRO
    for(i=0;i<2;i++)
    {
      if(Texture[i] && TexTrans[i] && sSystem->GetGfxSystem()==sSF_DIRECT3D)
      {
        StatePtr[0] = sD3DTSS_TEXTURETRANSFORMFLAGS+i*0x20;
        StatePtr[1] = sD3DTTFF_COUNT2;
        StatePtr+=2;
      }
    }
#endif
    sSystem->StateEnd(StatePtr,States,sizeof(States));
    sSystem->StateOptimise(States);
    Optimised = 1;
  }
  sSystem->SetStates(States);
#if sINTRO
  sSystem->SetMatLight(MatLight);
#endif
  if(Flags&1)
    sSystem->SetState(sD3DRS_TEXTUREFACTOR,GetColor32(Color));
  for(i=0;i<2;i++)
  {
    if(Texture[i] && Texture[i]->Texture!=sINVALID)
    {
#if sINTRO
      sSystem->GetTransform(TexTransMode[i],ttm);
      ttm.TransR();
      if(TexTrans[i])
      {
        sSystem->SetState(sD3DTSS_TEXTURETRANSFORMFLAGS+i*0x20,2);
        m2 = ttm;
        m3.InitSRT(&TexTrans[i]->s.x);
//        TexTrans[i]->Calc(m3);
        ttm.Mul4(m2,m3);
      }
      else if(TexTransMode[i]==0)
      {
        ttm.Init();
        ttm.i.x = 0.5f;
        ttm.j.y = 0.5f;
        ttm.k.z = 0.5f;
        ttm.l.x = 0.5f;
        ttm.l.y = 0.5f;
        ttm.l.z = 0.5f;
      }
      sSystem->SetTexture(i,Texture[i]->Texture,&ttm);
#else
      switch(TexTransMode[i])
      {
      case 0:
        sSystem->GetTransform(sGT_UNIT,ttm);
        ttm.TransR();
        if(TexTrans[i])
        {
          m2 = ttm;
          TexTrans[i]->Calc(m3);
          ttm.Mul4(m2,m3);
          sSystem->SetTexture(i,Texture[i]->Texture,&ttm);
        }
        else
          sSystem->SetTexture(i,Texture[i]->Texture,0);
        break;
      case 1:
        sSystem->GetTransform(sGT_VIEW,ttm);
        ttm.TransR();
        if(TexTrans[i])
        {
          m2 = ttm;
          TexTrans[i]->Calc(m3);
          ttm.Mul4(m2,m3);
        }
        sSystem->SetTexture(i,Texture[i]->Texture,&ttm);
        break;
      case 2:
        sSystem->GetTransform(sGT_MODELVIEW,ttm);
        ttm.TransR();
        if(TexTrans[i])
        {
          m2 = ttm;
          TexTrans[i]->Calc(m3);
          ttm.Mul4(m2,m3);
        }
        sSystem->SetTexture(i,Texture[i]->Texture,&ttm);
        break;
      }
#endif
    }
    else
    {
      sSystem->SetTexture(i,sINVALID,0);
    }
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   GenMaterial                                                        ***/
/***                                                                      ***/
/****************************************************************************/

GenMaterial::GenMaterial()
{
  sSetMem(Passes,0,sizeof(Passes));
}

/****************************************************************************/

void GenMaterial::Set()
{
  if(Passes[0])
    Passes[0]->Set();
}

/****************************************************************************/

void GenMaterial::Default()
{
  Passes[0] = new GenMatPass();
}

void GenMaterial::Tag()
{
  sInt i;

  for(i=0;i<GENMAT_MAXPASS;i++)
    sBroker->Need(Passes[i]);
}

#if !sINTRO
void GenMaterial::Copy(sObject *o)
{
  GenMaterial *om;
  sInt i;

  sVERIFY(o->GetClass() == sCID_GENMATERIAL);
  
  om = (GenMaterial *) o;
  for(i=0;i<GENMAT_MAXPASS;i++)
    Passes[i] = om->Passes[i];
  _Label = om->_Label;
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Script                                                             ***/
/***                                                                      ***/
/****************************************************************************/
/*
sBool Scene_New(class ScriptRuntime *sr) // obsolete
{
  sr->PushO(new GenScene);
  return sTRUE;
}
*/
/****************************************************************************/

GenScene * __stdcall Scene_Mesh(GenMesh *mesh,sF323 s,sF323 r,sF323 t)
{
  GenScene *scene;

  SCRIPTVERIFY(mesh);
  scene = new GenScene;
  sCopyMem(scene->SRT,&s.x,9*4);
  scene->Mesh = mesh;
  mesh->Prepare();

  return scene;
}

GenScene * __stdcall Scene_Add(GenScene *s1,GenScene *s2)
{
  GenScene *sb;

  SCRIPTVERIFY(s1);
  SCRIPTVERIFY(s2);

  if(s1->Childs.Count>1)
  {
    sb = s1;
  }
  else
  {
    sb = new GenScene;
    *sb->Childs.Add() = s1;
  }
  *sb->Childs.Add() = s2;
  
  return sb;
}

GenScene * __stdcall Scene_Trans(GenScene *sa,sF323 s,sF323 r,sF323 t)
{
  GenScene *scene;

  SCRIPTVERIFY(sa);
  scene = new GenScene;
  sCopyMem(scene->SRT,&s.x,9*4);
  *scene->Childs.Add() = sa;

  return scene;
}

GenScene * __stdcall Scene_Multiply(GenScene *sa,sF323 s,sF323 r,sF323 t,sInt count)
{
  GenScene *scene;

  SCRIPTVERIFY(sa);
  scene = new GenScene;
  sCopyMem(scene->SRT,&s.x,9*4);
  *scene->Childs.Add() = sa;
  scene->Multiply = count>>16;

  return scene;
}

GenScene * __stdcall Scene_Paint(GenScene *sa)
{
  sMatrix mat;
  SCRIPTVERIFY(sa);

  mat.Init();
  sa->Paint(mat);
  return sa;
}

GenScene * __stdcall Scene_Label(GenScene *sa,sInt label)
{
  sa->_Label = label;
  return sa;
}

GenScene * __stdcall Scene_Light(sF323 s,sF323 r,sF323 t,sInt mode,sInt mask,sInt label,sF32 range,sF32 rangecut,sF32 amplify,sF32 spotfalloff,sF32 spotinner,sF32 spotouter,sInt4 diff,sInt4 amb,sInt4 spec)
{
  GenScene *scene;
  GenLight *light;

  scene = new GenScene;
  light = new GenLight;
  sCopyMem(scene->SRT,&s.x,9*4);
  scene->Multiply = 1;
  scene->Light = light;

  light->Mode = (mode>>16)+1;   // warning: bit 3 is camspace! don't overflow here!
  light->Mask = 1<<(mask>>16);
  light->Range = range;
  light->RangeCut = rangecut;
  light->Amplify = amplify;
  light->SpotFalloff = spotfalloff;
  light->SpotInner = spotinner*(sPIF/2);
  light->SpotOuter = spotouter*(sPIF/2);
  light->Diffuse = diff;
  light->Ambient = amb;
  light->Specular = spec;
  light->_Label = label;

  return scene;
}
/*
GenScene * __stdcall Scene_PaintLater(GenScene *sa)
{
  return sa;
}
*/
GenScene * __stdcall Scene_PartScene(GenParticles *p)
{
  GenScene *sa;

  sa = new GenScene;
  sa->Part = p;

  return sa;
}

/****************************************************************************/
/****************************************************************************/

#if sINTRO_X

GenMatPass *PopPass(sInt pass,sObject *obj)
{
  GenMaterial *mat;

  sVERIFY(obj->GetClass()==sCID_GENMATERIAL);
  mat = (GenMaterial *) obj;

  pass = pass>>16;
  if(mat->Passes[pass]==0)
    mat->Passes[pass] = new GenMatPass;
  return mat->Passes[pass];
}

#else
GenMatPass *PopPass(sInt pass,sObject *obj)
{
  GenMaterial *mat;
  GenMesh *mesh;

  pass = pass>>16;
#if !sINTRO
  if(!obj)
  {
    SCRIPTVERIFY(0);
  }
#endif
  if(obj->GetClass()==sCID_GENMESH)
  {
    mesh = (GenMesh *) obj;
    sVERIFY(mesh->Mtrl.Count>=2);
    mat = mesh->Mtrl[1].Material;
#if sINTRO
    sVERIFY(mat)
#else
    if(mat==0)
    {
      mat = new GenMaterial;
      mesh->Mtrl[1].Material =  mat;
    }
#endif
  }
  else
  {
    SCRIPTVERIFY(obj->GetClass()==sCID_GENMATERIAL);
    mat = (GenMaterial *) obj;
  }

  if(mat->Passes[pass]==0)
    mat->Passes[pass] = new GenMatPass;
  SCRIPTVERIFY(mat->Passes[pass]);
  return mat->Passes[pass];
}
#endif

/****************************************************************************/

GenMaterial * __stdcall Material_NewMaterial()
{
  return new GenMaterial;
}

sObject * __stdcall MatPass_MatBase(sObject *obj,sInt pass,sInt flags,sInt mode,sInt rp,sInt4 color)
{
  GenMatPass *matpass;
//  sInt i;

  matpass = PopPass(pass,obj);
  SCRIPTVERIFY(matpass);

  flags = flags >> 16;
  matpass->Color = color;
  if(flags & (sMBF_COLOR))
    matpass->Flags |= 1;

  sSystem->StateBase(matpass->StatePtr,flags,mode>>16,GetColor32(color));
#if !sINTRO
  matpass->RenderPass = rp>>16;
#endif
  return obj;
}

sObject * __stdcall MatPass_MatTexture(sObject *obj,GenBitmap *bm,sInt pass,sInt stage,sInt flags)
{
  GenMatPass *matpass;
  sInt i;

  stage = stage>>16;
  flags = flags>>16;

  matpass = PopPass(pass,obj);
  SCRIPTVERIFY(matpass);
  SCRIPTVERIFY(bm);
  SCRIPTVERIFY(bm->GetClass()==sCID_GENBITMAP);

  matpass->TexTransMode[stage] = 0;
  i = (flags & sMTF_UVMASK)>>8;
  if(i>0x0a)
  {
    flags = (flags & ~sMTF_UVMASK) | sMTF_UVPOS;
    matpass->TexTransMode[stage] = i-0x0a;
  }
/*
  if((flags & sMTF_UVMASK)==0x0b00)
  {
    flags = (flags & ~sMTF_UVMASK) | sMTF_UVPOS;
    matpass->TexTransMode[stage] = 1;
  }
  if((flags & sMTF_UVMASK)==0x0c00)
  {
    flags = (flags & ~sMTF_UVMASK) | sMTF_UVPOS;
    matpass->TexTransMode[stage] = 2;
  }
  */
  sSystem->StateTex(matpass->StatePtr,stage,flags);
  if(bm && bm->Texture == sINVALID)
    Bitmap_MakeTexture(bm);
  matpass->Texture[stage] = bm;     // bm may be 0

  return obj;
}

sObject * __stdcall MatPass_MatFinalizer(sObject *obj,sInt pass,sInt program,sF32 aspect,sF32 size,sF32 enlarge)
{
  GenMatPass *matpass;

  matpass = PopPass(pass,obj);
  SCRIPTVERIFY(matpass);
  
  matpass->Size = size;
  matpass->Aspect = aspect;
  matpass->Programm = program>>16;
  matpass->Enlarge = enlarge;

  return obj;
}

sObject * __stdcall MatPass_MatLabel(sObject *obj,sInt label,sInt pass)
{
  GenMatPass *matpass;

  matpass = PopPass(pass,obj);
  SCRIPTVERIFY(matpass);
 
  matpass->_Label = label;

  return obj;
}

sObject * __stdcall MatPass_MatTexTrans(sObject *obj,sInt label,sInt pass,sInt stage,sF323 s,sF323 r,sF323 t)
{
  GenMatPass *matpass;
  GenMatrix *mat;

  stage =  stage>>16;
  matpass = PopPass(pass,obj);
  SCRIPTVERIFY(matpass);
  SCRIPTVERIFY(stage>=0 && stage<GENMAT_MAXTEX);

  mat = new GenMatrix;
  matpass->TexTrans[stage] = mat;

  sCopyMem(&mat->s,&s,9*4);

//  matpass->TFlags[stage] |= sMTF_UVTRANS;
  mat->_Label = label;
 
  return obj;
}

#if !sINTRO_X
sObject * __stdcall MatPass_MatDX7(sObject *obj,sInt pass,sInt color0,sInt color1,sInt alpha0,sInt alpha1)
{
  GenMatPass *matpass;
  sInt i;
  sU32 *data;

  matpass = PopPass(pass,obj);
  SCRIPTVERIFY(matpass);
  data = matpass->StatePtr;

  if(sSystem->GetGfxSystem()==sSF_DIRECT3D)
  {
    for(i=0;i<2;i++)
    {
      color0 = color0>>16;
      alpha0 = alpha0>>16;
      *data++ = sD3DTSS_COLOROP+i*32;
      *data++ = (color0>>8)+1;
      *data++ = sD3DTSS_COLORARG1+i*32;
      *data++ = ((color0&0x03)) | ((color0&0x0c)<<2);
      *data++ = sD3DTSS_COLORARG2+i*32;
      *data++ = (((color0&0x30)>>4)) | ((color0&0xc0)>>2);
      *data++ = sD3DTSS_ALPHAOP+i*32;
      *data++ = (alpha0>>8)+1;
      *data++ = sD3DTSS_ALPHAARG1+i*32;
      *data++ = ((alpha0&0x03)) | ((alpha0&0x0c)<<2);
      *data++ = sD3DTSS_ALPHAARG2+i*32;
      *data++ = (((alpha0&0x30)>>4)) | ((alpha0&0xc0)>>2);

      color0 = color1;
      alpha0 = alpha1;
    }
  }

  matpass->StatePtr = data;
  return obj;
}
#endif

#if !sINTRO_X
sObject * __stdcall MatPass_MatBlend(sObject *obj,sInt pass,sInt alphatest,sInt alpharef,sInt source,sInt op,sInt dest)
{
  GenMatPass *matpass;
  sU32 *data;

  matpass = PopPass(pass,obj);
  SCRIPTVERIFY(matpass);
  data = matpass->StatePtr;

  if(sSystem->GetGfxSystem()==sSF_DIRECT3D)
  {
    *data++ = sD3DRS_ALPHAFUNC;
    *data++ = (alphatest>>16)+1;
    *data++ = sD3DRS_ALPHAREF;
    *data++ = (alpharef>>16);
    if(op)
    {
      *data++ = sD3DRS_SRCBLEND;
      *data++ = (source>>16)+1;
      *data++ = sD3DRS_BLENDOP;
      *data++ = (op>>16);
      *data++ = sD3DRS_DESTBLEND;
      *data++ = (dest>>16)+1;
      *data++ = sD3DRS_ALPHABLENDENABLE;
      *data++ = 1;
    }
    else
    {
      *data++ = sD3DRS_ALPHABLENDENABLE;
      *data++ = 0;
    }
  }

  matpass->StatePtr = data;
  return obj;
}
#endif

#if !sINTRO_X
sObject * __stdcall MatPass_MatStencil(sObject *obj,sInt pass,sInt stenciltest,sInt stencilref,sInt sfail,sInt zfail,sInt zpass)
{
  GenMatPass *matpass;
  sU32 *data;

  matpass = PopPass(pass,obj);
  SCRIPTVERIFY(matpass);
  data = matpass->StatePtr;

  if(sSystem->GetGfxSystem()==sSF_DIRECT3D)
  {
    *data++ = sD3DRS_STENCILENABLE;
    *data++ = 1;
    *data++ = sD3DRS_STENCILFUNC;
    *data++ = (stenciltest>>16)+1;
    *data++ = sD3DRS_STENCILREF;
    *data++ = (stencilref>>16);
    *data++ = sD3DRS_STENCILFAIL;
    *data++ = (sfail>>16)+1;
    *data++ = sD3DRS_STENCILZFAIL;
    *data++ = (zfail>>16)+1;
    *data++ = sD3DRS_STENCILPASS;
    *data++ = (zpass>>16)+1;
  }

  matpass->StatePtr = data;
  return obj;
}
#endif

sObject * __stdcall MatPass_MatLight(sObject *obj,sInt pass,sInt ds,sInt ss,sInt as,sInt es,sInt4 d,sInt4 s,sInt4 a,sInt4 e,sF32 power,sInt specular,sInt mask)
{
  GenMatPass *matpass;
  sU32 *data;

  matpass = PopPass(pass,obj);
  SCRIPTVERIFY(matpass);
  data = matpass->StatePtr;

#if sINTRO
  *data++ = sD3DRS_DIFFUSEMATERIALSOURCE;
  *data++ = ds>>16;
  *data++ = sD3DRS_AMBIENTMATERIALSOURCE;
  *data++ = as>>16;
  *data++ = sD3DRS_SPECULARMATERIALSOURCE;
  *data++ = ss>>16;
  *data++ = sD3DRS_EMISSIVEMATERIALSOURCE;
  *data++ = es>>16;
  *data++ = sD3DRS_SPECULARENABLE;
  *data++ = (specular>>16)&1;

  matpass->MatLightSet = 1;
  matpass->MatLight.Diffuse.Init(d);
  matpass->MatLight.Ambient.Init(a);
  matpass->MatLight.Specular.Init(s);
  matpass->MatLight.Emissive.Init(e);
  matpass->MatLight.Power = power;
#else
  if(sSystem->GetGfxSystem()==sSF_DIRECT3D)
  {
    *data++ = sD3DMAT_DIFFUSE;
    *data++ = GetColor32(d);
    *data++ = sD3DMAT_AMBIENT;
    *data++ = GetColor32(a);
    *data++ = sD3DMAT_SPECULAR;
    *data++ = GetColor32(s);
    *data++ = sD3DMAT_EMISSIVE;
    *data++ = GetColor32(e);
    *data++ = sD3DMAT_POWER;
    *(sF32 *)data = power; data++;

    *data++ = sD3DRS_DIFFUSEMATERIALSOURCE;
    *data++ = ds>>16;
    *data++ = sD3DRS_AMBIENTMATERIALSOURCE;
    *data++ = as>>16;
    *data++ = sD3DRS_SPECULARMATERIALSOURCE;
    *data++ = ss>>16;
    *data++ = sD3DRS_EMISSIVEMATERIALSOURCE;
    *data++ = es>>16;

    *data++ = sD3DRS_SPECULARENABLE;
    *data++ = specular>>16;

    /* mach ich erst wenn das licht an ist */
  }
#endif

  matpass->LightMask = mask;
  matpass->StatePtr = data;
  return obj;
}

/****************************************************************************/
/***                                                                      ***/
/***   Particle System                                                    ***/
/***                                                                      ***/
/****************************************************************************/

GenParticles::GenParticles()
{
#if !sINTRO_X
  sSetMem(&Rate,0,sOFFSET(GenParticles,__last)-sOFFSET(GenParticles,Rate));
#if !sINTRO
  Rate = 25.0f;
  MaxPart = 1024;
  Lifetime = 0.125f;
#endif
  Scale.Init(1.0f,1.0f,1.0f);
#if !sINTRO_X
  Col0.Init(255,255,255,0);
  Col1.Init(255,255,255,0);
  Col2.Init(255,255,255,0);
  ColTime.Init(0.125,0.5,0.875);
  Size.Init(0.125,0.125);
  SizeTime.Init(0.125,0.875);
#endif
//  RateCount = 0;


//  LastFrame = 0;
  LastTime = -1;
  Part.Init();
  Geometry = sSystem->GeoAdd(sFVF_DOUBLE,sGEO_QUAD|sGEO_DYNAMIC);
  Material = new GenMaterial;
  Material->Default();
//  Spline = 0;

#endif
}

GenParticles::~GenParticles()
{
#if !sINTRO_X
  sSystem->GeoRem(Geometry);
  Part.Exit();
#endif
}

#if !sINTRO
void GenParticles::Copy(sObject *o)
{
  GenParticles *op;

  sVERIFY(o->GetClass() == sCID_GENPARTICLES);

  op = (GenParticles *) o;

  sCopyMem(&Rate,&op->Rate,sOFFSET(GenParticles,__last)-sOFFSET(GenParticles,Rate));

//  Material = op->Material;
//  Spline = op->Spline;
  _Label = op->_Label;
}
#endif

void GenParticles::Tag()
{
#if !sINTRO_X
  sBroker->Need(Material);
  sBroker->Need(Spline);
#endif
}

extern sInt TimeNow;
extern sInt TotalFrame;

#if !sINTRO_X
void GenParticles::Paint(sMatrix &mat,sInt time)
{
  sInt i,j,k;
  GenParticle *part;
  sF32 *fp;
  sF32 scatter,surface,size;
  sVector dx,dy,v,n,up;
  sMatrix unitmat,view,r;
  sU32 col;
  static sF32 quadu[] = {  0, 0, 1, 1, 0 };
  static sF32 quads[] = { -1,-1, 1, 1,-1 };

  sF32 jitter;
  sF32 s1,s2,st1,st2;
  sF32 ct1,ct2,ct3;
  sF32 f,fs,fc;
  sVector c1,c2,c3;
#if !sINTRO_X
  sF32 spval[4];
#endif

#if sINTRO
  i = TotalFrame;
#else
  sVERIFY(CurrentGenPlayer);
  i = CurrentGenPlayer->TotalFrame;
#endif

  if(LastFrame!=i)
  {
    LastFrame=i;

#if sINTRO
    j = TimeNow/44;
#else
    j = sSystem->GetTime();
#endif

    i = j/10;
    jitter = (j%10)*0.1f;
    time = i-LastTime;
    if(LastTime==-1)
      time = 1;
    if(time>50)
      time = 50;
    LastTime = i;

    sSystem->GetTransform(sGT_MODELVIEW,view);
    view.Trans3();
    unitmat.Init();
    s1 = Size.x;
    s2 = Size.y;
    st1 = SizeTime.x;
    st2 = SizeTime.y;
    ct1 = ColTime.x;
    ct2 = ColTime.y;
    ct3 = ColTime.z;
    c1.Init(Col0);
    c2.Init(Col1);
    c3.Init(Col2);

    Material->Set();
    sSystem->SetMatrix(unitmat);
    sSystem->GeoBegin(Geometry,Part.Count*4,0,&fp,0);

    for(j=0;j<Part.Count;)
    {
      part = &Part[j];
      for(i=0;i<time;i++)
      {
        part->xs += Gravity.x;
        part->ys += Gravity.y;
        part->zs += Gravity.z;
        part->x += part->xs;
        part->y += part->ys;
        part->z += part->zs;
        part->time += 0.01f/Lifetime;
        part->rot += RotSpeed;

#if !sINTRO_X
        if(SplineMode==2 && Spline)
        {
          Spline->Calc(part->time,spval);
          part->xs = (spval[0]-part->x)*SplineForce;
          part->ys = (spval[1]-part->y)*SplineForce;
          part->zs = (spval[2]-part->z)*SplineForce;
        }
#endif
      }

      if(part->time >= 1.0f)
      {
        Part[j] = Part[--Part.Count];
      }
      else
      {
        if(part->time < st1)
          size = sFade(0,s1,part->time/st1);
        else if(part->time < st2)
          size = sFade(s1,s2,(part->time-st1)/(st2-st1));
        else
          size = sFade(s2,0,(part->time-st2)/(1-st2));

        if(part->time < ct1)
        {
          v = c1;
          v.Scale4(part->time/ct1);
        }
        else if(part->time < ct2)
        {
          v.Lin4(c1,c2,(part->time-ct1)/(ct2-ct1));
        }
        else if(part->time < ct3)
        {
          v.Lin4(c2,c3,(part->time-ct2)/(ct3-ct2));
        }
        else
        {
          v = c3;
          v.Scale4(1-(part->time-ct3)/(1-ct3));
        }


        col = (sFtol(v.x*255)<<16)
            | (sFtol(v.y*255)<< 8)
            | (sFtol(v.z*255)    )
            | (sFtol(v.w*255)<<24);

        if(Flags&1)
        {
          f = (part->rot+(RotSpeed*jitter/65536.0f))/sPI2F;
          fs = sFSin(f);
          fc = sFCos(f);
          dx.Init();
          dx.AddScale3(view.i,fc);
          dx.AddScale3(view.j,fs);
          dy.Init();
          dy.AddScale3(view.i,-fs);
          dy.AddScale3(view.j,fc);
        }
        else
        {
          dx = view.i;
          dy = view.j;
        }
        dx.Scale3(size);
        dy.Scale3(size);

        v.x = part->x + part->xs*jitter;
        v.y = part->y + part->ys*jitter;
        v.z = part->z + part->zs*jitter;
#if !sINTRO_X
        if(SplineMode==1 && Spline)
        {
          Spline->Calc(part->time,spval);
          v.x += spval[0];
          v.y += spval[1];
          v.z += spval[2];
        }
#endif

        for(k=0;k<4;k++)
        {
          *fp++ = v.x + dx.x*quads[k] + dy.x*quads[k+1];
          *fp++ = v.y + dx.y*quads[k] + dy.y*quads[k+1];
          *fp++ = v.z + dx.z*quads[k] + dy.z*quads[k+1];
          *(sU32 *)fp = col; fp++;
          *fp++ = quadu[k+1];
          *fp++ = quadu[k];
          *fp++ = 0.0f;
          *fp++ = 0.0f;
        }

        j++;
      }
    }

    sSystem->GeoEnd(Geometry,Part.Count*4,0);
  	sSystem->GeoDraw(Geometry);
  }





  Part.AtLeast(MaxPart);

  RateCount += time;

  r.InitEulerPI2(&Rotate.x);
  scatter = Scatter;
  surface = Surface;

  up.Init3(0,1,0);
  while(RateCount > 0 && Part.Count < MaxPart)
  {
    n.InitRnd();
    v = n;
    n.Unit3();
    v.Scale3(1-surface);
    v.AddScale3(n,surface);
    if((Flags&1) && v.y<0)
      v.y = -v.y;

    n.InitRnd();
    n.Unit3();
    if(Scatter<0)
    {
      n.Scale3(1+scatter);
      n.AddScale3(v,-scatter);
    }
    else
    {
      n.Scale3(1-scatter);
      n.AddScale3(up,scatter);
    }
    
    v.x *= Scale.x;
    v.y *= Scale.y;
    v.z *= Scale.z;

    n.Rotate3(r);
    n.Unit3();
    n.Scale3(Speed+sFGetRnd()*SpeedRandom-SpeedRandom/2);
    v.Rotate3(r);

    part = &Part[Part.Count++];
    part->x = v.x + Translate.x + mat.l.x;
    part->y = v.y + Translate.y + mat.l.y;
    part->z = v.z + Translate.z + mat.l.z;
    part->time = 0;
    part->xs = n.x;
    part->ys = n.y;
    part->zs = n.z;
    part->rot = RotStart + sFGetRnd()*RotRand-RotRand/(2);
    RateCount -= (100/Rate);
  }
}
#endif

/****************************************************************************/
/****************************************************************************/

GenParticles * __stdcall Part_New(GenMaterial *mtrl,sF32 rate,sF32 jitter,sInt maxpart,sF32 life,sF323 grav)
{
  GenParticles *part;

  part = new GenParticles;

#if !sINTRO_X
  part->Material = mtrl;
  sCopyMem(&part->Rate,&rate,sizeof(rate)+sizeof(jitter)+sizeof(maxpart)+sizeof(life)+sizeof(grav));
  part->MaxPart >>= 16;
#endif
  /*
  part->Rate = rate;
  part->Jitter = jitter;
  part->MaxPart = maxpart>>16;
  part->Lifetime = life;
  part->Gravity = grav;
*/
  return part;
}

GenParticles * __stdcall Part_Emitter(GenParticles *sys,sF323 s,sF323 r,sF323 t,sInt flags,sF32 surface,sF32 scatter,sF32 speed,sF32 speedrand)
{
#if !sINTRO_X
  SCRIPTVERIFY(sys);

  sCopyMem(&sys->Scale,&s,sizeof(s)+sizeof(r)+sizeof(t)+sizeof(flags)+sizeof(surface)+sizeof(scatter)+sizeof(speed)+sizeof(speedrand));
  sys->Flags >>= 16;
/*
  sys->Scale = s;
  sys->Rotate = r;
  sys->Translate = t;
  sys->Flags = flags>>16;
  sys->Surface = surface;
  sys->Scatter = scatter;
  sys->Speed = speed;
  sys->SpeedRandom = speedrand;
*/
#endif
  return sys;
}

GenParticles * __stdcall Part_Rotate(GenParticles *sys,sF32 rotstart,sF32 rotrand,sF32 rotspeed)
{
#if !sINTRO_X
  SCRIPTVERIFY(sys);

  sCopyMem(&sys->RotStart,&rotstart,sizeof(rotstart)+sizeof(rotrand)+sizeof(rotspeed));
/*
  sys->RotStart = rotstart;
  sys->RotRand = rotrand;
  sys->RotSpeed = rotspeed;
  */
  sys->Flags |= 1;
#endif
  return sys;
}

GenParticles * __stdcall Part_Life(GenParticles *sys,sF322 size,sF322 sizetime,sF32 aspect,sInt4 col0,sInt4 col1,sInt4 col2,sF323 coltime)
{
#if !sINTRO_X
  SCRIPTVERIFY(sys);

  sCopyMem(&sys->Size,&size,sizeof(size)+sizeof(sizetime)+sizeof(aspect)+sizeof(col0)+sizeof(col1)+sizeof(col2)+sizeof(coltime));
/*
  sys->Size = size;
  sys->SizeTime = sizetime;
  sys->Aspect = aspect;
  sys->Col0 = col0;
  sys->Col1 = col1;
  sys->Col2 = col2;
  sys->ColTime = coltime;
  */
#endif
  return sys;
}

#if !sINTRO_X
GenParticles * __stdcall Part_Spline(GenParticles *sys,GenSpline *spline,sInt mode,sF32 force)
{
  SCRIPTVERIFY(sys);
  SCRIPTVERIFY(spline);

  sys->Spline = spline;
  sys->SplineMode = mode>>16;
  sys->SplineForce = force;
  
  return sys;
}
#endif

/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/****************************************************************************/

void GenMesh::DoPass1(GenMeshPass *pass,sInt mtrl)
{
  sU16 *ip;
  sInt *vp;
  sInt i,j,e,e0;
  GenMeshVert *v;
  sInt ic,vc,vi,vcmax,icmax;
  sInt i0,i1,i2;
  sInt fi;
  sBool quadbuffer;               // don't need real indexbuffer, render quads
  sBool lines;

  sInt prg;

  static sInt statvb[0x8000];
  static sU16 statib[4*0x8000];

  
  quadbuffer = 1;
  vcmax = 0x7f00/4;
  icmax = 0x7fff0000;

  prg = pass->MatPass->Programm;

  if(prg==MPP_STATIC || prg==MPP_DYNAMIC /*|| prg==MPP_SHADOW*/)
  {
    quadbuffer = 0;
    vcmax = 0x7f00;
    icmax = 0x17000;
  }

#if sINTRO_X
  lines = (prg==MPP_THICKLINES);
#else
  lines = ((prg==MPP_OUTLINES || prg==MPP_THICKLINES || prg==MPP_FINNS));
#endif

// count everything

  sZONE(MeshCount);
  sVERIFY(pass->BatchCount==0);

  pass->BatchCount = 0;
  fi = 0;
  for(i=0;i<Edge.Count;i++)
    Edge[i].Select = 0;

  while(fi<Face.Count)
  {
    ic = 0;
    vc = 0;
    vp = statvb;
    ip = statib;
    for(i=0;i<Vert.Count;i++)
      Vert[i].Select = 0;

    while(fi<Face.Count && vc<vcmax && ic<icmax)
    {
      if(Face[fi].Material == mtrl)
      {
        e0 = Face[fi].Edge;
        e = e0;
        j = 0;
        do
        {
          sVERIFY(GetFaceId(e)==GetFaceId(e0));
          if(lines)
          {
            if(GetEdge(e)->Select==0)
            {
              GetEdge(e)->Select = 1;
              *vp++ = GetVertId(e);
              *vp++ = GetVertId(e^1);
              *vp++ = GetFaceId(e);
              *vp++ = GetFaceId(e^1);
              vc+=4;
            }
          }
          else
          {
            v = GetVert(e);
            if(!v->Select)
            {
              v->Select = 1;
              v->Temp2 = vc;
              vi = v-Vert.Array;
              *vp++ = vi;
              vc++;
            }
            j++;
          }
          e = NextFaceEdge(e);
        }
        while(e!=e0);

        if(!quadbuffer)
        {
          sVERIFY(j>=3);
          e = Face[fi].Edge;
          i2 = GetVert(e)->Temp2; e = NextFaceEdge(e);
          e0 = e;
          i1 = GetVert(e)->Temp2; e = NextFaceEdge(e);
          i0 = GetVert(e)->Temp2; e = NextFaceEdge(e);
          do
          {
            *ip++ = i2;
            *ip++ = i1;
            *ip++ = i0;
            ic+=3;
            i1 = i0;
            i0 = GetVert(e)->Temp2;
            e = NextFaceEdge(e);
          }
          while(e!=e0);
        }
      }
      fi++;
    }
    if(vc>0)
    {
      if(quadbuffer)
      {
        pass->IndexCount[pass->BatchCount]=0;
        pass->IndexBuffers[pass->BatchCount]=0;
      }
      else
      {
        sVERIFY(ic);
        pass->IndexCount[pass->BatchCount]=ic;
        pass->IndexBuffers[pass->BatchCount] = new sU16[ic];
        sCopyMem(pass->IndexBuffers[pass->BatchCount],statib,ic*2);
      }
      pass->VertexCount[pass->BatchCount]=vc;
      pass->VertexBuffers[pass->BatchCount] = new sInt[vc];
      sCopyMem(pass->VertexBuffers[pass->BatchCount],statvb,vc*4);
      pass->Geometry[pass->BatchCount] = 0;

      pass->BatchCount++;
    }
    else
    {
      sVERIFY(fi==Face.Count);          // don't create any buffers if no vertices were found. ensure the loop terminates!
    }
  }
}

// create index/vertex buffer

void GenMesh::DoPass2(GenMeshPass *pass,sInt mtrl)
{
  sVertex3d *v3;
  sF32 *vb;
  sU16 *ip,*ip2;
  sInt *vp;
  sInt i,j,batch;
  sInt ic,vc;
#if !sINTRO
  sInt vvs,vuv,vnr;
#endif
  sInt handle;
  sInt prg;
  sInt facedots;
  sInt lines;
  sF32 enlarge;

  sZONE(MeshFinal);

  facedots = 0;

  prg = pass->MatPass->Programm;

  for(batch=0;batch<pass->BatchCount;batch++)
  {
    handle = pass->Geometry[batch];
    if(handle==0)
    {
      switch(prg)
      {
      case MPP_STATIC:
        handle = sSystem->GeoAdd(sFVF_DEFAULT3D,sGEO_TRI|sGEO_STATIC);
        break;
      case MPP_DYNAMIC:
//      case MPP_SHADOW:
        handle = sSystem->GeoAdd(sFVF_DEFAULT3D,sGEO_TRI);
        break;
      case MPP_SPRITES:
#if !sINTRO_X
      case MPP_TREES:
      case MPP_SPIKES:
      case MPP_OUTLINES:
      case MPP_FINNS:
#endif
      case MPP_THICKLINES:
        handle = sSystem->GeoAdd(sFVF_DEFAULT3D,sGEO_QUAD);
        break;
      default:
        sVERIFYFALSE;
      }
      pass->Geometry[batch] = handle;
    }

    pass->MatPass->Set();
    lines = 0;
    if(sSystem->GeoDraw(handle))
    {
      ip = pass->IndexBuffers[batch];
      vp = pass->VertexBuffers[batch];
      ic = pass->IndexCount[batch];
      vc = pass->VertexCount[batch];
      enlarge = pass->MatPass->Enlarge;
#if sINTRO
      sVERIFY(VertMask == (sGMF_POS|sGMF_NORMAL|sGMF_UV0));
//      if(VertMask != (sGMF_POS|sGMF_NORMAL|sGMF_UV0))
//        sFatal("Ficken");
#endif
      switch(prg)
      {
#if !sINTRO
      default:
        sVERIFYFALSE;
#endif
      case MPP_STATIC:
      case MPP_DYNAMIC:
        sSystem->GeoBegin(handle,vc,ic,(sF32 **)&v3,&ip2);

#if sINTRO
        for(i=0;i<vc;i++)
        {
          j = *vp++;
          vb = &VertBuf[j*3].x;
          v3->x  = vb[0];
          v3->y  = vb[1];
          v3->z  = vb[2];
          v3->nx = vb[4+0];
          v3->ny = vb[4+1];
          v3->nz = vb[4+2];
          v3->u  = vb[8+0];
          v3->v  = vb[8+1];
          v3++;
        }
#else
        vvs = VertSize;
        sVERIFY(VertMap[sGMI_UV0]!=-1);
        sVERIFY(VertMap[sGMI_NORMAL]!=-1);
        vuv = VertMap[sGMI_UV0]*4; if(vuv==-1) vuv=0;
        vnr = VertMap[sGMI_NORMAL]*4; if(vnr==-1) vnr=0;
        for(i=0;i<vc;i++)
        {
          j = *vp++;
          vb = &VertBuf[j*vvs].x;
          v3->x  = vb[0];
          v3->y  = vb[1];
          v3->z  = vb[2];
          v3->nx = vb[vnr+0];
          v3->ny = vb[vnr+1];
          v3->nz = vb[vnr+2];
          v3->u  = vb[vuv+0];
          v3->v  = vb[vuv+1];
          v3++;
        }
#endif

        sCopyMem(ip2,ip,ic*2);
        sSystem->GeoEnd(handle);
        sSystem->GeoDraw(handle);
        if(prg==MPP_DYNAMIC)
          sSystem->GeoFlush(handle);
        break;

      case MPP_THICKLINES:
#if !sINTRO_X
      case MPP_OUTLINES:
      case MPP_FINNS:
#endif
        lines = 1;
      case MPP_SPRITES:
#if !sINTRO_X
      case MPP_TREES:
      case MPP_SPIKES:
#endif
        {
          sMatrix mat,matt;
          sVector vx,vy;
          sVector p,p0,p1,pp0,pp1,n,n0,n1;
          sInt j0,j1;
          sF32 s0,s1;
          sInt f0,f1;
          sInt count;

          sSystem->GetTransform(sGT_MODELVIEW,mat);
          matt = mat;
          matt.Trans3();
          s0 = (pass->MatPass->Size)*(pass->MatPass->Aspect);
          s1 = (pass->MatPass->Size)/(pass->MatPass->Aspect);
#if !sINTRO
          vvs = VertSize;
          vnr = VertMap[sGMI_NORMAL]; if(vnr==-1) vnr=0;
#endif
          vx.Scale3(matt.j,-s0);
          vy.Scale3(matt.i,s1);

          if(lines)
          {
            vx.Init();
            vy.Init();
            if(facedots!=1 && prg != MPP_THICKLINES)
            {
              for(i=0;i<Face.Count;i++)
              {
#if !sINTRO
                p0 = VertBuf[GetVertId(Face[i].Edge)*vvs];
#else
                p0 = VertBuf[GetVertId(Face[i].Edge)*3];
#endif
                pp0.Rotate34(mat,p0);
                CalcFaceNormal(n0,i);
                n.Rotate3(mat,n0); 
                Face[i].Temp = n.Dot3(pp0)>0;
              }

              facedots = 1;
            }
            vc = vc>>2;
          }

          sSystem->GeoBegin(handle,vc*4,0,(sF32 **)&v3,0);
          count = 0;

          for(i=0;i<vc;i++)
          {
            if(lines)
            {
              j0 = *vp++;
              j1 = *vp++;
              f0 = *vp++;
              f1 = *vp++;
#if sINTRO
              p0 = VertBuf[j0*3];
              p1 = VertBuf[j1*3];
              n0 = VertBuf[j0*3+1];
              n1 = VertBuf[j1*3+1];
#else
              p0 = VertBuf[j0*vvs];
              p1 = VertBuf[j1*vvs];
              n0 = VertBuf[j0*vvs+vnr];
              n1 = VertBuf[j1*vvs+vnr];
#endif
              p0.AddScale3(n0,enlarge);
              p1.AddScale3(n1,enlarge);
              pp0.Rotate34(mat,p0);
              pp1.Rotate34(mat,p1);

#if !sINTRO_X
              if(prg!=MPP_THICKLINES)
              {
                if(Face[f0].Temp==Face[f1].Temp)
                  continue;
              }

              if(prg==MPP_OUTLINES || prg==MPP_THICKLINES)
#endif
              {
                p.x = pp0.x/pp0.z - pp1.x/pp1.z;
                p.y = pp0.y/pp0.z - pp1.y/pp1.z;
                p.z = 0;
                p.Unit3();

                vy.Scale3(matt.i,-p.x*s1);
                vy.AddScale3(matt.j,-p.y*s1);
                vx.Scale3(matt.j,p.x*s0);
                vx.AddScale3(matt.i,-p.y*s0);
              }
            }
            else
            {
              j = *vp++;
#if sINTRO
              p0 = p1 = VertBuf[j*3];
              n0 = n1 = VertBuf[j*3+1];
#else
              p0 = p1 = VertBuf[j*vvs];
              n0 = n1 = VertBuf[j*vvs+vnr];
#endif
#if !sINTRO_X
              if(prg == MPP_SPIKES)
              {
                vy.Cross3(n0,matt.k);
                vy.Unit3();
                vy.Scale3(s0);
                vx.Scale3(n0,-s1);
              }
#endif
#if !sINTRO
              if(prg == MPP_TREES)
                vx.Init4(0,s1*2,0,1);
#endif
            }

            v3->x  = p0.x-vx.x-vy.x;
            v3->y  = p0.y-vx.y-vy.y;
            v3->z  = p0.z-vx.z-vy.z;
            v3->nx = n0.x;
            v3->ny = n0.y;
            v3->nz = n0.z;
            v3->u  = 0.0f;
            v3->v  = 0.0f;
            v3++;
            v3->x  = p1.x-vx.x+vy.x;
            v3->y  = p1.y-vx.y+vy.y;
            v3->z  = p1.z-vx.z+vy.z;
            v3->nx = n1.x;
            v3->ny = n1.y;
            v3->nz = n1.z;
            v3->u  = 1.0f;
            v3->v  = 0.0f;
            v3++;
#if !sINTRO_X
            if(prg==MPP_FINNS)
            {
              p1.AddScale3(n1,s1);
              p0.AddScale3(n0,s1);
            }
            if(prg==MPP_SPIKES)
              vx.Init();
#endif
#if !sINTRO
            if(prg==MPP_TREES)
              vx.Init();
#endif
            v3->x  = p1.x+vx.x+vy.x;
            v3->y  = p1.y+vx.y+vy.y;
            v3->z  = p1.z+vx.z+vy.z;
            v3->nx = n1.x;
            v3->ny = n1.y;
            v3->nz = n1.z;
            v3->u  = 1.0f;
            v3->v  = 1.0f;
            v3++;
            v3->x  = p0.x+vx.x-vy.x;
            v3->y  = p0.y+vx.y-vy.y;
            v3->z  = p0.z+vx.z-vy.z;
            v3->nx = n0.x;
            v3->ny = n0.y;
            v3->nz = n0.z;
            v3->u  = 0.0f;
            v3->v  = 1.0f;
            v3++;
            count+=4;
          }
          sSystem->GeoEnd(handle,count,0);
          sSystem->GeoDraw(handle);
          sSystem->GeoFlush(handle);
        }
        break;

/*
#if !sINTRO
      case MPP_SHADOW:
        {
          sMatrix mat;
          sVector src;
          sF32 str;
          sVector p0,n,t;
          sF32 s0;

          vvs = VertSize;
          vnr = VertMap[sGMI_NORMAL]; if(vnr==-1) vnr=0;

          src.Init(0,5,0);
          str = 3;
          s0 = (pass->MatPass->Size);

          if(facedots!=2)
          {
            sSystem->GetTransform(sGT_MODEL,mat);
            for(i=0;i<Face.Count;i++)
            {
              p0 = VertBuf[GetVertId(Face[i].Edge)*vvs];
              p0.Sub3(src);
              CalcFaceNormal(t,i);
              n.Rotate3(mat,t); 
              Face[i].Temp = n.Dot3(p0)>0;
            }

            for(i=0;i<Edge.Count;i++)
            {
              j = Face[Edge[i].Face[0]].Temp;
              Vert[Edge[i].Vert[0]].Temp = j;
              Vert[Edge[i].Vert[1]].Temp = j;
            }
            facedots = 2;
          }

          sSystem->GeoBegin(handle,vc,ic,(sF32 **)&v3,&ip2);

          vvs = VertSize;
          vuv = VertMap[sGMI_UV0]*4; if(vuv==-1) vuv=0;
          vnr = VertMap[sGMI_NORMAL]*4; if(vnr==-1) vnr=0;
          for(i=0;i<vc;i++)
          {
            j = *vp++;
            vb = &VertBuf[j*vvs].x;

            if(Vert[Vert[j].First].Temp)
            {
              v3->x  = src.x + (vb[0]-src.x)*str;
              v3->y  = src.y + (vb[1]-src.y)*str;
              v3->z  = src.z + (vb[2]-src.z)*str;
            }
            else
            {
              v3->x  = vb[0]-vb[vnr+0]*s0;
              v3->y  = vb[1]-vb[vnr+1]*s0;
              v3->z  = vb[2]-vb[vnr+2]*s0;
            }
            v3->nx = vb[vnr+0];
            v3->ny = vb[vnr+1];
            v3->nz = vb[vnr+2];
            v3->u  = vb[vuv+0];
            v3->v  = vb[vuv+1];
            v3++;
          }

          sCopyMem(ip2,ip,ic*2);
//          ip+=ic;
          sSystem->GeoEnd(handle);
          sSystem->GeoDraw(handle);
        }
        break;
#endif
*/
      }
    }
  }
}

/****************************************************************************/

void GenMesh::Prepare()
{
  sInt pasi;
  sInt mati;
  GenMeshMtrl *mat;

  sREGZONE(MeshCount);
  sREGZONE(MeshFinal);

  if(!Prepared)
  {
    for(mati=1;mati<Mtrl.Count;mati++)
    {
      mat = &Mtrl[mati];
      if(mat->Material==0)
      {
        mat->Material = new GenMaterial;
        mat->Material->Default();
      }
      for(pasi=0;pasi<GENMAT_MAXPASS;pasi++)
      {
        if(mat->Material->Passes[pasi])
        {
          mat->Pass[pasi].MatPass = mat->Material->Passes[pasi];
          DoPass1(&mat->Pass[pasi],mati);
        }
      }
    }
    Prepared = 1;
  }
}

/****************************************************************************/

void GenMesh::Paint()
{
  sInt pasi;
  sInt mati;
  GenMeshMtrl *mat;

#if sINTRO
  sVERIFY(Prepared);
#else
  if(!Prepared)
    Prepare();
#endif
  if(StoreMode)
    RecReplay();

  for(mati=1;mati<Mtrl.Count;mati++)
  {
    mat = &Mtrl[mati];
    sVERIFY(mat->Material);
    for(pasi=0;pasi<GENMAT_MAXPASS;pasi++)
    {
      if(mat->Material->Passes[pasi])
      {
        mat->Pass[pasi].MatPass = mat->Material->Passes[pasi];
        sSystem->EnableLights(mat->Material->Passes[pasi]->LightMask);
        DoPass2(&mat->Pass[pasi],mati);
      }
    }
  }
}


/****************************************************************************/
/****************************************************************************/

#if !sINTRO
void GenMesh::PaintWire(sU32 mask)
{
  sF32 *fp;
  sU16 *ip;
  sInt i,j,ec,vc,ic,fc;
  sInt vcmax,icmax;
  sInt v,edge;
  sInt handle;
  sU32 states[256],*p;
  sU32 col;
  static sInt indices[256];
  sMatrix mat,matt;
  sVector pos,posi,vx,vy;

  ic = 0;
  ec = 0;

  if(Vert.Count==0 || Face.Count==0 || Edge.Count==0)
    return;

  if(StoreMode)
    RecReplay();

  vcmax = 0x1000;
  icmax = 0x6000;

  for(i=0;i<Edge.Count;i++)
  {
    if(Face[Edge[i].Face[0]].Material || Face[Edge[i].Face[1]].Material)
      ec++;
  }

  {

// faces

    p = states;
    sSystem->StateBase(p,sMBF_BLENDADD,sMBM_FLAT,0);
    sSystem->StateEnd(p,states,sizeof(states));
    sSystem->SetStates(states);

    handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_TRI);
    sSystem->GeoBegin(handle,vcmax,icmax,&fp,&ip);
    ic = 0;
    vc = 0;
    for(i=0;i<Vert.Count;i++)
      Vert[i].Temp = -1;
    for(i=0;i<Face.Count;i++)
    {
      if(Face[i].Material && Face[i].Mask&(mask>>8))
      {
        if(ic>icmax-0x200 || vc>vcmax-0x200)
        {
          sSystem->GeoEnd(handle,vc,ic);
          sSystem->GeoDraw(handle);
          sSystem->GeoBegin(handle,vcmax,icmax,&fp,&ip);
          for(j=0;j<Vert.Count;j++)
            Vert[j].Temp = -1;
          vc = 0;
          ic = 0;
        }
        fc=0;
        edge = Face[i].Edge;
        do
        {
          if(GetVert(edge)->Temp==-1)
          {
            GetVert(edge)->Temp = vc++;
            j = GetVertId(edge);
            *fp++ = VertPos(j).x;
            *fp++ = VertPos(j).y;
            *fp++ = VertPos(j).z;
            *(sU32 *)fp = 0xff804040; fp++;
          }
          indices[fc++] = GetVert(edge)->Temp;
          edge = NextFaceEdge(edge);
        }
        while(edge!=Face[i].Edge);
        sVERIFY(fc>2 && fc<256);
        ic+=(fc-2)*3;
        for(j=2;j<fc;j++)
        {
          *ip++ = indices[0];
          *ip++ = indices[j-1];
          *ip++ = indices[j];
        }
      }
    }
    sSystem->GeoEnd(handle,vc,ic);
    sSystem->GeoDraw(handle);
    sSystem->GeoRem(handle);

// edges

    if(ec>0)
    {
      p = states;
      sSystem->StateBase(p,0,sMBM_FLAT,0);
      sSystem->StateEnd(p,states,sizeof(states));
      sSystem->SetStates(states);

      handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE);
      sSystem->GeoBegin(handle,vcmax,0,&fp,0);
      vc = 0;
      for(i=0;i<Edge.Count;i++)
      {
        if(vc==vcmax)
        {
          sSystem->GeoEnd(handle,vc,0);
          sSystem->GeoDraw(handle);
          sSystem->GeoBegin(handle,vcmax,0,&fp,0);
          vc=0;
        }
        if(Face[Edge[i].Face[0]].Material || Face[Edge[i].Face[1]].Material)
        {
          col = (Edge[i].Mask & mask)?0xffffffff:0xff808080;
          if(Edge[i].Crease) // crease?
            col = (col & 0xff00ff00) | ((col & 0x00fe00fe) >> 1);

          v = Edge[i].Vert[0];
          *fp++ = VertPos(v).x;
          *fp++ = VertPos(v).y;
          *fp++ = VertPos(v).z;
          *(sU32 *)fp = col; fp++;

          v = Edge[i].Vert[1];
          *fp++ = VertPos(v).x;
          *fp++ = VertPos(v).y;
          *fp++ = VertPos(v).z;
          *(sU32 *)fp = col; fp++;
          vc+=2;
        }
      }
      sSystem->GeoEnd(handle,vc,0);
      sSystem->GeoDraw(handle);
      sSystem->GeoRem(handle);
    }

    vc = 0;
    for(i=0;i<Vert.Count;i++)
      if(Vert[i].Mask & (mask>>16))
        vc++;

    if(vc>0)
    {
      p = states;
      sSystem->StateBase(p,0,sMBM_FLAT,0);
      sSystem->StateEnd(p,states,sizeof(states));
      sSystem->SetStates(states);

      sSystem->GetTransform(sGT_MODELVIEW,mat);
      matt = mat;
      matt.Trans3();

      handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_QUAD);
      sSystem->GeoBegin(handle,vcmax,0,&fp,0);
      vc = 0;

      col = 0xff808080;
      for(i=0;i<Vert.Count;i++)
      {
        if(Vert[i].Mask & (mask>>16))
        {
          if(vc==vcmax)
          {
            sSystem->GeoEnd(handle,vc,0);
            sSystem->GeoDraw(handle);
            sSystem->GeoBegin(handle,vcmax,0,&fp,0);
            vc=0;
          }

          pos = VertPos(i);
          posi.Rotate34(mat,pos);
          vx = matt.i; vx.Scale3(posi.z*0.01f);
          vy = matt.j; vy.Scale3(posi.z*0.01f);

          *fp++ = pos.x-vx.x+vy.x;
          *fp++ = pos.y-vx.y+vy.y;
          *fp++ = pos.z-vx.z+vy.z;
          *(sU32 *)fp = col; fp++;
          *fp++ = pos.x+vx.x+vy.x;
          *fp++ = pos.y+vx.y+vy.y;
          *fp++ = pos.z+vx.z+vy.z;
          *(sU32 *)fp = col; fp++;
          *fp++ = pos.x+vx.x-vy.x;
          *fp++ = pos.y+vx.y-vy.y;
          *fp++ = pos.z+vx.z-vy.z;
          *(sU32 *)fp = col; fp++;
          *fp++ = pos.x-vx.x-vy.x;
          *fp++ = pos.y-vx.y-vy.y;
          *fp++ = pos.z-vx.z-vy.z;
          *(sU32 *)fp = col; fp++;

          vc+=4;
        }
      }

      sSystem->GeoEnd(handle,vc,0);
      sSystem->GeoDraw(handle);
      sSystem->GeoRem(handle);
    }
  }
}
#endif

#if !sINTRO
void GenMesh::PaintSolid()
{
  sVertex3d *v3;
  sU16 *ip;
  sInt i,j,e,e0,mtrl;
  GenMeshVert *v;
  sInt ic,vc,vi;
  sInt i0,i1,i2;
  sInt vvs,vuv,vnm;
  sInt fi,fs;
  sInt handle;

  ic = 0;

  if(StoreMode)
    RecReplay();

//  sSetRndSeed(0);
  for(mtrl=1;mtrl<Mtrl.Count;mtrl++)
  {
    fi=0;
    while(fi<Face.Count)
    {
      ic = 0;
      vc = 0;
      for(i=0;i<Vert.Count;i++)
        Vert[i].Select = 0;

      fs = fi;
      while(fi<Face.Count && vc<0x7f00 && ic<0x17000)
      {
        if(Face[fi].Material == mtrl /*&& Face[i].Select*/)
        {
          e0 = Face[fi].Edge;
          e = e0;
          j = 0;
          do
          {
            sVERIFY(GetFaceId(e)==GetFaceId(e0));
            v = GetVert(e);
            if(!v->Select)
            {
              v->Select = 1;
              v->Temp2 = vc;
              vi = v-Vert.Array;
              Vert[vc++].Temp = vi;
            }
            j++;

            e = NextFaceEdge(e);
          }
          while(e!=e0);
          ic += (j-2)*3;
        }
        fi++;
      }

      if(vc>=0 && ic>=0)
      {
        if(Mtrl[mtrl].Material)
          Mtrl[mtrl].Material->Set();

        handle = sSystem->GeoAdd(sFVF_DEFAULT3D,sGEO_TRI);
        sSystem->GeoBegin(handle,vc,ic,(sF32 **)&v3,&ip);

        vvs = VertSize;
        vuv = VertMap[sGMI_UV0]; if(vuv==-1) vuv=0;
				vnm = VertMap[sGMI_NORMAL]; if(vnm==-1) vnm=0;
        for(i=0;i<vc;i++)
        {
          j = Vert[i].Temp;
          v3->x  = VertBuf[j*vvs].x;// +sFGetRnd(0.1f);
          v3->y  = VertBuf[j*vvs].y;// +sFGetRnd(0.1f);
          v3->z  = VertBuf[j*vvs].z;// +sFGetRnd(0.1f);
					v3->nx = VertBuf[j*vvs+vnm].x;
					v3->ny = VertBuf[j*vvs+vnm].y;
					v3->nz = VertBuf[j*vvs+vnm].z;
          v3->u  = VertBuf[j*vvs+vuv].x;
          v3->v  = VertBuf[j*vvs+vuv].y;
          v3++;
        }

        for(i=fs;i<fi;i++)
        {
          if(Face[i].Material == mtrl /*&& Face[i].Select*/)
          {
            e = Face[i].Edge;
            i2 = GetVert(e)->Temp2; e = NextFaceEdge(e);
            e0 = e;
            i1 = GetVert(e)->Temp2; e = NextFaceEdge(e);
            i0 = GetVert(e)->Temp2; e = NextFaceEdge(e);
            do
            {
              *ip++ = i2;
              *ip++ = i1;
              *ip++ = i0;
              i1 = i0;
              i0 = GetVert(e)->Temp2;
              e = NextFaceEdge(e);
            }
            while(e!=e0);
          }
        }

        sSystem->GeoEnd(handle);
        sSystem->GeoDraw(handle);
        sSystem->GeoRem(handle);
      }
    }
  }
}
#endif
