/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "tron.hpp"
#include "wz4frlib/tron_ops.hpp"
#include "base/graphics.hpp"
#include "wz4frlib/wz4_ipp_shader.hpp"
#include "wz4frlib/wz4_ipp.hpp"
#include "wz4frlib/fxparticle_shader.hpp"
#include "tron_shader.hpp"

#define COMPILE_FR033 1
/****************************************************************************/

TronPOM::TronPOM()
{
  Source = 0;
  first = true;
  Anim.Init(Wz4RenderType->Script);
}

TronPOM::~TronPOM()
{
  Source->Release();
  Mesh->Release();
}

Wz4BSPError TronPOM::Init(sF32 planeThickness)
{
  Para = ParaBase;  
  return bsp.FromMesh(Mesh,planeThickness);
}


sInt TronPOM::GetPartCount()
{
  return Source->GetPartCount();
}

sInt TronPOM::GetPartFlags()
{
  return 0;
}

void TronPOM::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  Source->Simulate(ctx);
}

void TronPOM::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sInt i;
  if (first || Para.Mode)
  {
    Source->Func(pinfo,time,dt);
    Wz4Particle *part = pinfo.Parts;

    sMatrix34 mat;
    mat.EulerXYZ(Para.Direction.x,Para.Direction.y,Para.Direction.z);  
    sVector30 d(0.0,-1.0f,0.0f);
    d=d*mat;  

    for(i=0;i<pinfo.Alloc;i++)
    {
      if(part->Time>=0)
      { 
        sRay ray;
        sF32 tMin=0.0f;
        sF32 tMax=Para.Length;
        sF32 tHit;
        sVector30 hitNormal;
        sVector31 pos;
        part->Get(pos);

        ray.Dir=d;
        ray.Start=pos;
        if (bsp.TraceRay(ray,tMin,tMax,tHit,hitNormal))
        {
          pos+=d*tHit;
        }
        part->Pos = pos;
      }
      part++;
    }
    first=false;
  }
}

void TronPOM::Handles(wPaintInfo &pi, Wz4ParticlesParaPlaceOnMesh *para, wOp *op)
{
//  Wz4Render *rn = (Wz4Render *)op->RefObj;
//  TronPOM *o=(TronPOM *)rn->RootNode;
  sVector30 a;
  sMatrix34 mat;
  mat.EulerXYZ(para->Direction.x,para->Direction.y,para->Direction.z);
  sVector31 d(0.0,-para->Length,0.0f);
  d=d*mat;
  pi.Line3D(d,sVector31(0,0,0),0xffff0000,true);
}

/****************************************************************************/

TronSFF::TronSFF()
{
  Source = 0;
  Anim.Init(Wz4RenderType->Script);
}

TronSFF::~TronSFF()
{
  Source->Release();
}

void TronSFF::Init()
{
  Para = ParaBase;
}


sInt TronSFF::GetPartCount()
{
  return Source->GetPartCount();
}

sInt TronSFF::GetPartFlags()
{
  return Source->GetPartCount();
}

void TronSFF::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  Source->Simulate(ctx);
}

void TronSFF::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sInt i;
  Source->Func(pinfo,time,dt);

  ScaleX = 1.0f / Para.Size.x;
  ScaleY = 1.0f / Para.Size.y;
  ScaleZ = 1.0f / Para.Size.z;
  
  sVector30 pn;
  sMatrix34 mat;
  mat.EulerXYZ(Para.Rotation.x, Para.Rotation.y, Para.Rotation.z);
  pn.Init(0.0f,1.0f,0.0f);  
  
  pn = pn * mat;

  Plane.InitPlane(Para.Position, pn);

  Wz4Particle *part = pinfo.Parts;
  for(i=0;i<pinfo.Alloc;i++)
  {
    if(part->Time>=0)
    { 
      sVector31 pos;
      part->Get(pos);

      switch(Para.FieldType)
      {
        //Plane
        case 1 :
        {
          sF32 d = GetPlaneDistance(pos);
          sF32 f = CalcDistractionFactor(d);                    
          pos = pos + pn * f;
        }
        break;

        //Ellipsoid
        default :
        case 0 :
        {
          sF32 d = GetEllipsoidDistance(pos);
          sF32 f = CalcDistractionFactor(d);
          sVector30 n = pos - Para.Position;
          n.Unit();
          pos = pos + n * f;
        }
        break;      
      }

      part->Pos = pos;
    }
    part++;
  }
}

// Retexturizer

//
static void GCPPIT(const sVector31 &_a, const sVector31 &_b, const sVector31 &_c, const sVector31 &_p, sF32 &_ta, sF32 &_tb , sF32 &_tc)
{
  sVector30 ab = _b - _a;
  sVector30 ac = _c - _a;
  sVector30 ap = _p - _a;
  

  sF32 d1 = ab ^ ap;
  sF32 d2 = ac ^ ap;

  sF32 one = 1.0f;// - sEPSILON;
  _ta = 0.0f;
  _tb = 0.0f;
  _tc = 0.0f;

  if (d1 <= 0.0f && d2<= 0.0f)
  {
    _ta = one;
    //_r = (sVector31)(_a-_p);
    return;
  }


  sVector30 bp = _p - _b;

  sF32 d3 = ab ^ bp;
  sF32 d4 = ac ^ bp;

  if (d3>= 0.0f && d4<= d3) 
  {
    _tb = one;
    //_r = (sVector31)(_b-_p);
    return;
  }


  sF32 vc = d1*d4 - d3*d2;

  if (vc<=0.0f && d1 >= 0.0f && d3 <= 0.0f)
  {
    sF32 v = d1 / (d1-d3);
    _ta = one - v;
    _tb = v;
    //_r = (sVector31)(_a + v * ab - _p);
    return;
  }

  sVector30 cp = _p - _c;
  sF32 d5 = ab ^ cp;
  sF32 d6 = ac ^ cp;
  if (d6 >= 0.0f && d5 <= d6) 
  {
    _tc = one;
//    _r = (sVector31)(_c-_p);
    return;
  }


  sF32 vb = d5*d2 - d1*d6;
  if (vb <= 0.0f &&  d2 >= 0.0f && d6 <= 0.0f)
  {
    sF32 w = d2 /(d2-d6);
    _ta = one - w;
    _tc = w;
//    _r = (sVector31)(_a + w * ac - _p);
    return;
  }
  
  float va = d3*d6 - d5*d4;
  if (va <= 0.0f && (d4-d3) >= 0.0f && (d5-d6) >= 0.0f)
  {
    sF32 w = (d4-d3) / ((d4-d3) + (d5-d6));
   _tb = one - w;
   _tc = w;
    //_r = (sVector31)(_b + w * (_c - _b ) - _p);
    return;
  }

  sF32 denom = 1.0f / (va+vb+vc);
  sF32 v = vb * denom;
  sF32 w = vc * denom;

  _ta = one - v - w;
  _tb = v;
  _tc = w;

//  _r = (sVector31)(_a + (v * ab) + (w * ac)  - _p);
  return;
}


void TronRTR(Wz4Mesh *out, Wz4Mesh *in1, Wz4Mesh *in2)
{
  sVector31 BoxPos;
  sVector30 BoxDimH;
  sVector30 n(1.0f,0,0);

  tAABBoxOctree *oct = new tAABBoxOctree(4);
  out->CopyFrom(in1);

  Wz4MeshVertex *vp;
  Wz4MeshFace   *fp;

  sFORALL(in2->Vertices,vp)
  {    	
    oct->AddVertex(vp->Pos,n);  
  }

  oct->FinishVertices(0,false,false,BoxPos,BoxDimH);

  sFORALL(in2->Faces,fp)
  {
    if (fp->Count==3)
    {
      oct->AddTriangle(fp->Vertex[0], fp->Vertex[1], fp->Vertex[2], n);
    }
    
    if (fp->Count==4)
    {
      oct->AddTriangle(fp->Vertex[0], fp->Vertex[1], fp->Vertex[2], n);
      oct->AddTriangle(fp->Vertex[0], fp->Vertex[2], fp->Vertex[3], n);
    }   
  }

  oct->Finalize();

  
  sFORALL(out->Vertices,vp)
  {    
    sVector30 n;
    unsigned int id=~0;

    vp->U0     = 0.0f;
    vp->V0     = 0.0f;
    vp->U1     = 0.0f;
    vp->V1     = 0.0f;
#if !WZ4MESH_LOWMEM
    vp->Color0 = 0xff00ff00;
    vp->Color1 = 0xffffffff;
#endif
   /* sF32 d =*/ oct->GetClosestDistance(vp->Pos, &id, 1);

    if (id!=(~0))
    {      
   //   int j = 0;
#if !WZ4MESH_LOWMEM
      vp->Color0 = 0xffffffff;
#endif
      sVector31 a = oct->vertices[oct->tris[id].i1].v;
      sVector31 b = oct->vertices[oct->tris[id].i2].v;
      sVector31 c = oct->vertices[oct->tris[id].i3].v;
      sF32 t0;
      sF32 t1;
      sF32 t2;
    
      GCPPIT(a,b,c,vp->Pos,t0,t1,t2);

      sF32 u0 = in2->Vertices[oct->tris[id].i1].U0; 
      sF32 v0 = in2->Vertices[oct->tris[id].i1].V0; 
      sF32 u1 = in2->Vertices[oct->tris[id].i2].U0; 
      sF32 v1 = in2->Vertices[oct->tris[id].i2].V0; 
      sF32 u2 = in2->Vertices[oct->tris[id].i3].U0; 
      sF32 v2 = in2->Vertices[oct->tris[id].i3].V0; 


      //vp->U0 = u0*t0 + u1*t1 + u2*t2;
      //vp->V0 = v0*t0 + v1*t1 + v2*t2;

     
      vp->U0 = u0*t0 + u1*t1 + u2*t2;
      vp->V0 = v0*t0 + v1*t1 + v2*t2;
    }

//    *vp = in1->Vertices[j++]; //oct->tris[j].i1
    
//    vp->U0 = vp->Normal.x;
//    vp->V0 = vp->Normal.x;
  }
    
  out->Flush();
  out->CalcNormals();
  out->CalcTangents();
  delete oct;
}



