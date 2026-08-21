/*+**************************************************************************/
/***                                                                      ***/
/***   Cg/HLSL -> GLSL 1.20 translator for the macOS OpenGL 2.1 backend   ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef HEADER_ALTONA_BASE_SHADER_GLSL_MACOS
#define HEADER_ALTONA_BASE_SHADER_GLSL_MACOS

#include <stdarg.h>

struct sGlslBuf
{
  char *Ptr;
  int Len;
  int Cap;

  void Init();
  void Exit();
  void Add(const char *s);
  void AddN(const char *s,int n);
  void AddF(const char *fmt,...);
};

struct sGlslShaderInfo
{
  sGlslBuf Source;                      // translated GLSL

  int AttrCount;
  char AttrName[16][64];                // generic attribute name in the source
  int AttrIndex[16];                    // location it must be bound to

  int SamplerCount;
  char SamplerName[16][64];
  int SamplerReg[16];                   // texture stage the sampler reads

  int RegCount;                         // size of the constant register array
};

// Returns 1 and fills info on success. On failure info holds nothing and the
// caller should treat the shader as unavailable.
int sGlslTranslateShader(const char *src,int isvertex,sGlslShaderInfo *info);
void sGlslFreeShaderInfo(sGlslShaderInfo *info);

#endif
