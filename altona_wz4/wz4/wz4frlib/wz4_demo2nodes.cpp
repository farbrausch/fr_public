/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_demo2nodes.hpp"
#include "wz4_ipp_shader.hpp"

/****************************************************************************/

RNCamera::RNCamera()
{
  Anim.Init(Wz4RenderType->Script);
  Symbol = 0;
  Target = 0;
}

RNCamera::~RNCamera()
{
  Target->Release();
}

void RNCamera::Init(const sChar *splinename)
{
  if((Para.Mode&15)==3)
    Symbol = Wz4RenderType->Script->AddSymbol(splinename);
}

void RNCamera::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;

  ctx->CameraFlag = 1;
  sViewport oldview = ctx->View;
  sInt oldflags = ctx->RenderFlags;
  ctx->RenderFlags &= ~wRF_RenderWire;
  ctx->RenderFlags |= wRF_RenderMain;

  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  sF32 zoom = Para.Zoom;
  sTargetSpec spec;
  sTexture2D *db = 0;
  if(Target)
  {
    Target->Flush();
    Target->Acquire(ctx->RTSpec.Window.SizeX(),ctx->RTSpec.Window.SizeY());
    db = sRTMan->Acquire(Target->Texture->SizeX,Target->Texture->SizeY,sTEX_2D|sTEX_DEPTH24NOREAD|sTEX_NOMIPMAPS);
    spec.Init(Target->Texture->CastTex2D(),db);
    spec.Aspect = sF32(Target->Texture->SizeX)/Target->Texture->SizeY;
  }
  else
  {
    spec = ctx->RTSpec;
  }

  if(!(ctx->RenderFlags & wRF_FreeCam) || (Para.Clear & 0x40))
  {
    sMatrix34 mat,tilt;

    switch(Para.Mode&15)
    {
    case 0:
      mat.LookAt(Para.Target,Para.Position);
      break;
    case 1:
      mat.EulerXYZ(Para.Rot.x*sPI2F,Para.Rot.y*sPI2F,Para.Rot.z*sPI2F);
      mat.l = Para.Position;
      break;
    case 2:
      mat.EulerXYZ(Para.Rot.x*sPI2F,Para.Rot.y*sPI2F,Para.Rot.z*sPI2F);
      mat.l = Para.Target - mat.k*Para.Distance;
      break;
    case 3:
      {
        if(Symbol && Symbol->Value && Symbol->Value->Spline)
        {
          float data[8];
          sClear(data);
          ScriptSpline *spline = Symbol->Value->Spline;
          sInt count = spline->Count;
          spline->Eval(Para.SplineTime,data,count);

          switch(spline->Bindings)
          {
          case 2:
            mat.EulerXYZ(data[3]*sPI2F,data[4]*sPI2F,data[5]*sPI2F);
            mat.l.Init(data[0],data[1],data[2]);
            if(count>=6)
              zoom = data[6];
            if(count>=7)
            {
              tilt.EulerXYZ(0,0,data[7]*sPI2F);
              mat = tilt*mat;
            }
            break;

          case 3:
            mat.LookAt(sVector31(data[3],data[4],data[5]),sVector31(data[0],data[1],data[2]));
            if(count>=6)
              zoom = data[6];
            if(count>=7)
            {
              tilt.EulerXYZ(0,0,data[7]*sPI2F);
              mat = tilt*mat;
            }
            break;
          }
        }
      }
      break;
    }
    if((Para.Mode&15)!=3)
    {
      tilt.EulerXYZ(0,0,Para.Tilt*sPI2F);
      mat = tilt*mat;
    }

    sViewport view;
    view.Camera = ctx->ShakeCamMatrix*mat;
    view.ClipNear = Para.ClipNear;
    view.ClipFar = Para.ClipFar;
    view.SetTarget(spec);
    view.SetZoom(zoom);
    if(Para.Clear & 16)
      view.Orthogonal = sVO_ORTHOGONAL;
    view.Prepare();

    ctx->DisableShadowFlag = (Para.Clear & 0x80) ? 1 : 0;
    ctx->View = view;
    if(!(Para.Clear & 0x60))
    {
      ctx->SetCam = 1;
      ctx->SetCamMatrix = mat;
      ctx->SetCamZoom = zoom;
    }
  }

  SimulateChilds(ctx);

  sInt clrflg = 0;
  if(Para.Clear&1)
    clrflg |= sCLEAR_COLOR;
  if(Para.Clear&2)
    clrflg |= sCLEAR_ZBUFFER;
  if(!Target)
    clrflg |= sST_SCISSOR;

  if(oldflags&wRF_RenderZ)      // really just render a zbuffer
  {
    ctx->RenderControlZ(this,spec);
  }
  else                          // render a real image
  {
    ctx->RenderControl(this,clrflg,Para.ClearColor,spec);
  }

  sRTMan->Release(db);

  ctx->View = oldview;
  ctx->RenderFlags = oldflags;
}

/****************************************************************************/

RNBillboardCamera::RNBillboardCamera()
{
  Anim.Init(Wz4RenderType->Script);
  Target = 0;
  AllowRender = 0;
}

RNBillboardCamera::~RNBillboardCamera()
{
  Target->Release();
}

void RNBillboardCamera::Init(const sChar *splinename)
{
}

void RNBillboardCamera::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;

  sViewport oldview = ctx->View;
  sInt oldflags = ctx->RenderFlags;
  ctx->RenderFlags &= ~wRF_RenderWire;
  ctx->RenderFlags |= wRF_RenderMain;


  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);

  sTargetSpec spec;
  sTexture2D *db = 0;
  if(Target)
  {
    Target->Flush();
    Target->Acquire(ctx->RTSpec.Window.SizeX(),ctx->RTSpec.Window.SizeY());
    db = sRTMan->Acquire(Target->Texture->SizeX,Target->Texture->SizeY,sTEX_2D|sTEX_DEPTH24NOREAD|sTEX_NOMIPMAPS);
    spec.Init(Target->Texture->CastTex2D(),db);
    sEnlargeZBufferRT(Target->Texture->SizeX,Target->Texture->SizeY);

    // modify target window for altas

    sInt atlasmax = Target->Atlas.Entries.GetCount();
    if(atlasmax>0)
    {
      spec.Window = Target->Atlas.Entries[sClamp(Para.AtlasId,0,(atlasmax-1))].Pixels;
    }
  }
  else
  {
    spec = ctx->RTSpec;
  }

  sF32 zoom = Para.Zoom;
  sMatrix34 mat;
  mat = oldview.Camera;

  if((Para.Flags & 15)==0)
    mat.l = sVector31(mat.k * -Para.Distance);

  sViewport view;
  view.Camera = mat;
  view.ZoomX = oldview.ZoomX;
  view.ZoomY = oldview.ZoomY;
  view.ClipNear = Para.ClipNear;
  view.ClipFar = Para.ClipFar;
  view.SetTarget(spec);
  if((Para.Flags & 15)==0)
    view.SetZoom(zoom);
  view.Prepare();

  ctx->View = view;

  SimulateChilds(ctx);

  AllowRender = 1;
