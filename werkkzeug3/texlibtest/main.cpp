// This file is distributed under a BSD license. See LICENSE.txt for details.

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "w3texlib.hpp"

// ---- Helper routines

// Exit with a given error message.
static void errorExit(const char *format,...)
{
  va_list arg;

  va_start(arg,format);
  vprintf_s(format,arg);
  va_end(arg);

  exit(1);
}

// Load a file and return it as unsigned char *.
static unsigned char *loadFile(const char *name)
{
  FILE *f;
  unsigned char *buffer;
  long size;

  f = fopen(name,"rb");
  if(!f)
    return 0;
  
  fseek(f,0,SEEK_END);
  size = ftell(f);
  fseek(f,0,SEEK_SET);

  buffer = new unsigned char[size];
  fread(buffer,1,size,f);

  fclose(f);

  return buffer;
}

// Emits a 16-bit short (little endian)
static void emitShort(unsigned char *dest,int value)
{
  dest[0] = value & 0xff;
  dest[1] = value >> 8;
}

// Emits a 32-bit long (little endian)
static void emitLong(unsigned char *dest,long value)
{
  dest[0] = (unsigned char) ((value >>  0) & 0xff);
  dest[1] = (unsigned char) ((value >>  8) & 0xff);
  dest[2] = (unsigned char) ((value >> 16) & 0xff);
  dest[3] = (unsigned char) ((value >> 24) & 0xff);
}

// Emits a FOURCC
static void emitFourCC(unsigned char *dest,const char *value)
{
  dest[0] = value[0];
  dest[1] = value[1];
  dest[2] = value[2];
  dest[3] = value[3];
}

// ---- TGA file writer

// Save a block of BGRA pixel data as .TGA file
static bool saveImageDataTGA(const unsigned char *imgData,int xSize,int ySize,const char *filename)
{
  FILE *f = fopen(filename,"wb");
  if(!f)
    return false;

  // prepare header
  unsigned char buffer[18];
  memset(buffer,0,sizeof(buffer));

  buffer[ 2] = 2;                   // image type code 2 (RGB, uncompressed)
  emitShort(&buffer[12],xSize);     // width
  emitShort(&buffer[14],ySize);     // height
  buffer[16] = 32;                  // pixel size (bits)

  // write header
  if(!fwrite(buffer,sizeof(buffer),1,f))
  {
    fclose(f);
    return false;
  }

  // write image data
  for(int y=0;y<ySize;y++)
  {
    const unsigned char *lineBuf = &imgData[(ySize-1-y)*xSize*4];

    // write to file
    if(!fwrite(lineBuf,xSize*4,1,f))
    {
      fclose(f);
      return false;
    }
  }

  // write TGA file footer
  unsigned char footer[26];
  memset(footer,0,sizeof(footer));
  memcpy(&footer[8],"TRUEVISION-XFILE.\0",18);

  if(!fwrite(footer,sizeof(footer),1,f))
  {
    fclose(f);
    return false;
  }

  fclose(f);
  return true;
}

// ---- DDS file writer

