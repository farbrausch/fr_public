/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/chaosfx.hpp"
#include "wz4frlib/chaosfx_ops.hpp"
#include "wz4frlib/chaosfx_shader.hpp"
#include "wz4frlib/chaos_font.hpp"
#include "base/graphics.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Simple Example Effect                                              ***/
/***                                                                      ***/
/****************************************************************************/

RNCubeExample::RNCubeExample()
{
  Mtrl = 0;

  // this is always the same

  Anim.Init(Wz4RenderType->Script);   

  // create a cube.

  Geo = new sGeometry(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);
}

RNCubeExample::~RNCubeExample()
{
  delete Geo;

  // Mtrl is a wz4 object with refcounting

  Mtrl->Release();
}

void RNCubeExample::MakeCube()
{
  // i hate cubes

  sVertexStandard *vp=0;

  Geo->BeginLoadVB(24,sGD_FRAME,&vp);
  vp->Init(-1, 1,-1,  0, 0,-1, 1,0); vp++; // 3
  vp->Init( 1, 1,-1,  0, 0,-1, 1,1); vp++; // 2
  vp->Init( 1,-1,-1,  0, 0,-1, 0,1); vp++; // 1
  vp->Init(-1,-1,-1,  0, 0,-1, 0,0); vp++; // 0

  vp->Init(-1,-1, 1,  0, 0, 1, 1,0); vp++; // 4
  vp->Init( 1,-1, 1,  0, 0, 1, 1,1); vp++; // 5
  vp->Init( 1, 1, 1,  0, 0, 1, 0,1); vp++; // 6
  vp->Init(-1, 1, 1,  0, 0, 1, 0,0); vp++; // 7

  vp->Init(-1,-1,-1,  0,-1, 0, 1,0); vp++; // 0
  vp->Init( 1,-1,-1,  0,-1, 0, 1,1); vp++; // 1
  vp->Init( 1,-1, 1,  0,-1, 0, 0,1); vp++; // 5
  vp->Init(-1,-1, 1,  0,-1, 0, 0,0); vp++; // 4

  vp->Init( 1,-1,-1,  1, 0, 0, 1,0); vp++; // 1
  vp->Init( 1, 1,-1,  1, 0, 0, 1,1); vp++; // 2
  vp->Init( 1, 1, 1,  1, 0, 0, 0,1); vp++; // 6
  vp->Init( 1,-1, 1,  1, 0, 0, 0,0); vp++; // 5

  vp->Init( 1, 1,-1,  0, 1, 0, 1,0); vp++; // 2
  vp->Init(-1, 1,-1,  0, 1, 0, 1,1); vp++; // 3
  vp->Init(-1, 1, 1,  0, 1, 0, 0,1); vp++; // 7
  vp->Init( 1, 1, 1,  0, 1, 0, 0,0); vp++; // 6

  vp->Init(-1, 1,-1, -1, 0, 0, 1,0); vp++; // 3
  vp->Init(-1,-1,-1, -1, 0, 0, 1,1); vp++; // 0
  vp->Init(-1,-1, 1, -1, 0, 0, 0,1); vp++; // 4
  vp->Init(-1, 1, 1, -1, 0, 0, 0,0); vp++; // 7
  Geo->EndLoadVB();

  sU16 *ip=0;
  Geo->BeginLoadIB(6*6,sGD_FRAME,&ip);
  sQuad(ip,0, 0, 1, 2, 3);
  sQuad(ip,0, 4, 5, 6, 7);
  sQuad(ip,0, 8, 9,10,11);
  sQuad(ip,0,12,13,14,15);
  sQuad(ip,0,16,17,18,19);
  sQuad(ip,0,20,21,22,23);
  Geo->EndLoadIB();
}

void RNCubeExample::Simulate(Wz4RenderContext *ctx)
{
  // we animate the parameters, so we have to save them a lot
  Para = ParaBase;

  // bind parameter to scripting engine
  Anim.Bind(ctx->Script,&Para);

  // execute script
  SimulateCalc(ctx);

  // remove binding
//  Anim.UnBind(ctx->Script,&Para);

  // this op does not have any animated childs. 
  // so this call is redundant
  SimulateChilds(ctx);
}

// called per frame