#if sRENDERER==sRENDER_DX11
  ctx->RenderControl(this,Para.AtlasId ? 0 : sST_CLEARALL,Para.ClearColor,spec);
#else
  ctx->RenderControl(this,sST_CLEARALL,Para.ClearColor,spec);
#endif
  AllowRender = 0;

  sRTMan->Release(db);

  ctx->View = oldview;
  ctx->RenderFlags = oldflags;
}

void RNBillboardCamera::Render(Wz4RenderContext *ctx)
{
  if(AllowRender)
    Wz4RenderNode::Render(ctx);
}

/****************************************************************************/

static sU64 GetColor64(sU32 c)
{
  sU64 col;
  col = c;
  col = ((col&0xff000000)<<24) 
      | ((col&0x00ff0000)<<16) 
      | ((col&0x0000ff00)<< 8) 
      | ((col&0x000000ff)    );
  col = ((col | (col<<8))>>1)&0x7fff7fff7fff7fffULL;
  
  return col;
}

void RenderCode(GenBitmapParaRender *para,Wz4Render *in0,GenBitmap *out)
{
  // figure out target size

  sInt sx,sy;
  switch(para->Flags & 3)
  {
  default:
  case 0:
    sx = 1<<((para->Size2>>0)&255);
    sy = 1<<((para->Size2>>8)&255);
    break;
  case 1:
    if(Doc->IsPlayer)
    {
      sx = Doc->Spec.Window.SizeX();
      sy = Doc->Spec.Window.SizeY();
    }
    else
    {
      sx = Doc->DocOptions.ScreenX;
      sy = Doc->DocOptions.ScreenY;
    }
    sx = sx/para->Divider[0];     // fix this!
    sy = sy/para->Divider[1];
    break;
  case 2:
    sx = para->Size[0];
    sy = para->Size[1];
    break;
  }

  sRender3DBegin();

  // create target

  sTexture2D *rt = sRTMan->Acquire(sx,sy,(para->Flags & 16)?sTEX_R32F:sTEX_ARGB8888);
  sTexture2D *db = sRTMan->Acquire(sx,sy,sTEX_DEPTH24NOREAD);
  sTargetSpec spec(rt,db);
  spec.Aspect = sF32(sx)/sy;

  // prepare render context

  Wz4RenderContext ctx;
  ctx.Init(Wz4RenderType->Script,&spec,0);
  ctx.IppHelper = Wz4RenderType->IppHelper;
  if(para->Flags & 16)
    ctx.RenderFlags = wRF_RenderZ;
  else
    ctx.RenderFlags = wRF_RenderMain;

  // script execution

  ctx.DoScript(in0->RootNode,para->Time,para->BaseTime);

  // rendering, if not already done by camera

  if(ctx.CameraFlag==0)
  {
    sViewport view;
    ctx.View = view;
    ctx.RenderControl(in0->RootNode,sCLEAR_ALL,0,spec);
  }

  // read out texture 
  const sU8 *data;
  sInt pitch;
  sTextureFlags flags;
  sBeginReadTexture(data, pitch, flags,rt);

  out->Init(sx,sy);
  sU64 *dest = out->Data;
  if(para->Flags & 16)
  {
    for(sInt y=0;y<sy;y++)
    {
      for(sInt x=0;x<sx;x++)
      {
        sU64 c = sClamp(sInt(*(sF32 *)(data+pitch*y+4*x)*0x7fff),0,0x7fff);

        *dest++ = (c<<48)|(c<<32)|(c<<16)|c;
      }
    }
  }
  else
  {
    for(sInt y=0;y<sy;y++)
    {
      for(sInt x=0;x<sx;x++)
      {
        *dest++ = GetColor64(*(sU32 *)(data+pitch*y+4*x));
      }
    }
  }

  sEndReadTexture();  

  // free up

  sRTMan->Release(rt);
  sRTMan->Release(db);
  sRender3DEnd(0);
}

/****************************************************************************/
/****************************************************************************/

RNRenderMesh::RNRenderMesh()
{
  Mesh = 0;
  Anim.Init(Wz4RenderType->Script);
}

RNRenderMesh::~RNRenderMesh()
{
  Mesh->Release();
}

void RNRenderMesh::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);
}

void RNRenderMesh::Prepare(Wz4RenderContext *ctx)
{
  Mesh->BeforeFrame(Para.LightEnv,Matrices.GetCount(),Matrices.GetData());
}

void RNRenderMesh::Render(Wz4RenderContext *ctx)
{
  if(Para.Instances)
  {
    Mesh->RenderInst(ctx->RenderMode,Para.LightEnv,Matrices.GetCount(),Matrices.GetData());
  }
  else
  {
    sMatrix34CM *mat;
    sFORALL(Matrices,mat)
      Mesh->Render(ctx->RenderMode,Para.LightEnv,mat,Para.BoneTime,ctx->Frustum);
  }
}

/****************************************************************************/

RNMultiplyMesh::RNMultiplyMesh()
{
  Mesh = 0;

  Anim.Init(Wz4RenderType->Script);
}

RNMultiplyMesh::~RNMultiplyMesh()
{
  Mesh->Release();
}


void RNMultiplyMesh::Simulate(Wz4RenderContext *ctx)
{
  sSRT srt;

  // script

  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  Para.Count1 = sClamp(Para.Count1,1,1024);
  Para.Count2 = sClamp(Para.Count2,1,1024);
  Para.Count3 = sClamp(Para.Count3,1,1024);

  // transform

  srt.Scale = Para.Scale;
  srt.Rotate = Para.Rot;
  srt.Translate = Para.Trans;
  srt.MakeMatrix(Mat[0]);

  srt.Scale = Para.Scale1;
  srt.Rotate = Para.Rot1;
  srt.Translate = Para.Trans1;
  srt.MakeMatrix(Mat[1]);

  srt.Scale = Para.Scale2;
  srt.Rotate = Para.Rot2;
  srt.Translate = Para.Trans2;
  srt.MakeMatrix(Mat[2]);

  srt.Scale = Para.Scale3;
  srt.Rotate = Para.Rot3;
  srt.Translate = Para.Trans3;
  srt.MakeMatrix(Mat[3]);
}

