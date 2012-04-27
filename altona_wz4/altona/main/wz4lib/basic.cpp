/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "basic.hpp"
#include "basic_ops.hpp"
#include "wz4lib/gui.hpp"

#include "serials.hpp"
#include "wz4lib/videoencoder.hpp"

#define CACHEIMAGEDATA ((sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX) && !Doc->IsPlayer)

/****************************************************************************/

AnyType::AnyType() 
{
  Type = AnyTypeType; 
}

/****************************************************************************/

GroupType::GroupType()
{
  Type = GroupTypeType;
}

GroupType::~GroupType()
{
  wObject *obj;
  sFORALL(Members,obj)
  {
    if(obj)
      obj->Release();
  }
}

void GroupType::Add(wObject *obj)
{
  if(obj)
  {
    obj->AddRef();
    Members.AddTail(obj);
  }
}

GroupType *GroupType::Make(wObject *e1,wObject *e2)
{
  GroupType *grp = new GroupType;
  grp->Members.AddTail(e1); if(e1) e1->AddRef();
  grp->Members.AddTail(e2); if(e2) e2->AddRef();
  return grp;
}

GroupType *GroupType::Make(wObject *e1,wObject *e2,wObject *e3)
{
  GroupType *grp = new GroupType;
  grp->Members.AddTail(e1); if(e1) e1->AddRef();
  grp->Members.AddTail(e2); if(e2) e2->AddRef();
  grp->Members.AddTail(e3); if(e3) e3->AddRef();
  return grp;
}

wObject *GroupType::GetRaw(wObject *obj,sInt ind)
{
  if(!obj) return 0;
  if(!obj->IsType(GroupTypeType)) return (ind == 0) ? obj : 0;
  
  GroupType *grp = (GroupType *)obj;
  return (ind < grp->Members.GetCount()) ? grp->Members[ind] : 0;
}

wObject *GroupType::Copy()
{
  GroupType *d = new GroupType;

  d->Members = Members;
  sAddRefAll(d->Members);

  return d;
}

/****************************************************************************/

ScreenshotProxy::ScreenshotProxy()
{
  Type = ScreenshotProxyType;
  Root = 0;    
  Zoom = 1.7f;
  SizeX = 320;
  SizeY = 480;
  Flags = 0;
  Strobe = 0;
//  OpLink = 0;
  StartTime = 0;
  EndTime = 60.0f;
  FPS = 30.0f;
}

ScreenshotProxy::~ScreenshotProxy()
{
  sRelease(Root);
//  SetOp(0);
}

wObject *ScreenshotProxy::Copy()
{
  ScreenshotProxy *d = new ScreenshotProxy;

  d->Root = Root;
  d->View = View;
  d->Zoom = Zoom;
  d->SizeX = SizeX;
  d->SizeY = SizeY;
  d->Multisample = Multisample;
  d->Flags = Flags;
  d->Strobe = Strobe;
  d->StartTime = StartTime;
  d->EndTime = EndTime;
  d->FPS = FPS;

  return d;
}

