/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "adf.hpp"
#include "adf_ops.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_ipp.hpp"
#include "util/taskscheduler.hpp"

/****************************************************************************/

Wz4ADF::Wz4ADF()
{
  Type    = Wz4ADFType;  
  SDF     = 0;
  Texture = 0;
}

Wz4ADF::~Wz4ADF()
{  
  if (SDF)
    delete SDF;  
  if (Texture)
    delete Texture;
}

sBool Wz4ADF::TraceRay(sVector31 &p, sVector30 &n, const sRay &ray, const sF32 md, const sF32 mx, const sInt mi)
{  
  if (!SDF)
    return false;

  sAABBox box;
  SDF->GetBox(box);

  box.Min.x += 1/8192.0f;
  box.Min.y += 1/8192.0f;
  box.Min.z += 1/8192.0f;
  box.Max.x -= 1/8192.0f;
  box.Max.y -= 1/8192.0f;
  box.Max.z -= 1/8192.0f;

  p = ray.Start;
  sF32 d;  

  sF32 mind=0;
  sF32 maxd=10000.0;      

  if (ray.HitAABB(mind,maxd,box.Min,box.Max))      
  {        
    p = ray.Start + ray.Dir * mind;
    d = 0;                

    n.x = 0.0f;
    n.y = 1.0f;
    n.z = 0.0f;

    while (SDF->IsInBox(p))        
    {          
      d = SDF->GetDistance(p);
      if (d<=1.0f/64.0f)
      {
        SDF->GetNormal(p,n);
        return true;
      }                  
      n.x = 0.0f;
      n.y = 0.0f;
      n.z = 1.0f;
      p = p + ray.Dir * sAbs(d);
    }            
  }
  return false;  
}

void Wz4ADF::FromFile(sChar *name)
{
  SDF = new tSDF();
  SDF->Init(name);
}

Wz4BSPError Wz4ADF::FromMesh(Wz4Mesh *in, sF32 planeThickness, sInt Depth, sF32 GuardBand, sBool ForceCubeSampling, sBool UserBox, const sVector31 &BoxPos, const sVector30 &BoxDimH, sBool BruteForce)
{
  tAABBoxOctree *oct = new tAABBoxOctree(4);
  Wz4BSPError err=oct->FromMesh(in, planeThickness, ForceCubeSampling, UserBox, BoxPos, BoxDimH, BruteForce, GuardBand);

  SDF = new tSDF(); 

  if (err==WZ4BSP_OK)
    SDF->Init(oct,Depth,BruteForce,GuardBand);

  delete oct;
  return err;
}

sTexture3D * Wz4ADF::GetTexture()
{
  if (!Texture)
  {
    sInt rp,sp;
    sU8 *ptr;    
    sF32 *d;

    Texture = new sTexture3D(SDF->DimX,SDF->DimY,SDF->DimZ,sTEX_3D|sTEX_R32F|sTEX_NOMIPMAPS);
    Texture->BeginLoad(ptr,rp,sp,0);

    for(sInt z=0;z<SDF->DimZ;z++)
    {
      for(sInt y=0;y<SDF->DimY;y++)
      {      
        d=(sF32 *) (ptr + y*rp + z*sp);
        for(sInt x=0;x<SDF->DimX;x++)
        {
          d[x]=  SDF->SDF[z*SDF->DimXY+y*SDF->DimX+x];     
        }
      }
    }
    Texture->EndLoad();
  }

  return Texture;
}

/****************************************************************************/

