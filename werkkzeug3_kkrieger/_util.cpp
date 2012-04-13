// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_util.hpp"
#include "_start.hpp"
#include "_startdx.hpp"
#include "material11.hpp"

sPerfMon_ *sPerfMon;

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   simple helpers                                                     ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#pragma pack(push,2)
struct BMPHeader
{
  sU16 Magic;                     // 'B'+'M'*256
  sU32 FileSize;                  // size of the file
  sU32 pad0;                      // unused
  sU32 Offset;                    // offset from start to bits
              
  sU32 InfoSize;                  // size of the info part
  sU32 XSize;                     // Pixel Size
  sU32 YSize;
  sU16 Planes;                    // always 1
  sU16 BitCount;                  // 4,8,24
  sU32 Compression;               // always 0
  sU32 ImageSize;                 // size in bytes or 0
  sU32 XPelsPerMeter;             // bla
  sU32 YPelsPerMeter;
  sU32 ColorUsed;                 // for 8 bit 
  sU32 ColorImportant;            // bla
  sU32 Color[256];              // optional xbgr
};
#pragma pack(pop)

static sU32 Swap32(sU8 *&scan)
{
  sU32 val;
  val = (scan[0]<<24)|(scan[1]<<16)|(scan[2]<<8)|(scan[3]);
  scan+=4;
  return val;
}

static sU32 Swap16(sU8 *&scan)
{
  sU32 val;
  val = (scan[0]<<8)|(scan[1]);
  scan+=2;
  return val;
}

sBitmap *sLoadBitmap(sChar *name)
{
  sInt len;
  sChar *ext;
  sU8 *data,*mem;
  sInt size;
  sBitmap *bm;
  sBool ok;
//  static sChar jpath[sMAX_PATHNAME];

  len = sGetStringLen(name);
  if(len<4)
    return 0;
  ext = name + len - 4;

/*
  if(len<sizeof(jpath)-5 && ext[0]=='.')
  {
    sCopyMem(jpath,name,len);
    sCopyMem(jpath+len-4,".jpg",5);
    mem = data = sSystem->LoadFile(jpath,size);
    if(data!=0)
    {
      bm = new sBitmap;
      bm->Init(256,256);
      if(sSystem->LoadJPG(bm,mem,size))
      {
        bm->CompressedData = data;
        bm->CompressedSize = size;
        return bm;
      }
      else
      {
        delete[] data;
      }
    }
  }
    
*/

  ok = sTRUE;
  bm = new sBitmap;
  mem = data = sSystem->LoadFile(name,size);
  if(data==0)
    ok = sFALSE;

  if(ok && sCmpStringI(ext,".bmp")==0)
  {
    BMPHeader *hdr;
    sInt x,y;
    sU8 *d,*s;
    sU8 val;

    hdr = (BMPHeader *) data;

    if(hdr->XSize<1 || 
       hdr->YSize<1 || 
       hdr->Planes!=1 ||
       hdr->Magic != 'B'+'M'*256 ||
       hdr->Compression != 0)
    {
      ok = sFALSE;
    }

    bm->Init(hdr->XSize,hdr->YSize);
    s = data+hdr->Offset;

    switch(hdr->BitCount)
    {
    case 8:
      for(y=0;y<bm->YSize;y++)
      {
        d = (sU8*)(bm->Data + (bm->YSize-1-y)*bm->XSize);
        for(x=0;x<bm->XSize;x++)
        {
          val = *s++;
          *d++ = hdr->Color[val];
          *d++ = hdr->Color[val]>>8;
          *d++ = hdr->Color[val]>>16;
          *d++ = 255;
        }
      }
      break;

    case 24:
      for(y=0;y<bm->YSize;y++)
      {
        d = (sU8*)(bm->Data + (bm->YSize-1-y)*bm->XSize);
        for(x=0;x<bm->XSize;x++)
        {
          *d++ = s[0];
          *d++ = s[1];
          *d++ = s[2];
          *d++ = 255;
          s+=3;
        }
      }
      break;

    default:
      ok = sFALSE;
      break;
    }
  }
  else if(ok && sCmpStringI(ext,".pic")==0)
  {
    sU32 val;
    sU32 *d;
    sInt xs,ys,x,y;
    sBool hasalpha;
    sU32 alphamask;
    sInt count;
    sInt i;

    if(Swap32(data)!=0x5380f634) ok = sFALSE;
    data+=84;
    if(*data++!='P') ok = sFALSE;
    if(*data++!='I') ok = sFALSE;
    if(*data++!='C') ok = sFALSE;
    if(*data++!='T') ok = sFALSE;
    xs = Swap16(data);
    ys = Swap16(data);
    data+=8;
    if(ok)
      bm->Init(xs,ys);

    hasalpha = *data++;
    if(*data++!=8) ok = sFALSE;
    if(*data++!=2) ok = sFALSE;
    if(*data++!=0xe0) ok = sFALSE;
    alphamask = 0xff000000;
    if(hasalpha)
    {
      alphamask = 0;
      if(*data++!=0) ok = sFALSE;
      if(*data++!=8) ok = sFALSE;
      if(*data++!=2) ok = sFALSE;
      if(*data++!=0x10) ok = sFALSE;
    }

    if(ok)
    {
      for(y=0;y<ys;y++)
      {
        d = bm->Data + y*bm->XSize;   // color
        x = 0;
        while(x<xs)
        {
          count = *data++;
          if(count>=128)              // run
          {
            if(count==128)
              count = Swap16(data);
            else
              count = count-127;      
            val = (data[0]<<16)|(data[1]<<8)|data[2]|alphamask;
            data+=3;
            for(i=0;i<count;i++)
              *d++ = val;
          }
          else                        // copy
          {
            count++;
            for(i=0;i<count;i++)
            {
              *d++ = (data[0]<<16)|(data[1]<<8)|data[2]|alphamask;
              data+=3;            
            }
          }
          x+=count;
        }
        sVERIFY(x==xs);

        if(hasalpha)
        {
          d = bm->Data + y*bm->XSize;   // alpha
          x = 0;
          while(x<xs)
          {
            count = *data++;
            if(count>=128)              // run
            {
              if(count==128)
                count = Swap16(data);
              else
                count = count-127;      
              val = data[0]<<24;
              data+=1;
              for(i=0;i<count;i++)
                *d++ |= val;
            }
            else                        // copy
            {
              count++;
              for(i=0;i<count;i++)
              {
                *d++ |= data[0]<<24;
                data+=1;            
              }
            }
            x+=count;
          }
          sVERIFY(x==xs);
        }
      }
    }
  }

  if(mem)
    delete[] mem;

  if(!ok)
  {
    bm = 0;
  }

  return bm;
}

