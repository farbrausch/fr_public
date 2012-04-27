/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "easter.hpp"
#include "easter_shader.hpp"
#include "wz4_modmtrl_ops.hpp"

/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   CubeTrees                                                          ***/
/***                                                                      ***/
/****************************************************************************/

const sU32 RNCubeTrees::CubeVertexDecl[]  = { sVF_POSITION,sVF_NORMAL,sVF_TANGENT|sVF_F4,sVF_UV0,0 };
sVertexFormatHandle *RNCubeTrees::CubeVertexFormat = 0;

RNCubeTrees::RNCubeTrees()
{
  if (!CubeVertexFormat) CubeVertexFormat = sCreateVertexFormat(CubeVertexDecl);

  TreeGeo = new sGeometry;
  TreeGeo->Init(sGF_TRILIST|sGF_INDEX32,sVertexFormatStandard);
  CubeGeo = new sGeometry;
  CubeGeo->Init(sGF_TRILIST|sGF_INDEX32,CubeVertexFormat);

  TreeMtrl = 0;
  CubeMtrl = 0;

  Anim.Init(Wz4RenderType->Script);
}

RNCubeTrees::~RNCubeTrees()
{
  sRelease(TreeMtrl);
  sRelease(CubeMtrl);
  sDelete(TreeGeo);
  sDelete(CubeGeo);
}

/****************************************************************************/

void RNCubeTrees::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  Time = ctx->GetTime();

  CubeMatrices.HintSize(50*Array.GetCount());

  //SimulateChilds(ctx);
  CubeRotMat.EulerXYZ(Para.CubeRot.x*sPIF, Para.CubeRot.y*sPIF, Para.CubeRot.z*sPIF);

}


