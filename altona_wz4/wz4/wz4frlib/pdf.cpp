/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "pdf.hpp"
#include "pdf_ops.hpp"

/****************************************************************************/

#define MULTICORE  1

/****************************************************************************/

sMatrix34 Wz4PDF::Camera;

/****************************************************************************/

Wz4PDF::Wz4PDF()
{
  Type = Wz4PDFType;  
  Obj  = 0;
}

Wz4PDF::~Wz4PDF()
{
  if (Obj)
    Obj->Release();
}

sBool Wz4PDF::TraceRay(sVector31 &p, sVector30 &n, const sRay &ray, const sF32 md, const sF32 mx, const sInt mi)
{   
  sInt i=0;
  sF32 d=10000;  
  sF32 id=0;
  sVector31 np;
  sVector31 tp = ray.Start;  

  do
  {
    p = tp;
    sF32 td = Obj->GetDistance(p);          
    if (td<=d)
    {
      d=td;  
      np=p;
    }
    tp = tp + ray.Dir * sAbs(td);
    id+= sAbs(td);
  }while (d>md && i++!=mi && id<mx);
   
  if (d<=md)
    Obj->GetNormal(np,n);

  return d<=md;
}


/****************************************************************************/

sF32 Wz4PDFObj::ND = 1.0f/65536.0f; //just a value

/****************************************************************************/

Wz4PDFObj::Wz4PDFObj()
{
}

Wz4PDFObj::~Wz4PDFObj()
{
}

sBool Wz4PDFObj::IsIn(const sVector31 &p)
{
  return GetDistance(p)<=0.0f;
}

void Wz4PDFObj::GetNormal(const sVector31 &p, sVector30 &n)
{
  sVector31 np[6];
  sF32 nd[6];

  for (int i=0;i<6;i++)
  {
    np[i] = p;
  }

  np[0].x -= ND;
  np[1].x += ND;
  np[2].y -= ND;
  np[3].y += ND;
  np[4].z -= ND;
  np[5].z += ND;

  
  for (int i=0;i<6;i++)
  {
    nd[i] = GetDistance(np[i]);
  }

  n.x = nd[0] - nd[1];
  n.y = nd[2] - nd[3];
  n.z = nd[4] - nd[5];
  n.Unit();            
}

/****************************************************************************/

Wz4PDFCube::Wz4PDFCube()
{
}

Wz4PDFCube::~Wz4PDFCube()
{
}


/*
  ------
  |    |
  |    |
  ------  *
*/

sF32 Wz4PDFCube::GetDistance(const sVector31 &p)
{

  sF32 t=sAbs(p.x);  
  t = sMax(t,sAbs(p.y));
  t = sMax(t,sAbs(p.z));
  t -= 0.5f;

  if ((p.x>0.5f || p.x<-0.5f) &&
      (p.y>0.5f || p.y<-0.5f) &&
      (p.z>0.5f || p.z<-0.5f))
  {
    sF32 x=p.x>0.0f ? 0.5f:-0.5f;
    sF32 y=p.y>0.0f ? 0.5f:-0.5f;
    sF32 z=p.z>0.0f ? 0.5f:-0.5f;
    x-=p.x;
    y-=p.y;
    z-=p.z;
    t=sSqrt(x*x+y*y+z*z);
  }
 
  return t;
}

/****************************************************************************/

Wz4PDFAdd::Wz4PDFAdd()
{
}

Wz4PDFAdd::~Wz4PDFAdd()
{
  for (sInt i=0;i<Array.GetCount();i++)
  {
    Array[i]->Release();
  }
}

sF32 Wz4PDFAdd::GetDistance(const sVector31 &p)
{
  if (Array.GetCount())
  {
    sF32 d=Array[0]->GetDistance(p);

    for (sInt i=1;i<Array.GetCount();i++)
    {
      sF32 t=Array[i]->GetDistance(p);
      d = sMin(t,d);
    }  
    return d;
  }
  else
    return 0;
}

