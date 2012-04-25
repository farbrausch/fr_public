// This file is distributed under a BSD license. See LICENSE.txt for details.


#include "_types.hpp"
#include "_start.hpp"
#include "geneffectcubes.hpp"
#include "cubehorde_shader.hpp"

#include "_loader.hpp"
#include "_util.hpp"

#if !sPLAYER
#undef new
#include <vector>
#define new new(__FILE__,__LINE__)
#endif
#include <algorithm>
#include <xmmintrin.h>

/****************************************************************************/

static const sInt CubesPerInstance = 112; // this depends on the shader!
static const sInt ShadowMapRes = 2048;    // tweak me if you dare :)

#define SSEALIGN __declspec(align(16))

sVector __declspec(align(16)) CubeBuffer[0x10100*2];
sInt CubeCount = 0;

#if sPROFILE
sMAKEZONE(WormZone,"Worm", 0xffff7800);
sMAKEZONE(PaintCubes,"PaintCubes", 0xffff3810);
#endif

/****************************************************************************/
/* bounceworm                                                               */
/****************************************************************************/

inline void makecube(sVector *buffer,sInt nr,const sVector &pos,const sVector &dir,const sVector &scale)
{
  buffer[nr*2+0].Init(dir.x*scale.x,dir.y*scale.x,dir.z*scale.x,scale.y);
  buffer[nr*2+1].Init(pos.x,pos.y,pos.z,scale.z);
}

struct particle
{
  sVector p0;
  sVector p1;

  sInt getIndex() const
  {
    sInt xInd = sRange<sInt>(p1.x/5 + 12,24,0);
    sInt zInd = sRange<sInt>(p1.z/5 + 12,24,0);
    return xInd + zInd*24;
  }
} SSEALIGN Grid[0x10000];

struct gridgrowstruct
{
  sF32 Grow;
  sF32 Float,Float2,FloatInc;
} GridGrow[0x10000];

struct wormthing
{
  sVector Pos;
  sVector Speed;
  sInt Ratio;
} Worms[16];

sInt GridUsed;
sInt WormCount;

struct GridCompare
{
  bool operator()(const particle& a,const particle& b)
  {
    return a.getIndex() < b.getIndex();
  }
};


KObject * __stdcall Init_Effect_BounceWorm(KOp *op)
{
  return MakeEffect(0);
}