void RNCubeTrees::MakeBranch(sInt depth, sInt seed, sF32 time, sF32 width, sVector31 pos, sVector30 dir, sVector30Arg ref, sVertexStandard *&vp, sU16 *&ip, sInt &ic, sInt &vc)
{
  if (time<=0.0f || pos.y<0.0f) return;

  sF32 factor = sFPow(Para.WidthDecay,depth);
  sF32 speed =  Para.GrowSpeed * factor;
  sF32 grow = sMax(0.0f, time * speed);
  sF32 slen = Para.SegmentLength * factor;

  sRandom rnd; rnd.Seed(Para.Seed+100*depth+seed);

  sQuaternion q;
  q.Init(ref, Para.Bend);

  // lower vertices
  if (vp)
  {
    sVector30 r1 = ref*width;
    sVector30 r2 = r1 % dir; r2.Unit(); r2*=width;
    vp++->Init(pos-r1-r2,-r1,0,grow);
    vp++->Init(pos-r1+r2,-r1,1,grow);
    vp++->Init(pos-r1+r2, r2,0,grow);
    vp++->Init(pos+r1+r2, r2,1,grow);
    vp++->Init(pos+r1+r2, r1,0,grow);
    vp++->Init(pos+r1-r2, r1,1,grow);
    vp++->Init(pos+r1-r2,-r2,0,grow);
    vp++->Init(pos-r1-r2,-r2,1,grow);
  }
  sInt basev = vc;
  vc+=8;

  // branch vertices
  sInt sn=0;
  while (grow>0 && width>=Para.MinWidth && pos.y>=0)
  {
    sF32 seg = sMin(grow,slen);
    sF32 fade = seg/slen;
    pos+=dir*seg;
    width*=sPow(Para.WidthDecay,fade);
    grow-=seg;
    dir*=q;

    if (vp)
    {
      sVector30 r1 = ref*width;
      sVector30 r2 = r1 % dir; r2.Unit(); r2*=width;

      vp++->Init(pos-r1-r2,-r1,0,grow);
      vp++->Init(pos-r1+r2,-r1,1,grow);
      vp++->Init(pos-r1+r2, r2,0,grow);
      vp++->Init(pos+r1+r2, r2,1,grow);
      vp++->Init(pos+r1+r2, r1,0,grow);
      vp++->Init(pos+r1-r2, r1,1,grow);
      vp++->Init(pos+r1-r2,-r2,0,grow);
      vp++->Init(pos-r1-r2,-r2,1,grow);

      sF32 offs = vc-basev;
      sQuad(ip, basev+0, 0, 1, offs+1, offs);
      sQuad(ip, basev+2, 0, 1, offs+1, offs);
      sQuad(ip, basev+4, 0, 1, offs+1, offs);
      sQuad(ip, basev+6, 0, 1, offs+1, offs);
    }
    basev=vc;
    ic+=24;
    vc+=8;

    sInt code = rnd.Int32();
    if (grow>seg && pos.y>(width+Para.CubeSize) && !(code&3))
    {
      sF32 delay = ((code>>8)&0xff)/255.0f;
      sF32 time2 = (grow-seg)/speed-delay;
      if (time2>0)
      {

        // branch off?
        if (depth<2)
        {
          sF32 w2 = width*Para.WidthDecay;
          if (w2>Para.MinWidth)
          {
            sVector30 dir2 = ref;
            sVector30 ref2 = ref%dir; ref2.Unit();
            if (sFAbs(dir2.y)>=0.71f) sSwap(dir2,ref2); // branch goes to the sides
            if (code & 0x10) dir2=-dir2;
            if (code & 0x20) ref2=dir;
            if (code & 0x40) ref2=-ref2;

            MakeBranch(depth+1, seed+sn, time2,w2,pos+w2*dir2,dir2,ref2,vp,ip,ic,vc);
          }
        }

        // cube?
        if (depth==2 && vp && !CubeMatrices.IsFull() && (sn&1))
        {
          sVector30 cubedir = ref;
          if (sFAbs(cubedir.y)<0.7f) { cubedir = ref % dir; cubedir.Unit(); }
          if (cubedir.y>0) cubedir=-cubedir;

          sVector31 cubepos = pos+cubedir*width;

          sMatrix34 *mat = CubeMatrices.AddMany(1);
          mat->i=dir;
          mat->j=cubedir;
          mat->k=mat->i%mat->j;
          mat->l.Init(0,0,0);

          *mat = CubeRotMat**mat;

          sF32 window = sSquare(sFCos(sMin(1.0f,time2/Para.CubeGrowTime)*sPIF/2.0f));
          sF32 sx = time2*Para.CubeWobbleTime*sPIF;
          sF32 size = Para.CubeSize*(1.0f-window*(sFSin(sx)/sx));

          mat->Scale(size);
          mat->l=cubepos;
        }
      }
    }
    sn++;

  };

  // cap
  if (vp)
  {
    sVector30 r1 = ref * width;
    sVector30 r2 = r1 % dir; r2.Unit(); r2*=width;
    sVector30 n = dir; n.Unit();
    vp++->Init(pos-r1-r2, n, 0, 0);
    vp++->Init(pos-r1+r2, n, 0, 1);
    vp++->Init(pos+r1+r2, n, 1, 1);
    vp++->Init(pos+r1-r2, n, 1, 0);
    sQuad(ip,vc,0,1,2,3);
  }
  vc+=4;
  ic+=6;

}