/****************************************************************************/

struct BitmapFileHeader
{
  sU16  Type;
  sU16  SizeL;
  sU16  SizeH;
  sU16  Reserved1;
  sU16  Reserved2;
  sU16  OffsetL;
  sU16  OffsetH;
};

struct BitmapInfoHeader
{
  sU32 Size;
  sS32 XSize;
  sS32 YSize;
  sS16 Planes;
  sS16 BitCount;
  sU32 Compression;
  sU32 SizeImage;
  sS32 XPelsPerMeter;
  sS32 YPelsPerMeter;
  sU32 ColorsUsed;
  sU32 ColorsImportant;
};

sBool sSaveBMP(sChar *name,sBitmap *bm)
{
  sInt headersize;
  sInt bitmapsize;
  sInt colmapsize;

  sInt bitpp;
  sInt bpr;
  sInt xs,ys;
  sInt colors;
  sInt i,x,y;

  BitmapFileHeader bfh;
  BitmapInfoHeader bih;
  sU8 RGBColors[4*256];
  sU8 *Data,*Ptr,*src;
  sInt Size;

// get size

  xs=bm->XSize;
  ys=bm->YSize;

// get kind, bpr and colormap

  bitpp=24;
  colors=0;
  for(i=0;i<colors;i++)
  {
    RGBColors[i*4+0]=0xff;
    RGBColors[i*4+1]=0xff;
    RGBColors[i*4+2]=0xff;
    RGBColors[i*4+3]=0x00;
  }

  bpr=(bitpp*xs/8+3)&(~3);

// create header  

  headersize=sizeof(bfh)+sizeof(bih);
  bitmapsize=bpr*ys;
  colmapsize=colors*4;

  bfh.Type              = 0x4d42;
  bfh.SizeL             = (headersize+bitmapsize+colmapsize)&0x0ffff;
  bfh.SizeH             = (headersize+bitmapsize+colmapsize)/0x10000;
  bfh.Reserved1         = 0;
  bfh.Reserved2         = 0;
  bfh.OffsetL           = (headersize+colmapsize)&0x0ffff;
  bfh.OffsetH           = (headersize+colmapsize)/0x10000;

  bih.Size              = sizeof(bih);
  bih.XSize             = xs;
  bih.YSize             = ys;
  bih.Planes            = 1;
  bih.BitCount          = bitpp;
  bih.Compression       = 0;
  bih.SizeImage         = 0;
  bih.XPelsPerMeter     = 0;
  bih.YPelsPerMeter     = 0;
  bih.ColorsUsed        = colors;
  bih.ColorsImportant   = 0;

// create image

  Size = sizeof(bfh)+sizeof(bih)+colmapsize+ys*bpr;
  Data = new sU8[Size];
  Ptr = Data;

  sCopyMem(Ptr,&bfh,sizeof(bfh)); Ptr+=sizeof(bfh);
  sCopyMem(Ptr,&bih,sizeof(bih)); Ptr+=sizeof(bih);
  sCopyMem(Ptr,RGBColors,colmapsize); Ptr+=colmapsize;
  
  for(y=ys-1;y>=0;y--)
  {
    src=((sU8 *)bm->Data)+y*bm->XSize*4;
    for(x=0;x<xs;x++)
    {
      Ptr[x*3+0] = src[0];
      Ptr[x*3+1] = src[1];
      Ptr[x*3+2] = src[2];
      src+=4;
    }
    Ptr+=bpr;
  }

  sVERIFY(Ptr == Data + Size);

// save to disk

  sBool result = sSystem->SaveFile(name,(sU8 *)Data,Size);

// clean up

  delete[] Data;
  return result;
}

/****************************************************************************/

sBool sSaveTarga(sChar *name,sBitmap *bm)
{
  sU8 *data,*mem,*src;
  sInt x,y;

  mem = data = new sU8[32+bm->XSize*bm->YSize*3];
  
  sSetMem(data,0,32);
  data[0] = 32-18;
  data[1] = 0;
  data[2] = 2;
  *(sU16 *)(data+12) = bm->XSize;
  *(sU16 *)(data+14) = bm->YSize;
  data[16] = 24;
  data[17] = 0x30;
  data+=32;
  
  src = (sU8 *) bm->Data;
  for(y=0;y<bm->YSize;y++)
  {
    for(x=0;x<bm->XSize;x++)
    {
      data[0] = src[0];
      data[1] = src[1];
      data[2] = src[2];
      data+=3;
      src+=4;
    }
  }

  sBool result = sSystem->SaveFile(name,mem,32+bm->XSize*bm->YSize*3);
  delete[] mem;
  return result;
}

/****************************************************************************/

