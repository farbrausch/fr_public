/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "util/painter.hpp"
#include "util/shaders.hpp"
extern sU8 sFontData[];

/****************************************************************************/

sBasicPainter::sBasicPainter(sInt vertexmax)
{
  UVOffset = 0;
  XYOffset = 0;

  Geo = new sGeometry;
#if sCONFIG_QUADRICS
  Geo->Init(sGF_QUADRICS,sVertexFormatSingle);
#else
  Geo->Init(sGF_QUADLIST,sVertexFormatSingle);
#endif
  GeoX = new sGeometry;
  GeoX->Init(sGF_QUADLIST,sVertexFormatSingle);

  GeoXStandard = new sGeometry;
  GeoXStandard->Init(sGF_QUADLIST,sVertexFormatStandard);

  {
    const sInt tx = 128;
    const sInt ty = 128;

    sU32 col0 = 0x00ffffff;
    sU32 col1 = 0xffffffff;

    Tex = new sTexture2D(tx,ty,sTEX_ARGB8888|sTEX_2D,1);
    sU32 *data = (sU32 *)sAllocMem(tx*ty*4,16,sAMF_ALT);

    for(sInt y=0;y<16;y++)
    {
      for(sInt x=0;x<16;x++)
      {
        for(sInt i=0;i<8;i++)
        {
          sInt bits = sFontData[(y*16+x)*8+i];
          for(sInt j=0;j<8;j++)
          {
            data[(y*8+i)*128+x*8+j] = (bits&0x80) ? col1 : col0;
            bits = bits*2;
          }
        }
      }
    }

    Tex->LoadAllMipmaps((sU8 *)data);
    sFreeMem(data);
  }

  sSimpleMaterial *mtrl;

  mtrl = new sSimpleMaterial;
  mtrl->InitVariants(2);
  mtrl->Texture[0] = Tex;
  mtrl->TFlags[0] = sMTF_LEVEL0;
  mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF|sMTRL_VC_COLOR0;
  mtrl->BlendColor = sMB_OFF;
  mtrl->SetVariant(1);
  mtrl->BlendColor = sMB_ALPHA;
  mtrl->SetVariant(0);
  mtrl->Prepare(sVertexFormatSingle);
  Mtrl = mtrl;

  Env = new sMaterialEnv;
  Env->Fix();

  CubeMtrl = new sCubeMaterial;
  CubeMtrl->InitVariants(2);
  CubeMtrl->TFlags[0] = sMTF_LEVEL0|sMTF_EXTERN;
  CubeMtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF|sMTRL_VC_COLOR0;
  CubeMtrl->BlendColor = sMB_OFF;
  CubeMtrl->SetVariant(1);
  CubeMtrl->BlendColor = sMB_ALPHA;
  CubeMtrl->SetVariant(0);
  CubeMtrl->Prepare(sVertexFormatStandard);

#if !sCONFIG_QUADRICS
  Alloc = 5000;                           // :kludge: shamada: increased this value from
                                            //  10000 to 20000 to be able to have more debug
                                            //  graphs. decrease this for memory optimizing!
#if sPLATFORM==sPLAT_WINDOWS
  Alloc = 20000;                        // chaos: on PC, i really don't need to care :-)
#endif
  if(vertexmax)                         // new system: use constructor parameter
    Alloc = vertexmax;
  Used = 0;
  Vertices = new sVertexSingle[Alloc*4];
#endif

  View = new sViewport;
  View->Orthogonal = sVO_PIXELS;
  View->CenterX = 0.5f;
  View->CenterY = 0.5f;
  View->Prepare();

  sGraphicsCaps caps;
  sGetGraphicsCaps(caps);
  UVOffset = caps.UVOffset;
  XYOffset = caps.XYOffset;
}

sBasicPainter::~sBasicPainter()
{
  delete Mtrl;
  delete CubeMtrl;
  delete Geo;
  delete GeoX;
  delete GeoXStandard;
  delete Tex;
  delete Env;
  delete View;
#if !sCONFIG_QUADRICS
  delete[] Vertices;
#endif
}

void sBasicPainter::SetTarget(const sRect &target)
{
  View->SetTargetCurrent(&target);
  View->SetZoom(1);
  View->Prepare();
}

void sBasicPainter::SetTarget()
{
  View->SetTargetCurrent();
  View->Prepare();
}

void sBasicPainter::Begin()
{
#if sCONFIG_QUADRICS
  Geo->BeginQuadrics();
#else
  Used = 0;
#endif
}