void RNCubeTrees::Prepare(Wz4RenderContext *ctx)
{
  if(TreeMtrl) TreeMtrl->BeforeFrame(Para.LightEnv);
  if(CubeMtrl) CubeMtrl->BeforeFrame(Para.LightEnv);

  sVertexStandard *vp;
  sU16 *ip;

  TreeOk=sFALSE;
  CubeOk=sFALSE;
  CubeMatrices.Clear();

  // count tree vertices
  sInt vc=0;
  sInt ic=0;
  vp=0;
  ip=0;
  for (sInt i=0; i<Array.GetCount(); i++)
    MakeBranch(0,Array[i].Seed,Time-i*Para.SpawnDelay,Para.StartWidth,sVector31(0,0,0),sVector30(0,1,0),sVector30(1,0,0),vp,ip,ic,vc);

  // then make trees
  if (ic && vc)
  {
    TreeGeo->BeginLoad(sVertexFormatStandard,sGF_INDEX16|sGF_TRILIST,sGD_FRAME,vc,ic,&vp,&ip);
    ic=vc=0;
    for (sInt i=0; i<Array.GetCount(); i++)
    {
      Wz4RenderArrayCubeTrees &inst = Array[i];
      sF32 rs, rc;
      sFSinCos(inst.Rot*sPI2F,rs,rc);
      MakeBranch(0,inst.Seed,Time-i*Para.SpawnDelay,Para.StartWidth,inst.Pos,sVector30(0,1,0),sVector30(rc,0,rs),vp,ip,ic,vc);
    }
    TreeGeo->EndLoad(vc,ic);
    TreeOk=sTRUE;
  }
    
  // make cubes
  sInt nCubes = CubeMatrices.GetCount();
  if (nCubes)
  {
    CubeVertex *vp;

    CubeOk=sTRUE;
    CubeGeo->BeginLoad(CubeVertexFormat,sGF_INDEX16|sGF_TRILIST,sGD_FRAME,24*nCubes,36*nCubes,&vp,&ip);
    sInt base=0;
    for (sInt i=0; i<nCubes; i++)
    {
      static const sInt codes[6][4]={0,1,2,3, 1,5,3,7, 5,4,7,6, 4,0,6,2, 3,7,2,6, 5,1,4,0, };

      sMatrix34 &m=CubeMatrices[i];

      sVector31 v[8];
      v[0]=m.l            ;
      v[1]=m.l+m.i        ;
      v[2]=m.l    +m.j    ;
      v[3]=m.l+m.i+m.j    ;
      v[4]=m.l        +m.k;
      v[5]=m.l+m.i    +m.k;
      v[6]=m.l    +m.j+m.k;
      v[7]=m.l+m.i+m.j+m.k;

      sVector30 ns[6];
      sVector4 ts[6];
      ns[0]=-m.k; ns[0].Unit(); ts[0].Init(m.i.x,m.i.y,m.i.z,1);
      ns[1]=m.i;  ns[1].Unit(); ts[1].Init(m.k.x,m.k.y,m.k.z,-1);
      ns[2]=-ns[0];             ts[2].Init(-m.i.x,-m.i.y,-m.i.z,-1);
      ns[3]=-ns[1];             ts[3].Init(-m.k.x,-m.k.y,-m.k.z,1);
      ns[4]=m.j;  ns[4].Unit(); ts[4].Init(m.k.x,m.k.y,m.k.z,1);
      ns[5]=-ns[4];             ts[5].Init(-m.k.x,-m.k.y,-m.k.z,-1);

      for (sInt f=0; f<6; f++)
      {
        const sVector30 &n = ns[f];
        const sVector4 &t = ts[f];
        vp++->Init(v[codes[f][0]],n,t,0,1);
        vp++->Init(v[codes[f][1]],n,t,1,1);
        vp++->Init(v[codes[f][2]],n,t,0,0);
        vp++->Init(v[codes[f][3]],n,t,1,0);
        sQuad(ip,base,2,3,1,0);
        base+=4;
      }
    }
    CubeGeo->EndLoad();
  }
}

void RNCubeTrees::Render(Wz4RenderContext *ctx)
{
  if(ctx->IsCommonRendermode())
  {
    if(TreeMtrl->SkipPhase(ctx->RenderMode,Para.LightEnv&15)) return;

    sMatrix34CM *model;
    sFORALL(Matrices, model)
    {
      TreeMtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv&15,model,0,0,0);
      if (TreeOk)
       TreeGeo->Draw();

      if (CubeMtrl) // use treemtrl if no cubemtrl supplied
        CubeMtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv&15,model,0,0,0);

      if (CubeOk)
        CubeGeo->Draw();
    }
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   TV Noise \o/                                                       ***/
/***                                                                      ***/
/****************************************************************************/

RNTVNoise::RNTVNoise()
{
  Geo = new sGeometry(sGF_LINELIST,sVertexFormatSingle);

  sRandomMT rnd;

  sImage img(256,256);
  for (sInt i=0; i<65536; i++)
    img.Data[i]=0xff000000|0x10101*rnd.Int(256);

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_CULLOFF|sMTRL_ZOFF;
  Mtrl->BlendColor = sMB_ALPHA;
  Mtrl->Texture[0] = sLoadTexture2D(&img,sTEX_ARGB8888);
  Mtrl->TFlags[0] = sMTF_TILE|sMTF_LEVEL2;
  Mtrl->Prepare(sVertexFormatSingle);

  Anim.Init(Wz4RenderType->Script);
}

RNTVNoise::~RNTVNoise()
{
  sDelete(Mtrl->Texture[0]);
  delete Mtrl;
  delete Geo;
}

