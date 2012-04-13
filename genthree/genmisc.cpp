// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "genmisc.hpp"
#include "cslrt.hpp"
#include "genplayer.hpp"
#include "gencode.hpp"
#include "genbitmap.hpp"
#include "genfx.hpp"
#include "genmesh.hpp"
#include "_viruz2.hpp"
#include "_mp3.hpp"

/****************************************************************************/
/****************************************************************************/

#if !sINTRO
sObject *FindLabel_(sObject *obj,sInt label);
sObject *FindLabel(sObject *obj,sInt label)
{
  sObject *res;
  res = FindLabel_(obj,label);
  if(!res)
    sDPrintF("label %08x not found\n",label);
  SCRIPTVERIFY(res);
  return res;
}

sObject *FindLabel_(sObject *obj,sInt label)
#define FindLabel FindLabel_
#else
sObject *FindLabel(sObject *obj,sInt label)
#endif
{
  GenScene *scene;
  GenMesh *mesh;
  GenFXChain *fx;
  GenMaterial *mat;
  GenMatPass *pass;
  sInt i,max;
  sObject *res;

  res = 0;

  if(obj==0)
    return 0;
  if(obj->_Label==label)
    return obj;

  switch(obj->GetClass())
  {
  case sCID_GENSCENE:
    scene = (GenScene *)obj;
    if(res = FindLabel(scene->Mesh,label))
      return res;
    if(res = FindLabel(scene->Light,label))
      return res;
    max = scene->Childs.Count;
    for(i=0;i<max;i++)
      if(res = FindLabel(scene->Childs[i],label))
        return res;
    break;

  case sCID_GENMESH:
    mesh = (GenMesh *)obj;
#if sINTRO_X
    if(res = FindLabel(mesh->Mtrl[1].Material,label))
      return res;
#else
    max = mesh->Mtrl.Count;
    for(i=0;i<max;i++)
      if(res = FindLabel(mesh->Mtrl[i].Material,label))
        return res;
#endif
    for(i=0;i<mesh->AnimLabelCount;i++)
      if(res = FindLabel(mesh->AnimLabels[i],label))
        return res;
    break;

  case sCID_GENFXCHAIN:
    fx = (GenFXChain *)obj;
    if(res = FindLabel(fx->Object,label))
      return res;
    for(i=0;i<FXCHAIN_MAXIN;i++)
      if(res = FindLabel(fx->Input[i],label))
        return res;
    break;

  case sCID_GENMATERIAL:
    mat = (GenMaterial *)obj;
    for(i=0;i<GENMAT_MAXPASS;i++)
      if(res = FindLabel(mat->Passes[i],label))
        return res;
    break;

  case sCID_GENMATPASS:
    pass = (GenMatPass *)obj;
#if !sINTRO_X
    for(i=0;i<GENMAT_MAXTEX;i++)
      if(res = FindLabel(pass->Texture[i],label))
        return res;
#endif
    for(i=0;i<GENMAT_MAXTEX;i++)
      if(res = FindLabel(pass->TexTrans[i],label))
        return res;
    break;
  }

  return 0;
}
#undef FindLabel 

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   System Functions                                                   ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if !sINTRO
sInt __stdcall Sys_PrintDez(sInt i)
{
  sDPrintF("%d",i>>16);
  return 0;
}

sInt __stdcall Sys_PrintHex(sInt i)
{
  sDPrintF("%08x",i);
  return 0;
}

sInt __stdcall Sys_Print(sChar *str)
{
  sDPrintF("%s",str);
  return sTRUE;
}
#endif

/****************************************************************************/

sObject *__stdcall Sys_Copy(sObject *obj)
{
  sObject *no;
#if !sINTRO
  if(!obj)
    return 0;
#endif
  no = obj;

  switch(obj->GetClass())
  {
  case sCID_GENBITMAP:
    no = new GenBitmap;
    ((GenBitmap *)no)->Copy(obj);
    break;

  case sCID_GENMESH:
    no = new GenMesh;
    ((GenMesh *)no)->Copy(obj);
    break;
  }

  return no;
}

