/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "codec_old.hpp"
#include "base/graphics.hpp"

/****************************************************************************/
// DXT1/DXT3/DXT5 Texture Compression
/****************************************************************************/


#define BITMAP_ARGB8888  1       // 32 bit true color
#define BITMAP_DXT1     12       // DXT1 = Compressed, 1-bit alpha
#define BITMAP_DXT3     13       // DXT3 = Compressed, 4-bit nonpremultiplied alpha
#define BITMAP_DXT5     14       // DXT5 = Compressed, interpolated nonpremultiplied alpha


inline sInt sRange(sInt val,sInt max,sInt min) { return sClamp(val,min,max); }

class OldBitmap
{
public:

  // DXT-Texture-compression
  sU32 inline Error3(const sU8 *buffer,const sU32 color1,const sU32 color2);
  sU32 inline Error4(const sU8 *buffer,const sU32 color1,const sU32 color2);
  void inline Encode3(sU16 *d,const sU8 *buffer,const sU32 color1,const sU32 color2);
  void inline Encode4(sU16 *d,const sU8 *buffer,const sU32 color1,const sU32 color2);
  void CompressTexture(const OldBitmap *src,sU32 format,sBool alpha);
  void inline DXT1_Decompress(sU32 *dst, const sU32 pitch,const sU16 *src);
  void inline DXT3_Decompress(sU32 *dst, const sU32 pitch,const sU16 *src);
  void inline DXT5_Decompress(sU32 *dst, const sU32 pitch,const sU16 *src);
  void DecompressTexture(const OldBitmap *src);

// constructor, destructor, etc

  OldBitmap() {}
  ~OldBitmap() {}

// bitmap content  
  
  sInt Kind;                      // sBITMAP_??? kind
  sS32 XSize;                     // Width
  sS32 YSize;                     // Height
  sInt Mipmaps;                   // number of mipmaps. 1 = no mipmaps, >1 = as countet, 0 = please create automatically (only for RGB8888, ARGB1555 and RGB565)
  sInt BPR;                       // Bytes per Row
//  sInt Size;                      // bytes allocated for all mipmaps
  sU8 *Data;                      // pointer to pixels
//  sU32 Flags;
};


/****************************************************************************/

static sU8 disttab[512];

const unsigned long Mask0565 = 0x00f8fcf8;
const unsigned long Mask1565 = 0x80f8fcf8;

/****************************************************************************/

sU32 inline OldBitmap::Error3(const sU8 *buffer,const sU32 color1,const sU32 color2)
{
  union {
    struct
    {
      sU8 b,g,r,a;
    };
    sU32 dw;
  } color[3];         // Color gradient
  const sU8 *c1,*c2;  // Pointers to color1&2
  sInt i,j;
  sU32 minerror;
  sU32 dr,dg,db;
  sU32 error;
  sU32 result = 0;// Sum up errors

  color[0].dw = color1;
  color[1].dw = color2;
  c1 = (sU8*)&color1;
  c2 = (sU8*)&color2;
  color[2].r = ((sU32)c1[2]+(sU32)c2[2])/2;
  color[2].g = ((sU32)c1[1]+(sU32)c2[1])/2;
  color[2].b = ((sU32)c1[0]+(sU32)c2[0])/2;
  for (i = 0;i < 16;i++)
  {
    // Find best match for this color
    minerror = 0x7FFFFFFF;
    for (j = 0;j < 3;j++)
    {
      dr = disttab[256+buffer[2]-color[j].r];
      dg = disttab[256+buffer[1]-color[j].g];
      db = disttab[256+buffer[0]-color[j].b];
      error = dr*dr+dg*dg+db*db;
      //error = dr+dg+db;
      if (error < minerror)
        minerror = error;
    }
    // Sum up error
    result += minerror;
    buffer+=4;
  }
  return result;
}

/****************************************************************************/

