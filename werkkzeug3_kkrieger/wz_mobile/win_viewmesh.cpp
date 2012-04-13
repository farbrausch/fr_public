// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "win_viewmesh.hpp"
#include "main_mobile.hpp"
#include "gen_bitmap_class.hpp"
#include "gen_mobmesh.hpp"
#include "player_mobile/engine_soft.hpp"

const sInt OrigSizeX = 176;
const sInt OrigSizeY = 220;

/****************************************************************************/
/***                                                                      ***/
/***   View Mesh                                                          ***/
/***                                                                      ***/
/****************************************************************************/

ViewMeshWin_::ViewMeshWin_()
{
  sISRT srt;
  Flags |= sGWF_PAINT3D;

  srt.Init();
  srt.s.Init(0x10000,0x10000,0x10000);

  ShowOp = 0;
  FullSize = 0;
  ThisTime = LastTime = sSystem->GetTime();
  OnCommand(CMD_VIEWMESH_RESET);
  ClearColor = 0xff000000;
  MeshWire = 1;

  Mtrl = new sMaterial11;
  Mtrl->ShaderLevel = sPS_00;
  Mtrl->Combiner[sMCS_VERTEX] = sMCOA_SET;
  Mtrl->BaseFlags = sMBF_NONORMAL|sMBF_NOTEXTURE;
  Mtrl->Compile();

  {
    static sGuiMenuList ml[] = 
    {
      { sGML_COMMAND  ,'r'      ,CMD_VIEWMESH_RESET       ,0,"Reset" },
      { sGML_COMMAND  ,'c'      ,CMD_VIEWMESH_COLOR       ,0,"Color" },
      { sGML_CHECKMARK,sKEY_TAB ,CMD_VIEWMESH_FULLSIZE    ,sOFFSET(ViewMeshWin_,FullSize)       ,"Full size" },
      { sGML_SPACER },
      { sGML_CHECKMARK,'^'      ,CMD_VIEWMESH_WIRE        ,sOFFSET(ViewMeshWin_,MeshWire)       ,"wireframe" },
      { sGML_CHECKMARK,'o'      ,CMD_VIEWMESH_ORTHOGONAL  ,sOFFSET(ViewMeshWin_,Cam.Ortho)      ,"orthogonal" },
      { sGML_CHECKMARK,'p'      ,CMD_VIEWMESH_GRID        ,sOFFSET(ViewMeshWin_,ShowXZPlane)    ,"grid" },
      { sGML_CHECKMARK,'t'      ,CMD_VIEWMESH_90DEGREE    ,sOFFSET(ViewMeshWin_,Quant90Degree)  ,"fix to 90°" },
      { sGML_SPACER },  
      { sGML_CHECKMARK,'h'      ,CMD_VIEWMESH_ORIGSIZE    ,sOFFSET(ViewMeshWin_,ShowOrigSize)   ,"original size" },
      { sGML_CHECKMARK,'H'|sKEYQ_SHIFT,CMD_VIEWMESH_DOUBLESIZE  ,sOFFSET(ViewMeshWin_,ShowDoubleSize) ,"double size" },
      { sGML_CHECKMARK,0        ,CMD_VIEWMESH_ENGINEDEBUG ,sOFFSET(ViewMeshWin_,ShowEngineDebug),"engine debugs" },
      { sGML_CHECKMARK,'k'      ,CMD_VIEWMESH_SPLINE      ,sOFFSET(ViewMeshWin_,PlaySpline)     ,"play spline" },

      { 0 },
    };
    SetMenuList(ml);
  }

  Tools = new sToolBorder;
  Tools->AddLabel(".preview mesh");
  Tools->AddContextMenu(CMD_VIEWMESH_MENU);
  AddBorder(Tools);

  Soft = new SoftEngine;
}

ViewMeshWin_::~ViewMeshWin_()
{
  sRelease(Mtrl);

  delete Soft;
}

/****************************************************************************/

