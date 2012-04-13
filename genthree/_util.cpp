// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "_util.hpp"
#include "_start.hpp"

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
/****************************************************************************/
/***                                                                      ***/
/***   GuiPainter - draw sprites and print text                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sGuiPainter::sGuiPainter()
{
  sU32 *p;

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
 
  PaintZ = 0.0f;
  PrintZ = 0.0f;
  PaintCol = ~0;
  PrintCol = 0xff000000;

  GeoHandle = sSystem->GeoAdd(sFVF_DEFAULT2D,sGEO_QUAD);
  LineHandle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE);

  Clipping = 0;

  p = TexMat;
  sSystem->StateBase(p,sMBF_ZON,sMBM_TEX,0);
  sSystem->StateTex(p,0,sMTF_CLAMP);
  sSystem->StateEnd(p,TexMat,sizeof(TexMat));

  p = FlatMat;
  sSystem->StateBase(p,sMBF_ZON,sMBM_FLAT,0);
  sSystem->StateEnd(p,FlatMat,sizeof(FlatMat));

  p = AlphaMat;
  sSystem->StateBase(p,sMBF_ZON|sMBF_BLENDALPHA,sMBM_TEX,0);
  sSystem->StateTex(p,0,sMTF_CLAMP);
  sSystem->StateEnd(p,AlphaMat,sizeof(AlphaMat));
}

/****************************************************************************/

