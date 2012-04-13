// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __GENMESH2_HPP__
#define __GENMESH2_HPP__

#include "_types.hpp"
#include "genmaterial.hpp"
#if sLINK_XSI
#include "_xsi.hpp"
#include "cslrt.hpp"
#endif

/****************************************************************************/

#define sGM_MAXTEX 8

struct GenMeshVert;
struct GenMeshEdge;
struct GenMeshFace;
struct GenMeshCell;

class GenMaterial;
class GenBitmap;

// selection mask:  00vvffee --- vert:corn:face:edge

/****************************************************************************/
 
struct GenMeshElem                          // same for all elements of a mesh
{
  sU8 Mask;                                 // most selections are done with bitmask
  sU8 Id;                                   // you can assign an id and generate masks from it
  sU8 Select;                               // selection is set from the mask and used internally
  sU8 Used;                                 // unused elements are (supposed to be) removed during cleanup.

  void InitElem();
  void SelElem(sU32 mask1,sU32 mask0=0);
};

struct GenMeshVert : public GenMeshElem     // vertex data. maybe multiple per corner
{
  sInt Next;                                // next vertex
  sInt First;                               // the vertex containing the "real" position.
  sInt Temp;
  sInt Temp2;
  sInt ReIndex;
  sU8 Weight[4];                            // palette skinning
  sU8 Matrix[4];

  void Init();
  void Sel(sU32 mask1,sU32 mask0=0) {SelElem(mask1>>16,mask0>>16);}
};

struct GenMeshEdge : public GenMeshElem     // edge of a mesh
{
  sInt Next[2];                             // next edge in face. low bit set when edge[i]->Face[1]==face[i]
  sInt Prev[2];                             // prev edge in face. low bit set when edge[i]->Face[1]==face[i]

  sInt Face[2];                             // faces each side of edge
  sInt Vert[2];                             // vertices connected by edge

  sInt Temp[2];
  sInt Crease;

  void Init();
  void Sel(sU32 mask1,sU32 mask0=0) {SelElem(mask1>>0,mask0>>0);}
};       

struct GenMeshFace : public GenMeshElem     // face of a mesh
{
  sInt Material;                            // material used, 0 = face deleted
  sInt Edge;                                // one of the edges. low bit set when Edge->Face[1]==this
  sInt Temp;
  sInt Temp2;
  sInt Temp3;

  void Init();
  void Sel(sU32 mask1,sU32 mask0=0) {SelElem(mask1>>8,mask0>>8);}
};

struct GenMeshPass
{
  class GenMatPass *MatPass;                // ptr to material pass, contains FVF and mode

  sInt BatchCount;
  sInt Geometry[GENMAT_MAXBATCH];           // handle to geometry object
  sInt IndexCount[GENMAT_MAXBATCH];         // number of indices in indexbuffer. have to store static indexbuffers if vertexbuffer is dynamic. stupid...
  sInt VertexCount[GENMAT_MAXBATCH];        // in case of dynamic vertexbuffer, this contains a cached mapping from mesh vertex buffer to hardware vertex buffer
  sU16 *IndexBuffers[GENMAT_MAXBATCH];      // the indices. vertx buffer must be clipped to 32k vertices max (for matrox g400 bug)
  sInt *VertexBuffers[GENMAT_MAXBATCH];     // indices here! a mesh may contain more than 64k vertices

  void Init();
	void Reset();
};

struct GenMeshMtrl : public GenMeshElem     // material of a face
{
  GenMaterial *Material;                    // the material to use
  GenMeshPass Pass[GENMAT_MAXPASS];          // information about each pass to render.

  void Init();
  void Exit();
};


struct GenMeshMatrix
{
  sF32 BaseSRT[9];
  sF32 TransSRT[9];
  sMatrix Matrix;
  sInt Parent;
  sInt Used;
};

struct GenMeshCurve 
{
  sInt Offset;
  sInt Matrix;
  sInt Curve;
};

/****************************************************************************/

