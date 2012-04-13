// This code is in the public domain. See LICENSE for details.

#include "types.h"
#include "opsys.h"

extern frOperator* create_TGRandomDots();
extern frOperator* create_TOTrivial();
extern frOperator* create_TGText();
extern frOperator* create_TFRotoZoom();
extern frOperator* create_TFAdjust();
extern frOperator* create_TCAdd();
extern frOperator* create_TCBump();
extern frOperator* create_TGPerlin();
extern frOperator* create_TCDisplace();
extern frOperator* create_TCSetAlpha();
extern frOperator* create_TCBlend();
extern frOperator* create_TGGradient();
extern frOperator* create_TCMultiply();
extern frOperator* create_TGCrystal();
extern frOperator* create_TFBlurV2();
extern frOperator* create_TFInstallFont();

extern frOperator* create_GGPrimitive();
extern frOperator* create_GCAdd();
extern frOperator* create_GFTransform();
extern frOperator* create_GFCloneXForm();
extern frOperator* create_GSMaterial();
extern frOperator* create_GFSelect();
extern frOperator* create_GFDeleteSelection();
extern frOperator* create_GFBend();
extern frOperator* create_GFUVMap();
extern frOperator* create_GFDoubleSided();
extern frOperator* create_GSLoadMaterial();
extern frOperator* create_GSLighting();
extern frOperator* create_GFObjMerge();
extern frOperator* create_GFApplyLightmap();
extern frOperator* create_GFWuschel();
extern frOperator* create_GFPlode();
extern frOperator* create_GFDumbNormals();
extern frOperator* create_GFDisplacement();
extern frOperator* create_GFGlowVerts();
extern frOperator* create_GCShapeAnim();
extern frOperator* create_GSRenderPass();
extern frOperator* create_GGLorentz();
extern frOperator* create_GFJitter();

extern frOperator* create_CGCamera();

frOperatorDef operatorDefs[] =
{
  0x00,    0, 0,  0,  create_TGRandomDots,
  0x01,    0, 0, 10,  create_TOTrivial, // rectangle
  0x02,    0, 0,  3,  create_TOTrivial, // solid
  0x03,    0, 0,  0,  create_TGText,
  0x07,    1, 0,  2,  create_TOTrivial, // make normal map
  0x08,    1, 0,  0,  create_TFRotoZoom,
  0x09,    1, 0,  8,  create_TFAdjust,
  0x0a, 0xff, 0,  0,  create_TCAdd,
  0x0b,    2, 0,  0,  create_TCBump,
  0x0f,    0, 0,  0,  create_TGPerlin,
  0x10,    2, 0,  6,  create_TOTrivial, // displace
  0x11,    2, 0,  1,  create_TOTrivial, // set alpha
  0x12,    3, 0,  1,  create_TOTrivial, // blend
  0x13,    0, 0,  0,  create_TGGradient,
  0x14, 0xff, 0,  0,  create_TCMultiply,
  0x15,    0, 0,  0,  create_TGCrystal,
  0x16,    1, 0,  0,  create_TFBlurV2,
	//0x18,		 1, 0,  0,	create_TFInstallFont,

  0x80,    0, 0,  0,  create_GGPrimitive,
  0x81, 0xff, 0,  0,  create_GCAdd,
  0x82,    1, 0,  0,  create_GFTransform,
  0x83,    1, 0,  0,  create_GFCloneXForm,
  0x84, 0xff, 2,  0,  create_GSMaterial,
  0x85,    1, 0,  0,  create_GFSelect,
  0x86,    1, 0,  0,  create_GFDeleteSelection,
  0x87,    1, 0,  0,  create_GFBend,
  0x88,    1, 0,  0,  create_GFUVMap,
  0x89,    1, 0,  0,  create_GFDoubleSided,
  0x8d,    1, 1,  0,  create_GSLoadMaterial,
  0x8e,    1, 0,  0,  create_GSLighting,
  0x8f,    1, 0,  0,  create_GFObjMerge,
  //0x90,    1, 1,  0,  create_GFApplyLightmap,
  0x92,    1, 0,  0,  create_GFWuschel,
  0x93,    1, 0,  0,  create_GFPlode,
  0x94,    1, 0,  0,  create_GFDumbNormals,
  0x95,    1, 1,  0,  create_GFDisplacement,
  0x96,    1, 0,  0,  create_GFGlowVerts,
  0x97, 0xff, 0,  0,  create_GCShapeAnim,
  0x98,    1, 0,  0,  create_GSRenderPass,
	//0x9c,		 0,	0,  0,	create_GGLorentz,
	0x9d,		 1, 0,  0,	create_GFJitter,

  0xc0, 0xff, 1,  0,  create_CGCamera,
};
