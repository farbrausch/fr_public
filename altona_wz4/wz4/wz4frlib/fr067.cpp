/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1
#include "fr067.hpp"

/****************************************************************************/

RNFR067_IsoSplash::RNFR067_IsoSplash() : MC(0)
{
  Anim.Init(Wz4RenderType->Script);  

  for(sInt i=0;i<4;i++)
    Mtrl[i] = 0;

  MaxThread = sSched->GetThreadCount();

  NoiseXY = 0;
  NoiseYZ = 0;
  NoiseZX = 0;
  RubberMat = 0;
  PolarPhi = 0;
}

RNFR067_IsoSplash::~RNFR067_IsoSplash()
{
  for(sInt i=0;i<4;i++)
    Mtrl[i]->Release();

  delete[] NoiseXY;
  delete[] NoiseYZ;
  delete[] NoiseZX;
  delete[] RubberMat;
  delete[] PolarPhi;
}

void RNFR067_IsoSplash::Init()
{
  MC.Init(0,0);
  Size = (1<<Para.OctreeDivisions)*8+3;

  NoiseXY = new sF32[Size*Size];
  NoiseYZ = new sF32[Size*Size];
  NoiseZX = new sF32[Size*Size];
  RubberMat = new sMatrix34[Size];
  PolarPhi = new sF32[Size*Size];
}

void RNFR067_IsoSplash::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

static void MarchIsoTS(sStsManager *,sStsThread *thread,sInt start,sInt count,void *data)
{
  ((RNFR067_IsoSplash *)data)->MarchTask(thread,start,count);
}

void RNFR067_IsoSplash::Prepare(Wz4RenderContext *ctx)
{
  for(sInt i=0;i<4;i++)
  {
    if(Mtrl[i]) 
      Mtrl[i]->BeforeFrame(Para.LightEnv);
  }

  SphereEnable = (Para.SphereAmp!=0.0f);
  SphereAmp.x = 1.0f / Para.SphereDirections.x;
  SphereAmp.y = 1.0f / Para.SphereDirections.y;
  SphereAmp.z = 1.0f / Para.SphereDirections.z;
  SphereAmp = SphereAmp * SphereAmp;

  CubeEnable = (Para.CubeAmp!=0.0f);
  CubeAmp.x = 1.0f / Para.CubeDirections.x;
  CubeAmp.y = 1.0f / Para.CubeDirections.y;
  CubeAmp.z = 1.0f / Para.CubeDirections.z;

  NoiseEnable = (Para.NoiseAmp1!=0.0f) || (Para.NoiseAmp2!=0.0f);
  NoiseFreq1 = Para.NoiseFreq1*0x10000;
  NoisePhase1 = Para.NoisePhase1*0x10000;
  NoiseAmp1 = Para.NoiseAmp1;
  NoiseFreq2 = Para.NoiseFreq2*0x10000;
  NoisePhase2 = Para.NoisePhase2*0x10000;
  NoiseAmp2 = Para.NoiseAmp2;

  RotEnable = 0;
  RubberEnable = 0;
  if(Para.Rot.x!=0 || Para.Rot.y!=0 || Para.Rot.z!=0)
    RotEnable = 1;
  if(Para.Rubber.x!=0 || Para.Rubber.y!=0 || Para.Rubber.z!=0)
    RotEnable = RubberEnable = 1;

  PolarEnable = (Para.PolarAmp!=0.0f);


  for(sInt i=0;i<Size;i++)
  {
    sF32 py = sF32(i-1)/sF32((1<<Para.OctreeDivisions)*8);
    RubberMat[i].EulerXYZ((Para.Rot.x+Para.Rubber.x*py)*sPI2F,
                          (Para.Rot.y+Para.Rubber.y*py)*sPI2F,
                          (Para.Rot.z+Para.Rubber.z*py)*sPI2F);
  }

  sF32 f = 1.0f/((1<<Para.OctreeDivisions)*8);
  for(sInt y=0;y<Size;y++)
  {
    for(sInt x=0;x<Size;x++)
    {
      sF32 px = (x-1)*f*2-1;
      sF32 py = (y-1)*f*2-1;

      NoiseXY[Size*y+x] = NoiseAmp1*sPerlin2D(sInt(px*NoiseFreq1.x+NoisePhase1.x),
                                              sInt(py*NoiseFreq1.y+NoisePhase1.y),255,Para.NoiseSeed1)
                        + NoiseAmp2*sPerlin2D(sInt(px*NoiseFreq2.x+NoisePhase2.x),
                                              sInt(py*NoiseFreq2.y+NoisePhase2.y),255,Para.NoiseSeed2);

      NoiseYZ[Size*y+x] = NoiseAmp1*sPerlin2D(sInt(px*NoiseFreq1.y+NoisePhase1.y),
                                              sInt(py*NoiseFreq1.z+NoisePhase1.z),255,Para.NoiseSeed1)
                        + NoiseAmp2*sPerlin2D(sInt(px*NoiseFreq2.y+NoisePhase2.y),
                                              sInt(py*NoiseFreq2.z+NoisePhase2.z),255,Para.NoiseSeed2);

      NoiseZX[Size*y+x] = NoiseAmp1*sPerlin2D(sInt(px*NoiseFreq1.z+NoisePhase1.z),
                                              sInt(py*NoiseFreq1.x+NoisePhase1.x),255,Para.NoiseSeed1)
                        + NoiseAmp2*sPerlin2D(sInt(px*NoiseFreq2.z+NoisePhase2.z),
                                              sInt(py*NoiseFreq2.x+NoisePhase2.x),255,Para.NoiseSeed2);

      PolarPhi[Size*y+x] = sFSin(sATan2(px,py)*Para.PolarXZ*0.5f);
    }
  }



  // do it

  MakeNodes();

  March();

  sAABBoxC box;
  sF32 s = Para.GridSize/2.0f;
  box.Radius.Init(s,s,s);
  box.Center.Init(0,0,0);
  for(sInt i=0;i<4;i++)
    if(Mtrl[i])
      Mtrl[i]->BeforeFrame(Para.LightEnv,1,&box,Matrices.GetCount(),Matrices.GetData());
}

