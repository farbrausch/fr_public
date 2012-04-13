// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "start_mobile.hpp"
#include "wz_mobile/doc2.hpp"
#include "wz_mobile/gen_bitmap_class.hpp"
#include "wz_mobile/gen_mobmesh.hpp"
#include "main.hpp"
#include "engine_soft.hpp"
#include "pa.hpp"

extern sU32 ExportData[];

#define PROGRESS (93864*4)
//#define PROGRESS 0x1000000
#define BIGTIME 1

sU32 KKrunchyDepacker(sU8 *dst,const sU8 *src);

sInt AppMode;

/****************************************************************************/

sDirEntryCE DirEnt[10];
sInt DirMax;
sInt DirCursor;

/****************************************************************************/

SoftImage *Images[16];
SoftMesh *Meshes[16];
GenMobMesh *MobMeshes[16];
sInt ImageCount;
sInt MeshCount;

sInt CurrentImage;
sInt CurrentMesh;
sInt MouseX;
sInt MouseY;
sInt ImageMode;
sInt ShowMode;
sInt ScrollX;
sInt ScrollY;

sInt StartTime;

SoftEngine *Engine;

/****************************************************************************/

void AddImage(SoftImage *img)
{
  Images[ImageCount++] = img;
}

void AddMesh(GenMobMesh *mob)
{
  PA(L"MakeSoftMesh");
  MobMeshes[MeshCount] = mob;
  Meshes[MeshCount++] = MakeSoftMesh(mob);
};

/****************************************************************************/

static sU8 NodeMem[0x20000*BIGTIME];

GenHandler FindHandler(sInt id)
{
  sInt i;
  for(i=0;GenBitmapHandlers[i].Handler;i++)
    if(GenBitmapHandlers[i].Id==id)
      return GenBitmapHandlers[i].Handler;
  for(i=0;GenMobMeshHandlers[i].Handler;i++)
    if(GenMobMeshHandlers[i].Id==id)
      return GenMobMeshHandlers[i].Handler;
  return 0;
}

sU32 bmroots[64];
sU32 meshroots[64];
GenNode *nodes[10240];

sInt CalcProgress(GenNode *node)
{
  sInt i;
  sInt p=1;

  if(node->CountOnce==2) return p;
  if(node->CountOnce==1) node->CountOnce = 2;
  if(node->CountOnce==3)
  {
    if(node->Inputs[0])
      p += CalcProgress(node->Inputs[0]);
    return p;
  }

  for(i=0;i<node->InputCount;i++)
    if(node->Inputs[i])
      p += CalcProgress(node->Inputs[i]);

  for(i=0;i<node->LinkCount;i++)
    if(node->Links[i])
      p += CalcProgress(node->Links[i]);

  return p;
}

