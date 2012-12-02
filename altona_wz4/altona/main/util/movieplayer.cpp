/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "movieplayer.hpp"
#include "shaders.hpp"
#include "base/system.hpp"
#include "base/graphics.hpp"

/****************************************************************************/

void sMoviePlayer::RenderToScreen(sBool zoom)
{
  // this is suboptimal but should do the trick for starters. You're always
  // free to override this function

  sFRect uv;
  sMaterial *mtrl;
  mtrl = GetFrame(uv);

  if (mtrl)
  {
    sGeometry geo;

    // make destination rect according to movie and screen aspect
    sMovieInfo info = GetInfo();
    sF32 arr=info.Aspect/sGetRendertargetAspect();
    sF32 w=1, h=1;
    // reverted change; the line below WORKS. if you want to switch behaviour, why not invert
    // the zoom parameter - that's what it is for.
    if ((arr>1.0f)^zoom) h/=arr; else w*=arr; 
    sFRect dest((1-w)/2,(1-h)/2,(1+w)/2,(1+h)/2);

    sU16 *ip;
    sVertexSingle *vp=0L;
    geo.BeginLoad(sVertexFormatSingle,sGF_INDEX16|sGF_TRILIST,sGD_STREAM,4,6,&vp,&ip);
    vp[0].Init(dest.x0,dest.y0,0.01f,0xffffffff,uv.x0,uv.y0);
    vp[1].Init(dest.x1,dest.y0,0.01f,0xffffffff,uv.x1,uv.y0);
    vp[2].Init(dest.x0,dest.y1,0.01f,0xffffffff,uv.x0,uv.y1);
    vp[3].Init(dest.x1,dest.y1,0.01f,0xffffffff,uv.x1,uv.y1);
    *ip++=0; *ip++=1; *ip++=2; *ip++=3; *ip++=2; *ip++=1;
    geo.EndLoad();

    sViewport view;
    view.Orthogonal=sVO_SCREEN;
    view.SetTargetCurrent();
    view.Prepare();

    sCBuffer<sSimpleMaterialPara> cb;
    cb.Data->Set(view);
    mtrl->Set(&cb);

    geo.Draw();
  }
}

/****************************************************************************/