sU32 inline OldBitmap::Error4(const sU8 *buffer,const sU32 color1,const sU32 color2)
{
  union {    
    struct
    {
      sU8 b,g,r,a;
    };
    sU32 dw;
  } color[4];     // Color gradient
  const sU8 *c1,*c2;    // Pointers to color1&2  
  sInt i,j;
  sU32 minerror;
  sU32 dr,dg,db;
  sU32 error; 
  sU32 result = 0;// Sum up errors

  color[0].dw = color1;
  color[1].dw = color2;
  c1 = (sU8*)&color1;
  c2 = (sU8*)&color2;
  color[2].r = (2*(sU32)c1[2]+  (sU32)c2[2])/3;
  color[2].g = (2*(sU32)c1[1]+  (sU32)c2[1])/3;
  color[2].b = (2*(sU32)c1[0]+  (sU32)c2[0])/3;
  color[3].r = (  (sU32)c1[2]+2*(sU32)c2[2])/3;
  color[3].g = (  (sU32)c1[1]+2*(sU32)c2[1])/3;
  color[3].b = (  (sU32)c1[0]+2*(sU32)c2[0])/3;
  for (i = 0;i < 16;i++)
  {
    // Find best match for this color
    minerror = 0x7FFFFFFF;
    for (j = 0;j < 4;j++)
    {
      dr = disttab[256+buffer[2]-color[j].r];
      dg = disttab[256+buffer[1]-color[j].g];
      db = disttab[256+buffer[0]-color[j].b];
      error = dr*dr+dg*dg+db*db;
      //error = dr+dg+db;
      if (error < minerror)
        minerror = error;
    }
    // Sum up error
    result += minerror;
    buffer+=4;
  }
  return result;
}

/****************************************************************************/

void inline OldBitmap::Encode3(sU16 *d,const sU8 *buffer,const sU32 color1,const sU32 color2)
{
  // Encode DXT1-block
  union {    
    struct
    {
      sU8 b,g,r,a;
    };
    sU32 dw;
  } color[3];     // Color gradient
  sU8 *p,*q;

  // Encode border-colors
  d[0] = ((color1&0xF80000)>>8)+((color1&0xFC00)>>5)+((color1&0xF8)>>3);
  d[1] = ((color2&0xF80000)>>8)+((color2&0xFC00)>>5)+((color2&0xF8)>>3);
  // Build gradient between border-colors
  color[0].dw = color1;
  color[1].dw = color2;
  p = (sU8*)&color1;
  q = (sU8*)&color2;
  color[2].r = ((sU32)p[2]+  (sU32)q[2])/2;
  color[2].g = ((sU32)p[1]+  (sU32)q[1])/2;
  color[2].b = ((sU32)p[0]+  (sU32)q[0])/2;
  // Encode source-colors
  sU32 minindex;
  sInt i,j;
  sU32 error,minerror;
  sU32 dr,dg,db;
  sU32 shift = 0;

  q = (sU8*)buffer;
  *((sU32*)&d[2]) = 0;
  for (i = 0;i < 16;i++)
  {
    if (q[3]&0x80)
    {
      // Find best match for this color
      minerror = 0x7FFFFFFF;
      for (j = 0;j < 3;j++)
      {
        dr = disttab[256+q[2]-color[j].r];
        dg = disttab[256+q[1]-color[j].g];
        db = disttab[256+q[0]-color[j].b];
        error = dr*dr+dg*dg+db*db;
        if (error < minerror)
        {
          minerror = error;
          minindex = j;
        }
      }
    }
    else
    {
      minindex = 3;
    }
    // Encode index         
    *((sU32*)&d[2]) += minindex<<shift;
    shift += 2;
    q+=4;
  }      
}

/****************************************************************************/