void __stdcall Exec_Effect_BounceWorm(KOp *op,KEnvironment *kenv,sInt _seed,sF32 _grav,
  sF32 _damp,sF32 _float,sF32 _size,sInt timeslice,sInt _skipcubes)
{
  sZONE(WormZone);

  sInt timeSliceLen = timeslice;

  static sInt lastSliceTime = 0;
  static sRandom rnd;
  static sBool FirstTime=1;
  static sInt s_seed=-1;
  if(FirstTime || lastSliceTime-100>kenv->CurrentTime || _seed!=s_seed)
  {
    Worms[0].Pos.Init(0,10,0);
    Worms[0].Speed.Init(0.0002f,0.0001f,0);
    Worms[0].Ratio = 256;
    WormCount = 1;
    FirstTime = 0;
    lastSliceTime = kenv->CurrentTime;
    rnd.Seed(_seed);
    GridUsed = 0;
    s_seed = _seed;
  }

  sInt slices = 0;

  if(kenv->CurrentTime - lastSliceTime >= timeSliceLen)
  {
    slices = (kenv->CurrentTime - lastSliceTime) / timeSliceLen;
    lastSliceTime += slices * timeSliceLen;
  }
  slices = sMin(slices,100);

  sMatrix mat;
  mat = kenv->ExecStack.Top();


  for(sInt i=0;i<slices;i++)
  {
    for(sInt j=0;j<10;j++)
    {
      sInt wormmax = WormCount;
      for(sInt k=0;k<wormmax;k++)
      {
        wormthing *w = &Worms[k];

        if(w->Ratio<0.2f && w->Ratio>0)
          w->Ratio-=0.002f;

        w->Pos.Add3(w->Speed);
        w->Speed.y -= 0.00001f*_grav;
        w->Speed.Scale3(_damp/*0.9993f*/);
        if(w->Pos.y<0)
        {
          if(rnd.Int(256)<64 && WormCount<16)
          {
            wormthing *q = &Worms[WormCount++];
            w->Ratio = w->Ratio*0.75;
            q->Pos = w->Pos;
            q->Speed = w->Speed;
            q->Ratio = w->Ratio;
            q->Speed.y *= -1;
            q->Speed.x *= 0.75f;
            q->Speed.z *= 0.75f;
            q->Speed.x += (rnd.Float(2)-1)*0.02;
            q->Speed.y += (rnd.Float(2)-1)*0.02;
            q->Speed.z += (rnd.Float(2)-1)*0.02;
          }
          w->Speed.y *= -1;
          w->Speed.x *= 0.75f;
          w->Speed.z *= 0.75f;
          w->Speed.x += (rnd.Float(2)-1)*0.02;
          w->Speed.y += (rnd.Float(2)-1)*0.02;
          w->Speed.z += (rnd.Float(2)-1)*0.02;
        }

        if(GridUsed<0x10000 && rnd.Int(256)<w->Ratio)
        {
          sInt ind = GridUsed++;
          particle *p = &Grid[ind];

          p->p0.x = (rnd.Float(2)-1)*0.25f;
          p->p0.y = (rnd.Float(2)-1)*0.25f;
          p->p0.z = (rnd.Float(2)-1)*0.25f;
          p->p0.w = (rnd.Float(1)+1)*0.25f;
          p->p1.x = w->Pos.x + (rnd.Float(2)-1)*(rnd.Float(2)-1)*1.5f;
          p->p1.y = w->Pos.y + (rnd.Float(2)-1)*(rnd.Float(2)-1)*1.5f;
          p->p1.z = w->Pos.z + (rnd.Float(2)-1)*(rnd.Float(2)-1)*1.5f;
          p->p1.w = (rnd.Float(1)+1)*0.25f;
          GridGrow[ind].Grow = 0.0f;
          GridGrow[ind].Float = 1;
          GridGrow[ind].Float2 = 0;
          GridGrow[ind].FloatInc = rnd.Float(1);
        }
      }
    }

    // 1.0f- vermeidet denormals (das ist wichtig ;)
    for(sInt j=0;j<GridUsed;j++)
    {
      gridgrowstruct *g = &GridGrow[j];
      g->Grow += (1.0f - g->Grow) * 0.01f;
      g->Float += g->FloatInc;
      g->Float2 += g->Float*0.00001f*_float;
    }
  }

  // sort for spatial locality every hundred frames or so
  // (helps early z in conjunction with z-order painting in cubehorde)
  static sInt ctr = 0;
  /*if(++ctr == 128)
  {
    std::sort(Grid,Grid+GridUsed,GridCompare());
    ctr = 0;
  }*/

  particle *p = &Grid[0];
  sVector *pOut = &CubeBuffer[0];
/*
  static const sF32 SSEALIGN constOne[4] = { 0.0f,1.0f,0.0f,0.0f };
  __m128 one = _mm_load_ps(constOne);

  for(sInt j=0;j<GridUsed;j++)
  {
    __m128 s = _mm_add_ss(one,_mm_load_ss(&GridGrow[j]));
    __m128 p0 = _mm_load_ps(&p->p0.x);
    __m128 p1 = _mm_load_ps(&p->p1.x);

    __m128 s1 = _mm_shuffle_ps(s,s,0x15);
    s = _mm_shuffle_ps(s,s,0x00);

    _mm_stream_ps(&pOut[0].x,_mm_mul_ps(p0,s));
    _mm_stream_ps(&pOut[1].x,_mm_mul_ps(p1,s1));

    p++;
    pOut+=2;
  }
*/

  sInt j=0;
  sInt skip = 0;
  sInt inc = 256-_skipcubes;
  for(sInt jj=0;jj<GridUsed;jj++)
  {
    skip += inc;
    if(skip>=256)
    {
      skip -= 256;
      particle *p = &Grid[jj];

      sF32 s = GridGrow[jj].Grow * _size;
      sVector pos; pos.Init4(p->p1.x,p->p1.y+ + GridGrow[jj].Float2,p->p1.z,1);
      pos.Rotate34(mat);

      CubeBuffer[j*2+0].x = p->p0.x * s;
      CubeBuffer[j*2+0].y = p->p0.y * s;
      CubeBuffer[j*2+0].z = p->p0.z * s;
      CubeBuffer[j*2+0].w = p->p0.w * s;
      CubeBuffer[j*2+1].x = pos.x;
      CubeBuffer[j*2+1].y = pos.y;
      CubeBuffer[j*2+1].z = pos.z;
      CubeBuffer[j*2+1].w = p->p1.w * s;

      j++;
    }
  }

  CubeCount = j;
}

/****************************************************************************/
/****************************************************************************/


KObject * __stdcall Init_Effect_Wirbeln(KOp *op)
{
  return MakeEffect(0);
}


const sInt wirbelcount = 0x8000;
struct wirbels
{
  sF32 rx,ry,rz,sy;
  sF32 phi,rad,ele,rho;
} SSEALIGN wirbeln[wirbelcount];