void sBasicPainter::End()
{
 #if sCONFIG_QUADRICS
  sCBuffer<sSimpleMaterialPara> cb;
  cb.Data->Set(*View);
  Mtrl->Set(&cb);
  Geo->EndQuadrics();
  Geo->Draw();
#else
  sVertexSingle *vp;
  sVERIFY(Used<=Alloc);
  if(Used>0)
  {
    sCBuffer<sSimpleMaterialPara> cb;
    cb.Data->Set(*View);
    Mtrl->Set(&cb);

    Geo->BeginLoadVB(Used*4,sGD_STREAM,&vp);
    sCopyMem(vp,Vertices,sizeof(sVertexSingle)*4*Used);
    Geo->EndLoadVB();
    Geo->Draw();
  }
#endif
}

void sBasicPainter::Clip(const sFRect &)
{
}

void sBasicPainter::ClipOff()
{
}

void sBasicPainter::PaintTexture(sTexture2D* tex,sU32 col,sInt xs,sInt ys,sInt xo,sInt yo,sBool noalpha)
{
  sFRect r;
  if (xs == -1 || ys == -1)
    r.Init(tex->SizeX,tex->SizeY);
  else
    r.Init(xs,ys);

  // patch texture
  sTextureBase* old_tex = Mtrl->Texture[0];
  Mtrl->Texture[0] = tex;
  Mtrl->Prepare(sVertexFormatSingle);

  // paint
  sCBuffer<sSimpleMaterialPara> cb;
  cb.Data->Set(*View);
  Mtrl->SetV(&cb,noalpha?1:0);

  sVertexSingle *vp;
  GeoX->BeginLoadVB(4,sGD_STREAM,&vp);
  vp[0].Init(r.x0+xo,r.y0+yo,0,col,0,0);
  vp[1].Init(r.x1+xo,r.y0+yo,0,col,1,0);
  vp[2].Init(r.x1+xo,r.y1+yo,0,col,1,1);
  vp[3].Init(r.x0+xo,r.y1+yo,0,col,0,1);
  GeoX->EndLoadVB();
  GeoX->Draw();

  // restore texture
  Mtrl->Texture[0] = old_tex;
}