sBool Import(sU32 *data)
{
  sU8 *mem = NodeMem;
  GenNode *node;
  sInt i,j;

// import data

  sSetMem(NodeMem,0,sizeof(NodeMem));

  if(!sReadBegin(data,0)) return 0;
  sInt nodecount = *data++;  sVERIFY(nodecount<sCOUNTOF(nodes));
  sInt bmrootcount = *data++;  sVERIFY(bmrootcount<sCOUNTOF(bmroots));
  sInt meshrootcount = *data++;  sVERIFY(meshrootcount<sCOUNTOF(meshroots));
  data+=3;

  for(i=0;i<bmrootcount;i++)
    bmroots[i] = *data++;

  for(i=0;i<meshrootcount;i++)
    meshroots[i] = *data++;

  for(i=0;i<nodecount;i++)
  {
    nodes[i] = (GenNode *)mem;
    mem += sizeof(GenNode);
    sVERIFY(mem<NodeMem+(sizeof(NodeMem)));
  }

  for(i=0;i<nodecount;i++)
  {
    sInt hndid = *data++;
    GenHandler hnd = FindHandler(hndid);
    sInt ic = *data++;
    sInt lc = *data++;
    sInt pc = *data++;

    sVERIFY(hnd);

    nodes[i]->Handler = hnd;
    nodes[i]->InputCount = ic;
    nodes[i]->LinkCount = lc;
    nodes[i]->ParaCount = pc;
    nodes[i]->Inputs = (GenNode **)mem; mem+=ic*sizeof(GenNode *);
    nodes[i]->Links = (GenNode **)mem; mem+=lc*sizeof(GenNode *);
    nodes[i]->Para = (sInt *)mem; mem+=pc*sizeof(sU32);
    nodes[i]->CountOnce = 0;
    if((hndid&0xfff0)==0x0020) nodes[i]->CountOnce = 1; 
    if(hndid==0x2110) nodes[i]->CountOnce = 3; 

    sVERIFY(mem<NodeMem+(sizeof(NodeMem)));

    for(j=0;j<ic;j++)
    {
      sU32 index = *data++;
//      sVERIFY(index!=0);
      if(index<(sU32)nodecount)
        nodes[i]->Inputs[j] = nodes[index];
    }
    for(j=0;j<lc;j++)
    {
      sU32 index = *data++;
      if(index<(sU32)nodecount)
        nodes[i]->Links[j] = nodes[index];
    }
    for(j=0;j<pc;j++)
      nodes[i]->Para[j] = *data++;
  }
  if(!sReadEnd(data)) return 0;;

// prepare progress

  sInt prog = 0;
  for(i=0;i<bmrootcount;i++)
    prog += CalcProgress(nodes[bmroots[i]])*16;
  sDPrintF(L"mark1: %d\n",prog);
  for(i=0;i<meshrootcount;i++)
    prog += CalcProgress(nodes[meshroots[i]]);
  sDPrintF(L"mark2: %d\n",prog);

  sProgressStart(prog);

// create bitmaps

  InitPA();

  for(i=0;i<bmrootcount;i++)
  {
    node = nodes[bmroots[i]];
    SoftImage *img = new SoftImage(256,256);
    GenBitmapTile_Calc(node,img->SizeX,img->SizeY,img->Data);
    node->Image = img;
    AddImage(img);
  }

  sDPrintF(L"progress 1: %d\n",sProgressGet());

// create meshes

  MeshCount = 0;
  for(i=0;i<meshrootcount && MeshCount<16;i++)
  {
    node = nodes[meshroots[i]];
    GenMobMesh *mob = CalcMobMesh(node);
    AddMesh(mob);
  }

// done

  sDPrintF(L"progress 2: %d\n",sProgressGet());
  sProgressEnd();

  ExitPA();

  return 1;
}

void InitSystem(const sChar *filename)
{
  if(filename)
  {
    sU32 *mem = (sU32 *)sLoadFile(filename);
    if(!mem)
      sFatal(L"could not load file <%s>",filename);

    sU32 *depack = new sU32[1024*64*BIGTIME];

    sInt depacksize = KKrunchyDepacker((sU8 *)depack,(sU8 *)mem);
    if(depacksize==0 || depacksize>1024*64*BIGTIME)
      sFatal(L"depacking error");

   
    if(!Import(depack))
      sFatal(L"error in data-file");
    delete[] depack;
    delete[] mem;
  }
  else
  {
    sFatal(L"no input");
  }
  StartTime = sGetTime();
}

/****************************************************************************/

void sAppInit()
{
  /*
  sU32 *d;
  Image *img;
  sInt x,y;
*/

  // Start

  ImageCount = 0;
  CurrentImage = 0;
  MeshCount = 0;
  CurrentMesh = 0;
  MouseX = 0;
  MouseY = 0;
  ScrollX = 0;
  ScrollY = 0;
  ImageMode = 1;
  ShowMode = 0;

  Engine = new SoftEngine;

  InitBitmaps();

  // exported image

  const sChar *filename = sGetShellParameter(0);

  if(filename)
  {
    AppMode = 1;
    InitSystem(filename);
  }
  else
  {
    DirMax = sLoadDirCE(L"*.kkm",DirEnt,sCOUNTOF(DirEnt));
  }
  StartTime = sGetTime();
}

void sAppExit()
{
  sInt i;

  for(i=0;i<ImageCount;i++)
    delete Images[i];
  for(i=0;i<MeshCount;i++)
  {
    delete Meshes[i];
    delete MobMeshes[i];
  }

  delete Engine;
}