void __stdcall Exec_Effect_Wirbeln(KOp *op,KEnvironment *kenv,
  sF32 _speed,sF32 _speeddelta,sF32 _size,sF32 _radius,sF32 _width,sF32 _height,
  sInt _wobble1,sInt _wobble2,sF32 _strength,sF32 _yscale,sF32 _yadd,sInt _count)
{
  static sInt firsttime = 1;
  static sRandom rnd;
  static sF32 s_speeddelta,s_size,s_radius,s_width,s_height;



  if(firsttime || s_speeddelta!=_speeddelta || s_size!=_size)
  {
    for(sInt i=0;i<wirbelcount;i++)
    {
      wirbels *w = &wirbeln[i];
      w->phi = rnd.Float(sPI2F);
      w->rho = (1+rnd.Float(_speeddelta));
      w->rad = (rnd.Float(2)-1);
      w->ele =(rnd.Float(2)-1);
      sF32 sx = (rnd.Float(1)+1)*_size;
      w->sy = (rnd.Float(1)+1)*_size;
//      w->sz = (rnd.Float(1)+1)*0.125;
      w->rx = (rnd.Float(2)-1)*0.5*sx;
      w->ry = (rnd.Float(2)-1)*0.5*sx;
      w->rz = (rnd.Float(2)-1)*0.5*sx;

      s_speeddelta = _speeddelta;
      s_size = _size;
      s_radius = _radius;
      s_width = _width;
      s_height = _height;
    }
    firsttime = 0;
  }


  sMatrix mat;
  mat = kenv->ExecStack.Top();

  sF32 phase = kenv->BeatTime/65536.0f*_speed;
  sInt used = 0;
  sF32 __declspec(align(16)) x[4],s[4],c[4];
  // make we'll stay within the bounds of 'wirbeln' during the loop
  sVERIFY(_count >=0 && _count < wirbelcount);
  for(sInt i=0;i<_count;i++)
  {
    {
      wirbels *w = &wirbeln[i];
      sF32 angle = w->phi+phase*w->rho;
      x[0] = angle;
      x[1] = angle*_wobble1;
      x[2] = angle*_wobble2;
      x[3] = 0;
      SSE_SinCos4(x,s,c);
      sF32 px = s[0]*(w->rad*_width+_radius)*(s[1]+_strength) ;
      sF32 py = w->ele*_height+(c[2]*_yscale+_yadd);
      sF32 pz = c[0]*(w->rad*_width+_radius)*(c[1]+_strength);

      sVector pos; pos.Init4(px,py,pz,1);
      pos.Rotate34(mat);

      CubeBuffer[used++].Init(w->rx,w->ry,w->rz,w->sy);
      CubeBuffer[used++].Init(pos.x,pos.y,pos.z,w->sy);
    }
  }

  CubeCount = _count; 
}



/****************************************************************************/
/****************************************************************************/

class BatchSorter
{
  const sF32 *Depth;

public:
  BatchSorter(const sF32 *depth)
    : Depth(depth)
  {
  }

  bool operator()(sInt i1,sInt i2) const
  {
    return Depth[i1] < Depth[i2];
  }
};


class CubeHordeEffect : public GenEffect
{
  static sInt ShadowRT;

  CubeHordeShader *Shader;
  CubeHordeShaderClip *ShaderReflect;
  CubeHordeShadowWrite *ShaderShadWr;
  CubeHordeShadowRender *ShaderShadRe;
  CubeHordeShadowRenderClip *ShaderShadReReflect;

  GroundPlaneShader *ShaderGPStencil;
  GroundPlaneShader *ShaderGP;

  sInt Geo;
  sInt GeoGround;

  void PaintItAll(sASCMaterial *shader,const sVector *xforms,sInt used,const sMatrix &mvp)
  {
    sCBuffer<CubeHordeXForm> xform;
    sInt count = 0;

    // prepare batch list
    sInt nBatches = (used+CubesPerInstance-1)/CubesPerInstance;
    sInt *batchInd = new sInt[nBatches];
    sF32 *batchDepth = new sF32[nBatches];

    for(sInt i=0;i<nBatches;i++)
    {
      sF32 avgZ = 0.0f;

      sInt startIdx = i*CubesPerInstance;
      sInt endIdx = sMin(startIdx+CubesPerInstance,used);
      for(sInt j=startIdx;j<endIdx;j++)
      {
        const sVector *trans = xforms+j*2+1;
        avgZ += trans->x*mvp.k.x + trans->y*mvp.k.y + trans->z*mvp.k.z;
      }

      batchInd[i] = i;
      batchDepth[i] = avgZ / (endIdx - startIdx);
    }

    // sort batches
    std::sort(batchInd,batchInd+nBatches,BatchSorter(batchDepth));

    sZONE(PaintCubes);

    // paint them
    for(sInt i=0;i<nBatches;i++)
    {
      sInt startIdx = batchInd[i] * CubesPerInstance;
      sInt endIdx = sMin(startIdx+CubesPerInstance,used);
      sInt batchCount = endIdx-startIdx;

      xform.Modify();
      xform.OverridePtr(CubeBuffer+startIdx*2);
      shader->Set(&Base,&Pixel,&xform);

      sSystem->GeoDraw(Geo,batchCount*6*6);
    }

    delete[] batchInd;
    delete[] batchDepth;
  }

public:
  sVector UpVector;
  sVector LightColor;
  sVector DarkColor;
  sVector ReflectLightColor;
  sVector ReflectDarkColor;
  sVector ShadowColor;
  sVector LightDir;
  sF32 LightDist;
  sF32 DepthRange;
  sF32 GroundPlaneY;
  sF32 GroundPlaneSize;

  sCBuffer<CubeHordeBasePara> Base;
  sCBuffer<CubeHordePixelPara> Pixel;

