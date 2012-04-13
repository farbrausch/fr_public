// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "gentex.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   The Generator                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void TexGen::Init(sInt bytes)
{
  Buffer = new sU8[bytes];
  BufferSize = bytes;
}

/****************************************************************************/

void TexGen::Exit()
{
  if(Buffer)
    delete[] Buffer;
}

/****************************************************************************/

void TexGen::Start(sInt xs,sInt ys,sInt tformat,sInt tilestack)
{
  sU8 *p;

  p = Buffer;
  SizeX = xs;
  SizeY = ys;

  Texture = (sU32 *) p;
  p += SizeX*SizeY*4;

  TData = (sU16 *) p;
  p += TG_TILEX*TG_TILEY*8*tilestack;
  TEnd = (sU16 *) p;

  PData = (sU16 *) p;
  PEnd = (sU16 *) (Buffer+BufferSize);
}

/****************************************************************************/

void TexGen::Start(sBitmap *bm,sInt tilestack)
{
  sU8 *p;

  p = Buffer;
  SizeX = bm->XSize;
  SizeY = bm->YSize;

  Texture = bm->Data;
  p += SizeX*SizeY*4;

  TData = (sU16 *) p;
  p += TG_TILEX*TG_TILEY*8*tilestack;
  TEnd = (sU16 *) p;

  PData = (sU16 *) p;
  PEnd = (sU16 *) (Buffer+BufferSize);
}

/****************************************************************************/

void TexGen::Execute(TexGenStep *step)
{
  TexGenStep *master;
  sInt i,j;
  sU32 *d,*s;
	sInt masterpage;

  do
  {
    master = step;
    sVERIFY(master->Loop > 0);

    master[0].Private = 0;
    for(i=0;i<master->Loop;i++)
      master[i+1].Private = 0;
		masterpage = master->Page;
		if(masterpage<0)
			masterpage = 0;

    for(TileY=0;TileY<SizeY;TileY+=TG_TILEY)
    {
      for(TileX=0;TileX<SizeX;TileX+=TG_TILEY)
      {
        TStack = TData;
        for(i=0;i<master->Loop;i++)
        {
          step = master+i+1;
          sVERIFY(step->Loop == 0);
          (*step->Handler)(this,step);
        }
        s = (sU32 *)Pop();
        sVERIFY(TStack == TData);

        if(master->Handler)
        {
          d = (sU32 *)GetTile(masterpage);
          for(i=0;i<TG_TILEY;i++)
          {
            sCopyMem4(d,s,TG_TILEX*2);
            d+=SizeX*2;
            s+=TG_TILEX*2;
          }
        }
        else
        {
          d = Texture+SizeX*TileY+TileX;
          for(i=0;i<TG_TILEY;i++)
          {
            for(j=0;j<TG_TILEX;j++)
            {
              d[j] = ((s[1]&0x7f800000)<< 1) | ((s[1]&0x00007f80)<<9) |
                     ((s[0]&0x7f800000)>>15) | ((s[0]&0x00007f80)>>7);
              s+=2;
            }
            d+=SizeX;
          }
        }
      }
    }
    if(master->Handler)
      (*master->Handler)(this,master);
    step = master+1+master->Loop;
  }
  while(master->Handler);
}

/****************************************************************************/
/****************************************************************************/

typedef void (*TexGenOp2)(struct TexGen *gen,sU32 *&data);

static TexGenOp2 TexGenOpsPage[] = 
{
  0
};

static TexGenOp2 TexGenOpsTile[] = 
{
  0
};

void TexGen::Execute(sU32 *&data)
{
  sU32 code;
  sU16 *lpend;
  sU32 *ldata;
  sU32 lcode;

  TStack = TData;

  code = *data++;
  for(;;)
  {
    if(code&0x80000000)
    {
      lpend = PEnd;
      ldata = data;
      lcode = code;
      for(TileY=0;TileY<SizeY;TileY+=TG_TILEY)
      {
        for(TileX=0;TileX<SizeX;TileX+=TG_TILEY)
        {
          PEnd = lpend;
          data = ldata;
          code = lcode;
          while(code&0x80000000)
          {
            (*TexGenOpsTile[code&0x7fffffff])(this,data);
            code = *data++;
          }
          sVERIFY(TStack==TData);
        }
      }
    }

    if(code==0)
    {
      break;
    }

    (*TexGenOpsTile[code&0x7fffffff])(this,data);
    code = *data++;
  }
}

/****************************************************************************/
/****************************************************************************/

sU16 *TexGen::GetPage(sInt i)
{
  sVERIFY(i>=0 && PData+(i+1)*SizeX*SizeY*4<=PEnd);
  return PData + i*SizeX*SizeY*4;
}

/****************************************************************************/

