/****************************************************************************/
/***                                                                      ***/
/***   Written by Fabian Giesen.                                          ***/
/***   I hereby place this code in the public domain.                     ***/
/***                                                                      ***/
/****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "gentexture.hpp"

#ifdef _WIN32
  #pragma comment(lib,"winmm.lib")
  #include <windows.h>
#else
  #include <sys/time.h>
  static long timeGetTime()
  {
      timeval tim;
      gettimeofday(&tim, NULL);
      return tim.tv_sec * 1000000 + tim.tv_usec / 10;
  }
  static int timeBeginPeriod(unsigned int period)
  {
      return 0;
  }
  static int timeEndPeriod(unsigned int period)
  {
      return 0;
  }
#endif

// 4x4 matrix multiply
static void MatMult(Matrix44 &dest,const Matrix44 &a,const Matrix44 &b)
{
  for(sInt i=0;i<4;i++)
    for(sInt j=0;j<4;j++)
      dest[i][j] = a[i][0]*b[0][j] + a[i][1]*b[1][j] + a[i][2]*b[2][j] + a[i][3]*b[3][j];
}

// Create a scaling matrix
static void MatScale(Matrix44 &dest,sF32 sx,sF32 sy,sF32 sz)
{
  sSetMem(dest,0,sizeof(dest));
  dest[0][0] = sx;
  dest[1][1] = sy;
  dest[2][2] = sz;
  dest[3][3] = 1.0f;
}

// Create a translation matrix
static void MatTranslate(Matrix44 &dest,sF32 tx,sF32 ty,sF32 tz)
{
  MatScale(dest,1.0f,1.0f,1.0f);
  dest[3][0] = tx;
  dest[3][1] = ty;
  dest[3][2] = tz;
}

// Create a z-axis rotation matrix
static void MatRotateZ(Matrix44 &dest,sF32 angle)
{
  sF32 s = sFSin(angle);
  sF32 c = sFCos(angle);

  MatScale(dest,1.0f,1.0f,1.0f);
  dest[0][0] = c;
  dest[0][1] = s;
  dest[1][0] = -s;
  dest[1][1] = c;
}

// Create a simple linear gradient texture
// Input colors are 0xaarrggbb (not premultiplied!)
static GenTexture LinearGradient(sU32 startCol,sU32 endCol)
{
  GenTexture tex;

  tex.Init(2,1);
  tex.Data[0].Init(startCol);
  tex.Data[1].Init(endCol);

  return tex;
}

// Create a pattern of randomly colored voronoi cells
static void RandomVoronoi(GenTexture &dest,const GenTexture &grad,sInt intensity,sInt maxCount,sF32 minDist)
{
  sVERIFY(maxCount <= 256);
  CellCenter centers[256];

  // generate random center points
  for(sInt i=0;i<maxCount;i++)
  {
    int intens = (rand() * intensity) / RAND_MAX;

    centers[i].x = 1.0f * rand() / RAND_MAX;
    centers[i].y = 1.0f * rand() / RAND_MAX;
    centers[i].color.Init(intens,intens,intens,255);
  }

  // remove points too close together
  sF32 minDistSq = minDist*minDist;
  for(sInt i=1;i<maxCount;)
  {
    sF32 x = centers[i].x;
    sF32 y = centers[i].y;

    // try to find a point closer than minDist
    sInt j;
    for(j=0;j<i;j++)
    {
      sF32 dx = centers[j].x - x;
      sF32 dy = centers[j].y - y;

      if(dx < 0.0f) dx += 1.0f;
      if(dy < 0.0f) dy += 1.0f;

      dx = sMin(dx,1.0f-dx);
      dy = sMin(dy,1.0f-dy);

      if(dx*dx + dy*dy < minDistSq) // point is too close, stop
        break;
    }

    if(j<i) // we found such a point
      centers[i] = centers[--maxCount]; // remove this one
    else // accept this one
      i++;
  }

  // generate the image
  dest.Cells(grad,centers,maxCount,0.0f,GenTexture::CellInner);
}

// Transforms a grayscale image to a colored one with a matrix transform
static void Colorize(GenTexture &img,sU32 startCol,sU32 endCol)
{
  Matrix44 m;
  Pixel s,e;

  s.Init(startCol);
  e.Init(endCol);

  // calculate matrix
  sSetMem(m,0,sizeof(m));
  m[0][0] = (e.r - s.r) / 65535.0f;
  m[1][1] = (e.g - s.g) / 65535.0f;
  m[2][2] = (e.b - s.b) / 65535.0f;
  m[3][3] = 1.0f;
  m[0][3] = s.r / 65535.0f;
  m[1][3] = s.g / 65535.0f;
  m[2][3] = s.b / 65535.0f;

  // transform
  img.ColorMatrixTransform(img,m,sTRUE);
}

// Save an image as .TGA file
static bool SaveImage(GenTexture &img,const char *filename)
{
  FILE *f = fopen(filename,"wb");
  if(!f)
    return false;

  // prepare header
  sU8 buffer[18];
  sSetMem(buffer,0,sizeof(buffer));

  buffer[ 2] = 2;                 // image type code 2 (RGB, uncompressed)
  buffer[12] = img.XRes & 0xff;   // width (low byte)
  buffer[13] = img.XRes >> 8;     // width (high byte)
  buffer[14] = img.YRes & 0xff;   // height (low byte)
  buffer[15] = img.YRes >> 8;     // height (high byte)
  buffer[16] = 32;                // pixel size (bits)

  // write header
  if(!fwrite(buffer,sizeof(buffer),1,f))
  {
    fclose(f);
    return false;
  }

  // write image data
  sU8 *lineBuf = new sU8[img.XRes*4];
  for(sInt y=0;y<img.YRes;y++)
  {
    const Pixel *in = &img.Data[y*img.XRes];

    // convert a line of pixels (as simple as possible - no gamma correction etc.)
    for(sInt x=0;x<img.XRes;x++)
    {
      lineBuf[x*4+0] = in->b >> 8;
      lineBuf[x*4+1] = in->g >> 8;
      lineBuf[x*4+2] = in->r >> 8;
      lineBuf[x*4+3] = in->a >> 8;

      in++;
    }

    // write to file
    if(!fwrite(lineBuf,img.XRes*4,1,f))
    {
      fclose(f);
      return false;
    }
  }

  fclose(f);
  return true;
}

int main(int argc,char **argv)
{
  // initialize generator
  InitTexgen();

  // colors
  Pixel black,white;
  black.Init(0,0,0,255);
  white.Init(255,255,255,255);

  timeBeginPeriod(1);
  sInt startTime = timeGetTime();

  for(sInt i=0;i<100;i++)
  {
    // create gradients
    GenTexture gradBW = LinearGradient(0xff000000,0xffffffff);
    GenTexture gradWB = LinearGradient(0xffffffff,0xff000000);
    GenTexture gradWhite = LinearGradient(0xffffffff,0xffffffff);

    // simple noise test texture
    GenTexture noise;
    noise.Init(256,256);
    noise.Noise(gradBW,2,2,6,0.5f,123,GenTexture::NoiseDirect|GenTexture::NoiseBandlimit|GenTexture::NoiseNormalize);

    /*// save test image
    if(!SaveImage(noise,"noise.tga"))
    {
      printf("Couldn't write 'noise.tga'!\n");
      return 1;
    }*/

    // 4 "random voronoi" textures with different minimum distances
    GenTexture voro[4];
    static sInt voroIntens[4] = {     37,     42,     37,     37 };
    static sInt voroCount[4]  = {     90,    132,    240,    255 };
    static sF32 voroDist[4]   = { 0.125f, 0.063f, 0.063f, 0.063f };

    for(sInt i=0;i<4;i++)
    {
      voro[i].Init(256,256);
      RandomVoronoi(voro[i],gradWhite,voroIntens[i],voroCount[i],voroDist[i]);
    }

    // linear combination of them
    LinearInput inputs[4];
    for(sInt i=0;i<4;i++)
    {
      inputs[i].Tex = &voro[i];
      inputs[i].Weight = 1.5f;
      inputs[i].UShift = 0.0f;
      inputs[i].VShift = 0.0f;
      inputs[i].FilterMode = GenTexture::WrapU|GenTexture::WrapV|GenTexture::FilterNearest;
    }

    GenTexture baseTex;
    baseTex.Init(256,256);
    baseTex.LinearCombine(black,0.0f,inputs,4);

    // blur it
    baseTex.Blur(baseTex,0.0074f,0.0074f,1,GenTexture::WrapU|GenTexture::WrapV);

    // add a noise layer
    GenTexture noiseLayer;
    noiseLayer.Init(256,256);
    noiseLayer.Noise(LinearGradient(0xff000000,0xff646464),4,4,5,0.995f,3,GenTexture::NoiseDirect|GenTexture::NoiseNormalize|GenTexture::NoiseBandlimit);

    baseTex.Paste(baseTex,noiseLayer,0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,GenTexture::CombineAdd,0);

    // colorize it
    Colorize(baseTex,0xff747d8e,0xfff1feff);

    // Create transform matrix for grid pattern
    Matrix44 m1,m2,m3;
    MatTranslate(m1,-0.5f,-0.5f,0.0f);
    MatScale(m2,3.0f * sSQRT2F,3.0f * sSQRT2F,1.0f);
    MatMult(m3,m2,m1);
    MatRotateZ(m1,0.125f * sPI2F);
    MatMult(m2,m1,m3);
    MatTranslate(m1,0.5f,0.5f,0.0f);
    MatMult(m3,m1,m2);

    // Grid pattern GlowRect
    GenTexture rect1,rect1x,rect1n;
    rect1.Init(256,256);
    rect1.LinearCombine(black,1.0f,0,0); // black background
    rect1.GlowRect(rect1,gradWB,0.5f,0.5f,0.41f,0.0f,0.0f,0.25f,0.7805f,0.64f);

    rect1x.Init(256,256);
    rect1x.CoordMatrixTransform(rect1,m3,GenTexture::WrapU|GenTexture::WrapV|GenTexture::FilterBilinear);

    // Make a normalmap from it
    rect1n.Init(256,256);
    rect1n.Derive(rect1x,GenTexture::DeriveNormals,2.5f);

    // Apply as bump map
    GenTexture finalTex;
    Pixel amb,diff;

    finalTex.Init(256,256);
    amb.Init(0xff101010);
    diff.Init(0xffffffff);
    finalTex.Bump(baseTex,rect1n,0,0,0.0f,0.0f,0.0f,-2.518f,0.719f,-3.10f,amb,diff,sTRUE);

    // Second grid pattern GlowRect
    GenTexture rect2,rect2x;
    rect2.Init(256,256);
    rect2.LinearCombine(white,1.0f,0,0); // white background
    rect2.GlowRect(rect2,gradBW,0.5f,0.5f,0.36f,0.0f,0.0f,0.20f,0.8805f,0.74f);

    rect2x.Init(256,256);
    rect2x.CoordMatrixTransform(rect2,m3,GenTexture::WrapU|GenTexture::WrapV|GenTexture::FilterBilinear);

    // Multiply it over
    finalTex.Paste(finalTex,rect2x,0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,GenTexture::CombineMultiply,0);
  }

  sInt totalTime = timeGetTime() - startTime;
  timeEndPeriod(1);

  printf("%d ms/tex\n",totalTime / 100);

  /*SaveImage(baseTex,"baseTex.tga");
  SaveImage(finalTex,"final.tga");*/

  return 0;
}
