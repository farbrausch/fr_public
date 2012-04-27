/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_demo2_ops.hpp"
#include "fr063_tron_shader.hpp"
#include "fr063_tron.hpp"

/****************************************************************************/

#define SIMSTEP (1.0f/60.0f)

/***************************************************************************
RNFR063_Sprites
***************************************************************************/

RNFR063_Sprites::RNFR063_Sprites()
{  
  static const sU32 desc[] =   
  {    
    sVF_STREAM0|sVF_UV1|sVF_F3,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_POSITION|sVF_F4,    
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_COLOR0|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV0|sVF_F4,    
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV2|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV3|sVF_F1,
    0,
  };  

  Format = sCreateVertexFormat(desc);  
  Geo = new sGeometry;
  Geo->Init(sGF_TRILIST|sGF_INDEX16,Format);
  Mtrl = 0;
  TextureDiff = 0;
  Time = 0;
  SDF = 0;

  Source = 0;

  Anim.Init(Wz4RenderType->Script);  
}

RNFR063_Sprites::~RNFR063_Sprites()
{
  delete Geo;
  delete Mtrl;
  TextureDiff->Release();
  Source->Release();
  SDF->Release();
}

void RNFR063_Sprites::Init()
{
  Para = ParaBase;

  Mtrl = new FR063StaticParticleShader;             
  Mtrl->Flags = sMTRL_CULLOFF;
  Mtrl->Texture[0] = TextureDiff->Texture;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;

  switch(Para.Mode & 0x000f)
  {
    default    : Mtrl->BlendColor = sMB_ADD; break;
    case 0x0001: Mtrl->BlendColor = sMB_PMALPHA; break;
    case 0x0002: Mtrl->BlendColor = sMB_MUL; break;
    case 0x0003: Mtrl->BlendColor = sMB_MUL2; break;
    case 0x0004: Mtrl->BlendColor = sMB_ADDSMOOTH; break;
    case 0x0005: Mtrl->BlendColor = sMB_ALPHA; break;
  }
  switch(Para.Mode & 0x0030)
  {
    default    : Mtrl->Flags |= sMTRL_ZOFF; break;
    case 0x0010: Mtrl->Flags |= sMTRL_ZREAD; break;
    case 0x0020: Mtrl->Flags |= sMTRL_ZWRITE; break;
    case 0x0030: Mtrl->Flags |= sMTRL_ZON; break;
  }
  switch(Para.Mode & 0x30000)
  {
    default     : Mtrl->BlendAlpha = sMBS_1|sMBO_ADD|sMBD_0; break;
    case 0x10000: Mtrl->BlendAlpha = sMBS_0|sMBO_ADD|sMBD_1; break;
    case 0x20000: Mtrl->BlendAlpha = sMBS_0|sMBO_ADD|sMBD_0; break;
    case 0x30000: Mtrl->BlendAlpha = sMBS_1|sMBO_ADD|sMBD_SAI; break;
  }

  Mtrl->Prepare(Format);

  PInfo.Init(Source->GetPartFlags(),Source->GetPartCount());
  Particles.AddMany(PInfo.Alloc);
  Particle *p;
  sRandom rnd;

  sF32 maxrowrandom=1.0f;
  sF32 rowoffs=0.0f;

  sFORALL(Particles,p)
  {
    p->Group=rnd.Int(Para.GroupCount);
    p->Pos.Init(0,0,0);
    p->RotStart = rnd.Float(1)-0.5f;
    p->RotRand = rnd.Float(2)-1;
    p->FadeRow = (rnd.Float(maxrowrandom)+sF32(p->Group))/sF32(Para.GroupCount)+rowoffs;
    p->SizeRand = 1 + ((rnd.Float(2)-1)*Para.SizeRand);
    p->TexAnimRand = rnd.Float(1)*Para.TexAnimRand;
    p->Dist = 0;
  }
  PartOrder.HintSize(PInfo.Alloc);
}


void RNFR063_Sprites::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);  
  SimulateCalc(ctx);
    
  Source->Simulate(ctx);
  Time = ctx->GetTime();
  SimulateChilds(ctx);
}

void RNFR063_Sprites::Prepare(Wz4RenderContext *ctx)
{
  sRandom rnd;
  sVector31 p;
  sF32 t;
  sF32 dist;
  PartVert0 *vp0;  
  Particle *part;
  sVector4 plane;
  sU16 *ip0;

  if(PInfo.Alloc==0) return;

  PartOrder.Clear();

  if (TextureDiff->Atlas.Entries.IsEmpty())
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
    UVRects.Resize(TextureDiff->Atlas.Entries.GetCount());
    BitmapAtlasEntry *e;

    sFORALL(TextureDiff->Atlas.Entries,e)
    {
      PartUVRect &r=UVRects[_i];
      r.u = e->UVs.x0;
      r.v = e->UVs.y0;
      r.du = e->UVs.x1-e->UVs.x0;
      r.dv = e->UVs.y1-e->UVs.y0;
    }
  }

  // calculate particle positions

  sViewport view = ctx->View;
  view.UpdateModelMatrix(sMatrix34(Matrices[0]));
  sMatrix34 mat;
  mat = view.Camera;
  plane.InitPlane(mat.l,mat.k);

  dist = (mat.l-(Para.Trans*view.Model)).Length();

  Source->Func(PInfo,Time,0);

  if(Para.Mode & 0x800)
  {
    sInt max = PInfo.GetCount();
    for(sInt i=0;i<max;i++)
    {
      PInfo.Parts[i].Get(p,t);
      if (t<0) continue;
      part = &Particles[i];

      p+=sVector30(Para.Trans);
      dist=-(plane ^ (p*view.Model));

      part->Time = t;
      part->Pos = p;
      part->Dist = dist;
      part->DistFade = 1;

      PartOrder.AddTail(part);
    }
  }
  else
  {
    if (dist>Para.FarFadeDistance) return;

    sF32 distfade=1.0f/Para.NearFadeDistance;
    sF32 globalfade=1.0f;
    if (dist>=(Para.FarFadeDistance-Para.FarFadeRange))
      globalfade=sClamp((Para.FarFadeDistance-dist)/Para.FarFadeRange,0.0f,1.0f);

    sInt max = PInfo.GetCount();
    for(sInt i=0;i<max;i++)
    {
      PInfo.Parts[i].Get(p,t);
      if (t<0) continue;
      part = &Particles[i];

      p+=sVector30(Para.Trans);
      dist=-(plane ^ (p*view.Model));

      if (dist<=-view.ClipNear)
      {
        part->Time = t;
        part->Pos = p;
        part->Dist = dist;

        sF32 df=sClamp(-(part->Dist+view.ClipNear)*distfade,0.0f,1.0f)*globalfade;

        part->DistFade = sSmooth(df*df);

        PartOrder.AddTail(part);
      }
    }
  }

  if (PartOrder.IsEmpty())
    return;

  if(Para.Mode & 0x0100)
    sHeapSortUp(PartOrder,&Particle::Dist);


  // prepare instance data

  Geo->BeginLoadVB(4,sGD_FRAME,&vp0,0);
  vp0[0].Init(1,0,sPI2F*0.25f*0);
  vp0[1].Init(1,1,sPI2F*0.25f*1);
  vp0[2].Init(0,1,sPI2F*0.25f*2);
  vp0[3].Init(0,0,sPI2F*0.25f*3);
  Geo->EndLoadVB(-1,0);
  Geo->BeginLoadIB(6,sGD_FRAME,&ip0);    // yes we need an index buffer, or D3DDebug is unhappy
  sQuad(ip0,0,0,1,2,3);
  Geo->EndLoadIB();

  // prepare particle data

  sF32 sx = Para.Size*sSqrt(sPow(2,Para.Aspect));
  sF32 sy = Para.Size/sSqrt(sPow(2,Para.Aspect));

  const sInt uvcounti = UVRects.GetCount()/Para.GroupCount;
  const sF32 uvcountf = uvcounti; 

  sF32 ga,gb,gc,gd;
  switch(Para.GrowMode)
  {
    default:
    case 0:    ga = 1; gb = 0; gc = 0; gd = 0;    break;
    case 1:    ga = 0; gb = 1; gc = 0; gd = 0;    break;
    case 2:    ga = 0; gb = 2; gc =-1; gd = 0;    break;
    case 3:    ga = 0; gb = 0; gc = 3; gd =-2;    break;
  }
  sF32 fi = Para.FadeIn;
  sF32 fo = Para.FadeOut;  

  if(fi+fo>1.0f)
    fo = 1-fi;

  sF32 fii = 0;
  sF32 foi = 0;
  if(fi>0.01f)
    fii = 1/fi;
  else 
    fi = 0; 
  if(fo>0.01f)
    foi = 1/fo;
  else 
    fo = 0;
  if(Para.GrowMode==0)
    fi = fo = 0;
  

  KeepCount=0;

  sAABBox box;
  SDF->GetObj()->GetBox(box);
  sF32 bl=box.Size().Length();

  sFORALL(PartOrder,part)
  {
    sVector30 n;
    sF32 fade=part->DistFade;
    part->GeoDataCache.rot = sMod(Para.RotStart+part->RotStart*Para.RotSpread+Time*(Para.RotSpeed+Para.RotRand*part->RotRand),1)*sPI2F;
    part->GeoDataCache.color = sVector4(1.0f, 1.0f, 1.0f, 1.0f);    
    SDF->GetObj()->GetNormal(part->Pos,n);
    n=-n;

    sVector4 in_ambient;
    sVector4 in_diffuse;
    sVector4 in_specular;
    
    sVector4 out_ambient;
    sVector4 out_diffuse;
    sVector4 out_specular;

    in_ambient.InitColor(Para.InAmbientColor);
    in_diffuse.InitColor(Para.InDiffuseColor);
    in_specular.InitColor(Para.InSpecColor);
   
    out_ambient.InitColor(Para.OutAmbientColor);
    out_diffuse.InitColor(Para.OutDiffuseColor);
    out_specular.InitColor(Para.OutSpecColor);

    sVector30 l=sVector30(Para.LightPos-part->Pos);
    sVector30 v=sVector30(ctx->View.Camera.l-part->Pos);
    
    float dist;
    float diffuse;
    float specular;
    sVector30 vReflection;
    
    dist=l.LengthSq()+0.00001;
    
    l.Unit();  
    v.Unit();
    
    diffuse = n^n;                        //Dot    
    diffuse = diffuse * Para.DiffusePower / dist;
    diffuse = sClamp(diffuse,0.0f,1.0f);  //Saturate

    in_diffuse  = in_diffuse * diffuse;
    out_diffuse = out_diffuse * diffuse;

    sVector30 h = l + v;       
    h.Unit();

    specular = n^h;    
    specular = specular<0.0f ? 0.0f:specular;
    specular = sPow(specular, Para.SpecHardness);    
    specular = specular * Para.SpecPower / dist;
    specular = sClamp(specular,0.0f,1.0f);
    
    in_specular  = in_specular * specular;
    out_specular = out_specular * specular;

    sVector4 in_col  = in_ambient  + in_diffuse  + in_specular;
    sVector4 out_col = out_ambient + out_diffuse + out_specular;

    sF32 d;
    sF32 od;
    sBool isin=SDF->GetObj()->IsInBox(part->Pos);

    part->Drop=false;
  
    if (isin)
    {
      d=od=SDF->GetObj()->GetDistance(part->Pos)/bl;
      isin=d<=0.0f;            
      d=d<0.0f ? 0.0f:d;
      d=(d-Para.SDFFadeOffset)*Para.SDFFadeMult;
    }       
    else 
    {
      d=1.0f;
    }       
    
    if (od<0.0f && Para.CollMode)
    {
      part->Pos-=(od+Para.WrapOffset)*n;
    }

    if (Para.DropMode==1)
      part->Drop=!isin;
    else if(Para.DropMode==2)
      part->Drop=isin;
    else
      part->Drop=false;

    if (!part->Drop)
      KeepCount++;
        
    d = sClamp(d,0.0f,1.0f);        
    part->GeoDataCache.color = in_col*(1.0f-d) + out_col*(d);
    
    t = sMod(part->Time,1);
    sF32 tt = 1;
    sF32 s = part->SizeRand;
    if(t<fi)
    {
      tt = t*fii;
      s *= (ga+tt*gb+tt*tt*gc+tt*tt*tt*gd);
    }
    else if(1-t<fo)
    {
      tt = (1-t)*foi;
      s *= (ga+tt*gb+tt*tt*gc+tt*tt*tt*gd);
    }
    part->GeoDataCache.sx = sx*s;
    part->GeoDataCache.sy = sy*s;
    part->GeoDataCache.u1 = t;
    part->GeoDataCache.v1 = part->FadeRow;
    part->GeoDataCache.px = part->Pos.x;
    part->GeoDataCache.py = part->Pos.y;
    part->GeoDataCache.pz = part->Pos.z;

    sInt texanim = 0;
    if(Para.Mode & 0x200)
    {
      texanim = sInt(part->TexAnimRand*32) % UVRects.GetCount();
    }
    else
    {
      texanim=sInt(uvcountf*sMod(t*Para.TexAnimSpeed+part->TexAnimRand,1));
      if (texanim<0) texanim+=uvcounti;
    }
    part->GeoDataCache.uvrect = UVRects[texanim+part->Group*uvcounti];
    part->GeoDataCache.fade = fade;   
  }
  
  if (KeepCount)
  {
    PartVert1 *vp1;
    Geo->BeginLoadVB(KeepCount,sGD_FRAME,&vp1,1);
    for (sInt i=0;i<PartOrder.GetCount();i++)
    {
      if (!PartOrder[i]->Drop)
        *vp1++=PartOrder[i]->GeoDataCache;
    }
    Geo->EndLoadVB(-1,1);
  }
}