/****************************************************************************/
/****************************************************************************/

#if sINTRO
extern "C" sU8 V2MTune[],V2MEnd[];
#endif

#if !sINTRO
void __stdcall Sys_LoadSound(GenPlayer *player,sChar *name,sInt mode)
{
  sMusicPlayer *mp;

  SCRIPTVERIFYVOID(name);

  if(player->MP && sCmpString(player->MPName,name)==0)
    return;

  switch(mode>>16)
  {
#if sLINK_VIRUZ2
  case 0:
    mp = new sViruz2;
    break;
#endif
#if sLINK_MP3
  case 1:
    mp = new sMP3Decoder;
    break;
#endif
  default:
    return;
  }

  if(mp && mp->Load(name))
  {
    mp->AllocRewind(1024*1024*64);
    player->MP = mp;
    sCopyString(player->MPName,name,sizeof(player->MPName));
  }
}
#endif

#if !sINTRO
void __stdcall Sys_PlaySound(GenPlayer *player,sInt song)
{
  if(player->MP)
    player->MP->Start(song>>16);
  return;
}
#endif


sObject * __stdcall Sys_FindLabel(sObject *obj,sInt label)
{
  if(obj)
    obj = FindLabel(obj,label);
  SCRIPTVERIFY(obj);

  return obj;
}