  CubeHordeEffect(sF32 groundPlaneY,sF32 groundPlaneSize)
  {
    GroundPlaneY = groundPlaneY;
    GroundPlaneSize = groundPlaneSize;

    // shadowmap rt
    if(ShadowRT == sINVALID)
      ShadowRT = sSystem->AddTexture(ShadowMapRes,ShadowMapRes,sTF_R32F|sTF_NEEDEXTRAZ,0);
    else
      sSystem->AddRefTexture(ShadowRT);

    // shaders
    Shader = new CubeHordeShader;
    Shader->Flags = sMTRL_ZON | sMTRL_CULLON;
    Shader->TFlags[0] = sMTF_LEVEL0;
    Shader->Texture[0] = -4; // -4^=CubeTex
    Shader->Prepare(0);

    ShaderReflect = new CubeHordeShaderClip;
    ShaderReflect->Flags = sMTRL_ZON | sMTRL_CULLINV;
    ShaderReflect->TFlags[0] = sMTF_LEVEL0;
    ShaderReflect->Texture[0] = -4; // -4^=CubeTex
    ShaderReflect->Prepare(0);

    ShaderShadWr = new CubeHordeShadowWrite;
    ShaderShadWr->Flags = sMTRL_ZON | sMTRL_CULLON;
    ShaderShadWr->Prepare(0);

    ShaderShadRe = new CubeHordeShadowRender;
    ShaderShadRe->Flags = sMTRL_ZON | sMTRL_CULLON;
    ShaderShadRe->TFlags[0] = sMTF_LEVEL0;
    ShaderShadRe->Texture[0] = -4; // -4^=CubeTex
    ShaderShadRe->TFlags[1] = sMTF_LEVEL0|sMTF_CLAMP;
    ShaderShadRe->Texture[1] = ShadowRT;
    ShaderShadRe->Prepare(0);

    ShaderShadReReflect = new CubeHordeShadowRenderClip;
    ShaderShadReReflect->Flags = sMTRL_ZON | sMTRL_CULLINV | sMTRL_STENCILSS;
    ShaderShadReReflect->FuncFlags[sMFT_STENCIL] = sMFF_NOTEQUAL;
    ShaderShadReReflect->StencilRef = 0;
    ShaderShadReReflect->StencilOps[sMSI_FAIL] = sMSO_KEEP;
    ShaderShadReReflect->StencilOps[sMSI_ZFAIL] = sMSO_KEEP;
    ShaderShadReReflect->StencilOps[sMSI_PASS] = sMSO_KEEP;
    ShaderShadReReflect->TFlags[0] = sMTF_LEVEL0;
    ShaderShadReReflect->Texture[0] = -4; // -4^=CubeTex
    ShaderShadReReflect->TFlags[1] = sMTF_LEVEL0|sMTF_CLAMP;
    ShaderShadReReflect->Texture[1] = ShadowRT;
    ShaderShadReReflect->Prepare(0);

    ShaderGP = new GroundPlaneShader;
    ShaderGP->Flags = sMTRL_ZON;
    ShaderGP->BlendColor = sMB_ALPHA;
    ShaderGP->TFlags[0] = sMTF_LEVEL0|sMTF_CLAMP;
    ShaderGP->Texture[0] = ShadowRT;
    ShaderGP->Prepare(0);

    ShaderGPStencil = new GroundPlaneShader;
    ShaderGPStencil->Flags = sMTRL_ZREAD | sMTRL_STENCILSS | sMTRL_MSK_RGBA;
    ShaderGPStencil->FuncFlags[sMFT_STENCIL] = sMFF_ALWAYS;
    ShaderGPStencil->StencilOps[sMSI_FAIL] = sMSO_KEEP;
    ShaderGPStencil->StencilOps[sMSI_ZFAIL] = sMSO_KEEP;
    ShaderGPStencil->StencilOps[sMSI_PASS] = sMSO_INC;
    ShaderGPStencil->TFlags[0] = sMTF_LEVEL0|sMTF_CLAMP;
    ShaderGPStencil->Texture[0] = ShadowRT;
    ShaderGPStencil->Prepare(0);

    // prepare cube geometry
    sU16 *ip;
    sVertexCubeSpecial *vp;
  
    Geo = sSystem->GeoAdd(sFVF_CUBESPEC,sGEO_TRI|sGEO_STATIC);
    sSystem->GeoBegin(Geo,8*CubesPerInstance,6*6*CubesPerInstance,
      (sF32 **)&vp,(void **)&ip);

    for(sInt i=0;i<CubesPerInstance;i++)
    {
      static sInt indices[4*6] = {
        2,3,1,0, // front
        3,7,5,1, // right
        7,6,4,5, // back
        6,2,0,4, // left
        6,7,3,2, // top
        0,1,5,4, // bottom
      };

      // 6 quads (sides)
      for(sInt j=0;j<6;j++)
        sQuad(ip,8*i+indices[j*4+0],8*i+indices[j*4+1],8*i+indices[j*4+2],8*i+indices[j*4+3]);

      // vertices
      for(sInt j=0;j<8;j++)
      {
        vp->PosInd = j & 0xff;
        vp->NormInd = 0;
        vp->XFormInd = i*2;
        vp->_pad = 0;
        vp->Color = 0xffffffff;
        vp++;
      }
    }

    sSystem->GeoEnd(Geo);

    // groundplane geometry
    sVertexCompact *vpc;

    GeoGround = sSystem->GeoAdd(sFVF_COMPACT,sGEO_TRI|sGEO_STATIC);
    sSystem->GeoBegin(GeoGround,4,6,(sF32 **)&vpc,(void **)&ip);

    sQuad(ip,0,1,2,3);
    for(sInt i=0;i<4;i++)
    {
      sInt j = i^(i>>1);

      vpc->x = (j&1) ? GroundPlaneSize : -GroundPlaneSize;
      vpc->y = GroundPlaneY;
      vpc->z = (j&2) ? -GroundPlaneSize : GroundPlaneSize;
      //vpc->c0 = 0x80242490;
      vpc->c0 = 0;
      vpc++;
    }

    sSystem->GeoEnd(GeoGround);
  }