/*

Wz4TPGravity::Wz4TPGravity(Wz4GenParticles *in, Wz4ADF *df)
{  
  sInt count=in->Particles.GetCount();
  Particles.AddMany(count);
  Base = in;
  DF   = df;  
  Reset();
}

Wz4TPGravity::~Wz4TPGravity()
{
}
 
void Wz4TPGravity::Reset()
{
  Wz4GenParticles::Particle *p;

  LastTime = 0;

  sFORALL(Base->Particles,p)
  {
    Particles[_i]=*p;
  }
}


void Wz4TPGravity::Simulate(Wz4RenderContext *ctx)
{
  SimulateCalc(ctx);
}

sInt Wz4TPGravity::GetPartCount(sInt &flags)
{
  return Particles.GetCount();
}

sInt Wz4TPGravity::Func(Wz4Part *part,sQuaternion *orient,sF32 time,sF32 dt)
{ 
  int steps;
  int cnt=0;
  Wz4GenParticles::Particle *p;
    

  if (time<0.001f || time>15.0f)
  {
    steps=0;
    Reset();
  }
  else
  {    
    if (time<LastTime)
    {
      Reset();
      steps=time/SIMSTEP;
      LastTime=steps*SIMSTEP;      
    }    
    else
    {
      steps=(time-LastTime)/SIMSTEP;
      LastTime+=steps*SIMSTEP;
    }    
  }

  sFORALL(Particles,p)
  {    
    float time;
    for (int i=0;i<steps;i++)
    {
      if (p->Time>0 && DF->GetObj()->GetDistance(p->Pos)>0.0f)
      {
        p->Pos.y-=0.005f;
      }
      p->Time++;
    }
    
    if (p->Time>=0)
    {
      time=1.0f;
      cnt++;
    }
    else
    {
      time=-1.0f;
    }
    part[_i].Init(p->Pos,time);
  }
  

  return cnt;
}
*/


void Wz4ADF_Init(int w, int h)
{
}

void Wz4ADF_Exit()
{
}

struct tADF_Render
{
public:
  sVector30 dnx;
  sVector30 dny;
  sVector31 px;  
  sVector31 cp;
  sImage  *img;  
  wPaintInfo *pi;
  Wz4ADF  *adf;
};

void TaskCodeADF(sStsManager *m,sStsThread *th,sInt start,sInt count,void *data)
{
  sVERIFY(count==1);
  tADF_Render *mi = (tADF_Render *)data;
  sRay ray;
  sU32 *ptr = mi->img->Data;

  ptr+=start*mi->img->SizeX;

 // for(sInt y=0;y<img->SizeY;y++)
  {
    for(sInt x=0;x<mi->img->SizeX;x++)
    {
      sVector30 norm;
      sVector31 pos;

      ray.Start = mi->px + mi->dnx*x + mi->dny*start;
      ray.Dir = ray.Start-mi->cp; 
      ray.Dir.Unit();

      //sF32 dist;      
      //mi->pi->View->MakeRay(2.0f*x/mi->img->SizeX-1.0f,1.0f-2.0f*y/img->SizeY,ray2); 

      if (mi->adf->TraceRay(pos,norm,ray))
      {
        norm = (norm + sVector30(1.0f, 1.0f, 1.0f)) * 0.5f;
        unsigned int r = norm.x * 255;
        unsigned int g = norm.y * 255;
        unsigned int b = norm.z * 255;
        *ptr++ = 0xff0000000|(r<<16)|(g<<8)|(b<<0);
      }
      else
      {
        *ptr++ = 0xff000000; 
      }
    }
  }



  //mi->DoBatch(start);

  //sU32 *p = mi->Data + (start * mi->LinesPerBatch * mi->Pitch);
  //sInt v= th->GetIndex();
  //for(sInt i=0;i<6;i++)
  //{
  //  *p++ = (v&1)?0xffff0000 : 0xff00ff00;
  //  *p++ = (v&1)?0xffff0000 : 0xff00ff00;
  //  *p++ = (v&1)?0xffff0000 : 0xff00ff00;
  //  *p++ = (v&1)?0xffff0000 : 0xff00ff00;
  //  v = v>>1;
  //}
}



