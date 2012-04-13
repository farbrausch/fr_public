// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types2.hpp"
#include "doc.hpp"
#include "materials/material11.hpp"

class ViewMeshWin_ : public sGuiWindow2
{
  struct CameraState
  {
	  sF32 State[9];                  // SRT
    sF32 Zoom[2];                   // Zoom[perspective, ortho]
    sInt Ortho;                     // orthogonal mode
    sF32 Pivot;                     // camera rotation pivot
  };

  sInt ThisTime;
  sInt LastTime;

  sInt FullSize;
  sInt ShowPivotVecTimer;
  sInt MeshWire;
  sInt ShowXZPlane;
  sInt ShowOrigSize;
  sInt ShowDoubleSize;
  sInt ShowEngineDebug;
  sInt Quant90Degree;
  sU32 ClearColor;
  sInt PlaySpline;
  sInt SplineTime;


  sU32 QuakeMask;
  sF32 QuakeSpeedForw;
  sF32 QuakeSpeedSide;
  sF32 QuakeSpeedUp;
  sMatrix CamMatrix;
	sF32 CamStateBase[9];

  CameraState Cam;


  sMaterialEnv MtrlEnv;
  sMaterial11 *Mtrl;

  sVector DragVec;
  sVector PivotVec;
  sF32 DragStartFX;
  sF32 DragStartFY;
  sInt DragStartX;
  sInt DragStartY;
  sInt DragMode;                  // DM_???


  enum DragModeEnum
  {
    DM_ORBIT = 1,
    DM_DOLLY,
    DM_SCROLL,
    DM_ZOOM,
    DM_ROTATE,
    DM_PIVOT,
  };
  enum CommandEnum
  {
    CMD_VIEWMESH_RESET = 0x1000,
    CMD_VIEWMESH_COLOR,
    CMD_VIEWMESH_WIRE,
    CMD_VIEWMESH_ORTHOGONAL,
    CMD_VIEWMESH_ORIGSIZE,
    CMD_VIEWMESH_DOUBLESIZE,
    CMD_VIEWMESH_ENGINEDEBUG,
    CMD_VIEWMESH_GRID,
    CMD_VIEWMESH_90DEGREE,
    CMD_VIEWMESH_FULLSIZE,
    CMD_VIEWMESH_SPLINE,
    CMD_VIEWMESH_MENU,
  };

  sToolBorder *Tools;

  void Wireframe(class GenMobMesh *mesh,const sViewport &view);
  void Solid(class GenMobMesh *mesh,const sViewport &view,const sMaterialEnv &env);
  
public:
  ViewMeshWin_();
  ~ViewMeshWin_();
  void OnPaint();
  void OnPaint3d(sViewport &view);
  void OnKey(sU32 key);
  void OnDrag(sDragData &dd);
  sBool OnCommand(sU32 cmd);

  class GenMobMesh *Object;
  class SoftEngine *Soft;
  GenOp *ShowOp;

  void SetOp(GenOp *);
};