  ~CubeHordeEffect()
  {
    sSystem->RemTexture(ShadowRT);

    sRelease(Shader);
    sRelease(ShaderReflect);
    sRelease(ShaderShadWr);
    sRelease(ShaderShadRe);
    sRelease(ShaderShadReReflect);

    sRelease(ShaderGPStencil);
    sRelease(ShaderGP);

    sSystem->GeoRem(Geo);
    sSystem->GeoRem(GeoGround);
  }

  void Paint(const sMatrix &model,sInt paintFlags)
  {
    // simulate
    sInt used = CubeCount;
    const sInt totalCount = 65536;
    sVERIFY(used<=totalCount);

    // calc model-view-projection matrix
    sMatrix mvp;
    mvp.Mul4(model,sSystem->LastViewProject);

    // set constants
    Pixel.Modify();
    Pixel.Data->darkCol = DarkColor;
    Pixel.Data->lightCol = LightColor;

    Base.Modify();
    Base.Data->lightDir = LightDir;
    Base.Data->up = UpVector;
    Base.Data->shadowCol = ShadowColor;

    for(sInt i=0;i<8;i++)
      Base.Data->positions[i].Init((i&1) ? 1 : -1,(i&2) ? 1 : -1,(i&4) ? 1 : -1,0.0f);

    for(sInt i=0;i<6;i++)
    {
      Base.Data->normals[i].Init(0,0,0,0);
      Base.Data->normals[i][i/2] = (i&1) ? 1 : -1;
    }

    // set up projection (new); maybe determine bbox for scaling.
    sMatrix total,rescale,totalL,scaleL,mirror;

    Base.Data->lightDir = LightDir;

    total.InitDir(LightDir);
    total.Trans3(); // InitDir ist falschrum!!!!
    rescale.Init();
    rescale.i.x = 1.0f / GroundPlaneSize;
    rescale.j.y = 1.0f / GroundPlaneSize;
    rescale.k.z = 1.0f / DepthRange;
    rescale.l.Init(0.0f,0.0f,LightDist / DepthRange,1.0f);

    total.MulA(total,rescale);

    mirror.Init();
    mirror.j.Init(0.0f,-1.0f,0.0f,0.0f);
    mirror.l.Init(0.0f,2.0f * GroundPlaneY,0.0f,1.0f);

    // light projection
    scaleL.Init();
    scaleL.i.x = 0.5f;
    scaleL.j.y = -0.5f;
    scaleL.l.Init(0.5f,0.5f,0.0f);
    totalL.MulA(total,scaleL);

    Base.Data->lightMvp[0].Init(totalL.i.x,totalL.j.x,totalL.k.x,totalL.l.x);
    Base.Data->lightMvp[1].Init(totalL.i.y,totalL.j.y,totalL.k.y,totalL.l.y);
    Base.Data->lightMvp[2].Init(totalL.i.z,totalL.j.z,totalL.k.z,totalL.l.z);

    // render shadow map: mvp=rescaled light projection
    if(paintFlags & 1)
    {
      Base.Data->mvp = total;
      Base.Data->mvp.Trans4();

      // render to shadowmap here
      sViewport oldVP = sSystem->CurrentViewport;
      sViewport newVP;

      newVP.InitTex(ShadowRT);
      sSystem->SetViewport(newVP);
      sSystem->Clear(sVCF_ALL,~0);

      PaintItAll(ShaderShadWr,CubeBuffer,used,mvp);

      sSystem->SetViewport(oldVP);
    }

    if(paintFlags & 2)
    {
      // render ground plane into stencil
      Base.Modify();
      Base.Data->mvp = mvp;
      Base.Data->mvp.Trans4();

      ShaderGPStencil->Set(&Base);
      sSystem->GeoDraw(GeoGround);

      // render mirrored cubes
      Pixel.Modify();
      Pixel.Data->darkCol = ReflectDarkColor;
      Pixel.Data->lightCol = ReflectLightColor;
      Pixel.Data->clipPlane.Init4(0.0f,1.0f,0.0f,-GroundPlaneY);

      Base.Modify();
      sMatrix mat;
      mat.MulA(model,mirror);
      Base.Data->mvp.Mul4(mat,sSystem->LastViewProject);
      Base.Data->mvp.Trans4();
      PaintItAll(ShaderShadReReflect,CubeBuffer,used,mvp);
    }

    // render actual ground plane+cubes
    Pixel.Modify();
    Pixel.Data->darkCol = DarkColor;
    Pixel.Data->lightCol = LightColor;

    Base.Modify();
    Base.Data->mvp = mvp;
    Base.Data->mvp.Trans4();

    if(paintFlags & 4)
    {
      ShaderGP->Set(&Base);
      sSystem->GeoDraw(GeoGround);
    }

    if(paintFlags & 8)
      PaintItAll(ShaderShadRe,CubeBuffer,used,mvp);

    if(paintFlags & 16)
      PaintItAll(Shader,CubeBuffer,used,mvp);

    sResetConstantBuffers();
  }
};

sInt CubeHordeEffect::ShadowRT = sINVALID;