void RNMultiplyMesh::Prepare(Wz4RenderContext *ctx)
{
  sMatrix34 mat1,mat2,mat3,mat;

  sInt total = Para.Count1*Para.Count2*Para.Count3*Matrices.GetCount();
  Mats.Resize(total);

  if(Para.Flags & 1)
  {
    if(Para.Count1>1)
    {
      mat.l -= sVector30(Mat[3].l) * ((Para.Count3-1)*0.5f);
      mat.l -= sVector30(Mat[2].l) * ((Para.Count2-1)*0.5f);
      mat.l -= sVector30(Mat[1].l) * ((Para.Count1-1)*0.5f);
    }
  }
  mat3 = mat;

  sMatrix34CM *mp  = Mats.GetData();
  for(sInt i=0;i<Para.Count3;i++)
  {
    mat2 = mat3;
    for(sInt i=0;i<Para.Count2;i++)
    {
      mat1 = mat2;
      for(sInt i=0;i<Para.Count1;i++)
      {
        mat = Mat[0] *mat1;

        for(sInt i=0;i<Matrices.GetCount();i++)
        {
          *mp++ = mat*sMatrix34(Matrices[i]);
        }
        mat = mat1;
        mat1 = Mat[1] * mat;
      }
      mat = mat2;
      mat2 = Mat[2] * mat;
    }
    mat = mat3;
    mat3 = Mat[3] * mat;
  }

  if(Para.Flags & 2)
    sReverse(Mats);

  if(total<10)
    Mesh->BeforeFrame(Para.LightEnv,Mats.GetCount(),Mats.GetData());
  else
    Mesh->BeforeFrame(Para.LightEnv);
}


void RNMultiplyMesh::Render(Wz4RenderContext *ctx)
{
  if(Mesh)
  {
    Mesh->RenderInst(ctx->RenderMode,Para.LightEnv,Mats.GetCount(),Mats.GetData());
  }
}

/****************************************************************************/

RNAdd::RNAdd()
{
  TimeOverride = 0;
}

void RNAdd::Simulate(Wz4RenderContext *ctx)
{
  SimulateCalc(ctx);

  if (TimeOverride != 0)
  {
    ctx->SetTime(*TimeOverride);
    ctx->SetLocalTime(*TimeOverride);
  }

  SimulateChilds(ctx);
}


/****************************************************************************/

RNTransform::RNTransform()
{
  Anim.Init(Wz4RenderType->Script);
}

void RNTransform::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

void RNTransform::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  sSRT srt;
  sMatrix34 mul;

  srt.Scale = Para.Scale;
  srt.Rotate = Para.Rot;
  srt.Translate = Para.Trans;
  srt.MakeMatrix(mul);

  TransformChilds(ctx,mul*mat);
}

/****************************************************************************/

RNTransformPivot::RNTransformPivot()
{
  Anim.Init(Wz4RenderType->Script);
}

void RNTransformPivot::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

void RNTransformPivot::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  sSRT srt;
  sMatrix34 mat0,mat1;

  srt.Scale = Para.Scale;
  srt.Rotate = Para.Rot;
  srt.Translate = Para.Trans;
  srt.MakeMatrix(mat1);
  mat1.l = mat1.l + sVector30(Para.Pivot);
  mat0.l = -Para.Pivot;

  TransformChilds(ctx,(mat0*mat1)*mat);
}

/****************************************************************************/

RNSplinedObject::RNSplinedObject()
{
  Anim.Init(Wz4RenderType->Script);
  Spline = 0;
  SplineSymbol = 0;
}

void RNSplinedObject::Init(const sChar *splinename)
{
  SplineSymbol = Wz4RenderType->Script->AddSymbol(splinename);
}

void RNSplinedObject::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);

  Spline = 0;
  if(SplineSymbol && SplineSymbol->Value && SplineSymbol->Value->Spline) 
    Spline = SplineSymbol->Value->Spline;

  SimulateChilds(ctx);
}

static inline void SlowThreePoint(sMatrix34 &m,const sVector31 &p0,const sVector31 &p1,const sVector31 &p2,const sVector30 &tweak)
{
  m.k = p1-p0;
  m.j.Cross(p2-p1,p2-p0);
  m.k.Unit();
  m.j.Unit();
  m.i.Cross(m.j,m.k);
  m.i.Unit();
  m.j.Cross(m.k,m.i);
  m.j.Unit();
  /*
  sVector30 db;

  m.k = p2-p0;
  m.k*=sRSqrt(m.k.LengthSq());
  db = p1-p0;

  db*=sRSqrt(db.LengthSq());

  m.i.Cross(db,m.k); 
  m.i = m.i + tweak;
  m.j.Cross(m.k,m.i); 

  m.i*=sRSqrt(m.i.LengthSq());
  m.j*=sRSqrt(m.j.LengthSq());
  */
}


void RNSplinedObject::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  sSRT srt;
  sMatrix34 mat0;

  if(Spline)
  {
    sVector31 p0,p1,p2;
    sInt channels = sMin(Spline->Count,3);
    sF32 length = Spline->Length();

    sF32 t0 = Para.Tick * (1-Para.TimeDelta*2) * length;
    sF32 t1 = t0 + Para.TimeDelta*1*length;
    sF32 t2 = t0 + Para.TimeDelta*2*length;

    Spline->Eval(t0,&p0.x,channels);
    Spline->Eval(t1,&p1.x,channels);

    sVector30 up(0,1,0);
    if(Para.Mode & 16)
    {
      up = sVector30(p0);
      up.Unit();
    }

    switch(Para.Mode & 3)
    {
    case 0:
      mat0.LookAt(p0,p1,up);
      break;
    case 1:
      Spline->Eval(t2,&p2.x,channels);
      SlowThreePoint(mat0,p0,p1,p2,up);
      mat0.l = p0;
      break;
    case 2:
      Spline->Eval(t2,&p2.x,channels);
      {
        sVector30 x,d0,d1;
        d0 = p2-p0; d0.Unit();
        d1 = p1-p0; d1.Unit();
        x.Cross(d0,d1);
        sF32 ry = (x^up)*Para.RotateAmount;
        mat0.EulerXYZ(ry,0,0);
      }
      break;
    }
  }
  TransformChilds(ctx,mat0*mat);
}