#define sGMI_POS          (0)
#define sGMI_NORMAL       (1)
#define sGMI_TANGENT      (2)
#define sGMI_COLOR0       (3)
#define sGMI_COLOR1       (4)
#define sGMI_UV0          (5)
#define sGMI_UV1          (6)
#define sGMI_UV2          (7)
#define sGMI_UV3          (8)
#define sGMI_LAST         sGMI_UV3

#define sGMF_POS          (1<<0)
#define sGMF_NORMAL       (1<<1)
#define sGMF_TANGENT      (1<<2)
#define sGMF_COLOR0       (1<<3)
#define sGMF_COLOR1       (1<<4)
#define sGMF_UV0          (1<<5)
#define sGMF_UV1          (1<<6)
#define sGMF_UV2          (1<<7)
#define sGMF_UV3          (1<<8)

#define sGMF_NORMALS      (sGMF_NORMAL|sGMF_TANGENT)

/****************************************************************************/

class GenMeshAnim : public sObject
{
public:
  sU32 GetClass() { return sCID_GENMESHANIM; }
  void Copy(sObject *);

  sInt3 s,r,t;
  sInt3 v;
  sInt p;
  sInt MatrixIndex;
  sInt Operation;
  sU32 Flags;
  sMatrix OMat;
  sVector OVec;
};

#define GMA_NOP         0
#define GMA_TRANSFORM   1
#define GMA_PERLIN      2
#define GMA_BONE        3
#define GMA_EXNORMAL    4
#define GMA_EWK         5

#define GMF_DOUBLE      0x0001        // this label has no connection, it duplicates data from label above!

/****************************************************************************/

class GenMesh : public sObject
{
public:
  GenMesh();
  ~GenMesh();
  void Tag();
  sU32 GetClass() { return sCID_GENMESH; }
  void Copy(sObject *);

  static sU32 Features2Mask(sInt colorSets,sInt uvSets);

  void Init(sU32 vertmask,sInt vertcount);    // count is only an estimate (state too large!)
  void Realloc(sInt vertcount);
  void MarkAnimLabel(sInt mode);
  void SetAnimLabel(sInt label,sU32 flags);

  sArray<GenMeshVert> Vert;
  sArray<GenMeshEdge> Edge;
  sArray<GenMeshFace> Face;
  sArray<GenMeshMtrl> Mtrl;       // material 0 is unused!

  sVector *VertBuf;               // structure of arrays for Vertexbuffers, 0 = unused
  sInt VertMask;                  // gmf flags
  sInt VertSize;                  // vectors per vertex
  sInt VertCount;                 // vertex count
  sInt VertAlloc;                 // vertices allocated
  sInt VertMap[16];               // map to index;

  sArray<GenMeshMatrix> BoneMatrix;
  sArray<GenMeshCurve> BoneCurve;
  sInt KeyCount;
  sInt CurveCount;
  sF32 *KeyBuf;
  sInt Prepared;                  // once an object is preared, you may not change it anymore!!!
  sInt Anim0,Anim1;               // bone animation range (in frames)

  sInt StoreMode;                 // if switched on, RecEnd adds to Store.
  sInt RecMode;                   // 0=off, 1=immediate, 2=record
  sInt RecLevel;                  // RecBegin() RecEnd() counting
  sArray<sVector> RecVert;    // vertices at start of recording
  sArray<sU32> RecCmd;
  sArray<sObject*> RecObj;
  sArray<sMatrix> RecMat;

  GenMeshAnim *AnimLabels[32];    // animation labels
  sInt AnimLabelCount;            // number of animlabels already allocated
  sInt AnimLabelLastMat;          // last animatable records matrix
  sInt AnimLabelLastOp;           // last animatable records GMA_??? (nop for empty)

  sInt RecLastSource;             // temporary for interpreting the command stream
	sInt RecLastDest;
	sInt RecOpCount;                // don't know. written but never read.
	sU32 *RecOpCountPtr;
	sInt RecCommandLevel;           // used to correctly determine RecOpCount

