// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __WINVIEW_HPP__
#define __WINVIEW_HPP__

#include "_gui.hpp"
#include "werkkzeug.hpp"
#include "kkriegergame.hpp"
#include "engine.hpp"
#include "genminmesh.hpp"

class sMaterial11;

#if sLINK_GUI

/****************************************************************************/

#define CMD_VIEW_POPUP              0x0100
#define CMD_VIEW_RESET              0x0101
#define CMD_VIEW_COLOR              0x0102
#define CMD_VIEW_WIRE               0x0107
#define CMD_VIEW_TILE               0x0108
#define CMD_VIEW_ALPHA              0x0109
#define CMD_VIEW_GAME               0x010a
#define CMD_VIEW_ORTHO              0x010b
#define CMD_VIEW_SELECTION          0x010c
#define CMD_VIEW_QUANT              0x010d
#define CMD_VIEW_SHOWLIGHTS         0x010e
#define CMD_VIEW_WIREOPT            0x010f
#define CMD_VIEW_WIREOPT2           0x0110
#define CMD_VIEW_90DEGREE           0x0111
#define CMD_VIEW_SHADOWTOG          0x0112
#define CMD_VIEW_LINKEDIT           0x0113
#define CMD_VIEW_GRID               0x0114
#define CMD_VIEW_ORIGSIZE           0x0115
#define CMD_VIEW_SCRATCHTIME0       0x0116
#define CMD_VIEW_SCRATCHTIME1       0x0117
#define CMD_VIEW_CAM2KEY            0x0118
#define CMD_VIEW_KEY2CAM            0x0119
#define CMD_VIEW_NEXTCAM            0x011a
#define CMD_VIEW_PREVCAM            0x011b
#define CMD_VIEW_FREECAMERA         0x011c
#define CMD_VIEW_TOGGLECAMSPEED0    0x011d
#define CMD_VIEW_TOGGLECAMSPEED2    0x011e
#define CMD_VIEW_TOGGLECAMSPEED3    0x011f
#define CMD_VIEW_NORMALS            0x0120
#define CMD_VIEW_FORCECAMLIGHT      0x0121

#define CMD_VIEW_LIGHT              0x0180
#define CMD_VIEW_USE                0x0190
#define CMD_VIEW_SETCAM             0x01a0
#define CMD_VIEW_GETCAM             0x01b0

#define CMD_VIEW_SETMATMESH         0x01c0

/****************************************************************************/

struct CameraState
{
	sF32 State[9];                  // SRT
  sF32 Zoom[2];                   // Zoom[perspective, ortho]
  sInt Ortho;                     // orthogonal mode
  sF32 Pivot;                     // camera rotation pivot
};

struct HintLine
{
  sF32 x0,y0,z0;
  sF32 x1,y1,z1;
  sU32 col;
};

class WinView : public sGuiWindow
{
  sVector DragVec;
  sVector PivotVec;
  sF32 DragStartFX;
  sF32 DragStartFY;
  sInt DragStartX;
  sInt DragStartY;
  sInt DragMode;                  // DM_???
  sInt DragCID;                   // sCID of current object
  sF32 *DragEditPtrX;
  sF32 *DragEditPtrY;
  WerkOp *DragEditOp;

  sInt LastTime;                  // for kkrieger ontick stuff
  sInt ThisTime;
  sInt ShowPivotVecTimer;         // set this timer when you want to show the pivot-vector for some frames.

  sInt LightPosX,LightPosY;       // for normalmap viewing

  sU32 QuakeMask;
  sF32 QuakeSpeedForw;
  sF32 QuakeSpeedSide;
  sF32 QuakeSpeedUp;
  sMatrix CamMatrix;
	sF32 CamStateBase[9];

  CameraState Cam;
  CameraState CamSave[16];

  sInt BitmapX;
  sInt BitmapY;
  sInt BitmapTile;
  sInt BitmapAlpha;
  sInt MeshWire;
  sInt SceneWire;
  sInt SceneLight;
  sInt SceneUsage;
  sInt QuantMode;                 // quantise SRT editing
  sInt BrowserMode;
  sInt Quant90Degree;
  sInt ScratchTimeMode;
  sInt ShowNormals;
  sInt ForceCameraLight;
  
  sInt RecordVideo;
  sF32 VideoFPS;
  sInt VideoFrame;

  sPerfInfo Perf;

  sInt Fullsize;

  void MakeCam(sMaterialEnv &env,sViewport &view);
  void ShowBitmap(sViewport &view,class GenBitmap *tex);
  void ShowScene(sViewport &view,WerkOp *op,KEnvironment *kenv);
  void ShowMesh(sViewport &view,class GenMesh *mesh,KEnvironment *kenv);
  void ShowMinMesh(sViewport &view,class GenMinMesh *mesh,KEnvironment *kenv);
  void ShowIPP(sViewport &view,WerkOp *op,KEnvironment *kenv);
  void ShowDemo(sViewport &view,WerkOp *op,KEnvironment *kenv);
  void ShowMaterial(sViewport &view,WerkOp *op,KEnvironment *kenv);
  void ShowSimpleMesh(sViewport &view,class GenSimpleMesh *mesh,KEnvironment *kenv);

  void ShowHints(sU32 flags,sMaterialEnv *env);
  void ShowNormalsAndTangents(GenMinMesh *mesh,sMaterialEnv *env);
  void ShowDynCell(sMaterialEnv *env);
  void ShowMeshLights(class GenMesh *mesh,sMaterialEnv *env);

  void LeaveGameMode();
  void EnterGameMode();

  void UpdateSceneWire();

  void FlushSceneWireR(WerkOp *op);
  void FlushSceneWire();
  void FlushMeshWire();
  void SetObjectIntern(WerkOp *,sBool real);

public:
  WinView();
  ~WinView();
  void Tag();
  void OnPaint();
  void OnFrame();
  void OnPaint3d(sViewport &view);
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  sBool OnCommand(sU32 cmd);
  void OnTick(sInt ticks);

  void SetObject(WerkOp *);
  void SetObjectB(WerkOp *);      // for Browser
  void SetOff();

  void ShowHintsPart(sMatrix &mat,const struct HintLine *data,sInt count,sU32 col,sMaterialEnv *env);
  void ShowBlobSplineHints(struct BlobSpline *spline,sMaterialEnv *env);

  void RecordAVI(sChar *filename,sF32 fps=30.0f);

  class WerkkzeugApp *App;
  WerkOp *Object;                 // object currently really showed
  WerkOp *RealObject;             // object selected for showing, not the object from browser

  sInt StatWire;                  // use wireframe statistics instesad
  sInt StatWireTri;
  sInt StatWireLine;
  sInt StatWireVert;
  sInt CamSpeed;

  sInt SelEdge;
  sInt SelVertex;
  sInt SelFace;
  sInt ShowLights;
  sU32 WireColor[EWC_MAX];
  sF32 HintSize;
  sInt WireOptions;
  sInt BitmapZoom;
  sInt ShowXZPlane;
  sInt ShowOrigSize;
  struct KKriegerGame Game;
  sInt GameMode;

  sInt MatMeshType;
  GenMesh *MatSrcMesh[2];
  EngMesh *MatMesh;

  sMaterialEnv MtrlEnv;

  sMaterial11 *MtrlTex;
  sMaterial11 *MtrlTexAlpha;
  sMaterial11 *MtrlNormal;
  sMaterial11 *MtrlNormalLight;
  sMaterial11 *MtrlFlatZ;

  sMaterial11 *MtrlCollision;

  sInt GeoLine;
  sInt GeoFace;
};

/****************************************************************************/

#endif
#endif