/****************************************************************************/

RNLookAt::RNLookAt()
{
  Anim.Init(Wz4RenderType->Script);
}

void RNLookAt::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNLookAt::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  sMatrix34 mul0,mul1;

  mul0.LookAt(Para.Target,Para.Pos,Para.UpVector);
  mul1.EulerXYZ(0,0,Para.Tilt*sPI2F);
  TransformChilds(ctx,mul1*mul0*mat);
}

/****************************************************************************/

RNSkyCube::RNSkyCube()
{
  Anim.Init(Wz4RenderType->Script);
}

void RNSkyCube::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNSkyCube::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  sMatrix34 mat0;
  mat0.l = -sVector31(sVector30(ctx->View.View.l) * ctx->View.Camera);

  TransformChilds(ctx,mat*mat0);
}

/****************************************************************************/

RNShaker::RNShaker()
{
  Anim.Init(Wz4RenderType->Script);
}

void RNShaker::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);

  sMatrix34 mat1,mat0,mat,mat0i;
  sF32 tx,ty,tz,rx,ry,rz;
  sF32 time,ramp;

//  mat = ctx->ShakeCamMatrix;
  sMatrix34 matold = ctx->ShakeCamMatrix;
  time = Para.Animate;
  sF32 t0 = Para.TimeRange[0];
  sF32 t1  = Para.TimeRange[1];


  if(time>t0 && time<t1)
  {
    sF32 t = (time-t0) / (t1-t0);
    switch(Para.Mode&15)
    {
    default:
    case 0:
      ramp = t;
      break;
    case 1:
      ramp = 1-t;
      break;
    case 2:
      ramp = sFSin(t*sPIF);
      break;
    case 3:
      ramp = 1;
      break;
    }

    ramp *= Para.Amp;

    if(Para.Mode & 32)
    {
      sVector30 v;

      v = Para.TranslateFreq * time * 0x10000;
      tx = sPerlin3D(v.x,v.y,v.z,255,1);
      ty = sPerlin3D(v.x,v.y,v.z,255,2);
      tz = sPerlin3D(v.x,v.y,v.z,255,3);
      v = Para.RotateFreq * time * 0x10000;
      rx = sPerlin3D(v.x,v.y,v.z,255,4);
      ry = sPerlin3D(v.x,v.y,v.z,255,5);
      rz = sPerlin3D(v.x,v.y,v.z,255,6);
    }
    else
    {
      tx = sFSin(time*Para.TranslateFreq.x*sPI2F);
      ty = sFSin(time*Para.TranslateFreq.y*sPI2F);
      tz = sFSin(time*Para.TranslateFreq.z*sPI2F);
      rx = sFSin(time*Para.RotateFreq.x*sPI2F);
      ry = sFSin(time*Para.RotateFreq.y*sPI2F);
      rz = sFSin(time*Para.RotateFreq.z*sPI2F);
    }

    tx = tx*Para.TranslateAmp.x*ramp;
    ty = ty*Para.TranslateAmp.y*ramp;
    tz = tz*Para.TranslateAmp.z*ramp;
    rx = rx*Para.RotateAmp.x*ramp;
    ry = ry*Para.RotateAmp.y*ramp;
    rz = rz*Para.RotateAmp.z*ramp;

    mat1.EulerXYZ(rx*sPI2,ry*sPI2,rz*sPI2);
    mat1.l.Init(tx,ty,tz);

    if(Para.Mode&16)
      mat0.l = Para.Center;
    else
      mat0 = ctx->View.Camera;

    mat0i = mat0;

    mat0i.InvertOrthogonal();

//    mat = mat * mat0i * mat1 * mat0;
    mat = mat1;
  }

  ctx->ShakeCamMatrix = mat;

  SimulateChilds(ctx);

  ctx->ShakeCamMatrix = matold;
}

/****************************************************************************/

RNMultiply::RNMultiply()
{
  Anim.Init(Wz4RenderType->Script);
}

void RNMultiply::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

void RNMultiply::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  sSRT srt;
  sMatrix34 PreMat;
  sMatrix34 MulMat;
  sMatrix34 accu;

  srt.Scale = Para.PreScale;
  srt.Rotate = Para.PreRot;
  srt.Translate = Para.PreTrans;
  srt.MakeMatrix(PreMat);

  srt.Scale = Para.Scale;
  srt.Rotate = Para.Rot;
  srt.Translate = Para.Trans;
  srt.MakeMatrix(MulMat);

  if(Para.Count>1 && (Para.Flags&1))
    accu.l = sVector31(sVector30(srt.Translate) * ((Para.Count-1)*-0.5));

  for(sInt i=0;i<sMax(1,Para.Count);i++)
  {
    TransformChilds(ctx,PreMat*accu*mat);
    accu = accu * MulMat;
  }
}

/****************************************************************************/

RNClip::RNClip()
{
  Active = 0;
  Anim.Init(Wz4RenderType->Script);
}

void RNClip::Init()
{
  if(Para.Flags & 16)
  {
    Doc->CacheWarmupBeat.AddTail(sInt(Para.StartTime*0x10000));
  }
}

void Wz4RenderNode::SetTimeAndSimulate(Wz4RenderContext *ctx,sF32 newtime,sF32 localoffset)
{
  sF32 time = ctx->GetBaseTime();
//  sF32 oldtime = ctx->GetTime();
  sF32 oldlocal = ctx->GetLocalTime();
  ctx->SetTime(newtime);
  ctx->SetLocalTime(time + localoffset);
  SimulateChilds(ctx);
  ctx->SetTime(time);
  ctx->SetLocalTime(oldlocal);
}