void RNTVNoise::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Time = ctx->GetBaseTime();

  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNTVNoise::Prepare(Wz4RenderContext *ctx)
{
  sVertexSingle *vp;

  sFRect r,uv0,uv1;
  sInt sx = ctx->RTSpec.Window.SizeX();
  sInt sy = ctx->RTSpec.Window.SizeY();
  sF32 z = 0.0f;

  sRandomKISS rnd;
  rnd.Seed(*(sU32*)&Time);

  Geo->BeginLoadVB(2*sy,sGD_FRAME,&vp);

  sF32 uvscale = 1.41f*sx/256.0f;

  for (sInt i=0; i<sy; i++)
  {
    sVector2 a(rnd.Float(1), rnd.Float(1));
    sF32 s,c;
    sFSinCos(rnd.Float(sPI2F),s,c);
    sVector2 b=a+uvscale*sVector2(s,c);

    /*
    sF32 sl = Para.Scanlines*sF32(i)/sF32(sy)*sPIF;
    sF32 brightness = sFade(sFAbs(sFSin(sl)),1.0f-Para.ScanlineDarken,1.0f);
    sU32 color = (sInt(255*Para.Alpha)<<24)|(0x10101*sInt(brightness*255.0f));
    */
    sU32 color = (sInt(255*Para.Alpha)<<24)| 0xffffff;

    vp++->Init(-0.0f,i-0.0f,z,color,a.x,a.y);
    vp++->Init(sx-0.0f,i-0.0f,z,color,b.x,b.y);
  }

  Geo->EndLoadVB();
}

void RNTVNoise::Render(Wz4RenderContext *ctx)
{
  if (Para.Alpha<0.004f) return;

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sViewport view;
    view.Orthogonal = sVO_PIXELS;
    view.Prepare();

    sCBuffer<sSimpleMaterialPara> cb;
    cb.Data->Set(view);

    Mtrl->Set(&cb);
    Geo->Draw();
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   Irgendson Raymarchingkram                                          ***/
/***                                                                      ***/
/****************************************************************************/

RNJulia4D::RNJulia4D()
{
  Geo = new sGeometry(sGF_TRISTRIP,sVertexFormatDouble);

  Pass1Mtrl = new Julia4DPass1Mtrl;
  Pass1Mtrl->Flags = sMTRL_CULLOFF|sMTRL_ZOFF;
  Pass1Mtrl->Prepare(sVertexFormatDouble);

  Pass2Mtrl = new Julia4DPass2Material;
  Pass2Mtrl->Flags = sMTRL_CULLOFF|sMTRL_ZOFF;
  Pass2Mtrl->TFlags[0] = sMTF_CLAMP|sMTF_LEVEL0|sMTF_EXTERN;
  Pass2Mtrl->Prepare(sVertexFormatDouble);

  Pass3Mtrl = new Julia4DPass3Material;
  Pass3Mtrl->Flags = sMTRL_CULLOFF|sMTRL_ZOFF;
  Pass3Mtrl->TFlags[0] = sMTF_CLAMP|sMTF_LEVEL0|sMTF_EXTERN;
  Pass3Mtrl->TFlags[1] = sMTF_CLAMP|sMTF_LEVEL0|sMTF_EXTERN;
  Pass3Mtrl->Prepare(sVertexFormatDouble);

  Anim.Init(Wz4RenderType->Script);

  DistRT = 0;
}

RNJulia4D::~RNJulia4D()
{
  delete Pass1Mtrl;
  delete Pass2Mtrl;
  delete Pass3Mtrl;
  delete Geo;
}

void RNJulia4D::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Time = ctx->GetBaseTime();

  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);

  ctx->RenderFlags |= wRF_RenderZ;
}

void RNJulia4D::Prepare(Wz4RenderContext *ctx)
{
  sInt sx = ctx->ScreenX;
  sInt sy = ctx->ScreenY;
  sF32 zx = 1.0f/ctx->View.ZoomX;
  sF32 zy = 1.0f/ctx->View.ZoomY;

  sVertexDouble *vp;
  Geo->BeginLoadVB(4,sGD_FRAME,&vp);
  vp++->Init( 0, 0, 0.1f, 0xffffffff,-zx, zy, 0, 0);
  vp++->Init(sx, 0, 0.1f, 0xffffffff, zx, zy, 1, 0);
  vp++->Init( 0,sy, 0.1f, 0xffffffff,-zx,-zy, 0, 1);
  vp++->Init(sx,sy, 0.1f, 0xffffffff, zx,-zy, 1, 1);
  Geo->EndLoadVB();
}