void inline OldBitmap::Encode4(sU16 *d,const sU8 *buffer,const sU32 color1,const sU32 color2)
{
  // Encode DXT1-block
  union {    
    struct
    {
      sU8 b,g,r,a;
    };
    sU32 dw;
  } color[4];     // Color gradient
  sU8 *p,*q;

  // Encode border-colors
  d[0] = ((color1&0xF80000)>>8)+((color1&0xFC00)>>5)+((color1&0xF8)>>3);
  d[1] = ((color2&0xF80000)>>8)+((color2&0xFC00)>>5)+((color2&0xF8)>>3);
  // Build gradient between border-colors
  color[0].dw = color1;
  color[1].dw = color2;
  p = (sU8*)&color1;
  q = (sU8*)&color2;
  color[2].r = (2*(sU32)p[2]+  (sU32)q[2])/3;
  color[2].g = (2*(sU32)p[1]+  (sU32)q[1])/3;
  color[2].b = (2*(sU32)p[0]+  (sU32)q[0])/3;
  color[3].r = (  (sU32)p[2]+2*(sU32)q[2])/3;
  color[3].g = (  (sU32)p[1]+2*(sU32)q[1])/3;
  color[3].b = (  (sU32)p[0]+2*(sU32)q[0])/3;
  // Encode source-colors
  sU32 minindex;
  sInt i,j;
  sU32 error,minerror;
  sU32 dr,dg,db;
  sU32 shift = 0;

  q = (sU8*)buffer;
  *((sU32*)&d[2]) = 0;
  for (i = 0;i < 16;i++)
  {
    // Find best match for this color
    minerror = 0x7FFFFFFF;
    for (j = 0;j < 4;j++)
    {
      dr = disttab[256+q[2]-color[j].r];
      dg = disttab[256+q[1]-color[j].g];
      db = disttab[256+q[0]-color[j].b];
      error = dr*dr+dg*dg+db*db;
      if (error < minerror)
      {
        minerror = error;
        minindex = j;
      }
    }
    // Encode index         
    *((sU32*)&d[2]) += minindex<<shift;
    shift += 2;
    q+=4;
  }      
}

/****************************************************************************/

// If you want to tune the quality of this algorithm even more, be VERY
// careful! The smallest change can decrease image quality or cause
// color glitches. Always compare resulting compressed images with the original
// image and the result of the NVidia compression tool (nvdxt.exe).
// A good way to test for color variations is to put the compressed image
// in a layer over the original image and then subtract the layers.
// After that increase brightness/contrast to make variations visible.
// I used Paintshop Pro 70, Adobe Photoshop should do it too.