void RNFR063_Sprites::Render(Wz4RenderContext *ctx)
{
  if (PartOrder.IsEmpty() || KeepCount==0)
    return;

  sBool zwrite;
  switch(Para.Mode & 0x0030)
  {
    case 0x0020: case 0x0030: zwrite=sTRUE; break;
    default: zwrite=sFALSE; break;
  }

  if(ctx->IsCommonRendermode(zwrite))
  {
    sMatrix34CM *cm;
    sMatrix34 mat;
    sCBuffer<FR063StaticParticlePSPara> cb1;
    sU32 fade0col;
    switch(Para.Mode & 0x000f)
    {
      default    : fade0col=0x00000000; break;
      case 0x0001: fade0col=0x00000000; break;
      case 0x0002: fade0col=0x00ffffff; break;
      case 0x0003: fade0col=0x00808080; break;
      case 0x0004: fade0col=0x00000000; break;
      case 0x0005: fade0col=0x00ffffff; break;
    }
    cb1.Data->fade0col.InitColor(fade0col);
    cb1.Modify();

    sCBuffer<FR063StaticParticleVSPara> cb0;
    sFORALL(Matrices,cm)
    {
      sViewport view = ctx->View;
      view.UpdateModelMatrix(sMatrix34(*cm));
      mat = view.ModelView;
      mat.Invert3();

      sF32 tscale = 1;
      if(Para.Mode & 0x4000)
      {
        sF32 d = cm->x.x*cm->x.x + cm->y.x*cm->y.x + cm->z.x*cm->z.x;
        tscale = sSqrt(d);
      }


      cb0.Data->mvp = view.ModelScreen;
      cb0.Data->di = mat.i*tscale;
      cb0.Data->dj = mat.j*tscale;
      cb0.Modify();

      Mtrl->Set(&cb0,&cb1);
      sGeometryDrawInfo di;
      di.Flags = sGDI_Instances;
      di.InstanceCount = KeepCount;
      Geo->Draw(di);
    }
  } 
}

/***************************************************************************
RNFR063ClothGridSimRender
***************************************************************************/

RNFR063ClothGridSimRender::RNFR063ClothGridSimRender()
{    
  Geo = new sGeometry(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);
  Anim.Init(Wz4RenderType->Script);   
}

RNFR063ClothGridSimRender::~RNFR063ClothGridSimRender()
{
  DF->Release();
  Mtrl->Release();  
  delete Geo;
}

void RNFR063ClothGridSimRender::Init(Wz4ADF *dfobj)
{
	int i,j;
  Para=ParaBase;
  DF=dfobj;

  sVector30 x=Para.AX;
  sVector30 y=Para.AY;
  sVector31 pos=Para.Pos;
  int xc=Para.XC;
  int yc=Para.YC;
  float c1=Para.C1;
  float c2=Para.C2;
  sF32 m=Para.Mass;
  float ixc=1.0f/xc;
  float iyc=1.0f/yc;
  sInt n = Array.GetCount();
	sVector31 temp;
 	
  if (xc<2) xc=2;
  if (yc<2) yc=2;
  
	int maxParticle=xc*yc;
  Particles.AddMany(maxParticle);

  Moveable.Resize(Array.GetCount());

  x = x * ixc;
  y = y * iyc;

  sVector30 xn=x;
	sVector30 yn=y;

	int k=0;
	for(j=0;j<yc;j++)
  {
		for(i=0;i<xc;i++)
    {            
			temp=pos+xn*i+yn*j;
      Particles[k++].Init(pos+xn*i+yn*j, sVector30(0,0,0),m,0);            
		}
	}
  
  for (int i=0;i<n;i++)
  {
    int x=Array[i].Para.GridX%xc;
    int y=Array[i].Para.GridY%yc;
    int j=x+y*xc;
    sInt t=Array[i].Para.Type+1;
    Moveable[i]=&Particles[j];
    Particles[j].T=t;
    sPoolString name(Array[i].Para.SplineName);
    Array[i].Symbol = Wz4RenderType->Script->AddSymbol(name);
    Array[i].Script = 0;
  }

	int maxSprings= xc*(yc-1)+yc*(xc-1)+ xc*(yc-2)+yc*(xc-2)+(yc-1)*(xc-1)*2; // berechne anzahl noetiger federn
  Springs.AddMany(maxSprings);
	
	k=0;

	//Structure Springs
	for(j=0;j<yc;j++)
  {
		for(i=0;i<(xc-1);i++)
    {
			Springs[k++].Init(&Particles[i+j*xc],&Particles[(i+1)+j*xc] ,c1);
		}
	}
	
  for( j=0;j<(yc-1);j++)
  {
		for( i=0;i<xc;i++)
    {
			Springs[k++].Init(&Particles[i+j*xc],&Particles[i+(j+1)*xc] ,c1);
		}
	}

	//Bend Springs
	for(j=0;j<yc;j++)
  {
		for(i=0;i<(xc-2);i++)
    {
			Springs[k++].Init(&Particles[i+j*xc],&Particles[(i+2)+j*xc] ,c1);
		}
	}
	
  for(j=0;j<(yc-2);j++)
  {
		for(i=0;i<xc;i++)
    {
			Springs[k++].Init(&Particles[i+j*xc],&Particles[i+(j+2)*xc] ,c1);
		}
	}

	//Shear Springs
	for(j=0;j<(yc-1);j++)
  {
		for(i=0;i<(xc-1);i++)
    {
			Springs[k++].Init(&Particles[i+j*xc],&Particles[i+1+(j+1)*xc] ,c2);
			Springs[k++].Init(&Particles[i+1+j*xc],&Particles[i+(j+1)*xc] ,c2);
		}
	}  
  
  sInt steps = ParaBase.TimeOffset/SIMSTEP;  

  for(i=0;i<n;i++)
  {
    if (Moveable[i]->T==1 || Moveable[i]->T==3) //Fixing-Total, Release-AfterInit
    {
      Moveable[i]->M=0.0f;
      if (Moveable[i]->T==1)
      {
        sVector30 p=(sVector30)Moveable[i]->P;
        for (int j=0;j<maxParticle;j++)
        {
          sVector30 dv=((sVector30)Particles[j].P)-p;
          sF32 d=dv.Length()/(Array[i].Para.DragRadius+0.0001);
          d=sClamp(d,0.0f,1.0f);
          Particles[j].M*=d;        
        }
      }
    }      
  }

  if (ParaBase.DebugDoSim==0)
    steps=0;

  CalcParticles(steps,true,0.0f);

  sInt cnt=Particles.GetCount();

  for (sInt i=0;i<cnt;i++)
  {
    Particles[i].Freeze();
  }

  for(i=0;i<n;i++)
  {
    if (Moveable[i]->T==2)  //Fixing-AfterInit
      Moveable[i]->M=0.0f;
    else if (Moveable[i]->T==3) //Release-AfterInit
      Moveable[i]->M=m;
  }

  if (Para.DebugSubdivide)
    Vertices.Resize(2*yc*xc-xc-yc+1);
  else
    Vertices.Resize(yc*xc);


  LastTime=0.0f;
}




void RNFR063ClothGridSimRender::Simulate(Wz4RenderContext *ctx)
{ 
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
  
  sInt n = Array.GetCount();  
  for(sInt i=0;i<n;i++)
  {
    if (Array[i].Symbol && Array[i].Symbol->Value && Array[i].Symbol->Value->Spline)
    {
      Array[i].Script=Array[i].Symbol->Value->Spline;
    }
    else
    {
      Array[i].Script=0;
    }
  }  
  Time=ctx->GetTime();  
  
  Simulate(Time*ParaBase.TimeScale);
}

struct CalcNormalsStruct
{
  sInt             xc;
  sInt             yc;
  sVertexStandard *vp;
};