void ScreenshotProxyType_::Show(wObject *obj,wPaintInfo &pi)
{
  ScreenshotProxy *proxy = (ScreenshotProxy*) obj;
  if(proxy->Root)
  {
    sInt shot = (proxy->Strobe&1);
    proxy->Strobe = 0;
    sBool movie = (proxy->Flags & 0x40) && shot && proxy->EndTime>proxy->StartTime && proxy->FPS>0;
    
    sInt xs,ys;
    sRect r;
    switch(proxy->Flags & 0x30)
    {
    case 0x20:    // exact size
      r = pi.Client;
      xs = proxy->SizeX;
      ys = proxy->SizeY;
      if(proxy->SizeX<=r.SizeX() && proxy->SizeY<=r.SizeY())
      {
        r.x0 = pi.Client.x0 + (pi.Client.SizeX()-proxy->SizeX)/2;
        r.y0 = pi.Client.y0 + (pi.Client.SizeY()-proxy->SizeY)/2;
        r.x1 = r.x0 + proxy->SizeX;
        r.y1 = r.y0 + proxy->SizeY;
      }
      else
      {
        sInt xc = sInt(pi.Client.SizeY()*1.0f*proxy->SizeX/proxy->SizeY);
        sInt yc = sInt(pi.Client.SizeX()*1.0f*proxy->SizeY/proxy->SizeX);
        if(xc <= pi.Client.SizeX())
        {
          r.x0 = pi.Client.x0 + (pi.Client.SizeX()-xc)/2;
          r.x1 = r.x0 + xc;
          r.y0 = pi.Client.y0;
          r.y1 = pi.Client.y1;
        }
        else if(yc <= pi.Client.SizeY())
        {
          r.x0 = pi.Client.x0;
          r.x1 = pi.Client.x1;
          r.y0 = pi.Client.y0 + (pi.Client.SizeY()-yc)/2;
          r.y1 = r.y0 + yc;
        }
        else
        {
          sVERIFYFALSE;
        }
      }
      break;

    case 0x10:    // use only aspect ratio
      r = pi.Client;
      xs = sInt(pi.Client.SizeY()*1.0f*proxy->SizeX/proxy->SizeY);
      ys = sInt(pi.Client.SizeX()*1.0f*proxy->SizeY/proxy->SizeX);
      if(xs <= pi.Client.SizeX())
      {
        r.x0 = pi.Client.x0 + (pi.Client.SizeX()-xs)/2;
        r.x1 = r.x0 + xs;
        r.y0 = pi.Client.y0;
        r.y1 = pi.Client.y1;
      }
      else if(ys <= pi.Client.SizeY())
      {
        r.x0 = pi.Client.x0;
        r.x1 = pi.Client.x1;
        r.y0 = pi.Client.y0 + (pi.Client.SizeY()-ys)/2;
        r.y1 = r.y0 + ys;
      }
      else
      {
        sVERIFYFALSE;
      }
      xs = proxy->SizeX;
      ys = proxy->SizeY;
      break;

    default:      // use unmodified client rect
      r = pi.Client;
      xs = r.SizeX();
      ys = r.SizeY();
      break;
    }


    sInt tflags = sTEX_2D|sTEX_RENDERTARGET|sTEX_NOMIPMAPS;
    if(proxy->Multisample&1)
      tflags |= sTEX_MSAA;
    sTexture2D *rt = new sTexture2D(xs,ys,tflags|sTEX_ARGB8888);
    sTexture2D *db = new sTexture2D(xs,ys,tflags|sTEX_DEPTH24NOREAD);

    sSetTarget(sTargetPara(sST_CLEARCOLOR,0,pi.Spec));  // clear the client rect
    sSetTarget(sTargetPara(sST_CLEARALL,0,0,rt,db)); // clear the actual rendertarget
    sTargetSpec oldspec = pi.Spec;
    pi.Spec.Init(rt,db);
    pi.Spec.Aspect = sF32(pi.Spec.Window.SizeX())/pi.Spec.Window.SizeY();

    sInt framecount=1;
    sInt digits=0;

    sVideoEncoder *venc=0;

    sChar *fsrc=sFindFileExtension(proxy->SaveName);

    if (movie)
    {
      framecount=sInt((proxy->EndTime-proxy->StartTime)*proxy->FPS+0.5f);

      sInt d=10;
      for (digits=1; d<framecount; digits++) d*=10;
      if (!fsrc) fsrc=L"avi";
      if (!sCmpStringI(fsrc,L"avi"))
      {
        sPreventPaint();
        venc=sCreateVideoEncoder(proxy->SaveName, proxy->FPS);
        if (!venc)
          movie=shot=0;
        else if (App->MusicData && App->MusicSize)
          venc->SetAudioFormat(16,2,44100);
      }
    }
    else
    {
      if (!fsrc) fsrc=L"bmp";
    }

    sCheckBreakKey();
    for (sInt frame=0; frame<framecount; frame++)
    {
      if (movie)
      {
        sInt ms=sInt(1000.0f*(proxy->StartTime+sF32(frame)/proxy->FPS)+0.5f);
        sInt beat=Doc->MilliSecondsToBeats(ms);
        pi.TimeMS=ms;
        pi.TimeBeat=beat;
        if (shot) sDPrintF(L"rendering frame %d at %dms(beat %08x\n",frame,ms,beat);

        if (ProgressPaintFunc) ProgressPaintFunc(frame,framecount);
      }

      if(!pi.CamOverride)
      {
        pi.Zoom3D = proxy->Zoom;
        pi.View = &proxy->View;
        pi.Grid = 0;
      }
      pi.View->SetTargetCurrent();
      pi.View->SetZoom(pi.Zoom3D);
      pi.View->Prepare();

      Doc->Show(proxy->Root,pi);
      sResolveTarget();

      if(shot)
      {
        if (venc)
        {
          venc->WriteFrame(rt);
          if (App->MusicData)
          {
            sInt nextms=sInt(1000.0f*(proxy->StartTime+sF32(frame+1)/proxy->FPS));
            sInt smpl=sMin(App->MusicSize,sMulDiv(pi.TimeMS,44100,1000));
            sInt nextsmpl=sMin(App->MusicSize,sMulDiv(nextms,44100,1000));
            if (nextsmpl>smpl)
              venc->WriteAudioFrame(App->MusicData+2*smpl,nextsmpl-smpl);
          }
        }
        else if (!sCmpStringI(fsrc,L"bmp"))
        {
          sString<256> fn=proxy->SaveName;

          if (digits>0)
          {
            sChar *fdst=sFindFileExtension(fn);
            if (*fdst) *fdst=0;

            if (fsrc && fdst)
            {
              sString<32> fmt;
              fmt.PrintF(L"%%0%dd.%%s",digits);
              *fdst=0;
              fn.PrintAddF(fmt,frame,fsrc);
            }
          }

          fn.PrintF(fn,frame);
          sSaveRT(fn,rt);
        }
      }

      if(sCheckBreakKey())
        break;
    }

    delete venc;
    if (movie)
      sUpdateWindow();    

    pi.Spec = oldspec;

    sCopyTexturePara cp(sCT_FILTER,pi.Spec.Color2D,rt);
    cp.DestRect = pi.Spec.Window;
#if sRENDERER==sRENDER_DX11
    cp.SourceRect.Init(0,0,sMin(rt->SizeX,pi.Spec.Window.SizeX()),sMin(rt->SizeY,pi.Spec.Window.SizeY()));
#endif
    sCopyTexture(cp);

    delete rt;
    delete db;
  }
}