  // i = edge<<1 | direction
  inline GenMeshEdge *GetEdge(sU32 i) { return &Edge.Array[i>>1]; }
  GenMeshFace *GetFace(sU32 i);
  GenMeshFace *GetFaceI(sU32 i);
  GenMeshVert *GetVert(sU32 i);
  sInt GetFaceId(sU32 i);
  sInt GetVertId(sU32 i);
  sInt NextFaceEdge(sU32 i);
  sInt PrevFaceEdge(sU32 i);
  sInt NextVertEdge(sU32 i);
  sInt PrevVertEdge(sU32 i);

  /*inline GenMeshFace *GetFace(sU32 i) { return &Face.Array[GetEdge(i)->Face[i&1]]; }
  inline GenMeshFace *GetFaceI(sU32 i) { return &Face.Array[GetEdge(i)->Face[(i&1)^1]]; }
  inline GenMeshVert *GetVert(sU32 i) { return &Vert.Array[GetEdge(i)->Vert[i&1]]; }
  inline sInt GetFaceId(sU32 i) { return GetEdge(i)->Face[i&1]; }
  inline sInt GetVertId(sU32 i) { return GetEdge(i)->Vert[i&1]; }
  inline sInt NextFaceEdge(sU32 i) { return GetEdge(i)->Next[i&1]; }
  inline sInt PrevFaceEdge(sU32 i) { return GetEdge(i)->Prev[i&1]; }
	inline sInt NextVertEdge(sU32 i) { return GetEdge(i)->Prev[i&1]^1; }
  inline sInt PrevVertEdge(sU32 i) { return GetEdge(i)->Next[(i&1)^1]; }*/

  inline sVector &VertPos(sInt v) { return VertBuf[v*VertSize]; }
  inline sVector &VertNorm(sInt v) { return VertBuf[v*VertSize+1]; }

#if !sPLAYER
	sInt GetValence(sU32 e);
	sInt GetDegree(sU32 e);
#endif

  sInt AddVert();
  sInt AddNewVert();
  sInt AddCopiedVert(sInt src);
  void SplitFace(sU32 e0,sU32 e1,sU32 dmask1=0,sU32 dmask0=0);						// create edge from start of e0 to start of e1 and split face
  void SplitBridge(sU32 i,sU32 e,sInt &va,sInt &vb,sU32 dmask1=0,sU32 dmask0=0);    // creates a "bridge" edge between i and e
  sInt AddEdge(sInt v0,sInt v1,sInt face);
	void MakeFace(sInt face,sInt nedges,...);
#if !sINTRO
  void Verify();
#endif
  void ReIndex();
  void CleanupMesh();
  sBool IsBorderEdge(sU32 i,sInt mode);

  void RecPlay(sU32 *code,sMatrix *matrices,sObject **objects);
  sU32 *RecBegin(sU32 command=0);
  sInt RecMatrix(const sMatrix &mat);
	sInt RecObject(sObject *obj);
  void RecEnd();
  void RecEnd(sU32 *);
  void RecStoreMode();
  void RecReplay();

	sU32 *RecSourceInd(sU32 *data,sInt ind);
	sU32 *RecDestInd(sU32 *data,sInt ind);

	void RecAddScaleArray(sInt dest,sU32 count,sInt mode,sU32* data);
	void RecAddScale(sInt dest,sU32 count,sInt mode,...);

	sU32 *RecTransformInner(sU32 *code,sMatrix *matrices);
	sU32 *RecBoneInner(sU32 *code,sMatrix *matrices);
	sU32 *RecAddScaleInner(sU32 *code,sMatrix *matrices);
	sU32 *RecNormalInner(sU32 *code,sMatrix *matrices);
	sU32 *RecDisplaceInner(sU32 *code,sMatrix *matrices,sObject **objects);
	sU32 *RecBevelInner(sU32 *code,sMatrix *matrices);
	sU32 *RecPerlinInner(sU32 *code,sMatrix *matrices);
	sU32 *RecCopyInner(sU32 *code,sMatrix *matrices,sObject **objects);
  sU32 *RecF2VNInner(sU32 *code,sMatrix *matrices);
  sU32 *RecEWKInner(sU32 *code,sMatrix *matrices);