void RNClip::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;

  Anim.Bind(ctx->Script,&Para);
  Wz4RenderNode::SimulateCalc(ctx);

  if(Para.Flags & 4)                   // Local
  {
    sF32 time = ctx->GetLocalTime()+Para.LocalOffset;
    Active = (time>=Para.StartTime && time<Para.StartTime+Para.Length);
    if(Active && Para.Enable)
    {
      SetTimeAndSimulate(ctx,sF32(time-Para.StartTime)/Para.Length*(Para.Time1-Para.Time0)+Para.Time0,-Para.StartTime+Para.Time0*Para.Length/(Para.Time1-Para.Time0));
    }
  }
  else                            // Global
  {
    sF32 time = ctx->GetBaseTime();
    Active = (time>=Para.StartTime && time<Para.StartTime+Para.Length);
    if(Active && Para.Enable)
    {
      if(Para.Flags & 1)
      {
        SetTimeAndSimulate(ctx,sF32(time-Para.StartTime)/Para.Length*(Para.Time1-Para.Time0)+Para.Time0,-Para.StartTime+Para.Time0*Para.Length/(Para.Time1-Para.Time0));
        /*
        sF32 oldtime = ctx->GetTime();
        sF32 oldlocal = ctx->GetLocalTime();
        ctx->SetTime(sF32(time-Para.StartTime)/Para.Length*(Para.Time1-Para.Time0)+Para.Time0);
        ctx->SetLocalTime(time - Para.StartTime);
        SimulateChilds(ctx);
        ctx->SetTime(oldtime);
        ctx->SetLocalTime(oldlocal);
        */
      }
      else
      {
        SimulateChilds(ctx);
      }
    }
  }
}

void RNClip::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  if(Active && Para.Enable) Wz4RenderNode::Transform(ctx,mat);
}

void RNClip::Prepare(Wz4RenderContext *ctx)
{
  if(Active && Para.Enable) Wz4RenderNode::Prepare(ctx);
}

void RNClip::Render(Wz4RenderContext *ctx)
{
  if(Active && Para.Enable) Wz4RenderNode::Render(ctx);
}

/****************************************************************************/

RNClipRand::RNClipRand()
{
  Active = 0;
  Anim.Init(Wz4RenderType->Script);
}

void RNClipRand::Init()
{
  Wz4RenderNode *rn;
  Clips.HintSize(Childs.GetCount());
  sRandomKISS rnd;
  rnd.Seed(sU32(sGetTimeUS()));
  sFORALL(Childs,rn)
  {
    clip c;
    c.Child = rn;
    c.Length = Para.DefaultClipLength;
    c.Start = 0;
    c.Sort =rnd.Float(1);
    *Clips.AddMany(1) = c;
  }
  sSortUp(Clips,&clip::Sort);

  clip *c;
  sF32 time = Para.StartTime;
  sFORALL(Clips,c)
  {
    c->Start = time;
    if(c->Child->ClipRandDuration>0)
      time += c->Child->ClipRandDuration;
    else
      time += Para.DefaultClipLength;
  }
}

void RNClipRand::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;

  Anim.Bind(ctx->Script,&Para);
  Wz4RenderNode::SimulateCalc(ctx);

  sF32 time = ctx->GetBaseTime();
  Active = (time>=Para.StartTime && time<Para.StartTime+Para.Length);
  ActiveClip = -1;
  if(Active && Para.Enable)
  {
    clip *c;
    sFORALL(Clips,c)
    {
      if(time>=c->Start && time<c->Start+c->Length)
      {
        ActiveClip = _i;
      }
    }
  }
  if(ActiveClip>=0)
  {
    clip *c = &Clips[ActiveClip];
    SetTimeAndSimulate(ctx,sF32(time-c->Start)/c->Length,Para.StartTime);
/*
    ctx->SetTime(sF32(time-c->Start)/c->Length);
    SimulateChild(ctx,c->Child);
    ctx->SetTime(time);
*/
  }
}

void RNClipRand::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  if(Active && Para.Enable && ActiveClip>=0) Clips[ActiveClip].Child->Transform(ctx,mat);
}

void RNClipRand::Prepare(Wz4RenderContext *ctx)
{
  if(Active && Para.Enable && ActiveClip>=0) Clips[ActiveClip].Child->Prepare(ctx);
}

void RNClipRand::Render(Wz4RenderContext *ctx)
{
  if(Active && Para.Enable && ActiveClip>=0) Clips[ActiveClip].Child->Render(ctx);
}

/****************************************************************************/

RNMultiClip::RNMultiClip()
{
}

void RNMultiClip::Init(const Wz4RenderParaMultiClip &para,sInt count,const Wz4RenderArrayMultiClip *arr)
{
  Para = ParaBase = para;

  Clips.AddMany(count);
  for(sInt i=0;i<count;i++)
  {
    Clips[i] = arr[i];
    if(Para.Flags & 16)
      Doc->CacheWarmupBeat.AddTail(sInt(Clips[i].Start*0x10000));
  }
}

void RNMultiClip::Simulate(Wz4RenderContext *ctx)
{
  Wz4RenderArrayMultiClip *clip;

  Para = ParaBase;
  Wz4RenderNode::SimulateCalc(ctx);
  
  sF32 time = ctx->GetBaseTime();
  Active = 0;
  if(Para.MasterEnable)
  {
    sFORALL(Clips,clip)
    {
      if(time>=clip->Start && time<clip->Start+clip->Length && clip->Enable)
      {
        Active = 1;
        if(Para.Flags & 1)
        {
          SetTimeAndSimulate(ctx,sF32(time-clip->Start)/clip->Length*(clip->Time1-clip->Time0)+clip->Time0,0);
/*
          ctx->SetTime(sF32(time-clip->Start)/clip->Length*(clip->Time1-clip->Time0)+clip->Time0);
          SimulateChilds(ctx);
          */
        }
        else
        {
          SimulateChilds(ctx);
        }
      }
    }
  }
//  if((Para.Flags & 1) && Active)
//    ctx->SetTime(time);
}

void RNMultiClip::Prepare(Wz4RenderContext *ctx)
{
  if(Active) Wz4RenderNode::Prepare(ctx);
}

void RNMultiClip::Render(Wz4RenderContext *ctx)
{
  if(Active) Wz4RenderNode::Render(ctx);
}

/****************************************************************************/

RNFader::RNFader()
{
  _Fader = AddSymbol(L"Fader");
  _Rotor = AddSymbol(L"Rotor");
}