void RNFR067_IsoSplash::MakeNodes()
{
  sInt n = 1<<Para.OctreeDivisions;

  Nodes.Clear();
  sF32 s = 2.0f/n;
  sF32 b = -1.0f;
  for(sInt z=0;z<n;z++)
  {
    for(sInt y=0;y<n;y++)
    {
      for(sInt x=0;x<n;x++)
      {
        IsoNode *n = Nodes.AddMany(1);
        n->Min.Init((x+0)*s+b,(y+0)*s+b,(z+0)*s+b);
        n->Max.Init((x+1)*s+b,(y+1)*s+b,(z+1)*s+b);
        n->px = x*8;
        n->py = y*8;
        n->pz = z*8;
      }
    }
  }
}

void RNFR067_IsoSplash::March()
{
  MC.Begin();


  if(Para.Multithreading&1)
  {
    sStsWorkload *wl = sSched->BeginWorkload();
    sInt count = Nodes.GetCount();
    sStsTask *task = wl->NewTask(MarchIsoTS,this,count,0);
    task->Granularity = sMax(1,count/64);
    wl->AddTask(task);
    wl->Start();
    while(sSched->HelpWorkload(wl))
    {
      MC.ChargeGeo();
    }
    wl->Sync();
    wl->End();
  }
  else
  {
    MarchIsoTS(0,0,0,Nodes.GetCount(),this);
  }

  MC.End();
}

sF32 RNFR067_IsoSplash::func(const sVector31 &pp,sInt px,sInt py,sInt pz)
{
  sVector31 p;
  if(RotEnable)
    p = pp*RubberMat[py];
  else
    p = pp;

  sF32 f = 0;

  if(SphereEnable)
    f += Para.SphereAmp/sFSqrt(p.x*p.x*SphereAmp.x + p.y*p.y*SphereAmp.y + p.z*p.z*SphereAmp.z);

  if(CubeEnable)
    f += Para.CubeAmp/sMax(sMax(sFAbs(p.x*CubeAmp.x),sFAbs(p.y*CubeAmp.y)),sFAbs(p.z*CubeAmp.z));

  if(NoiseEnable)
  {
    f += NoiseXY[py*Size+px];
    f += NoiseYZ[pz*Size+py];
    f += NoiseZX[px*Size+pz];
  }

  if(PolarEnable)
  {
    sF32 a = 0;
    if(Para.PolarY!=0)
    {
      sF32 rr = sSqrt(p.x*p.x+p.z*p.z);
      sF32 aa = sATan2(p.y,rr);
      a = sFCos(aa*Para.PolarY);
    }
    sF32 b = PolarPhi[px*Size+pz];//sFSin(sATan2(p.x,p.z)*Para.PolarXZ*0.5f);
    sF32 d = sFSqrt(p.x*p.x + p.y*p.y + p.z*p.z)-Para.PolarCenter;

    f += Para.PolarAmp/sMax(0.0f,sFSqrt(a*a + b*b + d*d*Para.PolarCenterAmount));
  }

  return f-Para.IsoValue;
}