static void TaskCodeCalcNormals(sStsManager *m,sStsThread *th,sInt start,sInt count,void *data)
{
  sInt end=start+1;
  CalcNormalsStruct *d=(CalcNormalsStruct *)data;
  sInt xc=d->xc;
//  sInt yc=d->yc;
  sVertexStandard *vp=d->vp;

  //Calc Normals
  for(sInt y=start;y<end;y++)
  {
    for(sInt x=0;x<(xc-1);x++)
    {
      sInt id1=y*xc+x;
      sInt id2=y*xc+x+1;      
      sInt id3=(y+1)*xc+x+1;
      sInt id4=(y+1)*xc+x;
      
      sVector30 p1(vp[id1].px,vp[id1].py,vp[id1].pz);
      sVector30 p2(vp[id2].px,vp[id2].py,vp[id2].pz);
      sVector30 p3(vp[id3].px,vp[id3].py,vp[id3].pz);
      sVector30 p4(vp[id4].px,vp[id4].py,vp[id4].pz);

      p2=p2-p1;
      p3=p3-p1;
      p4=p4-p1;

      sVector30 n1;
      sVector30 n2;

      n1.Cross(p2,p3);
      n2.Cross(p3,p4);

      n1.Unit();
      n2.Unit();

      vp[id1].nx+=n1.x+n2.x;
      vp[id2].nx+=n1.x;
      vp[id3].nx+=n1.x+n2.x;
      vp[id4].nx+=n2.x;

      vp[id1].ny+=n1.y+n2.y;
      vp[id2].ny+=n1.y;
      vp[id3].ny+=n1.y+n2.y;
      vp[id4].ny+=n2.y;

      vp[id1].nz+=n1.z+n2.z;
      vp[id2].nz+=n1.z;
      vp[id3].nz+=n1.z+n2.z;
      vp[id4].nz+=n2.z;
    }
  }    
}


static void GetVertex(sVertexStandard *s, int x, int y, int xc, int yc, sVector30 &p, sF32 &u, sF32 &v)
{
  x=sMin(xc-1,sMax(x,0));
  y=sMin(yc-1,sMax(y,0));
  p=sVector30(s[y*(2*xc-1)+x].px,s[y*(2*xc-1)+x].py,s[y*(2*xc-1)+x].pz);
  u=s[y*(2*xc-1)+x].u0;
  v=s[y*(2*xc-1)+x].v0;
}

// called per frame
void RNFR063ClothGridSimRender::Prepare(Wz4RenderContext *ctx)
{
  int x,y;
  int i,j;
  int xc=Para.XC;
  int yc=Para.YC; 
  sVertexStandard *vp=0;
   
  vp=&Vertices[0];
  i=0;
  j=0;

  if (Para.DebugSubdivide)
  {
    for(y=0;y<yc;y++)
    {
      for(x=0;x<xc;x++)
      {
        float u=x;
        float v=y;
        u=u/(xc-1);
        v=v/(yc-1);
        vp[j].px=Particles[i].P.x;
        vp[j].py=Particles[i].P.y;
        vp[j].pz=Particles[i].P.z;
        vp[j].u0=u;
        vp[j].v0=v;
        vp[j].nx=0.0f;
        vp[j].ny=0.0f;
        vp[j].nz=0.0f;      
        i++;
        j++;
      }
      j+=xc-1;
    }
  }
  else
  {
   for(y=0;y<yc;y++)
    {
      for(x=0;x<xc;x++)
      {
        float u=x;
        float v=y;
        u=u/(xc-1);
        v=v/(yc-1);
        vp[j].px=Particles[i].P.x;
        vp[j].py=Particles[i].P.y;
        vp[j].pz=Particles[i].P.z;
        vp[j].u0=u;
        vp[j].v0=v;
        vp[j].nx=0.0f;
        vp[j].ny=0.0f;
        vp[j].nz=0.0f;      
        i++;
        j++;
      }      
    }
  }


  //interpolate
  if (Para.DebugSubdivide)
  {
    sF32 c1,c2,c3,c4;
    sHermite(0.5f,c1,c2,c3,c4);

    for(y=0;y<(yc-1);y++)
    {
      for(x=0;x<(xc-1);x++)
      { 
        sVector30 p1;
        sVector30 p2;
        sVector30 p3;
        sVector30 p4;
        sVector30 p5;
        sVector30 p6;
        sVector30 p7;
        sVector30 p8;
        sF32      u0,v0;
        sF32      u1,v1;
        sF32      u2,v2;
        sF32      u3,v3;
        sF32      u4,v4;
        sF32      u5,v5;
        sF32      u6,v6;
        sF32      u7,v7;
        sF32      u8,v8;

        GetVertex(vp,x-1,y-1,xc,yc,p1,u1,v1);
        GetVertex(vp,x-0,y-0,xc,yc,p2,u2,v2);
        GetVertex(vp,x+1,y+1,xc,yc,p3,u3,v3);
        GetVertex(vp,x+2,y+2,xc,yc,p4,u4,v4);        
        
        GetVertex(vp,x+2,y-1,xc,yc,p5,u5,v5);
        GetVertex(vp,x+1,y-0,xc,yc,p6,u6,v6);
        GetVertex(vp,x-0,y+1,xc,yc,p7,u7,v7);
        GetVertex(vp,x-1,y+2,xc,yc,p8,u8,v8);

        //p1=(p2+p3)*0.5f;
        //p5=(p6+p7)*0.5f;

        p1=p1*c1+p2*c2+p3*c3+p4*c4;
        p5=p5*c1+p6*c2+p7*c3+p8*c4;
        p1=(p1+p5)*0.5;

        u0=(u1*c1+u2*c2+u3*c3+u4*c4+u5*c1+u6*c2+u7*c3+u8*c4)*0.5f;
        v0=(v1*c1+v2*c2+v3*c3+v4*c4+v5*c1+v6*c2+v7*c3+v8*c4)*0.5f;

        i=y*(2*xc-1)+xc+x;
        vp[i].px=p1.x; vp[i].py=p1.y; vp[i].pz=p1.z;
        vp[i].u0=u0; vp[i].v0=v0; 
      }        
    }    
  } 
/*
  if (Para.DebugUseMulticore)
  {
    static CalcNormalsStruct cn;
    cn.vp=vp;
    cn.xc=xc;
    cn.yc=yc;

    sStsWorkload *wl = sSched->BeginWorkload();
    sStsTask *task = wl->NewTask(TaskCodeCalcNormals,&cn,yc-1,0);
    wl->AddTask(task);
    wl->Start();
    wl->Sync();
    wl->End();
  }
  else
*/
  

    //Calc Normals

  if (Para.DebugSubdivide)
  {
    for(sInt y=0;y<(yc-1);y++)
    {
      for(sInt x=0;x<(xc-1);x++)
      {
        sInt id1=y*(2*xc-1)+x;
        sInt id2=y*(2*xc-1)+x+1;
        sInt id4=(y+1)*(2*xc-1)+x;
        sInt id3=(y+1)*(2*xc-1)+x+1;
        sInt id5=id1+xc;

        sVector30 p1(vp[id1].px,vp[id1].py,vp[id1].pz);
        sVector30 p2(vp[id2].px,vp[id2].py,vp[id2].pz);
        sVector30 p3(vp[id3].px,vp[id3].py,vp[id3].pz);
        sVector30 p4(vp[id4].px,vp[id4].py,vp[id4].pz);
        sVector30 p5(vp[id5].px,vp[id5].py,vp[id5].pz);

        p1=p1-p5;
        p2=p2-p5;
        p3=p3-p5;
        p4=p4-p5;

        sVector30 n1;
        sVector30 n2;
        sVector30 n3;
        sVector30 n4;

        n1.Cross(p1,p2);
        n2.Cross(p2,p3);
        n3.Cross(p3,p4);
        n4.Cross(p4,p1);

        n1.Unit();
        n2.Unit();
        n3.Unit();
        n4.Unit();

        sVector30 vn1=sVector30(vp[id1].nx,vp[id1].ny,vp[id1].nz);
        sVector30 vn2=sVector30(vp[id2].nx,vp[id2].ny,vp[id2].nz);
        sVector30 vn3=sVector30(vp[id3].nx,vp[id3].ny,vp[id3].nz);
        sVector30 vn4=sVector30(vp[id4].nx,vp[id4].ny,vp[id4].nz);
        sVector30 vn5;//=sVector30(vp[id5].nx,vp[id5].ny,vp[id5].nz);

        vn1=vn1+n4+n1;
        vn2=vn2+n1+n2;
        vn3=vn3+n2+n3;
        vn4=vn4+n3+n4;
        vn5=n1+n2+n3+n4;
        
        vp[id1].nx=vn1.x; vp[id1].ny=vn1.y; vp[id1].nz=vn1.z;
        vp[id2].nx=vn2.x; vp[id2].ny=vn2.y; vp[id2].nz=vn2.z;
        vp[id3].nx=vn3.x; vp[id3].ny=vn3.y; vp[id3].nz=vn3.z;
        vp[id4].nx=vn4.x; vp[id4].ny=vn4.y; vp[id4].nz=vn4.z;
        vp[id5].nx=vn5.x; vp[id5].ny=vn5.y; vp[id5].nz=vn5.z;
      }
    }    
  }
  else
  {
    for(sInt y=0;y<(yc-1);y++)
    {
      for(sInt x=0;x<(xc-1);x++)
      {
        sInt id1;
        sInt id2;
        sInt id4;
        sInt id3;

        if (Para.DebugFlipEdges)
        {
          id4=y*xc+x;
          id1=y*xc+x+1;          
          id2=(y+1)*xc+x+1;
          id3=(y+1)*xc+x;
        }
        else
        {
          id1=y*xc+x;
          id2=y*xc+x+1;          
          id3=(y+1)*xc+x+1;
          id4=(y+1)*xc+x;
        }

        sVector30 p1(vp[id1].px,vp[id1].py,vp[id1].pz);
        sVector30 p2(vp[id2].px,vp[id2].py,vp[id2].pz);
        sVector30 p3(vp[id3].px,vp[id3].py,vp[id3].pz);
        sVector30 p4(vp[id4].px,vp[id4].py,vp[id4].pz);        
        
        p2=p2-p1;
        p3=p3-p1;
        p4=p4-p1;

        sVector30 n1;
        sVector30 n2;

        n1.Cross(p2,p3);
        n2.Cross(p3,p4);

        n1.Unit();
        n2.Unit();

        sVector30 vn1=sVector30(vp[id1].nx,vp[id1].ny,vp[id1].nz);
        sVector30 vn2=sVector30(vp[id2].nx,vp[id2].ny,vp[id2].nz);
        sVector30 vn3=sVector30(vp[id3].nx,vp[id3].ny,vp[id3].nz);
        sVector30 vn4=sVector30(vp[id4].nx,vp[id4].ny,vp[id4].nz);

        vn1=vn1+n1+n2;
        vn2=vn2+n1;
        vn3=vn3+n1+n2;
        vn4=vn4+n2;
        
        vp[id1].nx=vn1.x; vp[id1].ny=vn1.y; vp[id1].nz=vn1.z;
        vp[id2].nx=vn2.x; vp[id2].ny=vn2.y; vp[id2].nz=vn2.z;
        vp[id3].nx=vn3.x; vp[id3].ny=vn3.y; vp[id3].nz=vn3.z;
        vp[id4].nx=vn4.x; vp[id4].ny=vn4.y; vp[id4].nz=vn4.z;        
      }
    }  
  }
  
  if (Para.DebugSubdivide)
  {
    for (i=0;i<(2*yc*xc-xc-yc+1);i++)
    {
      sVector30 t(vp[i].nx,vp[i].ny,vp[i].nz);
      t.Unit();    
      vp[i].nx=t.x;
      vp[i].ny=t.y;
      vp[i].nz=t.z;
    }
  }
  else
  {
    for (i=0;i<yc*xc;i++)
    {
      sVector30 t(vp[i].nx,vp[i].ny,vp[i].nz);
      t.Unit();    
      vp[i].nx=t.x;
      vp[i].ny=t.y;
      vp[i].nz=t.z;
    }
  }

  //Fill GEO Object with right indices
  sU16 *ip=0;
  if (Para.DebugSubdivide)
  {
    sInt nbfaces = (xc-1)*(yc-1)*4;
    Geo->BeginLoadIB(nbfaces*3,sGD_FRAME,&ip);
    for(sInt y=0;y<(yc-1);y++)
    {
      for(sInt x=0;x<(xc-1);x++)
      {
        sInt id1=y*(2*xc-1)+x;
        sInt id2=y*(2*xc-1)+x+1;
        sInt id4=(y+1)*(2*xc-1)+x;
        sInt id3=(y+1)*(2*xc-1)+x+1;
        sInt id5=id1+xc;

        *ip++=id5;
        *ip++=id1;
        *ip++=id2;

        *ip++=id5;
        *ip++=id2;
        *ip++=id3;

        *ip++=id5;
        *ip++=id3;
        *ip++=id4;

        *ip++=id5;
        *ip++=id4;
        *ip++=id1;
      }
    }    
    Geo->EndLoadIB();    
  }
  else
  {
    sInt nbfaces = (xc-1)*(yc-1)*2;
    Geo->BeginLoadIB(nbfaces*3,sGD_FRAME,&ip);
    for(sInt y=0;y<(yc-1);y++)
    {
      for(sInt x=0;x<(xc-1);x++)
      {
        sInt id1=y*xc+x;
        sInt id2=y*xc+x+1;        
        sInt id3=(y+1)*xc+x+1;
        sInt id4=(y+1)*xc+x;
        
       // sVector30 p1(Vertices[id1].px,Vertices[id1].py,Vertices[id1].pz);
       // sVector30 p2(Vertices[id2].px,Vertices[id2].py,Vertices[id2].pz);
       // sVector30 p3(Vertices[id3].px,Vertices[id3].py,Vertices[id3].pz);
       // sVector30 p4(Vertices[id4].px,Vertices[id4].py,Vertices[id4].pz);

       // sVector30 n1(Vertices[id1].nx,Vertices[id1].ny,Vertices[id1].nz);
       // sVector30 n2(Vertices[id2].nx,Vertices[id2].ny,Vertices[id2].nz);
       // sVector30 n3(Vertices[id3].nx,Vertices[id3].ny,Vertices[id3].nz);
       // sVector30 n4(Vertices[id4].nx,Vertices[id4].ny,Vertices[id4].nz);

       // n2=(n1+n2+n3)*0.333f; //face normal 1
       // n3=(n1+n3+n4)*0.333f; //face normal 1
       // 
       // sVector30 t1=(p1+p3)*0.5f;
       // sVector30 t2=(p2+p4)*0.5f;
       // sVector30 t3=(p1+p2+p3+p4)*0.25f;

       //// sF32 h;
       // t1=t1-p2;
       // t2=t2-p2;
       // sF32 h=t1^t2;

       // if (h>0.0f)
        if (Para.DebugFlipEdges)
        {
          *ip++=id2;
          *ip++=id3;
          *ip++=id4;

          *ip++=id2;
          *ip++=id4;
          *ip++=id1;
        }
        else
        {          
          *ip++=id1;
          *ip++=id2;
          *ip++=id3;

          *ip++=id1;
          *ip++=id3;
          *ip++=id4;
        }
      }
    }    
    Geo->EndLoadIB();    
  }  

  Geo->BeginLoadVB(2*yc*xc-xc-yc+1,sGD_FRAME,&vp);
  sCopyMem(vp,&Vertices[0],sizeof(sVertexStandard)*Vertices.GetCount());
  Geo->EndLoadVB();
}