void sBasicPainter::PaintTexture(sTextureCube *tex, sInt xs/*=-1*/, sInt ys/*=-1*/, sInt xo_/*=0*/, sInt yo_/*=0*/, sBool noalpha/*=sTRUE*/)
{
  sFRect r;
  if (xs == -1 || ys == -1)
    r.Init(tex->SizeXY,tex->SizeXY);
  else
  {
    r.Init(xs/4.0f,ys/3.0f);
  }
  r.Move(xo_,yo_);

  // patch texture
  sTextureBase* old_tex = CubeMtrl->Texture[0];
  CubeMtrl->Texture[0] = tex;

  // paint
  sCBuffer<sSimpleMaterialPara> cb;
  cb.Data->Set(*View);
  CubeMtrl->SetV(&cb,noalpha?1:0);

  sVector30 d[8];
  for(sInt i=0;i<8;i++)
    d[i].Init((i&1)?1.0f:-1.0f,(i&2)?1.0f:-1.0f,(i&4)?1.0f:-1.0f);

  sVertexStandard *vp;
  GeoXStandard->BeginLoadVB(24,sGD_STREAM,&vp);

  // -x
  sFRect pr = r; pr.Move(0,r.SizeY());
  vp[0].Init(pr.x0,pr.y0,0, d[2].x, d[2].y, d[2].z ,0,0);
  vp[1].Init(pr.x1,pr.y0,0, d[6].x, d[6].y, d[6].z ,1,0);
  vp[2].Init(pr.x1,pr.y1,0, d[4].x, d[4].y, d[4].z ,1,1);
  vp[3].Init(pr.x0,pr.y1,0, d[0].x, d[0].y, d[0].z ,0,1);
  vp += 4;

  // +z
  pr.Move(r.SizeX(),0);
  vp[0].Init(pr.x0,pr.y0,0, d[6].x, d[6].y, d[6].z ,0,0);
  vp[1].Init(pr.x1,pr.y0,0, d[7].x, d[7].y, d[7].z ,1,0);
  vp[2].Init(pr.x1,pr.y1,0, d[5].x, d[5].y, d[5].z ,1,1);
  vp[3].Init(pr.x0,pr.y1,0, d[4].x, d[4].y, d[4].z ,0,1);
  vp += 4;

  // +x
  pr.Move(r.SizeX(),0);
  vp[0].Init(pr.x0,pr.y0,0, d[7].x, d[7].y, d[7].z ,0,0);
  vp[1].Init(pr.x1,pr.y0,0, d[3].x, d[3].y, d[3].z ,1,0);
  vp[2].Init(pr.x1,pr.y1,0, d[1].x, d[1].y, d[1].z ,1,1);
  vp[3].Init(pr.x0,pr.y1,0, d[5].x, d[5].y, d[5].z ,0,1);
  vp += 4;

  // -z
  pr.Move(r.SizeX(),0);
  vp[0].Init(pr.x0,pr.y0,0, d[3].x, d[3].y, d[3].z ,0,0);
  vp[1].Init(pr.x1,pr.y0,0, d[2].x, d[2].y, d[2].z ,1,0);
  vp[2].Init(pr.x1,pr.y1,0, d[0].x, d[0].y, d[0].z ,1,1);
  vp[3].Init(pr.x0,pr.y1,0, d[1].x, d[1].y, d[1].z ,0,1);
  vp += 4;

  // +y
  pr = r; pr.Move(r.SizeX(),0);
  vp[0].Init(pr.x0,pr.y0,0, d[2].x, d[2].y, d[2].z ,0,0);
  vp[1].Init(pr.x1,pr.y0,0, d[3].x, d[3].y, d[3].z ,1,0);
  vp[2].Init(pr.x1,pr.y1,0, d[7].x, d[7].y, d[7].z ,1,1);
  vp[3].Init(pr.x0,pr.y1,0, d[6].x, d[6].y, d[6].z ,0,1);
  vp += 4;

  // -y
  pr.Move(0,r.SizeY()*2);
  vp[0].Init(pr.x0,pr.y0,0, d[4].x, d[4].y, d[4].z ,0,0);
  vp[1].Init(pr.x1,pr.y0,0, d[5].x, d[5].y, d[5].z ,1,0);
  vp[2].Init(pr.x1,pr.y1,0, d[1].x, d[1].y, d[1].z ,1,1);
  vp[3].Init(pr.x0,pr.y1,0, d[0].x, d[0].y, d[0].z ,0,1);
  vp += 4;

  GeoXStandard->EndLoadVB();
  GeoXStandard->Draw();

  // restore texture
  CubeMtrl->Texture[0] = old_tex;
}


/****************************************************************************/

void sBasicPainter::Box(const sFRect &r,const sU32 col)
{
  sU32 c[4];
  c[0]=c[1]=c[2]=c[3]=col;
  Box(r,c);
}

void sBasicPainter::Box(const sFRect &r,const sU32 *col)
{
  sVertexSingle *vp;

#if sCONFIG_QUADRICS
  Geo->BeginQuad((void **)&vp,1);
#else
  vp = Vertices + Used*4;
  Used++;
  sVERIFY(Used<=Alloc);
#endif

  const sF32 f=4.0f/128;
  vp[0].Init(r.x0,r.y0,1,col[0],f,f);
  vp[1].Init(r.x1,r.y0,1,col[1],f,f);
  vp[2].Init(r.x1,r.y1,1,col[2],f,f);
  vp[3].Init(r.x0,r.y1,1,col[3],f,f);
#if sCONFIG_QUADRICS
  Geo->EndQuad();
#endif
}

void sBasicPainter::Line(sF32 x0,sF32 y0,sF32 x1,sF32 y1,sU32 c0,sU32 c1,sBool skiplastpixel)
{
  sVertexSingle *vp;
  sVector30 d,e;

#if sCONFIG_QUADRICS
  Geo->BeginQuad((void **)&vp,1);
#else
  vp = Vertices + Used*4;
  Used++;
  sVERIFY(Used<=Alloc);
#endif

  d.Init(x1-x0,y1-y0,0);
  d.Unit();
  d = d * 1.0f;
  e.Init(d.y,-d.x,0);

  const sF32 fc=12.0f/128;
  const sF32 f0=3.5f/128;
  const sF32 f1=5.5f/128;
  vp[0].Init( x0-d.x+e.x , y0-d.y+e.y ,0,c0,fc,f0);
  vp[1].Init( x0-d.x-e.x , y0-d.y-e.y ,0,c0,fc,f1);
  if(skiplastpixel)
  {
    vp[3].Init( x1-d.x+e.x , y1-d.y+e.y ,0,c1,fc,f0);
    vp[2].Init( x1-d.x-e.x , y1-d.y-e.y ,0,c1,fc,f1);
  }
  else
  {
    vp[3].Init( x1+d.x+e.x , y1+d.y+e.y ,0,c1,fc,f0);
    vp[2].Init( x1+d.x-e.x , y1+d.y-e.y ,0,c1,fc,f1);
  }
#if sCONFIG_QUADRICS
  Geo->EndQuad();
#endif
}