/****************************************************************************/

RNSharpen::RNSharpen()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = new tSharpen4Material();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatBasic);
}

RNSharpen::~RNSharpen()
{
  delete Mtrl;
}

void RNSharpen::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNSharpen::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sRTMan->SetTarget(dest);

    sCBuffer<tSharpen4MaterialVSPara> cbv;
    sCBuffer<tSharpen4MaterialPSPara> cbp;

    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbp.Data->rh.x = 1.0f/ctx->ScreenY;
    cbp.Data->rw.x = 1.0f/ctx->ScreenX;
    
    int y,x;
    
    float sum = 0;
    int s = 4;    

    float op[4*4];

    for (y=0;y<=(s/2);y++)
    {
      for (x=0;x<=(s/2);x++)
      {
        float v= -((x+1)*(y+1)/(float)s)*20;
        op[y*s+x]= v;
        op[y*s+s-x-1]= v;
        op[(s-y-1)*s+x]= v;
        op[(s-y-1)*s+s-x-1]= v;
      }  
    }
    
    op[s*(s/2)+s/2]= 0;

    for (y=0;y<s;y++)
    {
      for (x=0;x<s;x++)
      {
        sum+=op[y*s+x];
      }  
    }
    
    op[s*(s/2)+s/2]= (sAbs(sum))*Para.Strength/10;
 
    sum+=op[s*(s/2)+s/2];
    sum= 1.0f/sum;

    for (y=0;y<4;y++)
    {
      for (x=0;x<4;x++)
      {
        cbp.Data->factors[y*4+x].x = op[y*4+x]*sum;
        cbp.Data->factors[y*4+x].y = -10.0f;
        cbp.Data->factors[y*4+x].z = -10.0f;
        cbp.Data->factors[y*4+x].w = -10.0f;
      }
    }


    Mtrl->Texture[0] = src;
    Mtrl->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl->Texture[0] = 0;

    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}



/****************************************************************************/

RNSBlur::RNSBlur()
{
  Anim.Init(Wz4RenderType->Script);
  Geo = new sGeometry(sGF_QUADLIST,sVertexFormatSingle);
  
  Mtrl_STL = new tSTLMaterial();
  Mtrl_STL->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl_STL->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl_STL->Prepare(sVertexFormatBasic);

  Mtrl_LTS = new tLTSMaterial();
  Mtrl_LTS->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl_LTS->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl_LTS->Prepare(sVertexFormatBasic);
}

RNSBlur::~RNSBlur()
{
  delete Geo;
  delete Mtrl_LTS;
  delete Mtrl_STL;
}

void RNSBlur::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNSBlur::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer < tLTSMaterialVSPara > cbltsv;
    sCBuffer < tLTSMaterialPSPara > cbltsp;
    sCBuffer < tSTLMaterialVSPara > cbstlv;
    sCBuffer < tSTLMaterialPSPara > cbstlp;

    sInt sx = ctx->ScreenX;
    sInt sy = ctx->ScreenY;
    
    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();    

    sTexture2D *blur0 = sRTMan->Acquire(sx,sy,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB32F);
    sTexture2D *blur1 = sRTMan->Acquire(sx,sy,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB32F);
        
    sRTMan->SetTarget(blur0);

    ctx->IppHelper->GetTargetInfo(cbltsv.Data->mvp);
    ctx->IppHelper->GetTargetInfo(cbstlv.Data->mvp);

    cbltsp.Data->scale.x = Para.Intensity;
    cbstlp.Data->ivexpo.x = 1.0f / Para.Intensity; 

    //LTS
    Mtrl_LTS->Texture[0] = src;
    Mtrl_LTS->Set(&cbltsv,&cbltsp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl_LTS->Texture[0] = 0;
    
    sRTMan->SetTarget(blur1);

    ctx->IppHelper->Blur(blur1,blur0,Para.Radius,Para.Amp);//,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB32F);

    //STL    
    sRTMan->SetTarget(dest);
    Mtrl_STL->Texture[0] = blur1;
    Mtrl_STL->Set(&cbstlv,&cbstlp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl_STL->Texture[0] = 0;

//    sRTMan->Release(blur0);
    sRTMan->Release(blur1);
    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}


/****************************************************************************/

RNFocusBlur::RNFocusBlur()
{
  Anim.Init(Wz4RenderType->Script);
  Geo = new sGeometry(sGF_QUADLIST,sVertexFormatSingle);
  
  Mtrl = new tFocusBlurMat();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->TFlags[1] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;  
  Mtrl->Prepare(sVertexFormatBasic);
}

RNFocusBlur::~RNFocusBlur()
{
  delete Geo;
  delete Mtrl;  
}

void RNFocusBlur::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNFocusBlur::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sTexture2D *blur = sRTMan->Acquire(src->SizeX,src->SizeY,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB8888);    
    
    sCBuffer<tFocusBlurMatVSPara> cbv;
    sCBuffer<tFocusBlurMatPSPara> cbp;

    //Blur Screen
    sRTMan->SetTarget(blur);  
    sRTMan->AddRef(src);
    ctx->IppHelper->Blur(blur, src, Para.BlurScreenRadius*(src->SizeY/1080.0f), Para.BlurScreenAmp);

    sRTMan->SetTarget(dest);  
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();

    cbp.Data->focuspoint.x=Para.FocusPoint[0]/100.0f;
    cbp.Data->focuspoint.y=Para.FocusPoint[1]/100.0f;
    cbp.Data->focuspoint.z=Para.FocusAmp;
    cbp.Data->focuspoint.w=Para.FocusBias;

    cbp.Modify();
    Mtrl->Texture[0] = blur;
    Mtrl->Texture[1] = src;
    Mtrl->Set(&cbv,&cbp);

    ctx->IppHelper->DrawQuad(dest,blur);

    sRTMan->Release(blur);    
    sRTMan->Release(src);
    sRTMan->Release(dest);

    /*
    sCBuffer < tLTSMaterialVSPara > cbltsv;
    sCBuffer < tLTSMaterialPSPara > cbltsp;
    sCBuffer < tSTLMaterialVSPara > cbstlv;
    sCBuffer < tSTLMaterialPSPara > cbstlp;

    sInt sx = ctx->ScreenX;
    sInt sy = ctx->ScreenY;
    
    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();    

    sTexture2D *blur0 = sRTMan->Acquire(sx,sy,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB32F);
    sTexture2D *blur1 = sRTMan->Acquire(sx,sy,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB32F);
        
    sRTMan->SetTarget(blur0);

    ctx->IppHelper->GetTargetInfo(cbltsv.Data->mvp);
    ctx->IppHelper->GetTargetInfo(cbstlv.Data->mvp);

    cbltsp.Data->scale.x = Para.Intensity;
    cbstlp.Data->ivexpo.x = 1.0f / Para.Intensity; 

    //LTS
    Mtrl_LTS->Texture[0] = src;
    Mtrl_LTS->Set(&cbltsv,&cbltsp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl_LTS->Texture[0] = 0;
    
    sRTMan->SetTarget(blur1);

    ctx->IppHelper->Blur(blur1,blur0,Para.Radius,Para.Amp);//,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB32F);

    //STL    
    sRTMan->SetTarget(dest);
    Mtrl_STL->Texture[0] = blur1;
    Mtrl_STL->Set(&cbstlv,&cbstlp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl_STL->Texture[0] = 0;

//    sRTMan->Release(blur0);
    sRTMan->Release(blur1);
    sRTMan->Release(src);
    sRTMan->Release(dest);
    */
  }
}


/****************************************************************************/

RNBloom::RNBloom()
{ 
  Anim.Init(Wz4RenderType->Script);
}

RNBloom::~RNBloom()
{
  delete MtrlMask;
  delete MtrlComp;
}