void Wz4PDFAdd::GetNormal(const sVector31 &p, sVector30 &n)
{
  int j=0;

  if (Array.GetCount())
  {

    sF32 d = Array[0]->GetDistance(p);
    for (sInt i=1;i<Array.GetCount();i++)
    {
      sF32 td  = Array[i]->GetDistance(p);
      if (td<d)
      {
        d=td;
        j=i;
      }
    }  
    Array[j]->GetNormal(p,n);
  }
  else
  {
    n=sVector30(0.0f,0.0f,0.0f);
  }  
}

void Wz4PDFAdd::AddObj(Wz4PDFObj *obj)
{
  Array.AddTail(obj);
}

/****************************************************************************/

Wz4PDFTransform::Wz4PDFTransform()
{ 
}

Wz4PDFTransform::~Wz4PDFTransform()
{
  Obj->Release();
}

sF32 Wz4PDFTransform::GetDistance(const sVector31 &p)
{
  sVector31 np;
  Modify(p,np);
  return Obj->GetDistance(np);
}

void Wz4PDFTransform::GetNormal(const sVector31 &p, sVector30 &n)
{
  sVector31 np;
  Modify(p,np);
  Obj->GetNormal(np,n);
}

void Wz4PDFTransform::Init(Wz4PDFObj *obj, sVector31 &scale, sVector30 &rot, sVector31 &trans)
{
  Obj=obj;
  Scale=scale;
  Rot=rot;
  Trans=trans;
  sSRT srt;
  srt.Scale = Scale;  
  srt.Rotate = Rot;
  srt.Translate = Trans;
  srt.Invert();
  srt.MakeMatrix(Mat);  
}

/****************************************************************************/

Wz4PDFSphere::Wz4PDFSphere()
{
}

Wz4PDFSphere::~Wz4PDFSphere()
{
}

sF32 Wz4PDFSphere::GetDistance(const sVector31 &p)
{
  return sqrtf(p.x*p.x + p.y*p.y + p.z*p.z) - 1.0f;  
}

/****************************************************************************/

Wz4PDFTwirl::Wz4PDFTwirl()
{ 
}

Wz4PDFTwirl::~Wz4PDFTwirl()
{
  Obj->Release();
}

sF32 Wz4PDFTwirl::GetDistance(const sVector31 &p)
{
  sVector31 np;
  Modify(p,np);
  return Obj->GetDistance(np);
}

void Wz4PDFTwirl::Init(Wz4PDFObj *obj, sVector30 &scale, sVector30 &bias)
{
  Obj=obj;
  Scale=scale;
  Bias=bias;  
}

/****************************************************************************/

Wz4PDFFromADF::Wz4PDFFromADF(Wz4ADF *adf)
{
  ADF=adf;
}

Wz4PDFFromADF::~Wz4PDFFromADF()
{
}

sF32 Wz4PDFFromADF::GetDistance(const sVector31 &p)
{
  return ADF->GetObj()->GetDistance(p);
}

void Wz4PDFFromADF::GetNormal(const sVector31 &p, sVector30 &n)
{
  ADF->GetObj()->GetNormal(p,n);
}

/****************************************************************************/

Wz4PDFMerge::Wz4PDFMerge()
{
}

Wz4PDFMerge::~Wz4PDFMerge()
{
  Obj1->Release();
  Obj2->Release();
}

void Wz4PDFMerge::Init(Wz4PDFObj *obj1, Wz4PDFObj *obj2, sInt type, sF32 factor)
{
  Obj1=obj1;
  Obj2=obj2;
  Type=type;
  Factor=factor;
}

sF32 Wz4PDFMerge::GetDistance(const sVector31 &p)
{
  sF32 d1=Obj1->GetDistance(p);
  sF32 d2=Obj2->GetDistance(p);
  sF32 d;
  switch(Type)
  {
    case 5 :
      if (d1<0)
        d1=0;
      if (d2<0)
        d2=0;
      d=d1*(1.0f-Factor)+d2*Factor;
    break;
    case 4 :              
      if (d1<d2)
      {
        d=d1*(1.0f-Factor)+d2*Factor;
      }
      else
      {
        d=d2*(1.0f-Factor)+d1*Factor;
      }
    break;

    case 3 :
        d=d1*(1.0f-Factor)+d2*Factor;
    break;

    case 2 :
      d1=-d1;
      d = sMax(d1,d2);
    break;

    case 1 :
      d = sMax(d1,d2);
    break;
    default :
      d = sMin(d1,d2);
    break;
  }
  return d;
}