  void Mask2Sel(sU32 mask);
  void All2Sel(sBool set=1);
  void Id2Mask(sU32 mask,sInt id);
  void Mask2Mask(sU32 mask,sU32 dmask1,sU32 dmask0=0);
  void Sel2Mask(sU32 dmask1,sU32 dmask0=0);
  void All2Mask(sU32 dmask1,sU32 dmask0=0);
  void Face2Vert();
	void Edge2Vert(sInt uvs);
  void SetMaterial(sInt id,GenMaterial *mat);
	sInt FindCrease(sInt edge,sInt mask);
  void CalcNormal(sVector &n,sInt o0,sInt o1,sInt o2);
  void CalcFaceNormal(sVector &n,sInt f);

  void TransVert(sMatrix &mat,sInt sj=0,sInt dj=0);
  void SelectCube(const sVector &vc,const sVector &vs,sBool set,sU32 dmask1,sU32 dmask0=0);
  void Ring(sInt count,sU32 dmask,sF32 radius,sF32 phase);
  void Extrude(sU32 dmask1,sU32 dmask0=0,sInt mode=0);
  void Subdivide(sF32 alpha=1.0f);
  sInt Bones(sF32 phase);
  void BonesModify(sInt matrix,sF32 phase);
  void FixVertCycle(sInt edge);
  void SingleCrease(sU32 i,sU32 dmask1=0,sU32 dmask=0,sInt what=0xfe);
  void Crease(sInt selType=0,sU32 dmask1=0,sU32 dmask0=0,sInt what=0xfe);
  void UnCrease(sBool edge=0,sInt what=0xfe);
	void CalcNormals(sInt type=0,sInt calcWhat=1);
	void Triangulate(sInt threshold=5,sU32 dmask1=0,sU32 dmask0=0,sInt type=0); // Triangulates selected polygons
	void Cut(const sVector &plane,sInt mode=1); // Cut or clip by a plane
	void ExtrudeNormal(sF32 distance);
	void Displace(GenBitmap *bitmap,sF32 amplitude);
	void Bevel(sF32 elevate,sF32 pullin,sInt mode,sU32 dmask1=0,sU32 dmask0=0);
	void Perlin(const sMatrix &mat,const sVector &amp);
	sBool Add(GenMesh *other);
	void DeleteFaces();

#if sLINK_XSI
  void ImportXSI(sXSILoader *xsi);
  void ImportXSIR(sXSIModel *xm,sInt parent);
  void ImportXSI(sXSICluster *xc,sInt firstvertex);
#endif
  void WriteCompact(sU8 *&p);
  void ReadCompact(sU8 *&p,sU32 gmf);
  void Prepare();                 // prepare fore painting
  void Paint();                   // paint fast
  void DoPass1(GenMeshPass *pass,sInt mati);
  void DoPass2(GenMeshPass *pass,sInt mati);
  void PaintWire(sU32 mask);      // paint debug wireframe
  void PaintSolid();              // paint debug solid
};

#define RM_POS		sGMF_POS
#define RM_NORMAL	(sGMF_NORMAL|sGMF_TANGENT)
#define RM_REST		(sGMF_COLOR0|sGMF_COLOR1|sGMF_UV0|sGMF_UV1|sGMF_UV2|sGMF_UV3)
#define RM_ALL		(RM_POS|RM_NORMAL|RM_REST)

#define IM_XFORM		1			// innerloop mode straight transform
#define IM_BONE			2			// bones
#define IM_ADDSCL		3			// add-scale
#define IM_NORMAL		4			// normals
#define IM_DISPLACE	5			// displacement
#define IM_BEVEL		6			// bevel
#define IM_PERLIN		7			// perlin
#define IM_COPY			8			// copy
#define IM_F2VN     9     // face to vert normal
#define IM_EWK      10    // extrudewk

