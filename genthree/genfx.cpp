// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "cslrt.hpp"
#include "genfx.hpp"
#include "genmesh.hpp"
#include "genmaterial.hpp"
#include "genmisc.hpp"
#include "_util.hpp"
#include "_startdx.hpp"

#define RH_NOP					0
#define RH_BEGINVIEW		1
#define RH_ENDVIEW			2
#define RH_LAYER2D			3
#define RH_MERGECOLOR		4
#define RH_PAINT				5
#define RH_BLEND4X			6

sMAKEZONE(FXRender,"FXRender",0xff20e020);
sU32 GetColor32(const sInt4 &colx);


static void RotateFromChain(sF32 *dest,sF32 x,sF32 y,GenFXChain *chain,sBool invert=sFALSE)
{
	sF32 angle,zoom[2],m00,m01,m10,m11;
	sInt i;

	for(i=0;i<2;i++)
	{
    zoom[i]=(&chain->Zoom.x)[i]; if(!zoom[i]) zoom[i]=0.01f;
    if(!invert)
      zoom[i]=1.0f/zoom[i];
	}

	angle = chain->Rotate*sPI2F/65536.0f;
	m00 = sFCos(angle)*zoom[0];
	m01 = sFSin(angle)*zoom[1];
	m10 = -sFSin(angle)*zoom[0];
	m11 = sFCos(angle)*zoom[1];
	if(invert)
		sSwap(m01,m10);

	dest[0] = x*m00 + y*m01 + (chain->Center.x+0.5f-m00-m01)*0.5f;
	dest[1] = x*m10 + y*m11 + (chain->Center.y+0.5f-m10-m11)*0.5f;
}

/****************************************************************************/

static void RH_Nop(GenFXChain *chain,FXViewport &view)
{
	view.ClearFlags = sVCF_ALL;
	view.ClearColor.Color = GetColor32(chain->Color);
	sSystem->BeginViewport(view);
	sSystem->EndViewport();
}

sObject * __stdcall FXChain_New(sInt size,sInt ocount,sInt4 color)
{
	GenFXChain *chain;

	chain = new GenFXChain;
	chain->Viewport = FXMaster->Alloc(ocount>>16,size>>16);
	chain->Color = color;
	SCRIPTVERIFY(chain->Viewport != -1);
	return chain;
}

/****************************************************************************/

static void RH_BeginViewport(GenFXChain *chain,FXViewport &view)
{
	sCamera cam;
	sVector vec,pos;
	sInt i;

//	cam.Init();
	for(i=0;i<3;i++)
	{
		(&pos.x)[i]=(&chain->CamPos.x)[i];
		(&vec.x)[i]=(&chain->CamRot.x)[i];
	}

	switch(chain->Param[0]>>2)
	{
	case 0: // rotate
    vec.x /= sPI2F;   // CHAOS: wertebereich ändern!
    vec.y /= sPI2F;
    vec.z /= sPI2F;
		cam.CamPos.InitEulerPI2(&vec.x); 
		break;

	case 1: // lookat
		vec.Sub3(pos);
		cam.CamPos.InitDir(vec);
		break;
	}

	cam.CamPos.l=pos;
	cam.CamPos.l.w = 1.0f;
	cam.ZoomX = cam.ZoomY = chain->Zoom.x / 65536.0f;
	cam.AspectX = 1.333f;
	cam.AspectY = chain->Zoom.y / 65536.0f;
  cam.CenterX = 0.0f; cam.CenterY = 0.0f;
  cam.NearClip = 0.125f;
  cam.FarClip = 256.0f;

	view.ClearFlags = chain->Param[0]&3;
	view.ClearColor.Color = GetColor32(chain->Color);
	sSystem->BeginViewport(view);
	sSystem->SetCamera(cam);
}