void RNBloom::Init()
{
  int i=0,j=0;
  Para = ParaBase;

  if (Para.CalcMaskFromScreen&1)
    i |= tBloomMaskMat::EXTRA_GRAY;
  if (Para.MultiplyScreenCol&1)
    i |= tBloomMaskMat::EXTRA_MSC;
  if (Para.UseAlpha&1)
    i |= tBloomMaskMat::EXTRA_ABUF;
  if (Para.UseZBuf&1)
    i |= tBloomMaskMat::EXTRA_ZBUF;

  if((Para.PreBlurMode & 3) == 1)
    i |= tBloomMaskMat::EXTRA_PRETAP5;
  if((Para.PreBlurMode & 3) == 2)
    i |= tBloomMaskMat::EXTRA_PRETAP9;


  switch(Para.CompFormula)
  {
  default:
    j |= tBloomCompMat::EXTRA_ADD;
    break;
  case 1:
    j |= tBloomCompMat::EXTRA_SCREEN;
    break;
  case 2:
    j |= tBloomCompMat::EXTRA_MUL;
    break;
  }

  MtrlMask = new tBloomMaskMat();
  MtrlMask->Extra = i;
  MtrlMask->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlMask->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlMask->TFlags[1] = sMTF_LEVEL2 | sMTF_TILE | sMTF_EXTERN;
  MtrlMask->Prepare(sVertexFormatDouble);  

  MtrlComp = new tBloomCompMat();
  MtrlComp->Extra = j;
  MtrlComp->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlComp->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlComp->TFlags[1] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlComp->Prepare(sVertexFormatDouble);  

}

void RNBloom::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
  if (Para.UseZBuf)
  {
    ctx->RenderFlags |= wRF_RenderZ;
  }
}

void RNBloom::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sInt divider = 2;
    if((Para.CalcMaskFromScreen&0xc)==0x04) divider=1;
    if((Para.CalcMaskFromScreen&0xc)==0x08) divider=4;
    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sTexture2D *mask = sRTMan->Acquire(src->SizeX/divider,src->SizeY/divider,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB8888);
    sTexture2D *blur = sRTMan->Acquire(src->SizeX/divider,src->SizeY/divider,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB8888);    
    
    sCBuffer<tBloomMaskVSPara> cbv;
    sCBuffer<tBloomMaskPSPara> cbp;

    sRTMan->SetTarget(dest);  
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();

    cbp.Data->HighlightCol.InitColor(Para.HighlightCol);
    cbp.Data->MaskCol.InitColor(Para.MaskCol);
    if(Para.CalcMaskFromScreen&2)
      cbp.Data->MaskCol = cbp.Data->MaskCol*2-sVector4(1,1,1,1);
    cbp.Data->MaskScaleBias.Init(Para.MaskAmp/3.0f, Para.MaskBias, 0.0f, 0.0f);
    cbp.Data->AlphaScaleBias.Init(Para.AlphaAmp, Para.AlphaBias, 0.0f, 0.0f);
    cbp.Data->ZBufScaleBias.Init(Para.ZBufAmp, Para.ZBufBias, 0.0f, 0.0f);
    cbp.Data->GrayCol.InitColor(Para.GrayMaskCol);
    cbp.Data->GrayCol = cbp.Data->GrayCol * Para.GrayMaskAmp;
    cbp.Data->GrayCol.w = Para.GrayMaskBias;
    cbp.Data->BlendFactor.Init(Para.BlendFactor.x, Para.BlendFactor.y, Para.BlendFactor.z, 0.0f);
    cbp.Data->Misc.Init(Para.PreBlurSize/src->SizeX,Para.PreBlurSize/src->SizeY,0,0);
    cbp.Modify();
  
    sTexture2D *d;

    sRTMan->SetTarget(mask);
    d = mask;

    MtrlMask->Texture[0] = src;

    if (Para.UseZBuf)
    {      
      MtrlMask->Texture[1] = ctx->ZTarget;
    }
    else
    {
      MtrlMask->Texture[1] = 0;
    }

    MtrlMask->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src,0,0,Para.PreBlurMode ? IPPH_BLURRY : 0);
    MtrlMask->Texture[0] = 0;
    MtrlMask->Texture[1] = 0;

    if(Para.DebugShow==1)
    {
      ctx->IppHelper->Copy(dest,mask);
    }

    if (Para.DebugShow==0 || Para.DebugShow==2)
    {            
      if (Para.DebugShow==2)
      {
        //Show Mask Only
        sRTMan->SetTarget(dest);  
        d = dest;
      }
      else
      {
        sRTMan->SetTarget(blur);
        d = blur;
      }
      
      sRTMan->AddRef(mask);
      ctx->IppHelper->Blur(d, mask, Para.BlurRadius*(src->SizeY/1080.0f)*2/divider, Para.BlurAmp);

      if (Para.DebugShow==0)
      {
        sRTMan->SetTarget(dest);
        sCBuffer<tBloomCompVSPara> cbv;
        sCBuffer<tBloomCompPSPara> cbp;
        ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
        cbv.Modify();
        cbp.Data->BloomCol.InitColor(Para.CompBloomCol);
        cbp.Data->BloomCol = cbp.Data->BloomCol * Para.CompBloomAmp;
        cbp.Data->ScreenCol.InitColor(Para.CompScreenCol);
        cbp.Data->ScreenCol = cbp.Data->ScreenCol * Para.CompScreenAmp;
        
        MtrlComp->Texture[0] = src;
        MtrlComp->Texture[1] = blur;
        MtrlComp->Set(&cbv,&cbp);
        ctx->IppHelper->DrawQuad(dest,src);
        MtrlComp->Texture[0] = 0;
        MtrlComp->Texture[1] = 0;
      }
    }

    sRTMan->Release(blur);    
    sRTMan->Release(mask);
    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}


/****************************************************************************/

RNPromist::RNPromist()
{ 
  Anim.Init(Wz4RenderType->Script);
}

RNPromist::~RNPromist()
{
  delete MtrlMask;
  delete MtrlComp;
  delete MtrlCopy;
}

void RNPromist::Init()
{
  int i=0;
  Para = ParaBase;

  if (Para.CalcMaskFromScreen)
    i |= tBloomMaskMat::EXTRA_GRAY;

  if (Para.MultiplyScreenCol)
    i |= tBloomMaskMat::EXTRA_MSC;

  if (Para.UseAlpha)
    i |= tBloomMaskMat::EXTRA_ABUF;

  if (Para.UseZBuf)
    i |= tBloomMaskMat::EXTRA_ZBUF;

  MtrlMask = new tBloomMaskMat();
  MtrlMask->Extra = i;
  MtrlMask->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlMask->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlMask->TFlags[1] = sMTF_LEVEL2 | sMTF_TILE | sMTF_EXTERN;
  MtrlMask->Prepare(sVertexFormatDouble);  

  MtrlComp = new tPromistCompMat();
  MtrlComp->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlComp->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlComp->TFlags[1] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlComp->TFlags[2] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlComp->Prepare(sVertexFormatDouble);  

  MtrlCopy = new tCopyMat();
  MtrlCopy->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlCopy->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlCopy->Prepare(sVertexFormatDouble);  
}

void RNPromist::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
  if (Para.UseZBuf)
  {
    ctx->RenderFlags |= wRF_RenderZ;
  }
}