void OldBitmap::CompressTexture(const OldBitmap *src,sU32 format,sBool alpha)
{
  static sBool first = sTRUE;
  OldBitmap *dst;
  sBool opaque;
  sInt i,j;
  sInt levels;      // Current MipMap-level
  sInt sizeX,sizeY; // Current MipMap-size
  sU32 levelOfsSrc; // Offset into Data of current MipMap (in bytes)
  sU32 levelOfsDst;
  sInt x,y;
  sU32 color1,color2; // Final pair of colors
  sU32 c1,c2;         // Temporary pair of colors
  sU32 *s;    // Source-pointer
  sU16 *d;    // Destination-pointer
  sU32 dOfs;  // Offset between DXT-blocks in 16-bit words, 4 words for DXT1, 8 words for DXT3 and DXT5
  sU8 *p,*q;  // Multi-purpose byte-pointers
  sU32 sbuf888[16]; // 4x4 Source block serialized into 16 double-words
  sU32 sbuf565[16]; // 888 correctly rounded to (1)565 but each channel still in one byte!
  sU32 error,minerror;

  sVERIFY(src);
  sVERIFY((src->Kind == BITMAP_ARGB8888) && (src->XSize%4 == 0) && (src->YSize%4 == 0));
  sVERIFY(format == BITMAP_DXT1 || format == BITMAP_DXT3 || format == BITMAP_DXT5);  

  if (format == BITMAP_DXT1)
  {
    dOfs = 4;       // With DXT1 each 4x4 pixel block is compressed into 64bit
  }
  else 
  {
    dOfs = 8;
    alpha = false;  // Only DXT1- mixes color and alpha
  }

  dst = this;

/*  switch(format)
  {
  case BITMAP_DXT1:
    dst->Init(BITMAP_DXT1,src->XSize,src->YSize,Mipmaps);
    break;
  case BITMAP_DXT3:
    dst->Init(BITMAP_DXT3,src->XSize,src->YSize,Mipmaps);
    break;
  case BITMAP_DXT5:
    dst->Init(BITMAP_DXT5,src->XSize,src->YSize,Mipmaps);
    break;
  }*/

  if (first)
  {
    for(i=0;i<512;i++)
      disttab[i] = sMin(255,sAbs(256-i));
    first = sFALSE;
  }
  levels = 0;   // Start with full-size
  sizeX = XSize;
  sizeY = YSize;
  levelOfsSrc = 0;
  levelOfsDst = 0;
  do
  {
    sVERIFY((sizeX%4 == 0) && (sizeY%4 == 0));
    // Take 4x4 pixel blocks from source-image and compress them into destination-image.
    for (y = 0;y <  sizeY/4;y++)
    {
      s = (sU32*)(((sU8*)src->Data)+levelOfsSrc+y*4*(sizeX*4)); // sizeX*4 -> Bytes-per-row
      d = (sU16*)(((sU8*)dst->Data)+levelOfsDst+y*sizeX*(dOfs/2))+(dOfs-4); // DXT1-colors are encoded into second 64-bit block
      for (x = 0;x < sizeX/4;x++)
      {      
        // Serialize 4x4 pixel-block
        for (i = 0;i < 4;i++)
        {
          sCopyMem(&sbuf888[i*4],s,4*4);
          s += sizeX;//src->BPR/4;
        }
        s -= sizeX*4;//src->BPR;
        // Make a 565-copy of serialized block
        p = (sU8*)sbuf888;      
        if (alpha)
        {
          opaque = true;
          for (i = 0;i < 16;i++)    // One bit alpha for DXT1
          {
            if (!((sbuf565[i] = sbuf888[i] & Mask1565)&0x80000000))
              opaque = false;
            p += 4;
          }
        }
        else
        {
          opaque = true;
          for (i = 0;i < 16;i++)    // Opaque DXT1 or DXT3/DXT5 with alpha
          {
            sbuf565[i] = sbuf888[i] & Mask0565;
            p += 4;          
          }
          // Convert source-alpha into 4-bit nonpremultiplied alpha for DXT3
          if (format == BITMAP_DXT3)
          {
            sInt shift;

            d -= 4;
            for (j = 0;j < 4;j++)
            {
              shift = 28;
              *d = 0;
              for (i = 0;i < 4;i++)
              {
                *d += (sbuf888[i+4*j]&0xF0000000)>>shift;
                shift-=4;
              }
              d++;
            }
          }
          // Find min/max source-alpha to encode interpolated nonpremultiplied alpha for DXT5.
          // This part caused a lot of problems and therefore the implementation has finally 
          // become a bit "lengthy" :-) Each particular case is explicitly taken car of.
          if (format == BITMAP_DXT5)
          {
            sU32 ablend[8];
            sU64 bitcode[8];
            sU32 a,amin = 0xFF,amax = 0x00;          
            sU64 *dblock;
            sBool maxmin = false,maxmax = false;;
            sU64 shift;
            sInt minerror,error;
            sInt c,min;
        
            for (i = 0;i < 16;i++)
            {
              a = ((sU8*)&sbuf888[i])[3];
              if (a == 0x00)
                maxmin = true;
              else
              if (a == 0xFF)
                maxmax = true;
              else
              {
                if (a < amin)
                  amin = a;
                if (a > amax)
                  amax = a;
              }
            }
            if (amin > amax)
            {
              if (maxmin && (!maxmax))
              {
                amin = amax = 0;
              }
              else
              if ((!maxmin) && maxmax)
              {
                amin = amax = 0xFF;
              }
              else           
              {
                amin = 0;amax = 0xFF;
              }
              maxmin = maxmax = false;
            }
            d -= 4;
            sSetMem(d,0,8);
            dblock = (sU64*)d;          
            if (maxmin && maxmax)
            {
              *d = amin+(amax<<8);
              ablend[0] = 0x00;             bitcode[0] = 0x06;
              ablend[1] = amin;             bitcode[1] = 0x00;
              ablend[2] = (4*amin+1*amax)/5;bitcode[2] = 0x02;
              ablend[3] = (3*amin+2*amax)/5;bitcode[3] = 0x03;
              ablend[4] = (2*amin+3*amax)/5;bitcode[4] = 0x04;
              ablend[5] = (1*amin+4*amax)/5;bitcode[5] = 0x05;
              ablend[6] = amax;             bitcode[6] = 0x01;
              ablend[7] = 0xFF;             bitcode[7] = 0x07;          
            }
            else
            {
              if (maxmin) amin = 0x00;            
              if (maxmax) amax = 0xFF;
              *d = amax+(amin<<8);
              ablend[0] = amin;             bitcode[0] = 0x01;
              ablend[1] = (6*amin+1*amax)/7;bitcode[1] = 0x07;
              ablend[2] = (5*amin+2*amax)/7;bitcode[2] = 0x06;
              ablend[3] = (4*amin+3*amax)/7;bitcode[3] = 0x05;
              ablend[4] = (3*amin+4*amax)/7;bitcode[4] = 0x04;
              ablend[5] = (2*amin+5*amax)/7;bitcode[5] = 0x03;
              ablend[6] = (1*amin+6*amax)/7;bitcode[6] = 0x02;
              ablend[7] = amax;             bitcode[7] = 0x00;
            }
            shift = 16;
            for (j = 0;j < 4;j++)
            {
              for (i = 0;i < 4;i++)
              {
                minerror = 0xFFF; 
                a = ((sU8*)&sbuf888[i+j*4])[3];
                for (c = 0;c < 8;c++)
                {
                  if ((error = disttab[256+a-ablend[c]]) <= minerror)
                  {
                    minerror = error;
                    min = c;
                  }
                  else
                  {                    
                    break;
                  }
                }
                *dblock += (bitcode[min])<<shift;
                shift += 3;
              }
            }
            d += 4;
          }
        }
        // Find extremes by trying all possible pairs of colors
        color1 = color2 = sbuf565[0];
        if (opaque)
          minerror = Error4((sU8*)sbuf888,color1,color2);
        else
          minerror = Error3((sU8*)sbuf888,color1,color2);
        for (i = 0;i < 15;i++)
        {
          c1 = sbuf565[i];
          c1 = (c1&0xF8FCF8) + ((c1&0xE000E0)>>5) + ((c1&0x00C000)>>6);
          for (j = i+1;j < 16;j++)
          {
            c2 = sbuf565[j];       
            c2 = (c2&0xF8FCF8) + ((c2&0xE000E0)>>5) + ((c2&0x00C000)>>6);
            if ( ( opaque && ((error = Error4((sU8*)sbuf888,c1,c2)) < minerror))
              || (!opaque && ((error = Error3((sU8*)sbuf888,c1,c2)) < minerror))
              )
            {
              minerror = error;
              color1 = c1;
              color2 = c2;
            }
          }
        }
        // Put variance on colors to reduce error some more
        sInt c;
        sInt start1,start2,end1,end2;      
        sInt scanstep,scanrange;

        p = (sU8*)&c1;
        q = (sU8*)&c2;
        for (c = 0;c < 3;c++)
        {
          scanstep  = 4;
          scanrange = 16;
          c1 = color1;
          c2 = color2;
          start1  = *p-scanrange/2;start1 = sRange(start1,0xFF,0x00);
          end1    = start1+scanrange;end1 = sRange(  end1,0xFF,0x00);
          start2  = *q-scanrange/2;start2 = sRange(start2,0xFF,0x00);
          end2    = start2+scanrange;end2 = sRange(  end2,0xFF,0x00);
          for (i = start1;i <= end1;i+=scanstep)
          {
            *p = i;
            for (j = start2;j <= end2;j+=scanstep)
            {
              *q = j;
              if (c1 != c2)
              {
                c1 = (c1&0xF8FCF8) + ((c1&0xE000E0)>>5) + ((c1&0x00C000)>>6);
                c2 = (c2&0xF8FCF8) + ((c2&0xE000E0)>>5) + ((c2&0x00C000)>>6);              
                if ( ( opaque && (error = Error4((sU8*)sbuf888,c1,c2)) < minerror)
                  || (!opaque && (error = Error3((sU8*)sbuf888,c1,c2)) < minerror)
                  )
                {
                  minerror = error;
                  ((sU8*)&color1)[c] = *p;
                  ((sU8*)&color2)[c] = *q;
                }
              }
            }
          }
          p++;q++;
        }
        // Sort final color pair
        if (  opaque && color1 < color2
          || !opaque && color1 > color2
        )
          sSwap(color1,color2);
        else
        if (opaque && color1 == color2)
          color2 = 0;

        // Encode DXT-block
        if (opaque)
          Encode4(d,(sU8*)sbuf888,color1,color2);
        else
          Encode3(d,(sU8*)sbuf888,color1,color2);

        s+=4;
        d+=dOfs;
      }
    }
    levels++;
    levelOfsSrc += sizeX*sizeY*4;  // Always 4 byte-per-pixel, we only compress ARGB8888
    switch(format)
    {
    case BITMAP_DXT1:
      levelOfsDst += sizeX*sizeY/2;      
      break;
    case BITMAP_DXT3:
    case BITMAP_DXT5:
      levelOfsDst += sizeX*sizeY;
      break;
    }    
    sizeX /= 2;
    sizeY /= 2;
  } while (levels < Mipmaps);  
/*  Copy(dst);
  dst->Release();
  dst = sNULL;*/
}