sObject * __stdcall FXChain_BeginViewport(GenFXChain *chn,sInt3 pos,sInt3 parm2,sInt aspect,sInt mode,sInt ocount,sInt4 color,sInt zoom)
{
	GenFXChain *chain;
	sInt i;

	SCRIPTVERIFY(chn);

	chain = new GenFXChain;
	chain->RenderHandler = RH_BEGINVIEW;
	chain->Input[0] = chn;
	chain->Viewport = chn->Viewport;
	chain->Zoom.y = aspect;
	chain->Param[0] = mode>>16;
	chain->Zoom.x = zoom;
	chain->Color = color;
	for(i=0;i<6;i++)
		(&chain->CamPos.x)[i] = (&pos.x)[i]/65536.0f;
	FXMaster->Ref(chain->Viewport,sAbs(ocount>>16)-1);

  return chain;
}

sObject * __stdcall FXChain_Viewport(GenFXChain *chn,GenScene *scene,sInt3 pos,sInt3 parm2,sInt aspect,sInt mode,sInt ocount,sInt4 color,sInt zoom)
{
  chn = (GenFXChain *) FXChain_BeginViewport(chn,pos,parm2,aspect,mode,0x10000,color,zoom);
  chn = (GenFXChain *) FXChain_PaintScene(chn,scene,0x10000);
  chn = (GenFXChain *) FXChain_EndViewport(chn,ocount);
  return chn;
}

/****************************************************************************/

void __stdcall FXChain_Render(GenFXChain *chain)
{
	SCRIPTVERIFYVOID(chain);
	chain->Render();
}

/****************************************************************************/

static void RH_Layer2D(GenFXChain *chain,FXViewport &view)
{
	sMatrix mat;
	GenMaterial *mtrl;
	sF32 *vp,buf[2];
	sInt i,j;
	sInt handle;

	view.ClearFlags = (chain->Param[9] >> 16) & 3;
	sSystem->BeginViewport(view);
	mtrl=(GenMaterial *) chain->Object;
	mat.Init();
#if sINTRO
	mat.i.x = sSystem->DView.Width;
  mat.j.y = sSystem->DView.Height;
#else
	mat.i.x = sSystem->ViewportX;
	mat.j.y = sSystem->ViewportY;
#endif

	sSystem->SetCameraOrtho();
	sSystem->SetMatrix(mat);
	mtrl->Set();
  if(chain->Param[9] & 0x20000) // clearz?
    sSystem->SetState(sD3DRS_ZFUNC,sD3DCMP_ALWAYS); // safer with catalyst 3.8+

	handle = sSystem->GeoAdd(sFVF_DEFAULT2D,sGEO_QUAD);
	sSystem->GeoBegin(handle,4,0,&vp,0);

	for(i=0;i<4;i++)
	{
		j=i^(i>>1);

		RotateFromChain(buf,
			((j&1) ? chain->Param[2] : chain->Param[0]) / 65536.0f,
			((j&2) ? chain->Param[3] : chain->Param[1]) / 65536.0f,chain,sTRUE);
    *vp++ = buf[0] - 0.5f/mat.i.x;
    *vp++ = buf[1] - 0.5f/mat.j.y;
		*vp++ = chain->Param[8] / 65536.0f;
		*((sU32 *) vp) = GetColor32(chain->Color);
		vp++;
		*vp++ = ((j&1) ? chain->Param[6] : chain->Param[4]) / 65536.0f;
		*vp++ = ((j&2) ? chain->Param[7] : chain->Param[5]) / 65536.0f;
		*vp++ = 0.0f;
		*vp++ = 0.0f;
	}

	sSystem->GeoEnd(handle);
  sSystem->GeoDraw(handle);
	sSystem->GeoRem(handle);
	sSystem->EndViewport();
}

sObject * __stdcall FXChain_Layer2D(GenFXChain *chn,GenMaterial *mtrl,sInt4 color,sInt4 screen,sInt4 uv,sInt z,sInt clear,sInt ocount,sF322 zoom,sInt rotate,sF322 center)
{
	GenFXChain *chain;
	sInt i;

	SCRIPTVERIFY(chn);
	SCRIPTVERIFY(mtrl);

	chain = new GenFXChain;
	chain->Object = mtrl;
	chain->RenderHandler = RH_LAYER2D;
	chain->Input[0] = chn;
	chain->Color = color;
	chain->Zoom = zoom;
	chain->Rotate = rotate;
	chain->Center = center;

	for(i=0;i<10;i++)
		chain->Param[i]=(&screen.x)[i];
	chain->Viewport = chn->Viewport;
	FXMaster->Ref(chain->Viewport,sAbs(ocount>>16)-1);

	return chain;
}

