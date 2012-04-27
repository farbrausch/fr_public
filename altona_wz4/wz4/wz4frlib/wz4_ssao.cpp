/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_ssao.hpp"
#include "wz4frlib/wz4_demo_ops.hpp"

#if sPLATFORM!=sPLAT_PS3

/****************************************************************************/

static const sU32 VertXYZWDesc[] =
{
  sVF_POSITION|sVF_F4,
  sVF_END
};

static sVector30 RandomDir(sRandomMT &rnd)
{
  sVector30 r;
  sF32 l;

  do
  {
    r.x = rnd.Float(2.0f) - 1.0f;
    r.y = rnd.Float(2.0f) - 1.0f;
    r.z = rnd.Float(2.0f) - 1.0f;
    l = r.LengthSq();
  } while(l==0.0f || l>1.0f);

  r *= sRSqrt(l);
  return r;
}

Wz4SsaoCamera::Wz4SsaoCamera()
{
  Type = Wz4ViewportType;
  Position.Init(0,0,-5);
  Target.Init(0,0,0);
  Zoom = 1.7f;
  ClipNear = 0.125f;
  ClipFar = 4096;
  ClearColor = 0xff000000;

  Node = 0;
  AnimTarget = 0;
  AnimPosition = 0;
  Clip = 0;

  FmtXYZW = sCreateVertexFormat(VertXYZWDesc);

  DepthMaterial = new Wz4SSAODepthShader;
  DepthMaterial->Prepare(sVertexFormatTSpace4);

  NormalMaterial = new Wz4SSAONormalShader;
  NormalMaterial->Flags = sMTRL_ZOFF;
  NormalMaterial->Prepare(FmtXYZW);

  SSAOMaterial = new Wz4SSAOShader;
  SSAOMaterial->TFlags[0] = sMTF_LEVEL1|sMTF_BORDER_WHITE|sMTF_EXTERN;
  SSAOMaterial->TFlags[1] = sMTF_LEVEL0|sMTF_TILE|sMTF_EXTERN;
  SSAOMaterial->Prepare(sVertexFormatTSpace4);

  SSAOBlurMaterial = new Wz4SSAOBlurShader;
  SSAOBlurMaterial->TFlags[0] = sMTF_LEVEL1|sMTF_CLAMP|sMTF_EXTERN;
  SSAOBlurMaterial->Prepare(FmtXYZW);

  QuadGeo = new sGeometry;
  QuadGeo->Init(sGF_QUADLIST,FmtXYZW);

  sVector4 *verts;
  QuadGeo->BeginLoadVB(4,sGD_STATIC,&verts);
  for(sInt i=0;i<4;i++)
  {
    sInt j = i^(i>>1);
    verts[i].x = (j&1) ? 1.0f : -1.0f;
    verts[i].y = (j&2) ? -1.0f : 1.0f;
    verts[i].z = 0.0f;
    verts[i].w = 1.0f;
  }
  QuadGeo->EndLoadVB();

  RTDepth = 0;
  RTNormal = 0;
  RTSSAO = 0;

  // create random direction texture
  static const sInt RandomSize=4;
  RandomTex = new sTexture2D(RandomSize,RandomSize,sTEX_2D|sTEX_ARGB16F|sTEX_NOMIPMAPS);
  sRandomMT rnd;
  rnd.Seed(0);

  sU8 *ptr;
  sInt pitch;
  RandomTex->BeginLoad(ptr,pitch);
  for(sInt y=0;y<RandomSize;y++)
  {
    sHalfFloat *d = (sHalfFloat*) (ptr+y*pitch);
    for(sInt x=0;x<RandomSize;x++)
    {
      sVector30 dir = RandomDir(rnd);
      d[x*4+0].Set(dir.x);
      d[x*4+1].Set(dir.y);
      d[x*4+2].Set(dir.z);
      d[x*4+3].Set(0.0f);
    }
  }
  RandomTex->EndLoad();

  // and random constant buffer
  RandomCB.Modify();
//  sInt n = sCOUNTOF(RandomCB.Data->sampleVec);
  for(sInt i=0;i<sCOUNTOF(RandomCB.Data->sampleVec);i++)
  {
    sVector30 vec = RandomDir(rnd) * (rnd.Float(0.5f) + 0.5f);
    RandomCB.Data->sampleVec[i] = RandomDir(rnd);
  }
}

Wz4SsaoCamera::~Wz4SsaoCamera()
{
  Node->Release();
  AnimTarget->Release();
  AnimPosition->Release();
  Clip->Release();

  sDelete(QuadGeo);
  sDelete(RTDepth);
  sDelete(RTNormal);
  sDelete(RTSSAO);
  sDelete(RandomTex);

  sDelete(DepthMaterial);
  sDelete(NormalMaterial);
  sDelete(SSAOMaterial);
  sDelete(SSAOBlurMaterial);
}