void Wz4ADF_Render(sImage *img, Wz4ADF  *adf, wPaintInfo &pi)
{
  sRay ray;
  sVector30 normal, dir;

  sRay rtl;
  sRay rtr;
  sRay rtb;
  sRay rcp;
  pi.View->MakeRay(-1, 1,rtl);
  pi.View->MakeRay( 1, 1,rtr);
  pi.View->MakeRay(-1,-1,rtb);
  pi.View->MakeRay(0,0,rcp);
  
  sRay ray2;

  sVector30 dnx= (rtr.Start - rtl.Start) * (1.0f/img->SizeX);
  sVector30 dny= (rtb.Start - rtl.Start) * (1.0f/img->SizeY);
  sVector31 px=rtl.Start;  
  sVector31 cp=pi.View->Camera.l;// rcp.Start;

  tADF_Render mi;

  mi.dnx=dnx;
  mi.dny=dny;
  mi.px=px;
  mi.cp=cp;
  mi.img=img;
  mi.adf=adf;
  mi.pi=&pi;
  
#if 1
 
  sStsWorkload *wl = sSched->BeginWorkload();
  sStsTask *task = wl->NewTask(TaskCodeADF,&mi,img->SizeY,0);
  wl->AddTask(task);
  wl->Start();
  wl->Sync();
  wl->End();
#else
  sU32 *ptr = img->Data;

  for(sInt y=0;y<img->SizeY;y++)
  {
    for(sInt x=0;x<img->SizeX;x++)
    {
      sVector30 norm;
      sVector31 pos;

      ray.Start = px + dnx*x + dny*y;
      ray.Dir = ray.Start-cp; 
      ray.Dir.Unit();

      //sF32 dist;      
      pi.View->MakeRay(2.0f*x/img->SizeX-1.0f,1.0f-2.0f*y/img->SizeY,ray2); 

      if (adf->TraceRay(pos,norm,ray))
      {
        norm = (norm + sVector30(1.0f, 1.0f, 1.0f)) * 0.5f;
        unsigned int r = norm.x * 255;
        unsigned int g = norm.y * 255;
        unsigned int b = norm.z * 255;
        *ptr++ = 0xff0000000|(r<<16)|(g<<8)|(b<<0);
      }
      else
      {
        *ptr++ = 0xff000000; 
      }
    }
  }
#endif

/*

  for(sInt y=0;y<img->SizeY;y++)
  {
    for(sInt x=0;x<img->SizeX;x++)
    {
      sVector30 norm;
      sVector31 pos;
      sF32 dist;
      dir = pi.View->Camera.k;
      pi.View->MakeRay(2.0f*x/img->SizeX-1.0f,1.0f-2.0f*y/img->SizeY,ray);                    
      if (adf->TraceRay(pos,norm,ray))
      {
        norm = (norm + sVector30(1.0f, 1.0f, 1.0f)) * 0.5f;
        unsigned int r = norm.x * 255;
        unsigned int g = norm.y * 255;
        unsigned int b = norm.z * 255;
        *ptr++ = 0xff0000000|(r<<16)|(g<<8)|(b<<0);
      }
      else
      {
        *ptr++ = 0xff000000; 
      }
    }
  }
*/
}

