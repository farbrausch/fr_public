// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "w3texmain.hpp"
#include "w3texlib.hpp"
#include "genbitmap.hpp"
#include "genvector.hpp"
#include "_start.hpp"
#include "_rygdxt.hpp"

/****************************************************************************/

namespace Werkk3TexLib
{

static KClass ClassList[] =
{
  0x21, 0x06000003, "bbc",                Bitmap_Flat,
  0x22, 0x0600000b, "bbbbebbggcc",        Bitmap_Perlin,
  0x23, 0x06000102, "bc",                 Bitmap_Color,
  0x24, 0x87000001, "b",                  Bitmap_Merge,
  0x25, 0x06000103, "bbb",                Bitmap_Format,
  0x27, 0x0600010b, "eeeeeecegbb",        Bitmap_GlowRect,
  0x28, 0x06000104, "ccib",               Bitmap_Dots,
  0x29, 0x06000104, "bggg",               Bitmap_Blur,
  0x2a, 0x06000301, "b",                  Bitmap_Mask,
  0x2b, 0x06000104, "gggg",               Bitmap_HSCB,
  0x2c, 0x06000108, "eggeebbb",           Bitmap_Rotate,
  0x2d, 0x06000202, "eb",                 Bitmap_Distort,
  0x2e, 0x06000102, "gb",                 Bitmap_Normals,
  0x2f, 0x0600010b, "bgggeeccege",        Bitmap_Light,
  0x30, 0x0600020e, "bgggeeccegecge",     Bitmap_Bump,
  0x31, 0x468a0109, "eeggcbbgg",          Bitmap_Text,
  0x32, 0x0600000d, "bbcccbbggbebf",      Bitmap_Cell,
//  0x33, 0x06000102, "bb",                 Bitmap_Wavelet, // kill?
  0x34, 0x06000008, "bbccgegb",           Bitmap_Gradient,
  0x35, 0x06000103, "bcc",                Bitmap_Range,
  0x36, 0x0600010a, "eggeebcbbc",         Bitmap_RotateMul,
  0x37, 0x06000107, "ggeeeeb",            Bitmap_Twirl,
  0x38, 0x06000104, "beee",               Bitmap_Sharpen,
  0x39, 0x0600010b, "eeeeeecegbb",        Bitmap_GlowRect,
  0x3a, 0x46090000, "",                   Bitmap_Import,
  0x3b, 0x06000109, "eeeeeeeee",          Bitmap_ColorBalance,
  0x3c, 0x06000101, "b",                  Bitmap_Unwrap,
  0x3d, 0x0600000e, "bbcccffbbbbbff",     Bitmap_Bricks,
  0x3e, 0x06000101, "g",                  Bitmap_Bulge,
  0x43, 0x06000208, "gggggggb",           Bitmap_Paste,
  0x44, 0x46090102, "cb",                 Bitmap_VectorPath, // only for testing atm!
  0x00, 0x00000000, 0,                    0,
};

/****************************************************************************/

// Maps an internal format to a W3ImageFormat
static W3ImageFormat MapFormat(sInt format)
{
  static const W3ImageFormat formatMap[sTF_MAX] =
  {
    W3IF_NONE,          // sTF_NONE
    W3IF_BGRA8,         // sTF_A8R8G8B8
    W3IF_NONE,          // sTF_A8
    W3IF_NONE,          // sTF_R16F
    W3IF_NONE,          // sTF_A2R10G10B10
    W3IF_UVWQ8,         // sTF_Q8W8V8U8
    W3IF_NONE,          // sTF_A2W10V10U10
    W3IF_NONE,          // sTF_A1R5G5B5
    W3IF_DXT1,          // sTF_DXT1
    W3IF_DXT5,          // sTF_DXT5
    W3IF_NONE,
    W3IF_NONE,
    W3IF_NONE,
  };

  if(format<0 || format>=sTF_MAX)
    return W3IF_NONE;
  else
    return formatMap[format];
}

// Calculates the number of bytes used by an image of given size
// in a given format.
static sInt CalcMipLevelSize(sInt xSize,sInt ySize,W3ImageFormat format)
{
  switch(format)
  {
  case W3IF_BGRA8:
  case W3IF_UVWQ8:
    return xSize * ySize * 4; // 4 bytes/pixel

  case W3IF_DXT1:
  case W3IF_DXT5:
    // DXTed images always have an integral number of blocks.
    // Convert x/y sizes to # of blocks in that direction.
    xSize = (xSize + 3) >> 2;
    ySize = (ySize + 3) >> 2;
    
    // One block is 8 bytes (DXT1) or 16 bytes (all other DXT formats)
    return xSize * ySize * (format == W3IF_DXT1 ? 8 : 16);

  default: // should never happen.
    return 0;
  }
}

// Converts an image to an output format
static void RenderMipLevel(sU8 *d,const sU64 *s,sInt xs,sInt ys,sInt oxs,W3ImageFormat outFmt,sInt dxtquality)
{
  switch(outFmt)
  {
  case W3IF_BGRA8:
  case W3IF_UVWQ8:
    for(sInt y=0;y<ys;y++)
    {
      const sU16 *s16 = (sU16 *) s;

      if(outFmt == W3IF_BGRA8)
      {
        for(sInt x=0;x<xs;x++)
        {
          d[0] = s16[0] >> 7;
          d[1] = s16[1] >> 7;
          d[2] = s16[2] >> 7;
          d[3] = s16[3] >> 7;

          d += 4;
          s16 += 4;
        }
      }
      else // UVWQ8
      {
        for(sInt x=0;x<xs;x++)
        {
          sF32 vx,vy,vz,len;

          vx = s16[2] - 0x4000;
          vy = s16[1] - 0x4000;
          vz = s16[0] - 0x4000;
          len = vx*vx + vy*vy + vz*vz;
          if(len)
          {
            len = sFInvSqrt(len);
            vx *= len;
            vy *= len;
            vz *= len;
          }

          d[0] = sInt(vx*127.0f);
          d[1] = sInt(vy*127.0f);
          d[2] = sInt(vz*127.0f);
          d[3] = (s16[3] - 0x4000) >> 7;

          d += 4;
          s16 += 4;
        }
      }

      s += oxs;
    }
    break;

  case W3IF_DXT1:
  case W3IF_DXT5:
    {
      sU8 pixels[16*4];
      sU32 block[4];
      
      const sU16 *s16 = (const sU16 *) s;
      sU32 *d32 = (sU32 *) d;
      sInt shiftX = sGetPower2(oxs*4);

      for(sInt y=0;y<ys;y+=4)
      {
        for(sInt x=0;x<xs;x+=4)
        {
          sU8 *dp = pixels;

          for(sInt yb=0;yb<4;yb++)
          {
            const sU16 *sp = s16 + (((y+yb) & (ys-1))<<shiftX);

            for(sInt xb=0;xb<4;xb++)
            {
              const sU16 *sr = sp + ((x+xb) & (xs-1))*4;

              dp[0] = (sr[0]>>7) & 0xff;
              dp[1] = (sr[1]>>7) & 0xff;
              dp[2] = (sr[2]>>7) & 0xff;
              dp[3] = (sr[3]>>7) & 0xff;
              dp += 4;
            }
          }

          sCompressDXTBlock((sU8 *) block,(sU32 *) pixels,outFmt==W3IF_DXT5,dxtquality);
          d32[0] = block[0];
          d32[1] = block[1];
          if(outFmt == W3IF_DXT1)
            d32 += 2;
          else
          {
            d32[2] = block[2];
            d32[3] = block[3];
            d32 += 4;
          }
        }
      }
    }
    break;
  }
}

// Downsample an image to create the next mip level (in-place)
static void DownsampleMipLevel(sU64 *data,sInt xs,sInt ys,sInt oxs,sBool filter)
{
  const sU64 *s = data;
  sU64 *d = data;

  for(sInt y=0;y<ys;y+=2)
  {
	  const sU16 *s16 = (const sU16 *) s;
	  sU16 *d16 = (sU16 *) d;

	  s+=oxs*2;
	  d+=oxs;

    sInt lineOffs = oxs*4;

	  for(sInt x=0;x<xs;x+=2)
	  {
      if(filter)
      {
		    d16[0] = (s16[0]+s16[4]+s16[lineOffs+0]+s16[lineOffs+4]+2)>>2;
		    d16[1] = (s16[1]+s16[5]+s16[lineOffs+1]+s16[lineOffs+5]+2)>>2;
		    d16[2] = (s16[2]+s16[6]+s16[lineOffs+2]+s16[lineOffs+6]+2)>>2;
		    d16[3] = (s16[3]+s16[7]+s16[lineOffs+3]+s16[lineOffs+7]+2)>>2;
      }
      else
      {
        d16[0] = s16[0];
		    d16[1] = s16[1];
		    d16[2] = s16[2];
		    d16[3] = s16[3];
      }
		  d16+=4;
		  s16+=8;
	  }
  }
}

// Converts a GenBitmap cache object to a W3Image structure.
static W3Image *CacheToImage(GenBitmap *bmp,sInt rootFlags)
{
  // first, check whether the format is supported and return 0
  // if it's not.
  W3ImageFormat outFmt = MapFormat(bmp->Format);
  if(outFmt == W3IF_NONE)
    return 0;

  // Allocate the image structure.
  W3Image *img = new W3Image(bmp->XSize,bmp->YSize,outFmt);

  if(img)
  {
    sU64 *data = new sU64[bmp->Size];

    if(rootFlags & RF_NOALPHA)
    {
      for(sInt i=0;i<bmp->Size;i++)
        data[i] = (bmp->Data[i] & 0x0000ffffffffffffull) | 0x7fff000000000000ull;
    }
    else
      sCopyMem(data,bmp->Data,bmp->Size*8);

    sInt mipdir = bmp->TexMipTresh & 16;
    sInt alpha = bmp->TexMipTresh & 32;
    sInt dxtquality = bmp->TexMipTresh >> 6;
    sInt mipthresh = bmp->TexMipTresh & 15;

    sInt xs = bmp->XSize;
    sInt ys = bmp->YSize;
    sInt oxs = xs;
    sInt level = 0;

    // the main miplevel loop
    for(;;)
    {
      img->MipSize[level] = CalcMipLevelSize(xs,ys,outFmt);
      img->MipData[level] = new unsigned char[img->MipSize[level]];

      // render this miplevel
      RenderMipLevel(img->MipData[level],data,xs,ys,oxs,outFmt,dxtquality);
      if(xs<=1 || ys<=1 || (bmp->TexMipCount && level+1>=bmp->TexMipCount))
        break;

      // downsample to next miplevel
      sBool filter = mipthresh <= level;
      if(mipdir)
        filter = !filter;

      DownsampleMipLevel(data,xs,ys,oxs,filter);

      xs >>= 1;
      ys >>= 1;
      level++;
    }

    img->MipLevels = level+1;
    delete[] data;
  }

  return img;
}

/****************************************************************************/

KObject::KObject() 
{ 
  ClassId=KC_NULL; 
  RefCount = 1;
}

KObject::~KObject() 
{
}

void KObject::Copy(KObject *o) 
{
  ClassId=o->ClassId;
}

KObject *KObject::Copy()
{
  KObject *r;
  r = new KObject;
  r->Copy(this);
  return r;
}

/****************************************************************************/

sBool KOp::CalcNoRec(sBool decreaseInRef)
{
  sBool ok;

  sRelease(Cache);
  Cache = Call();

  ok = Cache != 0;
  if(ok)
    CacheCount = OutputCount;

  if(decreaseInRef)
  {
    for(sInt j=0;j<GetInputCount();j++)
      GetInput(j)->CalcUsed();

    for(sInt j=0;j<GetLinkMax();j++)
      GetLink(j)->CalcUsed();
  }

  return ok;
}

sInt KOp::Calc(sInt flags)
{
  sInt i;
  sInt changed;

// early break recursion
  if(Cache)
    return 0;

// find changed

  changed = (flags & KCF_NEED) ? !Cache : sFALSE;
  if(Cache && !changed)
    flags &= ~KCF_NEED;

  for(i=0;i<InputCount;i++)
  {
    if(Input[i])
      changed |= Input[i]->Calc(flags&~KCF_ROOT);
  }
  if(!(Convention & OPC_DONTCALLLINK))
  {
    for(i=0;i<LinkMax;i++)
    {
      if(Link[i])
        changed |= Link[i]->Calc(flags&~KCF_ROOT);
    }
  }

// calculate

  if(changed)
    CalcNoRec(flags & KCF_PRECALC);

// done

  return changed;
}

/****************************************************************************/

#pragma warning(disable : 4731)   // EBP CHANGED WARNING

static sInt CallCode(sInt code,sInt *para,sInt count)
{
  sInt result;
  __asm
  {
    mov eax,code
    mov esi,para
    mov ecx,count
    push ebp
    mov ebp,esp
    sub esp,ecx
    sub esp,ecx
    sub esp,ecx
    sub esp,ecx
    mov edi,esp
    rep movsd

    call eax

    mov esp,ebp
    pop ebp
    mov result,eax
  }

  return result; 
}

#pragma warning(default : 4731)

/****************************************************************************/

#pragma warning(push)
#pragma warning(disable: 4311) // pointer truncation
#pragma warning(disable: 4312) // int->pointer

KObject *KOp::Call()
{
  sU32 data[KK_MAXINPUT+128];
  sInt p,pNoPara;
  sInt i,max;
  KObject *o;
  KOp *op;
  KObject *result;
  sInt error;

  p = 0;
#if !sINTRO
  error = 0;
#endif

// update

  if(Convention&(OPC_KOP|OPC_ALTINIT))
  {
    data[p++] = (sU32) this;
  }
  if(Convention&OPC_KENV)
  {
    data[p++] = (sU32) 0;/*kenv;*/
  }

  pNoPara = p;

// inputs
  max = OPC_GETINPUT(Convention);
  for(i=0;i<max;i++)
  {
    op = GetInput(i);

    if(op)
    {
      o = op->Cache;
      if(o==0)
        error = 1;
      else
        o->AddRef();
      data[p++] = (sU32) o;
    }
    else
    {
      data[p++] = 0;
    }
  }

// links

  max = OPC_GETLINK(Convention);
  for(i=0;i<max;i++)
  {
    if(Convention & OPC_DONTCALLLINK)
    {
      data[p++] = (sU32) Link[i];
    }
    else
    {
      o = (Link[i] ? Link[i]->Cache : 0);
      if(o)
        o->AddRef();
      data[p++] = (sU32) o;
    }
  }

// normal parameters

  max = OPC_GETDATA(Convention);
  for(i=0;i<max;i++)
    data[p++] = *GetEditPtrU(i);

// strings

  max = OPC_GETSTRING(Convention);
  for(i=0;i<max;i++)
    data[p++] = (sU32) GetString(i);

// unlimited inputs

  if(Convention&OPC_VARIABLEINPUT)
  {
    data[p++] = InputCount;
    for(i=0;i<InputCount;i++)
    {
      if(Input[i] && Input[i]->Cache && p<KK_MAXINPUT+128)
      {
        Input[i]->Cache->AddRef();
        data[p++] = (sU32) Input[i]->Cache;
      }
      else
        error = 1;
    }
  }

  if(Convention&OPC_ALTINIT)
    p = pNoPara;

  result = (KObject *)CallCode((sInt)InitHandler,(sInt *)data,p);

  return result;
}

#pragma warning(pop)

void KOp::Purge()
{
  sRelease(Cache);
  CacheCount = 0;
}

void KOp::CalcUsed()
{
  if(this && !--CacheCount && Cache)
    sRelease(Cache);
}

void KOp::Init(sU32 convention)
{
  sInt size;
  sU32 *mem;
  sInt i;

  DataMax   = OPC_GETDATA(convention);
  InputMax  = OPC_GETINPUT(convention);
  LinkMax   = OPC_GETLINK(convention);
  StringMax = OPC_GETSTRING(convention);

  if(convention & OPC_VARIABLEINPUT)
    InputMax = 255;
  OutputMax = 0;

  size = DataMax+InputMax+LinkMax+OutputMax+StringMax*2;
  mem = Memory = new sU32[size];
  sSetMem(mem,0,size*4);

  Input = (KOp**) mem; mem+=InputMax;
  Link = (KOp**) mem; mem+=LinkMax;
  Output = (KOp**) mem; mem+=OutputMax;
  String = (sChar**)mem; mem+=StringMax;
  DataEdit = mem; mem+=DataMax;
  for(i=0;i<StringMax;i++)
  {
    String[i] = (sChar *)mem;
    String[i][0] = 0;
    mem++;
  }
  InputCount = 0;
  OutputCount = 0;
  BlobData = 0;
  BlobSize = 0;
}

void KOp::Exit()
{
  if(Memory)
  {
    delete[] Memory;
    Memory = 0;
  }
}

/****************************************************************************/

static KClass *FindClass(sInt typeByte)
{
  for(sInt i=0;ClassList[i].Id;i++)
    if(ClassList[i].Id == typeByte)
      return &ClassList[i];

  return 0;
}

/****************************************************************************/

sBool KDoc::Init(const sU8 *&dataPtr)
{
  const sU8 *data = (const sU8 *) dataPtr;
  sInt opTypes[256];

  // check magic value
  if(sCmpMem(data,"TPWT",4))
    return sFALSE;
  else
    data += 4;

  sInt version = sReadShort(data);
  if(version < 1 || version > 2)
    return sFALSE;

  sInt rootCount = sReadShort(data);
  Roots.Init(rootCount);
  RootFlags.Init(rootCount);
  RootNames.Init(rootCount);
  RootImages.Init(rootCount);

  sInt opCount = sReadShort(data);
  Ops.Init(opCount);
  Ops.Count = opCount;
  
  sInt opTypeCount = sReadShort(data);
  if(opTypeCount > sCOUNTOF(opTypes))
    return sFALSE;

  // read roots
  for(sInt i=0;i<rootCount;i++)
  {
    sInt rootNum = sReadShort(data);
    sInt rootFlags = 0;
    if(version >= 2)
      rootFlags = *data++;

    *Roots.Add() = rootNum;
    *RootFlags.Add() = rootFlags;

    sChar *str = (sChar *) data;
    *RootNames.Add() = str;
    data += sGetStringLen(str)+1;
  }

  // read op classes
  for(sInt i=0;i<opTypeCount;i++)
    opTypes[i] = sReadShort(data);

  // read op types+connections, build the tree
  for(sInt i=0;i<opCount;i++)
  {
    sInt max,delta;
    sInt typeByte = *data++;
    KClass *cls = FindClass(opTypes[typeByte & 0x7f]);
    if(!cls) // unknown class is an error
      return sFALSE;

    KOp *op = &Ops[i];
    sSetMem(op,0,sizeof(KOp));
    op->Init(cls->Convention);
    op->Command = typeByte & 0x7f;
    op->Convention = cls->Convention;
    op->InitHandler = cls->InitHandler;
    op->ExecHandler = 0;
    op->Packing = cls->Packing;
    op->OpId = i;

    if(typeByte & 0x80)
      max = 1;
    else
      max = (cls->Convention & OPC_FLEXINPUT) ? *data++ : OPC_GETINPUT(cls->Convention);

    for(sInt in=0;in<max;in++)
    {
      delta = (typeByte & 0x80) ? 0 : sReadShort(data);
      KOp *dest = &Ops[i - 1 - delta];

      op->AddInput(dest);
      dest->AddOutput(op);
    }
  }

  for(sInt i=0;i<opCount;i++)
  {
    sInt delta;
    KOp *op = &Ops[i];

    for(sInt in=0;in<OPC_GETLINK(op->Convention);in++)
    {
      delta = sReadShort(data);
      KOp *dest = delta ? &Ops[delta - 1] : 0;
      op->SetLink(in,dest);
      if(dest)
        dest->AddOutput(op);
    }
  }

  // read ops sorted by type
  for(sInt i=0;i<opTypeCount;i++)
  {
    for(sInt in=0;in<opCount;in++)
    {
      KOp *op = &Ops[in];
      if(op->Command == i)
      {
        // read data
        sChar *pack = op->Packing;

        for(sInt j=0;pack[j];j++)
        {
          sChar typeByte = pack[j];
          if(typeByte>='A' && typeByte<='Z')
            typeByte += 0x20;

          switch(typeByte)
          {
          case 'g': *op->GetEditPtrF(j) = sReadF16(data);                          break;
          case 'f': *op->GetEditPtrF(j) = sReadF24(data);                          break;
          case 'e': *op->GetEditPtrF(j) = sReadX16(data);                          break;
          case 'i': *op->GetEditPtrS(j) = *((sS32 *) data); data += 4;             break;
          case 's': *op->GetEditPtrS(j) = *((sS16 *) data); data += 2;             break;
          case 'b': *op->GetEditPtrU(j) = *data++;                                 break;
          case 'c': *op->GetEditPtrU(j) = *((sU32 *) data); data += 4;             break;
          case 'm': *op->GetEditPtrU(j) = *((sU32 *) data) & 0xffffff; data += 3;  break;
          }
        }

        for(sInt j=0;j<OPC_GETSTRING(op->Convention);j++)
        {
          op->SetString(j,(sChar *)data);
          while(*data)
            data++;
          data++;
        }

        if(op->Convention&OPC_BLOB)
        {
          sInt size = *((sInt *) data); data += 4;
          op->SetBlobSize(size);
        }
      }
    }
  }

  // read blobs
  for(sInt i=0;i<opCount;i++)
  {
    sInt j = Ops[i].GetBlobSize();
    if(j)
    {
      Ops[i].SetBlobData(data);
      data += j;
    }
  }

  // roots have an extra output reference
  for(sInt i=0;i<Roots.Count;i++)
    Ops[Roots[i]].OutputCount++;

  /*// (DEBUG) transform command numbers
  for(sInt i=0;i<opCount;i++)
    Ops[i].Command = opTypes[Ops[i].Command];*/

  dataPtr = data;
  return sTRUE;
}

void KDoc::Exit()
{
  for(sInt i=0;i<Ops.Count;i++)
    Ops[i].Exit();

  Ops.Exit();
  Roots.Exit();
  RootNames.Exit();
  RootImages.Exit();
}

sBool KDoc::Calculate(W3ProgressCallback cb)
{
  sBool error = sFALSE;
  RootImages.Count = 0;

  // new calc logic
  sInt curRoot = 0;

  // report start of calc
  if(cb && !cb(0,Ops.Count)) // aborted
    return sFALSE;

  for(sInt i=0;i<Ops.Count;i++)
  {
    KOp *op = &Ops[i];
    if(!op->CalcNoRec(sTRUE))
    {
      error = sTRUE;
      break;
    }

    // is this a root?
    if(curRoot < Roots.Count && i == Roots[curRoot])
    {
      // convert cache to image, then release it
      if(op->Cache)
      {
        *RootImages.Add() = CacheToImage((GenBitmap *) op->Cache,RootFlags[curRoot]);
        op->CalcUsed();
      }
      else
        *RootImages.Add() = 0;

      curRoot++;
    }

    // call the callback (if specified)
    if(cb && !cb(i+1,Ops.Count)) // aborted
    {
      // aborted!
      error = sTRUE;
      break;
    }
  }

  // on error/abort, free everything
  if(error)
  {
    // free allocated root images
    for(sInt i=0;i<RootImages.Count;i++)
      delete RootImages[i];

    RootImages.Count = 0;

    // free op data
    for(sInt i=0;i<Ops.Count;i++)
      Ops[i].Purge();
  }

  return !error;
}

/****************************************************************************/

}

/****************************************************************************/