/****************************************************************************/

TextObject::TextObject()
{
  Type = TextObjectType;
}

wObject *TextObject::Copy()
{
  TextObject *d = new TextObject;

  d->Text = Text.Get();

  return d;
}

/****************************************************************************/

void BitmapAtlas::InitAtlas(sInt power,sInt sx,sInt sy)
{
  sInt tx=1,ty=1;
  switch(power)
  {
  default:
  case 0: tx=1; ty=1; break;
  case 1: tx=2; ty=1; break;
  case 2: tx=2; ty=2; break;
  case 3: tx=4; ty=2; break;
  case 4: tx=4; ty=4; break;
  case 5: tx=8; ty=4; break;
  }
  Entries.Resize(1<<power);
  for(sInt i=0;i<(1<<power);i++)
  {
    sInt n = i;
    sInt px = n & (tx-1);
    n = n/tx;
    sInt py = n & (ty-1);
    Entries[i].Pixels.x0 = (px+0) * (sx/tx);
    Entries[i].Pixels.y0 = (py+0) * (sy/ty);
    Entries[i].Pixels.x1 = (px+1) * (sx/tx);
    Entries[i].Pixels.y1 = (py+1) * (sy/ty);

    Entries[i].UVs.x0 = sF32(px+0)/tx;
    Entries[i].UVs.y0 = sF32(py+0)/ty;
    Entries[i].UVs.x1 = sF32(px+1)/tx;
    Entries[i].UVs.y1 = sF32(py+1)/ty;
  }
}