void RNPromist::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sTexture2D *mask = sRTMan->Acquire(src->SizeX/2,src->SizeY/2,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB8888);
    sTexture2D *mblur = sRTMan->Acquire(src->SizeX/2,src->SizeY/2,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB8888);
    sTexture2D *sblur = sRTMan->Acquire(src->SizeX/2,src->SizeY/2,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB8888);    
    
    sCBuffer<tBloomMaskVSPara> cbv;
    sCBuffer<tBloomMaskPSPara> cbp;

    sRTMan->SetTarget(dest);  
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();

    cbp.Data->HighlightCol.InitColor(Para.HighlightCol);
    cbp.Data->MaskCol.InitColor(Para.MaskCol);
    cbp.Data->MaskScaleBias.Init(Para.MaskAmp/3.0f, Para.MaskBias, 0.0f, 0.0f);
    cbp.Data->AlphaScaleBias.Init(Para.AlphaAmp, Para.AlphaBias, 0.0f, 0.0f);
    cbp.Data->ZBufScaleBias.Init(Para.ZBufAmp, Para.ZBufBias, 0.0f, 0.0f);
    cbp.Data->GrayCol.InitColor(Para.GrayMaskCol);
    cbp.Data->GrayCol = cbp.Data->GrayCol * Para.GrayMaskAmp;
    cbp.Data->GrayCol.w = Para.GrayMaskBias;
    cbp.Data->BlendFactor.Init(Para.BlendFactor.x, Para.BlendFactor.y, Para.BlendFactor.z, 0.0f);
    cbp.Data->Misc.Init(0,0,0,0);
    cbp.Modify();

    //Blur Screen
    sRTMan->SetTarget(sblur);  
    sRTMan->AddRef(src);
    ctx->IppHelper->Blur(sblur, src, Para.BlurScreenRadius*(src->SizeY/1080.0f), Para.BlurScreenAmp);


    //Create Mask    
    sRTMan->SetTarget(mask);

    MtrlMask->Texture[0] = src;

    if (Para.UseZBuf)
      MtrlMask->Texture[1] = ctx->ZTarget;
    else
      MtrlMask->Texture[1] = 0;

    MtrlMask->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src);
    MtrlMask->Texture[0] = 0;
    MtrlMask->Texture[1] = 0;

    //Blur Mask
    sRTMan->SetTarget(mblur);
    sRTMan->AddRef(mask);

    ctx->IppHelper->Blur(mblur, mask, Para.BlurRadius*(src->SizeY/1080.0f), Para.BlurAmp);

    //Final Composition or just copy request buffer
    sRTMan->SetTarget(dest);
    switch (Para.DebugShow)
    {
      //Mask
      case 1 :        

      //BlurredMask
      case 2 :

      //BlurredScreen
      case 3 :
      {        
        sCBuffer<tCopyVSPara> cbv;
        sCBuffer<tCopyPSPara> cbp;
        ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
        cbv.Modify();

        if (Para.DebugShow==1)
          MtrlCopy->Texture[0] = mask;
        else  if(Para.DebugShow==2)
          MtrlCopy->Texture[0] = mblur;
        else
         MtrlCopy->Texture[0] = sblur;
       
        MtrlCopy->Set(&cbv,&cbp);
        ctx->IppHelper->DrawQuad(dest,src);
        MtrlCopy->Texture[0] = 0;        
      }
      break;

      //Result      
      case 0 :
      default :
      {
        sRTMan->SetTarget(dest);
        sCBuffer<tPromistCompVSPara> cbv;
        sCBuffer<tPromistCompPSPara> cbp;
        ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
        cbv.Modify();
        cbp.Data->ScreenBlurredCol.InitColor(Para.CompBloomCol);
        cbp.Data->ScreenBlurredCol = cbp.Data->ScreenBlurredCol * Para.CompBloomAmp;
        cbp.Data->ScreenNormalCol.InitColor(Para.CompScreenCol);
        cbp.Data->ScreenNormalCol = cbp.Data->ScreenNormalCol * Para.CompScreenAmp;
        
        MtrlComp->Texture[0] = src;
        MtrlComp->Texture[1] = sblur;
        MtrlComp->Texture[2] = mblur;
        MtrlComp->Set(&cbv,&cbp);
        ctx->IppHelper->DrawQuad(dest,src);
        MtrlComp->Texture[0] = 0;
        MtrlComp->Texture[1] = 0;
      }
      break;
    }

    sRTMan->Release(sblur);    
    sRTMan->Release(mblur);    
    sRTMan->Release(mask);
    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNDoF3::RNDoF3()
{ 
  Anim.Init(Wz4RenderType->Script);
}

RNDoF3::~RNDoF3()
{
  delete MtrlMask;
  delete MtrlComp;
  delete MtrlCopy;
}

void RNDoF3::Init()
{
//  int i=0;
  Para = ParaBase;

  MtrlMask = new tDoF3MaskMat();  
  MtrlMask->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlMask->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlMask->Prepare(sVertexFormatDouble);  

  MtrlComp = new tDoF3CompMat();
  MtrlComp->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlComp->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlComp->TFlags[1] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlComp->TFlags[2] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlComp->Prepare(sVertexFormatDouble);  

  MtrlCopy = new tCopyMat();
  MtrlCopy->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlCopy->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlCopy->Prepare(sVertexFormatDouble);  
}

void RNDoF3::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
  ctx->RenderFlags |= wRF_RenderZ;  
}

void RNDoF3::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sTexture2D *mask = sRTMan->Acquire(src->SizeX/2,src->SizeY/2,sTEX_2D|sTEX_NOMIPMAPS|sTEX_R16F);
    sTexture2D *mblur = sRTMan->Acquire(src->SizeX/2,src->SizeY/2,sTEX_2D|sTEX_NOMIPMAPS|sTEX_R16F);
    sTexture2D *sblur = sRTMan->Acquire(src->SizeX/2,src->SizeY/2,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB8888);    
    
    sCBuffer<tDoF3MaskVSPara> cbv;
    sCBuffer<tDoF3MaskPSPara> cbp;

    sRTMan->SetTarget(dest);  
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();   
    
    sF32 focaldepth = Para.FocalDepth / ctx->View.ClipFar;
    sF32 hfocusrange = (Para.FocusRange / ctx->View.ClipFar) / 2.0f;

    cbp.Data->Params.x = focaldepth - hfocusrange;
    cbp.Data->Params.y = 1.0f / cbp.Data->Params.x * Para.UnfocusRangeBefore;
    cbp.Data->Params.z = focaldepth + hfocusrange;
    cbp.Data->Params.w = 1.0f / (1.0f - cbp.Data->Params.z) * Para.UnfocusRangeAfter;
    
    sF32  BlurScreenRadius = Para.BlurMaskRadius * 2;
    sF32  BlurScreenAmb = 1.0f;
    
    if (Para.Advanced==1)
    {
      BlurScreenRadius = Para.BlurScreenRadius;
      BlurScreenAmb    = Para.BlurScreenAmb;    
    }
    cbp.Modify();

    //Blur Screen
    sRTMan->SetTarget(sblur);  
    sRTMan->AddRef(src);
    ctx->IppHelper->Blur(sblur, src, BlurScreenRadius*(src->SizeY/1080.0f), BlurScreenAmb);

    //Create Mask    
    sRTMan->SetTarget(mask);

    MtrlMask->Texture[0] = ctx->ZTarget;
    MtrlMask->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src);
    MtrlMask->Texture[0] = 0;    

    //Blur Mask
    sRTMan->SetTarget(mblur);
    sRTMan->AddRef(mask);
   
    ctx->IppHelper->Blur(mblur, mask, Para.BlurMaskRadius*(src->SizeY/1080.0f), Para.BlurMaskAmb);

    //Final Composition or just copy request buffer
    sRTMan->SetTarget(dest);
    switch (Para.DebugShow)
    {
      //Mask
      case 1 :        

      //BlurredMask
      case 2 :

      //BlurredScreen
      case 3 :
      {        
        sCBuffer<tCopyVSPara> cbv;
        sCBuffer<tCopyPSPara> cbp;
        ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
        cbv.Modify();

        if (Para.DebugShow==1)
          MtrlCopy->Texture[0] = mask;
        else  if(Para.DebugShow==2)
          MtrlCopy->Texture[0] = mblur;
        else
         MtrlCopy->Texture[0] = sblur;
       
        MtrlCopy->Set(&cbv,&cbp);
        ctx->IppHelper->DrawQuad(dest,src);
        MtrlCopy->Texture[0] = 0;        
      }
      break;

      //Result      
      case 0 :
      default :
      {
        sRTMan->SetTarget(dest);
        sCBuffer<tDoF3CompVSPara> cbv;
        sCBuffer<tDoF3CompPSPara> cbp;
        ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
        cbv.Modify();

        cbp.Data->ScreenBlurredCol.Init(1.0f,1.0f,1.0f,1.0f);
        cbp.Data->ScreenNormalCol.Init(1.0f,1.0f,1.0f,1.0f);
        
        if (Para.Advanced==1)
        {
          cbp.Data->ScreenBlurredCol.Init(Para.CompMaskAmb,Para.CompMaskAmb,Para.CompMaskAmb,Para.CompMaskAmb);
          cbp.Data->ScreenNormalCol.Init(Para.CompScreenAmb,Para.CompScreenAmb,Para.CompScreenAmb,Para.CompScreenAmb);          
        }
     
        MtrlComp->Texture[0] = src;
        MtrlComp->Texture[1] = sblur;
        MtrlComp->Texture[2] = mblur;
        MtrlComp->Set(&cbv,&cbp);
        ctx->IppHelper->DrawQuad(dest,src);
        MtrlComp->Texture[0] = 0;
        MtrlComp->Texture[1] = 0;
      }
      break;
    }

    sRTMan->Release(sblur);    
    sRTMan->Release(mblur);    
    sRTMan->Release(mask);
    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}

RNGlow::RNGlow()
{ 
  Anim.Init(Wz4RenderType->Script);
}

RNGlow::~RNGlow()
{
  delete MtrlGradient;
  delete MtrlMask;
  delete MtrlComb;
}

void RNGlow::Init()
{
  int e1=0;
  int e2=0;
  Para = ParaBase;

  if (Para.MaskEdgeGlow)
  {
    e1 = tGlowMaskMat::EXTRA_EDGE;
  }
  switch (Para.CombineMode)
  {
    case 0 :
      e2 = tGlowCombMat::EXTRA_ADD;
    break;
    case 2 :
      e2 = tGlowCombMat::EXTRA_MULTIPLY;
    break;
    case 3 :
      e2 = tGlowCombMat::EXTRA_SUB;
    break;
    default :
      e2 = tGlowCombMat::EXTRA_BLEND;
    break;
  }

  MtrlGradient = new tGlowGradientMat();  
  MtrlGradient->Flags = sMTRL_ZOFF|sMTRL_CULLOFF|sMTRL_NOZRENDER;
  MtrlGradient->TFlags[0] = sMTF_LEVEL1 | sMTF_CLAMP;
  MtrlGradient->Prepare(sVertexFormatDouble);  

  MtrlMask = new tGlowMaskMat();  
  MtrlMask->Extra = e1;
  MtrlMask->Flags = sMTRL_ZOFF|sMTRL_CULLOFF|sMTRL_NOZRENDER;
  MtrlMask->TFlags[0] = sMTF_LEVEL1 | sMTF_CLAMP;
  MtrlMask->Prepare(sVertexFormatDouble);  
  
  MtrlComb = new tGlowCombMat();
  MtrlComb->Extra = e2;
  MtrlComb->Flags = sMTRL_ZOFF|sMTRL_CULLOFF|sMTRL_NOZRENDER;
  MtrlComb->TFlags[0] = sMTF_LEVEL1 | sMTF_CLAMP;
  MtrlComb->TFlags[1] = sMTF_LEVEL1 | sMTF_CLAMP;
  MtrlComb->TFlags[2] = sMTF_LEVEL1 | sMTF_CLAMP;
  MtrlComb->Prepare(sVertexFormatDouble);  
}