sBool sSaveBitmap(sChar *name,sBitmap *bm)
{
  sInt len;
  sChar *ext;

  len = sGetStringLen(name);
  if(len<4)
    return 0;
  ext = name + len - 4;

  if(sCmpStringI(ext,".bmp")==0)
    return sSaveBMP(name,bm);
  else if(sCmpStringI(ext,".tga")==0)
    return sSaveTarga(name,bm);
  else 
    return 0;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   GuiPainter - draw sprites and print text                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sGuiPainter::sGuiPainter()
{
//  sU32 *p;

  Mat = 0;
  Job = 0;
  Text = 0;
  Ext = 0;
  Rect = 0;
  Lines = 0;

  MatAlloc = 0;
  MatCount = 0;
  JobAlloc = 0;
  JobCount = 0;
  TextAlloc = 0;
  TextCount = 0;
  ExtAlloc = 0;
  ExtCount = 0;
  RectAlloc = 0;
  RectCount = 0;
  LineAlloc = 0;
  LineCount = 0;
  GeoHandle = 0;
  LineHandle = 0;

  MtrlEnv.Init();
  MtrlEnv.Orthogonal = sMEO_PIXELS;
 
  PaintZ = 0.0f;
  PrintZ = 0.0f;
  PaintCol = ~0;
  PrintCol = 0xff000000;

  GeoHandle = sSystem->GeoAdd(sFVF_DOUBLE,sGEO_QUAD);
  LineHandle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE);

  Clipping = 0;
}

/****************************************************************************/

sGuiPainter::~sGuiPainter()
{
  if(Mat)
  {
    for(sInt i=0;i<MatCount;i++)
    {
      if(Mat[i].Letters)
        delete[] Mat[i].Letters;
      sRelease(Mat[i].Material);
    }
    delete[] Mat;
  }
  if(Job)
    delete[] Job;
  if(Text)
    delete[] Text;
  if(Ext)
    delete[] Ext;
  if(Rect)
    delete[] Rect;
  if(Lines)
    delete[] Lines;
  if(GeoHandle)
    sSystem->GeoRem(GeoHandle);
  if(LineHandle)
    sSystem->GeoRem(LineHandle);
}

/****************************************************************************/

void sGuiPainter::Tag()
{
}

/****************************************************************************/

void sGuiPainter::Init(sInt mat,sInt job,sInt text,sInt ext,sInt rect,sInt line)
{
  MatAlloc = mat;
  JobAlloc = job;
  TextAlloc = text;
  ExtAlloc = ext;
  RectAlloc = rect;
  LineAlloc = line;

  MatCount = 0;
  JobCount = 0;
  TextCount = 0;
  ExtCount = 0;
  RectCount = 0;
  LineCount = 0;

  sVERIFY(Mat==0);
  sVERIFY(Job==0);
  sVERIFY(Text==0);
  sVERIFY(Ext==0);
  sVERIFY(Rect==0);
  sVERIFY(Lines==0);

  Mat = new sGuiPainterMaterial[mat];
  Job = new sGuiPainterJob[job];
  Text = new sChar[text];
  Ext = new sGuiPainterExtPos[ext];
  Rect = new sRect[rect];
  Lines = new sGuiPainterLine[line];

  LoadMaterial(new sSimpleMaterial(sINVALID,sMBF_BLENDALPHA,0,0));
}

/****************************************************************************/
/****************************************************************************/

sInt sGuiPainter::LoadFont(sChar *name,sInt height,sInt width,sInt style)
{
  sHostFont *hf;
  sInt xs,ys;
  sBool toggle;
  sChar letters[256];
  sHostFontLetter *hlet;
  sGuiPainterLetter *let;
  sInt i,j;
  sBitmap *bm;
  sU32 *data;
  sInt handle;
  sInt tex;
  sTexInfo ti;

// prepare list of letters

  j=0;
  for(i=0x20;i<0x7f;i++)
    letters[j++] = i;
  for(i=0xa0;i<0x100;i++)
    letters[j++] = i;
  letters[j++] = 0;
  hlet = new sHostFontLetter[256];

// find right size

  xs = 64;
  ys = 64;
  toggle = 0;
  hf = 0;
  while(xs<2048 && ys<2048)
  {
    if(hf)
      delete hf;
    hf = new sHostFont;
    hf->Init(name,height,width,style);
    if(hf->PrepareCheck(xs,ys,letters))
      break;

    if(toggle)
      ys = ys*2;
    else
      xs = xs*2;
    toggle = !toggle;
  }

// create bitmap

  bm = new sBitmap;
  bm->Init(xs,ys);
  sSetMem4(bm->Data,0xff008000,xs*ys);
  hf->Prepare(bm,letters,hlet);

  data = (sU32 *)bm->Data;
  for(i=0;i<xs*ys;i++)
    data[i] = (data[i]<<24)|0xffffff;

// register material

  ti.Init(bm);
  tex = sSystem->AddTexture(ti);

  sVERIFY(sISHANDLE(tex));
  handle = LoadMaterial(new sSimpleMaterial(tex,sMBF_BLENDALPHA,0,0),bm->XSize,bm->YSize);
  if(sISHANDLE(handle))
  {
    let = new sGuiPainterLetter[256];
    Mat[handle].Letters = let;
    Mat[handle].Advance = hf->GetAdvance();
    Mat[handle].Height = hf->GetHeight();
    Mat[handle].BaseLine = hf->GetBaseline();

    for(i=0;i<256;i++)
    {
      let[i].PosX = hlet[i].Location.x0;
      let[i].PosY = hlet[i].Location.y0;
      let[i].PreKern = hlet[i].Pre;
      let[i].CellWidth = hlet[i].Location.x1-hlet[i].Location.x0;
      let[i].PostKern = hlet[i].Post;
      let[i].pad = 0;
    }
  }
  delete hlet;
  delete hf;

  return handle;
}

/****************************************************************************/