void RNFader::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;

  // set vars

  ScriptValue *VFader,*VRotor;

  ctx->Script->PushGlobal();
  VFader = ctx->Script->MakeFloat(8);
  VRotor = ctx->Script->MakeFloat(8);
  for(sInt i=0;i<8;i++)
  {
    VFader->FloatPtr[i] = (&Para.FaderA.x)[i];
    VRotor->FloatPtr[i] = (&Para.RotorA.x)[i];
  }
  ctx->Script->BindGlobal(_Fader,VFader);
  ctx->Script->BindGlobal(_Rotor,VRotor);

  SimulateCalc(ctx);

  // recurse

  SimulateChilds(ctx);

  ctx->Script->PopGlobal();
}

/****************************************************************************/

RNVariable::RNVariable()
{
  _Symbol = 0;
}

void RNVariable::Init(Wz4RenderParaVariable *para,const sChar *name)
{
  _Symbol = AddSymbol(name);
  Count = para->Count;
  Value = para->Value;
}

void RNVariable::Simulate(Wz4RenderContext *ctx)
{
  // set vars

  ScriptValue *sv;

  ctx->Script->PushGlobal();
  sv = ctx->Script->MakeFloat(Count);
  for(sInt i=0;i<4;i++)
    sv->FloatPtr[i] = Value[i];

  ctx->Script->BindGlobal(_Symbol,sv);

  SimulateCalc(ctx);

  // recurse

  SimulateChilds(ctx);

  ctx->Script->PopGlobal();
}

/****************************************************************************/
/****************************************************************************/

RNSpline::RNSpline()
{
  Spline = 0;
}

RNSpline::~RNSpline()
{
  delete Spline;
}

void RNSpline::Init(Wz4RenderParaSpline *para,sInt ac,Wz4RenderArraySpline *ar,const sChar *name)
{
  Name = name ? name : L"spline";
  ScriptSplineKey *dest;
  sInt count = para->Dimensions+1;
  sInt sum[8];
  sVERIFY(count>=1 && count<=8);

  Spline = new ScriptSpline;
  Spline->Init(count);
  Spline->Mode = para->Spline;
  Spline->Flags = para->Flags;
  Spline->MaxTime = para->MaxTime;
  Spline->Bindings = para->GrabCamMode;

  if(para->GrabCamMode==1)
    Spline->RotClampMask = 0x07;
  if(para->GrabCamMode==2)
    Spline->RotClampMask = 0x38;


  sClear(sum);
  for(sInt i=0;i<ac;i++)
  {
    if(ar[i].Use>=0 && ar[i].Use<count)
      sum[ar[i].Use]++;
  }
  for(sInt i=0;i<count;i++)
    Spline->Curves[i]->HintSize(sum[i]+((para->Flags&1)?4:0));
  for(sInt i=0;i<ac;i++)
  {
    if(ar[i].Use>=0 && ar[i].Use<count)
    {
      dest = Spline->Curves[ar[i].Use]->AddMany(1);
      dest->Time = ar[i].Time;
      dest->Value = ar[i].Value;
    }
  }
  for(sInt i=0;i<count;i++)
    sSortUp(*Spline->Curves[i],&ScriptSplineKey::Time);

  if((para->Flags & SSF_BoundMask)==SSF_BoundWrap) // loop spline
  {
    for(sInt i=0;i<count;i++)
    {
      sInt max = Spline->Curves[i]->GetCount();
      if(max>=2)
      {
        ScriptSplineKey k0,k1,k2,k3;

        k0 = (*Spline->Curves[i])[0];      k0.Time += para->MaxTime;
        k1 = (*Spline->Curves[i])[1];      k1.Time += para->MaxTime;
        k2 = (*Spline->Curves[i])[max-2];  k2.Time -= para->MaxTime;
        k3 = (*Spline->Curves[i])[max-1];  k3.Time -= para->MaxTime;

        Spline->Curves[i]->AddHead(k3);
        Spline->Curves[i]->AddHead(k2);
        Spline->Curves[i]->AddTail(k0);
        Spline->Curves[i]->AddTail(k1);
      }
    }
  }
}

void RNSpline::Simulate(Wz4RenderContext *ctx)
{
  ScriptContext *scr = ctx->Script;
  scr->PushGlobal();
  scr->BindGlobal(scr->AddSymbol(Name),scr->MakeValue(Spline));

  SimulateCalc(ctx);
  SimulateChilds(ctx);
  scr->PopGlobal();
}

/****************************************************************************/

RNLayer2D::RNLayer2D()
{
  Texture[0] = 0;
  Texture[1] = 0;
  Mtrl = 0;
  Geo = 0;

  Anim.Init(Wz4RenderType->Script);
}

RNLayer2D::~RNLayer2D()
{
  Texture[0]->Release();
  Texture[1]->Release();
  delete Mtrl;
  delete Geo;
}

void RNLayer2D::Init()
{
  static sInt blend[16] = { sMB_OFF,sMB_PMALPHA,sMB_ADD,sMB_SUB,sMB_MUL,sMB_MUL2,sMB_ADDSMOOTH,sMB_ALPHA,sMBS_1|sMBO_ADD|sMBD_SCI,
    sMBS_DA|sMBO_ADD|sMBD_1 , sMBS_DAI|sMBO_ADD|sMBD_1 };

  Para = ParaBase;

  Mtrl = new Layer2dMtrl;
  Mtrl->BlendColor = blend[Para.Blend&15];
  if(Para.Blend & 0x30000)
  {
    if(Mtrl->BlendColor==sMB_OFF)
      Mtrl->BlendColor = sMBS_1|sMBO_ADD|sMBD_0;
    if((Para.Blend & 0x30000)==0x10000)
      Mtrl->BlendAlpha = sMBS_0|sMBO_ADD|sMBD_1;
    if((Para.Blend & 0x30000)==0x20000)
      Mtrl->BlendAlpha = sMBS_0|sMBO_ADD|sMBD_0;
    if((Para.Blend & 0x30000)==0x30000)
      Mtrl->BlendAlpha = sMBS_1|sMBO_ADD|sMBD_SAI;
  }

  Mtrl->Flags = sMTRL_CULLOFF|sMTRL_ZOFF|sMTRL_LIGHTING;
  if(Texture[0]) Mtrl->Texture[0] = Texture[0]->Texture;
  if(Texture[1]) Mtrl->Texture[1] = Texture[1]->Texture;
  Mtrl->TFlags[0] = sConvertOldUvFlags(Para.Filter);
  Mtrl->TFlags[1] = sConvertOldUvFlags(Para.Filter);
  Mtrl->MixMode = Para.Mix;
  Mtrl->Prepare(sVertexFormatDouble);

  Geo = new sGeometry(sGF_QUADLIST,sVertexFormatDouble);
}