/****************************************************************************/

static void RH_MergeColor(GenFXChain *chain,FXViewport &view)
{
	sU32 states[256],*p,color;
	sF32 *vp,buf[2];
	sInt i,j,handle;
	FXViewport *src1,*src2;
	sMatrix mat;

	sVERIFY(chain->Input[0]->Viewport > 0);
	
  view.ClearFlags = sVCF_COLOR;
	sSystem->BeginViewport(view);
	p = states;
	src1 = &FXMaster->Viewport[chain->Input[0]->Viewport];
	if(chain->Input[1])
		src2 = &FXMaster->Viewport[chain->Input[1]->Viewport];
	else
		src2 = 0;

	mat.Init();
	color = chain->Param[1];

	sSystem->StateBase(p,0,chain->Param[0],0);
	sSystem->StateTex(p,0,sMTF_CLAMP|sMTF_FILTER);
	sSystem->StateTex(p,1,sMTF_UV1|sMTF_FILTER|sMTF_CLAMP);
  if(!src2)
  {
    *p++ = sD3DTSS_1|sD3DTSS_COLOROP;   *p++ = sD3DTOP_MODULATE4X;
    *p++ = sD3DTSS_1|sD3DTSS_COLORARG1; *p++ = sD3DTA_CURRENT;
    *p++ = sD3DTSS_1|sD3DTSS_COLORARG2; *p++ = sD3DTA_DIFFUSE|sD3DTA_ALPHAREPLICATE;
    *p++ = sD3DTSS_1|sD3DTSS_ALPHAOP;   *p++ = sD3DTOP_SELECTARG1;
    *p++ = sD3DTSS_1|sD3DTSS_ALPHAARG1; *p++ = sD3DTA_CURRENT;
    *p++ = sD3DTSS_2|sD3DTSS_COLOROP;   *p++ = sD3DTOP_DISABLE;

    color = (color & 0xffffff) | (chain->Param[2] << 24);
  }
	sSystem->StateEnd(p,states,sizeof(states));
	
	sSystem->SetCameraOrtho();
	sSystem->SetMatrix(mat);
	sSystem->SetStates(states);
	sSystem->SetTexture(0,src1->RenderTarget);
	if(src2)
		sSystem->SetTexture(1,src2->RenderTarget);
	else
		src2 = src1;

	handle = sSystem->GeoAdd(sFVF_DEFAULT2D,sGEO_QUAD);
	sSystem->GeoBegin(handle,4,0,&vp,0);

	for(i=0;i<4;i++)
	{
		j=i^(i>>1);

		*vp++ = ((j&1) ? view.Window.XSize() : 0) - 0.5f;
		*vp++ = ((j&2) ? view.Window.YSize() : 0) - 0.5f;
		*vp++ = 1.0f;
		*((sU32 *) vp) = color; vp++;
		RotateFromChain(buf,(j&1)?1.0f:0.0f,(j&2)?1.0f:0.0f,chain);
    *vp++ = buf[0] * src1->XScale;
    *vp++ = buf[1] * src1->YScale;
		*vp++ = (j&1) ? src2->XScale : 0.0f;
		*vp++ = (j&2) ? src2->YScale : 0.0f;
	}

	sSystem->GeoEnd(handle);
  sSystem->GeoDraw(handle);
	sSystem->GeoRem(handle);
	sSystem->EndViewport();
  sSystem->SetTexture(0,0);
  sSystem->SetTexture(1,0);
}

sObject * __stdcall FXChain_Merge(GenFXChain *c1,GenFXChain *c2,sInt mode,sInt param,sF322 zoom,sInt rotate,sF322 center,sInt size,sInt ocount)
{
	GenFXChain *chain;

	SCRIPTVERIFY(c1);
	SCRIPTVERIFY(c2);

	chain = new GenFXChain;
	chain->RenderHandler = RH_MERGECOLOR;
	chain->Input[0] = c1;
	chain->Input[1] = c2;
	chain->Param[0] = (mode>>16)+3;
	chain->Param[1] = (sRange<sU32>(param>>8,255,0)<<24) | 0xffffff;
	chain->Zoom = zoom;
	chain->Rotate = rotate;
	chain->Center = center;
	chain->Viewport = FXMaster->Alloc(ocount>>16,size>>16);
	SCRIPTVERIFY(chain->Viewport!=-1);
	FXMaster->Ref(c1->Viewport,-1);
	FXMaster->Ref(c2->Viewport,-1);

	return chain;
}