void RNGlow::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNGlow::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sInt sx = ctx->ScreenX;
    sInt sy = ctx->ScreenY;

    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sTexture2D *grad = sRTMan->Acquire(512,1,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB32F);
    sTexture2D *mask = sRTMan->Acquire(sx/2,sy/2,sTEX_2D|sTEX_NOMIPMAPS|sTEX_R16F);
    sTexture2D *blur = sRTMan->Acquire(sx/2,sy/2,sTEX_2D|sTEX_NOMIPMAPS|sTEX_R16F);

    sRTMan->AddRef(mask);

    //Render Gradient Texture
    sCBuffer<tGlowGradientMatVSPara> cbv_grad;
    sCBuffer<tGlowGradientMatPSPara> cbp_grad;

    sRTMan->SetTarget(grad);  
    ctx->IppHelper->GetTargetInfo(cbv_grad.Data->mvp);
    cbv_grad.Modify(); 

    cbp_grad.Data->in_color.InitColor(Para.GradInColor);
    cbp_grad.Data->out_color.InitColor(Para.GradOutColor);
    cbp_grad.Data->factors.Init(Para.GradShift,Para.GradIntensity,Para.GradOffset,0.0f);
    cbp_grad.Modify();
    MtrlGradient->Set(&cbv_grad,&cbp_grad);
    ctx->IppHelper->DrawQuad(grad); 
    
    //Render Mask Texture
    sCBuffer<tGlowMaskMatVSPara> cbv_mask;
    sCBuffer<tGlowMaskMatPSPara> cbp_mask;

    sRTMan->SetTarget(mask);  
    ctx->IppHelper->GetTargetInfo(cbv_mask.Data->mvp);
    cbv_mask.Modify(); 

    cbp_mask.Data->textpar.Init(1.0f/sx,1.0f/sy,0.0f,0.0f);
    cbp_mask.Data->maskpar.Init(-Para.MaskThres+Para.MaskSoftenThres*0.5f, 1.0f/Para.MaskSoftenThres, 0.0f, 0.0f);
    cbp_mask.Modify();
    MtrlMask->Texture[0] = src;
    MtrlMask->Set(&cbv_mask,&cbp_mask);
    ctx->IppHelper->DrawQuad(mask,src); 
    MtrlMask->Texture[0] = 0;

    //Blur Mask
    sRTMan->SetTarget(blur);
    ctx->IppHelper->Blur(blur,mask,Para.BlurMaskRadius*sx/800,Para.BlurMaskAmp*sx/800);

    //Combine
    sRTMan->SetTarget(dest);
    sCBuffer<tGlowCombMatVSPara> cbv_comb;
    sCBuffer<tGlowCombMatPSPara> cbp_comb;

    ctx->IppHelper->GetTargetInfo(cbv_comb.Data->mvp);
    cbv_comb.Modify(); 
    cbp_comb.Modify();
    MtrlComb->Texture[0]=src;
    MtrlComb->Texture[1]=blur;
    MtrlComb->Texture[2]=grad;
    MtrlComb->Set(&cbv_comb,&cbp_comb);
    ctx->IppHelper->DrawQuad(dest,src);
    MtrlComb->Texture[0]=0;
    MtrlComb->Texture[1]=0;
    MtrlComb->Texture[2]=0;

    switch (Para.DebugShow)
    {      
      case 1 : //blurmask
        ctx->IppHelper->Copy(dest,blur);    
      break;
      case 2 : //mask
        ctx->IppHelper->Copy(dest,mask);
      break;
      case 3 : //gradient
        ctx->IppHelper->Copy(dest,grad);
      break;
      default :
      break;
    }    

    sRTMan->Release(blur);
    sRTMan->Release(grad);
    sRTMan->Release(mask);
    sRTMan->Release(dest);
    sRTMan->Release(src);
  }
}


/****************************************************************************/
#ifdef COMPILE_FR033

FR033_MeteorShowerSim::FR033_MeteorShowerSim()
{
  Anim.Init(Wz4RenderType->Script);
  GroupLogos = 0;

}

FR033_MeteorShowerSim::~FR033_MeteorShowerSim()
{
  GroupLogos->Release();  
}

void FR033_MeteorShowerSim::Init()
{
  int i=0;
  Para=ParaBase;

  sInt nblogos=GroupLogos->Members.GetCount();
  sInt nbmeteors=Para.MF_Count;  
  
  //Init Asteroids
  Meteors.Resize(nbmeteors);
  sRandomKISS rg(ParaBase.MF_Seed);
     
  for (i=0;i<nbmeteors;i++)
  {
    Meteors[i].Mesh = (Wz4Mesh *)(GroupLogos->Members[rg.Int(nblogos)]);
    Meteors[i].Speed = (sVector30)(Para.MF_StartSpeed)+sVector30(rg.Float(Para.MF_RandSpeed.x),rg.Float(Para.MF_RandSpeed.y),rg.Float(Para.MF_RandSpeed.z));
    Meteors[i].StartPos = Para.MF_StartPos+sVector30(rg.Float(Para.MF_RandPos.x)-Para.MF_RandPos.x/2.0f,rg.Float(Para.MF_RandPos.y)-Para.MF_RandPos.y/2.0f,rg.Float(Para.MF_RandPos.z)-Para.MF_RandPos.z/2.0f);        
    Meteors[i].StartPos -= Meteors[i].Speed*(Para.MF_StartTime + rg.Float(Para.MF_RandTime));
    Meteors[i].CollTime = -Meteors[i].StartPos.y / Meteors[i].Speed.y;
    Meteors[i].Enable = true;
  } 
}

void FR033_MeteorShowerSim::Simulate(Wz4RenderContext *ctx)
{
  // we animate the parameters, so we have to save them a lot
  Para=ParaBase;

  // bind parameter to scripting engine
  Anim.Bind(ctx->Script,&Para);

  // execute script
  SimulateCalc(ctx);
}

sInt FR033_MeteorShowerSim::GetPartCount()
{
  return Meteors.GetCount();
}
sInt FR033_MeteorShowerSim::GetPartFlags()
{
  return 0;
}

void FR033_MeteorShowerSim::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sInt cnt=0;

  Wz4Particle *part = pinfo.Parts;
  for (int i=0;i<Meteors.GetCount();i++)
  {
    Meteors[i].Pos=Meteors[i].StartPos + Meteors[i].Speed*(time+dt);

    sVector31 a(Meteors[i].Pos.x,Meteors[i].Pos.y,Meteors[i].Pos.z);        //Position of Meteor
    sVector30 b=(sVector30)(Meteors[i].StartPos + Meteors[i].Speed*Meteors[i].CollTime); //Position of Impact
    
    part->Time=time;
    part->Pos=Meteors[i].Pos;
    Meteors[i].Enable=false;

    if(time>=Meteors[i].CollTime-Para.MF_OffTime && time<=Meteors[i].CollTime+Para.MF_OnTime)
    {
      Meteors[i].Enable=true;
      cnt++;
    }    
    else
    {
      part->Time=-1;
    }
    part++;
    cnt++;    
  }
  pinfo.Used = cnt;
}


FR033_WaterSimRender::FR033_WaterSimRender()
{
  static const sU32 desc[] = 
  {
    sVF_STREAM0|sVF_UV1|sVF_F3,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_POSITION|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV0|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV2|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV3|sVF_F1,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_COLOR0,
    0,
  };

  WaterMtrl = 0;   
  Dancer = 0;
  MeteorSim = 0;

  WaterGeo = new sGeometry(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);

  DancerFormat = sCreateVertexFormat(desc);
  DancerGeo = new sGeometry;
  DancerGeo->Init(sGF_TRILIST|sGF_INDEX16,DancerFormat);

  Anim.Init(Wz4RenderType->Script);   
}

FR033_WaterSimRender::~FR033_WaterSimRender()
{
  delete DancerGeo;
  delete WaterGeo;
  delete DancerMtrl;

  WaterMtrl->Release();
  Dancer->Release();
  MeteorSim->Release();
}
#define POSY_TABLE_CNT 1024