struct WireFormat
{
  sVector31 Pos;
  sVector30 Normal;
  sU32 Color;
};

// called for each rendering: once for ZOnly, once for MAIN, may be for shadows...
void RNFR063ClothGridSimRender::Render(Wz4RenderContext *ctx)
{   
  if (ctx->PaintInfo && (ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_WIRE)
  {
    int x,y;
    int i;
    int xc=Para.XC;
    int yc=Para.YC;     
    WireFormat *vp;   
    sGeometry *WireGeoLines;        // geometry to hold lines

    sVertexFormatHandle *fmt=Wz4MtrlType->GetDefaultFormat(ctx->RenderMode | sRF_MATRIX_ONE);  

    WireGeoLines = new sGeometry;
    WireGeoLines->Init(sGF_LINELIST|sGF_INDEX32,fmt);


    if (Para.DebugSubdivide)
    {
      WireGeoLines->BeginLoadVB(2*yc*xc-xc-yc+1,sGD_FRAME,&vp);
      for (i=0;i<(2*xc*yc-xc-yc+1);i++)
      {     
        vp[i].Pos=sVector31(Vertices[i].px,Vertices[i].py,Vertices[i].pz);
        vp[i].Normal=sVector30(0.0f,0.0f,0.0f);
        vp[i].Color=0xff0000ff;          
      }
      WireGeoLines->EndLoadVB();
    }
    else
    {
      WireGeoLines->BeginLoadVB(yc*xc,sGD_FRAME,&vp);
      i=0;
      for(y=0;y<yc;y++)
      {
        for(x=0;x<xc;x++)
        {
          vp[i].Pos=sVector31(Vertices[i].px,Vertices[i].py,Vertices[i].pz);
          vp[i].Normal=sVector30(0.0f,0.0f,0.0f);
          vp[i].Color=0xff0000ff;
          i++;
        }
      }
      WireGeoLines->EndLoadVB();
    }

    if (Para.DebugSubdivide)
    {
      sU32 *ip;
      sInt ic = (xc-1)*(yc-1)*6*2;
      
      WireGeoLines->BeginLoadIB(ic,sGD_STATIC,&ip);
      for(sInt y=0;y<(yc-1);y++)
      {
        for(sInt x=0;x<(xc-1);x++)
        {
          sInt id1=y*(2*xc-1)+x;
          sInt id2=y*(2*xc-1)+x+1;
          sInt id4=(y+1)*(2*xc-1)+x;
          sInt id3=(y+1)*(2*xc-1)+x+1;
          sInt id5=id1+xc;

          *ip++=(sU32)id1;
          *ip++=(sU32)id2;

          *ip++=(sU32)id4;
          *ip++=(sU32)id1;

          *ip++=(sU32)id1;
          *ip++=(sU32)id5;

          *ip++=(sU32)id2;
          *ip++=(sU32)id5;        

          *ip++=(sU32)id3;
          *ip++=(sU32)id5;        

          *ip++=(sU32)id4;
          *ip++=(sU32)id5;        
        }
      }  
      WireGeoLines->EndLoadIB();
    }
    else
    {
      sU32 *ip;
      sInt ic = (xc-1)*(yc-1)*5*2 + xc*yc*2;
      WireGeoLines->BeginLoadIB(ic,sGD_STATIC,&ip);
      for(sInt y=0;y<(yc-1);y++)
      {
        for(sInt x=0;x<(xc-1);x++)
        {
          sInt id1=y*xc+x;
          sInt id2=y*xc+x+1;
          sInt id4=(y+1)*xc+x;
          sInt id3=(y+1)*xc+x+1;

          *ip++=(sU32)id1;
          *ip++=(sU32)id2;

          *ip++=(sU32)id2;
          *ip++=(sU32)id3;

          *ip++=(sU32)id3;
          *ip++=(sU32)id4;

          *ip++=(sU32)id4;
          *ip++=(sU32)id1;

          *ip++=(sU32)id1;
          *ip++=(sU32)id3;        
        }
      }  
      WireGeoLines->EndLoadIB();
    }

    

    sMatrix34CM *mat;
    sFORALL(Matrices,mat)
    {
      sMatrix34 mt(*mat);
      sCBuffer<sSimpleMaterialPara> cb0;
      cb0.Data->mvp =mt*ctx->View.ModelScreen;//*(*mat);
      cb0.Modify(); 
      ctx->PaintInfo->DrawMtrl->Set(&cb0,0);

      WireGeoLines->Draw();      
    }

    delete WireGeoLines;
  }


  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_ZONLY ||
     (ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_ZNORMAL ||
     (ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_DIST)
  {
    // here the material can decline to render itself
    if(Mtrl->SkipPhase(ctx->RenderMode,Para.LightEnv)) return;

    // the effect may be placed multple times in the render graph.
    // this loop will get all the matrices:
    sMatrix34CM *mat;
    sFORALL(Matrices,mat)
    {
      // here i add our own matrix
      sMatrix34CM mat1 = sMatrix34CM(sMatrix34(*mat));

      // render it once
      Mtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv,&mat1,0,0,0);
      Geo->Draw();
    }
  }

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    // here the material can decline to render itself
    if(Mtrl->SkipPhase(ctx->RenderMode,Para.LightEnv)) return;

    // the effect may be placed multple times in the render graph.
    // this loop will get all the matrices:
    sMatrix34CM *mat;
    sFORALL(Matrices,mat)
    {
      // here i add our own matrix
      sMatrix34CM mat1 = sMatrix34CM(sMatrix34(*mat));

      // render it once
      Mtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv,&mat1,0,0,0);
      Geo->Draw();
    }
  }
}


void RNFR063ClothGridSimRender::Simulate(sF32 time)
{
  sInt steps=0;      
 
  sF32 d=time-LastTime;
  
  if (d<0.0f && d>-0.02f)
    d=0.0f;

  if (time<=0.0f)
  {
    Reset();
    LastTime=0;
    return;  
  }
  if (d<0.0f)
  {
    Reset();        
    steps=time/SIMSTEP;
    LastTime=steps*SIMSTEP;
    time=0.0f;
  }
  else
  {
    steps=d/SIMSTEP;    
    time=LastTime;
    LastTime+=steps*SIMSTEP;    
  }    
  
  if (steps>0)
    CalcParticles(steps,false,time/Para.TimeScale);
}


static void TaskCodeSimulate(sStsManager *m,sStsThread *th,sInt start,sInt count,void *data)
{
  sArray <SCSpring> *springs = (sArray <SCSpring> *)data;
  sInt ns=springs->GetCount();
  sInt n=(ns+63)/64;
  sInt s=n*start;
  sInt e=s+n;
  if (e>ns)
    e=ns;
 
  for(sInt i=s;i<e;i++)
  {
    (*springs)[i].CalcForces();
  }
}

