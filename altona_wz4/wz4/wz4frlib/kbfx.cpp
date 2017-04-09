/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/kbfx.hpp"
#include "util/ipp.hpp"
#include "base/graphics.hpp"
#include "wz4frlib/kbfx_shader.hpp"
#include "wz4frlib/wz4_ipp.hpp"

/****************************************************************************/

/****************************************************************************/

RNVideoDistort::RNVideoDistort()
{
    Anim.Init(Wz4RenderType->Script);
    Mtrl = new VideoDistortMtrl();
    Mtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF;
    Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
    Mtrl->Prepare(sVertexFormatSingle);

    Geo = new sGeometry(sGF_QUADLIST, sVertexFormatSingle);

    LastTime = 0;
}

RNVideoDistort::~RNVideoDistort()
{
    delete Mtrl;
    delete Geo;
}

void RNVideoDistort::Simulate(Wz4RenderContext *ctx)
{
    Para = ParaBase;
    Anim.Bind(ctx->Script, &Para);
    SimulateCalc(ctx);
    SimulateChilds(ctx);
}

void RNVideoDistort::Render(Wz4RenderContext *ctx)
{
    RenderChilds(ctx);

    if ((ctx->RenderMode & sRF_TARGET_MASK) == sRF_TARGET_MAIN)
    {
        sTexture2D *src = sRTMan->ReadScreen();
        sTexture2D *dest = sRTMan->WriteScreen();
        sRTMan->SetTarget(dest);

        sViewport view;
        view.Orthogonal = sVO_PIXELS;
        view.SetTargetCurrent();
        view.Prepare();

        int GS = 16;
        int gridx = (view.TargetSizeX + GS - 1) / GS;
        int gridy = (view.TargetSizeY + GS - 1) / GS;
        int gridn = gridx*gridy;

        sF32 time = ctx->GetBaseTime();
        if (LastTime == 0) LastTime = time;
        sF32 td = time - LastTime;

        if (int(time / Para.KeyFrameInterval) != int(LastTime / Para.KeyFrameInterval) || Blocks.GetCount() != gridn)
        {
            Blocks.Resize(gridn);
            for (int i = 0; i < gridn; i++)
            {
                Blocks[i].Index = i + 100*gridn;
                Blocks[i].Color = 0;
            }
        }

        sInt a = int(LastTime * 100);
        sInt b = int(time * 100);

        for (sInt i = a; i < b; i++)
            if (Rnd.Float(1) < Para.Chance)
                AddGlitch(gridx, gridy);

        sCBuffer<VideoDistortVSPara> cbv;
        sCBuffer<VideoDistortPSPara> cbp;

        cbv.Data->mvp = view.ModelScreen;

        sF32 stepu = (sF32)GS / (sF32)view.TargetSizeX;
        sF32 stepv = (sF32)GS / (sF32)view.TargetSizeY;

        Mtrl->Texture[0] = src;
        Mtrl->Set(&cbv, &cbp);

        sGraphicsCaps caps;
        sGetGraphicsCaps(caps);
        sF32 xy = caps.XYOffset;
        sF32 uv = caps.UVOffset;

        int nb = 0;
        sVertexSingle *vp, *ovp;
        Geo->BeginLoadVB(4 * gridn, sGD_STREAM, &vp);
        ovp = vp;
        
        for (int i = 0; i < gridn; i++)
        {
            sInt index = Blocks[i].Index % gridn;
            sU32 col = Blocks[i].Color;

            sInt gx = index % gridx;
            sInt gy = index / gridx;

            sF32 u0 = gx * stepu + uv;
            sF32 v0 = gy * stepv + uv;
            sF32 u1 = u0 + stepu;
            sF32 v1 = v0 + stepv;

            sF32 sx = GS*(i % gridx) + xy;
            sF32 sy = GS*(i / gridx) + xy;

            vp++->Init(sx     , sy     , 0, col, u0, v0);
            vp++->Init(sx + GS, sy     , 0, col, u1, v0);
            vp++->Init(sx + GS, sy + GS, 0, col, u1, v1);
            vp++->Init(sx     , sy + GS, 0, col, u0, v1);

        }

        Geo->EndLoadVB(vp-ovp);
        Geo->Draw();
        
        LastTime = time;
        Mtrl->Texture[0] = 0;
        Mtrl->Prepare(sVertexFormatSingle);
        sRTMan->Release(src);
        sRTMan->Release(dest);
    }
}