/****************************************************************************/

struct tPDF_Render
{
public:
  sVector30 dnx;
  sVector30 dny;
  sVector31 px;  
  sVector31 cp;
  sImage  *img;  
  wPaintInfo *pi;
  Wz4PDF  *pdf;
};

void TaskCodePDF(sStsManager *m,sStsThread *th,sInt start,sInt count,void *data)
{
  sVERIFY(count==1);
  tPDF_Render *mi = (tPDF_Render *)data;
  sRay ray;
  sU32 *ptr = mi->img->Data;

  ptr+=start*mi->img->SizeX;

  for(sInt x=0;x<mi->img->SizeX;x++)
  {
    sVector30 norm;
    sVector31 pos;

    ray.Start = mi->px + mi->dnx*x + mi->dny*start;
    ray.Dir = ray.Start-mi->cp; 
    ray.Dir.Unit();


    if (mi->pdf->TraceRay(pos,norm,ray))
    {
      norm = (norm + sVector30(1.0f, 1.0f, 1.0f)) * 0.5f;
      unsigned int r = norm.x * 255;
      unsigned int g = norm.y * 255;
      unsigned int b = norm.z * 255;
      *ptr++ = 0xff0000000|(r<<16)|(g<<8)|(b<<0);
    }
    else
    {
      *ptr++ = 0xff000000; 
    }
  }
}

void Wz4PDFObj_Render(sImage *img, Wz4PDF *pdf, wPaintInfo &pi)
{  
  sRay ray;
  sVector30 normal, dir;

  Wz4PDF::Camera = pi.View->Camera;

  sRay rtl;
  sRay rtr;
  sRay rtb;
  sRay rcp;
  pi.View->MakeRay(-1, 1,rtl);
  pi.View->MakeRay( 1, 1,rtr);
  pi.View->MakeRay(-1,-1,rtb);
  pi.View->MakeRay(0,0,rcp);
    
  sVector30 dnx= (rtr.Start - rtl.Start) * (1.0f/img->SizeX);
  sVector30 dny= (rtb.Start - rtl.Start) * (1.0f/img->SizeY);
  sVector31 px=rtl.Start;  
  sVector31 cp=pi.View->Camera.l;// rcp.Start;

#if MULTICORE
  tPDF_Render mi;

  mi.dnx=dnx;
  mi.dny=dny;
  mi.px=px;
  mi.cp=cp;
  mi.img=img;
  mi.pdf=pdf;
  mi.pi=&pi;
  
  sStsWorkload *wl = sSched->BeginWorkload();
  sStsTask *task = wl->NewTask(TaskCodePDF,&mi,img->SizeY,0);  //sSched->NewTask(TaskCodePDF,&mi,img->SizeY,0);
  wl->AddTask(task);
  wl->Start();
  wl->Sync();
  wl->End();
#else
  sU32 *ptr = img->Data;

  for(sInt y=0;y<img->SizeY;y++)
  {
    for(sInt x=0;x<img->SizeX;x++)
    {
      sVector30 norm;
      sVector31 pos;

      ray.Start = px + dnx*x + dny*y;
      ray.Dir = ray.Start-cp; 
      ray.Dir.Unit();


      if (pdf->TraceRay(pos,norm,ray))
      {
        norm = (norm + sVector30(1.0f, 1.0f, 1.0f)) * 0.5f;
        unsigned int r = norm.x * 255;
        unsigned int g = norm.y * 255;
        unsigned int b = norm.z * 255;
        *ptr++ = 0xff0000000|(r<<16)|(g<<8)|(b<<0);
      }
      else
      {
        *ptr++ = 0xff000000; 
      }
    }
  }
#endif
}