sObject * __stdcall FXChain_Color(GenFXChain *chn,sInt mode,sInt4 color,sInt size,sInt ocount,sF322 zoom,sInt rotate,sF322 center,sInt lighten)
{
	GenFXChain *chain;

	SCRIPTVERIFY(chn);

	chain = new GenFXChain;
	chain->RenderHandler = RH_MERGECOLOR;
	chain->Input[0] = chn;
	chain->Param[0] = (mode>>16)+sMBM_ADDCOL;
	chain->Param[1] = GetColor32(color);
  chain->Param[2] = sRange((lighten+65536)>>10,255,0);
	chain->Zoom = zoom;
	chain->Rotate = rotate;
	chain->Center = center;
	chain->Viewport = FXMaster->Alloc(ocount>>16,size>>16);
	SCRIPTVERIFY(chain->Viewport!=-1);
	FXMaster->Ref(chn->Viewport,-1);

	return chain;
}

/****************************************************************************/

void RH_EndViewport(GenFXChain *chain,FXViewport &view)
{
	sSystem->EndViewport();
}

sObject * __stdcall FXChain_EndViewport(GenFXChain *chn,sInt ocount)
{
	GenFXChain *chain;

	SCRIPTVERIFY(chn);

	chain = new GenFXChain;
	chain->RenderHandler = RH_ENDVIEW;
	chain->Input[0] = chn;
	chain->Viewport = chn->Viewport;
	FXMaster->Ref(chain->Viewport,sAbs(ocount>>16)-1);

	return chain;
}

/****************************************************************************/

void RH_PaintScene(GenFXChain *chain,FXViewport &view)
{
	sMatrix mat;
	mat.Init();
	((GenScene *) chain->Object)->Paint(mat);
}

sObject * __stdcall FXChain_PaintScene(GenFXChain *chn,GenScene *scn,sInt ocount)
{
	GenFXChain *chain;

	SCRIPTVERIFY(chn);
	SCRIPTVERIFY(scn);

	chain = new GenFXChain;
	chain->RenderHandler = RH_PAINT;
	chain->Object = scn;
	chain->Input[0] = chn;
	chain->Viewport = chn->Viewport;
	FXMaster->Ref(chain->Viewport,sAbs(ocount>>16)-1);

	return chain;
}

/****************************************************************************/

void __stdcall FXChain_ResetAlloc()
{
  FXMaster->ResetAlloc();
}

/****************************************************************************/