void RNVideoDistort::AddGlitch(int gx, int gy)
{
    int gn = gx*gy;
    int type = Rnd.Int(5);
    switch (type)
    {
    case 0:
    {
        // shift picture around
        int start = Rnd.Int(gn);
        int offs = Rnd.Int(gx)-gx/2;
        for (int i = start; i < gn; i++)
            Blocks[i].Index += offs;
    } break;
    case 1:
    {
        // colorize
        int start = Rnd.Int(gn);
        int end = sMin(start + Rnd.Int(gx), gn);
        sU32 c = Rnd.Int(2) ? 0x00ff0000 : 0x0000ff00;
        for (int i = start; i < end; i++)
            Blocks[i].Color |= c;
    } break;
    case 2:
    {
        // line shift
        int start = Rnd.Int(gn);
        int end = sMin(start + Rnd.Int(2 * gx), gn);
        sU32 c = Rnd.Int(2) ? 0x00ff0000 : 0x0000ff00;
        for (int i = start; i < end; i++)
            Blocks[i].Index += gx;
    } break;
    case 3:
    {
        // random block to random pos
        Blocks[Rnd.Int(gn)].Index = Rnd.Int(gn) + 100 * gn;
    }
    case 4:
    {
        // random block to random color
        Blocks[Rnd.Int(gn)].Color = Rnd.Int(2) ? 0x00ff0000 : 0x0000ff00;
    }
    default:
    {
    } break;
    }
}

/****************************************************************************/

RNBlockTransition::RNBlockTransition()
{  
  Anim.Init(Wz4RenderType->Script);
  Mtrl = 0;
  Geo = 0;
  Texture = 0;
  LastTime = 0;
}

RNBlockTransition::~RNBlockTransition()
{
  sRelease(Texture);
  delete Mtrl;
  delete Geo;
}

void RNBlockTransition::Init()
{
  Geo = new sGeometry(sGF_QUADLIST, sVertexFormatSingle);

  Mtrl = new BlockTransMtrl();
  Mtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  if (Texture) Mtrl->Texture[0] = Texture->Texture;
  Mtrl->Prepare(sVertexFormatSingle);
}

void RNBlockTransition::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script, &Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNBlockTransition::Render(Wz4RenderContext *ctx)
{
  if ((ctx->RenderMode & sRF_TARGET_MASK) == sRF_TARGET_MAIN)
  {
    sViewport view;
    view.Orthogonal = sVO_PIXELS;
    view.SetTargetCurrent();
    view.Prepare();

    int GS = Para.BlockSize;
    int gridx = (view.Target.SizeX() + GS - 1) / GS;
    int gridy = (view.Target.SizeY() + GS - 1) / GS;
    int gridn = gridx*gridy;

    Blocks.Clear();
    Blocks.HintSize(gridn);
    sF32 stepu = (sF32)GS / (sF32)view.Target.SizeX();
    sF32 stepv = (sF32)GS / (sF32)view.Target.SizeY();

    Rnd.Seed((sU32)Para.Seed);

    int n = 0;
    while (n < gridn)
    {
      int w = Para.StreakSize + Rnd.Int(Para.StreakDeviate+1) - Para.StreakDeviate / 2;
      bool yo = Rnd.Float(1) <= Para.Chance;
      int srcdev = (Rnd.Int(2 * Para.SrcDeviate + 1) - Para.SrcDeviate);

      w = sMin(w, gridn - n);
      if (w <= 0) continue;

      if (yo)
      {
        for (int i = 0; i < w; i++)
        {
          Block b;
          b.Color = 0xffffffff;
          b.Index = n + i;
          b.SrcIndex = n + i + srcdev;
          Blocks.AddTail(b);
        }
      }

      n += w;
    }
      
    if (Blocks.IsEmpty())
      return;

    sGraphicsCaps caps;
    sGetGraphicsCaps(caps);
    sF32 xy = caps.XYOffset;
    sF32 uv = caps.UVOffset;

    sCBuffer<BlockTransVSPara> cbv;
    cbv.Data->mvp = view.ModelScreen;
    
    Mtrl->Set(&cbv);

    sVertexSingle *vp, *ovp;
    Geo->BeginLoadVB(4 * gridn, sGD_STREAM, &vp);
    ovp = vp;

    for (int i = 0; i < Blocks.GetCount(); i++)
    {
      sInt index = Blocks[i].Index % gridn;
      sInt srcindex = Blocks[i].SrcIndex % gridn;
      sU32 col = Blocks[i].Color;

      sInt gx = srcindex % gridx;
      sInt gy = srcindex / gridx;

      sF32 u0 = gx * stepu + uv;
      sF32 v0 = gy * stepv + uv;
      sF32 u1 = u0 + stepu;
      sF32 v1 = v0 + stepv;

      sF32 sx = GS*(index % gridx) + xy;
      sF32 sy = GS*(index / gridx) + xy;

      vp++->Init(sx, sy, 0, col, u0, v0);
      vp++->Init(sx + GS, sy, 0, col, u1, v0);
      vp++->Init(sx + GS, sy + GS, 0, col, u1, v1);
      vp++->Init(sx, sy + GS, 0, col, u0, v1);
    }

    Geo->EndLoadVB(vp - ovp);
    Geo->Draw();
  }
}