void RNLayer2D::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;

  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);
  SimulateChilds(ctx);
}

void RNLayer2D::Prepare(Wz4RenderContext *ctx)
{
  sVertexDouble *vp;

  sFRect r,uv0,uv1;
  const sF32 screenX = ctx->RTSpec.Window.SizeX();
  const sF32 screenY = ctx->RTSpec.Window.SizeY();

  sTexture2D *tex=0;
  if(Mtrl && Mtrl->Texture[0]) 
    tex=Mtrl->Texture[0]->CastTex2D();

  sF32 aspect=1.0f;
  switch (Para.Aspect&3)
  {
  case 1: 
    aspect=screenX/screenY; 
    break; 
  case 2:
    if(tex)
      aspect=sF32(tex->SizeX)/sF32(tex->SizeY);
    break;
  default: break;
  }

  sF32 sizeY=screenY;
  if (tex && Para.Aspect&4) sizeY*=tex->SizeY/DocScreenY;

  sF32 sizeX=sizeY*aspect;
  if (!(Para.Aspect & 4))
  {
    if (sizeY>screenY)
    {
      sizeX*=screenY/sizeY;
      sizeY=screenY;
    }
    if (sizeX>screenX)
    {
      sizeY*=screenX/sizeX;
      sizeX=screenX;
    }
  }

  sF32 xalign=0, yalign=0;
  switch (Para.Align&3)
  {
  case 0: xalign=0.0f; break;
  case 1: xalign=0.5f; break;
  case 2: xalign=1.0f; break;
  }
  switch (Para.Align&12)
  {
  case 0: yalign=0.0f; break;
  case 4: yalign=0.5f; break;
  case 8: yalign=1.0f; break;
  }

  r.x0 = Para.Center[0]*screenX - Para.Size[0]*sizeX*xalign;    
  r.x1 = Para.Center[0]*screenX + Para.Size[0]*sizeX*(1-xalign);  
  r.y0 = Para.Center[1]*screenY - Para.Size[1]*sizeY*yalign;
  r.y1 = Para.Center[1]*screenY + Para.Size[1]*sizeY*(1-yalign);
  uv0.x0 = Para.ScrollUV[0];
  uv0.x1 = Para.ScrollUV[0]+Para.ScaleUV[0];
  uv0.y0 = Para.ScrollUV[1];
  uv0.y1 = Para.ScrollUV[1]+Para.ScaleUV[1];
  uv1.x0 = Para.ScrollUV2[0];
  uv1.x1 = Para.ScrollUV2[0]+Para.ScaleUV2[0];
  uv1.y0 = Para.ScrollUV2[1];
  uv1.y1 = Para.ScrollUV2[1]+Para.ScaleUV2[1];
  sF32 z = 0.0f;
  sU32 col = Para.Color;

  Geo->BeginLoadVB(4,sGD_FRAME,&vp);
  vp[0].Init(r.x0-0.5f,r.y0-0.5f,z,col,uv0.x0,uv0.y0,uv1.x0,uv1.y0);
  vp[1].Init(r.x1-0.5f,r.y0-0.5f,z,col,uv0.x1,uv0.y0,uv1.x1,uv1.y0);
  vp[2].Init(r.x1-0.5f,r.y1-0.5f,z,col,uv0.x1,uv0.y1,uv1.x1,uv1.y1);
  vp[3].Init(r.x0-0.5f,r.y1-0.5f,z,col,uv0.x0,uv0.y1,uv1.x0,uv1.y1);
  Geo->EndLoadVB();
}

