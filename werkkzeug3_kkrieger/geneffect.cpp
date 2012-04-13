// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "geneffect.hpp"
#include "genmaterial.hpp"
#include "genbitmap.hpp"
#include "genmesh.hpp"
#include "genminmesh.hpp"
#include "kkriegergame.hpp"
#include "genoverlay.hpp"
#include "engine.hpp"
#include "material11.hpp"
#include "_util.hpp"
#include "_startdx.hpp"
#include "fried/fried.hpp"
#include <xmmintrin.h>

extern sF32 GlobalFps;
extern sBool IntroHighTexRes;

/****************************************************************************/
/***                                                                      ***/
/***   Dummy effect, use this to start new effects                        ***/
/***                                                                      ***/
/****************************************************************************/

GenEffect::GenEffect()
{
  ClassId = KC_EFFECT;
  Material = 0;
  Pass = 0;
  Usage = ENGU_OTHER;
  NeedCurrentRender = sFALSE;
  EffectData = 0;
  Op = 0;
}

GenEffect::~GenEffect()
{
  if(Material)
    Material->Release();

  if(EffectData)
    ::operator delete(EffectData);
}

void GenEffect::Copy(KObject *o)
{
  GenEffect *oe;

  sVERIFY(o->ClassId==KC_EFFECT);
  oe = (GenEffect *) o;

  Pass = oe->Pass;
  Usage = oe->Usage;
  NeedCurrentRender = oe->NeedCurrentRender;
  Op = oe->Op;

  Material = oe->Material;
  if(Material)
    Material->AddRef();
}

KObject *GenEffect::Copy()
{
  GenEffect *e;
  e = new GenEffect;
  e->Copy(this);
  return e;
}

/****************************************************************************/

GenEffect *MakeEffect(class GenMaterial *mtrl)
{
  GenEffect *gen;

  gen = new GenEffect;
  if(mtrl)
  {
    gen->Pass = mtrl->Passes[0].Pass;
    gen->Usage = mtrl->Passes[0].Usage;
    mtrl->Release();
  }

  return gen;
}

/****************************************************************************/

void SetVert(sF32 *fp,sF32 x,sF32 y,sF32 z,sF32 u,sF32 v,sU64 col)
{
  fp[0] = x;
  fp[1] = y;
  fp[2] = z;
  fp[3] = 0;
  fp[4] = 0;
  *((sU32 *)(&fp[5])) =
      (((col>>53)&0xff)<<24) | (((col>>39)&0xff)<< 0)
    | (((col>>23)&0xff)<< 8) | (((col>> 7)&0xff)<<16);
  /**((sU64 *)(&fp[3])) = col;
  *((sU32 *)(&fp[5])) = 0;*/
  fp[6] = u;
  fp[7] = v;
}

/****************************************************************************/

KObject * __stdcall Init_Effect_Dummy(GenMaterial *mtrl,sF323 size)
{
  return MakeEffect(mtrl);
}