sBool Wz4SsaoCamera::Paint(const sRect *window,sInt time,const sViewport *free)
{
  struct PaintJob
  {
    sMatrix34 *mat;
    ChaosMeshCluster *cl;
  };

  sViewport view;
  if(Node && (Clip==0 || Clip->Active))
  {
    SceneMatrices sm;
    SceneInstances *inst;
    SceneInstance *i;

    sm.Seed();
    Node->Recurse(&sm);

    sBool doBlur = MaxTime != 0;

    sVector31 pos = Position;
    sVector31 tar = Target;
    if(AnimTarget) tar = sVector31(AnimTarget->Value);
    if(AnimPosition) pos = sVector31(AnimPosition->Value);
    sMatrix34 mat;
    mat.LookAt(tar,pos);

    sInt rtX,rtY;
    rtX = window->SizeX(); rtY = window->SizeY();
    if(!RTSSAO || RTSSAO->SizeX != rtX || RTSSAO->SizeY != rtY)
    {
      sDelete(RTSSAO);
      RTSSAO = new sTexture2D(rtX,rtY,sTEX_ARGB8888|sTEX_2D|sTEX_RENDERTARGET|sTEX_NOMIPMAPS);
      sEnlargeZBufferRT(rtX,rtY);
    }

    rtX = window->SizeX()/3; rtY = window->SizeY()/3;
    if(!RTDepth || RTDepth->SizeX != rtX || RTDepth->SizeY != rtY)
    {
      sDelete(RTDepth);
      RTDepth = new sTexture2D(rtX,rtY,sTEX_R32F|sTEX_2D|sTEX_RENDERTARGET|sTEX_NOMIPMAPS);
      sEnlargeZBufferRT(rtX,rtY);
    }

    sSetRendertarget(window,sCLEAR_NONE);
    view.Camera = mat;
    view.ClipNear = ClipNear;
    view.ClipFar = ClipFar;
    if(free)
      view = *free;
    view.SetTargetCurrent();
    view.SetZoom(Zoom);
    Wz4ShaderEnv env;
    env.Set(&view,1);

    // flatten the hiearchy
    sArray<PaintJob> jobs;

    sFORALL(sm.Instances,inst)
    {
      if(inst->Object && inst->Object->Type==ChaosMeshType)
      {
        ChaosMesh *cm = (ChaosMesh *) inst->Object;
        cm->Charge();

        sFORALL(inst->Matrices,i)
        {
          env.Set(&view,i->Matrix);
                  
          ChaosMeshCluster *cl;
          sFORALL(cm->Clusters,cl)
          {
//            if(env.Visible(cl->Bounds))
            {
              PaintJob *job = jobs.AddMany(1);
              job->mat = &i->Matrix;
              job->cl = cl;
            }
          }
        }
      }
    }

    // paint depths
    sSetRendertarget(0,RTDepth,sCLEAR_ZBUFFER|sRTZBUF_FORCEMAIN,0xffffffff);
    view.SetTarget(RTDepth);
    view.Prepare();

    sCBuffer<Wz4SSAOShaderCamera> cb_cam;
    sCBuffer<Wz4SSAOShaderPixel> cb_pixel;

    cb_cam.Modify();
    cb_cam.Data->Set(view);
    cb_pixel.Modify();
    cb_pixel.Data->Set(view);

    for(sInt i=0;i<jobs.GetCount();i++)
    {
      env.Set(&view,*jobs[i].mat);
      ChaosMeshCluster *cl = jobs[i].cl;

      DepthMaterial->Set(&cb_cam,&cb_pixel); 
      cl->Geo->Draw();
    }

    /*// paint normals
    sSetRendertarget(0,RTNormal,sCLEAR_ALL);
    view.SetTarget(RTNormal);
    view.Prepare();
    cb_pixel.Modify();
    cb_pixel.Data->Set(view);

    NormalMaterial->Texture[0] = RTDepth;
    NormalMaterial->Set(&cb_cam,&cb_pixel);
    QuadGeo->Draw();*/

    // paint ssao
    if(!doBlur)
    {
      sSetRendertarget(window,sCLEAR_ALL);
      view.SetTargetCurrent();
      view.SetZoom(Zoom);
    }
    else
    {
      sSetRendertarget(0,RTSSAO,sCLEAR_ALL);
      view.SetTarget(RTSSAO);
    }
    view.Prepare();
    cb_cam.Modify();
    cb_cam.Data->Set(view);
    cb_pixel.Modify();
    cb_pixel.Data->Set(view);

    SSAOMaterial->Texture[0] = RTDepth;
    SSAOMaterial->Texture[1] = RandomTex;

    for(sInt i=0;i<jobs.GetCount();i++)
    {
      env.Set(&view,*jobs[i].mat);
      ChaosMeshCluster *cl = jobs[i].cl;

      SSAOMaterial->Set(&cb_cam,&cb_pixel,&RandomCB); 
      cl->Geo->Draw();
    }

    if(doBlur)
    {
      sSetRendertarget(window,sCLEAR_ALL);
      view.SetTargetCurrent();
      view.SetZoom(Zoom);
      view.Prepare();
      cb_pixel.Modify();
      cb_pixel.Data->Set(view);

      SSAOBlurMaterial->Texture[0] = RTSSAO;
      SSAOBlurMaterial->Set(&cb_cam,&cb_pixel);
      QuadGeo->Draw();
    }

    return 1;
  }
  else
  {
    return 0;
  }
}


/****************************************************************************/

#endif