void RNCubeExample::Prepare(Wz4RenderContext *ctx)
{
  // create vertex buffer here per frame.
  // it might be used multiple times.

  MakeCube();

  // this is the right place for  complicated math

  sSRT srt;
  srt.Scale = Para.Scale;
  srt.Rotate = Para.Rot*sPI2F;
  srt.Translate = Para.Trans;
  srt.MakeMatrix(Matrix);

  if(Mtrl) Mtrl->BeforeFrame(Para.LightEnv);
}

// called for each rendering: once for ZOnly, once for MAIN, may be for shadows...

void RNCubeExample::Render(Wz4RenderContext *ctx)
{
  // here we can filter out passes that do not interesst us

  if(ctx->IsCommonRendermode())
  {
    // here the material can decline to render itself

    if(Mtrl->SkipPhase(ctx->RenderMode,Para.LightEnv)) return;

    // the effect may be placed multple times in the render graph.
    // this loop will get all the matrices:

    sMatrix34CM *mat;
    sFORALL(Matrices,mat)
    {
      // here i add our own matrix

      sMatrix34CM mat1 = sMatrix34CM(sMatrix34(*mat)*Matrix);

      // render it once

      Mtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv,&mat1,0,0,0);
      Geo->Draw();
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Print with font                                                    ***/
/***                                                                      ***/
/****************************************************************************/

RNPrint::RNPrint()
{
  Font = 0;
  Anim.Init(Wz4RenderType->Script);
}

RNPrint::~RNPrint()
{
  Font->Release();
}


void RNPrint::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;

  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);
  SimulateChilds(ctx);
}


void RNPrint::Render(Wz4RenderContext *ctx)
{
  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sMatrix34CM *model;
    sViewport view = ctx->View;
    sFORALL(Matrices,model)
    {
      view.Model = sMatrix34(*model);
      view.Prepare();
      Font->Print(view,Text,Para.Color,1.0f/Font->Height);   // hier fehlt die matrix im text op!
    }
 
  }  
}

/****************************************************************************/
/***                                                                      ***/
/***   Ribbons                                                            ***/
/***                                                                      ***/
/****************************************************************************/

RNRibbons::RNRibbons()
{
  Geo = new sGeometry;
  Geo->Init(sGF_TRILIST|sGF_INDEX32,sVertexFormatTangent);
  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_CULLOFF|sMTRL_ZON|sMTRL_LIGHTING;
  Mtrl->Prepare(sVertexFormatStandard);
  MtrlEx = 0;

  Anim.Init(Wz4RenderType->Script);
}

RNRibbons::~RNRibbons()
{
  delete Geo;
  delete Mtrl;
  MtrlEx->Release();
}

/****************************************************************************/

void RNRibbons::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

void RNRibbons::Prepare(Wz4RenderContext *ctx)
{
  sMatrix34 mat;
  sF32 rx,ry,rz;
  sVector31 p;
  sVertexTangent  *vp;

  sInt max = Para.Steps;
  const sF32 scale = 0.001f;

  if(MtrlEx) MtrlEx->BeforeFrame(0);

  Geo->BeginLoadVB(2*max*Para.Around,sGD_FRAME,&vp);
  for(sInt j=0;j<Para.Around;j++)
  {
    sF32 jis = sF32(j)/Para.Around;
    p = Para.Trans;

    sF32 ox = (Para.Spread[0]*jis+Para.Start[0])*sPI2F;
    sF32 oy = (Para.Spread[1]*jis+Para.Start[1])*sPI2F;
    sF32 oz = (Para.Spread[2]*jis+Para.Start[2])*sPI2F;

    for(sInt i=0;i<max;i++)
    {
      rx = sCos(i*Para.Freq[0]*scale+ox)*Para.Waber[0];
      ry = sCos(i*Para.Freq[1]*scale+oy)*Para.Waber[1];
      rz = sCos(i*Para.Freq[2]*scale+oz)*Para.Waber[2];
      mat.EulerXYZ(rx,ry,rz);

      p += mat.k * Para.Forward;
      vp->Init(p-mat.i*Para.Side,-mat.j,0.0f,i+(1/Para.Steps));
      vp->tx = vp->nx * (vp->nx * p.x);
      vp->ty = vp->ny * (vp->ny * p.y);
      vp->tz = vp->nz * (vp->nz * p.z);
      vp++;
      vp->Init(p+mat.i*Para.Side,-mat.j,1.0f,i+(1/Para.Steps));
      vp->tx = vp->nx * (vp->nx * p.x);
      vp->ty = vp->ny * (vp->ny * p.y);
      vp->tz = vp->nz * (vp->nz * p.z);
      vp++;
    }
  }
  Geo->EndLoadVB();

  sU32 *ip;
  Geo->BeginLoadIB(6*(max-1)*Para.Around,sGD_FRAME,&ip);
  for(sInt j=0;j<Para.Around;j++)
  {
    sInt v = j*2*max;
    for(sInt i=0;i<max-1;i++)
      sQuad(ip,v+i*2,0,1,3,2);
  }
  Geo->EndLoadIB();
}