/****************************************************************************/
/***  3d                                                                  ***/
/****************************************************************************/

sInt Help;

void s3dOnMouse(sInt mode,sInt mx,sInt my)
{
  switch(mode)
  {
  case sMDD_START:
    MouseX = mx;
    MouseY = my;
    break;
  case sMDD_DRAG:
/*
    if(ZoomMode)
    {
      Zoom -= (mx-MouseX)*0x400;
      if(Zoom<0x1000) Zoom = 0x1000;
      if(Zoom>0x100000) Zoom = 0x10000;
    }
    else*/
    {
      ScrollX += (mx-MouseX)*0x10000;
      ScrollY += (my-MouseY)*0x10000;
    }
    MouseX = mx;
    MouseY = my;
    break;
  }
}

void s3dOnKey(sInt key)
{
  switch(key)
  {
  case sMKEY_UP:
    if(!ImageMode)
      ScrollY -= 16;
    break;
  case sMKEY_DOWN:
    if(!ImageMode)
      ScrollY += 16;
    break;
  case sMKEY_LEFT:
    if(ImageMode)
    {
      CurrentMesh--;
      if(CurrentMesh<0)
        CurrentMesh = MeshCount-1;
    }
    else
    {
      ScrollX -= 16;
    }
    break;
  case sMKEY_RIGHT:
    if(ImageMode)
    {
      CurrentMesh++;
      if(CurrentMesh>=MeshCount)
        CurrentMesh = 0;
    }
    else
    {
      ScrollX += 16;
    }
    break;

  case sMKEY_APP0:
  case '1':
    if(ImageMode)
    {
      Engine->Tesselate = !Engine->Tesselate;
    }
    else
    {
      CurrentImage++;
      if(CurrentImage>=ImageCount)
        CurrentImage = 0;
    }
    break;
  case sMKEY_APP1:
  case '2':
    if(ImageMode)
    {
      ShowMode = (ShowMode+1) % 3;
      Help = 0;
    }
    else
    {
      CurrentImage--;
      if(CurrentImage<0)
        CurrentImage = ImageCount-1;
    }
    break;
  case sMKEY_APP2:
  case '3':
    ImageMode = !ImageMode;
    break;

  case sMKEY_APP3:
  case sMKEY_ESCAPE:
    sExit();
    break;
  default:
    if(!(key&0x80000000))
      Help = !Help;
    break;
  }
}

void s3dOnPaint(sInt dx,sInt dy,sU16 *screen)
{
  sInt time = sGetTime()-StartTime;
  static sInt accTime=0;

  time = time/2;
//  time = (accTime+=4);

  //time = sGetTime()-StartTime;
//  time = StartTime; StartTime += 10;
/*
  sInt dist
  mat.InitEuler(0x400,time*4/4,0);
  dist = -(sISin(time*1)+0xa000)*256*6;
  mat.l.x = sIMul(mat.k.x,dist);
  mat.l.y = sIMul(mat.k.y,dist);
  mat.l.z = sIMul(mat.k.z,dist);
*/

  Engine->Begin(screen,dx,dy,0);

  if(MeshCount==0)
    ImageMode = 0;

  if(ImageMode)
  {
    sIMatrix34 mat;
    sIVector3 c,t;

    GenMobMesh *mob = MobMeshes[CurrentMesh];
    sInt maxtime = mob->CamSpline[mob->CamSpline.Count-1].Time;
    time = sMulDiv(time,sFIXONE,1000);
    if(time>=maxtime)
    {
      time = 0;
      StartTime = sGetTime();
      CurrentMesh++;
      if(CurrentMesh>=MeshCount)
        CurrentMesh = 0;
      GenMobMesh *mob = MobMeshes[CurrentMesh];
    }
//    time %= maxtime;
    mob->CalcSpline(time,c,t);
    mat.k.Sub(t,c);
    mat.k.Unit();
    mat.j.Init(0,sFIXONE,0);
    mat.i.Cross(mat.j,mat.k);
    mat.i.Unit();
    mat.j.Cross(mat.k,mat.i);
    mat.j.Unit();
    mat.l.Init(c.x*256,c.y*256,c.z*256);

    Engine->SetViewport(mat,sFIXONE);
    Engine->Paint(Meshes[CurrentMesh]);
  }
  else
  {
    SoftPixel *dest = Engine->GetBackBuffer();
    for(sInt y=0;y<dy;y++)
    {
      for(sInt x=0;x<dx;x++)
      {
        sU8 *tex = (sU8 *) (&Images[CurrentImage]->Data[((x+ScrollX)&255)+((y+ScrollY)&255)*256]);

        sInt b = tex[0];
        sInt g = tex[1];
        sInt r = tex[2];

        dest->Color = (b>>3)|((g&0xfc)<<3)|((r&0xf8)<<8);
        dest++;
      }
    }
  }

  if(Help || (!ImageMode))
  {
    sInt x=10;
    sInt y=10;
    if(ImageMode)
    {
      Engine->Print(x,y,L"2 -> debug info"); y+=10;
      Engine->Print(x,y,L"3 -> textures"); y+=10;
      Engine->Print(x,y,L"5 -> toggle help"); y+=10;
    }
    else
    {
      sChar buffer[64];
      sSPrintF(buffer,sCOUNTOF(buffer),L"texture %d / %d",CurrentImage+1,ImageCount);
      Engine->Print(x,y,buffer); y+=10;
      Engine->Print(x,y,L"1 -> previous texture"); y+=10;
      Engine->Print(x,y,L"2 -> next texture"); y+=10;
      Engine->Print(x,y,L"3 -> back to 3d"); y+=10;
    }
  }
  else if(ImageMode)
  {
    if(ShowMode == 1)
      Engine->DebugOut();
    if(ShowMode == 2)
      PrintPA(Engine);
  }

  Engine->End();

  sKeepRunning();

}