/****************************************************************************/

KObject * __stdcall Init_Effect_Cubes(KOp *op)
{
  sF32 groundPlaneY = *op->GetEditPtrF(9);
  sF32 groundPlaneSize = *op->GetEditPtrF(10);

  CubeHordeEffect *fx = new CubeHordeEffect(groundPlaneY,groundPlaneSize);
  fx->Pass = *op->GetEditPtrU(11);

  sREGZONE(WormZone);
  sREGZONE(PaintCubes);

  return fx;
}

void __stdcall Exec_Effect_Cubes(KOp *op,KEnvironment *kenv)
{
  CubeHordeEffect *fx = (CubeHordeEffect *) op->Cache;

  if(fx)
  {
    sMatrix invModel;

    kenv->CurrentCam.ModelSpace = kenv->ExecStack.Top();
    sSystem->SetViewProject(&kenv->CurrentCam);

    invModel = kenv->CurrentCam.ModelSpace;
    invModel.Trans3();

    // copy parameters
    sCopyMem(&fx->UpVector,op->GetAnimPtrF(0),3*sizeof(sF32));
    fx->UpVector.w = 0.0f;
    fx->UpVector.UnitSafe3();

    fx->LightColor.InitColor(*op->GetAnimPtrU(3));
    fx->DarkColor.InitColor(*op->GetAnimPtrU(15));
    fx->ReflectLightColor.InitColor(*op->GetAnimPtrU(13));
    fx->ReflectDarkColor.InitColor(*op->GetAnimPtrU(16));
    fx->ShadowColor.Init(0,0,0,0);
    fx->ShadowColor.w = *op->GetAnimPtrF(14);
    
    sCopyMem(&fx->LightDir,op->GetAnimPtrF(4),3*sizeof(sF32));
    fx->LightDir.w = 0.0f;
    fx->LightDir.Rotate3(invModel);
    fx->LightDir.UnitSafe3();
    fx->LightDist = *op->GetAnimPtrF(7);
    fx->DepthRange = *op->GetAnimPtrF(8);

    fx->Paint(kenv->CurrentCam.ModelSpace,*op->GetEditPtrU(12));
  }
}

/****************************************************************************/



/****************************************************************************/
/* xsi2cube                                                                 */
/****************************************************************************/

#if !sPLAYER
using namespace sLoader;

struct Edge
{
  sInt a,b;

  Edge(sInt e0,sInt e1)
    : a(e0),b(e1)
  {
  }

  bool operator == (const Edge& x)
  {
    return a == x.a && b == x.b;
  }

  bool operator < (const Edge& x)
  {
    return a < x.a || (a == x.a && b < x.b);
  }
};

struct MatchFirstVert
{
  bool operator()(const Edge& e1,const Edge& e2)
  {
    return e1.a < e2.a;
  }

  bool operator()(const Edge& e,sInt v)
  {
    return e.a < v;
  }

  bool operator()(sInt v,const Edge& e)
  {
    return v < e.a;
  }
};

struct OutCube
{
  sVector v0;
  sVector v1;

  OutCube(const sVector& pos,const sVector& dirX,sF32 scaleY,sF32 scaleZ)
  {
    v0 = dirX;
    v0.w = scaleY;
    v1 = pos;
    v1.w = scaleZ;
  }
};