void RNRibbons::Render(Wz4RenderContext *ctx)
{
  sMaterialEnv env;

  if(ctx->IsCommonRendermode())
  {
    if(!MtrlEx)
    {
      env.AmbientColor = 0xff404040;
      env.LightColor[0] = 0xffc0c0c0;
      env.LightColor[1] = 0xffc0c0c0;
      env.LightDir[0].Init(0,1,0);
      env.LightDir[1].Init(0,-1,0);
      env.Fix();
    }
    else
    {
      if(MtrlEx->SkipPhase(ctx->RenderMode,Para.LightEnv)) return;
    }

    sMatrix34CM *mat;
    sFORALL(Matrices,mat)
    {
      sViewport view = ctx->View;
      view.UpdateModelMatrix(sMatrix34(*mat));

      if(!MtrlEx)
      {
        sCBuffer<sSimpleMaterialEnvPara> cb;
        cb.Data->Set(view,env);
        Mtrl->Set(&cb);
      }
      else
        MtrlEx->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv,mat,0,0,0);

      Geo->Draw();
    }
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   Ribbons2                                                            ***/
/***                                                                      ***/
/****************************************************************************/

RNRibbons2::RNRibbons2()
{
  Geo = new sGeometry;
  Geo->Init(sGF_TRILIST|sGF_INDEX32,sVertexFormatTangent);
  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_CULLOFF|sMTRL_ZON|sMTRL_LIGHTING;
  Mtrl->Prepare(sVertexFormatStandard);
  MtrlEx = 0;

  Anim.Init(Wz4RenderType->Script);
}

RNRibbons2::~RNRibbons2()
{
  delete Geo;
  delete Mtrl;
  MtrlEx->Release();
}

/****************************************************************************/

void RNRibbons2::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

void RNRibbons2::Prepare(Wz4RenderContext *ctx)
{
  sVertexTangent *vp;
  sVector31 pos;
  sVector30 speed;
  sVector30 camdir,norm;
  sVector30 dir,d0,d1;
  sVector31 p0,p1,p2,p3;

  if(MtrlEx) MtrlEx->BeforeFrame(0);

  Random.Seed(1);

  sInt maxi = Para.Count;
  sInt maxj = Para.Length;

  Sources[0] = sVector31(Para.Source0);
  Sources[1] = sVector31(Para.Source1);
  Sources[2] = sVector31(Para.Source2);
  Sources[3] = sVector31(Para.Source3);
  Power[0] = Para.Power0;
  Power[1] = Para.Power1;
  Power[2] = Para.Power2;
  Power[3] = Para.Power3;

  sViewport view = ctx->View;
  view.UpdateModelMatrix(sMatrix34(Matrices[0]));
  sMatrix34 mat;
  mat = view.ModelView;
  mat.Invert3();

  dir.Init(1,0,0);
  camdir = mat.k;

  Geo->BeginLoadVB(2*maxi*maxj,sGD_FRAME,&vp);
  for(sInt i=0;i<maxi;i++)
  {
    pos.InitRandom(Random);
    pos = pos * Para.Spread + Para.Trans;
    speed.Init(1,0,0);
    for(sInt j=0;j<maxj;j++)
    {
      for(sInt i=0;i<4;i++)
      {
        sVector30 d = pos-Sources[i];
        sF32 len = sSqrt(d^d);
//        if(len>1)
//          len = 1/len;

        len = 2/(len*len*len*len*len * (sExp(1/len)-1));
        d.Unit();
        speed = speed + d*len*Power[i];
      }
      speed.Unit();

      d0.Cross(camdir,speed);
      d0.Unit();

      d0 *= Para.Side;

      pos += speed*Para.Forward;
      norm.Cross(d0, speed);
      norm.Unit();

      vp[0].Init(pos-d0,norm,0.0f,j+(1/Para.Length));
      vp->tx = vp->nx * (vp->nx * pos.x);
      vp->ty = vp->ny * (vp->ny * pos.y);
      vp->tz = vp->nz * (vp->nz * pos.z);
      vp[1].Init(pos+d0,norm,1.0f,j+(1/Para.Length));
      vp->tx = vp->nx * (vp->nx * pos.x);
      vp->ty = vp->ny * (vp->ny * pos.y);
      vp->tz = vp->nz * (vp->nz * pos.z);
      vp+=2;
    }
  }
  Geo->EndLoadVB();

  sU32 *ip;
  sInt n = 0;
  Geo->BeginLoadIB(6*maxi*(maxj-1),sGD_FRAME,&ip);
  for(sInt i=0;i<maxi;i++)
  {
    for(sInt j=0;j<maxj-1;j++)
    {
      sQuad(ip,n,0,1,3,2);
      n+=2;
    }
    n+=2;
  }

  Geo->EndLoadIB();
}

void RNRibbons2::Render(Wz4RenderContext *ctx)
{
  sMaterialEnv env;

  if(ctx->IsCommonRendermode())
  {
    if(!MtrlEx)
    {
      env.AmbientColor = 0xff404040;
      env.LightColor[0] = 0xffc0c0c0;
      env.LightColor[1] = 0xffc0c0c0;
      env.LightDir[0].Init(0,1,0);
      env.LightDir[1].Init(0,-1,0);
      env.Fix();
    }
    else
    {
      if(MtrlEx->SkipPhase(ctx->RenderMode,Para.LightEnv)) return;
    }

    sMatrix34CM *mat;
    sFORALL(Matrices,mat)
    {
      sViewport view = ctx->View;
      view.UpdateModelMatrix(sMatrix34(*mat));

      if(!MtrlEx)
      {
        sCBuffer<sSimpleMaterialEnvPara> cb;
        cb.Data->Set(view,env);
        Mtrl->Set(&cb);
      }
      else
        MtrlEx->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv,mat,0,0,0);

      Geo->Draw();
    }
  }
}