void FR033_WaterSimRender::Init()
{
  Para = ParaBase;

  int i=0;
  int xc=Para.WG_VertXZ[0];
  int zc=Para.WG_VertXZ[1];
   
  FR033_MeteorShowerSim *ms=(FR033_MeteorShowerSim *)(MeteorSim->RootNode);     
  Waves.Resize(ms->Meteors.GetCount());
  PosY.Resize(Para.DP_Count*POSY_TABLE_CNT);

  WaterMesh.MakeGrid(xc-1,zc-1);
  

  i=0;
  for (int z=0;z<zc;z++)
  {
    for (int x=0;x<xc;x++)
    {
      float px=  (((float)x)/(xc-1)-0.5f)*Para.WG_SizeXZ[0];
      float pz=  (((float)z)/(zc-1)-0.5f)*Para.WG_SizeXZ[1];
      WaterMesh.Vertices[i++].Pos = sVector30(px, 0, pz) + Para.WG_Pos;
    }
  }

  sU16 *ip=0;
  sInt nbfaces = WaterMesh.Faces.GetCount();
  WaterGeo->BeginLoadIB(nbfaces*6,sGD_FRAME,&ip);
  for (i=0;i<nbfaces;i++)
  {  
    sQuad(ip,0, WaterMesh.Faces[i].Vertex[0], WaterMesh.Faces[i].Vertex[1], WaterMesh.Faces[i].Vertex[2], WaterMesh.Faces[i].Vertex[3]);
  }
  WaterGeo->EndLoadIB();

  DancerMtrl = new StaticParticleShader;
  DancerMtrl->Flags = sMTRL_CULLOFF|sMTRL_ZON;
  DancerMtrl->Texture[0] = Dancer->Texture;
  DancerMtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  DancerMtrl->FuncFlags[sMFT_ALPHA] = sMFF_GREATER;
  DancerMtrl->AlphaRef=0x80;
  DancerMtrl->BlendColor = sMB_ALPHA;    
  DancerMtrl->Prepare(DancerFormat);
  Dancers.Resize(Para.DP_Count);

  sRandomKISS rg(Para.DP_Seed);


  const sF32 maxdist = 1.5f;
  sInt timeout = 10000;




  for (i=0;i<Para.DP_Count;i++)
  {
tryagain:
    sF32 x,z;
    do
    {
      x = rg.Float(2)-1;
      z = rg.Float(2)-1;
    }
    while(x*x+z*z>1);


    Dancers[i].pos.x=x*0.5f*(Para.DP_DimXZ[0])+Para.DP_StartPos[0];
    Dancers[i].pos.y=0;
    Dancers[i].pos.z=z*0.5f*(Para.DP_DimXZ[1])+Para.DP_StartPos[2];
    Dancers[i].scale.x=rg.Float(Para.DP_RandScale[0])+Para.DP_Scale[0];
    Dancers[i].scale.y=rg.Float(Para.DP_RandScale[1])+Para.DP_Scale[1];

    if(timeout>0)
    {
      for(sInt j=0;j<i;j++)
      {
        x = Dancers[i].pos.x - Dancers[j].pos.x;
        z = Dancers[i].pos.z - Dancers[j].pos.z;
        if(x*x+z*z<maxdist*maxdist)
        {
          timeout--;
          goto tryagain;
        }
      }
    }

    Dancers[i].atlas=rg.Int(32);

    sF32 px=Dancers[i].pos.x/Para.WG_SizeXZ[0]+0.5f;
    sF32 pz=Dancers[i].pos.z/Para.WG_SizeXZ[1]+0.5f;;

    px=sMax(sMin(px,Para.WG_SizeXZ[0]),0.0f);
    pz=sMax(sMin(pz,Para.WG_SizeXZ[1]),0.0f);

    px*=xc;
    pz*=zc;

    sInt ix1=px;
    sInt iz1=pz;    

    if (ix1>=xc)
      ix1=xc-1;

    if (iz1>=zc)
      iz1=zc-1;

    Dancers[i].vi=ix1+iz1*xc;    
  }

  
  if (Dancer->Atlas.Entries.IsEmpty())
  {
    UVRects.Resize(1);
    PartUVRect &r=UVRects[0];
    r.u=0;
    r.v=0;
    r.du=1;
    r.dv=1;
  }
  else
  {
    UVRects.Resize(Dancer->Atlas.Entries.GetCount());
    BitmapAtlasEntry *e;

    sFORALL(Dancer->Atlas.Entries,e)
    {
      PartUVRect &r=UVRects[_i];
      r.u = e->UVs.x0;
      r.v = e->UVs.y0;
      r.du = e->UVs.x1-e->UVs.x0;
      r.dv = e->UVs.y1-e->UVs.y0;
    }
  }
}


void FR033_WaterSimRender::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;

  // bind parameter to scripting engine
  Anim.Bind(ctx->Script,&Para);

  // execute script
  SimulateCalc(ctx);

  // this op does not have any animated childs. 
  // so this call is redundant
  SimulateChilds(ctx);

  Time=ctx->GetTime();  
  Simulate(Time);
}

void FR033_WaterSimRender::Prepare(Wz4RenderContext *ctx)
{  
  int i;
  sU16 *ip0;
  PartVert0 *vp0;
  PartVert1 *vp1;

  // create vertex buffer here per frame.
  // it might be used multiple times.    
  MakeGrid();

  sViewport view = ctx->View;
  view.UpdateModelMatrix(sMatrix34(Matrices[0]));
  sMatrix34 mat;
  mat = view.Camera;

  DancerGeo->BeginLoadVB(4,sGD_FRAME,&vp0,0);
  vp0[0].Init(1,0,sPI2F*0.25f*0);
  vp0[1].Init(1,1,sPI2F*0.25f*1);
  vp0[2].Init(0,1,sPI2F*0.25f*2);
  vp0[3].Init(0,0,sPI2F*0.25f*3);
  DancerGeo->EndLoadVB(-1,0);

  DancerGeo->BeginLoadIB(6,sGD_FRAME,&ip0);    // yes we need an index buffer, or D3DDebug is unhappy
  sQuad(ip0,0,0,1,2,3);
  DancerGeo->EndLoadIB();

  sInt atlascount = sMax(1,Dancer->Atlas.Entries.GetCount());
  DancerGeo->BeginLoadVB(ParaBase.DP_Count,sGD_FRAME,&vp1,1);
  for (i=0;i<ParaBase.DP_Count;i++)
  {    
    vp1->fade=1.0f;
    vp1->px=Dancers[i].pos.x;    
    vp1->py=WaterMesh.Vertices[Dancers[i].vi].Pos.y + ParaBase.DP_StartPos[1];   
    vp1->pz=Dancers[i].pos.z;
    vp1->rot=0.0f;
    vp1->sx=Dancers[i].scale.x; //Scale
    vp1->sy=Dancers[i].scale.y;  //Scale        
    vp1->u1=0.0f;
    vp1->v1=0.0f;
    vp1->uvrect.du=UVRects[Dancers[i].atlas%atlascount].du;
    vp1->uvrect.dv=UVRects[Dancers[i].atlas%atlascount].dv;
    vp1->uvrect.u=UVRects[Dancers[i].atlas%atlascount].u;
    vp1->uvrect.v=UVRects[Dancers[i].atlas%atlascount].v;
    vp1->c0 = 0xffffffff;
    vp1++;
  }
  DancerGeo->EndLoadVB(-1,1);  
}

void FR033_WaterSimRender::Render(Wz4RenderContext *ctx)
{
  // here we can filter out passes that do not interesst us

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    // here the material can decline to render itself

    if(WaterMtrl->SkipPhase(ctx->RenderMode,0)) return;

    // the effect may be placed multple times in the render graph.
    // this loop will get all the matrices:

    sMatrix34CM *mat;
    sFORALL(Matrices,mat)
    {
      sMatrix34 mat1 = sMatrix34(*mat)*Matrix;
      sMatrix34CM mat3 = (sMatrix34CM)mat1;
      // render it once
      WaterMtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,1,&mat3,0,0,0);
      WaterGeo->Draw();
    }
    
    FR033_MeteorShowerSim *ms=(FR033_MeteorShowerSim *)(MeteorSim->RootNode);


    // this is the wrong way of doing it. you should only comunicate with the particle generator using ms->Func()
    sInt partcount = ms->GetPartCount();
    Wz4PartInfo pinfo;
    pinfo.Init(0,partcount);

    ms->Func(pinfo,Time,0);

    sFORALL(Matrices,mat)
    {
      // here i add our own matrix

      sMatrix34 mat1 = sMatrix34(*mat)*Matrix;
      sMatrix34CM mat3 = (sMatrix34CM)mat1;

      // render it once

      for (int i=0;i<ms->Meteors.GetCount();i++)
      {        
        sSRT srt;
        srt.Translate=ms->Meteors[i].Pos;
        
        sMatrix34 mat2;
        srt.MakeMatrix(mat2);
        
        sMatrix34 mat4;       
        mat4.Look(ms->Meteors[i].Speed);

        mat2 = mat4*mat2;
        mat3=(sMatrix34CM )(mat2*mat1);

        if (ms->Meteors[i].Enable)
          ms->Meteors[i].Mesh->Render(sRF_TARGET_MAIN,0,&mat3,ctx->GetTime(),ctx->Frustum);
      }         
    }

    sMatrix34 mat2;
    sCBuffer<StaticParticlePSPara> cb1;
    cb1.Data->col.InitColor(0xffffffff);
    cb1.Data->fade0col.InitColor(0xff00ff00);
    sCBuffer<StaticParticleVSPara> cb0;

    sFORALL(Matrices,mat)
    {
      sViewport view = ctx->View;
      view.UpdateModelMatrix(sMatrix34(*mat));
      mat2 = view.ModelView;
      mat2.Invert3();

      cb0.Data->mvp = view.ModelScreen;
      cb0.Data->di = mat2.i;
      cb0.Data->dj = mat2.j;
      cb0.Modify();

      DancerMtrl->Set(&cb0,&cb1);
      DancerGeo->Draw(0,0,ParaBase.DP_Count,0);
    }
  }
}

