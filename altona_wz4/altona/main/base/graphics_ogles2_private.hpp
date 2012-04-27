/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_BASE_GRAPHICS_OGLES2_PRIVATE_HPP
#define FILE_BASE_GRAPHICS_OGLES2_PRIVATE_HPP

#include "base/types.hpp"

/****************************************************************************/


#include "base/types.hpp"

/****************************************************************************/

enum
{
  sMTRL_MAXTEX = 4,                                  // max number of textures
  sMTRL_MAXPSTEX = 4,                                // max number of pixel shader samplers
  sMTRL_MAXVSTEX = 0,                                 // max number of vertex shader samplers
};

/****************************************************************************/

class sGeometryPrivate 
{
protected:
  sGeometryPrivate();
  ~sGeometryPrivate();
  sInt VAlloc;
  sInt VUsed;
  sInt VUsedElements;
  sU8 *VPtr;
  sInt IAlloc;
  sInt IUsed;
  sInt IUsedElements;
  sU8 *IPtr;
};

/****************************************************************************/

enum sVertexFormatHandlePrivateEnum
{
  sVF_MAXATTRIB = 16,
};

struct sVertexFormatHandlePrivate
{
  friend class sGeometry;
  friend class sMaterial;
  struct AttrData
  {
    sInt Offset;
    sInt Normalized;
    sInt Type;
    sInt Size;
    const char *Name;
  } Attr[sVF_MAXATTRIB];
  sInt AttrCount;
};

/****************************************************************************/

class sTextureBasePrivate 
{
  friend class sMaterial;
protected:
  sTextureBasePrivate();
  ~sTextureBasePrivate();
  sU8 *LoadBuffer;
  sInt GLName;
  sInt GLIFormat;
  sInt GLFormat;
  sInt GLType;
  sInt LoadMipmap;
};

/****************************************************************************/

class sShaderPrivate 
{
};

/****************************************************************************/

class sMaterialPrivate 
{
protected:
  sMaterialPrivate();
  ~sMaterialPrivate();
  
  sInt GLName;
  sInt VSSlot[4];
  
  sInt GLTexMin[sMTRL_MAXTEX];
  sInt GLTexMax[sMTRL_MAXTEX];
  sInt GLTexS[sMTRL_MAXTEX];
  sInt GLTexT[sMTRL_MAXTEX];
  
  sInt GLBCSrc,GLBCDst,GLBCFunc;
  sInt GLBASrc,GLBADst,GLBAFunc;
  sVector4 GLBlendFactor;
};

/****************************************************************************/

class sOccQueryPrivate 
{
};

/****************************************************************************/

class sCBufferBasePrivate 
{
public:
  void *DataPersist;
  void **DataPtr;
  sU64 Mask;
};

/****************************************************************************/

class sGfxThreadContextPrivate
{
};

/****************************************************************************/

class sGpuToCpuPrivate
{
};

/****************************************************************************/

#endif // FILE_BASE_GRAPHICS_OGLES2_PRIVATE_HPP