/*

RNRenderADF::RNRenderADF()
{ 
  Anim.Init(Wz4RenderType->Script);
}

RNRenderADF::~RNRenderADF()
{
  delete Mtrl;
  delete texture;
  delete MtrlShadow;
}

void RNRenderADF::Init()
{
  sInt rp,sp;
  sU8 *ptr;
//  sHalfFloat *d;
  sF32 *d;

  Mtrl = new tADFMat();  
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF|sMTRL_NOZRENDER;
  Mtrl->TFlags[0] = sMTF_LEVEL0 | sMTF_CLAMP;
  Mtrl->Prepare(sVertexFormatDouble);  

  MtrlShadow = new tADFShadowMat();
  MtrlShadow->Flags = sMTRL_ZOFF|sMTRL_CULLOFF|sMTRL_NOZRENDER;
  MtrlShadow->TFlags[0] = sMTF_LEVEL0 | sMTF_CLAMP;
  MtrlShadow->TFlags[1] = sMTF_LEVEL0 | sMTF_CLAMP;
  MtrlShadow->Prepare(sVertexFormatDouble);  

  texture = new sTexture3D(SDF->DimX,SDF->DimY,SDF->DimZ,sTEX_3D|sTEX_R32F|sTEX_NOMIPMAPS);
  texture->BeginLoad(ptr,rp,sp,0);

  for(sInt z=0;z<SDF->DimZ;z++)
  {
    for(sInt y=0;y<SDF->DimY;y++)
    {      
      d=(sF32 *) (ptr + y*rp + z*sp);
      for(sInt x=0;x<SDF->DimX;x++)
      {
        d[x]=  SDF->SDF[z*SDF->DimXY+y*SDF->DimX+x];     
      }
    }
  }
  texture->EndLoad();
}

void RNRenderADF::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);
  SimulateChilds(ctx);
  //ctx->RenderFlags |= wRF_RenderZ;  
}

void RNRenderADF::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);
  
  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {    
    sInt sx = ctx->ScreenX;
    sInt sy = ctx->ScreenY;

    sRay rtl;
    sRay rtr;
    sRay rtb;
    sRay rcp;
       
    ctx->View.MakeRay(-1, 1,rtl);
    ctx->View.MakeRay( 1, 1,rtr);
    ctx->View.MakeRay(-1,-1,rtb);
    ctx->View.MakeRay(0,0,rcp);
  
    sRay ray2;

    sVector30 dnx= (rtr.Start - rtl.Start);
    sVector30 dny= (rtb.Start - rtl.Start);
    sVector31 px=rtl.Start;  
    sVector31 cp=ctx->View.Camera.l;// rcp.Start;

    sTexture2D *dest = sRTMan->WriteScreen();
//    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *mask = sRTMan->Acquire(sx,sy,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB16F);
    
    sRTMan->SetTarget(mask);

    //Render Gradient Texture
    sCBuffer<tADFMatVSPara> cbv;
    sCBuffer<tADFMatPSPara> cbp;

    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify(); 

    sF32 imd=1.0f/sMax(sMax(SDF->DimX, SDF->DimY), SDF->DimZ);

    sAABBox box;
    SDF->GetBox(box);

    cbp.Data->dnx=dnx;
    cbp.Data->dny=dny;
    cbp.Data->px=rtl.Start;
    cbp.Data->cp=cp;
    cbp.Data->sp.Init(SDF->STBX,SDF->STBY,SDF->STBZ,0);
    cbp.Data->op.Init(box.Min.x,box.Min.y,box.Min.z,0);
    cbp.Data->mp.Init(box.Max.x,box.Max.y,box.Max.z,0);
    cbp.Data->id.Init(1.0f/(SDF->DimX-1),1.0f/(SDF->DimY-1),1.0f/(SDF->DimZ-1),imd);
    cbp.Data->d.Init(SDF->DimX,SDF->DimY,SDF->DimZ,0);
    cbp.Data->pstep.Init(SDF->PStepX,SDF->PStepY,SDF->PStepZ,0);

    cbp.Modify();
    Mtrl->Texture[0]=texture;
    Mtrl->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest);
    Mtrl->Texture[0]=0;



    sRTMan->SetTarget(dest);
    sCBuffer<tADFShadowMatVSPara> cbv_shadow;
    sCBuffer<tADFShadowMatPSPara> cbp_shadow;

    ctx->IppHelper->GetTargetInfo(cbv_shadow.Data->mvp);
    cbv_shadow.Modify(); 

    cbp_shadow.Data->dnx=dnx;
    cbp_shadow.Data->dny=dny;
    cbp_shadow.Data->px=rtl.Start;
    cbp_shadow.Data->cp=cp;    

    cbp_shadow.Data->sp.Init(SDF->STBX,SDF->STBY,SDF->STBZ,0);
    cbp_shadow.Data->op.Init(box.Min.x,box.Min.y,box.Min.z,0);
    cbp_shadow.Data->mp.Init(box.Max.x,box.Max.y,box.Max.z,0);
    cbp_shadow.Data->id.Init(1.0f/(SDF->DimX-1),1.0f/(SDF->DimY-1),1.0f/(SDF->DimZ-1),imd);
    cbp_shadow.Data->d.Init(SDF->DimX,SDF->DimY,SDF->DimZ,0);
    cbp_shadow.Data->pstep.Init(SDF->PStepX,SDF->PStepY,SDF->PStepZ,0);


    cbp_shadow.Data->light=Para.LightPos;
    cbp_shadow.Data->phongpara.Init(Para.PhongMax,Para.PhongPower,0,0);
    cbp_shadow.Data->diffusecolor.InitColor(Para.DiffuseColor);
    cbp_shadow.Data->ambientcolor.InitColor(Para.AmbientColor);
    cbp_shadow.Data->speccolor.InitColor(Para.SpecColor);



    //cbp_shadow.Data->
    cbp_shadow.Modify();

    MtrlShadow->Texture[0]=texture;
    MtrlShadow->Texture[1]=mask;
    MtrlShadow->Set(&cbv_shadow,&cbp_shadow);
    ctx->IppHelper->DrawQuad(dest,mask);
    MtrlShadow->Texture[0]=0;
    MtrlShadow->Texture[1]=0;

    sRTMan->Release(mask);
  //  sRenderTargetManager->Release(src);    
    sRTMan->Release(dest);
  } 
}

*/