void ViewMeshWin_::OnPaint()
{
  if(Flags & sGWF_CHILDFOCUS)
  {
    sU32 qual = sSystem->GetKeyboardShiftState();
    if(qual&sKEYQ_CTRL)
      App->StatusMouse="l:zoom   m:scroll [menu]   r:dolly [wire]";
    else if(qual&sKEYQ_ALT)
      App->StatusMouse="l:pivot   m:scroll [menu]   r:orbit [wire]";
    else if(qual&sKEYQ_SHIFT)
      App->StatusMouse="l:edit x   m:edit y [menu]   r:edit z";
    else
      App->StatusMouse="l:rotate   m:scroll [menu]   r:orbit [wire]";
/*
    if(Object)
      sSPrintF(App->StatusWindow,"%d Faces %d Vertices",Object->Faces.Count,Object->Vertices.Count);
    else
      App->StatusWindow = "no mesh";
*/
  }

  if(!ShowOp)
  {
    ClearBack(1);
  }
  else if(ShowOrigSize || ShowDoubleSize)
  {
    sInt xs,ys;
    sRect r,r1;

    r = Client;
    xs = OrigSizeX;
    ys = OrigSizeY;
    if(ShowDoubleSize)
    {
      xs *= 2;
      ys *= 2;
    }
    if(xs>r.XSize())
    {
      xs = r.XSize();
      ys = r.XSize()*OrigSizeY/OrigSizeX;
    }
    if(ys>r.YSize())
    {
      ys = r.YSize();
      xs = sMin(xs,r.YSize()*OrigSizeX/OrigSizeY);
    }
    if(ShowDoubleSize)
    {
      xs &= ~1;
      ys &= ~1;
    }

    r.x0 = r.x0+(r.x1-r.x0-xs)/2;
    r.y0 = r.y0+(r.y1-r.y0-ys)/2;
    r.x1 = r.x0+xs;
    r.y1 = r.y0+ys;

    r1 = r;
    r.Extend(1);
    sPainter->PaintHole(sGui->FlatMat,Client,r,0xff000000);
    sPainter->PaintHole(sGui->FlatMat,r,r1,0xff808080);
  }


}