static void XSI2CubeProcessCluster(std::vector<OutCube> &Cubes,Cluster *cl)
{
  Face *fc;

  typedef std::vector<Edge> EdgeVec;
  typedef std::vector<sInt> IntVec;

  // calc number of edges we need
  EdgeVec edges;
  sInt nEdges = 0;
  sFORALL(cl->Faces,fc)
    nEdges += fc->Count*2;

  edges.reserve(nEdges);

  // make list of (directed) edges
  sFORALL(cl->Faces,fc)
  {
    sInt count = fc->Count;
    if(count < 3)
      continue;

    edges.push_back(Edge(fc->Index[count-1],fc->Index[0]));
    edges.push_back(Edge(fc->Index[0],fc->Index[count-1]));
    for(sInt i=0;i<count-1;i++)
    {
      edges.push_back(Edge(fc->Index[i],fc->Index[i+1]));
      edges.push_back(Edge(fc->Index[i+1],fc->Index[i]));
    }
  }

  // sort and eliminate equivalent ones
  std::sort(edges.begin(),edges.end());
  edges.erase(std::unique(edges.begin(),edges.end()),edges.end());

  // now, all edges outgoing from vertex a are listed in order.
  // determine connected components of vertices.
  IntVec connComp,connCompSize;
  EdgeVec::iterator itCur,itEnd;

  connComp.reserve(cl->Vertices.Count);
  connCompSize.resize(cl->Vertices.Count);
  itCur = edges.begin();
  itEnd = edges.end();

  for(sInt i=0;i<cl->Vertices.Count;i++)
  {
    sInt component = i; // first, each vertex is its own component
      
    // advance edge iterator till we're at the current vertex
    while(itCur != itEnd && itCur->a < i)
      ++itCur;

    // go through all connected vertices and see if we can merge
    for(; itCur != itEnd && itCur->a == i; ++itCur)
      if(itCur->b < i)
      {
        if(connComp[itCur->b] < component) // can merge?
          component = itCur->b; // yes, do it!
        else
        {
          sVERIFY(component < itCur->b);
          connComp[itCur->b] = component;
        }
      }

    connComp.push_back(component);
  }

  for(size_t i=0;i<connComp.size();i++)
  {
    connComp[i] = connComp[connComp[i]];
    ++connCompSize[connComp[i]];
  }

  // determine unique connected components and their starting vertices
  std::sort(connComp.begin(),connComp.end());
  connComp.erase(std::unique(connComp.begin(),connComp.end()),connComp.end());

  for(IntVec::iterator it = connComp.begin(); it != connComp.end(); ++it)
  {
    if(connCompSize[*it] != 8) // ignore connected components with !=8 vertices
      continue;

    std::pair<EdgeVec::iterator,EdgeVec::iterator> range;
    range = std::equal_range(edges.begin(),edges.end(),*it,MatchFirstVert());
    if(range.second - range.first == 3)
    {
      // 3 outgoing vertices for this one, this looks good.
      // determine edge vectors, size and cube center.
      int v0 = *it;
      int v1 = range.first->b;
      int v2 = (range.first+1)->b;
      int v3 = (range.first+2)->b;

      sVector ev0,ev1,ev2,center,tmp;
      ev0.Sub3(cl->Vertices[v1].Pos,cl->Vertices[v0].Pos);
      ev1.Sub3(cl->Vertices[v2].Pos,cl->Vertices[v0].Pos);
      ev2.Sub3(cl->Vertices[v3].Pos,cl->Vertices[v0].Pos);

      ev0.Scale3(0.5f);
      ev1.Scale3(0.5f);
      ev2.Scale3(0.5f);

      center = cl->Vertices[v0].Pos;
      center.Add3(ev0);
      center.Add3(ev1);
      center.Add3(ev2);

      // check for consistent orientation
      tmp.Cross3(ev0,ev1);
      if(tmp.Dot3(ev2) < 0.0f) // nope, inverted
        sSwap(ev1,ev2);

      // first one shouldn't be (almost) parallel to y axis
      if(sFAbs(ev0.y) >= ev0.Abs3() * 0.996f)
      {
        // permute cyclically
        tmp = ev0;
        ev0 = ev1;
        ev1 = ev2;
        ev2 = tmp;
      }

      // build output cube
      Cubes.push_back(OutCube(center,ev0,ev1.Abs3(),ev2.Abs3()));
    }
  }
}

static void XSI2CubeProcessModel(std::vector<OutCube> &Cubes,Model *mod)
{
  // TODO: transform matrices (but don't need them right now, so who cares)
  Model *cm;
  sFORALL(mod->Childs,cm)
    XSI2CubeProcessModel(Cubes,cm);

  Cluster *cc;
  sFORALL(mod->Clusters,cc)
    XSI2CubeProcessCluster(Cubes,cc);
}
#endif
class XSI2CubeEffect : public GenEffect
{
public:
  sVector *Data;
  sInt Count;

  XSI2CubeEffect(const sVector *data,sInt count,sInt norm)
  {
    Data = new sVector[count*2];
    sCopyMem(Data,data,count*2*sizeof(sVector));
    Count = count;

    if(norm)
      Normalize();
  }

  ~XSI2CubeEffect()
  {
    delete[] Data;
    Data = 0;
    Count = 0;
  }

  void Normalize()
  {
    // determine bbox
    sVector min,max;

    min.Init( 1e+15f, 1e+15f, 1e+15f);
    max.Init(-1e+15f,-1e+15f,-1e+15f);

    for(sInt i=0;i<Count;i++)
    {
      sVector &v = Data[i*2+1];

      min.x = sMin(min.x,v.x);
      max.x = sMax(max.x,v.x);
      min.y = sMin(min.y,v.y);
      max.y = sMax(max.y,v.y);
      min.z = sMin(min.z,v.z);
      max.z = sMax(max.z,v.z);
    }

    // determine longest axis and scale so that it goes from -1..1
    sF32 longestAxis = sMax(sMax(max.x-min.x,max.y-min.y),max.z-min.z);
    sF32 scalef = 2.0f / longestAxis;
    sF32 cx = 0.5f * (max.x + min.x);
    sF32 cy = 0.5f * (max.y + min.y);
    sF32 cz = 0.5f * (max.z + min.z);

    for(sInt i=0;i<Count;i++)
    {
      sVector &v = Data[i*2+1];

      Data[i*2+0].Scale4(scalef);
      v.x = (v.x - cx) * scalef;
      v.y = (v.y - cy) * scalef;
      v.z = (v.z - cz) * scalef;
      v.w *= scalef;
    }
  }
};

static sInt CubeMorpherActive = 0;
static sBool CubeMorpherFirst = sTRUE;
static sF32 CubeMorpherWeight = 0.0f;
static sInt CubeMorpherCount = 0;