void RNRibbons2::Eval(const sVector31 &pos,sVector30 &norm)
{
  sVector30 rad;
  rad.Init(0,0,0);
  for(sInt i=0;i<4;i++)
  {
    sVector30 d = pos-Sources[i];
    sF32 dist = sFSqrt(d^d);
    sF32 disti = 1/(dist+0.001);
    rad.x += d.x*disti;
    rad.y += d.y*disti;
    rad.z += d.z*disti;
  }
  rad.Unit();
  norm = rad;
}


/*

        float4 dist = posx*posx+posy*posy;
        dist = sqrt(dist);
        float4 disti = 1/(dist+0.001);
        float iso = 1/(dot(disti,1));
        float4 radx = posx * disti;
        float4 rady = posy * disti;
        float rad = atan2(dot(radx,1),dot(rady,1));
        
        float rx = rad/3.141*16+super.x;
        iso = 1/(sqrt(iso)+1.5);
        float ry = iso*64+super.y;
        result = tex2D(s0,float2(ry,rx));

*/


/****************************************************************************/
/***                                                                      ***/
/***  BlowNoise                                                           ***/
/***                                                                      ***/
/****************************************************************************/

struct BlowNoiseVertex // 44 bytes
{
  sVector31 Pos;
  sVector30 Normal;
  sVector30 Tangent;
  sF32 U,V;
};

RNBlowNoise::RNBlowNoise()
{
  static const sU32 desc[] = { sVF_POSITION,sVF_NORMAL,sVF_TANGENT|sVF_F3,sVF_UV0,0 };

  VertFormat = sCreateVertexFormat(desc);
  Geo = new sGeometry;
  Geo->Init(sGF_TRILIST|sGF_INDEX32,VertFormat);

  Mtrl = 0;
  Verts = 0;

  Anim.Init(Wz4RenderType->Script);
}

RNBlowNoise::~RNBlowNoise()
{
  delete Geo;
  delete[] Verts;
  Mtrl->Release();
  sDeleteAll(Layers);
}

