/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_cubemap.hpp"
#include "wz4_cubemap_ops.hpp"

/****************************************************************************/

static sU32 ReadPixel(Pixel *src)
{
  return ((src->a>>8)<<24) | ((src->r>>8)<<16) | ((src->g>>8)<<8) | ((src->b>>8)<<0);
}

static sU32 UMulShift8(sU32 a,sU32 b)
{
  return (sU64(a) * sU64(b) + 0x80) >> 8;
}

static sU32 MulIntens(sU32 a,sU32 b)
{
  sU32 x = a*b + 0x8000;
  return (x + (x >> 16)) >> 16;
}

// Returns the result of round(a*b/65536)
static sInt MulShift16(sInt a,sInt b)
{
  return (sS64(a) * sS64(b) + 0x8000) >> 16;
}

/****************************************************************************/
/***                                                                      ***/
/***   Cubemap                                                            ***/
/***                                                                      ***/
/****************************************************************************/

Wz4Cubemap::Wz4Cubemap()
{
  Type = Wz4CubemapType;
  Size = 0;
  Shift = 0;
  Mask = 0;
  SquareSize = 0;
  CubeSize = 0;
  RSize = 0;
  HSize = 0;

  Data = 0;
}

Wz4Cubemap::~Wz4Cubemap()
{
  delete[] Data;
}

void Wz4Cubemap::Init(sInt size)
{
  delete[] Data;

  Size = size;
  Shift = sFindLowerPower(Size);
  Mask = (Size-1);
  sVERIFY((1<<Shift)==Size);
  SquareSize = Size * Size;
  CubeSize = 6 * SquareSize;
  Data = new Pixel[CubeSize];
  RSize = 1.0f/(Size);
  HSize = 0.5f/Size;
}

void Wz4Cubemap::CopyFrom(const Wz4Cubemap *src)
{
  Init(src->Size);
  sCopyMem(Data,src->Data,CubeSize*sizeof(Pixel));
}

void Wz4Cubemap::CopyTo(class sImage **cubefaces)
{
  Pixel *src = Data;
  for(sInt i=0;i<6;i++)
  {
    cubefaces[i]->Init(Size,Size);
    sU32 *dest = cubefaces[i]->Data;
    for(sInt j=0;j<SquareSize;j++)
    {
      *dest = ReadPixel(src);
      dest++;
      src++;      
    }
  }
}

void Wz4Cubemap::MakeCube(sInt face,sF32 fx,sF32 fy,sVector30 &n) const
{
  static const sF32 dir[6][9] = 
  {
    {  1, 1, 1,  0, 0,-2,  0,-2, 0 }, // +x
    { -1, 1,-1,  0, 0, 2,  0,-2, 0 }, // -x
    { -1, 1,-1,  2, 0, 0,  0, 0, 2 }, // +y
    { -1,-1, 1,  2, 0, 0,  0, 0,-2 }, // -y
    { -1, 1, 1,  2, 0, 0,  0,-2, 0 }, // +z
    {  1, 1,-1, -2, 0, 0,  0,-2, 0 }, // -z
  };

  n.x = dir[face][0] + fx*dir[face][3] + fy*dir[face][6];
  n.y = dir[face][1] + fx*dir[face][4] + fy*dir[face][7];
  n.z = dir[face][2] + fx*dir[face][5] + fy*dir[face][8];
} 

void Wz4Cubemap::MakeCube(sInt i,sVector30 &n) const
{
  sInt x,y,f;
  sF32 fx,fy;

  x = i&Mask;
  y = (i>>Shift)&Mask;
  f = i>>(2*Shift);

  fx = x*RSize+HSize;
  fy = y*RSize+HSize;

  MakeCube(f,fx,fy,n);
}