// Save a W3Image as .DDS file
static bool saveImageDDS(const W3Image *image,const char *filename)
{
  if(image->Format == W3IF_NONE) // can't save images with unknown pixel format
    return false;

  FILE *f = fopen(filename,"wb");
  if(!f)
    return false;

  bool isRGB = (image->Format == W3IF_BGRA8 || image->Format == W3IF_UVWQ8);

  // prepare header
  unsigned char buffer[128];
  memset(buffer,0,sizeof(buffer));

  emitFourCC(&buffer[0],"DDS ");    // magic value
  emitLong(&buffer[4],124);         // DDSURFACEDESC2.dwSize

  if(isRGB)
  {
    // dwFlags = CAPS|HEIGHT|WIDTH|PITCH|PIXELFORMAT|MIPMAPCOUNT
    emitLong(&buffer[8],0x2100f);
  }
  else // DXT
  {
    // dwFlags = CAPS|HEIGHT|WIDTH|PIXELFORMAT|MIPMAPCOUNT|LINEARSIZE
    emitLong(&buffer[8],0xa1007);
  }

  emitLong(&buffer[12],image->YSize);   // dwHeight
  emitLong(&buffer[16],image->XSize);   // dwWidth
  emitLong(&buffer[20],isRGB ? image->XSize * 4 : image->MipSize[0]); // lPitch/dwLinearSize
  emitLong(&buffer[28],image->MipLevels); // dwMipMapCount

  // pixel format
  emitLong(&buffer[76],32);             // dwSize

  if(image->Format == W3IF_BGRA8)
    emitLong(&buffer[80],0x41);         // dwFlags: RGB|ALPHAPIXELS
  else if(image->Format == W3IF_UVWQ8)
    emitLong(&buffer[80],0x80000);      // dwFlags: BUMPDUDV
  else
    emitLong(&buffer[80],0x04);         // dwFlags: FOURCC

  if(isRGB)
  {
    emitLong(&buffer[ 88],32);          // dwRGBBitCount
    emitLong(&buffer[ 92],0x00ff0000);  // dwRBitMask
    emitLong(&buffer[ 96],0x0000ff00);  // dwGBitMask
    emitLong(&buffer[100],0x000000ff);  // dwBBitMask
    emitLong(&buffer[104],0xff000000);  // dwRGBAlphaBitMask
  }
  else
  {
    // dwFourCC
    emitFourCC(&buffer[84],(image->Format == W3IF_DXT5) ? "DXT5" : "DXT1");
  }

  // caps
  emitLong(&buffer[108],0x401008);      // dwCaps1=COMPLEX|TEXTURE|MIPMAP

  // write header
  if(!fwrite(buffer,sizeof(buffer),1,f))
  {
    fclose(f);
    return false;
  }

  // write data for mip levels
  for(int level=0;level<image->MipLevels;level++)
  {
    if(!fwrite(image->MipData[level],image->MipSize[level],1,f))
    {
      fclose(f);
      return false;
    }
  }

  fclose(f);
  return true;
}

// ---- Actual program

// Calc callback
static bool CalcCallback(int current,int max)
{
  printf("\rcalc... %3d%%",current*100/max);
  return true; // return true unless you wish to abort calculation
}

#pragma comment(lib, "winmm.lib")
extern "C" void __stdcall timeBeginPeriod(int period);
extern "C" void __stdcall timeEndPeriod(int period);
extern "C" int __stdcall timeGetTime();

int main()
{
  //static const char fileName[] = "example.ktx";
  //static const char fileName[] = "../data/debris_9241_tex.ktx";
  //static const char fileName[] = "../data/default.ktx";
  static const char fileName[] = "../data/crash/stripe_antipholus.ktx";

  W3TextureContext context;
  unsigned char *data;

  // load demo file
  if(!(data = loadFile(fileName)))
    errorExit("couldn't load data file '%s'!\n",fileName);

  if(!context.Load(data))
    errorExit("error in data file '%s'!\n",fileName);

  // show file contents
  for(int i=0;i<context.GetImageCount();i++)
    printf("%3d. %s\n",i+1,context.GetImageName(i));

  timeBeginPeriod(1);
  int startTime = timeGetTime();

  // calculate everything
  if(!context.Calculate(CalcCallback))
    errorExit("\nerror during calc.\n");

  printf("\ncalc time: %d ms",timeGetTime() - startTime);
  timeEndPeriod(1);

  printf("\n");

  // frees the document and associated data (only final images are left afterwards)
  context.FreeDocument();

  // save images
  printf("saving images...\n");

  for(int i=0;i<context.GetImageCount();i++)
  {
    const W3Image *img = context.GetImageByNumber(i);
    char namebuf[512];

    sprintf(namebuf,"%s.dds",context.GetImageName(i));
    saveImageDDS(img,namebuf);

    //sprintf(namebuf,"%s.tga",context.GetImageName(i));
    //saveImageDataTGA(img->MipData[0],img->XSize,img->YSize,namebuf);

    printf("%3d. %s\n",i+1,namebuf);
  }

  delete[] data;

  return 0;
}