template <class streamer> void BitmapAtlas::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::Wz4BitmapAtlas,2); version;

  s.Array(Entries);
  BitmapAtlasEntry *e;
  sFORALL(Entries,e)
  {
    s | e->Pixels;
    if(version>=2)
      s | e->UVs;
  }

  s.Footer();
}

void BitmapAtlas::Serialize(sWriter &stream) { Serialize_(stream); }
void BitmapAtlas::Serialize(sReader &stream) { Serialize_(stream); }

/****************************************************************************/

BitmapBase::BitmapBase()
{
  Type = BitmapBaseType;
}

void BitmapBase::CopyTo(sImageData *dest,sInt format)
{
  sImage img;
  CopyTo(&img);
  dest->Init2(format|sTEX_2D,0,img.SizeX,img.SizeY,1);
  dest->ConvertFrom(&img);
}

/****************************************************************************/

Texture2D::Texture2D()
{
  Type = Texture2DType;
  Texture = 0;
  Cache = 0;
}

Texture2D::~Texture2D()
{
  delete Texture;
  delete Cache;
}


void Texture2D::ConvertFrom(BitmapBase *bm,sInt format)
{
  sDelete(Cache);
  sImageData *id = new sImageData;
  bm->CopyTo(id,format);
  Texture = (sTexture2D *)id->CreateTexture();
  if(CACHEIMAGEDATA)
    Cache = id;
  else
    delete id;

  Atlas=bm->Atlas;
}

void Texture2D::ConvertFrom(sImage *img,sInt format)
{
  sDelete(Cache);
  sImageData *id = new sImageData;
  id->Init2(format,0,img->SizeX,img->SizeY,1);
  id->ConvertFrom(img);
  Texture = (sTexture2D *)id->CreateTexture();
  if(CACHEIMAGEDATA)
    Cache = id;
  else
    delete id;
}

void Texture2D::ConvertFrom(sImageData *id)
{
  sDelete(Cache);
  Texture = (sTexture2D *)id->CreateTexture();
  if(CACHEIMAGEDATA)
    Cache = id;
  else
    delete id;
}

wObject *Texture2D::Copy()
{
  if(!CACHEIMAGEDATA)
    return 0;

  Texture2D *d = new Texture2D;
  d->CopyFrom(this);
  return d;
}

void Texture2D::CopyFrom(Texture2D *tex)
{
  sDelete(Cache);
  Atlas = tex->Atlas;
  if (CACHEIMAGEDATA)
  {
    Cache = new sImageData;
    Cache->Copy(tex->Cache);
    Texture = (sTexture2D *)Cache->CreateTexture();
  }
  else
    sDPrintF(L"Texture2D: can't CopyFrom without caching!\n");
}

template <class streamer> void Texture2D::Serialize_(streamer &stream)
{
  sBool oldformat=sFALSE;

  if(stream.IsReading())
  {
    Cache = new sImageData;
    if (stream.PeekHeader()==sSerId::sImageData) 
      oldformat=sTRUE;
  }
  else
    sVERIFY(Cache);

  if (!oldformat)
  {
    sInt version=stream.Header(sSerId::Wz4Texture2D,1); version;
  }

  Cache->Serialize(stream);
  sVERIFY((Cache->Format & sTEX_TYPE_MASK)==sTEX_2D);

  if (!oldformat)
  {
    Atlas.Serialize(stream);
    stream.Footer();
  }

  if(stream.IsReading())
    Texture =  (sTexture2D *)Cache->CreateTexture();
  if(!CACHEIMAGEDATA)
    sDelete(Cache);
}

void Texture2D::Serialize(sWriter &stream) { Serialize_(stream); }
void Texture2D::Serialize(sReader &stream) { Serialize_(stream); }