void Wz4Cubemap::Sample(const sVector30 &n,Pixel &result) const
{
  sF32 ax,ay,az;
  sF32 nx,ny,nz;
  sF32 u,v,s;
  sInt fx,fy;
  sInt x00,x01,x10,x11;
  sInt y00,y01,y10,y11;
  sInt f00,f01,f10,f11;
  Pixel c00,c01,c10,c11,c0,c1;


  nx = n.x;
  ny = n.y;
  nz = n.z;
  ax = sFAbs(nx);
  ay = sFAbs(ny);
  az = sFAbs(nz);
  if(ax>ay && ax>az)
  {
    if(n.x>0) { f00 = 0; u=-nz; v=-ny; s= nx; }
    else      { f00 = 1; u= nz; v=-ny; s=-nx; }
  }
  else if(ay>az)
  {
    if(n.y>0) { f00 = 2; u= nx; v= nz; s= ny;  }
    else      { f00 = 3; u= nx; v=-nz; s=-ny;  }
  }
  else
  {
    if(n.z>0) { f00 = 4; u= nx; v=-ny; s= nz;  }
    else      { f00 = 5; u=-nx; v=-ny; s=-nz;  }
  }

  s = 1/s;
  u *= s; u = u*0.5f+0.5f;
  v *= s; v = v*0.5f+0.5f;
  fx = sInt(u*Size*256)-128;
  fy = sInt(v*Size*256)-128;
  x00 = fx>>8; fx = fx&255;
  y00 = fy>>8; fy = fy&255;
  x01 = x00+1;
  y01 = y00;
  f01 = f00;
  x10 = x00;
  y10 = y00+1;
  f10 = f00;
  x11 = x00+1;
  y11 = y00+1;
  f11 = f00;

  // bordercase code is wrong: should walk from face to face!

  x00 = sClamp(x00,0,Size-1);
  x01 = sClamp(x01,0,Size-1);
  x10 = sClamp(x10,0,Size-1);
  x11 = sClamp(x11,0,Size-1);
  y00 = sClamp(y00,0,Size-1);
  y01 = sClamp(y01,0,Size-1);
  y10 = sClamp(y10,0,Size-1);
  y11 = sClamp(y11,0,Size-1);

  c00 = Data[(f00<<(Shift*2))+(y00<<Shift)+x00];
  c01 = Data[(f01<<(Shift*2))+(y01<<Shift)+x01];
  c10 = Data[(f10<<(Shift*2))+(y10<<Shift)+x10];
  c11 = Data[(f11<<(Shift*2))+(y11<<Shift)+x11];

  c0.Lerp(fx*256,c00,c01);
  c1.Lerp(fx*256,c10,c11);
  result.Lerp(fy*256,c0,c1);
}


/****************************************************************************/
/****************************************************************************/

void Wz4Cubemap::Flat(const Pixel &col)
{
  for(sInt i=0;i<CubeSize;i++)
    Data[i] = col;
}

/****************************************************************************/

void Wz4Cubemap::Noise(const GenTexture *grad,sInt freq,sInt oct,sF32 fadeoff,sInt seed,sInt mode)
{
  sVERIFY(oct > 0);

//  seed = P(seed);

  sInt offset;
  sF32 scaling;
  
  if(mode & 2)
    scaling = (fadeoff - 1.0f) / (sFPow(fadeoff,oct) - 1.0f);
  else
    scaling = sMin(1.0f,1.0f / fadeoff);

  if(mode & 1) // absolute mode
  {
    offset = 0;
    scaling *= (1 << 24);
  }
  else
  {
    offset = 1 << 23;
    scaling *= (1 << 23);
  }

 // sInt offs = (1 << (16 - Shift + freq)) >> 1;

  Pixel *out = Data;
  for(sInt j=0;j<CubeSize;j++)
  {
    sVector30 normal;
    MakeCube(j,normal); normal.Unit();
    sInt nx = sInt(normal.x*0x08000);
    sInt ny = sInt(normal.y*0x08000);
    sInt nz = sInt(normal.z*0x08000);

    sInt n = offset;
    sF32 s = scaling;

    for(sInt i=freq;i<freq+oct;i++)
    {
//      sF32 nv = (mode & NoiseBandlimit) ? Noise2(px,py,mx,my,seed) : GNoise2(px,py,mx,my,seed);
      sF32 nv = sPerlin3D(nx<<i,ny<<i,nz<<i,((1<<i)-1)&0xff,seed);
      if(mode & 1)
        nv = sFAbs(nv);

      n += sInt(nv * s);
      s *= fadeoff;
    }

    grad->SampleGradient(*out,n);
    out++;
  }
}

/****************************************************************************/

void Wz4Cubemap::Glow(const Wz4Cubemap *background,const GenTexture *grad,const sVector30 &dir,sF32 radius)
{
  Pixel col;
  CopyFrom(background);
  for(sInt i=0;i<CubeSize;i++)
  {
    sVector30 normal;
    MakeCube(i,normal); normal.Unit();

    sF32 n = ((dir^normal)-(1-radius))/radius;
    if(n<0) n=0;
    grad->SampleGradient(col,sInt(n*(1<<24)));

    Data[i].CompositeAdd(col);
  }
}