void __stdcall Exec_Effect_Dummy(KOp *op,KEnvironment *kenv,sF323 size)
{
  GenMaterial *mtrl;
  sInt geo;
  sF32 *fp;
  sU16 *ip;

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(mtrl && mtrl->Passes.Count>0)
  {
    mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
    geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRI);
    sSystem->GeoBegin(geo,8,36,&fp,(void **)&ip);
    for(sInt i=0;i<8;i++)
    {
      fp[0] = (((i+0)&2)?-0.5:0.5)*size.x;
      fp[1] = (((i+1)&2)?-0.5:0.5)*size.y;
      fp[2] = (((i+0)&4)?-0.5:0.5)*size.z;
      fp[3] = 0;
      fp[4] = 0;
      *((sU32 *)(&fp[5])) = 0xffffffff;
      fp[6] = ((i+0)&2)?0:1;
      fp[7] = ((i+1)&2)?0:1;
      fp += 8;
    }
    sQuad(ip,3,2,1,0);
    sQuad(ip,4,5,6,7);
    sQuad(ip,0,1,5,4);
    sQuad(ip,1,2,6,5);
    sQuad(ip,2,3,7,6);
    sQuad(ip,3,0,4,7);
    sSystem->GeoEnd(geo);
    sSystem->GeoDraw(geo);
    sSystem->GeoRem(geo);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Print text and game-variables. Even multiline                      ***/
/***                                                                      ***/
/****************************************************************************/


// PRINTF functions
// 
// %lxn    print value
// l is number of digits, '0'-'9', requred
// x is what to print. see below
// n is optional index, may be multiple digits.
// ; is optional end, if you want to continue with digits after the format.
// what (x):
// wn : weapon ammo #n (ammo is 0..3, not 0..7) #4=current weapon
// l  : life
// a  : armour
// W  : weapon ammo of current 
// sn : switch #n
//
// %|              align this char to center
//
// %in=v ..|..;    conditional print
// i    is 'i'
// n    is switch number
// =    is '=' or '!='
// v    is value
// ' '  is an optional space
// ..   is some text, may be including \|
// |    seperator. else-part is optional.
// ;    is the end (required).
// example: "%i4=0---- ;continue %s4=0 ----;
// example: "%i3=0no|yes;
// example: "%i3=0no|this text is not the else part;
//
//
// %sn ..|..|..|..;     switch print
// s    is 's' or 'l', for s-mode or l-mode
// n    is switch number
// ' '  is an optional space in s-mode and a required linefeed in l-mode
// ..   is some text, may be including \|
// |    seperator '|' for s-mode and '\n' for l-mode.
// ;    is the end (required).
// example: "%s5low|medium|high;


// OLD PRINTF functions
// 
// %lxn; 
// l is number of digits, '0'-'9', requred
//   if it is a non-digit, it will be the base-digit. that is if you
//   say '%ds001', it will print the letter 'd' or 'e' depending on switch 1
//   this is useful with the "font0" feature, where you have the ascii code
//   0x40-0x7f cleanly arranged in a 8x8 matrix. 
// x is what to print. see belo
// n is optional index, may be multiple digits.
// ; is optional end, if you want to continue with digits after the format.
// 
// what (x):
// wn : weapon ammo #n (ammo is 0..3, not 0..7) #4=current weapon
// l  : life
// a  : armour
// W  : weapon ammo of current 
// sn : switch #n


/****************************************************************************/

KObject * __stdcall Init_Effect_Print(class GenMaterial *mtrl,sInt flags,sF32 sizex,sF32 sizey,sF32 spacex,sF32 spacey,sF32 radius,sChar *text)
{
  /*if(mtrl) mtrl->Release();
  return new GenEffect;*/
  return MakeEffect(mtrl);
}

struct EffectPrintInfo
{
  KEnvironment *KEnv;
  sF32 XSize;
  sF32 YSize;
  sF32 SpaceX;
  sF32 SpaceY;
  sF32 *fp;
  sU16 *ip;
  sInt Count;
  sInt Max;
  sInt Page;
  void Print(sF32 x,sF32 y,sInt chr,sF32 r);
  sF32 Width(sInt chr);
  sF32 Width(sChar *text,sInt count=-1);
};

void EffectPrintInfo::Print(sF32 x,sF32 y,sInt chr,sF32 r)
{
  sInt i;
  sF32 w;
  KLetterMetric lm;
  sF32 y0,y1,z0,z1,f;

  if(Count<Max)
  {
    lm = KEnv->Letters[Page][chr&0xff];
    w = lm.UV.YSize();

    if(r==0)
    {
      y0 = y;
      y1 = y-YSize;
      z0 = 0;
      z1 = 0;
    }
    else
    {
      f = 1/r;
      y0 = sFSin(f*(y))*r;
      z0 = -sFCos(f*(y))*r;
      y1 = sFSin(f*(y-YSize))*r;
      z1 = -sFCos(f*(y-YSize))*r;
    }

    if(w>0)
    {
      x += lm.PreSpace*XSize/w;
      w = lm.UV.XSize()/w;

      SetVert(fp+0*8,x        ,y0,z0,lm.UV.x0,lm.UV.y0);
      SetVert(fp+1*8,x+w*XSize,y0,z0,lm.UV.x1,lm.UV.y0);
      SetVert(fp+2*8,x        ,y1,z1,lm.UV.x0,lm.UV.y1);
      SetVert(fp+3*8,x+w*XSize,y1,z1,lm.UV.x1,lm.UV.y1);
      fp+=4*8;

      i = Count*4;
      sQuad(ip,i+0,i+1,i+3,i+2);
      Count++;
    }
  }
}

sF32 EffectPrintInfo::Width(sInt chr)
{
  sF32 w;
  KLetterMetric lm;

  lm = KEnv->Letters[Page][chr&0xff];
  w = lm.UV.YSize();
  if(w>0)
    return lm.Width/w*XSize;
    //return lm.UV.XSize()/w*XSize;
  else
    return 0;
}

sF32 EffectPrintInfo::Width(sChar *text,sInt maxcount)
{
  sF32 w;
  w = 0;
  while(*text && maxcount!=0)
  {
    w += Width(*text++) + SpaceX;
    maxcount--;
  }
  return w-SpaceX;
}

/****************************************************************************/

void __stdcall Exec_Effect_Print(KOp *op,KEnvironment *kenv,sInt flags,sF32 sizex,sF32 sizey,sF32 spacex,sF32 spacey,sF32 radius,sChar *text)
{
  GenMaterial *mat;
  sInt geo;
  sF32 x,y;
  EffectPrintInfo pi;
  sChar buffer[512];
  sChar *p;
  sInt i;
  sMaterialEnv env;
  sInt centerchar;
  sBool inf;

  mat = (GenMaterial *) op->GetLinkCache(0);
  if(!mat)
    return;

  if(mat->Passes.Count>0)
  {
    pi.KEnv = kenv;
    pi.XSize = sizex;
    pi.YSize = sizey;
    pi.SpaceX = spacex;
    pi.SpaceY = spacey;
    pi.Count = 0;
    pi.Max = 0x1000;
    pi.Page = (flags>>4)&7;
    y = 0;
    if(flags&2)
      y += sizey/2;
    
    if(flags&4)
    {
      env.Init();
      env.Orthogonal = sMEO_NORMALISED;
      env.ModelSpace.l.x = -1.0f;
      env.ModelSpace.l.y = 1.0f;
      sSystem->SetViewProject(&env);
      mat->Passes[0].Mtrl->Set(env);
    }
    else
    {
      env = kenv->CurrentCam;
      env.ModelSpace = kenv->ExecStack.Top();
      sSystem->SetViewProject(&env);
      mat->Passes[0].Mtrl->Set(env);
    }
    geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRI);
    sSystem->GeoBegin(geo,pi.Max*4,pi.Max*6,&pi.fp,(void **)&pi.ip);

    while(*text)
    {
      x = 0;
      centerchar = -1;

      p = buffer;
      while(*text!=0 && *text!='\n')
      {
        if(text[0]=='%' && text[1]>='0' && text[1]<='9' && text[2]!=0 && text[3]!=0)
        {
          sInt len;
          sInt base;
          sInt val;
          sInt what;
          sInt index;
          
          text++;
          len = 1;
          val = 0;
          index = 0;

          len = (*text++)-'0';
          base = '0';
          /*
          base  = *text++;
          if(base>='0' && base<='9')
          {
            len = base - '0';
            base = '0';
          }
          */
          what = *text++;
          while(*text>='0' && *text<='9')
            index = index*10+*text++-'0';
          if(*text==';')
            text++;

          inf = 0;
          switch(what)
          {
          case 'a':
            val = kenv->Game->Player.Armor;
            break;
          case 'l':
            val = kenv->Game->Player.Life;
            break;
          case 's':
            val = kenv->Game->Switches[index&255];
            break;
          case 'w':
            val = kenv->Game->Player.Ammo[index&3];
            break;
          case 'W':
            val = kenv->Game->Player.Ammo[(kenv->Game->Player.CurrentWeapon/2)&3];
            if(kenv->Game->Player.CurrentWeapon==0)
              inf = 1;
            break;
#if sPLAYER
          case 'f':
            val = GlobalFps*100;
            break;
#endif
          }

          if(inf)
          {
            *p++ = 'i';
            *p++ = 'n';
            *p++ = 'f';
          }
          else
          {
            for(i=0;i<len;i++)
            {
              p[len-i-1] = val%10+base;
              val/=10;
            }
            p+=len;
          }
        }
        else if(text[0]=='%' && (text[1]=='i' || text[1]=='s'))
        {
          sInt index;
          sInt compare;
          sInt cond;
          sInt current;
          sInt value;
          sInt mode;

          index = 0;
          cond = 0;
          compare = 0;
          current = 0;
          mode = text[1];

          text+=2;
          while(*text>='0' && *text<='9')
            index = index*10+*text++-'0';
          value = kenv->Game->Switches[index&255];

          if(mode=='i')
          {
            if(*text=='!')
            {
              cond = 1; text++;
            }
            if(*text=='=') text++;
            else cond = 1;

            while(*text>='0' && *text<='9')
              compare = compare*10+*text++-'0';
            value = (value==compare);

            if(!cond)
              value = !value;
          }


          current = 0;
          while(*text!=0 && *text!=';')
          {
            if(*text=='|')
            {
              current++;
            }
            else
            {
              if(text[0]=='\\' && text[1]!=0)
                text++;
              if(current==value)
                *p++ = *text;
              
            }
            text++;
          }
          if(*text==';') text++;
        }
        else if(text[0]=='%' && text[1]=='|')
        {
          centerchar = p-buffer;
          text+=2;
        }
        else
        {
          *p++ = *text++;
        }
      }
      *p++ =0 ;

      p = buffer;
      
      switch((flags&0x300)>>8)
      {
      case 0:
        break;
      case 1:
        if(centerchar>=0)
          x -= pi.Width(p,centerchar);
        else
          x -= pi.Width(p)/2;
        break;
      case 2:
        x -= pi.Width(p);
        break;
      }
      while(*p)
      {
        pi.Print(x,y,*p,radius);
        x += pi.Width(*p)+spacex;
        p++;
      }

      while(*text=='\n')
      {
        y-=sizey+spacey;
        text++;
      }
    }
    sSystem->GeoEnd(geo,pi.Count*4,pi.Count*6);
    sSystem->GeoDraw(geo);
    sSystem->GeoRem(geo);
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   Particles                                                          ***/
/***                                                                      ***/
/****************************************************************************/

#define MAXPARTSLOT 16

struct Particle
{
  sVector Pos;        // w = rotation
  sVector Speed;      // w = rotation
  sInt Time;
  sInt MaxTime;
};

struct ParticleMem : public KInstanceMem
{
  Particle *Array;
  sInt Max;
  sInt Used;
  sInt Burst;
};

struct ParticleMem2 : public KInstanceMem
{
  sInt BurstCount;
};

struct ParticleSystem
{
  Particle *Array;
  sInt Max;
  sInt Used;
};

static ParticleSystem ParticleSystems[MAXPARTSLOT];

void FreeParticles()
{
  sInt i;
  for(i=0;i<MAXPARTSLOT;i++)
  {
    delete[] ParticleSystems[i].Array;
    ParticleSystems[i].Array = 0;
  }
}

/****************************************************************************/

/*

KObject * __stdcall Init_Effect_Particles(class GenMaterial *mtrl,
  sInt mode,sInt count,sF32 time,sF32 rate,sF324 speed,
  sU32 col0,sU32 col1,sU32 col2,sF323 size,
  sF32 middle,sF32 fade,sF324 rpos,sF324 rspeed)
{
  GenEffect *effect;
  effect = new GenEffect;
  if(mtrl && mtrl->Passes.Count>0)
    effect->Pass = mtrl->Passes[0].Pass;
  return effect;
}

void __stdcall Exec_Effect_Particles(KOp *op,KEnvironment *kenv,sInt mode,
  sInt count,sF32 time,sF32 rate,sF324 speed,
  sU32 col0,sU32 col1,sU32 col2,sF323 size,
  sF32 fmiddle,sF32 ffade,sF324 rpos,sF324 rspeed)
{
  GenMaterial *mtrl;
  sInt geo;
  sF32 *fp;
  sInt i,j;
  ParticleMem *mem;
  Particle *part;
  sVector v,rv;
  sInt max,slices;
  sMatrix mat;
  sF32 s,ss,sc;
  sInt percent;
  sInt middle,fade,fadetime;
  sU64 c0,c1,c2;
  sU64 cr,cf;
  sU64 black;

  mem = kenv->GetInst<ParticleMem>(op);

#if !sINTRO
  if(count!=mem->Max && mem->Array)
  {
    delete[] mem->Array;
    mem->Max = count;
    mem->Burst = count;
    mem->Used = 0;
    mem->Array = new Particle[count];
    mem->DeleteArray = mem->Array;
  }
#endif

  if(mem->Reset)
  {
    mem->Max = count;
    mem->Burst = count;
    mem->Used = 0;
    mem->Array = new Particle[count];
    mem->DeleteArray = mem->Array;
  }

  i = (kenv->CurrentTime&0xffffff)*(rate/1000);
  j = ((kenv->CurrentTime-kenv->TimeDelta)&0xffffff)*(rate/1000);
  slices = kenv->TimeDelta;
  max =  i-j;
  if(max>mem->Max-mem->Used)
    max = mem->Max-mem->Used;

#if !sINTRO
  if(mode & 0x100)
  {
    max = 0;
    mem->Burst = count;
    mem->Used = 0;
  }
#endif
  if(mode & 0x010)
  {
    if(max>mem->Burst)
      max = mem->Burst;
    mem->Burst-=max;
  }

  sDPrintF("%08x: count %04x new %04x burst %04x\n",mem,mem->Used,max,mem->Burst);
  const sMatrix &matrix = kenv->ExecStack.Top();
  sVector speedVec;
  speedVec.Init(speed);

  for(i=0;i<max;i++)
  {
    part = &mem->Array[mem->Used++];
    part->Pos = matrix.l;
    v.Rotate3(matrix,speedVec);
    part->Speed.Init(v.x,v.y,v.z,speed.w);
    part->Time = 0;
    part->MaxTime = time*1000;
    do
      rv.Init(sFGetRnd()*2-1,sFGetRnd()*2-1,sFGetRnd()*2-1);
    while(rv.Dot3(rv)>1);
    if(mode&1)
      rv.Unit3();
    if(mode&8)
      part->Speed.Mul3(rv);
    part->Pos.x += rv.x * rpos.x;
    part->Pos.y += rv.y * rpos.y;
    part->Pos.z += rv.z * rpos.z;
    part->Pos.w = (sFGetRnd()*2-1) * rpos.w + sPI2F/8;
    do
      rv.Init(sFGetRnd()*2-1,sFGetRnd()*2-1,sFGetRnd()*2-1);
    while(rv.Dot3(rv)>1);
    if(mode&2)
      rv.Unit3();
    part->Speed.x += rv.x * rspeed.x;
    part->Speed.y += rv.y * rspeed.y;
    part->Speed.z += rv.z * rspeed.z;
    part->Speed.w += (sFGetRnd()*2-1) * rspeed.w;

    part->Speed.Scale4(0.001f);
  }

  col0=(col0&0xff00ff00)|((col0&0x00ff0000)>>16)|((col0&0x00ff)<<16);  c0=GetColor64(col0);
  col1=(col1&0xff00ff00)|((col1&0x00ff0000)>>16)|((col1&0x000000ff)<<16);  c1=GetColor64(col1);
  col2=(col2&0xff00ff00)|((col2&0x00ff0000)>>16)|((col2&0x000000ff)<<16);  c2=GetColor64(col2);
  
  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(mtrl && mem->Used>0 && mtrl->Passes.Count>0)
  {
    sMaterial11->MtrlSet(&mtrl->Passes[0],&kenv->CurrentCam);
    geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_QUAD);
    black = 0xffff000000000000LL;

    sSystem->GetTransform(sGT_MODELVIEW,mat);
    mat.Trans3();

    sSystem->GeoBegin(geo,mem->Used*4,0,&fp,0);
    for(i=0;i<mem->Used;)
    {
      part = mem->Array+i;

      middle = part->MaxTime*(1022*fmiddle+1)/1024;
      if(part->Time<middle)
      {
        percent = sDivShift(part->Time,middle);
        Fade64(cr,c0,c1,percent);
        s = sFade(size.x,size.y,percent/65536.0f);
      }
      else
      {
        percent = sDivShift(part->Time-middle,part->MaxTime-middle);
        Fade64(cr,c1,c2,percent);
        s = sFade(size.y,size.z,percent/65536.0f);
      }
      if(mode&4)
      {
        fade = 0xffff;
        fadetime = part->MaxTime*(1022*ffade+1)/1024;
        if(part->Time<fadetime)
          fade = sDivShift(part->Time,fadetime);
        if(part->Time>part->MaxTime-fadetime)
          fade = sDivShift(part->MaxTime-part->Time,fadetime);
        cf = cr;
        Fade64(cr,black,cf,fade);
      }



      sc = sFCos(part->Pos.w)*s;
      ss = sFSin(part->Pos.w)*s;
      
      v = part->Pos; v.AddScale3(mat.i, sc); v.AddScale3(mat.j, ss);
      SetVert(fp,v.x,v.y,v.z,0,0,cr); fp+=8;
      v = part->Pos; v.AddScale3(mat.i, ss); v.AddScale3(mat.j,-sc);
      SetVert(fp,v.x,v.y,v.z,0,1,cr); fp+=8;
      v = part->Pos; v.AddScale3(mat.i,-sc); v.AddScale3(mat.j,-ss);
      SetVert(fp,v.x,v.y,v.z,1,1,cr); fp+=8;
      v = part->Pos; v.AddScale3(mat.i,-ss); v.AddScale3(mat.j, sc);
      SetVert(fp,v.x,v.y,v.z,1,0,cr); fp+=8;

      part->Pos.AddScale4(part->Speed,slices/1000.0f);

      part->Time += slices;
      if(part->Time>part->MaxTime)
        mem->Array[i] = mem->Array[--mem->Used];
      else
        i++;
    }
    sSystem->GeoEnd(geo);
    sSystem->GeoDraw(geo);
    sSystem->GeoRem(geo);
  } 
}

*/

/****************************************************************************/
/****************************************************************************/

KObject * __stdcall Init_Effect_PartEmitter(
  sInt slot,sInt mode,sInt count,sF32 time,sF32 rate,
  sF324 speed,sF324 rpos,sF324 rspeed)
{
  return new GenScene;
}

KObject * __stdcall Init_Effect_PartSystem(class GenMaterial *mtrl,
  sInt slot,sInt count,sU32 col0,sU32 col1,sU32 col2,sF323 size,sF32 middle,sF32 fade,sF32 gravity,sInt flags)
{
  GenEffect *effect;
  ParticleSystem *psys;
  sVERIFY(slot>=0&&slot<MAXPARTSLOT);
  sVERIFY(count>0);

  psys = &ParticleSystems[slot];
  if(psys->Array)
    delete[] psys->Array;
  psys->Array = new Particle[count];
  psys->Used = 0;
  psys->Max = count;

  effect = new GenEffect;
  effect->Material = mtrl;
  if(mtrl && mtrl->Passes.Count>0)
    effect->Pass = mtrl->Passes[0].Pass;
  return effect;
}

/****************************************************************************/

void __stdcall Exec_Effect_PartEmitter(KOp *op,KEnvironment *kenv,
  sInt slot,sInt mode,sInt burst,sF32 time,sF32 rate,
  sF324 speed,sF324 rpos,sF324 rspeed)
{
  Particle *part;
  ParticleMem2 *mem2;
  sVector v,rv;
  sInt max,slices;
  ParticleSystem *psys;
  sInt i,j;

  sVERIFY(slot>=0&&slot<MAXPARTSLOT);
  psys = &ParticleSystems[slot];

  mem2 = kenv->GetInst<ParticleMem2>(op);
  if(mem2->Reset)
    mem2->BurstCount = burst;

  i = (kenv->CurrentTime&0xffffff)*(rate/1000);
  j = ((kenv->CurrentTime-kenv->TimeDelta)&0xffffff)*(rate/1000);
  slices = kenv->TimeDelta;
  max =  i-j;
  if(max>psys->Max-psys->Used)
    max = psys->Max-psys->Used;

#if !sINTRO
  if(mode & 0x100)
  {
    max = 0;
    mem2->BurstCount = burst;
  }
#endif

  if(burst)
  {
    if(max>mem2->BurstCount)
      max = mem2->BurstCount;
    mem2->BurstCount-=max;
  }

//  sDPrintF("%08x: count %04x new %04x burst %04x\n",mem,mem->Used,max,mem->Burst);

  const sMatrix &matrix = kenv->ExecStack.Top();
  sVector speedVec;
  speedVec.Init(speed);

  for(i=0;i<max;i++)
  {
    part = &psys->Array[psys->Used++];
    part->Pos = matrix.l;
    v.Rotate3(matrix,speedVec);
    part->Speed.Init(v.x,v.y,v.z,speed.w);
    part->Time = 0;
    part->MaxTime = time*1000;
    do
      rv.Init(sFGetRnd()*2-1,sFGetRnd()*2-1,sFGetRnd()*2-1);
    while(rv.Dot3(rv)>1);
    if(mode&1)
      rv.Unit3();
    if(mode&8)
      part->Speed.Mul3(rv);
    part->Pos.x += rv.x * rpos.x;
    part->Pos.y += rv.y * rpos.y;
    part->Pos.z += rv.z * rpos.z;
    part->Pos.w = (sFGetRnd()*2-1) * rpos.w + sPI2F/8;
    do
      rv.Init(sFGetRnd()*2-1,sFGetRnd()*2-1,sFGetRnd()*2-1);
    while(rv.Dot3(rv)>1);
    if(mode&2)
      rv.Unit3();
    part->Speed.x += rv.x * rspeed.x;
    part->Speed.y += rv.y * rspeed.y;
    part->Speed.z += rv.z * rspeed.z;
    part->Speed.w += (sFGetRnd()*2-1) * rspeed.w;

    part->Speed.Scale4(0.001f);
  }
}

void __stdcall Exec_Effect_PartSystem(KOp *op,KEnvironment *kenv,
  sInt slot,sInt count,sU32 col0,sU32 col1,sU32 col2,sF323 size,sF32 fmiddle,sF32 ffade,sF32 gravity,sInt flags)
{
  ParticleSystem *psys;
  GenMaterial *mtrl;
  sInt geo;
  sF32 *fp;
  sInt i;
  Particle *part;
  sVector v;
  sInt slices;
  sMatrix mat;
  sF32 s,ss,sc;
  sInt percent;
  sInt middle,fade,fadetime;
  sU64 c0,c1,c2;
  sU64 cr,cf;
  sU64 black;

  sVERIFY(slot>=0&&slot<MAXPARTSLOT);
  psys = &ParticleSystems[slot];

  slices = kenv->TimeDelta;

  col0=(col0&0xff00ff00)|((col0&0x00ff0000)>>16)|((col0&0x00ff)<<16);  c0=GetColor64(col0);
  col1=(col1&0xff00ff00)|((col1&0x00ff0000)>>16)|((col1&0x000000ff)<<16);  c1=GetColor64(col1);
  col2=(col2&0xff00ff00)|((col2&0x00ff0000)>>16)|((col2&0x000000ff)<<16);  c2=GetColor64(col2);

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(mtrl && psys->Used>0 && mtrl->Passes.Count>0)
  {
    kenv->CurrentCam.ModelSpace.Init();
//    for(sInt pass=0;pass<mtrl->Passes.Count;pass++)
    {
      mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
      geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_QUAD);
      black = 0xffff000000000000LL;

      if(flags & 1)
        mat = kenv->ExecStack.Top();
      else
        sSystem->GetTransform(sGT_MODELVIEW,mat);

      mat.Trans3();
      mat.i.Unit3();
      mat.j.Unit3();

      sSystem->GeoBegin(geo,psys->Used*4,0,&fp,0);
      for(i=0;i<psys->Used;)
      {
        part = psys->Array+i;

        middle = part->MaxTime*(1022*fmiddle+1)/1024;
        if(part->Time<middle)
        {
          percent = sDivShift(part->Time,middle);
          Fade64(cr,c0,c1,percent);
          s = sFade(size.x,size.y,percent/65536.0f);
        }
        else
        {
          percent = sDivShift(part->Time-middle,part->MaxTime-middle);
          Fade64(cr,c1,c2,percent);
          s = sFade(size.y,size.z,percent/65536.0f);
        }

        fade = 0xffff;
        fadetime = part->MaxTime*(1022*ffade+1)/1024;
        if(part->Time<fadetime)
          fade = sDivShift(part->Time,fadetime);
        if(part->Time>part->MaxTime-fadetime)
          fade = sDivShift(part->MaxTime-part->Time,fadetime);
        cf = cr;
        Fade64(cr,black,cf,fade);

        sFSinCos(part->Pos.w,ss,sc);
        ss *= s;
        sc *= s;
        /*sc = sFCos(part->Pos.w)*s;
        ss = sFSin(part->Pos.w)*s;*/
        
        v = part->Pos; v.AddScale3(mat.i, sc); v.AddScale3(mat.j, ss);
        SetVert(fp,v.x,v.y,v.z,0,0,cr); fp+=8;
        v = part->Pos; v.AddScale3(mat.i, ss); v.AddScale3(mat.j,-sc);
        SetVert(fp,v.x,v.y,v.z,0,1,cr); fp+=8;
        v = part->Pos; v.AddScale3(mat.i,-sc); v.AddScale3(mat.j,-ss);
        SetVert(fp,v.x,v.y,v.z,1,1,cr); fp+=8;
        v = part->Pos; v.AddScale3(mat.i,-ss); v.AddScale3(mat.j, sc);
        SetVert(fp,v.x,v.y,v.z,1,0,cr); fp+=8;

        /*for(si=0;si<slices;si++)
        {
          part->Speed.y -= gravity/31623.0f;
          part->Pos.Add4(part->Speed);
        }*/
        part->Pos.AddScale4(part->Speed,slices);
        part->Pos.y -= (slices * (slices + 1) / 2) * gravity/31623.0f;
        part->Speed.y -= slices * gravity/31623.0f;

        part->Time += slices;
        if(part->Time>part->MaxTime)
          psys->Array[i] = psys->Array[--psys->Used];
        else
          i++;
      }
      sSystem->GeoEnd(geo);
      sSystem->GeoDraw(geo);
      sSystem->GeoRem(geo);
    }
  } 
}

/****************************************************************************/
/***                                                                      ***/
/***   Billboards, 49games style!                                         ***/
/***                                                                      ***/
/****************************************************************************/

KObject * __stdcall Init_Effect_Billboards(GenMinMesh *mesh,GenMaterial *mtrl,sF32 size,sF32 aspect,sF32 maxdist,sInt mode)
{
  if(mtrl) mtrl->Release();
  if(mesh) mesh->Release();
  return new GenEffect;
}

/****************************************************************************/

void __stdcall Exec_Effect_Billboards(KOp *op,KEnvironment *kenv,sF32 size,sF32 aspect,sF32 maxdist,sInt mode)
{
  GenMaterial *mtrl;
  GenMinMesh *mesh;
  sVector v;
  sMatrix mat,mat0,mat1;
  sInt count;
  sF32 dist,dpos;

  static const sInt MAXBILL = 10240;
  static const sInt MAXOT = 1024;


  static BillboardOTElem Buffer[MAXBILL];
  static BillboardOTElem *OT[MAXOT];
 
  mtrl = (GenMaterial *) op->GetInput(1)->Cache;
  mesh = (GenMinMesh *) op->GetInput(0)->Cache;
  if(mtrl && mtrl->Passes.Count>0 && mesh && mesh->Vertices.Count>0)
  {
    mat0 = kenv->ExecStack.Top();               // model matrix
    mat1 = kenv->CurrentCam.CameraSpace;        // camera matrix
    sSystem->GetTransform(sGT_MODELVIEW,mat);   // align the billboards by this
    mat.Trans3();
    dpos = mat1.l.x*mat1.k.x + mat1.l.y*mat1.k.y + mat1.l.z*mat1.k.z;

    count = 0;
    for(sInt i=0;i<MAXOT;i++)
      OT[i] = 0;
    for(sInt i=0;i<mesh->Vertices.Count;i++)
    {
      BillboardOTElem *e = &Buffer[i];
      v.x = mesh->Vertices[i].Pos.x;
      v.y = mesh->Vertices[i].Pos.y;
      v.z = mesh->Vertices[i].Pos.z;

      v.Rotate34(mat0);
      e->x = v.x;
      e->y = v.y;
      e->z = v.z;
      e->size = 1;
      e->uvval = i&7;

      dist = - dpos + (e->x*mat1.k.x + e->y*mat1.k.y + e->z*mat1.k.z);
      sInt id = sInt(dist * (MAXOT/maxdist));
      if(id>=0 && id<MAXOT)
      {
        e->Next = OT[id];
        OT[id] = e;
        count++;
      }
    }

    if(count>0)
    {
      kenv->CurrentCam.ModelSpace.Init();
      mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
      PaintBillboards(kenv,count,OT,MAXOT,size,aspect,mode);
    }
  }
}

void PaintBillboards(KEnvironment *kenv,sInt count,BillboardOTElem **OT,sInt MAXOT,sF32 size,sF32 aspect,sInt mode)
{
  sInt geo;
  sF32 *fp;
  sU16 *ip;
  sVector v,pos;
  sU32 col;
  sMatrix mat;

  static const sFRect uv[4][8] =
  {
    {
      { 0.000f,0.000f,1.000f,1.000f },
      { 0.000f,0.000f,1.000f,1.000f },
      { 0.000f,0.000f,1.000f,1.000f },
      { 0.000f,0.000f,1.000f,1.000f },
      { 0.000f,0.000f,1.000f,1.000f },
      { 0.000f,0.000f,1.000f,1.000f },
      { 0.000f,0.000f,1.000f,1.000f },
      { 0.000f,0.000f,1.000f,1.000f },
    },
    {
      { 0.000f,0.000f,0.500f,1.000f },
      { 0.500f,0.000f,1.000f,1.000f },
      { 0.000f,0.000f,0.500f,1.000f },
      { 0.500f,0.000f,1.000f,1.000f },
      { 0.000f,0.000f,0.500f,1.000f },
      { 0.500f,0.000f,1.000f,1.000f },
      { 0.000f,0.000f,0.500f,1.000f },
      { 0.500f,0.000f,1.000f,1.000f },
    },
    {
      { 0.000f,0.000f,0.500f,0.500f },
      { 0.500f,0.000f,1.000f,0.500f },
      { 0.000f,0.500f,0.500f,1.000f },
      { 0.500f,0.500f,1.000f,1.000f },
      { 0.000f,0.000f,0.500f,0.500f },
      { 0.500f,0.000f,1.000f,0.500f },
      { 0.000f,0.500f,0.500f,1.000f },
      { 0.500f,0.500f,1.000f,1.000f },
    },
    {
      { 0.000f,0.000f,0.250f,0.500f },
      { 0.250f,0.000f,0.500f,0.500f },
      { 0.500f,0.000f,0.750f,0.500f },
      { 0.750f,0.000f,1.000f,0.500f },
      { 0.000f,0.500f,0.250f,1.000f },
      { 0.250f,0.500f,0.500f,1.000f },
      { 0.500f,0.500f,0.750f,1.000f },
      { 0.750f,0.500f,1.000f,1.000f },
    },
  };

  sF32 sx = 1 * size * aspect;
  sF32 sy = 1 * size / aspect;

  sSystem->GetTransform(sGT_MODELVIEW,mat);   // align the billboards by this
  mat.Trans3();
  mat.i.Unit3();
  mat.j.Unit3();

  geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRI);

  if(count>0)
  {
    switch(mode&3)
    {
    case 0:
      break;
    case 1:
      mat.j.Init(0,1,0);
      break;
    }
    col = ~0;

    sSystem->GeoBegin(geo,count*4,count*6,&fp,(void **)&ip);

    sInt um = (mode & 0x30)>>4;
    sInt ur = 0;

    for(sInt i=MAXOT-((mode&4)?2:1);i>=0;i--)
    {
      BillboardOTElem *e = OT[i];

      while(e)
      {
        pos.x = e->x;
        pos.y = e->y;
        pos.z = e->z;
        ur = e->uvval;

        v = pos; v.AddScale3(mat.i, sx*e->size); v.AddScale3(mat.j, sy*e->size);
        SetVert(fp,v.x,v.y,v.z,uv[um][ur].x1,uv[um][ur].y0,col); fp+=8;
        v = pos; v.AddScale3(mat.i, sx*e->size); v.AddScale3(mat.j,-sy);
        SetVert(fp,v.x,v.y,v.z,uv[um][ur].x1,uv[um][ur].y1,col); fp+=8;
        v = pos; v.AddScale3(mat.i,-sx*e->size); v.AddScale3(mat.j,-sy);
        SetVert(fp,v.x,v.y,v.z,uv[um][ur].x0,uv[um][ur].y1,col); fp+=8;
        v = pos; v.AddScale3(mat.i,-sx*e->size); v.AddScale3(mat.j, sy*e->size);
        SetVert(fp,v.x,v.y,v.z,uv[um][ur].x0,uv[um][ur].y0,col); fp+=8;

        e = e->Next;
      }
    }
    for(sInt i=0;i<count;i++)
      sQuad(ip,i*4+0,i*4+1,i*4+2,i*4+3);

    sSystem->GeoEnd(geo);
    sSystem->GeoDraw(geo);
    sSystem->GeoRem(geo);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Image Layer Effect                                                 ***/
/***                                                                      ***/
/****************************************************************************/

class sMaterialImgLayerAlpha : public sMaterial
{
  sInt Tex,AlphaTex;
  sInt Setup;

public:
  sF32 Fade;

  sMaterialImgLayerAlpha(sInt tex,sInt alphaTex)
  {
    sU32 states[128],*st=states;

    static const sU32 vShader[] = {
      0xfffe0101,                                         // vs.1.1
      0x0000001f, 0x80000000, 0x900f0000,                 // dcl_position v0
      0x0000001f, 0x80000005, 0x900f0001,                 // dcl_texcoord v1
      0x00000014, 0xc00f0000, 0x90e40000, 0xa0e40000,     // m4x4 oPos,v0,c0
      0x00000001, 0xe00f0000, 0x90e40001,                 // mov oT0,v1
      0x00000001, 0xe00f0001, 0x90e40001,                 // mov oT1,v2
      0x0000ffff,                                         // end
    };

    static const sU32 pShader[] = {
      0xffff0101,                                         // ps.1.1
      0x00000042, 0xb00f0000,                             // tex t0
      0x00000042, 0xb00f0001,                             // tex t1
      0x00000003, 0x80080000, 0xb0aa0001, 0xa0e40000,     // sub r0.a,t1,c0
      0x00000050, 0x800f0000, 0x80ff0000, 0xb0e40000,
        0xa0e40001,                                       // cnd r0,r0.a,t0,c1
      0x0000ffff,                                         // end
    };

    // render states
    *st++ = sD3DRS_ALPHATESTENABLE;           *st++ = 0;
    *st++ = sD3DRS_ZENABLE;                   *st++ = sD3DZB_TRUE;
    *st++ = sD3DRS_ZWRITEENABLE;              *st++ = 0;
    *st++ = sD3DRS_ZFUNC;                     *st++ = sD3DCMP_ALWAYS;
    *st++ = sD3DRS_CULLMODE;                  *st++ = sD3DCULL_NONE;
    *st++ = sD3DRS_COLORWRITEENABLE;          *st++ = 15;
    *st++ = sD3DRS_SLOPESCALEDEPTHBIAS;       *st++ = 0;
    *st++ = sD3DRS_DEPTHBIAS;                 *st++ = 0;
    *st++ = sD3DRS_FOGENABLE;                 *st++ = 0;
    *st++ = sD3DRS_STENCILENABLE;             *st++ = 0;
    *st++ = sD3DRS_ALPHABLENDENABLE;          *st++ = 1;
    *st++ = sD3DRS_SRCBLEND;                  *st++ = sD3DBLEND_SRCALPHA;
    *st++ = sD3DRS_DESTBLEND;                 *st++ = sD3DBLEND_INVSRCALPHA;
    *st++ = sD3DRS_BLENDOP;                   *st++ = sD3DBLENDOP_ADD;

    // texture stage setup
    *st++ = sD3DTSS_0|sD3DTSS_TEXCOORDINDEX;          *st++ = 0;
    *st++ = sD3DTSS_0|sD3DTSS_TEXTURETRANSFORMFLAGS;  *st++ = 0;
    *st++ = sD3DTSS_1|sD3DTSS_TEXCOORDINDEX;          *st++ = 1;
    *st++ = sD3DTSS_1|sD3DTSS_TEXTURETRANSFORMFLAGS;  *st++ = 0;

    // sampler setup
    *st++ = sD3DSAMP_0|sD3DSAMP_MAGFILTER;    *st++ = sD3DTEXF_LINEAR;
    *st++ = sD3DSAMP_0|sD3DSAMP_MINFILTER;    *st++ = sD3DTEXF_LINEAR;
    *st++ = sD3DSAMP_0|sD3DSAMP_MIPFILTER;    *st++ = sD3DTEXF_NONE;
    *st++ = sD3DSAMP_0|sD3DSAMP_ADDRESSU;     *st++ = sD3DTADDRESS_CLAMP;
    *st++ = sD3DSAMP_0|sD3DSAMP_ADDRESSV;     *st++ = sD3DTADDRESS_CLAMP;

    *st++ = sD3DSAMP_1|sD3DSAMP_MAGFILTER;    *st++ = sD3DTEXF_LINEAR;
    *st++ = sD3DSAMP_1|sD3DSAMP_MINFILTER;    *st++ = sD3DTEXF_LINEAR;
    *st++ = sD3DSAMP_1|sD3DSAMP_MIPFILTER;    *st++ = sD3DTEXF_NONE;
    *st++ = sD3DSAMP_1|sD3DSAMP_ADDRESSU;     *st++ = sD3DTADDRESS_CLAMP;
    *st++ = sD3DSAMP_1|sD3DSAMP_ADDRESSV;     *st++ = sD3DTADDRESS_CLAMP;

    // terminator
    *st++ = ~0U;                              *st++ = ~0U;

    // create setup
    Setup = sSystem->MtrlAddSetup(states,vShader,pShader);

    // save texture handles
    Tex = tex;
    AlphaTex = alphaTex;

    if(Tex)
      sSystem->AddRefTexture(Tex);

    if(AlphaTex)
      sSystem->AddRefTexture(AlphaTex);

    Fade = 0.0f;
  }

  ~sMaterialImgLayerAlpha()
  {
    if(Tex)
      sSystem->RemTexture(Tex);

    if(AlphaTex)
      sSystem->RemTexture(AlphaTex);

    sSystem->MtrlRemSetup(Setup);
  }

  void Set(const sMaterialEnv &env)
  {
    sMaterialInstance inst;
    sMatrix mat;
    sVector pc[2];

    inst.NumTextures = 2;
    inst.Textures[0] = Tex;
    inst.Textures[1] = AlphaTex;
    inst.NumVSConstants = 4;
    inst.VSConstants = &mat.i;
    inst.NumPSConstants = 2;
    inst.PSConstants = pc;

    mat.Mul4(env.ModelSpace,sSystem->LastViewProject);
    mat.Trans4();

    pc[0].Init(0.0f,0.0f,0.0f,0.5f - Fade);
    pc[1].Init(0.0f,0.0f,0.0f,0.0f);

    sSystem->MtrlSetSetup(Setup);
    sSystem->MtrlSetInstance(inst);
  }
};

#if sLINK_FRIED

struct ImageLayerData
{
  sInt XSize,YSize;
};

KObject * __stdcall Init_Effect_ImageLayer(KOp *op,GenBitmap *alpha,sInt pass,sInt quality,sF32 fade,sInt flags,sChar *filename)
{
  const sU8 *data;
  sInt size,xs,ys;
  sInt texHandle = sINVALID;
  sU8 *pic=0;

  data = op->GetBlob(size);
#if !sPLAYER
  if(!data || size >= 8 && !sCmpMem(data,"\x89PNG\x0D\x0A\x1A\x0A",8))
  {
    sU8 *imgData = sSystem->LoadFile(filename,size);

    // actually load the image
    if(imgData && sSystem->LoadBitmapCore(imgData,size,xs,ys,pic))
    {
      data = SaveFRIED(pic,xs,ys,FRIED_SAVEALPHA,124-quality,size);
      op->SetBlob(data,size);

      delete[] pic;
    }
  }
#endif

  if(data && LoadFRIED(data,size,xs,ys,pic))
  {
    sInt txs,tys;

    if(xs <= 2048 && ys <= 2048)
    {
      sU16 *bigpic;

      if(IntroHighTexRes || (flags & 1) || xs < 800 && ys < 800) // high quality
      {
        txs = xs;
        tys = ys;

        bigpic = new sU16[txs*tys*4];
        for(sInt i=0;i<txs*tys*4;i++)
          bigpic[i] = pic[i] << 7;
      }
      else // normal quality
      {
        // sample down
        txs = xs >> 1;
        tys = ys >> 1;
        bigpic = new sU16[txs*tys*4];
        sU16 *bigptr = bigpic;

        sInt step = xs * 4;

        for(sInt y=0;y<tys;y++)
        {
          sU8 *srcp = pic + (y * 2) * step;

          for(sInt x=0;x<txs;x++,srcp+=8)
          {
            for(sInt c=0;c<4;c++)
              *bigptr++ = (srcp[c+0] + srcp[c+4] + srcp[c+step] + srcp[c+step+4]) << 5;
          }
        }
      }

      texHandle = sSystem->AddTexture(txs,tys,sTF_A8R8G8B8,bigpic);
      delete[] bigpic;
      delete[] pic;
    }
    else
    {
      sDPrintF("image '%s' is too big (%dx%d pixels, max 2048 pixels in x/y)\n",
        filename,xs,ys);

      xs = ys = 0;
      delete[] pic;
    }
  }
  else
    xs = ys = 0;

  // save data for later
  ImageLayerData *layerData = new ImageLayerData;
  layerData->XSize = xs;
  layerData->YSize = ys;

  // set up the material
  GenMaterial *mtrl = new GenMaterial;
  if(!alpha)
  {
    mtrl->AddPass(new sSimpleMaterial(texHandle,sMBF_ZON|sMBF_BLENDALPHA|sMBF_DOUBLESIDED,6),
      ENGU_OTHER,MPP_STATIC,0,1.0f,1.0f);
  }
  else
  {
    if(alpha->Texture == sINVALID)
      alpha->MakeTexture(alpha->Format);

    mtrl->AddPass(new sMaterialImgLayerAlpha(texHandle,alpha->Texture),
      ENGU_OTHER,MPP_STATIC,0,1.0f,1.0f);

    alpha->Release();
  }

  if(texHandle != sINVALID)
    sSystem->RemTexture(texHandle);

  // set up the effect
  GenEffect *fx = new GenEffect;
  fx->Material = mtrl;
  fx->Usage = ENGU_OTHER;
  fx->Pass = pass;
  fx->EffectData = layerData;

  return fx;
}

void __stdcall Exec_Effect_ImageLayer(KOp *op,KEnvironment *kenv,sInt pass,sInt quality,sF32 fade,sInt flags,sChar *filename)
{
  GenEffect *fx = (GenEffect *) op->Cache;
  sVertexDouble *vert;
  ImageLayerData *data = (ImageLayerData *) fx->EffectData;

  // update fade value for alpha test materials
  sMaterial *mtrl = fx->Material->Passes[0].Mtrl;

  if(op->GetInput(0))
    ((sMaterialImgLayerAlpha *) mtrl)->Fade = fade;

  // set material
  kenv->CurrentCam.ModelSpace = kenv->ExecStack.Top();
  mtrl->Set(kenv->CurrentCam);

  // calc dimensions
  sF32 xDim = data->XSize / 256.0f;
  sF32 yDim = data->YSize / 256.0f;

  // paint the quad.
  sInt geo = sSystem->GeoAdd(sFVF_DOUBLE,sGEO_QUAD);

  sSystem->GeoBegin(geo,4,0,(sF32 **) &vert,0);
  vert[0].Init(-xDim, yDim,0,~0U,0,0);
  vert[1].Init( xDim, yDim,0,~0U,1,0);
  vert[2].Init( xDim,-yDim,0,~0U,1,1);
  vert[3].Init(-xDim,-yDim,0,~0U,0,1);
  sSystem->GeoEnd(geo);
  sSystem->GeoDraw(geo);

  sSystem->GeoRem(geo);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/