/****************************************************************************/
/***    file                                                              ***/
/****************************************************************************/

sInt lastkey;

void sFileOnPaint(sInt dx,sInt dy,sU16 *screen)
{
  sInt x,y;
  sChar buffer[64];

  Engine->Begin(screen,dx,dy,0);

  x=10;
  y=10;

  if(DirMax==0)
  {
    Engine->Print(x,y,L"no *.kkm files found");
  }
  else
  {
    for(sInt i=0;i<DirMax;i++)
    {
      sSPrintF(buffer,sCOUNTOF(buffer),L"%s %5d %s",i==DirCursor?L"->":L"  ",DirEnt[i].Size,DirEnt[i].Name);
      Engine->Print(x,y,buffer);
      y+=10;
    }
  }
//  y+=10;
//  sSPrintF(buffer,sCOUNTOF(buffer),L"lastkey %02x",lastkey);
//  Engine->Print(x,y,buffer);

  Engine->End();
}

void sFileOnMouse(sInt mode,sInt mx,sInt my)
{
}

void sFileOnKey(sInt key)
{
  lastkey = key;
  switch(key)
  {
  case sMKEY_UP:
    if(DirCursor>0)
      DirCursor--;
    break;
  case sMKEY_DOWN:
    if(DirCursor<DirMax-1)
      DirCursor++;
    break;
  case sMKEY_ENTER:
    AppMode++;
    InitSystem(DirEnt[DirCursor].Name);
    break;
  case sMKEY_APP3:
  case sMKEY_ESCAPE:
    sExit();
    break;
  }

}

/****************************************************************************/
/***    master                                                            ***/
/****************************************************************************/

void sAppOnPaint(sInt dx,sInt dy,sU16 *screen)
{
  switch(AppMode)
  {
  case 0:
    sFileOnPaint(dx,dy,screen);
    break;
  case 1:
    s3dOnPaint(dx,dy,screen);
    break;
  }
}

void sAppOnMouse(sInt mode,sInt mx,sInt my)
{
  switch(AppMode)
  {
  case 0:
    sFileOnMouse(mode,mx,my);
    break;
  case 1:
    s3dOnMouse(mode,mx,my);
    break;
  }
}

void sAppOnKey(sInt key)
{
  switch(AppMode)
  {
  case 0:
    sFileOnKey(key);
    break;
  case 1:
    s3dOnKey(key);
    break;
  }
}

void sAppOnPrint()
{
}

/****************************************************************************/