/****************************************************************************/

void Wz4Cubemap::ColorMatrixTransform(const Wz4Cubemap *x,const Matrix45 &matrix,sBool clampPremult)
{
  sInt m[4][5];

  sVERIFY(Size==x->Size);

  for(sInt i=0;i<4;i++)
  {
    for(sInt j=0;j<5;j++)
    {
      sVERIFY(matrix[i][j] >= -127.0f && matrix[i][j] <= 127.0f);
      m[i][j] = sInt(matrix[i][j] * 65536.0f);
    }
  }

  for(sInt i=0;i<CubeSize;i++)
  {
    Pixel &out = Data[i];
    const Pixel &in = x->Data[i];

    sInt r = MulShift16(m[0][0],in.r) + MulShift16(m[0][1],in.g) + MulShift16(m[0][2],in.b) + MulShift16(m[0][3],in.a) + m[0][4];
    sInt g = MulShift16(m[1][0],in.r) + MulShift16(m[1][1],in.g) + MulShift16(m[1][2],in.b) + MulShift16(m[1][3],in.a) + m[1][4];
    sInt b = MulShift16(m[2][0],in.r) + MulShift16(m[2][1],in.g) + MulShift16(m[2][2],in.b) + MulShift16(m[2][3],in.a) + m[2][4];
    sInt a = MulShift16(m[3][0],in.r) + MulShift16(m[3][1],in.g) + MulShift16(m[3][2],in.b) + MulShift16(m[3][3],in.a) + m[3][4];

    if(clampPremult)
    {
      out.a = sClamp<sInt>(a,0,65535);
      out.r = sClamp<sInt>(r,0,out.a);
      out.g = sClamp<sInt>(g,0,out.a);
      out.b = sClamp<sInt>(b,0,out.a);
    }
    else
    {
      out.r = sClamp<sInt>(r,0,65535);
      out.g = sClamp<sInt>(g,0,65535);
      out.b = sClamp<sInt>(b,0,65535);
      out.a = sClamp<sInt>(a,0,65535);
    }
  }
}

/****************************************************************************/

void Wz4Cubemap::CoordMatrixTransform(const Wz4Cubemap *in,const sMatrix34 &matrix)
{
  sVERIFY(Size==in->Size);
  sVector30 normal;
  sVector31 pos;

  for(sInt i=0;i<CubeSize;i++)
  {
    MakeCube(i,normal); 
    normal.Unit();

    pos = sVector31(normal) * matrix;

    in->Sample(sVector30(pos),Data[i]);
  }
}

/****************************************************************************/

void Wz4Cubemap::ColorRemap(const Wz4Cubemap *inTex,const GenTexture *mapR,const GenTexture *mapG,const GenTexture *mapB)
{
  sVERIFY(Size==inTex->Size);

//  const Pixel *in = inTex->Data;
//  Pixel *out = Data;

  for(sInt i=0;i<CubeSize;i++)
  {
    const Pixel &in = inTex->Data[i];
    Pixel &out = Data[i];

    if(in.a == 65535) // alpha==1, everything easy.
    {
      Pixel colR,colG,colB;

      mapR->SampleGradient(colR,(in.r << 8) + ((in.r + 128) >> 8));
      mapG->SampleGradient(colG,(in.g << 8) + ((in.g + 128) >> 8));
      mapB->SampleGradient(colB,(in.b << 8) + ((in.b + 128) >> 8));

      out.r = sMin(colR.r+colG.r+colB.r,65535);
      out.g = sMin(colR.g+colG.g+colB.g,65535);
      out.b = sMin(colR.b+colG.b+colB.b,65535);
      out.a = in.a;
    }
    else if(in.a) // alpha!=0
    {
      Pixel colR,colG,colB;
      sU32 invA = (65535U << 16) / in.a;

      mapR->SampleGradient(colR,UMulShift8(sMin(in.r,in.a),invA));
      mapG->SampleGradient(colG,UMulShift8(sMin(in.g,in.a),invA));
      mapB->SampleGradient(colB,UMulShift8(sMin(in.b,in.a),invA));

      out.r = MulIntens(sMin(colR.r+colG.r+colB.r,65535),in.a);
      out.g = MulIntens(sMin(colR.g+colG.g+colB.g,65535),in.a);
      out.b = MulIntens(sMin(colR.b+colG.b+colB.b,65535),in.a);
      out.a = in.a;
    }
    else // alpha==0
    {
      out = in;
    }
  }
}