/****************************************************************************/
// DXT1/DXT3/DX5 Texture Decompression
/****************************************************************************/

void inline OldBitmap::DXT1_Decompress(sU32 *dst, const sU32 pitch,const sU16 *src)
{
  union {    
    struct
    {
      sU8 b,g,r,a;
    };
    sU32 dw;
  } color[4]; // colors 1-4
  sInt x,y;
  sU32 mask = 0x03;
  sU32 shift = 0;
  sBool opaque = src[0]>src[1];

  color[0].b = ((sU32)(src[0]&0x001F)<<3)+((sU32)(src[0]&0x001C)>>2);
  color[0].g = ((sU32)(src[0]&0x07E0)>>3)+((sU32)(src[0]&0x0600)>>9);
  color[0].r = ((sU32)(src[0]&0xF800)>>8)+((sU32)(src[0]&0xE000)>>13);
  color[0].a = 0xFF;
  color[1].b = ((sU32)(src[1]&0x001F)<<3)+((sU32)(src[1]&0x001C)>>2);
  color[1].g = ((sU32)(src[1]&0x07E0)>>3)+((sU32)(src[1]&0x0600)>>9);
  color[1].r = ((sU32)(src[1]&0xF800)>>8)+((sU32)(src[1]&0xE000)>>13);  
  color[1].a = 0xFF;
  if (opaque)
  {
    color[2].r = (2*(sU32)color[0].r+  (sU32)color[1].r)/3;
    color[2].g = (2*(sU32)color[0].g+  (sU32)color[1].g)/3;
    color[2].b = (2*(sU32)color[0].b+  (sU32)color[1].b)/3;
    color[2].a = 0xFF;
    color[3].r = (  (sU32)color[0].r+2*(sU32)color[1].r)/3;
    color[3].g = (  (sU32)color[0].g+2*(sU32)color[1].g)/3;
    color[3].b = (  (sU32)color[0].b+2*(sU32)color[1].b)/3;
    color[3].a = 0xFF;
  }
  else
  {
    color[2].r = ((sU32)color[0].r+  (sU32)color[1].r)/2;
    color[2].g = ((sU32)color[0].g+  (sU32)color[1].g)/2;
    color[2].b = ((sU32)color[0].b+  (sU32)color[1].b)/2;
    color[2].a = 0xFF;
    color[3].r = 0x00;
    color[3].g = 0x00;
    color[3].b = 0x00;
    color[3].a = 0x00;
  }  
  for (y = 0;y < 4;y++)
  {    
    for (x = 0;x < 4;x++)
    {      
      *dst = color[((((sU32*)src)[1])&mask)>>shift].dw;
      mask <<= 2;
      shift += 2;
      dst++;
    }
    dst += pitch-4;
  }
}