void ViewMeshWin_::OnPaint3d(sViewport &rview)
{
  sVertexCompact *vp;
  sU16 *ip;
  sInt handle;
  sMaterialEnv env;
  sInt speedy;
  sViewport view = rview;

  GenMobMesh *mesh=0;
  GenNode *node=0;


  if(ShowOrigSize || ShowDoubleSize)
  {
    sInt xs,ys;
    sRect r;

    xs = OrigSizeX;
    ys = OrigSizeY;
    if(ShowDoubleSize)
    {
      xs *= 2;
      ys *= 2;
    }
    if(xs>view.Window.XSize())
    {
      xs = view.Window.XSize();
      ys = view.Window.XSize()*OrigSizeY/OrigSizeX;
    }
    if(ys>view.Window.YSize())
    {
      ys = view.Window.YSize();
      xs = sMin(xs,view.Window.YSize()*OrigSizeX/OrigSizeY);
    }
    if(ShowDoubleSize)
    {
      xs &= ~1;
      ys &= ~1;
    }

    r = view.Window;
    view.Window.x0 = r.x0+(r.x1-r.x0-xs)/2;
    view.Window.y0 = r.y0+(r.y1-r.y0-ys)/2;
    view.Window.x1 = view.Window.x0+xs;
    view.Window.y1 = view.Window.y0+ys;
  }

  if(ShowOp)
  {
    Doc->Connect();
    if(!ShowOp->Error)
    {
      mesh = (GenMobMesh *)FollowOp(ShowOp)->FindVariant(GVI_MESH_MOBMESH);
      if(mesh)
      {
        mesh->AddRef();
      }
      else
      {
        Doc->MakeNodePrepare();
        node = Doc->MakeNodeTree(ShowOp,GVI_MESH_MOBMESH,0,0);
        if(node)
        {
          mesh = (GenMobMesh *) CalcMobMesh(node);
        }
      }
    }

    // quake cam

    if(sGui->GetFocus()!=this)
    {
      QuakeSpeedForw=0;
      QuakeSpeedSide=0;
      QuakeSpeedUp=0;
      QuakeMask=0;
    }
    ThisTime = sSystem->GetTime();

    speedy = 1;
    if(sSystem->GetKeyboardShiftState()&sKEYQ_SHIFT)
      speedy = 4;
    sInt count = sMin((ThisTime/10)-(LastTime/10),100);
    for(sInt i=0;i<count;i++)
    {
      if(QuakeMask &  1) QuakeSpeedForw += speedy;
      if(QuakeMask &  2) QuakeSpeedForw -= speedy;
      if(QuakeMask &  4) QuakeSpeedSide += speedy;
      if(QuakeMask &  8) QuakeSpeedSide -= speedy;
      if(QuakeMask & 16) QuakeSpeedUp += speedy;
      if(QuakeMask & 32) QuakeSpeedUp -= speedy;
      QuakeSpeedForw *= 0.70f;
      QuakeSpeedSide *= 0.70f;
      QuakeSpeedUp   *= 0.70f;
	    Cam.State[6] += 0.03f*(QuakeSpeedSide*CamMatrix.i.x+QuakeSpeedUp*CamMatrix.j.x+QuakeSpeedForw*CamMatrix.k.x);
	    Cam.State[7] += 0.03f*(QuakeSpeedSide*CamMatrix.i.y+QuakeSpeedUp*CamMatrix.j.y+QuakeSpeedForw*CamMatrix.k.y);
	    Cam.State[8] += 0.03f*(QuakeSpeedSide*CamMatrix.i.z+QuakeSpeedUp*CamMatrix.j.z+QuakeSpeedForw*CamMatrix.k.z);
    }

    if(Quant90Degree)
    {
      Cam.State[3] = (sFRound(Cam.State[3]*4+1024.5)-1024)/4;
      Cam.State[4] = (sFRound(Cam.State[4]*4+1024.5)-1024)/4;
      Cam.State[5] = (sFRound(Cam.State[5]*4+1024.5)-1024)/4;
      Quant90Degree = 0;
    }
    if(DragMode==DM_ORBIT)
    {
		  Cam.State[6] = PivotVec.x - CamMatrix.k.x * Cam.Pivot;
		  Cam.State[7] = PivotVec.y - CamMatrix.k.y * Cam.Pivot;
		  Cam.State[8] = PivotVec.z - CamMatrix.k.z * Cam.Pivot;
    }
    CamMatrix.InitSRT(Cam.State);

    ShowPivotVecTimer -= ThisTime-LastTime;
    if(ShowPivotVecTimer<0)
      ShowPivotVecTimer = 0;
    if(sSystem->GetKeyboardShiftState()&sKEYQ_SHIFT)
      ShowPivotVecTimer++;


    // override cam with spline

    if(mesh && PlaySpline && mesh->CamSpline.Count>=2)
    {
      SplineTime += (ThisTime-LastTime)*sFIXONE/1000;
      sInt maxtime = mesh->CamSpline[mesh->CamSpline.Count-1].Time;
      if(SplineTime>=maxtime)
      {
        SplineTime -= maxtime;
        if(SplineTime>maxtime)
          SplineTime = 0;
      }

      sIVector3 c,t;
      sVector v;
      mesh->CalcSpline(SplineTime,c,t);

      v.Init(t.x-c.x,t.y-c.y,t.z-c.z);
      CamMatrix.InitDir(v);
      CamMatrix.l.Init(c.x*1.0f/sFIXONE,c.y*1.0f/sFIXONE,c.z*1.0f/sFIXONE);
    }

    // set viewport 

    LastTime = ThisTime;

    env.Init();
    env.CameraSpace = CamMatrix;
    env.Orthogonal = Cam.Ortho;
    env.ZoomX = 128.0f/Cam.Zoom[Cam.Ortho];
    env.ZoomY = env.ZoomX *  view.Window.XSize() / view.Window.YSize();

    // begin drawing

    sSystem->SetViewport(view);
    sSystem->Clear(sVCF_ALL,0xff000000);
    sSystem->SetViewProject(&env);
    Mtrl->Set(env);
    handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE|sGEO_DYNAMIC);

    if(mesh)
    {
      if(App->CurrentViewWin == this)
        sSPrintF(App->StatusObject,"Mesh: %d vert %d face %d clust",mesh->Vertices.Count,mesh->Faces.Count,mesh->Clusters.Count);

      if(MeshWire)
        Wireframe(mesh,view);
      else
        Solid(mesh,view,env);
      mesh->Release();
    }

    // draw grid

    if(ShowXZPlane)
    {
      sF32 x = sF32(sInt(CamMatrix.l.x));
      sF32 z = sF32(sInt(CamMatrix.l.z));
      sU32 col = 0xff808080;
      sSystem->GeoBegin(handle,404,404,(sF32 **)&vp,(void **)&ip);

      for(sInt i=0;i<101;i++)
      {
        vp[0].Init(x+i-50,0,z  -50,col);
        vp[1].Init(x+i-50,0,z  +50,col);
        vp[2].Init(x  -50,0,z+i-50,col);
        vp[3].Init(x  +50,0,z+i-50,col);
        vp+=4;
        ip[0] = i*4+0;
        ip[1] = i*4+1;
        ip[2] = i*4+2;
        ip[3] = i*4+3;
        ip+=4;
      }

      sSystem->GeoEnd(handle);
      sSystem->GeoDraw(handle);
    }

    // draw pivot

    if(ShowPivotVecTimer)
    {
      sF32 s=0.5f;
      sU32 col=0xffffff00;

      sSystem->GeoBegin(handle,6,6,(sF32 **)&vp,(void **)&ip);

      vp[0].Init(PivotVec.x-s,PivotVec.y  ,PivotVec.z  ,col);
      vp[1].Init(PivotVec.x+s,PivotVec.y  ,PivotVec.z  ,col);
      vp[2].Init(PivotVec.x  ,PivotVec.y-s,PivotVec.z  ,col);
      vp[3].Init(PivotVec.x  ,PivotVec.y+s,PivotVec.z  ,col);
      vp[4].Init(PivotVec.x  ,PivotVec.y  ,PivotVec.z-s,col);
      vp[5].Init(PivotVec.x  ,PivotVec.y  ,PivotVec.z+s,col);
      vp+=6;
      ip[0] = 0;
      ip[1] = 1;
      ip[2] = 2;
      ip[3] = 3;
      ip[4] = 4;
      ip[5] = 5;
      ip+=6;

      sSystem->GeoEnd(handle);
      sSystem->GeoDraw(handle);
    }

    GenOp *op = App->GetEditOp();
    op->Class->DrawHints(op,handle);

    // end drawing

    sSystem->GeoRem(handle);
  }
}