void RNFR063ClothGridSimRender::CalcParticles(sInt steps, sBool init, sF32 time)
{
  sInt j;
  
  if (Para.DebugDoSim==0)
    return;

  for (j=0;j<steps;j++)
  {
    sInt i;  
    sInt np=Particles.GetCount();
    sInt ns=Springs.GetCount();
    sInt nm=Moveable.GetCount();
    sVector30 G=Para.Gravity;
    sVector30 M=Para.Force;
    sF32 D=Para.Damping;   
    sInt count = Array.GetCount();  

    for(sInt i=0;i<count;i++)
    {
      sVector31 p;
      if (Array[i].Script)
        Array[i].Script->Eval(time,(sF32*)&p,3);
      Array[i].Pos=p;
    }
    time+=SIMSTEP/Para.TimeScale;

    for(i=0;i<np;i++)
    {    
      Particles[i].ClearForce();
      Particles[i].AddForce(Para.Force);
      Particles[i].AddForce(G);
      Particles[i].AddForce(-D*Particles[i].V);		
    }

    if (!init)
    {
      for(i=0;i<nm;i++)
      {		
        if (Moveable[i]->T==2||Moveable[i]->T==1)  //Fixing-AfterInit
          Moveable[i]->P = Moveable[i]->ROP+sVector30(Array[i].Pos);
      }
    }
    if (Para.DebugUseMulticore)
    {
      sStsWorkload *wl = sSched->BeginWorkload();
      sStsTask *task = wl->NewTask(TaskCodeSimulate,&Springs,64,0);
      wl->AddTask(task);
      wl->Start();
      wl->Sync();
      wl->End();
    }
    else
    {
      for(i=0;i<ns;i++)
      {
        Springs[i].CalcForces();
      }
    }

    for(i=0;i<ns;i++)
    {
      Springs[i].S->AddForce(Springs[i].F);
      Springs[i].E->SubForce(Springs[i].F);
    }

    for(i=0;i<np;i++)
    {
      tSDF *df=DF->GetObj();

      sVector30  v=Particles[i].V+Particles[i].F*Particles[i].M*SIMSTEP;
      sVector31 sp=Particles[i].P;
      sVector31 ep=sp+v;      

      sF32 d;
      d=df->GetDistance(ep)-Para.CollisionGuard;        
      if (d<0.0f)
      {
        sVector30 n;
        df->GetNormal(ep,n);         
        if (n.LengthSq()<0.01f)
        {
          n=sp-ep;
        }
        ep=ep+d*n;
        v=-n*Para.SlideFactor;
      }              
      Particles[i].P=ep;
      Particles[i].V=v;      
    }
  }
}

void RNFR063ClothGridSimRender::Reset()
{  
  sInt cnt=Particles.GetCount();
  for (sInt i=0;i<cnt;i++)
  {
    Particles[i].Reset();
  }
  LastTime=0;
}

void RNFR063ClothGridSimRender::Handles(wPaintInfo &pi, Wz4RenderParaFR063_ClothGridSimRender *para, wOp *op)
{  
  Wz4Render *rn = (Wz4Render *)op->RefObj;
  RNFR063ClothGridSimRender *o=(RNFR063ClothGridSimRender *)rn->RootNode;

  for (sInt i=0;i<o->Moveable.GetCount();i++)
  {
    sVector30 tv=(sVector30)(o->Moveable[i]->ROP);
    ScriptSpline *spline = o->Array[i].Script;
    if(spline && spline->Count==3)
    {      
      sVector31 v0,v1;
      spline->Eval(0,&v0.x,3);
      v0+=tv;
      for(sInt i=1;i<256;i++)
      {
        spline->Eval(spline->MaxTime*i/256,&v1.x,3);
        v1+=tv;
        pi.Line3D(v0,v1,(i&1)?0xffff0000:0xff00ff00);
        v0 = v1;
      }
    }    
    sVector31 p0=o->Moveable[i]->P;
    sVector31 p1=p0-sVector30(0.1f,0.0f,0.0f);
    sVector31 p2=p0+sVector30(0.1f,0.0f,0.0f);
    pi.Line3D(p1,p2,0xff00ff00);
    p1=p0-sVector30(0.0f,0.1f,0.0f);
    p2=p0+sVector30(0.0f,0.1f,0.0f);
    pi.Line3D(p1,p2,0xff00ff00);
    p1=p0-sVector30(0.0f,0.0f,0.1f);
    p2=p0+sVector30(0.0f,0.0f,0.1f);
    pi.Line3D(p1,p2,0xff00ff00);    
  }
}

/****************************************************************************/

RNFR063_MassBallColl::RNFR063_MassBallColl()
{
  static const sU32 desc[] =   
  {    
    sVF_STREAM0|sVF_UV0|sVF_F3,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_POSITION|sVF_F4,    
    0,
  };  

  Format = sCreateVertexFormat(desc);  
  Geo = new sGeometry;
  Geo->Init(sGF_TRILIST|sGF_INDEX16,Format);
  Mtrl = 0;
  SDF  = 0;
  Anim.Init(Wz4RenderType->Script);
}

RNFR063_MassBallColl::~RNFR063_MassBallColl()
{
  SDF->Release();
  delete Mtrl;
  delete Geo;
}

void RNFR063_MassBallColl::Init()
{
  sInt i;
//Init Material
  Mtrl = new FR063BallShader();
  Mtrl->Flags     = sMTRL_CULLOFF|sMTRL_ZON;  
  Mtrl->TFlags[0] = sMTF_LEVEL0|sMTF_CLAMP;  
  Mtrl->Prepare(Format);

//Init Ball Array  
  sRandomMT rnd;
  rnd.Seed((sU32)Para.Seed);  
  Balls.Resize(Para.NbOfBalls);
  
  for (i=0;i<Para.NbOfBalls;i++)
  {
    sVector31 pos=Para.PosStart + sVector30(rnd.Float(Para.PosRand.x),rnd.Float(Para.PosRand.y),rnd.Float(Para.PosRand.z));
    sVector30 velo(0,0,0);
    sF32      radius=Para.RadiusStart+rnd.Float(Para.RadiusRand);
    sF32      mass;
    if (Para.RadiusRand==0.0f)
      mass=0;
    else
      mass=(radius-Para.RadiusStart)/Para.RadiusRand;    
    
    mass = mass*Para.MassScale+Para.MassStart;

    Balls[i].Init(pos,velo,radius,mass);        
  }

  LastTime=0.0f;
}

void RNFR063_MassBallColl::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);  

  sF32 time=ctx->GetTime()*Para.TimeScale;
  sInt i;
  sInt steps=0;      
  sF32 d=time-LastTime;

  //Hack to fix music player bug
  if (d<0.0f && d>-0.04f)
  {
    d=0.0f;
    time=LastTime;
  }

  if (time<=0.0f)
  {
    for (i=0;i<Para.NbOfBalls;i++) Balls[i].Reset();
    LastTime=0;
    return;  
  }
  if (d<0.0f)
  {
    for (i=0;i<Para.NbOfBalls;i++) Balls[i].Reset();
    steps=time/Para.SimStep;
    LastTime=steps*Para.SimStep;    
  }
  else
  {
    steps=d/Para.SimStep;
    LastTime+=steps*Para.SimStep;
  }        
  Simulate(steps);
}



sInt RNFR063_MassBallColl::CollCheck(const sVector31 &sp, const sVector31 &ep, const sInt ball, sVector31 &cp, sF32 &t)
{
  tSDF *df=SDF->GetObj();
  sVector30 l=ep-sp;
  sF32 ll=l.Length()+0.0001f;//Calculate length of line + radius
  sF32 d=df->GetDistance(sp);  
  
  if (df->GetDistance(ep)<0.0f && d<0.0f) 
  {
    sVector30 n;    
    df->GetNormal(ep,n);
    cp=ep+(d*Para.SlidePower)*n;
    //cp=sp;
    t=0.0f;
    return COLLTYPE_STUCK;
  }  
  if (d<ll) //Take a closer
  {
    sInt i=0;
    t=(d/ll);//*0.5;           //0.5 sicherheits faktor
    sF32 dt=(1.0f-t)/16.0f; //
    sF32 z=t;
    do
    {
      t=z;      
      z+=dt;
      cp=sp+l*z;
      d=df->GetDistance(cp);      
      i++;      
    }while(d>0.0f && i<16);    
    cp=sp+l*t;
    return COLLTYPE_HIT;
  } 

  t=1.0f;
  cp=ep;
  return COLLTYPE_NONE;
}


void RNFR063_MassBallColl::Simulate(sInt steps)
{
  tSDF *df=SDF->GetObj();
  sInt i;
  sInt j;
  if (steps<=0)
    return;

  for (j=0;j<steps;j++)
  {
    for (i=0;i<Para.NbOfBalls;i++) 
    {       
      sVector31 sp=Balls[i].CP;
      sVector30 v=Balls[i].CV+Para.Gravity*Para.SimStep*Balls[i].M;
      sVector31 ep=sp+(v*Para.SimStep)+Balls[i].CR;
      sVector30 rv=sVector30(0,0,0);

      if (df->IsInBox(sp))
      {
        sF32 t;
        sVector31 cp;

        switch (CollCheck(sp,ep,i, cp, t))
        {
          case COLLTYPE_NONE :
          {
          }
          break;
          case COLLTYPE_STUCK :
          {
            ep=cp;
            v.Init(0,0,0);
          }
          break;
          case COLLTYPE_HIT :
          {
            sVector30 iv=-v;
            sVector30 norm;
            df->GetNormal(cp,norm);            
            if (norm.LengthSq()<0.00001) { norm=iv; norm.Unit(); }
            
            sVector30 ref=(2*(norm^iv)*norm-iv);
            v=ref*Para.ReflPower;
            rv=-norm*(1.0f-t)*Para.ReactPower;
            ep=cp;
          }
          break;
        }
      }      
      Balls[i].CV=v;
      Balls[i].CP=ep;      
      Balls[i].CR=rv;
    }
  }
}


void RNFR063_MassBallColl::Prepare(Wz4RenderContext *ctx)
{
  sInt i;
  VertStream0 *vp0;
  VertStream1 *vp1;
  sU16 *ip0;

  // prepare instance data
  Geo->BeginLoadVB(4,sGD_FRAME,&vp0,0);
  vp0[0].Init(1,-1,sPI2F*0.25f*0);
  vp0[1].Init(1,1,sPI2F*0.25f*1);
  vp0[2].Init(-1,1,sPI2F*0.25f*2);
  vp0[3].Init(-1,-1,sPI2F*0.25f*3);
  Geo->EndLoadVB(-1,0);
  Geo->BeginLoadIB(6,sGD_FRAME,&ip0);    // yes we need an index buffer, or D3DDebug is unhappy
  sQuad(ip0,0,0,1,2,3);
  Geo->EndLoadIB();

  Geo->BeginLoadVB(Para.NbOfBalls,sGD_FRAME,&vp1,1);
  for (i=0;i<Para.NbOfBalls;i++)
  {
    vp1[i].Init(Balls[i].CP, Balls[i].R);
  }  
  Geo->EndLoadVB(-1,1);
}