/****************************************************************************/

void inline OldBitmap::DXT3_Decompress(sU32 *dst, const sU32 pitch,const sU16 *src)
{
  sInt x,y;
  sInt shift;
  sU32 a;

  for (y = 0;y < 4;y++)
  {   
    shift = 0;
    for (x = 0;x < 4;x++)
    {    
      a = ((sU32)(*src)&(0x0F<<shift))<<(28-shift);      
      *dst = (*dst&0x00FFFFFF)+a+(a>>4);
      shift+=4;
      dst++;
    }
    dst += pitch-4;
    src++;
  }
}

/****************************************************************************/

void inline OldBitmap::DXT5_Decompress(sU32 *dst, const sU32 pitch,const sU16 *src)
{
  sU32 ablend[8];
  sInt x,y;
  sU32 a,a1,a2;
  sU64 shift;
  sU64 *dblock;

  a1 = ((sU8*)src)[0];
  a2 = ((sU8*)src)[1];
  ablend[0] = a1;
  ablend[1] = a2;
  if (a1 > a2)
  {
    ablend[2] = (6*a1+1*a2)/7;
    ablend[3] = (5*a1+2*a2)/7;
    ablend[4] = (4*a1+3*a2)/7;
    ablend[5] = (3*a1+4*a2)/7;
    ablend[6] = (2*a1+5*a2)/7;
    ablend[7] = (1*a1+6*a2)/7;
  }
  else
  {
    ablend[2] = (4*a1+1*a2)/5;
    ablend[3] = (3*a1+2*a2)/5;
    ablend[4] = (2*a1+3*a2)/5;
    ablend[5] = (1*a1+4*a2)/5;
    ablend[6] = 0x00;
    ablend[7] = 0xFF;
  }
  dblock = (sU64*)src;
  shift = 16;
  for (y = 0;y < 4;y++)
  {    
    for (x = 0;x < 4;x++)
    {    
      a = (*dblock&(((sU64)0x07)<<shift))>>shift;      
      ((sU8*)dst)[3] = ablend[a];
      shift+=3;
      dst++;
    }
    dst += pitch-4;    
  }
}