void RNFR067_IsoSplash::MarchTask(sStsThread *thread,sInt start,sInt count)
{
  sInt id = thread ? thread->GetIndex() : 0;
  sSchedMon->Begin(id,0xff8080);
  const sInt s1 = CellSize+1;
  const sInt s2 = CellSize+2;
  const sInt s3 = CellSize+3;
  for(sInt ii=start;ii<start+count;ii++)
  {
    IsoNode *node = &Nodes[ii];

    sF32 pots[s3][s3][s3];
    sVector31 p0 = node->Min;
    sVector31 pp;

    //  calculate some coefficients

    sF32 pd = (node->Max.x-node->Min.x)/CellSize;
    p0.x -= pd;
    p0.y -= pd;
    p0.z -= pd;

    sMatrix34 mat[s3];

    if(RubberEnable)
    {
      for(sInt y=0;y<s3;y++)
      {
         sF32 py = p0.y+pd*y;
         mat[y].EulerXYZ((Para.Rot.x+Para.Rubber.x*py)*sPI2F,
                         (Para.Rot.y+Para.Rubber.y*py)*sPI2F,
                         (Para.Rot.z+Para.Rubber.z*py)*sPI2F);
      }
    }
    else
    {
      mat[0].EulerXYZ(Para.Rot.x*sPI2F,Para.Rot.y*sPI2F,Para.Rot.z*sPI2F);
      for(sInt y=1;y<s3;y++)
        mat[y] = mat[0];
    }

    // check some points

    sInt testpoints[9][3] =
    {
      { 5,5,5 },
      { 1,1,1 },
      { 1,1,9 },
      { 1,9,1 },
      { 1,9,9 },
      { 9,1,1 },
      { 9,1,9 },
      { 9,9,1 },
      { 9,9,9 },
    };

    sF32 pmin = 1000;
    sF32 pmax = -1000;
    for(sInt i=0;i<9;i++)
    {
      pp.x = p0.x+pd*testpoints[i][0];
      pp.y = p0.y+pd*testpoints[i][1];
      pp.z = p0.z+pd*testpoints[i][2];
      sF32 f = func(pp
        ,testpoints[i][0]+node->px
        ,testpoints[i][1]+node->py
        ,testpoints[i][2]+node->pz);
      pmin = sMin(pmin,f);
      pmax = sMax(pmax,f);       
    }
    if(!(pmax>-Para.QuickOutSaveGuard && pmin<Para.QuickOutSaveGuard))
      continue;


    // generate volume

    pmin = 1000;
    pmax = -1000;
    for(sInt z=0;z<s3;z++)
    {
      for(sInt y=0;y<s3;y++)
      {
        for(sInt x=0;x<s3;x++)
        {
          pp.x = p0.x+pd*x;
          pp.y = p0.y+pd*y;
          pp.z = p0.z+pd*z;
          sF32 f = func(pp,x+node->px
                          ,y+node->py
                          ,z+node->pz);
          pots[z][y][x] = f;
          
          pmin = sMin(pmin,f);
          pmax = sMax(pmax,f);       
        }
      }
    }

    if(!(pmax>0 && pmin<0))
      continue;

    // generate normals

    MCPotField pot[s1+1][s1][s1];
    sVector30 n;

    for(sInt z=1;z<s2;z++)
    {
      for(sInt y=1;y<s2;y++)
      {
        for(sInt x=1;x<s2;x++)
        {
          sF32 w = pots[z][y][x];
          n.x = pots[z][y][x+1] - pots[z][y][x-1];
          n.y = pots[z][y+1][x] - pots[z][y-1][x];
          n.z = pots[z+1][y][x] - pots[z-1][y][x];

          sF32 e = n.x*n.x + n.y*n.y + n.z*n.z;

          if(e>1e-24f)
          {
            e=sFRSqrt(e);
            n.x = e*n.x;
            n.y = e*n.y;
            n.z = e*n.z;
          }
          else
          {
            n.Init(0,0,0);
          }

          pot[z-1][y-1][x-1].x = -n.x;
          pot[z-1][y-1][x-1].y = -n.y;
          pot[z-1][y-1][x-1].z = -n.z;
          pot[z-1][y-1][x-1].w = w;
        }
      }
    }

    // marching cubes

    MC.March(3,&pot[0][0][0],pd*Para.GridSize*0.5f,sVector31(sVector30(node->Min)*(Para.GridSize*0.5f)),id);
  }
  sSchedMon->End(id);
}

void RNFR067_IsoSplash::Render(Wz4RenderContext *ctx)
{
//  if(!ctx->IsCommonRendermode()) return;

  sMatrix34CM *model;
  sFORALL(Matrices,model)
  {
    for(sInt i=0;i<4;i++)
    {
      if(Mtrl[i] && !Mtrl[i]->SkipPhase(ctx->RenderMode,Para.LightEnv))
      {
        sInt n = sClamp(Para.Shells[i],1,256);
        for(sInt j=0;j<n;j++)
        {
          Mtrl[i]->ShellExtrude = j/sF32(n);
          Mtrl[i]->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv,model,0,0,0);
          MC.Draw();
        }
      }
    }
  }
}


/****************************************************************************/