void RNJulia4D::Render(Wz4RenderContext *ctx)
{
  sMatrix34 view2model;
  view2model = Matrices[0];
  view2model.Invert34();
  sVector4 cp=Matrices[0].y;//(((view2model.i.y,view2model.j.y,view2model.k.y,-view2model.l.y);
  view2model = ctx->View.Camera*view2model;

  sMatrix34 mv=view2model;
  mv.Invert34();

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_ZNORMAL /*||
     (ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_ZONLY */)
  {
    sViewport view;

    // pass 1: make distance target
    DistRT=sRTMan->Acquire(ctx->ScreenX,ctx->ScreenY,sTEX_2D|sTEX_R32F|sTEX_NOMIPMAPS);
    sSetTarget(sTargetPara(sST_CLEARALL,0,0,DistRT));
    view.SetTargetCurrent();
    view.Orthogonal = sVO_PIXELS;
    view.Prepare();

    sCBuffer<Julia4DVSPara> cbv1;
    cbv1.Data->Set(view,view2model);
    sCBuffer<Julia4DPSPara> cbp1;
    cbp1.Data->C=Para.C;
    cbp1.Data->ClipPlane=cp;

    Pass1Mtrl->Set(&cbv1, &cbp1);
    Geo->Draw();

    sTargetPara para(sST_CLEARNONE,0,0,ctx->ZTarget,sGetRTDepthBuffer());
    sSetTarget(para);
    view.SetTargetCurrent();
    view.Prepare();

    // pass 2: calc z/normals
    sCBuffer<Julia4DVSPara> cbv2;
    cbv2.Data->Set(view,view2model);
    sCBuffer<Julia4DPSPara> cbp2;
    cbp2.Data->C=Para.C;
    //cbp2.Data->Params1.Init(ctx->View.Proj.k.z,ctx->View.Proj.l.z/ctx->View.ClipFar,0,0);
    cbp2.Data->Params1=view2model.k/ctx->View.ClipFar;
    cbp2.Data->mv=mv;

    Pass2Mtrl->Texture[0]=DistRT;
    Pass2Mtrl->Set(&cbv2, &cbp2);
    Geo->Draw();
  }

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  { 
    // pass 3: create image
    sViewport view;
    view.SetTargetCurrent();
    view.Orthogonal = sVO_PIXELS;
    view.Prepare();

    sCBuffer<Julia4DVSPara> cbv2;
    cbv2.Data->Set(view,view2model);
    sCBuffer<Julia4DPSPara> cbp2;
    cbp2.Data->C=Para.C;
    cbp2.Data->Params1=view2model.k/ctx->View.ClipFar;
    cbp2.Data->mv=mv;

    ModLightEnv *mle=ModMtrlType->LightEnv[0];
    if (mle)
    {
      cbp2.Data->ldir_vs = mle->Lights[0].ws_Dir*ctx->View.View;
      cbp2.Data->lc_front = mle->Lights[0].ColFront;
      cbp2.Data->lc_mid = mle->Lights[0].ColMiddle;
      cbp2.Data->lc_back = mle->Lights[0].ColBack;
      cbp2.Data->ambient=mle->Ambient;
      cbp2.Data->Fog.Init(mle->FogAdd,mle->FogMul,mle->FogDensity,0);
      cbp2.Data->FogColor=mle->FogColor;
    }
    else
    {
      cbp2.Data->ldir_vs = sVector30(0,1,0)*ctx->View.View;
      cbp2.Data->lc_front.Init(0.5,0.5,0.5,0);
      cbp2.Data->lc_mid.Init(0,0,0,0);
      cbp2.Data->lc_back.Init(0,0,0,0);
      cbp2.Data->ambient.Init(0.5,0.5,0.5,0);
      cbp2.Data->Fog.Init(0,0,0,0);
      cbp2.Data->FogColor.Init(0,0,0,0);
    }

    cbp2.Data->Color.InitColor(Para.Color);
    if (Para.Flags&1) cbp2.Data->Color*=2;
    cbp2.Data->ldir_vs.w=Para.Power;
    cbp2.Data->lc_spec.InitColor(Para.Specular);

    Pass3Mtrl->Texture[0]=DistRT;
    Pass3Mtrl->Texture[1]=ctx->ZTarget;
    Pass3Mtrl->Set(&cbv2,&cbp2);
    Geo->Draw();

    sRTMan->Release(DistRT);
    DistRT=0;
  }
}