/****************************************************************************/

void OldBitmap::DecompressTexture(const OldBitmap *src)
{
  OldBitmap *dst;
  sInt levels;      // Current MipMap-level
  sInt sizeX,sizeY; // Current MipMap-size
  sU32 levelOfsSrc; // Offset into Data of current MipMap (in bytes)
  sU32 levelOfsDst;
  sInt x,y;
  sU16 *s;
  sU32 *d;
  sU32 sOfs;

  sVERIFY(src);
  sVERIFY((src->XSize%4 == 0) && (src->YSize%4 == 0));

  switch(src->Kind)
  {
  case BITMAP_DXT1:  
    sOfs = 4;
    break;
  case BITMAP_DXT3:
  case BITMAP_DXT5:  
    sOfs = 8;
    break;
  default:
    sVERIFY(0);
    break;
  }

  dst = this;
  /*dst = new OldBitmap;
  dst->Init(BITMAP_ARGB8888,src->XSize,src->YSize,Mipmaps);*/

  levels = 0;
  sizeX = XSize;
  sizeY = YSize;
  levelOfsSrc = 0;
  levelOfsDst = 0;
  do
  {
    sVERIFY((sizeX%4 == 0) && (sizeY%4 == 0));
    for (y = 0;y < sizeY/4;y++)
    {
      s = (sU16*)(((sU8*)src->Data)+levelOfsSrc+y*sizeX*(sOfs/2))+(sOfs-4);
      d = (sU32*)(((sU8*)dst->Data)+levelOfsDst+y*4*(sizeX*4)); // sizeX*4 -> Bytes-per-row
      for (x = 0;x < sizeX/4;x++)
      {
        if (x == 10 && y == 8)
          x = x;
        DXT1_Decompress(d,sizeX/*dst->BPR/4*/,s);
        switch(src->Kind)
        {
          case BITMAP_DXT3:
            DXT3_Decompress(d,sizeX/*dst->BPR/4*/,s-4);
            break;
          case BITMAP_DXT5:
            DXT5_Decompress(d,sizeX/*dst->BPR/4*/,s-4);
            break;
        }
        s+=sOfs;
        d+=4;
      }
    }
    levels++;
    levelOfsDst += sizeX*sizeY*4;  // Always 4 byte-per-pixel, we only decompress to ARGB8888
    switch(src->Kind)
    {
    case BITMAP_DXT1:
      levelOfsSrc += sizeX*sizeY/2;      
      break;
    case BITMAP_DXT3:
    case BITMAP_DXT5:
      levelOfsSrc += sizeX*sizeY;
      break;
    }    
    sizeX /= 2;
    sizeY /= 2;
  } while (levels < Mipmaps);
/*  Copy(dst);
  dst->Release();
  dst = sNULL;*/
}

/****************************************************************************/
/****************************************************************************/

CodecOld::CodecOld()
{
}

CodecOld::~CodecOld()
{
}

const sChar *CodecOld::GetName()
{
  return L"old";
}

/****************************************************************************/