KObject * __stdcall Init_Effect_XSI2Cube(KOp *op)
{
  sInt size;
  const sU8 *data = op->GetBlob(size);

#if !sPLAYER
  if(!data)
  {
    Scene scene;
    std::vector<OutCube> cubes;

    sChar *filename = op->GetString(0);
    if(scene.LoadXSI(filename) && scene.Optimize())
    {
      XSI2CubeProcessModel(cubes,scene.Root);

      sInt blobSize = cubes.size() * 2 * sizeof(sVector);
      if(blobSize)
      {
        sDPrintF("xsi2cube successful, %d cubes generated\n",cubes.size());

        sU8 *blobData = new sU8[blobSize];
        sCopyMem(blobData,&cubes[0],blobSize);
        op->SetBlob(blobData,blobSize);
      }
      else
        op->SetBlob(0,0);
    }
    else
      op->SetBlob(0,0);

    data = op->GetBlob(size);
  }
#endif

  sInt norm = *op->GetEditPtrU(0) & 1;
  return new XSI2CubeEffect((const sVector *) data,size/(2*sizeof(sVector)),norm);
}

void __stdcall Exec_Effect_XSI2Cube(KOp *op,KEnvironment *kenv)
{
  XSI2CubeEffect *fx = (XSI2CubeEffect *) op->Cache;

  if(fx)
  {
    sMatrix mat;
    mat = kenv->ExecStack.Top();

    sF32 s1 = mat.i.Abs3();
    sF32 s2 = mat.j.Abs3();
    sF32 s3 = mat.k.Abs3();
    sF32 s = (s1 + s2 + s3) / 3.0f;

    if(CubeMorpherActive)
    {
      s *= CubeMorpherWeight;
      mat.i.Scale3(CubeMorpherWeight);
      mat.j.Scale3(CubeMorpherWeight);
      mat.k.Scale3(CubeMorpherWeight);
      mat.l.Scale3(CubeMorpherWeight);
    }

    const sVector *src = fx->Data;
    sVector *dst = CubeBuffer;

    if(!CubeMorpherActive || CubeMorpherFirst)
    {
      for(sInt i=0;i<fx->Count;i++)
      {
        sF32 x = src[0].x, y = src[0].y, z = src[0].z;

        dst[0].x = mat.i.x * x + mat.j.x * y + mat.k.x * z;
        dst[0].y = mat.i.y * x + mat.j.y * y + mat.k.y * z;
        dst[0].z = mat.i.z * x + mat.j.z * y + mat.k.z * z;
        dst[0].w = src[0].w * s;

        x = src[1].x; y = src[1].y; z = src[1].z;

        dst[1].x = mat.i.x * x + mat.j.x * y + mat.k.x * z + mat.l.x;
        dst[1].y = mat.i.y * x + mat.j.y * y + mat.k.y * z + mat.l.y;
        dst[1].z = mat.i.z * x + mat.j.z * y + mat.k.z * z + mat.l.z;
        dst[1].w = src[1].w * s;

        dst += 2;
        src += 2;
      }
    }
    else
    {
      for(sInt i=0;i<fx->Count;i++)
      {
        sF32 x = src[0].x, y = src[0].y, z = src[0].z;

        dst[0].x += mat.i.x * x + mat.j.x * y + mat.k.x * z;
        dst[0].y += mat.i.y * x + mat.j.y * y + mat.k.y * z;
        dst[0].z += mat.i.z * x + mat.j.z * y + mat.k.z * z;
        dst[0].w += src[0].w * s;

        x = src[1].x; y = src[1].y; z = src[1].z;

        dst[1].x += mat.i.x * x + mat.j.x * y + mat.k.x * z + mat.l.x;
        dst[1].y += mat.i.y * x + mat.j.y * y + mat.k.y * z + mat.l.y;
        dst[1].z += mat.i.z * x + mat.j.z * y + mat.k.z * z + mat.l.z;
        dst[1].w += src[1].w * s;

        dst += 2;
        src += 2;
      }
    }
    
    /*// TODO: get rid of memory copy?
    sCopyMem(CubeBuffer,fx->Data,fx->Count*2*sizeof(sVector));*/
    if(!CubeMorpherActive)
      CubeCount = fx->Count;
    else
    {
      if(!CubeMorpherCount)
        CubeMorpherCount = fx->Count;
      else
        CubeMorpherCount = sMin(CubeMorpherCount,fx->Count);
    }
  }
}

KObject * __stdcall Init_Effect_CubeMorpher(KOp *op)
{
  return MakeEffect(0);
}

void __stdcall Exec_Effect_CubeMorpher(KOp *op,KEnvironment *kenv)
{
  ++CubeMorpherActive;

  sInt inCount = op->GetInputCount();

  if(inCount)
  {
    sF32 time = sRange<sF32>(kenv->Var[KV_TIME].x,1.0f,0.0f) * (inCount-1);

    sInt timeDown = time;
    sF32 fracTime = time - timeDown;

    fracTime = (3.0f - 2.0f * fracTime) * fracTime * fracTime;
    CubeMorpherCount = 0;

    for(sInt i=0;i<inCount;i++)
    {
      sBool exec = sFALSE;

      if(i == timeDown)
      {
        CubeMorpherFirst = sTRUE;
        CubeMorpherWeight = 1.0f - fracTime;
        exec = sTRUE;
      }
      else if(i == timeDown+1)
      {
        CubeMorpherFirst = sFALSE;
        CubeMorpherWeight = fracTime;
        exec = sTRUE;
      }

      if(exec)
        op->GetInput(i)->Exec(kenv);
    }

    CubeCount = CubeMorpherCount;
  }

  CubeMorpherActive--;
}

/****************************************************************************/