/****************************************************************************/

CubemapBase::CubemapBase()
{
  Type = CubemapBaseType;
}

void CubemapBase::CopyTo(sImageData *dest,sInt format)
{
  sImage *img[6];
  for(sInt i=0;i<6;i++)
    img[i] = new sImage;

  CopyTo(img);
  sInt size = img[0]->SizeX;
  dest->Init2(format|sTEX_CUBE,0,size,size,6);
  dest->ConvertFromCube(img);

  for(sInt i=0;i<6;i++)
    delete img[i];
}

/****************************************************************************/

TextureCube::TextureCube()
{
  Type = TextureCubeType;
  Texture = 0;
  Cache = 0;
}

TextureCube::~TextureCube()
{
  delete Texture;
  delete Cache;
}

wObject *TextureCube::Copy()
{
  if(!CACHEIMAGEDATA)
    return 0;

  TextureCube *d = new TextureCube;
  d->Cache = new sImageData;
  d->Cache->Copy(Cache);
  d->Texture = (sTextureCube *) d->Cache->CreateTexture();

  return d;
}


void TextureCube::ConvertFrom(CubemapBase *bm,sInt format)
{
  sImageData *id = new sImageData;
  bm->CopyTo(id,format);
  Texture = (sTextureCube *)id->CreateTexture();

  if(CACHEIMAGEDATA)
    Cache = id;
  else
    delete id;
}

void TextureCube::InitDummy()
{
  sImageData *id= new sImageData;
  id->Init2(sTEX_CUBE|sTEX_ARGB8888|sTEX_NOMIPMAPS,1,8,8,6);
  for(sInt i=0;i<8*8*6;i++)
    ((sU32 *)id->Data)[i] = 0xff705030;
  Texture = (sTextureCube *)id->CreateTexture();
  delete id;
}

template <class streamer> void TextureCube::Serialize_(streamer &stream)
{
  if(stream.IsReading())
    Cache = new sImageData;
  else
    sVERIFY(Cache);
  Cache->Serialize(stream);
  sVERIFY((Cache->Format & sTEX_TYPE_MASK)==sTEX_CUBE);

  if(stream.IsReading())
    Texture = (sTextureCube *)Cache->CreateTexture();
  if(!CACHEIMAGEDATA)
    sDelete(Cache);
}
void TextureCube::Serialize(sWriter &stream) { Serialize_(stream); }
void TextureCube::Serialize(sReader &stream) { Serialize_(stream); }

/****************************************************************************/

void SceneInstance::Init()
{
  Matrix.Init();
  LightFlags = -1;
  Time = 0;//(sGetTime()%10000)/10000.0f;
  Temp = 0;
};

void SceneInstances::Init()
{
  Object=0;
  LodMapping = 0xffffffff;
}

void SceneMatrices::Seed()
{
  Matrices.AddMany(1)->Init();
}

/****************************************************************************/

Scene::Scene()
{
  Type = SceneType;
  Node=0; 
  DoTransform=0; 
  DoFilter = 0;
  LightFlags = -1;
  NameId = 0;
  LodMapping = 0xffffffff;
}

Scene::~Scene() 
{ 
  sRelease(Node);
  sReleaseAll(Childs); 
}

wObject *Scene::Copy()
{
  Scene *d = new Scene;

  d->Node = Node; Node->AddRef();
  d->Childs = Childs; sAddRefAll(Childs);
  d->Transform = Transform;
  d->DoTransform = DoTransform;
  d->DoFilter = DoFilter;
  d->LightFlags = LightFlags;
  d->NameId = NameId;
  d->LodMapping = LodMapping;

  return d;
}

void Scene::AddChild(MeshBase *mesh, const sMatrix34 &mat, sInt nameid/*=0*/)
{
  Scene *scene = new Scene;
  scene->DoTransform = 1;
  scene->NameId = nameid;
  scene->Transform = mat;
  scene->Node = mesh;
  mesh->AddRef();
  Childs.AddTail(scene);
}