void CodecOld::Pack(sImage *bmp,sImageData *dxt,sInt level)
{
  OldBitmap _bmp,_dxt;
  sInt mode;

  _bmp.Kind = BITMAP_ARGB8888;
  _bmp.XSize = bmp->SizeX;
  _bmp.YSize = bmp->SizeY;
  _bmp.Mipmaps = 1;
  _bmp.BPR = bmp->SizeX*4;
  _bmp.Data = (sU8 *) bmp->Data;

  switch(level)
  {
  case sTEX_DXT1:
    _dxt.Kind = BITMAP_DXT1;
    mode = 0;
    break;
  case sTEX_DXT1A:
    _dxt.Kind = BITMAP_DXT1;
    mode = 1;
    break;
  case sTEX_DXT3:
    _dxt.Kind = BITMAP_DXT3;
    mode = 1;
    break;
  case sTEX_DXT5:
    _dxt.Kind = BITMAP_DXT5;
    mode = 1;
    break;
  default:
    sVERIFYFALSE;
  }
  _dxt.XSize = dxt->SizeX;
  _dxt.YSize = dxt->SizeY;
  _dxt.Mipmaps = 1;
  _dxt.BPR = dxt->SizeX/2;
  _dxt.Data = (sU8 *) dxt->Data;

  _dxt.CompressTexture(&_bmp,_dxt.Kind,mode);
}

void CodecOld::Unpack(sImage *bmp,sImageData *dxt,sInt level)
{
  OldBitmap _bmp,_dxt;

  _bmp.Kind = BITMAP_ARGB8888;
  _bmp.XSize = bmp->SizeX;
  _bmp.YSize = bmp->SizeY;
  _bmp.Mipmaps = 1;
  _bmp.BPR = bmp->SizeX*4;
  _bmp.Data = (sU8 *) bmp->Data;

  switch(level)
  {
  case sTEX_DXT1:
  case sTEX_DXT1A:
    _dxt.Kind = BITMAP_DXT1;
    break;
  case sTEX_DXT3:
    _dxt.Kind = BITMAP_DXT3;
    break;
  case sTEX_DXT5:
    _dxt.Kind = BITMAP_DXT5;
    break;
  default:
    sVERIFYFALSE;
  }
  _dxt.XSize = dxt->SizeX;
  _dxt.YSize = dxt->SizeY;
  _dxt.Mipmaps = 1;
  _dxt.BPR = dxt->SizeX/2;
  _dxt.Data = (sU8 *) dxt->Data;

  _bmp.DecompressTexture(&_dxt);
}

/****************************************************************************/
/****************************************************************************/
/*
static sU32 make32(sU16 col)
{
  sInt r,g,b;

  r = (col&0xf800)>>8;  r = r | (r>>5);
  g = (col&0x07e0)>>3;  g = g | (g>>6);
  b = (col&0x001f)<<3;  b = b | (b>>5);

  return 0xff000000|(r<<16)|(g<<8)|(b<<0);
}

static void makecol(sU32 *c32)
{
  sU8 *c8 = (sU8 *) c32;

  c8[ 4] = (c8[0]*2+c8[12]*1)/3;
  c8[ 5] = (c8[1]*2+c8[13]*1)/3;
  c8[ 6] = (c8[2]*2+c8[14]*1)/3;
  c8[ 7] = (c8[3]*2+c8[15]*1)/3;

  c8[ 8] = (c8[0]*1+c8[12]*2)/3;
  c8[ 9] = (c8[1]*1+c8[13]*2)/3;
  c8[10] = (c8[2]*1+c8[14]*2)/3;
  c8[11] = (c8[3]*1+c8[15]*2)/3;
}

void CodecOld::Unpack(sImage *bmp,sImageData *dxt,sInt level)
{
  sU16 *s16;
  sU32 c[4];
  sU32 map;
  sU32 *d32;
  dxt->Init2(sTEX_DXT1|sTEX_2D,1,bmp->SizeX,bmp->SizeY,1);

  d32 = bmp->Data;
  s16 = (sU16 *)dxt->Data;
  for(sInt yy=0;yy<bmp->SizeY;yy+=4)
  {
    for(sInt xx=0;xx<bmp->SizeX;xx+=4)
    {
      c[0] = make32(*s16++);
      c[3] = make32(*s16++);
      makecol(&c[0]);
      map = *(sS32 *) s16; s16+=2;

      for(sInt y=0;y<4;y++)
      {
        for(sInt x=0;x<4;x++)
        {
          d32[y*bmp->SizeX+xx+x] = c[map&3];
          map = map>>2;
        }
      }
    }

    d32 += bmp->SizeX*4;
  }
}
*/
/****************************************************************************/
/****************************************************************************/