void RNFR063_MassBallColl::Render(Wz4RenderContext *ctx)
{
  sBool zwrite=sTRUE;

  if(ctx->IsCommonRendermode(zwrite))
  {
    sMatrix34CM *cm;
    sMatrix34 mat;
    sCBuffer<FR063BallPSPara> cb1;
    sCBuffer<FR063BallVSPara> cb0;
    
    cb1.Data->ambcol.InitColor(Para.AmbientColor);
    cb1.Data->ambcol*=Para.AmbientPower;
    cb1.Data->diffcol.InitColor(Para.DiffuseColor);
    cb1.Data->diffcol*=Para.DiffusePower;    
    cb1.Data->speccol.InitColor(Para.SpecColor);
    cb1.Data->speccol*=Para.SpecPower;
    cb1.Data->specprm.Init(Para.SpecHardness,0,0,0);

    sFORALL(Matrices,cm)
    {
      sViewport view = ctx->View;
      view.UpdateModelMatrix(sMatrix34(*cm));

      mat = view.View;      

      cb1.Data->proj = view.Proj;
      cb1.Data->lightpos=Para.LightPos*mat;
      cb1.Data->campos=ctx->View.Camera.l*mat;
      cb1.Modify();

      mat = view.ModelView;
      mat.Invert3();
      cb0.Data->mvp = view.ModelScreen;
      cb0.Data->mv  = view.ModelView;
      cb0.Data->di = mat.i;
      cb0.Data->dj = mat.j;
      cb0.Modify();
      
      Mtrl->Set(&cb0,&cb1);
      sGeometryDrawInfo di;
      di.Flags |= sGDI_Instances;
      di.InstanceCount = Para.NbOfBalls;
      Geo->Draw(di);
    }
  } 
}


/****************************************************************************/

SphCollSDF::SphCollSDF()
{
  SDF = 0;
}

SphCollSDF::~SphCollSDF()
{
  SDF->Release();
}

void SphCollSDF::Init()
{
}

void SphCollSDF::CollPart(RPSPH *s)
{
  tSDF *df=SDF->GetObj();
  sInt max = s->Parts[0]->GetCount();
  RPSPH::Particle *p = s->Parts[0]->GetData(); 

  if(Para.Flags & 1)
  {
    for(sInt i=0;i<max;i++)
    {
      sF32 d=df->GetDistance(p[i].NewPos);
      if (Para.Flags&16) d=-d;
      if (d<0.0f) p[i].Color = 0;      
    }
  }
  else
  {
    for(sInt i=0;i<max;i++)
    {
      if (df->IsInBox(p[i].NewPos))
      {
        sVector31 sp=p[i].OldPos;
        sVector31 ep=p[i].NewPos;
        sF32 d;
        d=df->GetDistance(ep);
        if (Para.Flags&16) d=-d;
        if (d<0.0f)
        {
          sVector30 n;
          df->GetNormal(ep,n);
          if (Para.Flags&16) n=-n;
          if (n.LengthSq()<0.001f)
          {
            n=p[i].NewPos-p[i].OldPos;
          }
          ep=ep+d*n;
        }
        p[i].NewPos=ep;
      }
    }
  }
}

/****************************************************************************/

void SphMorphSDF::Init()
{
}

void SphMorphSDF::CollPart(RPSPH *s)
{
  tSDF *df=SDF->GetObj();  
  sInt max = s->Parts[0]->GetCount();
  RPSPH::Particle *p = s->Parts[0]->GetData();

  if (s->SimStep<=Para.Time)
    return;

  for(sInt i=0;i<max;i++)
  {    
    sVector30 n;
    sF32 d=df->GetDistance(p[i].NewPos);
    df->GetNormal(p[i].NewPos,n);

    d=sAbs(d);

    p[i].NewPos-=n*(Para.Factor);
  }
}

/****************************************************************************/

void SphCollPDF::Init()
{
}

void SphCollPDF::CollPart(RPSPH *s)
{
  Wz4PDFObj *df=PDF->GetObj();
  sInt max = s->Parts[0]->GetCount();
  RPSPH::Particle *p = s->Parts[0]->GetData(); 

  if(Para.Flags & 1)
  {
    for(sInt i=0;i<max;i++)
    {
      sF32 d=df->GetDistance(p[i].NewPos);
      if (Para.Flags&16) d=-d;
      if (d<0.0f) p[i].Color = 0;      
    }
  }
  else
  {
    for(sInt i=0;i<max;i++)
    {
      sVector31 sp=p[i].OldPos;
      sVector31 ep=p[i].NewPos;
      sF32 d;
      d=df->GetDistance(ep);
      if (Para.Flags&16) d=-d;
      if (d<0.0f)
      {
        sVector30 n;
        df->GetNormal(ep,n);
        if (Para.Flags&16) n=-n;
        if (n.LengthSq()<0.001f)
        {
          n=p[i].NewPos-p[i].OldPos;
        }
        ep=ep+d*n;
      }
      p[i].NewPos=ep;
    }
  }  
}

/****************************************************************************/

FR063_SpritesExt::FR063_SpritesExt()
{
  Format = 0;
  Geo = 0;
  Mtrl = 0;
  TextureDiff = 0;
  TextureFade = 0;
  Anim.Init(Wz4RenderType->Script);
}

FR063_SpritesExt::~FR063_SpritesExt()
{
  delete Geo;
  delete Mtrl;
  TextureDiff->Release();
  TextureFade->Release();  
}

void FR063_SpritesExt::Init()
{
  Para = ParaBase;

  static const sU32 desc[] = 
  {
    sVF_STREAM0|sVF_UV1|sVF_F3,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_POSITION|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV0|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV2|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV3|sVF_F1,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV4|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV5|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_COLOR0|sVF_C4,
    0,
  };

  Format = sCreateVertexFormat(desc);
  Geo = new sGeometry;
  Geo->Init(sGF_TRILIST|sGF_INDEX16,Format);

  Mtrl = new StaticParticleShaderExt;
  Mtrl->Flags = sMTRL_CULLOFF;
  Mtrl->Texture[0] = TextureDiff->Texture;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;

  switch(Para.Mode & 0x000f)
  {
    default    : Mtrl->BlendColor = sMB_ADD; break;
    case 0x0001: Mtrl->BlendColor = sMB_PMALPHA; break;
    case 0x0002: Mtrl->BlendColor = sMB_MUL; break;
    case 0x0003: Mtrl->BlendColor = sMB_MUL2; break;
    case 0x0004: Mtrl->BlendColor = sMB_ADDSMOOTH; break;
    case 0x0005: Mtrl->BlendColor = sMB_ALPHA; break;
  }
  switch(Para.Mode & 0x0030)
  {
    default    : Mtrl->Flags |= sMTRL_ZOFF; break;
    case 0x0010: Mtrl->Flags |= sMTRL_ZREAD; break;
    case 0x0020: Mtrl->Flags |= sMTRL_ZWRITE; break;
    case 0x0030: Mtrl->Flags |= sMTRL_ZON; break;
  }

  Mtrl->Prepare(Format);

  Particle *p;
  sRandom rnd;

  sF32 maxrowrandom=1.0f;
  sF32 rowoffs=0.0f;
  if (TextureFade) 
  {
    rowoffs=1.0f/TextureFade->Texture->SizeY;
    maxrowrandom-=2*rowoffs;
    Mtrl->Texture[1] = TextureFade->Texture;
    Mtrl->TFlags[1] = sMTF_LEVEL2|sMTF_CLAMP;
  }

  sFORALL(Particles,p)
  {        
    p->FadeRow = rnd.Float(maxrowrandom)+rowoffs;
    p->TexAnimRand = rnd.Float(1)*Para.TexAnimRand;
    p->Dist = 0;
  }  
}


void FR063_SpritesExt::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);  
  SimulateChilds(ctx);
}

void FR063_SpritesExt::Prepare(Wz4RenderContext *ctx)
{
  sRandom rnd;
  sVector31 p;
  sF32 dist;
  PartVert0 *vp0;
  PartVert1 *vp1;
  Particle *part;
  sVector4 plane;
  sU16 *ip0;

  sInt nbp = Particles.GetCount();

  // texture might have changed, recalculate atlas uv
  if(TextureDiff->Atlas.Entries.IsEmpty())
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
    UVRects.Resize(TextureDiff->Atlas.Entries.GetCount());
    BitmapAtlasEntry *e;

    sFORALL(TextureDiff->Atlas.Entries,e)
    {
      PartUVRect &r=UVRects[_i];
      r.u = e->UVs.x0;
      r.v = e->UVs.y0;
      r.du = e->UVs.x1-e->UVs.x0;
      r.dv = e->UVs.y1-e->UVs.y0;
    }
  }

  // calculate particle positions
  sViewport view = ctx->View;
  if (Para.Mode&0x10000)
    view.UpdateModelMatrix(sMatrix34(Matrices[0]));
  sMatrix34 mat=view.Camera;  
  plane.InitPlane(mat.l,mat.k);

  dist = ((sVector30)(mat.l)).Length();
  
  if (dist>Para.FarFadeDistance) 
  {
    DoRender=false;
    return;
  }
  DoRender=true;

  sF32 distfade=1.0f/Para.NearFadeDistance;
  sF32 globalfade=1.0f;
  if (dist>=(Para.FarFadeDistance-Para.FarFadeRange))
    globalfade=sClamp((Para.FarFadeDistance-dist)/Para.FarFadeRange,0.0f,1.0f);

  mat=view.ModelView;  
  mat.Invert3();    

  sMatrix34 matv=(sMatrix34)Matrices[0];
  
  sF32 tscale = 1;
  if(Para.Mode & 0x4000)
  {
    sF32 d = Matrices[0].x.x*Matrices[0].x.x + Matrices[0].y.x*Matrices[0].y.x + Matrices[0].z.x*Matrices[0].z.x;
    tscale = sSqrt(d);
  }

  for(sInt i=0;i<nbp;i++)
  {
    sF32 a=1.0f;
    part = &Particles[i];    
        
    if (part->Para.Mode&0x10000)
    { 
      sVector30 d(-part->Para.AlignDir);
      sVector30 o=(sVector30)(part->Para.Pos*matv-view.Camera.l);        
      o.Unit();
      d.Unit();

//      sF32 fs=part->Para.FixAngle;
      sF32 fe=part->Para.FixAngle+part->Para.Range;
      sF32 fr=part->Para.Range;

      if (fe>1.0f)
      {        
        fr-=fe-1.0f;
        fe=1.0f;
      }            
      
      sF32 ir=1.0f/(fr+0.000001f);

      a= ((o^d)+1.0f)*0.5f;
      if (a>fe) a=fe-(a-fe);
      a=a-part->Para.FixAngle;
      a*=ir;      
      a=sClamp(a,0.0f,1.0f); 
    }
       
    sMatrix34 mata;            
    mata.Look(sVector30(part->Para.AlignDir));
    
    if (part->Para.Mode&1)
      mata=mat;      

    part->Di = mata.i*tscale;
    part->Dj = mata.j*tscale;

    p=part->Para.Pos;
    dist=-(plane^(p*view.Model));
    if (dist<=-view.ClipNear)
    {
      part->Time = 0;
      part->Pos = p;
      part->Dist = dist;
      sF32 df=sClamp(-(part->Dist+view.ClipNear)*distfade,0.0f,1.0f)*globalfade;
      part->DistFade = sSmooth(df*df)*a;      
    }
    else
    {
      part->Time = 0;
      part->Pos = p;
      part->Dist = dist;
      part->DistFade = 0;
    }
  }

  // prepare instance data
  Geo->BeginLoadVB(4,sGD_FRAME,&vp0,0);
  vp0[0].Init(1,0,sPI2F*0.25f*0);
  vp0[1].Init(1,1,sPI2F*0.25f*1);
  vp0[2].Init(0,1,sPI2F*0.25f*2);
  vp0[3].Init(0,0,sPI2F*0.25f*3);
  Geo->EndLoadVB(-1,0);
  Geo->BeginLoadIB(6,sGD_FRAME,&ip0);
  sQuad(ip0,0,0,1,2,3);
  Geo->EndLoadIB();

  // prepare particle data

  sF32 sx = 1.0f*sSqrt(sPow(2,Para.Aspect));
  sF32 sy = 1.0f/sSqrt(sPow(2,Para.Aspect));

  const sInt uvcounti = UVRects.GetCount();
  const sF32 uvcountf = uvcounti;

  Geo->BeginLoadVB(nbp,sGD_FRAME,&vp1,1);
  sVERIFY(nbp == Particles.GetCount());

  sFORALL(Particles,part)
  {
    vp1->px = part->Pos.x;
    vp1->py = part->Pos.y;
    vp1->pz = part->Pos.z;

    vp1->rot = 0.0f;
  
    sF32 t = sMod(part->Time,1);
//    sF32 tt = 1;
    sF32 s = part->Para.Size; 

    vp1->sx = sx*s;
    vp1->sy = sy*s;
    vp1->u1 = t;
    vp1->v1 = part->FadeRow;

    sInt texanim = 0;
    if(Para.Mode & 0x200)
    {
      texanim = sInt(part->TexAnimRand*32) % UVRects.GetCount();
    }
    else
    {
      texanim=sInt(uvcountf*sMod(t*Para.TexAnimSpeed+part->TexAnimRand,1));
      if (texanim<0) texanim+=uvcounti;
    }
    vp1->uvrect = UVRects[0];///Fix It UVRects[texanim+uvcounti];

    vp1->fade = part->DistFade;
    vp1->Color = part->Para.Color;

    vp1->dix = part->Di.x; vp1->diy = part->Di.y; vp1->diz = part->Di.z; vp1->diw=1.0f;
    vp1->djx = part->Dj.x; vp1->djy = part->Dj.y; vp1->djz = part->Dj.z; vp1->djw=1.0f;
    
    vp1++;
  }
  Geo->EndLoadVB(-1,1);
}