/****************************************************************************/

void sBasicPainter::RegisterFont(sInt /*fontid*/,const sChar * /*name*/,sInt /*height*/,sInt /*style*/)
{
}

void sBasicPainter::Print(sInt /*fontid*/,sF32 x,sF32 y,sU32 col,const sChar *text,sInt len,sF32 zoom)
{
  sVertexSingle *vp;

  if(len==-1) 
    len = sGetStringLen(text);

  if(!len)
    return;

#if sCONFIG_QUADRICS
  Geo->BeginQuad((void **)&vp,len);
#else
  vp = Vertices + Used*4;
  len = sMin(len,Alloc-Used);
  Used += len;
#endif
  for(sInt i=0;i<len;i++)
  {
    sInt c = text[i];
    sInt u = (c&15)*8;
    sInt v = (c/16)*8;
#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11
    sF32 uvo = UVOffset;
    sF32 xyo = XYOffset;
#else
    sF32 uvo = 0.0f;
    sF32 xyo = 0.0f;
#endif
    sFRect r(x+xyo,y+xyo,x+xyo+6*zoom,y+xyo+8*zoom);
    sFRect uv((u+uvo)/128.0f,(v+uvo)/128.0f,(u+6+uvo)/128.0f,(v+8+uvo)/128.0f);


    vp[0].Init(r.x0,r.y0,0,col,uv.x0,uv.y0);
    vp[1].Init(r.x1,r.y0,0,col,uv.x1,uv.y0);
    vp[2].Init(r.x1,r.y1,0,col,uv.x1,uv.y1);
    vp[3].Init(r.x0,r.y1,0,col,uv.x0,uv.y1);
    vp+=4;
    x+=6*zoom;
  }
#if sCONFIG_QUADRICS
  Geo->EndQuad();
#endif
}

sF32 sBasicPainter::GetWidth(sInt /*fontid*/,const sChar *text,sInt len)
{
  if(len==-1) 
    len = sGetStringLen(text);
  return (sF32)(len*6);
}