void RNBlowNoise::Init()
{
  SizeX = ParaBase.Tess[0];
  SizeY = ParaBase.Tess[1];
  Verts = new Vertex[SizeX*SizeY];
  InitNoise();
}

/****************************************************************************/

void RNBlowNoise::InitNoise()
{
  sInt repeats = 4;
  for(sInt y=0;y<BNN;y++)
    for(sInt x=0;x<BNN;x++)
      Noise[y][x].Value = sPerlin2D(x<<(16-BNNB+repeats),y<<(16-BNNB+repeats),(1<<repeats)-1,0);
  for(sInt y=0;y<BNN;y++)
  {
    for(sInt x=0;x<BNN;x++)
    {
      Noise[y][x].DX = (Noise[y][(x-1)&BNNM].Value - Noise[y][(x+1)&BNNM].Value)*0.5f;
      Noise[y][x].DY = (Noise[(y-1)&BNNM][x].Value - Noise[(y+1)&BNNM][x].Value)*0.5f;
    }
  }
}

void RNBlowNoise::SampleNoise(sF32 x,sF32 y,NoiseData &nd)
{
  sInt ix = sInt(x*BNN*65536);
  sInt iy = sInt(y*BNN*65536);
  sF32 fx = (ix&65535)/65536.0f;
  sF32 fy = (iy&65535)/65536.0f;
  ix = (ix>>16)&BNNM;
  iy = (iy>>16)&BNNM;
  NoiseData *nd00 = &Noise[(iy+0)&BNNM][(ix+0)&BNNM];
  NoiseData *nd01 = &Noise[(iy+0)&BNNM][(ix+1)&BNNM];
  NoiseData *nd10 = &Noise[(iy+1)&BNNM][(ix+0)&BNNM];
  NoiseData *nd11 = &Noise[(iy+1)&BNNM][(ix+1)&BNNM];
  sF32 f00 = (1-fy)*(1-fx);
  sF32 f01 = (1-fy)*(  fx);
  sF32 f10 = (  fy)*(1-fx);
  sF32 f11 = (  fy)*(  fx);

  nd.Value = nd00->Value*f00 + nd01->Value*f01 + nd10->Value*f10 + nd11->Value*f11;
  nd.DX    = nd00->DX   *f00 + nd01->DX   *f01 + nd10->DX   *f10 + nd11->DX   *f11;
  nd.DY    = nd00->DY   *f00 + nd01->DY   *f01 + nd10->DY   *f10 + nd11->DY   *f11;
}

/****************************************************************************/


void RNBlowNoise::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