void FR063_SpritesExt::Render(Wz4RenderContext *ctx)
{
  if (Particles.IsEmpty() || !DoRender)
    return;

  sBool zwrite;
  switch(Para.Mode & 0x0030)
  {
    case 0x0020: case 0x0030: zwrite=sTRUE; break;
    default: zwrite=sFALSE; break;
  }

  if(ctx->IsCommonRendermode(zwrite))
  {
    sMatrix34CM *cm;
    sMatrix34 mat;
    sCBuffer<StaticParticleExtPSPara> cb1;
    cb1.Data->col.InitColor(0xffffffff);

    sU32 fade0col;
    switch(Para.Mode & 0x000f)
    {
      default    : fade0col=0x00000000; break;
      case 0x0001: fade0col=0x00000000; break;
      case 0x0002: fade0col=0x00ffffff; break;
      case 0x0003: fade0col=0x00808080; break;
      case 0x0004: fade0col=0x00000000; break;
      case 0x0005: fade0col=0x00ffffff; break; //Fix
    }
    cb1.Data->fade0col.InitColor(fade0col);
    cb1.Modify();

    sCBuffer<StaticParticleExtVSPara> cb0;
    sFORALL(Matrices,cm)
    {
      sViewport view = ctx->View;
      view.UpdateModelMatrix(sMatrix34(*cm));
      cb0.Data->mvp = view.ModelScreen;
      cb0.Modify();
      Mtrl->Set(&cb0,&cb1);
      sGeometryDrawInfo di;
      di.Flags |= sGDI_Instances;
      di.InstanceCount = Particles.GetCount();
      Geo->Draw(di);
    }
  } 
}

void FR063_SpritesExt::Handles(wPaintInfo &pi, Wz4RenderParaFR063_SpritesExt *para, wOp *op)
{  
  Wz4Render *rn = (Wz4Render *)op->RefObj;
  FR063_SpritesExt *o=(FR063_SpritesExt *)rn->RootNode;

  for (sInt i=0;i<o->Particles.GetCount();i++)
  {
    pi.Handle3D(op, 1, o->Particles[i].Para.Pos, wHM_PLANE);  
    pi.Line3D(o->Particles[i].Para.Pos, o->Particles[i].Para.Pos+o->Particles[i].Para.AlignDir);
  }

}

/****************************************************************************/

RNFR063AermelKanal::RNFR063AermelKanal()
{
  Mtrl=0;  
  Mesh=0;
  DF=0;    
  InitDone=sFALSE;  
  PathSymbol=0;
  MovementSymbol=0;
  PathSpline=0;
  MovementSpline=0;
  Anim.Init(Wz4RenderType->Script);
  FixGeo = new sGeometry(sGF_TRILIST|sGF_INDEX32,sVertexFormatStandard);
  DynGeo = new sGeometry(sGF_TRILIST|sGF_INDEX32,sVertexFormatStandard);  
}

RNFR063AermelKanal::~RNFR063AermelKanal()
{    
  delete DynGeo;
  delete FixGeo;
  if (Mtrl) Mtrl->Release();  
  if (Mesh) Mesh->Release();
  if (DF) DF->Release();
}

void RNFR063AermelKanal::Init()
{
  Para = ParaBase;
  PathSymbol = Wz4RenderType->Script->AddSymbol(PathName);
  MovementSymbol = Wz4RenderType->Script->AddSymbol(MovementName);

  ParaBase.Segments=sMax(ParaBase.Segments, 1);
//  ParaBase.SubSegments=sMax(ParaBase.SubSegments, 1);
  ParaBase.Slices=sMax(ParaBase.Slices, 3);  
}


sF32 RNFR063AermelKanal::CalcTime(float time)
{
  time=sClamp(time,0.0f,1.0f);
  return time*PathSpline->Length();
}

void RNFR063AermelKanal::GetMovementData(float time, float &tunneltime, float &objtime, float &objrot)
{ 
  sInt dim;
  sF32 buf[3];
  tunneltime=objtime=objrot=0.0f;
  if (MovementSpline==0) return;
  dim=MovementSpline->Count;    
  if (dim==0) return;
  dim=sMin(dim,3);
  time=sClamp(time,0.0f,1.0f);
  time*=MovementSpline->Length();
  MovementSpline->Eval(time,buf,dim);
  tunneltime=sClamp(buf[0],0.0f,1.0f);
  
  if (dim>1) objtime=buf[1];
  if (dim>2) objrot=buf[2];      
}

void RNFR063AermelKanal::GetPos(float time, sVector31 &pos)
{    
  if (PathSpline && PathSpline->Count>=3)
  {
    time=CalcTime(time);
    PathSpline->Eval(time,&pos.x,3);
    return;
  }
  pos.Init(0,0,0);
  return;
}

int RNFR063AermelKanal::GetDataInfo(sInt &dim)
{
  if (PathSpline==0) return 0;
  dim=sMin(6,PathSpline->Count);
  if (dim<3) return 0;  
  if (dim==3) return 1;
  if (dim<6) return 3;
  return 7;
}

int RNFR063AermelKanal::GetData(float time, sVector31 &pos, sVector2 &radius, sF32 &rotation)
{
  sF32 buf[6]={0,0,0,0,0,0};
  sInt dim;
  int ret=GetDataInfo(dim);
  if (ret==0) return 0;
  time=CalcTime(time);
  PathSpline->Eval(time,buf,dim);
  pos.Init(buf[0],buf[1],buf[2]); 
  radius.Init((buf[3]+Para.RadiusOffset[0])*Para.RadiusScale[0],(buf[4]+Para.RadiusOffset[0])*Para.RadiusScale[1]);
  rotation=buf[5]+Para.RotationOffset;
  return ret;
}

void RNFR063AermelKanal::CalcRing(const sVector31 &pos, const sVector30 dir, const sVector2 radius, const sF32 rotation, sVertexStandard *buf)
{
  sInt i;
  sMatrix34 mata;
  sMatrix34 matb;
  mata.EulerXYZ(0,0,rotation); 
  matb.Look(dir);
  mata=mata*matb;

  for (i=0;i<Para.Slices;i++)
  {
    sVector30 tpos;
    sF32 f=i/((float)Para.Slices-1);
    f=(f+rotation)*2*sPI;
    tpos.x=sin(f)*radius.x;
    tpos.y=cos(f)*radius.y;
    tpos.z=0;
    tpos=tpos*mata+((sVector30)pos);
    if (i==(Para.Slices-1)) 
    {
      buf[i]=buf[0];
    }
    else
    {
      buf[i].px=tpos.x; buf[i].py=tpos.y; buf[i].pz=tpos.z;
    }
  }
}