void RH_Blend4x(GenFXChain *chain,FXViewport &view)
{
	FXViewport *src;
	sU32 states[256],*p;
	sF32 *vp,*ip,sx,sy,ox,oy;
	sMatrix mat;
	sInt handle,i,j,k;

	sSystem->BeginViewport(view);
	src = &FXMaster->Viewport[chain->Input[0]->Viewport];
	p = states;
	sSystem->StateTex(p,0,sMTF_CLAMP|sMTF_FILTER);
	sSystem->StateTex(p,1,sMTF_UV1|sMTF_CLAMP|sMTF_FILTER);
	sSystem->SetCameraOrtho();
	mat.Init();
	sSystem->SetMatrix(mat);
	sSystem->SetTexture(0,src->RenderTarget);
	sSystem->SetTexture(1,src->RenderTarget);

	ox = 0.0f;
	oy = 0.0f;
	sx = src->XScale;
	sy = src->YScale;

	if(chain->Param[0]) // coordmode not 0?
	{
		ox = src->Window.XSize();
		oy = src->Window.YSize();
		sx /= ox;
		sy /= oy;
	}

	ip = chain->ParamF;
	handle = sSystem->GeoAdd(sFVF_DEFAULT2D,sGEO_QUAD);
	for(i=0;i<2;i++)
	{
		sSystem->StateBase(p,sMBF_ZOFF|(i?sMBF_BLENDADD:0),sMBM_TEX,0);
		p[0] = sD3DTSS_ALPHAOP | sD3DTSS_0;		p[1] = sD3DTOP_SELECTARG2;
		p[2] = sD3DTSS_COLOROP | sD3DTSS_1;		p[3] = sD3DTOP_MODULATEALPHA_ADDCOLOR;
		p[4] = sD3DTSS_COLORARG1 | sD3DTSS_1;	p[5] = sD3DTA_CURRENT;
		p[6] = sD3DTSS_COLORARG2 | sD3DTSS_1;	p[7] = sD3DTA_TEXTURE;
    p += 8;
		sSystem->StateEnd(p,states,sizeof(states));
		sSystem->SetStates(states);

		sSystem->GeoBegin(handle,4,0,&vp,0);
		for(j=0;j<4;j++)
		{
			k=j^(j>>1);
			vp[0] = ((k&1) ? view.Window.XSize() : 0) - 0.5f;
			vp[1] = ((k&2) ? view.Window.YSize() : 0) - 0.5f;
			vp[2] = 1.0f;
      ((sU32 *) vp)[3] = chain->Param[1];
      vp[4] = sx * ((k&1) ? ip[2]+ox : ip[0]);
      vp[5] = sy * ((k&2) ? ip[3]+oy : ip[1]);
      vp[6] = sx * ((k&1) ? ip[6]+ox : ip[4]);
      vp[7] = sy * ((k&2) ? ip[7]+oy : ip[5]);
      vp += 8;
		}
		ip+=8;
		sSystem->GeoEnd(handle);
    sSystem->GeoDraw(handle);

		p = states;
	}

	sSystem->GeoRem(handle);
	sSystem->EndViewport();
  sSystem->SetTexture(0,0);
  sSystem->SetTexture(1,0);
}

sObject * __stdcall FXChain_Blend4x(GenFXChain *chn,sF324 r0,sF324 r1,sF324 r2,sF324 r3,sInt cmode,sInt size,sInt ocount,sInt amplify)
{
	GenFXChain *chain;
	sInt i;

	SCRIPTVERIFY(chn);

	chain = new GenFXChain;
	chain->RenderHandler = RH_BLEND4X;
	chain->Input[0] = chn;
	chain->Viewport = FXMaster->Alloc(ocount>>16,size>>16);
	SCRIPTVERIFY(chain->Viewport != -1);
	FXMaster->Ref(chn->Viewport,-1);

	for(i=0;i<16;i++)
		chain->ParamF[i]=(&r0.x)[i];

	chain->Param[0]=cmode>>16;
	chain->Param[1]=sRange(amplify>>10,255,0)*0x01010101;

	return chain;
}

/****************************************************************************/

sObject * __stdcall FXChain_Label(GenFXChain *chain,sInt label,sInt ocount)
{
	SCRIPTVERIFY(chain);
  GenFXChain *result;

  result = chain;
  if(chain->RenderHandler == RH_ENDVIEW && chain->Input[0])
  {
    if(chain->Input[0]->RenderHandler == RH_PAINT && chain->Input[0]->Input[0])
    {
      chain = chain->Input[0]->Input[0];
    }
  }
  chain->_Label = label;
	FXMaster->Ref(result->Viewport,sAbs(ocount>>16)-1);

	return result;
}

/****************************************************************************/

typedef void (*RenderProc)(GenFXChain *chain,FXViewport &view);

static RenderProc RenderHandlers[]=
{
	RH_Nop,
	RH_BeginViewport,
	RH_EndViewport,
	RH_Layer2D,
	RH_MergeColor,
	RH_PaintScene,
	RH_Blend4x,
};

/****************************************************************************/

GenFXChain::GenFXChain()
{
	sInt i;

	for(i=0;i<FXCHAIN_MAXIN;i++)
		Input[i]=0;

	Object = 0;
	RenderHandler = RH_NOP;
	Viewport = -1;
  RenderID = 0;
}