void ViewMeshWin_::Solid(GenMobMesh *mesh,const sViewport &view,const sMaterialEnv &env)
{
  sInt stride;
  sU32 *data;
  sIMatrix34 mat;
  GenMobCluster *cl;
  GenBitmap *bm;

  if(!mesh->Soft)
  {
    sFORALL(mesh->Clusters,cl)
    {
      for(sInt i=0;i<2;i++)
      {
        if(cl->TextureOp[i])
        {
          const sInt xs=256;
          const sInt ys=256;
          bm = CalcBitmap(cl->TextureOp[i],xs,ys,GenBitmapTile::Pixel8C::Variant);
          if(bm)
          {
            if(!bm->Soft)
            {
              bm->Soft = new SoftImage(xs,ys);
              for(sInt y=0;y<ys;y++)
                for(sInt x=0;x<xs;x++)
                  bm->Soft->Data[x+y*xs] = *(sU32 *)bm->GetAdr(x,y);
            }
            cl->Image[i] = bm->Soft;
          }
        }
      }
    }
    mesh->Soft = MakeSoftMesh(mesh);
  }

  sSystem->Begin2D(&data,stride);
  stride = stride/4;

  mat.i.x = env.CameraSpace.i.x * sFIXONE;
  mat.i.y = env.CameraSpace.i.y * sFIXONE;
  mat.i.z = env.CameraSpace.i.z * sFIXONE;
  mat.j.x = env.CameraSpace.j.x * sFIXONE;
  mat.j.y = env.CameraSpace.j.y * sFIXONE;
  mat.j.z = env.CameraSpace.j.z * sFIXONE;
  mat.k.x = env.CameraSpace.k.x * sFIXONE;
  mat.k.y = env.CameraSpace.k.y * sFIXONE;
  mat.k.z = env.CameraSpace.k.z * sFIXONE;
  mat.l.x = env.CameraSpace.l.x * sFIXONE * 256;
  mat.l.y = env.CameraSpace.l.y * sFIXONE * 256;
  mat.l.z = env.CameraSpace.l.z * sFIXONE * 256;


  if(!ShowDoubleSize)
  {
    Soft->Begin(data+view.Window.x0+view.Window.y0*stride,stride,view.Window.XSize(),view.Window.YSize(),ClearColor);
    Soft->SetViewport(mat,sFIXONE);
    Soft->Paint(mesh->Soft);
    if(ShowEngineDebug)
      Soft->DebugOut();
    Soft->End();
  }
  else
  {
    Soft->Begin(data+view.Window.x0+view.Window.y0*stride,stride,view.Window.XSize()/2,view.Window.YSize()/2,ClearColor);
    Soft->SetViewport(mat,sFIXONE);
    Soft->Paint(mesh->Soft);
    if(ShowEngineDebug)
      Soft->DebugOut();
    Soft->EndDouble();
  }

  sSystem->End2D();
}