void RNBlowNoise::Prepare(Wz4RenderContext *ctx)
{
  NoiseData nd;

  sVector31 p00,p01,p10,p11;
  sVector30 dx,dy,dz;

  if(Mtrl) Mtrl->BeforeFrame(Para.LightEnv);

  // perputate

  Vertex *v = Verts;
  for(sInt y=0;y<SizeY;y++)
  {
    sF32 fv = y/sF32(SizeY-1);
    sF32 fy = (fv-0.5f)*Para.Scale;
    for(sInt x=0;x<SizeX;x++)
    {
      sF32 fu = x/sF32(SizeX-1);
      sF32 fx = (fu-0.5f)*Para.Scale;
//      SampleNoise(fu,fv,nd);
      v->Pos.x = fx;//+nd.DX*Para->p2;
      v->Pos.y = 0;//+nd.Value*Para->p0;
      v->Pos.z = fy;//+nd.DY*Para->p2;
      v->U = fu;
      v->V = fv;
      v->Normal.Init(0,1,0);

      v++;
    }
  }

  // perputate

  sF32 time = Para.Scroll;
  Wz4RenderArrayBlowNoise *ar;
  sFORALL(Layers,ar)
  {
    if(ar->Flags&1)
    {
      for(sInt i=0;i<SizeX*SizeY;i++)
        Verts[i].PosOld = Verts[i].Pos;

      v = Verts;
      sF32 octscale = 0.125f * (1<<ar->Octave);
      sF32 sx = 0;
      sF32 sy = ar->Scroll[0]+ar->Scroll[1]*time;
      for(sInt y=0;y<SizeY;y++)
      {
        sInt ym = sMax(y-1,0);
        sInt yp = sMin(y+1,SizeY-1);
        for(sInt x=0;x<SizeX;x++)
        {
          sInt xm = sMax(x-1,0);
          sInt xp = sMin(x+1,SizeX-1);
          p00 = Verts[y *SizeX+xm].PosOld;
          p01 = Verts[y *SizeX+xp].PosOld;
          p10 = Verts[ym*SizeX+x ].PosOld;
          p11 = Verts[yp*SizeX+x ].PosOld;
          dx = p00-p01; dx.Unit();
          dz = p10-p11; dz.Unit();
          dy.Cross(dz,dx); dy.Unit();


          SampleNoise(v->U*octscale+sx,v->V*octscale+sy,nd);
          v->Pos = v->PosOld 
                 - dx*nd.DX*ar->Blow
                 + dy*nd.Value*ar->Depth
                 - dz*nd.DY*ar->Blow;

          v++;
        }
      }
    }
  }


  // calc normals and tangents

  for(sInt y=1;y<SizeY-1;y++)
  {
    for(sInt x=1;x<SizeX-1;x++)
    {
      v = Verts+x+y*SizeX;

      p00 = v[-1].Pos;
      p01 = v[ 1].Pos;
      p10 = v[-SizeX].Pos;
      p11 = v[ SizeX].Pos;
      dx = p00-p01;
      dy = p10-p11;
      v->Normal.Cross(dy,dx);
      v->Normal.Unit();
      v->Tangent = dx - v->Normal * (v->Normal * dx);
      v->Tangent.Unit();
    }
  }

  // load vb

  BlowNoiseVertex *vp;
  sU32 *ip;

  v = Verts;
  Geo->BeginLoadVB(SizeX*SizeY,sGD_FRAME,&vp);
  for(sInt i=0;i<SizeX*SizeY;i++)
  {
    vp->Pos = v->Pos;
    vp->Normal = v->Normal;
    vp->Tangent = v->Tangent;
    vp->U = v->U;
    vp->V = v->V;
    vp++;
    v++;
  }
  Geo->EndLoadVB();

  Geo->BeginLoadIB(6*(SizeX-1)*(SizeY-1),sGD_FRAME,&ip);
  for(sInt y=0;y<SizeY-1;y++)
    for(sInt x=0;x<SizeX-1;x++)
      sQuad(ip,x+y*SizeX,1,0,SizeX,SizeX+1);
  Geo->EndLoadIB();
}

void RNBlowNoise::Render(Wz4RenderContext *ctx)
{
  if(ctx->IsCommonRendermode())
  {
    if(Mtrl->SkipPhase(ctx->RenderMode,Para.LightEnv)) return;
    sMatrix34CM *mat;
    sFORALL(Matrices,mat)
    {
      Mtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv,mat,0,0,0);
      Geo->Draw();
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Classic Debris Style Chunk Motion                                  ***/
/***                                                                      ***/
/****************************************************************************/

RNDebrisClassic::RNDebrisClassic()
{
  Anim.Init(Wz4RenderType->Script);
}

RNDebrisClassic::~RNDebrisClassic()
{
  Mesh->Release();
}

void RNDebrisClassic::Init()
{
  Para = ParaBase;
}

/****************************************************************************/

void RNDebrisClassic::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);
}