sInt sGuiPainter::LoadBitmap(sChar *file)
{
  return LoadBitmap(sLoadBitmap(file));
}

/****************************************************************************/

sInt sGuiPainter::LoadBitmap(sBitmap *bm)
{
  sTexInfo ti;
  sInt tex;

  ti.Init(bm);
  tex = sSystem->AddTexture(ti);
  sVERIFY(sISHANDLE(tex));

  return LoadMaterial(new sSimpleMaterial(tex,0,0,0),bm->XSize,bm->YSize);
}

/****************************************************************************/

sInt sGuiPainter::LoadBitmapAlpha(sBitmap *bm)
{
  sTexInfo ti;
  sInt tex;

  ti.Init(bm);
  tex = sSystem->AddTexture(ti);
  sVERIFY(sISHANDLE(tex));

  return LoadMaterial(new sSimpleMaterial(tex,sMBF_BLENDALPHA,0,0),bm->XSize,bm->YSize);
}

/****************************************************************************/

sInt sGuiPainter::LoadMaterial(sMaterial *mtrl,sInt xs,sInt ys)
{
  sGuiPainterMaterial *mat;
  sVERIFY(MatCount<MatAlloc);

  mat = &Mat[MatCount];
  mat->FirstJob = 0;
  mat->LastJob = &mat->FirstJob;
  mat->Letters = 0;
  mat->Height = 0;
  mat->Advance = 0;
  mat->BaseLine = 0;
  mat->States = 0;
  mat->Material = mtrl;
  mat->Texture = 0;
  mat->QuadCount = 0;
  mat->XSize = xs;
  mat->YSize = ys;

  return MatCount++;
}

/****************************************************************************/
/****************************************************************************/