void ViewMeshWin_::Wireframe(GenMobMesh *mesh,const sViewport &view)
{
  sInt ic,vc,fc;
  GenMobFace *mf;
  GenMobVertex *mv;
  sU16 *ip;
  sVertexCompact *vp;
  sInt handle;

  // count object

  ic = 0;
  fc = 0;
  sFORALL(mesh->Faces,mf)
  {
    ic += mf->Count;
    if(mf->Select)
      fc += (mf->Count-2)*3;
  }

  // selected faces

  if(fc>0)
  {
    handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_TRI|sGEO_DYNAMIC);
    sSystem->GeoBegin(handle,mesh->Vertices.Count,fc,(sF32 **)&vp,(void **)&ip);    

    mv = mesh->Vertices.Array;
    vc = 0;
    sFORALL(mesh->Vertices,mv)
    {
      vp->Init(mv->Pos.x*1.0f/sFIXONE,mv->Pos.y*1.0f/sFIXONE,mv->Pos.z*1.0f/sFIXONE,0xff403020); 
      vp++;
      if(mv->Select) vc++;
    }

    sFORALL(mesh->Faces,mf)
    {
      if(mf->Select)
      {
        for(sInt i=2;i<mf->Count;i++)
        {
          *ip++ = mf->Vertex[0  ].Index;
          *ip++ = mf->Vertex[i-1].Index;
          *ip++ = mf->Vertex[i  ].Index;
        }
      }
    }

    sSystem->GeoEnd(handle);
    sSystem->GeoDraw(handle);
    sSystem->GeoRem(handle);
  }

  // draw wireframe

  handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE|sGEO_DYNAMIC);
  sSystem->GeoBegin(handle,mesh->Vertices.Count,ic*2,(sF32 **)&vp,(void **)&ip);    

  mv = mesh->Vertices.Array;
  vc = 0;
  sFORALL(mesh->Vertices,mv)
  {
    vp->Init(mv->Pos.x*1.0f/sFIXONE,mv->Pos.y*1.0f/sFIXONE,mv->Pos.z*1.0f/sFIXONE,0xff808080); 
    vp++;
    if(mv->Select) vc++;
  }

  sFORALL(mesh->Faces,mf)
  {
    sInt last = mf->Vertex[mf->Count-1].Index;
    for(sInt j=0;j<mf->Count;j++)
    {
      *ip++ = last;
      last = mf->Vertex[j].Index;
      *ip++ = last;
    }
  }

  sSystem->GeoEnd(handle);
  sSystem->GeoDraw(handle);

  if(mesh->CamSpline.Count>=2)
  {
    sIVector3 c,t;
    sInt maxtime;
    sInt ic;

    maxtime = mesh->CamSpline[mesh->CamSpline.Count-1].Time;
    ic = maxtime/sFIXONE;

    if(ic>0)
    {
      sSystem->GeoBegin(handle,ic*2,ic*2,(sF32 **)&vp,(void **)&ip);    
      for(sInt i=0;i<ic;i++)
      {
        mesh->CalcSpline(i*sFIXONE,c,t);

        *ip++ = i*2+0;
        *ip++ = i*2+1;
        vp->x = c.x*1.0f/sFIXONE;
        vp->y = c.y*1.0f/sFIXONE;
        vp->z = c.z*1.0f/sFIXONE;
        vp->c0 = 0xff406080;
        vp++;
        vp->x = t.x*1.0f/sFIXONE;
        vp->y = t.y*1.0f/sFIXONE;
        vp->z = t.z*1.0f/sFIXONE;
        vp->c0 = 0xff406080;
        vp++;
      }

      sSystem->GeoEnd(handle);
      sSystem->GeoDraw(handle);
    }

    ic = maxtime/(sFIXONE/16);

    if(ic>0)
    {
      sSystem->GeoBegin(handle,(ic-1)*2,(ic-1)*2,(sF32 **)&vp,(void **)&ip);    

      mesh->CalcSpline(0,c,t);
      for(sInt i=1;i<ic;i++)
      {
        vp->x = c.x*1.0f/sFIXONE;
        vp->y = c.y*1.0f/sFIXONE;
        vp->z = c.z*1.0f/sFIXONE;
        vp->c0 = 0xff406080;
        vp++;
        mesh->CalcSpline(i*(sFIXONE/16),c,t);
        vp->x = c.x*1.0f/sFIXONE;
        vp->y = c.y*1.0f/sFIXONE;
        vp->z = c.z*1.0f/sFIXONE;
        vp->c0 = 0xff406080;
        vp++;

        *ip++ = (i+1)*2+0;
        *ip++ = (i+1)*2+1;
      }

      sSystem->GeoEnd(handle);
      sSystem->GeoDraw(handle);
    }
  }

  sSystem->GeoRem(handle);

  // selected verts

  if(vc>0 && vc<0x4000)
  {
    sMatrix mat;

    handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_QUAD|sGEO_DYNAMIC);
    sSystem->GeoBegin(handle,vc*4,0,(sF32 **)&vp,0);

    sSystem->GetTransform(sGT_MODELVIEW,mat);
    mat.Trans4();

    const sF32 scale = 6.0f/view.Window.XSize();
    const sU32 col=0xffc0c0c0;
    sFORALL(mesh->Vertices,mv)
    {
      if(mv->Select)
      {
        sVector pos;
        pos.Init3(mv->Pos.x*1.0f/sFIXONE,mv->Pos.y*1.0f/sFIXONE,mv->Pos.z*1.0f/sFIXONE);
        sF32 projZ = pos.x*mat.k.x + pos.y*mat.k.y + pos.z*mat.k.z + mat.k.w;
        sF32 size = (sFAbs(projZ) + 0.001f) * scale;
        sVector vx,vy;

        // calc local axes
        vx.Scale3(mat.i,size);
        vy.Scale3(mat.j,size);

        // gen verts
        vp[0].x = pos.x - vx.x + vy.x;
        vp[0].y = pos.y - vx.y + vy.y;
        vp[0].z = pos.z - vx.z + vy.z;
        vp[0].c0 = col;

        vp[1].x = pos.x + vx.x + vy.x;
        vp[1].y = pos.y + vx.y + vy.y;
        vp[1].z = pos.z + vx.z + vy.z;
        vp[1].c0 = col;

        vp[2].x = pos.x + vx.x - vy.x;
        vp[2].y = pos.y + vx.y - vy.y;
        vp[2].z = pos.z + vx.z - vy.z;
        vp[2].c0 = col;

        vp[3].x = pos.x - vx.x - vy.x;
        vp[3].y = pos.y - vx.y - vy.y;
        vp[3].z = pos.z - vx.z - vy.z;
        vp[3].c0 = col;
        
        vp += 4;
      }
    }

    sSystem->GeoEnd(handle);
    sSystem->GeoDraw(handle);
    sSystem->GeoRem(handle);
  }
}