sU16 *TexGen::GetTile(sInt i)
{
  return GetPage(i) + TileX*4 + TileY*SizeX*4;
}

/****************************************************************************/

sU32 *TexGen::GetPrivate(sInt i)
{
  PEnd -= i*2;
  return (sU32 *)PEnd;
}


/****************************************************************************/

sU16 *TexGen::Push()
{
  sU16 *r = TData;
  TData+=SizeX*SizeY*4;
  return r;
}

/****************************************************************************/

sU16 *TexGen::Pop()
{
  TData-=SizeX*SizeY*4;
  return TData;
}

/****************************************************************************/

sU16 *TexGen::Tos()
{
  return TData - SizeX*SizeY*4;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Some Operators                                                     ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

static void Fade16(sU16 *d,sU16 *s0,sU16 *s1,sU16 fade)
{
	d[0] = s0[0] + ((((sInt)s1[0]-s0[0])*fade)>>15);
	d[1] = s0[1] + ((((sInt)s1[1]-s0[1])*fade)>>15);
	d[2] = s0[2] + ((((sInt)s1[2]-s0[2])*fade)>>15);
	d[3] = s0[3] + ((((sInt)s1[3]-s0[3])*fade)>>15);
}

/****************************************************************************/

static void Mult16(sU16 *d,sU16 *s0,sU16 *s1)
{
  d[0] = sRange((s0[0]*s1[0])>>14,0x7fff,0);
  d[1] = sRange((s0[1]*s1[1])>>14,0x7fff,0);
  d[2] = sRange((s0[2]*s1[2])>>14,0x7fff,0);
  d[3] = sRange((s0[3]*s1[3])>>14,0x7fff,0);
}

/****************************************************************************/
/***                                                                      ***/
/***   Nop                                                                ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpNop(TexGen *gen,TexGenStep *step)
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Load                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpLoad(TexGen *gen,TexGenStep *step)
{
  sU32 *d;
  sU32 *s;
  sInt y;

  d = (sU32*)gen->Push();
  s = (sU32*)gen->GetTile(step->Page);

  for(y=0;y<TG_TILEY;y++)
  {
    sCopyMem4(d,s,TG_TILEX*2);
    s+=gen->SizeX*2;
    d+=TG_TILEX*2;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Flat                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpFlat(TexGen *gen,TexGenStep *step)
{
  sU16 *d;
  sInt i;
  sU16 cr,cg,cb,ca;
  sU32 *para;

  para = step->Para;
  cb=((sU8 *)para)[0]; cb = (cb<<7)|(cb>>1);
  cg=((sU8 *)para)[1]; cg = (cg<<7)|(cg>>1);
  cr=((sU8 *)para)[2]; cr = (cr<<7)|(cr>>1);
  ca=((sU8 *)para)[3]; ca = (ca<<7)|(ca>>1);

  d = gen->Push();
  for(i=0;i<TG_TILEX*TG_TILEY;i++)
  {
    d[0] = cb;
    d[1] = cg;
    d[2] = cr;
    d[3] = ca;
    d+=4;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Tex2                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpTex2(TexGen *gen,TexGenStep *step)
{
  sU16 *d,*s;
  sInt i;
  sU32 *para;

  para = step->Para;
  s = gen->Pop();
  d = gen->Tos();
  switch(para[0])
  {
  case 0:  // add
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      d[0] = sRange<sInt>(d[0]+s[0],0x7fff,0);
      d[1] = sRange<sInt>(d[1]+s[1],0x7fff,0);
      d[2] = sRange<sInt>(d[2]+s[2],0x7fff,0);
      d[3] = sRange<sInt>(d[3]+s[3],0x7fff,0);
      d+=4;
      s+=4;
    }
    break;
  case 1:  // sub
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      d[0] = sRange<sInt>(s[0]-d[0],0x7fff,0);
      d[1] = sRange<sInt>(s[1]-d[1],0x7fff,0);
      d[2] = sRange<sInt>(s[2]-d[2],0x7fff,0);
      d[3] = sRange<sInt>(s[3]-d[3],0x7fff,0);
      d+=4;
      s+=4;
    }
    break;
  case 2:  // mul
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      d[0] = sRange<sInt>((d[0]*s[0])>>14,0x7fff,0);
      d[1] = sRange<sInt>((d[1]*s[1])>>14,0x7fff,0);
      d[2] = sRange<sInt>((d[2]*s[2])>>14,0x7fff,0);
      d[3] = sRange<sInt>((d[3]*s[3])>>14,0x7fff,0);
      d+=4;
      s+=4;
    }
    break;
  case 3:  // diff
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      d[0] = sRange<sInt>((0x8000+d[0]-s[0])>>1,0x7fff,0);
      d[1] = sRange<sInt>((0x8000+d[1]-s[1])>>1,0x7fff,0);
      d[2] = sRange<sInt>((0x8000+d[2]-s[2])>>1,0x7fff,0);
      d[3] = sRange<sInt>((0x8000+d[3]-s[3])>>1,0x7fff,0);
      d+=4;
      s+=4;
    }
    break;
  case 4:  // alpha
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      d[3] = (s[0]+s[1]+s[1]+s[2])>>2;
      d+=4;
      s+=4;
    }
    break;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Tex1                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpTex1(TexGen *gen,TexGenStep *step)
{
  sU16 *d;
  sInt i;
  sU32 *para;
  sInt cr,cg,cb,ca;

  para = step->Para;
  d = gen->Tos();
  cb = (para[1]>> 0)&0xff; cb = (cb<<7)|(cb>>1);
  cg = (para[1]>> 8)&0xff; cg = (cg<<7)|(cg>>1);
  cr = (para[1]>>16)&0xff; cr = (cr<<7)|(cr>>1);
  ca = (para[1]>>24)&0xff; ca = (ca<<7)|(ca>>1);
  switch(para[0])
  {
  case 0: // mul
    cr>>=7;
    cg>>=7;
    cb>>=7;
    ca>>=7;
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      d[0] = sRange<sInt>((d[0]*cr)>>8,0x7fff,0);
      d[1] = sRange<sInt>((d[1]*cg)>>8,0x7fff,0);
      d[2] = sRange<sInt>((d[2]*cb)>>8,0x7fff,0);
      d[3] = sRange<sInt>((d[3]*ca)>>8,0x7fff,0);
      d+=4;
    }
    break;
  case 1: // add
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      d[0] = sRange<sInt>(d[0]+cr,0x7fff,0);
      d[1] = sRange<sInt>(d[1]+cg,0x7fff,0);
      d[2] = sRange<sInt>(d[2]+cb,0x7fff,0);
      d[3] = sRange<sInt>(d[3]+ca,0x7fff,0);
      d+=4;
    }
    break;
  case 2: // sub
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      d[0] = sRange<sInt>(d[0]-cr,0x7fff,0);
      d[1] = sRange<sInt>(d[1]-cg,0x7fff,0);
      d[2] = sRange<sInt>(d[2]-cb,0x7fff,0);
      d[3] = sRange<sInt>(d[3]-ca,0x7fff,0);
      d+=4;
    }
    break;
  case 3: // gray
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      ca = (d[0]+d[1]+d[1]+d[2])>>2;
      d[0] = ca;
      d[1] = ca;
      d[2] = ca;
      d[3] = 0x7fff;
      d+=4;
    }
    break;
  case 4: // invert
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      d[0] = d[0]^0x7fff;
      d[1] = d[1]^0x7fff;
      d[2] = d[2]^0x7fff;
      d[3] = d[3];
      d+=4;
    }
    break;
  case 5: // scale
    cr>>=7;
    cg>>=7;
    cb>>=7;
    ca>>=7;
    for(i=0;i<TG_TILEX*TG_TILEY;i++)
    {
      d[0] = ((d[0]*cr)>>4)&0x7fff;
      d[1] = ((d[1]*cg)>>4)&0x7fff;
      d[2] = ((d[2]*cb)>>4)&0x7fff;
      d[3] = ((d[3]*ca)>>4)&0x7fff;
      d+=4;
    }
    break;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Glow                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpGlow(TexGen *gen,TexGenStep *step)
{
  struct prv
  {
    sF32 cx,cy,rx,ry,sx,sy;
    sU16 cr,cg,cb,ca;
    sF32 power,alpha;
    sRect cull;
  };
  sF32 cx,cy,rx,ry;
  prv *p;
  sU16 *d;
  sInt x,y;
  sF32 a;
  sInt f;
  sU32 *para;

  para = step->Para;
  p = (prv *)step->Private;
  if(!p)
  {
    step->Private = gen->GetPrivate(sizeof(prv)/4);
    p = (prv *)step->Private;
    p->cx = ((((sInt)para[0])*gen->SizeX)>>16);
    p->cy = ((((sInt)para[1])*gen->SizeY)>>16);
    p->rx = (para[2]*gen->SizeX)>>16;
    p->ry = (para[3]*gen->SizeY)>>16;
    p->sx = ((((sInt)para[7])*gen->SizeX)>>16)/2.0f;
    p->sy = ((((sInt)para[8])*gen->SizeY)>>16)/2.0f;
    p->cb=((sU8 *)&para[4])[0]; p->cb = (p->cb<<7)|(p->cb>>1);
    p->cg=((sU8 *)&para[4])[1]; p->cg = (p->cg<<7)|(p->cg>>1);
    p->cr=((sU8 *)&para[4])[2]; p->cr = (p->cr<<7)|(p->cr>>1);
    p->ca=((sU8 *)&para[4])[3]; p->ca = (p->ca<<7)|(p->ca>>1);
    p->alpha = para[5]*(65536.0f/255.0f);
    p->power = 64*256.0f/(para[6]+1);
    p->power = p->power*p->power;
    p->cull.x0 = sFtol(p->cx-p->rx-p->sx)-TG_TILEX+1;
    p->cull.y0 = sFtol(p->cy-p->ry-p->sy)-TG_TILEY+1;
    p->cull.x1 = sFtol(p->cx+p->rx+p->sx)+1;
    p->cull.y1 = sFtol(p->cy+p->ry+p->sy)+1;
    p->rx = p->rx*p->rx; if(p->rx==0) p->rx=0.0001f;
    p->ry = p->ry*p->ry; if(p->ry==0) p->ry=0.0001f;
  }
  if(p->cull.Hit(gen->TileX,gen->TileY))
  {
    rx = p->cx - gen->TileX;
    ry = p->cy - gen->TileY;

    d = gen->Tos();
    for(y=0;y<TG_TILEY;y++)
    {
      cy = y-ry;
      if(cy<-p->sy) cy=cy+p->sy;
      else if(cy>p->sy) cy=cy-p->sy;
      else cy = 0;

      for(x=0;x<TG_TILEX;x++)
      {
        cx = x-rx;
        if(cx<-p->sx) cx=cx+p->sx;
        else if(cx>p->sx) cx=cx-p->sx;
        else cx = 0;

        a = 1-(cx*cx/p->rx+cy*cy/p->ry);
        if(a>0)
        {
          f = sFtol(sFPow(a,p->power)*p->alpha);
          d[0] = d[0]+(((p->cb-d[0])*f)>>16);
          d[1] = d[1]+(((p->cg-d[1])*f)>>16);
          d[2] = d[2]+(((p->cr-d[2])*f)>>16);
          d[3] = d[3]+(((p->ca-d[3])*f)>>16);
        }
//        d[2]|=0x4000;

        d+=4;
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Dots (obsolete)                                                    ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpDots(TexGen *gen,TexGenStep *step)
{
  sInt i;
  sU16 *d;
  sInt c0[4];
  sInt c1[4];
  sInt f;
  sInt x;
  sU32 *para;

  para = step->Para;

  if(gen->TileX==0 && gen->TileY==0)
  {
    sSetRndSeed(para[3]^0x12345312);
    sGetRnd(1);
    sGetRnd(1);
    sGetRnd(1);
  }

  c0[0] = (para[0]>>16)&0xff; c0[0] = (c0[0]>>1)|(c0[0]<<7);
  c0[1] = (para[0]>> 8)&0xff; c0[1] = (c0[1]>>1)|(c0[1]<<7);
  c0[2] = (para[0]    )&0xff; c0[2] = (c0[2]>>1)|(c0[2]<<7);
  c0[3] = (para[0]>>24)&0xff; c0[3] = (c0[3]>>1)|(c0[3]<<7);
  c1[0] = (para[1]>>16)&0xff; c1[0] = (c1[0]>>1)|(c1[0]<<7); c1[0]-=c0[0];
  c1[1] = (para[1]>> 8)&0xff; c1[1] = (c1[1]>>1)|(c1[1]<<7); c1[1]-=c0[1];
  c1[2] = (para[1]    )&0xff; c1[2] = (c1[2]>>1)|(c1[2]<<7); c1[2]-=c0[2];
  c1[3] = (para[1]>>24)&0xff; c1[3] = (c1[3]>>1)|(c1[3]<<7); c1[3]-=c0[3];

  d = gen->Tos();

  for(i=0;i<(sInt)para[2];i++)
  {
    f = sGetRnd(256);
    x = sGetRnd(TG_TILEX*TG_TILEY);
    d[x*4+0] = c0[0]+c1[0]*f/256;
    d[x*4+1] = c0[1]+c1[1]*f/256;
    d[x*4+2] = c0[2]+c1[2]*f/256;
    d[x*4+3] = c0[3]+c1[3]*f/256;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Blur                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpBlur(TexGen *gen,TexGenStep *step)
{
  sInt x,y,xx;
  sInt size;
  sU32 mask;
  sInt amp;
  sInt shift;
  sInt s1,s2,d;
  sInt akku[4*32];
  sU16 *p,*q,*pp,*qq;
  sInt order,ordercount;
  sU32 *para;


// prepare

  para = step->Para;
  pp = gen->GetPage(step->Page);
  qq = gen->GetPage(0);
  order = para[3];

// blur x

  size = (para[0]*gen->SizeX)>>16;
  amp = (para[2]*256)/sMax(1,size);
  mask = (gen->SizeX-1)*4;

  for(ordercount=0;ordercount<order;ordercount++)
  {
    p = pp;
    q = qq;
    pp = q;
    qq = p;
    for(y=0;y<gen->SizeY;y++)
    {
      s2 = s1 = -((size+1)/2)*4;
      d = 0;
      akku[0] = 0;
      akku[1] = 0;
      akku[2] = 0;
      akku[3] = 0;
      for(x=0;x<size;x++)
      {
        akku[0] += p[(s1++)&mask];
        akku[1] += p[(s1++)&mask];
        akku[2] += p[(s1++)&mask];
        akku[3] += p[(s1++)&mask];
      }
      for(x=0;x<gen->SizeX;x++)
      {
        q[d++] = sRange(sMulShift(akku[0],amp),0x7fff,0);
        q[d++] = sRange(sMulShift(akku[1],amp),0x7fff,0);
        q[d++] = sRange(sMulShift(akku[2],amp),0x7fff,0);
        q[d++] = sRange(sMulShift(akku[3],amp),0x7fff,0);
        akku[0] += p[(s1++)&mask] - p[(s2++)&mask];
        akku[1] += p[(s1++)&mask] - p[(s2++)&mask];
        akku[2] += p[(s1++)&mask] - p[(s2++)&mask];
        akku[3] += p[(s1++)&mask] - p[(s2++)&mask];
      }
      p+=gen->SizeX*4;
      q+=gen->SizeX*4;
    }
  }

// blur y

  size = (para[1]*gen->SizeY)>>16;
  amp = (para[2]*256)/sMax(1,size);
  mask = (gen->SizeY-1)*gen->SizeX*4;
  shift = gen->SizeX*4;

  for(ordercount=0;ordercount<order;ordercount++)
  {
    p = pp;
    q = qq;

    for(xx=0;xx<gen->SizeX;xx+=32)
    {
      s2 = s1 = -((size+1)/2)*shift;
      d = 0;
      p = pp+xx*4;
      q = qq+xx*4;
      for(x=0;x<32;x++)
      {
        akku[x*4+0] = 0;
        akku[x*4+1] = 0;
        akku[x*4+2] = 0;
        akku[x*4+3] = 0;
      }
      for(y=0;y<size;y++)
      {
        for(x=0;x<32;x++)
        {
          akku[x*4+0] += p[x*4+0+(s1&mask)];
          akku[x*4+1] += p[x*4+1+(s1&mask)];
          akku[x*4+2] += p[x*4+2+(s1&mask)];
          akku[x*4+3] += p[x*4+3+(s1&mask)];
        }
        s1+=shift;
      }
      for(y=0;y<gen->SizeY;y++)
      {
        for(x=0;x<32;x++)
        {
          q[x*4+0] = sRange(sMulShift(akku[x*4+0],amp),0x7fff,0);
          q[x*4+1] = sRange(sMulShift(akku[x*4+1],amp),0x7fff,0);
          q[x*4+2] = sRange(sMulShift(akku[x*4+2],amp),0x7fff,0);
          q[x*4+3] = sRange(sMulShift(akku[x*4+3],amp),0x7fff,0);
          akku[x*4+0] += p[x*4+0+(s1&mask)] - p[x*4+0+(s2&mask)];
          akku[x*4+1] += p[x*4+1+(s1&mask)] - p[x*4+1+(s2&mask)];
          akku[x*4+2] += p[x*4+2+(s1&mask)] - p[x*4+2+(s2&mask)];
          akku[x*4+3] += p[x*4+3+(s1&mask)] - p[x*4+3+(s2&mask)];
        }
        s1+=shift;
        s2+=shift;
        q+=shift;
      }
    }
    p = pp;
    q = qq;
    pp = q;
    qq = p;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Mask                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpMask(TexGen *gen,TexGenStep *step)
{
  sU16 *a,*b,*f;
  sInt fade;
  sInt i;

  a = gen->Pop();
  b = gen->Pop();
  f = gen->Tos();

  for(i=0;i<TG_TILEX*TG_TILEY;i++)
  {
    fade = (f[0]+f[1]+f[1]+f[2])>>3;
    f[0] = (a[0]) + (( ((sInt)(b[0])-(a[0]))*fade)>>14);
    f[1] = (a[1]) + (( ((sInt)(b[1])-(a[1]))*fade)>>14);
    f[2] = (a[2]) + (( ((sInt)(b[2])-(a[2]))*fade)>>14);
    f[3] = (a[3]) + (( ((sInt)(b[3])-(a[3]))*fade)>>14);
    a+=4;
    b+=4;
    f+=4;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   HSVC                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpHSCB(TexGen *gen,TexGenStep *step)
{
  sU16 *d;
  sInt i;
  sU32 *para;

  sF32 fh,fs,fc,fb;
//  sF32 Gamma[256];
  sF32 ch,cs,cv,cr,cg,cb,min,max;

  para = step->Para;
  d = gen->Tos();

  fh = para[0]/65536.0f;
  fs = para[1]/16384.0f;
  fc = para[2]/16384.0f;
  fb = para[3]/16384.0f;

  fc = fc*fc;
/*
  for(i=0;i<256;i++)
    Gamma[i] = sFPow(i/256.0f,fc)*65536;
  Gamma[0] = sFPow(0.0000001,fc)*65536;
*/
  for(i=0;i<TG_TILEX*TG_TILEY;i++)
  {

// read, gamma, brightness
/*
    cr = sGamma[d[0]>>8]*fb;
    cg = Gamma[d[1]>>8]*fb;
    cb = Gamma[d[2]>>8]*fb;
    if(fb<0)
    {
      cr = -65535*fb+cr;
      cg = -65535*fb+cg;
      cb = -65535*fb+cb;
    }
*/
    cr = sFPow((d[0]+0.01)/32768.0f,fc)*32768.0f*fb;
    cg = sFPow((d[1]+0.01)/32768.0f,fc)*32768.0f*fb;
    cb = sFPow((d[2]+0.01)/32768.0f,fc)*32768.0f*fb;

// convert to hsv

    max = sMax(cr,sMax(cg,cb));
    min = sMin(cr,sMin(cg,cb));
    
    cv = max;
    if(max==min)
    {
      cs = 0;
      ch = 0;
    }
    else
    {
      cs = (max-min)/max;
      if(cr==max)
        ch = 0+(cg-cb)/(max-min);
      else if(cg==max)
        ch = 2+(cb-cr)/(max-min);
      else if(cb==max)
        ch = 4+(cr-cg)/(max-min);
    }

// hue and saturation

    ch += fh*6;
    cs *= fs;

// convert to rgb

    while(ch<-1) ch+=6;
    while(ch>5) ch-=6;

    max = cv;
    min = max-(cs*max);
    
    if(ch<0)
    {
      cr = max;
      cg = min;
      cb = min-ch*(max-min);
    }
    else if(ch<1)
    {
      cr = max;
      cb = min;
      cg = min+ch*(max-min);
    }
    else if(ch<2)
    {
      cg = max;
      cb = min;
      cr = min-(ch-2)*(max-min);
    }
    else if(ch<3)
    {
      cg = max;
      cr = min;
      cb = min+(ch-2)*(max-min);
    }
    else if(ch<4)
    {
      cb = max;
      cr = min;
      cg = min-(ch-4)*(max-min);
    }
    else if(ch<5)
    {
      cb = max;
      cg = min;
      cr = min+(ch-4)*(max-min);
    }

    d[0] = sRange<sInt>(sFtol(cr),0x7fff,0);
    d[1] = sRange<sInt>(sFtol(cg),0x7fff,0);
    d[2] = sRange<sInt>(sFtol(cb),0x7fff,0);
    d[3] = d[3];
    d+=4;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void TG_OpRotate(TexGen *gen,TexGenStep *step)
{
  sInt x,y;
  sS32 *para;
  sU16 *s,*d;
	sInt xs,ys;

  sU16 *c00,*c01,*c10,*c11;
  sInt m00,m01,m10,m11,m20,m21;
  sInt u,v,u0,u1,v0,v1;
	sU16 buf0[4];
	sU16 buf1[4];

  sF32 rotangle;
  sF32 rotzoom;

// prepare

  para = (sS32 *)step->Para;
  d = gen->GetPage(-step->Page);
  s = gen->GetPage(0);
	xs = gen->SizeX;
	ys = gen->SizeY;

// rotate

  rotangle = para[0]*sPI2F/65536;
	rotzoom = para[1]; if(para[1]==0) rotzoom = 0.01f;
  rotzoom = 64.0f/rotzoom*65536*256;
  m00 = sFtol( sFCos(rotangle)*rotzoom);
  m01 = sFtol( sFSin(rotangle)*rotzoom);
  m10 = sFtol(-sFSin(rotangle)*rotzoom);
  m11 = sFtol( sFCos(rotangle)*rotzoom);
  m20 = sFtol( para[2]*xs - (xs*m00+ys*m10)/2);
  m21 = sFtol( para[3]*ys - (xs*m01+ys*m11)/2);

  for(y=0;y<gen->SizeY;y++)
  {
    for(x=0;x<gen->SizeX;x++)
    {
      u = x*m00+y*m10+m20;
      v = x*m01+y*m11+m21;

			u0= (u>>16)   &(xs-1);
      v0= (v>>16)   &(ys-1);
      u1=((u>>16)+1)&(xs-1);
      v1=((v>>16)+1)&(ys-1);
      c00 = &s[(v0*xs+u0)*4];
      c01 = &s[(v0*xs+u1)*4];
      c10 = &s[(v1*xs+u0)*4];
      c11 = &s[(v1*xs+u1)*4];
			Fade16(buf0,c00,c01,(u&0xffff)>>1);
		  Fade16(buf1,c10,c11,(u&0xffff)>>1);
      Fade16(d,buf0,buf1,(v&0xffff)>>1);
      d+=4;
    }
  }
}

/****************************************************************************/

void TG_OpDistort(TexGen *gen,TexGenStep *step)
{
  sInt x,y;
  sS32 *para;
  sU16 *t,*d;
	sInt xs,ys;

  sU16 *c00,*c01,*c10,*c11;
  sInt u,v,u0,u1,v0,v1;
	sU16 buf0[4];
	sU16 buf1[4];
  sInt bumpx,bumpy;

// prepare

  para = (sS32 *)step->Para;
  d = gen->GetPage(step->Page);
  t = gen->Tos();
	xs = gen->SizeX;
	ys = gen->SizeY;
  bumpx = (para[0]*xs)>>14;
  bumpy = (para[0]*ys)>>14;

// rotate 

  for(y=0;y<TG_TILEY;y++)
  {

    for(x=0;x<TG_TILEX;x++)
    {
      u = ((gen->TileX+x)<<16) + ((t[0]-0x4000)*bumpx);
      v = ((gen->TileY+y)<<16) + ((t[1]-0x4000)*bumpy);

			u0= (u>>16)   &(xs-1);
      v0= (v>>16)   &(ys-1);
      u1=((u>>16)+1)&(xs-1);
      v1=((v>>16)+1)&(ys-1);
      c00 = &d[(v0*xs+u0)*4];
      c01 = &d[(v0*xs+u1)*4];
      c10 = &d[(v1*xs+u0)*4];
      c11 = &d[(v1*xs+u1)*4];
			Fade16(buf0,c00,c01,(u&0xffff)>>1);
		  Fade16(buf1,c10,c11,(u&0xffff)>>1);
      Fade16(t,buf0,buf1,(v&0xffff)>>1);
      t+=4;
    }
  }
}

/****************************************************************************/

void TG_OpNormals(TexGen *gen,TexGenStep *step)
{
  sInt xs,ys;
  sInt x,y;
  sU16 *sx,*sy,*s,*d;
  sInt *para;
  sInt vx,vy,vz;
  sF32 e;

  para = (sS32 *)step->Para;
  d = gen->GetPage(-step->Page);
  sx = sy = s = gen->GetPage(0);
	xs = gen->SizeX;
	ys = gen->SizeY;


  for(y=0;y<ys;y++)
  {
    sy = s;
    for(x=0;x<xs;x++)
    {
      vx = sx[((x-2)&(xs-1))*4]*1
         + sx[((x-1)&(xs-1))*4]*3
         - sx[((x  )&(xs-1))*4]*3
         - sx[((x+1)&(xs-1))*4]*1;      

      vy = sy[((y-2)&(ys-1))*xs*4]*1
         + sy[((y-1)&(ys-1))*xs*4]*3
         - sy[((y  )&(ys-1))*xs*4]*3
         - sy[((y+1)&(ys-1))*xs*4]*1;

      vx = sRange(((vx>>4) * para[0])>>8,0x3fff,-0x3fff);
      vy = sRange(((vy>>4) * para[0])>>8,0x3fff,-0x3fff);
      vz = 0;

      if(para[1])
      {
        vz = (0x3fff*0x3fff)-vx*vx-vy*vy;
        if(vz>0)
        {
//          vz = sFSqrt(vz/16384.0f/16384.0f)*0x3fff;
          vz = sFSqrt(vz);
        }
        else
        {
          e = sFInvSqrt(vx*vx+vy*vy)*0x3fff;
          vx *= e;
          vy *= e;
          vz = 0;
        }
      }

      d[0] = vx+0x4000;
      d[1] = vy+0x4000;
      d[2] = vz+0x4000;
      d[3] = 0xffff;

      d += 4;
      sy += 4;
    }
    sx += xs*4;
  }
}

/****************************************************************************/

void TG_OpBumpLight(TexGen *gen,TexGenStep *step)
{
  sInt xs,ys;
  sInt x,y;
  sU16 *d,*b,*s;
  sS32 *para;

  sU16 diff[4];
  sU16 ambi[4];
  sU16 spec[4];
  sU16 buff[4];
  sInt subcode;
  sF32 px,py,pz;                  // light position
  sF32 dx,dy,dz;                  // spot direction
  sF32 da,db;
  sF32 outer;
  sF32 falloff;
  sF32 bump;
  sF32 spow,samp;
  sF32 amp;
  sF32 e;

  sF32 lx,ly,lz;                  // light -> material
  sF32 nx,ny,nz;                  // material normal
  sF32 hx,hy,hz;                  // halfway vector (specular)
  sF32 df;                        // spot direction factor
  sF32 lf;                        // light factor
  sF32 sf;                        // specular factor

  para = (sS32 *)step->Para;
	xs = gen->SizeX;
	ys = gen->SizeY;

  diff[0] = (para[6]    )&0xff; diff[0] = (diff[0]>>1)|(diff[0]<<7);
  diff[1] = (para[6]>> 8)&0xff; diff[1] = (diff[1]>>1)|(diff[1]<<7);
  diff[2] = (para[6]>>16)&0xff; diff[2] = (diff[2]>>1)|(diff[2]<<7);
  diff[3] = (para[6]>>24)&0xff; diff[3] = (diff[3]>>1)|(diff[3]<<7);
  ambi[0] = (para[7]    )&0xff; ambi[0] = (ambi[0]>>1)|(ambi[0]<<7);
  ambi[1] = (para[7]>> 8)&0xff; ambi[1] = (ambi[1]>>1)|(ambi[1]<<7);
  ambi[2] = (para[7]>>16)&0xff; ambi[2] = (ambi[2]>>1)|(ambi[2]<<7);
  ambi[3] = (para[7]>>24)&0xff; ambi[3] = (ambi[3]>>1)|(ambi[3]<<7);
  spec[0] = (para[8]    )&0xff; spec[0] = (spec[0]>>1)|(spec[0]<<7);
  spec[1] = (para[8]>> 8)&0xff; spec[1] = (spec[1]>>1)|(spec[1]<<7);
  spec[2] = (para[8]>>16)&0xff; spec[2] = (spec[2]>>1)|(spec[2]<<7);
  spec[3] = (para[8]>>24)&0xff; spec[3] = (spec[3]>>1)|(spec[3]<<7);

  subcode = para[0];
  px = 1.0f*para[1]*xs/0x8000-(xs/2);
  py = 1.0f*para[2]*ys/0x8000-(ys/2);
  pz = 1.0f*para[3]*xs/0x10000+1;
  da = para[4]*sPI2F/0x10000;
  db = para[5]*sPIF/0x10000;
  dx = sFSin(da)*sFCos(db);
  dy = sFCos(da)*sFCos(db);
  dz = sFSin(db);
  outer = (para[9]+1.0f)/0x10000;
  falloff = para[10]*1.0f/0x4000;
  bump = (16384.0f*256.0f*128.0f)/(para[11]+1.0f);
  amp = para[12]<<1;
  spow = (para[13]+1)/512.0f;
  samp = para[14]<<1;
  
  if(para[15])
  {
    s = gen->Pop();
    d = b = gen->Tos();
  }
  else
  {
    s = d = gen->Tos();
    b = 0;
  }


  lf = 1.0f;
  sf = 0.0f;
  df = 1.0f;
  nx = 0;
  ny = 0;
  nz = 1;
  lx = dx;
  ly = dy;
  lz = dz;

  if(subcode==0)
  {
    px = px-dx*pz/dz;
    py = py-dy*pz/dz;
  }

  for(y=0;y<TG_TILEY;y++)
  {
    for(x=0;x<TG_TILEX;x++)
    {

      if(subcode!=2)
      {
        lx = x+gen->TileX-px;
        ly = y+gen->TileY-py;
        lz = pz;
        e = 1/sFSqrt(lx*lx+ly*ly+lz*lz);
        lx*=e;
        ly*=e;
        lz*=e;
      }

      if(b)
      {
        nx = (b[0]-0x4000)/16384.0f;
        ny = (b[1]-0x4000)/16384.0f;
        nz = (b[2]-0x4000)/16384.0f;
/*
        e = 1/sFSqrt(nx*nx+ny*ny+nz*nz);
        nx*=e;
        ny*=e;
        nz*=e;
*/
        b+=4;
      }

      lf = lx*nx+ly*ny+lz*nz;

      if(samp)
      {
        hx = lx;
        hy = ly;
        hz = lz+1;
        e = 1/sFSqrt(hx*hx+hy*hy+hz*hz);
        hx*=e;
        hy*=e;
        hz*=e;
        sf = hx*nx+hy*ny+hz*nz;
        sf = sFPow(sf,spow);
      }

      if(subcode==0)
      {
        df = (lx*dx+ly*dy+lz*dz);
        if(df<outer)
          df = 0;
        else
          df = sFPow((df-outer)/(1-outer),falloff);
      }
      
      Fade16(buff,ambi,diff,sRange<sInt>(sFtol(df*lf*amp),0x7fff,0));
      if(s)
      {
        Mult16(buff,buff,s);
        s+=4;
      }
      Fade16(d,buff,spec,sRange<sInt>(sFtol(df*sf*samp),0x7fff,0));
      d+=4;
    }
  }
}

/****************************************************************************/
/****************************************************************************/