sF32 sBasicPainter::GetHeight(sInt /*fontid*/)
{
  return 8;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

sPainter::sPainter(sInt vertexmax) : sBasicPainter(vertexmax)
{
  FontId = 0;
  TextColor = ~0;
  Zoom = 1.0f;
  Advance = 10;
  AltColor = ~0;
}


void sPainter::Box(const sFRect &r,sU32 col)
{
  sU32 cols[4];
  cols[0] = col;
  cols[1] = col;
  cols[2] = col;
  cols[3] = col;

  sBasicPainter::Box(r,cols);
}

void sPainter::Box(sF32 x0,sF32 y0,sF32 x1,sF32 y1,sU32 col)
{
  Box(sFRect(x0,y0,x1,y1),col);
}


void sPainter::SetPrint(sInt fontid,sU32 col,sF32 zoom,sU32 altcol,sInt advance)
{
  FontId = fontid;
  TextColor = col;
  AltColor = altcol;
  Zoom = zoom;
  Advance = advance;
}

void sPainter::Print(sF32 x,sF32 y,const sChar *txt)
{
  Print0(x,y,txt);
}

void sPainter::Print0(sF32 x,sF32 y,const sChar *txt)
{
  const sChar *start;
  sInt alt = 0;
  sF32 x0 = x;

  while(*txt)
  {
    for(;;)
    {
      if(*txt=='\'')
      {
        alt = 1-alt;
        txt++;
      }
      else if(*txt=='\n')
      {
        x = x0;
        y += Advance*Zoom;
        txt++;
      }
      else
      {
        break;
      }
    }

    start = txt;
    while(*txt!=0 && *txt!='\'' && *txt!='\n')
      txt++;

    if(start!=txt)
      Print(FontId,x,y,alt ? AltColor : TextColor,start,txt-start,Zoom);
    x += GetWidth(FontId,start,txt-start)*Zoom;
  }
}

/****************************************************************************/




sU8 sFontData[] = 
//ChrSet generated by ChrsetEditor
{
    255,255,255,255,255,255,255,255,
    0,0,0,0,255,0,0,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    0,0,0,0,0,0,0,0,
    32,32,32,32,32,0,32,0,
    80,80,0,0,0,0,0,0,
    80,80,248,80,248,80,80,0,
    32,120,160,112,40,240,32,0,
    192,200,16,32,64,152,24,0,
    96,144,144,96,168,144,104,0,
    32,32,64,0,0,0,0,0,
    16,32,64,64,64,32,16,0,
    64,32,16,16,16,32,64,0,
    0,80,112,248,112,80,0,0,
    0,32,32,248,32,32,0,0,
    0,0,0,0,0,32,32,64,
    0,0,0,248,0,0,0,0,
    0,0,0,0,0,0,32,0,
    8,16,16,32,32,64,64,128,
    112,136,152,168,200,136,112,0,
    48,16,16,16,16,16,16,0,
    240,8,8,16,32,64,248,0,
    240,8,8,48,8,8,240,0,
    16,48,80,144,248,16,16,0,
    248,128,128,240,8,8,240,0,
    112,128,128,240,136,136,112,0,
    248,8,16,16,32,32,32,0,
    112,136,136,112,136,136,112,0,
    112,136,136,120,8,8,112,0,
    0,0,32,0,0,32,0,0,
    0,0,32,0,0,32,32,64,
    0,16,32,64,32,16,0,0,
    0,0,248,0,248,0,0,0,
    0,64,32,16,32,64,0,0,
    240,8,8,16,32,0,32,0,
    112,136,184,168,184,128,112,0,
    112,136,136,248,136,136,136,0,
    240,136,136,240,136,136,240,0,
    120,128,128,128,128,128,120,0,
    224,144,136,136,136,144,224,0,
    248,128,128,224,128,128,248,0,
    248,128,128,224,128,128,128,0,
    120,128,128,152,136,136,120,0,
    136,136,136,248,136,136,136,0,
    112,32,32,32,32,32,112,0,
    8,8,8,8,136,136,112,0,
    136,144,160,192,160,144,136,0,
    128,128,128,128,128,128,248,0,
    136,216,168,136,136,136,136,0,
    136,200,168,152,136,136,136,0,
    112,136,136,136,136,136,112,0,
    240,136,136,240,128,128,128,0,
    112,136,136,136,168,144,104,0,
    240,136,136,240,160,144,136,0,
    120,128,128,112,8,8,240,0,
    248,32,32,32,32,32,32,0,
    136,136,136,136,136,136,112,0,
    136,136,136,136,136,80,32,0,
    136,136,136,168,168,216,136,0,
    136,136,80,32,80,136,136,0,
    136,136,136,80,32,32,32,0,
    248,8,16,32,64,128,248,0,
    48,32,32,32,32,32,48,0,
    128,64,64,32,32,16,16,8,
    96,32,32,32,32,32,96,0,
    32,80,136,0,0,0,0,0,
    0,0,0,0,0,0,0,248,
    32,32,16,0,0,0,0,0,
    0,0,112,8,120,136,120,0,
    128,128,240,136,136,136,240,0,
    0,0,120,128,128,128,120,0,
    8,8,120,136,136,136,120,0,
    0,0,112,136,248,128,120,0,
    24,32,112,32,32,32,32,0,
    0,0,120,136,136,120,8,240,
    128,128,240,136,136,136,136,0,
    32,0,32,32,32,32,32,0,
    16,0,16,16,16,16,144,96,
    64,64,72,80,96,80,72,0,
    32,32,32,32,32,32,24,0,
    0,0,240,168,168,168,136,0,
    0,0,176,200,136,136,136,0,
    0,0,112,136,136,136,112,0,
    0,0,240,136,136,240,128,128,
    0,0,120,136,136,120,8,8,
    0,0,88,96,64,64,64,0,
    0,0,120,128,112,8,240,0,
    32,32,112,32,32,32,24,0,
    0,0,136,136,136,136,112,0,
    0,0,136,136,80,80,32,0,
    0,0,136,136,168,168,80,0,
    0,0,136,80,32,80,136,0,
    0,0,136,136,80,32,32,64,
    0,0,248,16,32,64,248,0,
    24,32,32,64,32,32,24,0,
    32,32,32,32,32,32,32,0,
    192,32,32,16,32,32,192,0,
    104,176,0,0,0,0,0,0,
    168,80,168,80,168,80,168,80,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    0,0,0,0,0,0,0,0,
    0,32,0,32,32,32,32,32,
    32,120,160,160,120,32,0,0,
    48,72,64,224,64,64,248,0,
    136,112,136,112,136,0,0,0,
    136,80,32,112,32,32,32,0,
    32,32,32,0,32,32,32,0,
    112,136,96,144,72,48,136,112,
    80,80,0,0,0,0,0,0,
    120,132,180,164,180,132,120,0,
    56,72,88,104,0,120,0,0,
    0,40,80,160,80,40,0,0,
    248,8,0,0,0,0,0,0,
    0,0,0,248,0,0,0,0,
    120,132,180,172,180,172,132,120,
    248,0,0,0,0,0,0,0,
    64,160,64,0,0,0,0,0,
    32,32,248,32,32,0,248,0,
    96,144,32,64,240,0,0,0,
    224,16,96,16,224,0,0,0,
    16,32,0,0,0,0,0,0,
    0,0,136,136,136,216,168,128,
    120,232,232,104,40,40,40,0,
    0,0,48,48,0,0,0,0,
    0,0,0,0,0,0,32,96,
    32,96,32,32,32,0,0,0,
    48,72,72,48,0,120,0,0,
    0,160,80,40,80,160,0,0,
    64,200,80,40,80,184,8,0,
    64,200,80,56,72,144,24,0,
    192,72,48,232,80,184,8,0,
    32,0,32,64,128,136,112,0,
    64,32,112,136,248,136,136,0,
    16,32,112,136,248,136,136,0,
    32,80,112,136,248,136,136,0,
    104,176,112,136,248,136,136,0,
    80,112,136,136,248,136,136,0,
    48,80,112,136,248,136,136,0,
    60,48,80,124,144,144,156,0,
    120,128,128,128,128,128,120,32,
    64,32,248,128,224,128,248,0,
    16,32,248,128,224,128,248,0,
    32,80,248,128,224,128,248,0,
    80,0,248,128,224,128,248,0,
    64,32,112,32,32,32,112,0,
    16,32,112,32,32,32,112,0,
    32,80,112,32,32,32,112,0,
    80,0,112,32,32,32,112,0,
    96,80,72,232,72,80,96,0,
    104,176,200,168,152,136,136,0,
    64,32,112,136,136,136,112,0,
    16,32,112,136,136,136,112,0,
    32,80,112,136,136,136,112,0,
    104,176,112,136,136,136,112,0,
    80,112,136,136,136,136,112,0,
    0,136,80,32,80,136,0,0,
    120,152,152,168,200,200,240,0,
    64,32,136,136,136,136,112,0,
    16,32,136,136,136,136,112,0,
    32,80,136,136,136,136,112,0,
    80,136,136,136,136,136,112,0,
    16,32,136,80,32,32,32,0,
    128,128,240,136,136,240,128,128,
    96,144,144,176,136,136,176,0,
    64,32,112,8,120,136,120,0,
    16,32,112,8,120,136,120,0,
    32,80,112,8,120,136,120,0,
    104,176,112,8,120,136,120,0,
    80,0,112,8,120,136,120,0,
    48,80,112,8,120,136,120,0,
    0,0,216,36,124,176,236,0,
    0,0,120,128,128,128,120,32,
    64,32,112,136,248,128,120,0,
    16,32,112,136,248,128,120,0,
    32,80,112,136,248,128,120,0,
    80,0,112,136,248,128,120,0,
    64,32,0,32,32,32,32,0,
    16,32,0,32,32,32,32,0,
    32,80,0,32,32,32,32,0,
    80,0,32,32,32,32,32,0,
    80,96,16,120,136,136,112,0,
    104,176,0,176,200,136,136,0,
    64,32,0,112,136,136,112,0,
    16,32,0,112,136,136,112,0,
    32,80,0,112,136,136,112,0,
    104,176,0,112,136,136,112,0,
    80,0,112,136,136,136,112,0,
    0,32,0,248,0,32,0,0,
    0,4,120,152,168,200,112,128,
    64,32,136,136,136,136,112,0,
    16,32,136,136,136,136,112,0,
    32,80,136,136,136,136,112,0,
    80,0,136,136,136,136,112,0,
    16,32,136,136,80,32,32,64,
    128,176,200,136,136,200,176,128,
    80,0,136,136,80,32,32,64
};