void ViewMeshWin_::OnKey(sU32 key)
{
  switch(key)
  {
  case sKEY_WHEELUP:
    Cam.Pivot = sRange<sF32>(Cam.Pivot*1.125f,1024,0.125f);
    ShowPivotVecTimer = 250;
    CamMatrix.InitSRT(Cam.State);
    PivotVec = CamMatrix.l;
    PivotVec.AddScale3(CamMatrix.k,Cam.Pivot);
    break;

  case sKEY_WHEELDOWN:
    Cam.Pivot = sRange<sF32>(Cam.Pivot/1.125f,1024,0.125f);
    ShowPivotVecTimer = 250;
    CamMatrix.InitSRT(Cam.State);
    PivotVec = CamMatrix.l;
    PivotVec.AddScale3(CamMatrix.k,Cam.Pivot);
    break;

  default:    
    MenuListKey(key);
    break;
  }

  switch(key&(0x8001ffff))
  {
  case 'w':
  case 'W':
    QuakeMask |= 1;
    break;
  case 's':
  case 'S':
    QuakeMask |= 2;
    break;
  case 'd':
  case 'D':
    QuakeMask |= 4;
    break;
  case 'a':
  case 'A':
    QuakeMask |= 8;
    break;
  case 'q':
  case 'Q':
    QuakeMask |= 16;
    break;
  case 'e':
  case 'E':
    QuakeMask |= 32;
    break;
  case 'w'|sKEYQ_BREAK:
  case 'W'|sKEYQ_BREAK:
    QuakeMask &= ~1;
    break;
  case 's'|sKEYQ_BREAK:
  case 'S'|sKEYQ_BREAK:
    QuakeMask &= ~2;
    break;
  case 'd'|sKEYQ_BREAK:
  case 'D'|sKEYQ_BREAK:
    QuakeMask &= ~4;
    break;
  case 'a'|sKEYQ_BREAK:
  case 'A'|sKEYQ_BREAK:
    QuakeMask &= ~8;
    break;
  case 'q'|sKEYQ_BREAK:
  case 'Q'|sKEYQ_BREAK:
    QuakeMask &= ~16;
    break;
  case 'e'|sKEYQ_BREAK:
  case 'E'|sKEYQ_BREAK:
    QuakeMask &= ~32;
    break;
  }
}