void RNLayer2D::Render(Wz4RenderContext *ctx)
{
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

RNBeat::RNBeat()
{
  Spline = 0;
}

RNBeat::~RNBeat()
{
  delete Spline;
}

void RNBeat::Init(Wz4RenderParaBeat *para,sInt ac,Wz4RenderArrayBeat *ar,const sChar *name)
{
  Name = name ? name : L"beat";
  Amp = para->Amp;
  Bias = para->Bias;
  ScriptSplineKey *dest;

  sU8 *bytesbase = (sU8 *)ar;
  sInt max = ac*16;
  sF32 tf = 1.0f/(1<<para->Tempo);

  delete Spline;
  Spline = new ScriptSpline;
  Spline->Init(1);
  Spline->Mode = (para->Mode & 15) ? SSM_Hermite : SSM_Linear;
  Spline->Flags = para->SplineFlags;
  Spline->MaxTime = max*tf;

  // loop

  sU8 *bytes = new sU8[max];

  sInt ls = 0;
  sInt le = 0;
  sInt loop = 0;
  sInt li = 0;

  for(sInt i=0;i<max;i++)
  {
    sInt byte = bytesbase[i];
    switch(byte & RNB_LoopMask)
    {
    case RNB_LoopStart:
      ls = i;
      loop = 0;
      break;
    case RNB_LoopEnd:
      le = i+1;
      loop = 1;
      li = ls;
      break;
    case RNB_LoopDone:
      loop = 0;
      break;
    }

    if(loop==2)
    {
      bytes[i] = bytesbase[li++];
      if(li==le)
        li=ls;
    }
    else
    {
      bytes[i] = bytesbase[i];
    }
    if(loop==1)
      loop=2;
  }

  // count keyse

  sInt keys = 0;
  for(sInt i=0;i<max;i++)
  {
    if(bytes[i]&RNB_LevelMask)
      keys += 3;
  }
  Spline->Curves[0]->HintSize(keys);

  // keys

  sF32 amp = para->Amp;
  sF32 d0 = para->Release;
  sF32 d1 = para->Attack;
  sF32 t0 = -1000;
  sF32 v0 = 0;
  sF32 bias = para->Bias;

  for(sInt i=0;i<max;i++)
  {
    sInt val = sMin(bytes[i]&RNB_LevelMask,para->Levels);
    if(val)
    {
      sF32 v1 = (&para->Level0)[val*3];
      if(para->Mode&32)
        d1 = para->Attack*(&para->Att0)[val*3];
      sF32 t1 = i*tf+para->Timeshift;
      if(t1<=t0)                // old attack is not finished and we need to start another one?
        continue;               // don't care about that case :-)

      if(para->Mode&16)         // normal attack mode
      {
        sF32 t0e = t0+v0*d0;    // last one
        sF32 v = 0;
        if(t0e<t1)              // go to zero
        {
          dest = Spline->Curves[0]->AddMany(2);
          dest[0].Time = t0e;
          dest[0].Value = bias;
          dest[1].Time = t1;
          dest[1].Value = bias;
        }
        else                    // don't reach zero
        {
          dest = Spline->Curves[0]->AddMany(1);
          dest[0].Time = t1;
          v = v0-(t1-t0)/d0;
          dest[0].Value = v*amp+bias;
        }
        t1 += sMax<sF32>(0,(v1-v)*d1);
        dest = Spline->Curves[0]->AddMany(1);
        dest[0].Time = t1;
        dest[0].Value = v1*amp+bias;
      }
      else                      // preattack-mode
      {
        sF32 t0e = t0+v0*d0;    // last one
        sF32 t1e = t1-v1*d1;    // this one

        if(t0e<t1e)             // simple case: go down to zero properly
        {
          dest = Spline->Curves[0]->AddMany(2);
          dest[0].Time = t0e;
          dest[0].Value = bias;
          dest[1].Time = t1e;
          dest[1].Value = bias;
        }
        else                    // hard case: calculate intersection
        {
          sF32 t = (d0*d1*v0 - d0*d1*v1 + d0*t1 + d1*t0)/(d0+d1);
          sF32 v = 0;
          if(t<=t0)
          {
            t = t0;
            v = v1-(t1-t)/d1;
          }
          else if(t>=t1)
          {
            t = t1;
            v = v0-(t-t0)/d0;
          }
          else
          {
            v = v0-(t-t0)/d0;
          }
          dest = Spline->Curves[0]->AddMany(1);
          dest[0].Time = t;
          dest[0].Value = v*amp+bias;
        }
      }

      // put the tip

      dest = Spline->Curves[0]->AddMany(1);
      dest[0].Time = t1;
      dest[0].Value = v1*amp+bias;

      if(para->Mode&32)
        d0 = para->Release*(&para->Rel0)[val*3];
      t0 = t1;
      v0 = v1;
    }
  }
  if(v0>0)
  {
    dest = Spline->Curves[0]->AddMany(1);
    dest[0].Time = t0 + v0*d0;
    dest[0].Value = bias;
  }

  delete[] bytes;
}


void RNBeat::Simulate(Wz4RenderContext *ctx)
{
  ScriptContext *scr = ctx->Script;
  scr->PushGlobal();
  scr->BindGlobal(scr->AddSymbol(Name),scr->MakeValue(Spline));

  SimulateCalc(ctx);
  SimulateChilds(ctx);
  scr->PopGlobal();
}

/****************************************************************************/
/***                                                                      ***/
/***   BoneTrain - Debris bridges                                         ***/
/***                                                                      ***/
/****************************************************************************/

RNBoneTrain::RNBoneTrain()
{
  Anim.Init(Wz4RenderType->Script);
  mats = 0;
  mats0 = 0;
  Count = 0;

  Symbol = 0;
}

RNBoneTrain::~RNBoneTrain()
{
  Mesh->Release();
  delete[] mats;
  delete[] mats0;
}


void RNBoneTrain::Init(const sChar *splinename)
{
  if(Mesh->Skeleton)
  {
    Count = Mesh->Skeleton->Joints.GetCount();
    mats = new sMatrix34[Count];
    mats0 = new sMatrix34CM[Count];
  }
  Symbol = Wz4RenderType->Script->AddSymbol(splinename);
}

void RNBoneTrain::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;

  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  if(Symbol && Symbol->Value && Symbol->Value->Spline && Mesh->Skeleton) 
  {
    ScriptSpline *spline = Symbol->Value->Spline;
    sInt count = Mesh->Skeleton->Joints.GetCount();
    sF32 t0 = Para.Start;
    sF32 t1 = Para.Start+Para.Length;
    sF32 td = t1-t0;
    sMatrix34 mat;
    sVector30 up = Para.UpVector;
    up.Unit();
    sInt um = Para.Flags & 15;

    // calculate keys

    BendKey *bk = new BendKey[count];
    for(sInt i=0;i<count;i++)
    {
      sF32 data[8];
      sF32 datat[8];
      sF32 fi = sF32(i)/count;
      bk[i].Time = t0+td*fi;
      spline->Eval(bk[i].Time,data,sMin(5,spline->Count));
      spline->Eval(bk[i].Time+0.01,datat,sMin(3,spline->Count));
      bk[i].Pos.x = data[0];
      bk[i].Pos.y = data[1];
      bk[i].Pos.z = data[2];
      bk[i].Dir.x = datat[0]-data[0];
      bk[i].Dir.y = datat[1]-data[1];
      bk[i].Dir.z = datat[2]-data[2];
      if(spline->Count>=3)
        bk[i].Twist = data[3];
      else
        bk[i].Twist = 0;
      bk[i].Twist += Para.Twist + Para.Twirl*fi;
      if(spline->Count>=4)
        bk[i].Scale = data[4];
      else
        bk[i].Scale = 1;
    }

    // the original direction


    BendMatrices(count,bk,mats,up,um);
    delete[] bk;

    // basepose matrix

    for(sInt i=0;i<count;i++)
    {
      sMatrix34 mat = mats[i];
      mats[i] = Mesh->Skeleton->Joints[i].BasePose * mat;
    }
  }
}

void RNBoneTrain::Prepare(Wz4RenderContext *ctx)
{
  if(Mesh->Skeleton && Mesh->Skeleton->Joints.GetCount()==Count)
  {
    Mesh->BeforeFrame(Para.LightEnv);
  }
}

void RNBoneTrain::Render(Wz4RenderContext *ctx)
{
  if(Mesh->Skeleton && Mesh->Skeleton->Joints.GetCount()==Count)
  {
    sInt max = Count;

    sMatrix34CM *matp;

    sFORALL(Matrices,matp)
    {
      /*
      for(sInt i=0;i<max;i++)
        mats0[i] = sMatrix34CM(mats[i])*(*matp);

      Mesh->RenderBone(ctx->RenderMode,Para.LightEnv,max,mats0,max);
      */
      for(sInt i=0;i<max;i++)
        mats0[i] = sMatrix34CM(mats[i]);
      Mesh->RenderBone(ctx->RenderMode,Para.LightEnv,max,mats0,max,matp);
    }
  }
}

/****************************************************************************/
/****************************************************************************/