void FR033_WaterSimRender::Simulate(sF32 time)
{
  int i;
  sInt cnt=0;

  FR033_MeteorShowerSim *ms=(FR033_MeteorShowerSim *)(MeteorSim->RootNode);

//Simulate Meteor
  for (i=0;i<ms->Meteors.GetCount();i++)
  {            
    if ((ms->Meteors[i].CollTime-time)<=0.0f && ms->Meteors[i].Enable)
    {
      sVector31 p=ms->Meteors[i].StartPos+ms->Meteors[i].Speed*ms->Meteors[i].CollTime;
      Waves[cnt].Pos = sVector2(p.x,p.z);
      Waves[cnt++].Time = time-ms->Meteors[i].CollTime;
    }
  }

//Simulate Water
  int xc=ParaBase.WG_VertXZ[0];
  int zc=ParaBase.WG_VertXZ[1];
  sF32 ml=ParaBase.WG_SizeXZ[0] * ParaBase.WG_SizeXZ[0] + ParaBase.WG_SizeXZ[1] * ParaBase.WG_SizeXZ[1];
 
  sF32 maxtime = ms->Para.MF_OnTime;
  sF32 treshtime = 0.90*maxtime;   // fade out after 75% of ontime
  sF32 resttime = 1.0f/(0.10f*maxtime);

  for (i=0;i<POSY_TABLE_CNT;i++)
  {
    for (int j=0;j<cnt;j++)
    {
      sF32 l= i/1024.0f;
      sF32 a = Waves[j].Time*ParaBase.WS_FreqTime - l*ParaBase.WS_FreqExpansion;
      sF32 b = ParaBase.WS_Amplitude/(Waves[j].Time*ParaBase.WS_DampingTime + l*ParaBase.WS_DampingExpansion  + 1.0f);
      if (a<0.0f) a=0.0f;
      if(Waves[j].Time>treshtime)
        b *= (maxtime-Waves[j].Time)*resttime;
      PosY[j*1024+i]= -sin(a*ParaBase.WS_Freq)*b;
    }
  }

  for (i=0;i<xc*zc;i++)
  {
    WaterMesh.Vertices[i].Pos.y = 0;
    for (int j=0;j<cnt;j++)
    {
      sF32 l= (Waves[j].Pos - sVector2(WaterMesh.Vertices[i].Pos.x,WaterMesh.Vertices[i].Pos.z)).LengthSq()/ml;      
      if (l<0) l=0;
      if (l>(POSY_TABLE_CNT-2)) l=POSY_TABLE_CNT-2;

      l=l*POSY_TABLE_CNT;
      sF32 fl=sFrac(l);
      int il=l;

      WaterMesh.Vertices[i].Pos.y -= (PosY[j*POSY_TABLE_CNT+il]*(1.0f-fl)) + (PosY[j*POSY_TABLE_CNT+(il+1)]*fl);
    }        
  }
}

void FR033_WaterSimRender::MakeGrid()
{
  int i;
  int xc=ParaBase.WG_VertXZ[0];
  int zc=ParaBase.WG_VertXZ[1];
  sVertexStandard *vp=0;

  WaterMesh.CalcNormals();

  WaterGeo->BeginLoadVB(zc*xc,sGD_FRAME,&vp);
	for(i=0;i<xc*zc;i++)
  {
    vp->Init(WaterMesh.Vertices[i].Pos.x, WaterMesh.Vertices[i].Pos.y, WaterMesh.Vertices[i].Pos.z, 
             WaterMesh.Vertices[i].Normal.x, WaterMesh.Vertices[i].Normal.y, WaterMesh.Vertices[i].Normal.z, 
             (WaterMesh.Vertices[i].U0+ParaBase.WG_OffsetUV[0])*ParaBase.WG_ScaleUV[0], (WaterMesh.Vertices[i].V0+ParaBase.WG_OffsetUV[1])*ParaBase.WG_ScaleUV[1]);
    vp++;
	}
  WaterGeo->EndLoadVB();
}

#endif

/****************************************************************************/

static void AddGlyph(GenBitmap *out, sFont2D *font, sInt &cursorx, sInt &cursory, sInt sizex, sInt sizey, sInt safety, sInt c, sInt spacepre, sInt spacecell, sInt spacepost)
{
  sChar text[2];
  sFont2D::sLetterDimensions dim = font->sGetLetterDimensions(c);
  text[0] = c;
  text[1] = 0;
    
  
  if ((cursorx+safety*2+dim.Cell)>sizex)
  {
    cursorx=safety;
    cursory+=safety*2+font->GetHeight();
  }

  out->Atlas.Entries[c].Pixels.Init(dim.Cell+safety*2-spacecell, dim.Pre+spacepre, dim.Post+spacepost, 0);
  
  float u1=sF32(cursorx-safety)/sizex;
  float v1=sF32(cursory)/sizey;
  float u2=sF32(cursorx+dim.Cell+safety-1)/sizex;
  float v2=sF32(cursory+font->GetHeight()-1)/sizey;
  
  out->Atlas.Entries[c].UVs.Init(u1,v1,u2,v2);

  font->Print(0,cursorx-dim.Pre+1,cursory,text,1);
  cursorx+=dim.Cell+safety*2;
}


void TronInitFont(GenBitmap *out, const sChar *name, const sChar *letter, sInt sizex, sInt sizey, sInt width, sInt height, sInt style,sInt safety, sInt spacepre, sInt spacecell, sInt spacepost, sU32 col)
{
  int i;

  out->Init(sizex, sizey);
  out->Flat(0x00);
  
  //Load Font
  sFont2D *font;
  sInt cursorx=safety;
  sInt cursory=safety;

  sInt flags = 0;
  if(style & 1) flags |= sF2C_BOLD;
  if(style & 2) flags |= sF2C_ITALICS;
  if(style & 4) flags |= sF2C_UNDERLINE;
  if(style & 8) flags |= sF2C_STRIKEOUT;
  if(style & 16) flags |= sF2C_SYMBOLS;

  font = new sFont2D(name,height,flags,width);
  
  sRender2DBegin(sizex,sizey);

  sSetColor2D(sGC_MAX+0,0x000000);
  sSetColor2D(sGC_MAX+1,0xffffff);

  sRect2D(0,0,sizex,sizey,sGC_MAX+0);
  font->SetColor(sGC_MAX+1,sGC_MAX+0);

  //Init Atlas
  out->Atlas.Entries.Resize(256);

  for (i=0;i<256;i++)
  {
    out->Atlas.Entries[i].Pixels.Init();
    out->Atlas.Entries[i].UVs.Init();
  }

  const sChar *s = letter;
  while(*s)
  {
    if(s[0]==0x5c && s[1]!=0)
    {     
      AddGlyph(out, font, cursorx, cursory, sizex, sizey, safety, s[1], spacepre, spacecell, spacepost);
      s+=2;
    }
    else if(s[1]=='-')
    {
      sInt c0 = s[0];
      sInt c1 = s[2];
      s+=3;

      for(sInt i=c0;i<=c1;i++)
        AddGlyph(out, font, cursorx, cursory, sizex, sizey, safety, i, spacepre, spacecell, spacepost);     
    }
    else
    {   
      AddGlyph(out, font, cursorx, cursory, sizex, sizey, safety, *s, spacepre, spacecell, spacepost);
      s++;
    }
  }

  out->Atlas.Entries[0].Pixels.Init(font->GetHeight(),font->GetBaseline(),0,0);

  sRender2DGet((sU32 *)out->Data);
  sU32 *src=(sU32 *)out->Data;
  sU64 *dst=(sU64 *)out->Data;

  src+=sizex*sizey-1;
  dst+=sizex*sizey-1;

  for(sInt i=0;i<sizex*sizey;i++)
  {
    sU32 alpha=(src[0]&0xff);
    sU32 color=col&0xffffff;

    alpha=(alpha*(col>>24))/255;    
    alpha=alpha<<24;
    
    dst[0]=out->GetColor64(alpha|color);
    
    src--;
    dst--;    
  }
    
  sRender2DEnd();
  delete font;
}


enum 
{
  E_TEXT_ALIGN_LEFT=0,
  E_TEXT_ALIGN_RIGHT,
  E_TEXT_ALIGN_CENTER
};