void ViewMeshWin_::OnDrag(sDragData &dd)
{
  sInt mode;
  sU32 qual;
  sVector v;

  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 0;
    mode = 0;
    if(dd.Buttons& 1) mode = 1;   // lmb
    if(dd.Buttons& 2) mode = 2;   // rmb
    if(dd.Buttons& 4) mode = 3;   // mmb
    if(dd.Buttons& 8) mode = 4;   // xmb
    if(dd.Buttons&16) mode = 5;   // ymb
    qual = sSystem->GetKeyboardShiftState();
    if(qual&sKEYQ_CTRL)
      mode |= 16;
    else if(qual&sKEYQ_ALT)
      mode |= 32;
    else if(qual&sKEYQ_SHIFT)
      mode |= 64;

    if((dd.Buttons&2) && dd.Flags&sDDF_DOUBLE) 
    { 
      sGui->Post(CMD_VIEWMESH_WIRE,this); 
      break; 
    }

    switch(mode)
    {
    case 0x01:
      DragMode = DM_ROTATE;
			sCopyMem(CamStateBase,Cam.State,9*sizeof(sF32));
      break;
    case 0x11:
      DragMode = DM_ZOOM;
      DragStartFY = Cam.Zoom[Cam.Ortho];
      break;
    case 0x21:
      DragMode = DM_PIVOT;
      DragStartFY = Cam.Pivot;
	  	CamMatrix.InitSRT(Cam.State);
      break;

    case 0x02:
    case 0x22:
      DragMode = DM_ORBIT;
      DragStartY = 0;
      v.Init(Cam.State[6],Cam.State[7],Cam.State[8]);
      v.Sub3(PivotVec);
      v.Neg3();
      Cam.Pivot = v.Abs3();
      //mat.InitDir(v);
      //mat.FindEuler(Cam.State[3],Cam.State[4],Cam.State[5]);
      Cam.State[3] = sFATan2(-v.y,sFSqrt(v.x*v.x+v.z*v.z));
      Cam.State[4] = sFATan2(v.x,v.z);
      Cam.State[3] *= 1/sPI2F;  
      Cam.State[4] *= 1/sPI2F;
      Cam.State[5] = 0;//1/sPI2F;
			sCopyMem(CamStateBase,Cam.State,9*sizeof(sF32));
	  	CamMatrix.InitSRT(Cam.State);
      break;
    case 0x04:
    case 0x12:
      DragMode = DM_DOLLY;
      DragVec = CamMatrix.l;
      break;

    case 0x03:
    case 0x13:
    case 0x23:
      DragMode = DM_SCROLL;
      DragVec = CamMatrix.l;
      break;

    case 0x05:
      sGui->Post(CMD_VIEWMESH_WIRE,this);
      break;
    }
    break;
  case sDD_DRAG:
    switch(DragMode)
    {
    case DM_ORBIT:   // orbit 3d
			Cam.State[3]=CamStateBase[3]+(dd.DeltaY-DragStartY)*0.004f;
      if(Cam.State[3]>0.2499f)
      {
        Cam.State[3]=CamStateBase[3]=0.2499f;
        DragStartY = dd.DeltaY;
      }
      if(Cam.State[3]<-0.2499f)
      {
        Cam.State[3]=CamStateBase[3]=-0.2499f;
        DragStartY = dd.DeltaY;
      }
			Cam.State[4]=CamStateBase[4]+dd.DeltaX*0.004f;
			CamMatrix.InitSRT(Cam.State);
      ShowPivotVecTimer = 1;
      break;
    case DM_DOLLY:   // pan 3d
      CamMatrix.l = DragVec;
      CamMatrix.l.AddScale3(CamMatrix.k,(dd.DeltaY-dd.DeltaX)*0.02f);
      Cam.State[6] = CamMatrix.l.x;
      Cam.State[7] = CamMatrix.l.y;
      Cam.State[8] = CamMatrix.l.z;
      break;
    case DM_SCROLL:   // scroll 3d
      CamMatrix.l = DragVec;
      CamMatrix.l.AddScale3(CamMatrix.i,-dd.DeltaX*0.02f);
      CamMatrix.l.AddScale3(CamMatrix.j, dd.DeltaY*0.02f);
      Cam.State[6] = CamMatrix.l.x;
      Cam.State[7] = CamMatrix.l.y;
      Cam.State[8] = CamMatrix.l.z;
      break;
    case DM_ZOOM:   // zoom 3d
      if(Cam.Ortho==0)
        Cam.Zoom[0] = sRange<sF32>(DragStartFY + (dd.DeltaY-dd.DeltaX)*0.2f,256,1);
      else
        Cam.Zoom[1] = sRange<sF32>(DragStartFY + (dd.DeltaY-dd.DeltaX)*2.0f,4096,1);
      break;
    case DM_ROTATE:   // rotate 3d
			Cam.State[3]=CamStateBase[3]+dd.DeltaY*0.002f;
			Cam.State[4]=CamStateBase[4]+dd.DeltaX*0.002f;
			CamMatrix.InitSRT(Cam.State);
      break;
    case DM_PIVOT:
      Cam.Pivot = sRange<sF32>(DragStartFY + dd.DeltaY*0.125f,128,0.125f);
      ShowPivotVecTimer = 1;
      CamMatrix.InitSRT(Cam.State);
      PivotVec = CamMatrix.l;
      PivotVec.AddScale3(CamMatrix.k,Cam.Pivot);
      break;
    }
    break;
  case sDD_STOP:
    DragMode = 0;
    break;
  }
}