sInt __stdcall Sys_Pow(sInt a,sInt b)
{
	return sFPow(a/65536.0f,b/65536.0f)*65536.0f;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Camera Functions                                                   ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

/*
static sCamera sGlobal_Camera;
static sMatrix sGlobal_Matrix;

void __stdcall Global_CamFrustrum(sInt near_,sInt far_)
{
  sGlobal_Camera.FarClip = near_/65536.0f;
  sGlobal_Camera.NearClip = far_/65536.0f;
}

void __stdcall Global_CamPos(sInt3 t,sInt3 r)
{
  sGlobal_Camera.CamPos.InitEuler(r.x/65536.0f,r.y/65536.0f,r.z/65536.0f);
  sGlobal_Camera.CamPos.l.Init(t);
}

void __stdcall Global_CamSet(void)
{
  sSystem->SetCamera(sGlobal_Camera);
}

void __stdcall Global_ObjPos(sInt3 s,sInt3 r,sInt3 t)
{
  sGlobal_Matrix.InitEuler(r.x/65536.0f,r.y/65536.0f,r.z/65536.0f);
  sGlobal_Matrix.i.Scale4(s.x);
  sGlobal_Matrix.j.Scale4(s.y);
  sGlobal_Matrix.k.Scale4(s.z);
  sGlobal_Matrix.l.Init(t);
}

void __stdcall Global_ObjSet(void)
{
  sSystem->SetMatrix(sGlobal_Matrix);
}
*/
/****************************************************************************/
/*
void __stdcall Global_ObjMat(GenMatrix *m)
{
  SCRIPTVERIFYVOID(m);
  m->Calc(sGlobal_Matrix);
  sSystem->SetMatrix(sGlobal_Matrix);
}

void __stdcall Global_CamMat(GenMatrix *m)
{
  SCRIPTVERIFYVOID(m);
  m->Calc(sGlobal_Matrix);
  sSystem->SetMatrix(sGlobal_Matrix);
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   Matrix Functions                                                   ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO
GenMatrix::GenMatrix()
{
  sSetMem(&s,0,sOFFSET(GenMatrix,Parent)+4-sOFFSET(GenMatrix,s));
  s.Init(1.0f,1.0f,1.0f);
}

void GenMatrix::Copy(sObject *o)
{
  GenMatrix *om;
  sVERIFY(o->GetClass()==sCID_GENMATRIX);
  om = (GenMatrix *) o;
  sCopyMem(&s,&om->s,sOFFSET(GenMatrix,Parent)+4-sOFFSET(GenMatrix,s));
  _Label = om->_Label;
}
#endif

#if !sINTRO
void GenMatrix::Calc(sMatrix &mat)
{
  sMatrix a,b;
  sVector v;
  sF32 dist;

  if(Parent)
    Parent->Calc(a);
  else
    a.Init();
  
  switch(Mode)
  {
  case MM_INIT:
    mat.InitSRT(&s.x);
    break;
  case MM_ORBIT:
    dist = sFSqrt(t.x*t.x+t.y*t.y+t.z*t.z);
    mat.InitSRT(&s.x);
    mat.l.Scale3(mat.k,-dist);
    break;
  case MM_LOCKAT:
    v.Init(a.l.x-t.x,a.l.y-t.y,a.l.z-t.z,1);
    mat.InitDir(v);
    mat.i.Scale3(s.x);
    mat.j.Scale3(s.y);
    mat.k.Scale3(s.z);
    break;
  case MM_MUL:
    b.InitSRT(&s.x);
    mat.Mul4(a,b);
    break;
  case MM_TRANSPOSE:
    mat = a;
    mat.Trans4();
    break;
  }
}
#endif

/****************************************************************************/

#if !sINTRO
GenMatrix * __stdcall Matrix_Generate(sF323 s,sF323 r,sF323 t,sInt mode)
{
  return Matrix_Modify(0,s,r,t,mode);
}

GenMatrix * __stdcall Matrix_Modify(GenMatrix *in,sF323 s,sF323 r,sF323 t,sInt mode)
{
  GenMatrix *mat;

  mat = new GenMatrix;
  mat->Parent = in;
  sCopyMem(&mat->s,&s,sizeof(s)+sizeof(r)+sizeof(t)+sizeof(mode));

  return mat;
}

GenMatrix * __stdcall Matrix_Label(GenMatrix *mat,sInt label)
{
  mat->_Label = label;
  return mat;
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Spline Functions                                                   ***/
/***                                                                      ***/
/****************************************************************************/

GenSpline::GenSpline()
{
  Keys.Init(16);
  Tension = 0;
  Continuity = 0;
  Bias = 0;
}

GenSpline::~GenSpline()
{
  Keys.Exit();
}

#if !sINTRO

void GenSpline::Clear()
{
  Keys.Count = 0;
}

void GenSpline::Copy(sObject *o)
{
  GenSpline *so;
  sVERIFY(o->GetClass()==sCID_GENSPLINE);

  so = (GenSpline *)o;
  Dimensions = so->Dimensions;
  Keys.Copy(so->Keys);
  _Label = so->_Label;
}
#endif

void GenSpline::Sort()
{
  sInt i,j;

  for(i=0;i<Keys.Count-1;i++)
    for(j=i+1;j<Keys.Count;j++)
      if(Keys[i].Time > Keys[j].Time)
        sSwap(Keys[i],Keys[j]);
}

void GenSpline::Calc(sF32 time,sF32 *val)
{
  sInt i;
  sF32 fade;
  sF32 f1,f2,f3,f4;
  sF32 t0,t1,t2,t3;
  sF32 a1,b1,a2,b2;
  sF32 *p0,*p1,*p2,*p3;
  sInt j;

  if(Keys.Count<2)
  {
    if(Keys.Count==0)
    {
      for(i=0;i<Dimensions;i++)
        val[i] = 0;
    }
    else
    {
      for(i=0;i<Dimensions;i++)
        val[i] = Keys[0].Value[i];
    }
  }
  else
  {
    time = sRange<sF32>(time,Keys[Keys.Count-1].Time,Keys[0].Time);

    for(i=1;i<Keys.Count;i++)
    {
      t1 = Keys[i-1].Time;
      t2 = Keys[i+0].Time;
      if(t1>=t2)
        continue;
      if(time <= t2)
      {
        a1 = (1-Tension)*(1-Continuity)*(1+Bias)/2;
        b1 = (1-Tension)*(1+Continuity)*(1-Bias)/2;
        a2 = (1-Tension)*(1+Continuity)*(1+Bias)/2;
        b2 = (1-Tension)*(1-Continuity)*(1-Bias)/2;

        p1 = Keys[i-1].Value;
        p2 = Keys[i+0].Value;

        if(i>=2)
        {
          p0 = Keys[i-2].Value;
          t0 = p0[-1];
        }
        else
        {
          p0 = p2;
          a1 = -a1;
          t0 = t1+(t1-t2);
        }

        if(i+1<Keys.Count)
        {
          p3 = Keys[i+1].Value;
          t3 = p3[-1];
        }
        else
        {
          p3 = p1;
          b2 = -b2;
          t3 = t2+(t2-t1);
        }

        fade = ((time-t1)/(t2-t1));
        t0 = t1-t0;
        t1 = t2-t1;
        t2 = t3-t2;
        f1 =  2*fade*fade*fade - 3*fade*fade + 1;
        f2 = -2*fade*fade*fade + 3*fade*fade;
        f3 =    fade*fade*fade - 2*fade*fade + fade;
        f4 =    fade*fade*fade -   fade*fade;
        
        f3 = f3 * 2*t1/(t1+t0);
        f4 = f4 * 2*t1/(t1+t2);

        for(j=0;j<Dimensions;j++)
        {
          val[j] = f1 * p1[j]
                 + f2 * p2[j]
                 + f3 * (a1*(p1[j]-p0[j]) + b1*(p2[j]-p1[j]))
                 + f4 * (a2*(p2[j]-p1[j]) + b2*(p3[j]-p2[j]));
        }

        return;
      }
    }
  }
}

/****************************************************************************/

GenSpline * __stdcall Spline_New(sInt dimensions,sInt t,sInt c)
{
  GenSpline *spline;

  spline = new GenSpline;
  spline->Dimensions = dimensions>>16;
  spline->Tension = t/65536.0f;
  spline->Continuity = c/65536.0f;

  return spline;
}

GenSpline * __stdcall Spline_Sort(GenSpline *spline)
{
  SCRIPTVERIFY(spline);
  spline->Sort();
  return spline;
}

GenSpline * __stdcall Spline_AddKey(GenSpline *spline,sInt time,sInt *val,sInt count)
{
  sInt i;
  GenSplineKey *key;

  SCRIPTVERIFY(spline);

  key = spline->Keys.Add();
  key->Time = time/65536.0f;
  for(i=0;i<count;i++)
    key->Value[i] = val[i]/65536.0f;

  return spline;
}

void Spline_Calc(GenSpline *spline,sInt time)
{
  sF32 valf[4];
  sInt i;

  spline->Calc(time/65536.0f,valf);
  for(i=0;i<spline->Dimensions;i++)
    ScriptRuntimeInterpreter->PushI(sFtol(valf[i]*0x10000));
}

/****************************************************************************/

GenSpline * __stdcall Spline_AddKey1(GenSpline *spline,sInt time,sInt val)
{
  return Spline_AddKey(spline,time,&val,1);
}

GenSpline * __stdcall Spline_AddKey2(GenSpline *spline,sInt time,sInt2 val)
{
  return Spline_AddKey(spline,time,&val.x,2);
}

GenSpline * __stdcall Spline_AddKey3(GenSpline *spline,sInt time,sInt3 val)
{
  return Spline_AddKey(spline,time,&val.x,3);
}

GenSpline * __stdcall Spline_AddKey4(GenSpline *spline,sInt time,sInt4 val)
{
  return Spline_AddKey(spline,time,&val.x,4);
}


void __stdcall Spline_Calc1(GenSpline *spline,sInt time)
{
  sVERIFY(spline->Dimensions==1);
  Spline_Calc(spline,time);
}

void __stdcall Spline_Calc2(GenSpline *spline,sInt time)
{
  sVERIFY(spline->Dimensions==2);
  Spline_Calc(spline,time);
}

void __stdcall Spline_Calc3(GenSpline *spline,sInt time)
{
  sVERIFY(spline->Dimensions==3);
  Spline_Calc(spline,time);
}

void __stdcall Spline_Calc4(GenSpline *spline,sInt time)
{
  sVERIFY(spline->Dimensions==4);
  Spline_Calc(spline,time);
}

/****************************************************************************/