sGuiPainter::~sGuiPainter()
{
  sInt i;
  if(Mat)
  {
    for(i=0;i<MatCount;i++)
      if(Mat[i].Letters)
        delete[] Mat[i].Letters;
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
  sGuiPainterMaterial *mm;

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

  mm = &Mat[MatCount++];
  mm->FirstJob = 0;
  mm->LastJob = &mm->FirstJob;
  mm->Letters = 0;
  mm->Height = 0;
  mm->Advance = 0;
  mm->BaseLine = 0;
  mm->States = FlatMat;
  mm->Texture = 0;
  mm->QuadCount = 0;
  mm->XSize = 1;
  mm->YSize = 1;
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
  handle = LoadMaterial(AlphaMat,tex,bm->XSize,bm->YSize);
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
  return LoadMaterial(TexMat,tex,bm->XSize,bm->YSize);
}

/****************************************************************************/

sInt sGuiPainter::LoadBitmapAlpha(sBitmap *bm)
{
  sTexInfo ti;
  sInt tex;

  ti.Init(bm);
  tex = sSystem->AddTexture(ti);
  sVERIFY(sISHANDLE(tex));
  return LoadMaterial(AlphaMat,tex,bm->XSize,bm->YSize);
}

/****************************************************************************/

sInt sGuiPainter::LoadMaterial(sU32 *data,sInt tex,sInt xs,sInt ys)
{
  sGuiPainterMaterial *mat;
  sVERIFY(MatCount<MatAlloc);

  if(data==0)
    data = TexMat;

  mat = &Mat[MatCount];
  mat->FirstJob = 0;
  mat->LastJob = &mat->FirstJob;
  mat->Letters = 0;
  mat->Height = 0;
  mat->Advance = 0;
  mat->BaseLine = 0;
  mat->States = data;
  mat->Texture = tex;
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

  job = &Job[JobCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = x;
  job->y = y;
  job->z = PaintZ;
  job->Color = col==0 ? PaintCol : col;
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

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(ExtCount<ExtAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  job = &Job[JobCount++];
  ext = &Ext[ExtCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = x;
  job->y = y;
  job->z = PaintZ;
  job->Color = col==0 ? PaintCol : col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = Clipping;
  ext->XYUV[0][0]=x   ; ext->XYUV[0][1]=y   ; ext->XYUV[0][2]=0; ext->XYUV[0][3]=0;
  ext->XYUV[1][0]=x+xs; ext->XYUV[1][1]=y   ; ext->XYUV[1][2]=1; ext->XYUV[1][3]=0;
  ext->XYUV[2][0]=x+xs; ext->XYUV[2][1]=y+ys; ext->XYUV[2][2]=1; ext->XYUV[2][3]=1;
  ext->XYUV[3][0]=x   ; ext->XYUV[3][1]=y+ys; ext->XYUV[3][2]=0; ext->XYUV[3][3]=1;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::Paint(sInt handle,sRect r,sU32 col)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(ExtCount<ExtAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  job = &Job[JobCount++];
  ext = &Ext[ExtCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = r.x0;
  job->y = r.y0;
  job->z = PaintZ;
  job->Color = col==0 ? PaintCol : col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = Clipping;
  ext->XYUV[0][0]=r.x0; ext->XYUV[0][1]=r.y0; ext->XYUV[0][2]=0; ext->XYUV[0][3]=0;
  ext->XYUV[1][0]=r.x1; ext->XYUV[1][1]=r.y0; ext->XYUV[1][2]=1; ext->XYUV[1][3]=0;
  ext->XYUV[2][0]=r.x1; ext->XYUV[2][1]=r.y1; ext->XYUV[2][2]=1; ext->XYUV[2][3]=1;
  ext->XYUV[3][0]=r.x0; ext->XYUV[3][1]=r.y1; ext->XYUV[3][2]=0; ext->XYUV[3][3]=1;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::Paint(sInt handle,sRect &pos,sFRect &uv,sU32 col)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

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
  job->Color = col==0 ? PaintCol : col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = Clipping;
  ext->XYUV[0][0]=pos.x0; ext->XYUV[0][1]=pos.y0; ext->XYUV[0][2]=uv.x0; ext->XYUV[0][3]=uv.y0;
  ext->XYUV[1][0]=pos.x1; ext->XYUV[1][1]=pos.y0; ext->XYUV[1][2]=uv.x1; ext->XYUV[1][3]=uv.y0;
  ext->XYUV[2][0]=pos.x1; ext->XYUV[2][1]=pos.y1; ext->XYUV[2][2]=uv.x1; ext->XYUV[2][3]=uv.y1;
  ext->XYUV[3][0]=pos.x0; ext->XYUV[3][1]=pos.y1; ext->XYUV[3][2]=uv.x0; ext->XYUV[3][3]=uv.y1;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

void sGuiPainter::Paint(sInt handle,sInt *pos,sF32 *uv,sU32 col)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

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
  job->Color = col==0 ? PaintCol : col;
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
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::PaintF(sInt handle,sF32 x,sF32 y,sU32 col)
{
  sGuiPainterJob *job;

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  job = &Job[JobCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = x;
  job->y = y;
  job->z = PaintZ;
  job->Color = col==0 ? PaintCol : col;
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

  sVERIFY(JobCount<JobAlloc);
  sVERIFY(ExtCount<ExtAlloc);
  sVERIFY(handle>=0 && handle<MatCount);

  job = &Job[JobCount++];
  ext = &Ext[ExtCount++];
  job->Next = 0;
  job->Handle = handle;
  job->x = x;
  job->y = y;
  job->z = PaintZ;
  job->Color = col==0 ? PaintCol : col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = Clipping;
  ext->XYUV[0][0]=x   ; ext->XYUV[0][1]=y   ; ext->XYUV[0][2]=0; ext->XYUV[0][3]=0;
  ext->XYUV[1][0]=x+xs; ext->XYUV[1][1]=y   ; ext->XYUV[1][2]=1; ext->XYUV[1][3]=0;
  ext->XYUV[2][0]=x+xs; ext->XYUV[2][1]=y+ys; ext->XYUV[2][2]=1; ext->XYUV[2][3]=1;
  ext->XYUV[3][0]=x   ; ext->XYUV[3][1]=y+ys; ext->XYUV[3][2]=0; ext->XYUV[3][3]=1;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::PaintF(sInt handle,sF32 x0,sF32 y0,sF32 x1,sF32 y1,sF32 u0,sF32 v0,sF32 u1,sF32 v1,sU32 col)
{
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;

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
  job->Color = col==0 ? PaintCol : col;
  job->Text = 0;
  job->ExtPos = ext;
  job->ClipRect = Clipping;
  ext->XYUV[0][0]=x0; ext->XYUV[0][1]=y0; ext->XYUV[0][2]=u0; ext->XYUV[0][3]=v0;
  ext->XYUV[1][0]=x1; ext->XYUV[1][1]=y0; ext->XYUV[1][2]=u1; ext->XYUV[1][3]=v0;
  ext->XYUV[2][0]=x1; ext->XYUV[2][1]=y1; ext->XYUV[2][2]=u1; ext->XYUV[2][3]=v1;
  ext->XYUV[3][0]=x0; ext->XYUV[3][1]=y1; ext->XYUV[3][2]=u0; ext->XYUV[3][3]=v1;
  *Mat[handle].LastJob = job;
  Mat[handle].LastJob = &job->Next;
  Mat[handle].QuadCount++;
}

/****************************************************************************/

void sGuiPainter::Line(sInt x0,sInt y0,sInt x1,sInt y1,sU32 col)
{
  sGuiPainterLine *line;
  sRect r;

  sVERIFY(LineCount<LineAlloc);

  if(Clipping)
  {
    r = Rect[RectCount-1];
    if(!r.Hit(x0,y0)) return;
    if(!r.Hit(x1,y1)) return;
  }
  line = &Lines[LineCount++];
  line->x0 = x0+0.5f;
  line->y0 = y0+0.5f;
  line->x1 = x1+0.5f;
  line->y1 = y1+0.5f;
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

void sGuiPainter::Print(sInt handle,sInt x,sInt y,sChar *text,sU32 col,sInt len)
{
  if(len==-1) len = sGetStringLen(text);

  sGuiPainterJob *job;

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

  sCopyMem(Text+TextCount,text,len);
  TextCount+=len;
  Text[TextCount++]=0;

  if(Mat[handle].QuadCount>0x1800)
    Flush();
}

/****************************************************************************/

void sGuiPainter::PrintC(sInt handle,sRect &r,sInt align,sChar *text,sU32 col,sInt len)
{
  sF32 x,y,xs,ys;
  sInt i;

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

    if(!(align&sFA_LEFT) || xs<r.x1-x)
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
    }
  }
}

/****************************************************************************/

sInt sGuiPainter::PrintM(sInt handle,sRect &rect,sInt align,sChar *text,sU32 col,sInt maxlen)
{
  sInt xs,x,y,h,w;
  sInt len,commit;
  sChar *textend;

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

sInt sGuiPainter::GetWidth(sInt handle,sChar *text,sInt len)
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

  sSystem->SetTexture(0,Mat[handle].Texture);
  sSystem->SetStates(Mat[handle].States);
}

/****************************************************************************/
/****************************************************************************/

void sGuiPainter::FlushRect(sVertex2d *&vp,sGuiPainterJob *job,sU32 col,sF32 x0,sF32 y0,sF32 x1,sF32 y1,sF32 u0,sF32 v0,sF32 u1,sF32 v1)
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

  vp[0].Init(x0,y0,job->z,col,u0,v0);
  vp[1].Init(x1,y0,job->z,col,u1,v0);
  vp[2].Init(x1,y1,job->z,col,u1,v1);
  vp[3].Init(x0,y1,job->z,col,u0,v1);
  vp+=4;
}

void sGuiPainter::Flush()
{
  sGuiPainterMaterial *mat;
  sGuiPainterJob *job;
  sGuiPainterExtPos *ext;
  sGuiPainterLetter *let;
  sVertex2d *vp,*vs;
  sInt i,max;
  sF32 xs,ys;
  sF32 u0,u1,v0,v1;
  sF32 xsi,ysi;
  sChar *text;
  sBool swap;
  sScreenInfo si;
  sU32 col;

  sSystem->GetScreenInfo(0,si);
  swap = si.SwapVertexColor;

  for(i=0;i<MatCount;i++)
  {
    mat = &Mat[i];

    max = mat->QuadCount;   

    if(max>0)
    {
      job = mat->FirstJob;
      sSystem->SetCameraOrtho();
      sSystem->SetTexture(0,mat->Texture);
      sSystem->SetStates(mat->States);
      sSystem->GeoBegin(GeoHandle,max*4,0,(sF32**)&vp,0);
      vs = vp;

      while(job)
      {
        col = job->Color;
        if(swap)
          col = (col&0xff00ff00) | ((col&0x00ff0000)>>16) | ((col&0x000000ff)<<16);
        if(job->ExtPos)
        {
          ext = job->ExtPos;
          if(job->ClipRect)    // set only if job is axxis aligned
          {
            FlushRect(vp,job,col,ext->XYUV[0][0],ext->XYUV[0][1],ext->XYUV[2][0],ext->XYUV[2][1]
                                ,ext->XYUV[0][2],ext->XYUV[0][3],ext->XYUV[2][2],ext->XYUV[2][3]);
          }
          else
          {
            vp[0].Init(ext->XYUV[0][0],ext->XYUV[0][1],job->z,col,ext->XYUV[0][2],ext->XYUV[0][3]);
            vp[1].Init(ext->XYUV[1][0],ext->XYUV[1][1],job->z,col,ext->XYUV[1][2],ext->XYUV[1][3]);
            vp[2].Init(ext->XYUV[2][0],ext->XYUV[2][1],job->z,col,ext->XYUV[2][2],ext->XYUV[2][3]);
            vp[3].Init(ext->XYUV[3][0],ext->XYUV[3][1],job->z,col,ext->XYUV[3][2],ext->XYUV[3][3]);
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
            u0 = xsi*0.5f+xsi*let->PosX;
            u1 = xsi*0.5f+xsi*(let->PosX+xs);
            v0 = ysi*0.5f+ysi*let->PosY;
            v1 = ysi*0.5f+ysi*(let->PosY+ys);

            FlushRect(vp,job,col,job->x,job->y,job->x+xs,job->y+ys,u0,v0,u1,v1);
/*
            vp[0].Init(job->x   ,job->y   ,job->z,job->Color,u0,v0);
            vp[1].Init(job->x+xs,job->y   ,job->z,job->Color,u1,v0);
            vp[2].Init(job->x+xs,job->y+ys,job->z,job->Color,u1,v1);
            vp[3].Init(job->x   ,job->y+ys,job->z,job->Color,u0,v1);
            vp+=4;
*/
            job->x += xs+let->PostKern;
          }
        }
        else
        {       
          FlushRect(vp,job,col,job->x,job->y,job->x+mat->XSize,job->y+mat->YSize,0,0,1,1);
/*
          vp[0].Init(job->x           ,job->y           ,job->z,job->Color,0,0);
          vp[1].Init(job->x+mat->XSize,job->y           ,job->z,job->Color,1,0);
          vp[2].Init(job->x+mat->XSize,job->y+mat->YSize,job->z,job->Color,1,1);
          vp[3].Init(job->x           ,job->y+mat->YSize,job->z,job->Color,0,1);
          vp+=4;
*/
        }
        job = job->Next;
      }
//      max = (vp-vs)/4;
/*
      for(j=0;j<max;j++)
        sQuad(ip,j*4+0,j*4+1,j*4+2,j*4+3);
*/
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

    sSystem->SetStates(FlatMat);
    sSystem->GeoBegin(LineHandle,LineCount*4,0,&fp,0);

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
      *fp++ = Lines[i].x1;
      *fp++ = Lines[i].y1;
      *fp++ = PaintZ;
      *(sU32 *)fp = Lines[i].col; fp++;
      *fp++ = Lines[i].x0;
      *fp++ = Lines[i].y0;
      *fp++ = PaintZ;
      *(sU32 *)fp = Lines[i].col; fp++;
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

  Token = tok.Index;
  rec = &sPerfMon->Rec[sPerfMon->DBuffer][sPerfMon->Index++];
  rec->Token = Token;
  rec->Time = sSystem->PerfTime();
  rec->Type = 1;
}

sPerfMonZone::~sPerfMonZone()
{
  sPerfMonRecord *rec;

  rec = &sPerfMon->Rec[sPerfMon->DBuffer][sPerfMon->Index++];
  rec->Token = Token;
  rec->Time = sSystem->PerfTime();
  rec->Type = 2;
  Token = -1;
}

/****************************************************************************/

sPerfMon_::sPerfMon_()
{

// reset

  TokenCount = 0;
  DBuffer = 0;
  Index = 0;
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

// write footer

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

// switch buffers

  sVERIFY(Index<0x6fff);
  DBuffer = 1-DBuffer;
  Index = 0;
  IndexSound = 0;
  sSystem->PerfKalib();

// write header of next frame

  Rec[DBuffer][Index].Time = sSystem->PerfTime();
  Rec[DBuffer][Index].Token = 0;
  Rec[DBuffer][Index].Type = 1;
  Index++;
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

/****************************************************************************/
/****************************************************************************/