enum
{
  E_TOKEN_TYPE_GLYPH = 0,
  E_TOKEN_TYPE_ALIGN,
  E_TOKEN_TYPE_NEWLINE
};

class tChar
{
public:
  sInt          Type;
  sChar         Glyph;
  sInt          Font;
  sInt          Align;
  inline void Set(sInt type, sInt activefont, sInt activealign, sChar glyph=0) 
  {
    Type=type;
    Font=activefont;
    Align=activealign;
    Glyph=glyph;
  }
};

static bool CheckChar(sChar c, bool &nl, bool &ot, bool &ct)
{
  if (c==0)
    return true;

  nl=c==10;   //New Line
  ot=c=='<';  //open token
  ct=c=='>';  //close token
  return false;
}

static bool IncText(sInt &i, sChar *Text)
{
  if (Text[i])
  {
    i++;
    return false;
  }
  else
  {
    return true;
  }
}

void TronPrint(Wz4Mesh *out, GenBitmap **bmp, sInt nb, const sChar *text, sF32 zoom, sInt lwidth, sF32 spacex, sF32 spacey)
{
  sInt i=0;
  sU32 j=0;
#if !WZ4MESH_LOWMEM
  sU32 color=0xffffffff;
#endif
  sInt activefont=0;
  sInt activealign=E_TEXT_ALIGN_LEFT;
  sChar *Text=(sChar *)text;
  sInt cnt_char=0;

  sInt activewidth=0;
  bool nl;
  bool ot;
  bool ct;

  for (i=0;i<nb;i++)
  {
    if (bmp[i]->Atlas.Entries.GetCount()!=256)
    {
      out->MakeCube(100,100,100);
      return;
    }
  }

  tChar ps;
  sArray<tChar> token; 
  sArray<sInt>  strwidth;
  token.Clear();
  strwidth.Clear();
  sInt lastglyphpost=0;

  while (Text[i])
  {
    CheckChar(Text[i], nl, ot, ct);
    if (nl) //New Line
    {      
      ps.Set(E_TOKEN_TYPE_NEWLINE,activefont,activealign);
      token.AddTail(ps);
      IncText(i,Text);
      strwidth.AddTail(activewidth-lastglyphpost);
      activewidth=0;
    }
    else if (ct) //close token
    {
      IncText(i,Text);
    }
    else if (ot) //open token
    {
      IncText(i,Text);
      bool eos=CheckChar(Text[i], nl, ot, ct);
  
      if (!(eos||nl||ot||ct))
      {
        switch (Text[i])
        {
          case 'a' : //align
          {
            IncText(i,Text);
            eos=CheckChar(Text[i], nl, ot, ct);
            if (!(eos||nl||ot||ct))
            {              
              activealign=(Text[i]-'1')%3;
              IncText(i,Text);
              ps.Set(E_TOKEN_TYPE_ALIGN,activefont,activealign);
              token.AddTail(ps);
              strwidth.AddTail(activewidth);
              activewidth=0;
            }
          }
          break;
          case 'f' : //font
          {
            IncText(i,Text);
            eos=CheckChar(Text[i], nl, ot, ct);
            if (!(eos||nl||ot||ct))
            {
              activefont=(Text[i]-'1')%nb;
              IncText(i,Text);
            }
          }
          break;        
        }
      }

      if (!(nl||ot))
        IncText(i,Text);
            
    }
    else
    {
      cnt_char++;

      ps.Set(E_TOKEN_TYPE_GLYPH,activefont,activealign,Text[i]);
      token.AddTail(ps);

      if (activewidth!=0) //not first
      {
        activewidth += bmp[activefont]->Atlas.Entries[(unsigned int)Text[i]].Pixels.y0;
      }
      activewidth+=bmp[activefont]->Atlas.Entries[(unsigned int)Text[i]].Pixels.x0 + bmp[activefont]->Atlas.Entries[(unsigned int)Text[i]].Pixels.x1+spacex;
      lastglyphpost=bmp[activefont]->Atlas.Entries[(unsigned int)Text[i]].Pixels.x1+spacex;
      IncText(i,Text);
    }
  }

  strwidth.AddTail(0);
  sInt max_height=bmp[0]->Atlas.Entries[0].Pixels.x0+spacey*zoom;
  sInt max_baseline=bmp[0]->Atlas.Entries[0].Pixels.y0;

  for (i=0;i<nb;i++)
  {
    Wz4MeshCluster *cl = new Wz4MeshCluster;
    SimpleMtrl *mtrl=new SimpleMtrl;

    Texture2D *tex=new Texture2D();
    tex->ConvertFrom(bmp[i],sTEX_ARGB8888);
    mtrl->SetTex(0,tex);
    mtrl->SetMtrl(sMTRL_ZOFF|sMTRL_CULLOFF, sMB_ALPHA);
    mtrl->SetAlphaTest(sMFF_GREATER,0);

    mtrl->Prepare();

    cl->Mtrl=mtrl;      
    out->Clusters.AddTail(cl);
    
    if (max_height<bmp[i]->Atlas.Entries[0].Pixels.x0)
    {
      max_height=bmp[i]->Atlas.Entries[0].Pixels.x0;
      max_baseline=bmp[i]->Atlas.Entries[0].Pixels.y0;
    }    
  }

  Wz4MeshVertex *mv = out->Vertices.AddMany(4*cnt_char);
  Wz4MeshFace *mf = out->Faces.AddMany(cnt_char);  
 
  sF32 lpx=0;
  sF32 rpx=(lwidth-strwidth[0])*zoom;
  sF32 cpx=((lwidth-strwidth[0])/2.0f)*zoom;

  sF32 py=0;
  sInt k=0;
  sInt l=1;

  activefont=0;
  activealign=E_TEXT_ALIGN_LEFT;

  for (i=0;i<token.GetCount();i++)
  {
    switch (token[i].Type)
    {
      case E_TOKEN_TYPE_GLYPH :
      {
        j=token[i].Glyph;
        activefont=token[i].Font;

        sInt height=bmp[activefont]->Atlas.Entries[0].Pixels.x0;
        sInt baseline=max_baseline-bmp[activefont]->Atlas.Entries[0].Pixels.y0;
        sFRect uvs=bmp[activefont]->Atlas.Entries[j].UVs;
        sRect rect=bmp[activefont]->Atlas.Entries[j].Pixels;

        sF32 px=lpx;

        if (activealign==E_TEXT_ALIGN_RIGHT)
        {
          px=rpx;
        }
        else if (activealign==E_TEXT_ALIGN_CENTER)
        {
          px=cpx;
        }

        if (px>0.0f)
        {
          px+=rect.y0*zoom;
        }

        mf->Init(4);
        mf->Vertex[0]=k+0;
        mf->Vertex[1]=k+1;
        mf->Vertex[2]=k+2;
        mf->Vertex[3]=k+3;    
        mf->Cluster=activefont;
        mf++;

        mv->Init();
        mv->Pos.x = px+rect.x0*zoom;
        mv->Pos.y = py+baseline*zoom;
        mv->U0 = uvs.x1;
        mv->V0 = uvs.y0;
#if !WZ4MESH_LOWMEM
        mv->Color0 = color;
#endif
        mv++;

        mv->Init();
        mv->Pos.x =  px+rect.x0*zoom;
        mv->Pos.y =  py+(height+baseline)*zoom;
        mv->U0 = uvs.x1;
        mv->V0 = uvs.y1;
#if !WZ4MESH_LOWMEM
        mv->Color0 = color;
#endif
        mv++;

        mv->Init();
        mv->Pos.x = px;
        mv->Pos.y = py+(height+baseline)*zoom;
        mv->U0 = uvs.x0;
        mv->V0 = uvs.y1;
#if !WZ4MESH_LOWMEM
        mv->Color0 = color;
#endif
        mv++;

        mv->Init();
        mv->Pos.x = px;
        mv->Pos.y = py+baseline*zoom;
        mv->U0 = uvs.x0;
        mv->V0 = uvs.y0;
#if !WZ4MESH_LOWMEM
        mv->Color0 = color;
#endif
        mv++;

        if (activealign==E_TEXT_ALIGN_RIGHT)
        {
          rpx+=(rect.y0+rect.x0+rect.x1+spacex)*zoom;
        }
        else if (activealign==E_TEXT_ALIGN_CENTER)
        {
          cpx+=(rect.y0+rect.x0+rect.x1+spacex)*zoom;
        }
        else
        {
          lpx+=(rect.y0+rect.x0+rect.x1+spacex)*zoom;
        }
       
        k+=4;        
      }
      break;

      case E_TOKEN_TYPE_NEWLINE :
      {
        lpx=0;
        rpx=(lwidth-strwidth[l])*zoom;
        cpx=((lwidth-strwidth[l++])/2.0f)*zoom;
        py+=max_height;
      }
      break;
      case E_TOKEN_TYPE_ALIGN :
      {
        lpx=0;
        rpx=(lwidth-strwidth[l])*zoom;
        cpx=((lwidth-strwidth[l++])/2.0f)*zoom;
        activealign=token[i].Align;
      }
      break;

    }
  }

  out->CalcNormals();
  out->CalcTangents();
}