void RNFR063AermelKanal::Init2()
{
  sInt i;
  sInt nas=Para.Segments;
  InitDone=sTRUE;  
 
//Create Fix Mesh  
  Sts=1.0f/(nas-1);  

  //Init Vertices  
  FixVertices.Resize(nas*Para.Slices);
  DynVertices.Resize((Para.Range*2+1)*Para.Slices);
  sVector30 dir;
  sF32 len=0;
  
  for (i=0;i<nas*Para.Slices;i++)
  {              
    FixVertices[i].nx=0.0f; FixVertices[i].ny=0.0f; FixVertices[i].nz=0.0f;    
  }

  //First Init position and calc length of spline
  for (i=0;i<nas;i++)
  { 
    sVector31 p1;
    sVector31 p2;    
    sVector31 pos;
    sVector2  radius;
    sF32      rotation;
    float t1=i*Sts;
    float t2=(i+1)*Sts;        
    GetPos(t1,p1);  //current pos    
    GetPos(t2,p2);  //next pos    
    if (i!=(nas-1)) dir=p2-p1;  //special case, keep previous dir     
    GetData(t1, pos, radius, rotation);    
    CalcRing(pos,dir,radius,rotation,&FixVertices[i*Para.Slices]);    
    for (int j=0;j<Para.Slices;j++)
    {
      int h=j-1;
      int k=j+1;
      if (k>=Para.Slices) k=0;
      if (h<0) h=Para.Slices-1;

      h+=i*Para.Slices;
      j+=i*Para.Slices;
      k+=i*Para.Slices;

      sVector30 p1(FixVertices[h].px,FixVertices[h].py,FixVertices[h].pz);
      sVector30 p2(FixVertices[j].px,FixVertices[j].py,FixVertices[j].pz);
      sVector30 p3(FixVertices[k].px,FixVertices[k].py,FixVertices[k].pz);

      p2=p2-p1; p3=p3-p1;
      p2.Cross(p2,dir);
      p3.Cross(p3,dir);
      p2.Unit();
      p3.Unit();

      p2=p2+p3;

      FixVertices[j].nx+=p2.x; FixVertices[j].ny+=p2.y; FixVertices[j].nz+=p2.z;      
    }        
    len+=dir.Length();
  }
//  sF32 totallen=len;

  sArray<sF32> sl;  //slices length  
  sl.Resize(Para.Slices);
  for (i=0;i<Para.Slices;i++) sl[i]=0.0f;

  //Calculate length each slice vertex path
  for (i=0;i<nas;i++)
  {        
    int o=i+1;
    if (o==nas) o=i-1;

    for (int j=0;j<Para.Slices;j++)
    {
      int k=i*Para.Slices+j;
      int l=o*Para.Slices+j;
      sVector31 p1=sVector31(FixVertices[k].px,FixVertices[k].py,FixVertices[k].pz);
      sVector31 p2=sVector31(FixVertices[l].px,FixVertices[l].py,FixVertices[l].pz);
      sVector30 dir=p1-p2;
      sl[j]+=dir.Length();
    }
  }

  sArray<sF32> tl;  //temporary length  
  tl.Resize(Para.Slices);
  for (i=0;i<Para.Slices;i++) tl[i]=0.0f;

  for (i=0;i<nas;i++)
  {        
    int o=i+1;
    if (o==nas) o=i-1;

    for (int j=0;j<Para.Slices;j++)
    {
      int k=i*Para.Slices+j;
      int l=o*Para.Slices+j;
      sVector31 p1=sVector31(FixVertices[k].px,FixVertices[k].py,FixVertices[k].pz);
      sVector31 p2=sVector31(FixVertices[l].px,FixVertices[l].py,FixVertices[l].pz);
      sVector30 dir=p1-p2;
      tl[j]+=dir.Length();      
      sF32 _tl=tl[j];
      sF32 _sl=sl[j];
      sF32 u0=_tl/_sl;
      sF32 v0=((float)j)/(Para.Slices-1);
      FixVertices[k].u0=u0;
      FixVertices[k].v0=v0;
    }
  }
  
  for (i=0;i<nas*Para.Slices;i++)
  { 
    sVector30 t(FixVertices[i].nx,FixVertices[i].ny,FixVertices[i].nz);
    t.Unit();
    FixVertices[i].nx=t.x; FixVertices[i].ny=t.y; FixVertices[i].nz=t.z;
  }

  sVertexStandard *vp=0;
  FixGeo->BeginLoadVB(FixVertices.GetCount(),sGD_STATIC,&vp);
  sCopyMem(vp,&FixVertices[0],sizeof(sVertexStandard)*FixVertices.GetCount());
  FixGeo->EndLoadVB();

  int *ip=0;  
  FixGeo->BeginLoadIB((nas-1)*Para.Slices*2*3,sGD_STATIC,&ip);
  for (i=0;i<(nas-1);i++)
  {   
    int j=i*Para.Slices;
    int k=j+Para.Slices-1;
    int n=k+1;
    int m=k+Para.Slices;
    for (int o=0;o<Para.Slices;o++)
    {  
      *ip++=n;
      *ip++=j;
      *ip++=k;      
      *ip++=m;
      *ip++=n;
      *ip++=k;      
      k=j;
      m=n;
      j++;
      n++;
    }
  }    
  FixGeo->EndLoadIB();
  DrawRange[0].Start=0;
  DrawRange[0].End=3;
  DrawRange[1].Start=3;
  DrawRange[1].End=(nas-1)*Para.Slices*2*3;
}

void RNFR063AermelKanal::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);

  //Init
  if (PathSymbol && PathSymbol->Value && PathSymbol->Value->Spline) PathSpline=PathSymbol->Value->Spline;
  if (MovementSymbol  && MovementSymbol->Value  && MovementSymbol->Value->Spline)  MovementSpline=MovementSymbol->Value->Spline;
    
  if (!InitDone)// && PathSpline)  
  {
    Init2();
  }

  if (Para.DebugMode&1) InitDone=sFALSE;  
  
  sInt i;    
  sF32 time=ctx->GetTime();  
  sF32 objtime=time;
  sF32 rot=0.0f;
  if (MovementSpline)
  {
    sF32 t;
    GetMovementData(time, t, objtime, rot);
    time=t;
  }

  if (PathSpline)
  {
    sMatrix34 AdfMat;
    sSRT srtt;
    sSRT srto;

    sVector31 p1;
    sVector31 p2;
    sVector30 tunneldir;
    sVector30 objdir;
    sMatrix34 matlt;
    sMatrix34 matlo;
    sMatrix34 matc;

    GetPos(objtime,p1);
    GetPos(objtime+Sts/32.0f,p2);
    objdir=p2-p1;
    matlo.Look(p2-p1);
    srto.Translate = p1;
    matc.EulerXYZ(0,0,rot);
    srto.MakeMatrix(ObjMat);  
    ObjMat=matlo*ObjMat;
    ObjMat=matc*ObjMat;

    //Calculate direction of spline part
    GetPos(time,p1);
    GetPos(time+Sts/32.0f,p2);    
    tunneldir=p2-p1;
    matlt.Look(p2-p1);
    srtt.Translate = p1;

    srtt.MakeMatrix(TunnelMat);    
    TunnelMat=matlt*TunnelMat;
    TunnelMat.Invert34();
    ObjMat=ObjMat*TunnelMat;

    sInt ranges=Para.Range;
    sInt rangee=Para.Range;
    sInt seg=time*Para.Segments;
    sInt segs=seg-ranges;
    sInt sege=seg+rangee;
    if (segs<0) { ranges+=segs; segs=0; }
    if (sege>=Para.Segments) { rangee-=(sege-(Para.Segments-1)); sege=Para.Segments-1; }    
    sInt range=ranges+rangee;
    sVertexStandard *vp=0;

    sCopyMem(&DynVertices[0],&FixVertices[segs*Para.Slices],sizeof(sVertexStandard)*(range+1)*Para.Slices);

    const sVertexStandard *s=&FixVertices[segs*Para.Slices];
    sVertexStandard *d=&DynVertices[0];
    

    for (i=1;i<range*Para.Slices;i++)
    {
      sVector30 n;
      d[i]=s[i];
      sVector31 op=sVector31(s[i].px,s[i].py,s[i].pz);
      //sVector31 tp=op*AdfMat;
      //sF32 dist=DF->GetObj()->GetDistance(tp);
      //DF->GetObj()->GetNormal(tp,n);        
      //if (dist>0.0f) dist=0.0f;    
      //tp=tp+dist*n;       
      //op=tp*ObjMat;    
      d[i].px=op.x;d[i].py=op.y;d[i].pz=op.z;
    }

    //Start
    DynGeo->BeginLoadVB((range+1)*Para.Slices,sGD_FRAME,&vp);
    sCopyMem(vp,&DynVertices[0],sizeof(sVertexStandard)*(range+1)*Para.Slices);
    DynGeo->EndLoadVB();

    int *ip=0;  
    DynGeo->BeginLoadIB(range*Para.Slices*2*3,sGD_FRAME,&ip);
    for (i=0;i<range;i++)
    {   
      int j=i*Para.Slices;
      int k=j+Para.Slices-1;
      int n=k+1;
      int m=k+Para.Slices;
      for (int o=0;o<Para.Slices;o++)
      {  
        *ip++=n;
        *ip++=j;
        *ip++=k;      
        *ip++=m;
        *ip++=n;
        *ip++=k;      
        k=j;
        m=n;
        j++;
        n++;
      }
    }    
    DynGeo->EndLoadIB();

    DrawRange[0].End=segs*Para.Slices*2*3;
    DrawRange[1].Start=sege*Para.Slices*2*3;           
  }
}

void RNFR063AermelKanal::Prepare(Wz4RenderContext *ctx)
{
  if (Mesh)
    Mesh->BeforeFrame(Para.LightEnv,Matrices.GetCount(),Matrices.GetData());
}

void RNFR063AermelKanal::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  TransformChilds(ctx,mat);
}

void RNFR063AermelKanal::TransformChilds(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  Wz4RenderNode *c;
  sMatrix34 m;

  Matrices.AddTail(sMatrix34CM(m));

  sFORALL(Childs,c)
    c->Transform(ctx,mat*ObjMat);
}

void RNFR063AermelKanal::Render(Wz4RenderContext *ctx)
{
  sMatrix34CM *mat;

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_ZONLY ||
     (ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_ZNORMAL ||
     (ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_DIST)
  {
    // here the material can decline to render itself
    if(Mtrl->SkipPhase(ctx->RenderMode,Para.LightEnv)) return;

    // the effect may be placed multple times in the render graph.
    // this loop will get all the matrices:  
    sFORALL(Matrices,mat)
    {
      // here i add our own matrix
      sMatrix34CM mat1 = sMatrix34CM(sMatrix34(*mat)*Matrix);

      // render it once
      Mtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv,&mat1,0,0,0);
      if (PathSpline && FixGeo)
      {
        sGeometryDrawInfo di;
        di.Flags |= sGDI_Ranges;
        di.Ranges = DrawRange;
        di.RangeCount = 2;
        FixGeo->Draw(di);
      }
    }
  }

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    // here the material can decline to render itself
    if(Mtrl->SkipPhase(ctx->RenderMode,Para.LightEnv)) return;
    
    // the effect may be placed multple times in the render graph.
    // this loop will get all the matrices:    
    sFORALL(Matrices,mat)
    {
      // here i add our own matrix      
      
      sMatrix34CM mat1 = sMatrix34CM(sMatrix34(*mat)*Matrix);
      mat1=sMatrix34CM(TunnelMat*sMatrix34(mat1));
      
      
      // render it once
      Mtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv,&mat1,0,0,0);
      if (PathSpline && FixGeo && DynGeo) 
      {
        sGeometryDrawInfo di;
        di.Flags = sGDI_Ranges;
        di.Ranges = DrawRange;
        di.RangeCount = 2;
        FixGeo->Draw(di);
        DynGeo->Draw();
      }
    }
  }

  if (Mesh)
  {
    sFORALL(Matrices,mat)
    {
      *mat=sMatrix34CM(ObjMat*sMatrix34(*mat));
      Mesh->Render(ctx->RenderMode,Para.LightEnv,mat,0,ctx->Frustum);
    }
  }
  
  RenderChilds(ctx);
}

void RNFR063AermelKanal::Handles(wPaintInfo &pi, Wz4RenderParaFR063_ClothGridSimRender *para, wOp *op)
{
}