void GenFXChain::Tag()
{
	sInt i;

	for(i=0;i<FXCHAIN_MAXIN;i++)
		sBroker->Need(Input[i]);
	sBroker->Need(Object);
}

#if !sINTRO
void GenFXChain::Copy(sObject *o)
{
	sInt i;
	GenFXChain *src;
	
	sVERIFY(o->GetClass() == sCID_GENFXCHAIN);
	src = (GenFXChain *) o;

	for(i=0;i<FXCHAIN_MAXIN;i++)
		Input[i] = src->Input[i];
	Object = src->Object;
	for(i=0;i<FXCHAIN_MAXPARAM;i++)
		Param[i] = src->Param[i];
	RenderHandler = src->RenderHandler;

	Color = src->Color;
	Zoom = src->Zoom;
	Rotate = src->Rotate;
	Center = src->Center;

	Viewport = src->Viewport;
  RenderID = src->RenderID;
  _Label = src->_Label;
}
#endif
void GenFXChain::RenderR(sInt id)
{
	sInt i;

  if(RenderID!=id)
	{
		for(i=0;i<FXCHAIN_MAXIN;i++)
		{
			if(Input[i])
				Input[i]->RenderR(id);
		}

		if(Viewport!=-1)
			RenderHandlers[RenderHandler](this,FXMaster->Viewport[Viewport]);

    RenderID = id;
	}
}

void GenFXChain::Render()
{
  static sInt RenderIDCounter=0;
	sZONE(FXRender);
	RenderR(++RenderIDCounter);
}

/****************************************************************************/

void FXViewport::InitFrom(const sViewport &vp)
{
	sViewport::operator =(vp);
	RefCount = 0;
	Size = -1;
	XScale = 1.0f;
	YScale = 1.0f;
}

void FXViewport::InitTex(sInt handle,sInt size)
{
#if sINTRO
  Screen = -1;
  RenderTarget = handle;
  Window.Init();
  ClearFlags = sVCF_ALL;
  ClearColor.Color = 0xff000000;
#else
	sViewport::InitTex(handle);
#endif
	RefCount = 0;
	Size = size;
	XScale = 1.0f;
	YScale = 1.0f;
}

/****************************************************************************/

FXMaster_::FXMaster_()
{
	static sInt counts[] = { 1,3,3,3 };

  ViewportCount = 1+1+3+3+3; // match with counts, please.
  Viewport = new FXViewport[ViewportCount]; 
  Viewport[0].RefCount = 0;
  Viewport[0].Size = -1;

	CreateRendertargets(counts);
}

FXMaster_::~FXMaster_()
{
	sInt i;

	for(i=0;i<ViewportCount;i++)
	{
		if(Viewport[i].Size!=-1)
			sSystem->RemTexture(Viewport[i].RenderTarget);
	}

  delete[] Viewport;
}

void FXMaster_::CreateRendertargets(const sInt *count)
{
	sInt i,j,handle;
	sTexInfo ti;
	FXViewport *vp;
#if sINTRO
	static sInt size[4*2] = { 1024,1024, 512,256, 256,256, 128,64 };
#else
	static sInt size[4*2] = { 1024,512 , 512,256, 256,256, 128,64 };
#endif

  vp = Viewport+1;
  /*
#if !sINTRO
  sScreenInfo si;
  sSystem->GetScreenInfo(0,si);
  size[0] = sMin(si.XSize,sSCREENX);
  size[1] = sMin(si.YSize,sSCREENY);
  sInt xs,ys;
	for(xs=1;xs<vp.Window.XSize();xs*=2);
	for(ys=1;ys<vp.Window.YSize();ys*=2);
  size[0] = xs;
  size[1] = ys;
#else
  size[0] = sSCREENX;//sMin(si.XSize,800);
  size[1] = sSCREENY;//sMin(si.YSize,600);
#endif
*/
	for(i=0;i<4;i++)
	{
    ti.XSize = size[i*2+0];
    ti.YSize = size[i*2+1];
    ti.Flags = sTIF_RENDERTARGET;
    ti.Format = 0;
    ti.Bitmap = 0;

		for(j=0;j<count[i];j++)
		{
			handle = sSystem->AddTexture(ti);
			vp->InitTex(handle,i);
			vp->Window.Init(0,0,ti.XSize,ti.YSize);
      vp->XScale = 1.0f;
      vp->YScale = 1.0f;
      vp++;
		}
	}

#if 0//sINTRO
  // assumes exactly one fullscreen rt
  vp[1].Window.x1 = 800;
  vp[1].Window.y1 = 600;
  vp[1].XScale = 800.0f / 1024.0f;
  vp[1].YScale = 600.0f / 1024.0f;
#endif
}