/****************************************************************************/

RNGlitch2D::RNGlitch2D()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = 0;
  Geo = 0;
  Texture = 0;
  LastTime = 0;
}

RNGlitch2D::~RNGlitch2D()
{
  sRelease(Texture);
  delete Mtrl;
  delete Geo;
}

void RNGlitch2D::Init()
{
  Geo = new sGeometry(sGF_QUADLIST, sVertexFormatDouble);

  Mtrl = new Glitch2DMtrl();
  Mtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_TILE | sMTF_EXTERN;
  if (Texture) Mtrl->Texture[0] = Texture->Texture;
  Mtrl->Prepare(sVertexFormatDouble);
}

void RNGlitch2D::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script, &Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNGlitch2D::Render(Wz4RenderContext *ctx)
{
  if ((ctx->RenderMode & sRF_TARGET_MASK) == sRF_TARGET_MAIN)
  {
    sViewport view;
    view.Orthogonal = sVO_PIXELS;
    view.SetTargetCurrent();
    view.Prepare();

    int GS = Para.LineSize;
    int gridy = (view.Target.SizeY() + GS - 1) / GS;

    Rnd.Seed((sU32)Para.Seed);

    sGraphicsCaps caps;
    sGetGraphicsCaps(caps);
    sF32 xy = caps.XYOffset;
    sF32 uv = caps.UVOffset;
    sF32 stepv = (sF32)GS / (sF32)view.Target.SizeY();

    sCBuffer<BlockTransVSPara> cbv;
    cbv.Data->mvp = view.ModelScreen;

    Mtrl->Set(&cbv);
    
    sVertexDouble *vp, *ovp;
    Geo->BeginLoadVB(4 * gridy, sGD_STREAM, &vp);
    ovp = vp;

    float x0 = xy;
    float x1 = x0 + view.Target.SizeX();


    for (int i = 0; i < gridy; i++)
    {
      bool yo = Rnd.Float(1) <= Para.Chance;
      sF32 xdev = Rnd.FloatSigned(1);

      sU32 col = 0xffffffff;
      sF32 u0 = uv;
      sF32 v0 = i * stepv + uv;
      sF32 sy = GS* i + xy;
      float t2u = 0;

      if (yo)
      {
        u0 += Para.ShiftX * xdev;
        v0 += Para.ShiftY * xdev;
        t2u += Para.ShiftRGB * xdev;
      }

      sF32 u1 = u0 + 1;
      sF32 v1 = v0 + stepv;

      vp++->Init(x0, sy, 0, col, u0, v0, t2u, 0);
      vp++->Init(x1, sy, 0, col, u1, v0, t2u, 0);
      vp++->Init(x1, sy + GS, 0, col, u1, v1, t2u, 0);
      vp++->Init(x0, sy + GS, 0, col, u0, v1, t2u, 0);
    }

    Geo->EndLoadVB(vp - ovp);
    Geo->Draw();
 
  }
}



/****************************************************************************/