void RNDebrisClassic::Render(Wz4RenderContext *ctx)
{
  sInt max = Mesh->Chunks.GetCount();
  if(Para.Mode&1)
    max = Para.Count1*Para.Count2*Para.Count3;
  if(max==0)
    return;
  sF32 u2,v2;
  sVector31 uv;
  sMatrix34CM *mats = new sMatrix34CM[max];
  sSRT srt;
  sMatrix34 mat;

  // gather parameters

  sMatrix34 proj[3];
  sInt flags[3];
  sInt maxx[3];
  sInt maxy[3];
  sInt scrollu[3];
  sInt scrollv[3];
  sF32 displace[3];
  sF32 tumble[3];
  sF32 scale[3];
  sF32 texbias[3];

  flags[0] = Para.Flags & ~0x300;
  flags[1] = Para.FlagsB;
  flags[2] = Para.FlagsC;
  displace[0] = Para.Displace;
  displace[1] = Para.DisplaceB;
  displace[2] = Para.DisplaceC;
  tumble[0] = Para.Tumble;
  tumble[1] = Para.TumbleB;
  tumble[2] = Para.TumbleC;
  scale[0] = Para.Size;
  scale[1] = Para.SizeB;
  scale[2] = Para.SizeC;
  texbias[0] = Para.TexBias;
  texbias[1] = Para.TexBiasB;
  texbias[2] = Para.TexBiasC;
  srt.Scale.Init(Para.Scale[0],Para.Scale[1],1);
  srt.Translate.Init(Para.Trans[0],Para.Trans[1],0);
  srt.Rotate.Init(0,0,Para.Rotate*sPI2F);
  srt.MakeMatrixInv(proj[0]);
  srt.Scale.Init(Para.ScaleB[0],Para.ScaleB[1],1);
  srt.Translate.Init(Para.TransB[0],Para.TransB[1],0);
  srt.Rotate.Init(0,0,Para.RotateB*sPI2F);
  srt.MakeMatrixInv(proj[1]);
  srt.Scale.Init(Para.ScaleC[0],Para.ScaleC[1],1);
  srt.Translate.Init(Para.TransC[0],Para.TransC[1],0);
  srt.Rotate.Init(0,0,Para.RotateC*sPI2F);
  srt.MakeMatrixInv(proj[2]);

  for(sInt i=0;i<3;i++)
  {
    switch(flags[i] & 3)
    {
    case 1:
      sSwap(proj[i].j,proj[i].k);
      sSwap(proj[i].i,proj[i].j);
      break;
    case 2:
      sSwap(proj[i].j,proj[i].k);
      break;
    }

    maxx[i] = 1;
    maxy[i] = 1;
    if(Bitmap[i].SizeX>0)
    {
      maxx[i] = 256*Bitmap[i].SizeX;
      maxy[i] = 256*Bitmap[i].SizeY;
    }
  }
  scrollu[0] = sInt(Para.Scroll [0]*maxx[0]) & (maxx[0]-1);
  scrollv[0] = sInt(Para.Scroll [1]*maxy[0]) & (maxy[0]-1);
  scrollu[1] = sInt(Para.ScrollB[0]*maxx[1]) & (maxx[1]-1);
  scrollv[1] = sInt(Para.ScrollB[1]*maxy[1]) & (maxy[1]-1);
  scrollu[2] = sInt(Para.ScrollC[0]*maxx[2]) & (maxx[2]-1);
  scrollv[2] = sInt(Para.ScrollC[1]*maxy[2]) & (maxy[2]-1);

  sVector31 p;
  sVector30 normal = Para.Normal;
  normal.Unit();
  sVector30 random(0,0,0);
  for(sInt i=0;i<max;i++)
  {
    if(Para.Mode & 1)
    {
      sInt t1 = i % Para.Count1;
      sInt t2 = i / Para.Count1 % Para.Count2;
      sInt t3 = i / Para.Count1 / Para.Count2 % Para.Count3;

      p = Para.Trans0 + Para.Trans1*t1 + Para.Trans2*t2 + Para.Trans3*t3;

    }
    else
    {
      Wz4ChunkPhysics *ch = &Mesh->Chunks[i];
      p = ch->COM;
      normal = ch->Normal;
      random = ch->Random;
    }

    sF32 d = 0;
    sF32 t = 0;
    sF32 s = 0;
    for(sInt j=0;j<3;j++)
    {
      if(Bitmap[j].SizeX>0 && (flags[j]&8))
      {
        uv = p*proj[j];

        if(flags[j] & 4)
        {
          u2 = sATan2(uv.x,uv.y)/sPI2F+0.5f;
          v2 = sSqrt(uv.x*uv.x+uv.y*uv.y);
        }
        else
        {
          u2 = uv.x*0.5f+0.5f;
          v2 = uv.y*0.5f+0.5f;
        }

        sInt u = sInt(u2*maxx[j]);
        sInt v = sInt(v2*maxy[j]);

        sU32 col = 0;
        switch(flags[j] & 0x30)
        {
        default:  // border
          if(u>=0 && v>=0 && u<maxx[j] && v<maxy[j])
            col = Bitmap[j].Filter(u+scrollu[j],v+scrollv[j],1);
          break;
        case 0x10:
          u &= maxx[j]-1;
          v &= maxy[j]-1;
          col = Bitmap[j].Filter(u+scrollu[j],v+scrollv[j],1);
          break;
        case 0x20:
          u = sClamp(u,0,maxx[j]-1);
          v = sClamp(v,0,maxy[j]-1);
          col = Bitmap[j].Filter(u+scrollu[j],v+scrollv[j],1);
          break;
        }

        sF32 val = col/65535.0f;
        val = sMax<sF32>(0,val+texbias[j]);
        if(flags[j]&0x80)
          val = val*2-1;
        switch(flags[j]&0x300)
        {
        case 0x000:
          d += val*displace[j];
          t += val*tumble[j];
          s += val*scale[j];
          break;
        case 0x100:
          d *= val*displace[j];
          t *= val*tumble[j];
          s *= val*scale[j];
          break;
        case 0x200:
          d = sMin(d,val*displace[j]);
          t = sMin(t,val*tumble[j]);
          s = sMin(s,val*scale[j]);
          break;
        case 0x300:
          d = sMax(d,val*displace[j]);
          t = sMax(t,val*tumble[j]);
          s = sMax(s,val*scale[j]);
          break;
        }
      }
    }

    s = sPow(2,s);
    mat.RotateAxis(random,t*sPI2F);
    mat.i *= s;
    mat.j *= s;
    mat.k *= s;
    mat.l.Init(0,0,0);
    if(!(Para.Mode&1))
      mat.l = -p*mat;
    mat.l += sVector30(p) + d*normal;
//    mat.l = sVector31(sVector30(mat.l)*1/s);
    mats[i] = mat;
  }

  sMatrix34CM *mats0 = new sMatrix34CM[max];
  sMatrix34CM *matp;

  sFORALL(Matrices,matp)
  {
    for(sInt i=0;i<max;i++)
      mats0[i] = mats[i]*(*matp);

    if(Para.Mode&1)
    {
      Mesh->RenderInst(ctx->RenderMode,Para.LightEnv,max,mats0);
    }
    else
    {
      Mesh->RenderBone(ctx->RenderMode,Para.LightEnv,max,mats0,max);
    }
  }

  delete[] mats;
  delete[] mats0;
}