void Scene::TransformAdd(SceneMatrices *sm,const sMatrix34 &mat)
{
  sMatrix34 *mp = sm->Matrices.AddMany(SaveMatrix.GetCount());
  for(sInt i=0;i<SaveMatrix.GetCount();i++)
    mp[i] = mat * SaveMatrix[i];
}

void Scene::OpTransform(SceneMatrices *sm)
{
  TransformAdd(sm,Transform);
}

void Scene::OpFilter(SceneMatrices *sm,sInt start,sInt end)
{
}

void Scene::Recurse(SceneMatrices *sm,sInt nameid)
{
  Scene *sc;
  if(DoTransform)
  {
    SaveMatrix = sm->Matrices;
    sm->Matrices.Clear();
    OpTransform(sm);
  }

  sInt filterstart = sm->Instances.GetCount();
  if(NameId)
    nameid = NameId;
  sFORALL(Childs,sc)
    sc->Recurse(sm,nameid);

  if(Node && Node->IsType(MeshBaseType))
  {
    SceneInstances *inst = new SceneInstances;
    inst->Init();
    inst->Object = Node;
    inst->LodMapping = LodMapping;

    SceneInstance *sinst = inst->Matrices.AddMany(sm->Matrices.GetCount());
    sMatrix34 *mp;
    sFORALL(sm->Matrices,mp)
    {
      sinst->Init();
      sinst->Matrix = *mp;
      if(LightFlags!=-1)
        sinst->LightFlags = LightFlags;
      sinst->NameId = nameid;
      sinst++;
    }
    sm->Instances.AddTail(inst);
  }

  if(DoFilter)
  {
    OpFilter(sm,filterstart,sm->Instances.GetCount());
  }

  // set lods
  if(LodMapping!=0xffffffff)
  {
    for(sInt i=filterstart;i<sm->Instances.GetCount();i++)
    {
      SceneInstances *inst = sm->Instances[i];
      inst->LodMapping = LodMapping;
    }
  }

  /*
  if(NameId)
  {
    SceneInstances *ss;
    SceneInstance *si;
    sFORALL(sm->Instances,ss)
      sFORALL(ss->Matrices,si)
        si->NameId = NameId;
  }
  */
  if(DoTransform)
  {
    sm->Matrices = SaveMatrix;
    SaveMatrix.Clear();
  }
}

void Scene::GatherStats(MeshStats &stat)
{
  SceneMatrices sm;
  SceneInstances *si;
  SceneInstance *sii;

  sm.Seed();
  Recurse(&sm);

  sFORALL(sm.Instances,si)
    sFORALL(si->Matrices,sii)
      si->Object->GatherStats(stat);  
}

/****************************************************************************/
/***                                                                      ***/
/***   Unit tests by comparing bitmaps                                    ***/
/***                                                                      ***/
/****************************************************************************/


UnitTest::UnitTest()
{
  Type = UnitTestType;
  Errors = 0;
  Total = 0;
  Skipped = 0;
}

UnitTest::~UnitTest()
{
}

wObject *UnitTest::Copy()
{
  UnitTest *d = new UnitTest;

  d->Errors = Errors;
  d->Total = Total;
  d->Skipped = Skipped;
  d->Render.Copy(&Render);
  d->Compare.Copy(&Compare);

  return d;
}