void sGuiPainter::Paint(sInt handle,sInt x,sInt y,sU32 col)
{
  sGuiPainterJob *job;

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  col = col==0 ? PaintCol : col;
  job = &Job[JobCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = x;
  job->y = y;
  job->z = PaintZ;
  job->Color = col;
  job->Text = 0;
  job->ExtPos = 0;
  job->ClipRect = Clipping;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::Paint(sInt handle,sInt x,sInt y,sInt xs,sInt ys,sU32 col)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

  if(xs<=0 || ys<=0) return;
  CheckOverflow();

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(ExtCount<ExtAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  col = col==0 ? PaintCol : col;
  job = &Job[JobCount++];
  ext = &Ext[ExtCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = x;
  job->y = y;
  job->z = PaintZ;
  job->Color = col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = Clipping;
  ext->XYUV[0][0]=x   ; ext->XYUV[0][1]=y   ; ext->XYUV[0][2]=0; ext->XYUV[0][3]=0;
  ext->XYUV[1][0]=x+xs; ext->XYUV[1][1]=y   ; ext->XYUV[1][2]=1; ext->XYUV[1][3]=0;
  ext->XYUV[2][0]=x+xs; ext->XYUV[2][1]=y+ys; ext->XYUV[2][2]=1; ext->XYUV[2][3]=1;
  ext->XYUV[3][0]=x   ; ext->XYUV[3][1]=y+ys; ext->XYUV[3][2]=0; ext->XYUV[3][3]=1;
  ext->Col[0] = ext->Col[1] = ext->Col[2] = ext->Col[3] = col;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::Paint(sInt handle,const sRect &r,sU32 col)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

  CheckOverflow();

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(ExtCount<ExtAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  col = col==0 ? PaintCol : col;
  job = &Job[JobCount++];
  ext = &Ext[ExtCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = r.x0;
  job->y = r.y0;
  job->z = PaintZ;
  job->Color = col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = Clipping;
  ext->XYUV[0][0]=r.x0; ext->XYUV[0][1]=r.y0; ext->XYUV[0][2]=0; ext->XYUV[0][3]=0;
  ext->XYUV[1][0]=r.x1; ext->XYUV[1][1]=r.y0; ext->XYUV[1][2]=1; ext->XYUV[1][3]=0;
  ext->XYUV[2][0]=r.x1; ext->XYUV[2][1]=r.y1; ext->XYUV[2][2]=1; ext->XYUV[2][3]=1;
  ext->XYUV[3][0]=r.x0; ext->XYUV[3][1]=r.y1; ext->XYUV[3][2]=0; ext->XYUV[3][3]=1;
  ext->Col[0] = ext->Col[1] = ext->Col[2] = ext->Col[3] = col;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::Paint(sInt handle,const sRect &pos,const sFRect &uv,sU32 col)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

  CheckOverflow();

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(ExtCount<ExtAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  col = col==0 ? PaintCol : col;
  job = &Job[JobCount++];
  ext = &Ext[ExtCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = 0;
  job->y = 0;
  job->z = PaintZ;
  job->Color = col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = Clipping;
  ext->XYUV[0][0]=pos.x0; ext->XYUV[0][1]=pos.y0; ext->XYUV[0][2]=uv.x0; ext->XYUV[0][3]=uv.y0;
  ext->XYUV[1][0]=pos.x1; ext->XYUV[1][1]=pos.y0; ext->XYUV[1][2]=uv.x1; ext->XYUV[1][3]=uv.y0;
  ext->XYUV[2][0]=pos.x1; ext->XYUV[2][1]=pos.y1; ext->XYUV[2][2]=uv.x1; ext->XYUV[2][3]=uv.y1;
  ext->XYUV[3][0]=pos.x0; ext->XYUV[3][1]=pos.y1; ext->XYUV[3][2]=uv.x0; ext->XYUV[3][3]=uv.y1;
  ext->Col[0] = ext->Col[1] = ext->Col[2] = ext->Col[3] = col;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

void sGuiPainter::Paint(sInt handle,const sInt *pos,const sF32 *uv,sU32 col)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

  CheckOverflow();

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(ExtCount<ExtAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  col = col==0 ? PaintCol : col;
  job = &Job[JobCount++];
  ext = &Ext[ExtCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = 0;
  job->y = 0;
  job->z = PaintZ;
  job->Color = col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = 0;//Clipping;
  ext->XYUV[0][0]=pos[0]; ext->XYUV[0][1]=pos[1]; 
  ext->XYUV[1][0]=pos[2]; ext->XYUV[1][1]=pos[3]; 
  ext->XYUV[2][0]=pos[4]; ext->XYUV[2][1]=pos[5]; 
  ext->XYUV[3][0]=pos[6]; ext->XYUV[3][1]=pos[7];
  if(uv)
  {
    ext->XYUV[0][2]=uv[0]; ext->XYUV[0][3]=uv[1]; 
    ext->XYUV[1][2]=uv[2]; ext->XYUV[1][3]=uv[3]; 
    ext->XYUV[2][2]=uv[4]; ext->XYUV[2][3]=uv[5]; 
    ext->XYUV[3][2]=uv[6]; ext->XYUV[3][3]=uv[7];
  }
  else
  {
    ext->XYUV[0][2]=0; ext->XYUV[0][3]=0; 
    ext->XYUV[1][2]=0; ext->XYUV[1][3]=0; 
    ext->XYUV[2][2]=0; ext->XYUV[2][3]=0; 
    ext->XYUV[3][2]=0; ext->XYUV[3][3]=0;
  }
  ext->Col[0] = ext->Col[1] = ext->Col[2] = ext->Col[3] = col;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::Paint(sInt handle,sGuiPainterVert *v,sInt aligned)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

  CheckOverflow();

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(ExtCount<ExtAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  job = &Job[JobCount++];
  ext = &Ext[ExtCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = 0;
  job->y = 0;
  job->z = PaintZ;
  job->Color = ~0;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = aligned?Clipping:0;
  for(sInt i=0;i<4;i++)
  {
    ext->XYUV[i][0] = v[i].x;
    ext->XYUV[i][1] = v[i].y;
    ext->XYUV[i][2] = v[i].u;
    ext->XYUV[i][3] = v[i].v;
    ext->Col[i] = v[i].col;
  }
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::PaintHole(sInt handle,const sRect &out,const sRect &in,sU32 col)
{
  sRect r;

  r.Init(out.x0,out.y0,out.x1,in .y0); Paint(handle,r,col);
  r.Init(out.x0,in .y0,in .x0,in .y1); Paint(handle,r,col);
  r.Init(in .x1,in .y0,out.x1,in .y1); Paint(handle,r,col);
  r.Init(out.x0,in .y1,out.x1,out.y1); Paint(handle,r,col);
}

/****************************************************************************/

void sGuiPainter::PaintF(sInt handle,sF32 x,sF32 y,sU32 col)
{
  sGuiPainterJob *job;

  CheckOverflow();

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  col = col==0 ? PaintCol : col;
  job = &Job[JobCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = x;
  job->y = y;
  job->z = PaintZ;
  job->Color = col;
  job->Text = 0;
  job->ExtPos = 0;
  job->ClipRect = Clipping;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::PaintF(sInt handle,sF32 x,sF32 y,sF32 xs,sF32 ys,sU32 col)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

  CheckOverflow();

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(ExtCount<ExtAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  col = col==0 ? PaintCol : col;
  job = &Job[JobCount++];
  ext = &Ext[ExtCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = x;
  job->y = y;
  job->z = PaintZ;
  job->Color = col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = Clipping;
  ext->XYUV[0][0]=x   ; ext->XYUV[0][1]=y   ; ext->XYUV[0][2]=0; ext->XYUV[0][3]=0;
  ext->XYUV[1][0]=x+xs; ext->XYUV[1][1]=y   ; ext->XYUV[1][2]=1; ext->XYUV[1][3]=0;
  ext->XYUV[2][0]=x+xs; ext->XYUV[2][1]=y+ys; ext->XYUV[2][2]=1; ext->XYUV[2][3]=1;
  ext->XYUV[3][0]=x   ; ext->XYUV[3][1]=y+ys; ext->XYUV[3][2]=0; ext->XYUV[3][3]=1;
  ext->Col[0] = ext->Col[1] = ext->Col[2] = ext->Col[3] = col;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::PaintF(sInt handle,sF32 x0,sF32 y0,sF32 x1,sF32 y1,sF32 u0,sF32 v0,sF32 u1,sF32 v1,sU32 col)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

  CheckOverflow();

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(ExtCount<ExtAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  col = col==0 ? PaintCol : col;
  job = &Job[JobCount++];
  ext = &Ext[ExtCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = 0;
  job->y = 0;
  job->z = PaintZ;
  job->Color = col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = Clipping;
  ext->XYUV[0][0]=x0; ext->XYUV[0][1]=y0; ext->XYUV[0][2]=u0; ext->XYUV[0][3]=v0;
  ext->XYUV[1][0]=x1; ext->XYUV[1][1]=y0; ext->XYUV[1][2]=u1; ext->XYUV[1][3]=v0;
  ext->XYUV[2][0]=x1; ext->XYUV[2][1]=y1; ext->XYUV[2][2]=u1; ext->XYUV[2][3]=v1;
  ext->XYUV[3][0]=x0; ext->XYUV[3][1]=y1; ext->XYUV[3][2]=u0; ext->XYUV[3][3]=v1;
  ext->Col[0] = ext->Col[1] = ext->Col[2] = ext->Col[3] = col;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::Line(sInt x0,sInt y0,sInt x1,sInt y1,sU32 col)
{
  sGuiPainterLine *line;
  sRect r;

  CheckOverflow();

  sVERIFY(LineCount<LineAlloc);

  if(Clipping)
  {
    r = Rect[RectCount-1];
    if(!r.Hit(x0,y0)) return;
    if(!r.Hit(x1,y1)) return;
  }

  // adjust for fill convention
  // (some more twiddling later: this is really just a matter of luck...
  // seems anything except straight horizontal or vertical lines is
  // interpreted differently on different hw)
  sInt len = sMax(sAbs(x1-x0),sAbs(y1-y0));

  if(len)
  {
    line = &Lines[LineCount++];

    line->x0 = x0 + 0.5f;
    line->y0 = y0 + 0.5f;
    line->x1 = x1 + 0.5f;
    line->y1 = y1 + 0.5f;
    line->col = col;
  }
}

void sGuiPainter::LineF(sF32 x0,sF32 y0,sF32 x1,sF32 y1,sU32 col)
{
  sGuiPainterLine *line;
  sRect r;

  CheckOverflow();

  sVERIFY(LineCount<LineAlloc);

  if(Clipping)
  {
    r = Rect[RectCount-1];
    if(!r.Hit(sFtol(x0),sFtol(y0))) return;
    if(!r.Hit(sFtol(x1),sFtol(y1))) return;
  }
  line = &Lines[LineCount++];
  line->x0 = x0;
  line->y0 = y0;
  line->x1 = x1;
  line->y1 = y1;
  line->col = col;
}

/****************************************************************************/

void sGuiPainter::PaintMode(sU32 col,sF32 z)
{
  PaintCol = col;
  PaintZ = z;
}

/****************************************************************************/
/****************************************************************************/

void sGuiPainter::Print(sInt handle,sInt x,sInt y,const sChar *text,sU32 col,sInt len)
{
  if(len==-1) len = sGetStringLen(text);

  sGuiPainterJob *job;

  CheckOverflow();

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(handle>=0 && handle<MatCount);
  sVERIFY(Mat[handle].Letters);
  sVERIFY(TextCount+len+1 < TextAlloc);

  job = &Job[JobCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = x;
  job->y = y;
  job->z = PrintZ;
  job->Color = col==0 ? PrintCol : col;
  job->Text = Text+TextCount;
  job->ExtPos = 0;
  job->ClipRect = Clipping;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount+=len;

  sCopyMem(Text+TextCount,(const sPtr)text,len);
  TextCount+=len;
  Text[TextCount++]=0;

  if(Mat[handle].QuadCount>0x1800)
    Flush();
}

/****************************************************************************/

sInt sGuiPainter::PrintC(sInt handle,sRect &r,sInt align,const sChar *text,sU32 col,sInt len)
{
  sF32 x,y,xs,ys;
  sInt i;
  sInt clipping = 0;

  sVERIFY(Mat[handle].Letters);
  if(len==-1) len = sGetStringLen(text);

  xs = GetWidth(handle,text,len);
  ys = GetHeight(handle);

  if(ys<=r.y1-r.y0)
  {
    x = r.x0;
    y = r.y0;
    if(align & sFA_BOTTOM)
    {
      y = r.y1 - ys;
    }
    if(!(align & (sFA_BOTTOM|sFA_TOP)))
    {
      y = r.y0 + (r.y1-r.y0-ys)/2;
    }

    if(align & sFA_RIGHT)
    {
      x = r.x1 - xs;
      if(align & sFA_SPACE)
        x -= GetWidth(handle," ",1);
    }
    else if(align & sFA_LEFT)
    {
      if(align & sFA_SPACE)
        x += GetWidth(handle," ",1);
    }
    else
    {
      x = r.x0 + (r.x1-r.x0-xs)/2;
    }

    if(align&sFA_CLIP)
    {
      if(x<r.x0)
        x = r.x0;
    }

    if(!(align&(sFA_LEFT|sFA_CLIP)) || xs<r.x1-x)
    {
      Print(handle,x,y,text,col,len);
    }
    else
    {
      xs=0;
      if(align & sFA_SPACE)
        xs += GetWidth(handle," ",1);
      for(i=0;i<len;i++)
      {
        xs += GetWidth(handle,text+i,1);
        if(xs > r.x1-r.x0)
          break;
      }
      Print(handle,x,y,text,col,i);
      clipping = 1;
    }
  }
  return clipping;
}

/****************************************************************************/

sInt sGuiPainter::PrintM(sInt handle,sRect &rect,sInt align,const sChar *text,sU32 col,sInt maxlen)
{
  sInt xs,x,y,h,w;
  sInt len,commit;
  const sChar *textend;

  if(maxlen==-1)
    maxlen = sGetStringLen(text);
  textend = text+maxlen;

  xs = rect.XSize();
  h = GetHeight(handle);
  x = rect.x0;
  y = rect.y0;
  while(text<textend)
  {
    len = 0;
    do
    {
      commit = len;
      if(text[len]=='\n')
      {
        commit++;
        break;
      }
      if(text+len<textend==0)
        break;
      while(text+len<textend && text[len]!=' ' && text[len]!='\n')
        len++;
      w = GetWidth(handle,text,len);
      while(text[len]==' ')
        len++;
    }
    while(w<=xs);

    if(commit==0)
    {
      for(commit=0;text+commit<textend && text[commit]!='\n' && GetWidth(handle,text,commit)<xs;commit++);
      commit--;
      if(commit<1 && text<textend)
        commit=1;
    }

    len = commit;
    while(len>0 && (text[len-1]==' ' || text[len-1]=='\n'))
      len--;
    Print(handle,x,y,text,col,len);
    y+=h;
    text+=commit;
  }

  return y;
}

/****************************************************************************/

void sGuiPainter::PrintMode(sU32 col,sF32 z)
{
  PrintCol = col;
  PrintZ = z;
}

/****************************************************************************/

sInt sGuiPainter::GetWidth(sInt handle,const sChar *text,sInt len)
{
  sInt i;
  sGuiPainterLetter *l;
  sInt w;

  sVERIFY(handle>=0 && handle<MatCount);
  sVERIFY(Mat[handle].Letters);
  if(len==-1) len = sGetStringLen(text);

  w = 0;
  for(i=0;i<len;i++)
  {
    l = &Mat[handle].Letters[text[i]&0xff];
    w += l->PreKern+l->CellWidth+l->PostKern;
  }

  return w;
}

/****************************************************************************/

sInt sGuiPainter::GetHeight(sInt handle)
{
  sVERIFY(handle>=0 && handle<MatCount);
  sVERIFY(Mat[handle].Letters);

  return Mat[handle].Advance;
}

/****************************************************************************/
  
void sGuiPainter::SetMaterial(sInt handle)
{
  sVERIFY(handle>=0 && handle<MatCount);
/*
  if(Mat[handle].States)
  {
    sSystem->SetTexture(0,Mat[handle].Texture);
    sSystem->SetStates(Mat[handle].States);
  }
  else*/
  {
    Mat[handle].Material->Set(MtrlEnv);
  }
}

/****************************************************************************/
/****************************************************************************/

void sGuiPainter::FlushRect(sVertexDouble *&vp,sGuiPainterJob *job,sF32 x0,sF32 y0,sF32 x1,sF32 y1,sF32 u0,sF32 v0,sF32 u1,sF32 v1,sU32 col00,sU32 col01,sU32 col10,sU32 col11)
{
  sRect r;
  if(job->ClipRect)
  {
    r = Rect[job->ClipRect];
    sVERIFY(r.x0<r.x1);
    sVERIFY(r.y0<r.y1);
    if(x1<=r.x0 || x0>=r.x1) return;
    if(y1<=r.y0 || y0>=r.y1) return;
    if(x0>=x1 || y0>=y1) return;
    if(x0<r.x0)
    {
      u0 = u0 + (u1-u0)*(r.x0-x0)/(x1-x0);
      x0 = r.x0;
    }
    if(x1>r.x1)
    {
      u1 = u1 + (u1-u0)*(r.x1-x1)/(x1-x0);
      x1 = r.x1;
    }
    if(y0<r.y0)
    {
      v0 = v0 + (v1-v0)*(r.y0-y0)/(y1-y0);
      y0 = r.y0;
    }
    if(y1>r.y1)
    {
      v1 = v1 + (v1-v0)*(r.y1-y1)/(y1-y0);
      y1 = r.y1;
    }
  }

  vp[0].Init(x0,y0,job->z,col00,u0,v0);
  vp[1].Init(x1,y0,job->z,col01,u1,v0);
  vp[2].Init(x1,y1,job->z,col10,u1,v1);
  vp[3].Init(x0,y1,job->z,col11,u0,v1);
  vp+=4;
}

void sGuiPainter::Flush()
{
  sGuiPainterMaterial *mat;
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;
  sGuiPainterLetter *let;
  sVertexDouble *vp,*vs;
  sInt i,max;
  sF32 xs,ys;
  sF32 u0,u1,v0,v1;
  sF32 xsi,ysi;
  sChar *text;
  sScreenInfo si;
  sU32 col;

  sSystem->GetScreenInfo(0,si);
  sSystem->SetViewProject(&MtrlEnv);

  for(i=0;i<MatCount;i++)
  {
    mat = &Mat[i];

    max = mat->QuadCount;   

    if(max>0)
    {
      job = mat->FirstJob;
      SetMaterial(i);
      sSystem->GeoBegin(GeoHandle,max*4,0,(sF32**)&vp,0);
      vs = vp;

      while(job)
      {
        col = job->Color;
        if(job->ExtPos)
        {
          ext = job->ExtPos;
          if(job->ClipRect)    // set only if job is axxis aligned
          {
            FlushRect(vp,job,ext->XYUV[0][0],ext->XYUV[0][1],ext->XYUV[2][0],ext->XYUV[2][1]
                            ,ext->XYUV[0][2],ext->XYUV[0][3],ext->XYUV[2][2],ext->XYUV[2][3]
                            ,ext->Col[0],ext->Col[1],ext->Col[2],ext->Col[3]);
          }
          else
          {
            vp[0].Init(ext->XYUV[0][0],ext->XYUV[0][1],job->z,ext->Col[0],ext->XYUV[0][2],ext->XYUV[0][3]);
            vp[1].Init(ext->XYUV[1][0],ext->XYUV[1][1],job->z,ext->Col[1],ext->XYUV[1][2],ext->XYUV[1][3]);
            vp[2].Init(ext->XYUV[2][0],ext->XYUV[2][1],job->z,ext->Col[2],ext->XYUV[2][2],ext->XYUV[2][3]);
            vp[3].Init(ext->XYUV[3][0],ext->XYUV[3][1],job->z,ext->Col[3],ext->XYUV[3][2],ext->XYUV[3][3]);
            vp+=4;
          }
        }
        else if(job->Text)
        {
          text = job->Text;
          ys = mat->Height;
          xsi = 1.0f/mat->XSize;
          ysi = 1.0f/mat->YSize;
          while(*text)
          {
            let = &mat->Letters[(*text++)&0xff];
            job->x += let->PreKern;
            xs = let->CellWidth;
            u0 = xsi*let->PosX;
            u1 = xsi*(let->PosX+xs);
            v0 = ysi*let->PosY;
            v1 = ysi*(let->PosY+ys);

            FlushRect(vp,job,job->x,job->y,job->x+xs,job->y+ys,u0,v0,u1,v1,col,col,col,col);
            job->x += xs+let->PostKern;
          }
        }
        else
          FlushRect(vp,job,job->x,job->y,job->x+mat->XSize,job->y+mat->YSize,0,0,1,1,col,col,col,col);
        job = job->Next;
      }

      sSystem->GeoEnd(GeoHandle,(vp-vs));
      sSystem->GeoDraw(GeoHandle);
    }

    mat->LastJob = &mat->FirstJob;
    mat->FirstJob = 0;
    mat->QuadCount=0;
  }

  if(LineCount>0)
  {
    sF32 *fp;

    SetMaterial(0);
    sSystem->GeoBegin(LineHandle,LineCount*2,0,&fp,0);

    for(i=0;i<LineCount;i++)
    {
      *fp++ = Lines[i].x0;
      *fp++ = Lines[i].y0;
      *fp++ = PaintZ;
      *(sU32 *)fp = Lines[i].col; fp++;
      *fp++ = Lines[i].x1;
      *fp++ = Lines[i].y1;
      *fp++ = PaintZ;
      *(sU32 *)fp = Lines[i].col; fp++;
      
      /**fp++ = Lines[i].x1;
      *fp++ = Lines[i].y1;
      *fp++ = PaintZ;
      *(sU32 *)fp = Lines[i].col; fp++;
      *fp++ = Lines[i].x0;
      *fp++ = Lines[i].y0;
      *fp++ = PaintZ;
      *(sU32 *)fp = Lines[i].col; fp++;*/
    }

    sSystem->GeoEnd(LineHandle);
    sSystem->GeoDraw(LineHandle);
  }

  JobCount = 0;
  ExtCount = 0;
  TextCount = 0;
  RectCount = 1;
  LineCount = 0;
  Rect[0].Init(0,0,0,0);
  Clipping = 0;
}

/****************************************************************************/

void sGuiPainter::CheckOverflow()
{
  if(JobCount>=JobAlloc || ExtCount>=ExtAlloc || LineCount>=LineAlloc)
    Flush();
}

/****************************************************************************/

void sGuiPainter::SetClipping(sRect &r)
{
  sVERIFY(RectCount<RectAlloc);
  Rect[RectCount++] = r;
}


/****************************************************************************/

void sGuiPainter::EnableClipping(sBool clip)
{
  if(clip)
    Clipping = RectCount-1;
  else
    Clipping = 0;
}



/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Performance Monitor                                                ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/
/*
sPerfMonToken::sPerfMonToken(sChar *name,sU32 color)
{
  Name = name;
  Color = color;
  Index = -1;
}
*/
sPerfMonZone::sPerfMonZone(sPerfMonToken &tok)
{
  sPerfMonRecord *rec;

  if(sPerfMon->Index<0x7000)
  {
    Token = tok.Index;
    rec = &sPerfMon->Rec[sPerfMon->DBuffer][sPerfMon->Index++];
    rec->Token = Token;
    rec->Time = sSystem->PerfTime();
    rec->Type = 1;

    sVERIFY(Token != -1);
  }
}

sPerfMonZone::~sPerfMonZone()
{
  sPerfMonRecord *rec;

  if(sPerfMon->Index<0x7000)
  {
    sVERIFY(Token != -1);

    rec = &sPerfMon->Rec[sPerfMon->DBuffer][sPerfMon->Index++];
    rec->Token = Token;
    rec->Time = sSystem->PerfTime();
    rec->Type = 2;
    Token = -1;
  }
}

/****************************************************************************/

sPerfMon_::sPerfMon_()
{

// reset

  TokenCount = 0;
  DBuffer = 0;
  Index = 0;
  IndexOld = 0;
  IndexSound = 0;
  Rec[0][0].Type = 0;
  Rec[1][0].Type = 0;
  SoundRec[0][0] = 0xffffffff;
  SoundRec[1][0] = 0xffffffff;

// register default zone

  { 
    static sPerfMonToken __zone_default={"default",0xffffffff,-1}; 
    Register(__zone_default);
  }

// write header

  Rec[DBuffer][Index].Time = sSystem->GetTime();
  Rec[DBuffer][Index].Token = 0;
  Rec[DBuffer][Index].Type = 1;
  Index++;
}

sPerfMon_::~sPerfMon_()
{
}

void sPerfMon_::Flip()
{
  sInt i;

// write footer

  if(Index>=0x7000)
    Index = 1;
  Rec[DBuffer][Index].Time = sSystem->PerfTime();
  Rec[DBuffer][Index].Token = 0;
  Rec[DBuffer][Index].Type = 2;
  Index++;
  Rec[DBuffer][Index].Time = 0;
  Rec[DBuffer][Index].Token = 0;
  Rec[DBuffer][Index].Type = 0;
  Index++;
  SoundRec[DBuffer][IndexSound] = 0xffffffff;
  IndexSound++;
  IndexOld = sMax(IndexOld,Index);

// switch buffers

  DBuffer = 1-DBuffer;
  Index = 0;
  IndexSound = 0;
  sSystem->PerfKalib();

// write header of next frame

  Rec[DBuffer][Index].Time = sSystem->PerfTime();
  Rec[DBuffer][Index].Token = 0;
  Rec[DBuffer][Index].Type = 1;
  Index++;

  for(i=0;i<16;i++)
    Markers[DBuffer][i] = 0;
}

void sPerfMon_::Register(sPerfMonToken &tok)
{
  if(tok.Index==-1)
  {
    sVERIFY(TokenCount<512);
    Tokens[TokenCount].Name = tok.Name;
    Tokens[TokenCount].Color = tok.Color;
    Tokens[TokenCount].EnterTime = 0;
    Tokens[TokenCount].TotalTime = 0;
    Tokens[TokenCount].AverageTime = 0;
    tok.Index = TokenCount++;
  }
}

void sPerfMon_::Marker(sInt index)
{
  Markers[DBuffer][index] = sSystem->PerfTime();
}

/****************************************************************************/
/****************************************************************************/