/****************************************************************************/
/***                                                                      ***/
/***   BoneVibrate - Wabern von skeletten                                 ***/
/***                                                                      ***/
/****************************************************************************/

RNBoneVibrate::RNBoneVibrate()
{
  Anim.Init(Wz4RenderType->Script);
  mats = 0;
  mate = 0;
  mats0 = 0;
  Count = 0;
}

RNBoneVibrate::~RNBoneVibrate()
{
  Mesh->Release();
  delete[] mats;
  delete[] mate;
  delete[] mats0;
}


void RNBoneVibrate::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);
}

void RNBoneVibrate::Init()
{
  if(Mesh->Skeleton)
  {
    Count = Mesh->Skeleton->Joints.GetCount();
    mate = new sMatrix34[Count];
    mats = new sMatrix34[Count];
    mats0 = new sMatrix34CM[Count];
  }
}

void RNBoneVibrate::Prepare(Wz4RenderContext *ctx)
{
  if(Mesh->Skeleton && Mesh->Skeleton->Joints.GetCount()==Count)
  {
    sInt max = Count;

    Mesh->BeforeFrame(Para.LightEnv);
    Mesh->Skeleton->Evaluate(0,mate,mats);
    sRandom rnd;
    rnd.Seed(Para.Seed);

    for(sInt i=Para.FirstBone;i<max;i++)
    {
      sMatrix34 mat;
      sVector30 n;
      n.InitRandom(rnd);
      n.Unit();

      mat.RotateAxis(n,sFSin((i*Para.Freq+Para.Phase)*sPI2F)*Para.Rot*sPI2F);
      mat.l.y += sFSin((i*Para.Freq+Para.Phase)*sPI2F)*Para.Trans;
      mats[i] = mats[i] * mat;
    }
  }
}

void RNBoneVibrate::Render(Wz4RenderContext *ctx)
{
  if(Mesh->Skeleton && Mesh->Skeleton->Joints.GetCount()==Count)
  {
    sInt max = Count;

    sMatrix34CM *matp;

    sFORALL(Matrices,matp)
    {
      for(sInt i=0;i<max;i++)
        mats0[i] = sMatrix34CM(mats[i])*(*matp);

      Mesh->RenderBone(ctx->RenderMode,Para.LightEnv,max,mats0,max);
    }
  }
}

/****************************************************************************/