void UnitTestType_::Show(wObject *obj,wPaintInfo &pi)
{
  UnitTest *ut = (UnitTest *) obj;
  sInt x0 = pi.Client.x0+10;
  sInt y0 = pi.Client.y0+10;

  y0+= 20;

  sClipPush();

  sInt xs = ut->Render.SizeX;
  sInt ys = ut->Render.SizeY;
  if(xs*ys>0)
  {
    // original image

    sRect sr(0,0,xs,ys);
    sRect dr(x0,y0,x0+xs,y0+ys);
    sStretch2D(ut->Render.Data,ut->Render.SizeX,sr,dr);
    sClipExclude(dr);
    x0 += 10+xs;

    // compare image

    sr.Init(0,0,xs,ys);
    dr.Init(x0,y0,x0+xs,y0+ys);
    sStretch2D(ut->Compare.Data,ut->Compare.SizeX,sr,dr);
    sClipExclude(dr);
    x0 += 10+xs;

    // difference image

    sImage diff(xs,ys);

    const sU8 *a = (const sU8 *) ut->Render.Data;
    const sU8 *b = (const sU8 *) ut->Compare.Data;
    sU8 *d = (sU8 *) diff.Data;
    for(sInt i=0;i<xs*ys*4;i++)
      d[i] = sClamp((a[i]-b[i])*16+128,0,255);

    dr.Init(x0,y0,x0+xs,y0+ys);
    sStretch2D(diff.Data,diff.SizeX,sr,dr);
    sClipExclude(dr);
    x0 += 10+xs;

    // alpha difference

    for(sInt i=0;i<xs*ys*4;i+=4)
    {
      d[i+0] = d[i+3];
      d[i+1] = d[i+3];
      d[i+2] = d[i+3];
    }

    dr.Init(x0,y0,x0+xs,y0+ys);
    sStretch2D(diff.Data,diff.SizeX,sr,dr);
    sClipExclude(dr);
    x0 += 10+xs;
  }

  sRect2D(pi.Client,sGC_BACK);
  x0 = pi.Client.x0+10;
  y0 = pi.Client.y0+10;
  sString<256> b;
  b.PrintF(L"%d Total, %d Errors, %d Skipped",ut->Total,ut->Errors,ut->Skipped);
  sGui->FixedFont->Print(0,x0,y0,b);

  sClipPop();
}

/****************************************************************************/

sInt UnitTest::Test(sImage &img,const sChar *filename,sInt flags)
{
  if(sGetStringLen(filename)==0)
    return 0;
  sBool mayfail = 0;
  if(Doc->TestOptions.Mode==wTOM_Compare && (flags & wTOF_NoCompare))
    mayfail = 1;
  if(Doc->TestOptions.Mode==wTOM_CrossCompare && (flags & wTOF_NOCrossCompare))
    mayfail = 1;

  sInt platform = wTOP_Unknown;
  if(sRENDERER==sRENDER_DX9)
    platform = wTOP_DirectX9;
  if(sRENDERER==sRENDER_DX11)
    platform = wTOP_DirectX11;
  if(Doc->TestOptions.Mode == wTOM_CrossCompare)
    platform = Doc->TestOptions.Compare;


  sString<sMAXPATH> path;
  path = App->UnitTestPath;
  switch(platform)
  {
    case wTOP_DirectX9:  path.AddPath(L"directx9"); break;
    case wTOP_DirectX11: path.AddPath(L"directx11"); break;
    default:             path.AddPath(L"unknwon"); break;
  }
  path.AddPath(filename);
  path.Add(L".pic");

  sInt result = 1;
  Total = 1;
  Errors = 0;
  Skipped = 0;

  if(Doc->TestOptions.Mode == wTOM_Write)
  {
    result &= img.SavePIC(path);
  }
  else    // compare or crosscompare
  {
    sImage img2;
    result &= img2.Load(path);

    if(img.SizeX!=img2.SizeX || img.SizeY!=img2.SizeY)
    {
      result = 0;
    }
    else
    {
      const sU8 *src = (const sU8 *)img.Data;
      const sU8 *dst = (const sU8 *)img2.Data;
      for(sInt i=0;i<img.SizeX*img.SizeY;i++)
      {
        sInt diff = src[i]-dst[i];
        if(diff>1 || diff<-1)
        {
          if(Doc->TestOptions.FailRed)
          if(mayfail)
          {
            Skipped = 1;
          }
          else
          {
            Errors = 1;
            result = 0;
          }
          break;
        }
      }
    }

    Render.Copy(&img);
    Compare.Copy(&img2);
  }

  return result;
}

/****************************************************************************/