/****************************************************************************/

void Wz4Cubemap::Binary(const Wz4Cubemap *tex0,const Wz4Cubemap *tex1,CombineOp op)
{
  sVERIFY(Size==tex0->Size);
  sVERIFY(Size==tex1->Size);

//  Pixel *out = Data;
  sInt transIn,transOut;

  for(sInt i=0;i<CubeSize;i++)
  {
    const Pixel &in0 = tex0->Data[i];
    const Pixel &in1  = tex1->Data[i];
    Pixel &out = Data[i];

    switch(op)
    {
    case CombineAdd:
      out.r = sMin(in0.r + in1.r,65535);
      out.g = sMin(in0.g + in1.g,65535);
      out.b = sMin(in0.b + in1.b,65535);
      out.a = sMin(in0.a + in1.a,65535);
      break;

    case CombineSub:
      out.r = sMax<sInt>(in0.r - in1.r,0);
      out.g = sMax<sInt>(in0.g - in1.g,0);
      out.b = sMax<sInt>(in0.b - in1.b,0);
      out.a = sMax<sInt>(in0.a - in1.a,0);
      break;

    case CombineMulC:
      out.r = MulIntens(in0.r,in1.r);
      out.g = MulIntens(in0.g,in1.g);
      out.b = MulIntens(in0.b,in1.b);
      out.a = MulIntens(in0.a,in1.a);
      break;

    case CombineMin:
      out.r = sMin(in0.r,in1.r);
      out.g = sMin(in0.g,in1.g);
      out.b = sMin(in0.b,in1.b);
      out.a = sMin(in0.a,in1.a);
      break;

    case CombineMax:
      out.r = sMax(in0.r,in1.r);
      out.g = sMax(in0.g,in1.g);
      out.b = sMax(in0.b,in1.b);
      out.a = sMax(in0.a,in1.a);
      break;

    case CombineSetAlpha:
      out.r = in0.r;
      out.g = in0.g;
      out.b = in0.b;
      out.a = in1.r;
      break;

    case CombinePreAlpha:
      out.r = MulIntens(in0.r,in1.r);
      out.g = MulIntens(in0.g,in1.r);
      out.b = MulIntens(in0.b,in1.r);
      out.a = in1.g;
      break;

    case CombineOver:
      transIn = 65535 - in1.a;

      out.r = MulIntens(transIn,in0.r) + in1.r;
      out.g = MulIntens(transIn,in0.g) + in1.g;
      out.b = MulIntens(transIn,in0.b) + in1.b;
      out.a += MulIntens(in1.a,65535-out.a);
      break;

    case CombineMultiply:
      transIn = 65535 - in1.a;
      transOut = 65535 - out.a;

      out.r = MulIntens(transIn,in0.r) + MulIntens(transOut,in1.r) + MulIntens(in1.r,in0.r);
      out.g = MulIntens(transIn,in0.g) + MulIntens(transOut,in1.g) + MulIntens(in1.g,in0.g);
      out.b = MulIntens(transIn,in0.b) + MulIntens(transOut,in1.b) + MulIntens(in1.b,in0.b);
      out.a += MulIntens(in1.a,transOut);
      break;

    case CombineScreen:
      out.r += MulIntens(in1.r,65535-in0.r);
      out.g += MulIntens(in1.g,65535-in0.g);
      out.b += MulIntens(in1.b,65535-in0.b);
      out.a += MulIntens(in1.a,65535-in0.a);
      break;

    case CombineDarken:
      out.r += in1.r - sMax(MulIntens(in1.r,in0.a),MulIntens(in0.r,in1.a));
      out.g += in1.g - sMax(MulIntens(in1.g,in0.a),MulIntens(in0.g,in1.a));
      out.b += in1.b - sMax(MulIntens(in1.b,in0.a),MulIntens(in0.b,in1.a));
      out.a += MulIntens(in1.a,65535-in0.a);
      break;

    case CombineLighten:
      out.r += in1.r - sMin(MulIntens(in1.r,in0.a),MulIntens(in0.r,in1.a));
      out.g += in1.g - sMin(MulIntens(in1.g,in0.a),MulIntens(in0.g,in1.a));
      out.b += in1.b - sMin(MulIntens(in1.b,in0.a),MulIntens(in0.b,in1.a));
      out.a += MulIntens(in1.a,65535-in0.a);
      break;
    }
  }
}

/****************************************************************************/