sBool ViewMeshWin_::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case CMD_VIEWMESH_MENU:
    PopupMenuList();
    break;

  case CMD_VIEWMESH_RESET:
    PivotVec.Init();
    ShowPivotVecTimer = 0;
    MeshWire = 0;
    ShowXZPlane = 0;
    ShowOrigSize = 0;
    ShowDoubleSize = 0;
    ShowEngineDebug = 0;
    Quant90Degree = 0;
    PlaySpline = 0;
    SplineTime = 0;

    QuakeSpeedForw=0;
    QuakeSpeedSide=0;
    QuakeSpeedUp=0;
    QuakeMask=0;

    Cam.State[0]=Cam.State[1]=Cam.State[2]=1.0f;
	  Cam.State[3]=Cam.State[4]=Cam.State[5]=0.0f;
	  Cam.State[6]=Cam.State[7]=0.0f; Cam.State[8]=-5.0f;
	  CamMatrix.InitSRT(Cam.State);
    Cam.Ortho = 0;
    Cam.Pivot = 5.0f;
    Cam.Zoom[0] = 128;
    Cam.Zoom[1] = 1024;
    break;

  case CMD_VIEWMESH_COLOR:
    break;
  case CMD_VIEWMESH_WIRE:
    MeshWire = !MeshWire;
    break;
  case CMD_VIEWMESH_ORTHOGONAL:
    Cam.Ortho = !Cam.Ortho;
    break;
  case CMD_VIEWMESH_ORIGSIZE:
    ShowOrigSize = !ShowOrigSize;
    ShowDoubleSize = 0;
    break;
  case CMD_VIEWMESH_DOUBLESIZE:
    ShowDoubleSize = !ShowDoubleSize;
    ShowOrigSize = 0;
    break;
  case CMD_VIEWMESH_ENGINEDEBUG:
    ShowEngineDebug = !ShowEngineDebug;
    break;
  case CMD_VIEWMESH_GRID:
    ShowXZPlane = !ShowXZPlane;
    break;
  case CMD_VIEWMESH_90DEGREE:
    Quant90Degree = !Quant90Degree;
    break;
  case CMD_VIEWMESH_SPLINE:
    PlaySpline = !PlaySpline;
    break;
  case CMD_VIEWMESH_FULLSIZE:
    FullSize = !FullSize;
    SetFullsize(FullSize);
    break;
  }
  return sTRUE;
}

/****************************************************************************/

void ViewMeshWin_::SetOp(GenOp *op)
{
  ShowOp = op;
}

/****************************************************************************/