void FXMaster_::SetMasterViewport(const sViewport &vp)
{
	Viewport[0].InitFrom(vp);
	sVERIFY(vp.Window.XSize() && vp.Window.YSize());

  // ist jetzt immer 800x600 
  /*
#if !sINTRO
	sInt i,w,h;
	sTexInfo ti;
#endif


#if !sINTRO
	// find next power of two and size fullsize RTs accordingly
  w = vp.Window.XSize(); // rechteckige rendertargets gehen. ohne tricks. DX9 ist toll
  h = vp.Window.YSize();

//	for(w=1;w<vp.Window.XSize();w*=2);
//	for(h=1;h<vp.Window.YSize();h*=2);

	if(w!=TempBitmap[0]->XSize || h!=TempBitmap[0]->YSize)
	{
		sBroker->RemRoot(TempBitmap[0]);
		TempBitmap[0] = new sBitmap;
		TempBitmap[0]->Init(w,h);
		sBroker->AddRoot(TempBitmap[0]);
		sDPrintF("fullsize rendertarget resized: new dimensions %d by %d\n",w,h);

		ti.Init(TempBitmap[0],sTF_32A,sTIF_RENDERTARGET);

		for(i=0;i<ViewportCount;i++)
		{
			if(!Viewport[i].Size)
			{
				if(Viewport[i].RenderTarget!=-1)
					sSystem->RemTexture(Viewport[i].RenderTarget);

				Viewport[i].RenderTarget = sSystem->AddTexture(ti);
				UpdateTempViewport(i,TempBitmap[0]);
			}
		}
	}
#endif
  */
}

#if !sINTRO
void FXMaster_::UpdateTempViewport(sInt i,sBitmap *bmp)
{
  sInt xs,ys;
	FXViewport *vp;

  xs = Viewport[0].Window.XSize(); ys = Viewport[0].Window.YSize();
  vp = &Viewport[i];
  vp->Window.Init(0,0,xs,ys);
  vp->XScale = 1.0f * xs / bmp->XSize;
  vp->YScale = 1.0f * ys / bmp->YSize;
  sSystem->UpdateTexture(vp->RenderTarget,bmp);
}
#endif

sInt FXMaster_::Alloc(sInt ocount,sInt size)
{
	sInt i;

	if(ocount<=0) // allocate screen
  {
    size = -1;
    ocount = -ocount;
  }

  for(i=ViewportCount-1;i>=0;i--)
  {
    if(Viewport[i].Size==size && !Viewport[i].RefCount)
    {
      Viewport[i].RefCount = ocount;
      break;
    }
  }

#if !sINTRO
  if(i==-1 && !ScriptRuntimeError)
  {
    static sChar buffer[128];
    static sChar *FXSizes[4] = { "full","large","medium","small" };
    sSPrintF(buffer,128,"out of %s-sized fxchain buffers",(size>=0 && size<=3)?FXSizes[size]:"???");
    ScriptRuntimeError = buffer;
  }
#endif

  return i;
}

void FXMaster_::Ref(sInt handle,sInt delta)
{
	sVERIFY(handle>=0 && handle<ViewportCount);
	Viewport[handle].RefCount += delta;
	sVERIFY(Viewport[handle].RefCount>=0);
}

void FXMaster_::ResetAlloc()
{
	sInt i;

	for(i=0;i<ViewportCount;i++)
		Viewport[i].RefCount=0;
}

FXMaster_ *FXMaster=0;

void InitFXMaster()
{
	sVERIFY(FXMaster == 0);
	sREGZONE(FXRender);
	FXMaster = new FXMaster_;
}

void CloseFXMaster()
{
	sVERIFY(FXMaster != 0);
	delete FXMaster;
}