/****************************************************************************/

sObject * __stdcall Mesh_Cube(GenMesh *msh,sInt tx,sInt ty,sInt tz,sInt crease);
sObject * __stdcall Mesh_SelectAll(GenMesh *msh,sU32 mask1,sU32 mask0);
sObject * __stdcall Mesh_SelectCube(GenMesh *msh,sF323 center,sF323 size,sInt flags,sU32 dmask1,sU32 dmask0);
sObject * __stdcall Mesh_Subdivide(GenMesh *msh,sInt mask,sF32 alpha,sInt count);
sObject * __stdcall Mesh_Extrude(GenMesh *msh,sInt smask,sInt mode,sInt dmask);
sObject * __stdcall Mesh_Transform(GenMesh *msh,sInt mask,sF323 s,sF323 r,sF323 t);
sObject * __stdcall Mesh_Cylinder(GenMesh *msh,sInt tx,sInt ty);
sObject * __stdcall Mesh_NewMesh(sInt colorSets,sInt uvSets);
sObject * __stdcall Mesh_Material(GenMesh *msh,GenMaterial *mat,sInt id,sInt mask);
sObject * __stdcall Mesh_TransformEx(GenMesh *msh,sInt mask,sInt sj,sInt dj,sF323 s,sF323 r,sF323 t);
sObject * __stdcall Mesh_Crease(GenMesh *msh,sInt mask,sInt what,sInt selType);
sObject * __stdcall Mesh_UnCrease(GenMesh *msh,sInt mask,sInt what,sInt selType);
sObject * __stdcall Mesh_CalcNormals(GenMesh *msh,sInt mode,sInt mask,sInt calcWhat);
sObject * __stdcall Mesh_Torus(GenMesh *msh,sInt x,sInt y,sF32 radiusi,sInt phase,sF32 arclen);
sObject * __stdcall Mesh_Sphere(GenMesh *msh,sInt x,sInt y);
sObject * __stdcall Mesh_Triangulate(GenMesh *msh,sInt threshold,sInt mask,sInt type);
sObject * __stdcall Mesh_Cut(GenMesh *msh,sF322 dir,sF32 offset,sInt mode);
sObject * __stdcall Mesh_ExtrudeNormal(GenMesh *msh,sF32 distance,sInt mask);
sObject * __stdcall Mesh_Displace(GenMesh *msh,GenBitmap *bmp,sF32 ampli,sInt mask);
sObject * __stdcall Mesh_Bevel(GenMesh *msh,sF32 elev,sF32 pull,sInt mask,sInt mode,sInt dmask);
sObject * __stdcall Mesh_Perlin(GenMesh *msh,sInt mask,sF323 s,sF323 r,sF323 t,sF323 ampl);
sObject * __stdcall Mesh_Add(GenMesh *msh,GenMesh *msh2);
sObject * __stdcall Mesh_DeleteFaces(GenMesh *msh,sInt mask);
sObject * __stdcall Mesh_Babe(sChar *filename,sInt dummy,sInt anim0,sInt anim1,sInt phase,sInt flags);
sObject * __stdcall Mesh_BeginRecord(GenMesh *msh);
sObject * __stdcall Mesh_AnimLabel(GenMesh *msh,sInt label,sU32 flags);
sObject * __stdcall Mesh_AnimLabelCont(GenMesh *msh);
sObject * __stdcall Mesh_SelectRandom(GenMesh *msh,sU32 ratio,sInt seed,sU32 dmask1,sU32 dmask0);
sObject * __stdcall Mesh_Multiply(GenMesh *msh,sF323 s,sF323 r,sF323 t,sInt count);
sObject * __stdcall Mesh_SelectAngle(GenMesh *msh,sF32 angle,sU32 dmask1,sU32 dmask0);
sObject * __stdcall Mesh_ExtrudeWK(GenMesh *msh,sInt mask,sInt mode,sInt count,sF32 distance,sF323 s,sF323 r,sInt scalemode); 

/****************************************************************************/

#endif
