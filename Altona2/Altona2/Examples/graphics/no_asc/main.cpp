/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sRunApp(new App,sScreenMode(0,"no asc",1280,720));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
  Mtrl = 0;
  Tex = 0;
  Geo = 0;
}

App::~App()
{
}

void App::OnInit()
{
  Screen = sGetScreen();
  Adapter = Screen->Adapter;
  Context = Adapter->ImmediateContext;

#if sConfigRender == sConfigRenderDX11 || sConfigRender == sConfigRenderNull
  static const sChar vspro[] = "vs_4_0";
  static const sChar vstxt[] = 
    "row_major float4x4 model2screen : register(c0);"
    "float3 ldir : register(c4);"
    "void main"
    "("
    "  in float3 ipos : POSITION,"
    "  in float3 inorm : NORMAL,"
    "  in float2 iuv : TEXCOORD,"
    "  out float4 ocol : COLOR,"
    "  out float2 ouv : TEXCOORD,"
    "  out float4 opos : SV_Position"
    ")"
    "{"
    "  opos = mul(model2screen,float4(ipos,1));"
    "  ocol = saturate(dot(ldir,inorm));"
    "  ouv = iuv;"
    "}";
  static const sChar pspro[] = "ps_4_0";
  static const sChar pstxt[] =
    "Texture2D tex0:register(t1);"
    "SamplerState sam0:register(s1);"
    "void main"
    "("
    "  in float4 icol : COLOR,"
    "  in float2 iuv : TEXCOORD,"
    "  out float4 ocol : SV_Target"
    ")"
    "{"
    "  ocol = icol * tex0.Sample(sam0,iuv);"
    "}";
#endif
#if sConfigRender==sConfigRenderDX9
  static const sChar vspro[] = "vs_3_0";
  static const sChar vstxt[] = 
    "row_major float4x4 MS2SS : register(c0);"
    "float3 ldir : register(c4);"
    "void main"
    "("
    "  in float3 ipos : POSITION,"
    "  in float3 inorm : NORMAL,"
    "  inout float2 uv : TEXCOORD,"
    "  out float4 ocol : COLOR,"
    "  out float4 opos : POSITION"
    ")"
    "{"
    "  opos = mul(MS2SS,float4(ipos,1));"
    "  ocol = saturate(dot(ldir,inorm));"
    "}";
  static const sChar pspro[] = "ps_3_0";
  static const sChar pstxt[] =
    "SamplerState sam0:register(s1);"
    "void main"
    "("
    "  in float4 icol : COLOR,"
    "  in float2 iuv : TEXCOORD,"
    "  out float4 ocol : COLOR0"
    ")"
    "{"
    "  ocol = icol * tex2D(sam0,iuv);"
    "}";
#endif
#if sConfigRender == sConfigRenderGL2
  static const sChar vspro[] = "120";
  static const sChar vstxt[] = 
    "#version 120\n"
    "attribute vec3 norm1;"
    "attribute vec3 pos0;"
    "attribute vec2 uv2;"
    "uniform vec4 vc0[5];\n"
    "varying vec4 COLOR;\n"
    "varying vec2 TEXCOORD0;\n"
    "void main()\n"
    "{"
    "  gl_Position = mat4x4(vec4(vc0[0].x,vc0[1].x,vc0[2].x,vc0[3].x),vec4(vc0[0].y,vc0[1].y,vc0[2].y,vc0[3].y),vec4(vc0[0].z,vc0[1].z,vc0[2].z,vc0[3].z),vec4(vc0[0].w,vc0[1].w,vc0[2].w,vc0[3].w)) * vec4(pos0.xyz,1);"
    "  COLOR = vec4(max(dot(vc0[4].xyz,norm1),0.0));"
    "  TEXCOORD0 = uv2;"
    "}";
  static const sChar pspro[] = "120";
  static const sChar pstxt[] =
    "#version 120\n"
    "varying vec4 COLOR;" 
    "varying vec2 TEXCOORD0;\n"
    "uniform sampler2D tex1;\n"
    "void main()"
    "{"
    "  gl_FragColor = COLOR * texture2D(tex1,TEXCOORD0);\n"
    "}";  
#endif
#if sConfigRender == sConfigRenderGLES2
  static const sChar vspro[] = "100";
  static const sChar vstxt[] = 
    "#version 100\n"
    "attribute vec3 norm1;"
    "attribute vec3 pos0;"
    "attribute vec2 uv2;"
    "uniform vec4 vc0[5];\n"
    "varying vec4 COLOR;\n"
    "varying vec2 TEXCOORD0;\n"
    "void main()\n"
    "{"
    "  gl_Position = mat4(vec4(vc0[0].x,vc0[1].x,vc0[2].x,vc0[3].x),vec4(vc0[0].y,vc0[1].y,vc0[2].y,vc0[3].y),vec4(vc0[0].z,vc0[1].z,vc0[2].z,vc0[3].z),vec4(vc0[0].w,vc0[1].w,vc0[2].w,vc0[3].w)) * vec4(pos0.xyz,1);"
    "  COLOR = vec4(max(dot(vc0[4].xyz,norm1),0.0));"
    "  TEXCOORD0 = uv2;"
    "}";
  static const sChar pspro[] = "100";
  static const sChar pstxt[] =
    "#version 100\n"
    "varying lowp vec4 COLOR;" 
    "varying mediump vec2 TEXCOORD0;\n"
    "uniform sampler2D tex1;\n"
    "void main()"
    "{"
    "  gl_FragColor = COLOR * texture2D(tex1,TEXCOORD0);\n"
    "}";  
#endif
  static const sU32 vfdesc[] =
  {
    sVF_Position,
    sVF_Normal,
    sVF_UV0,
    sVF_End,
  };

  sShaderBinary *bin;

  bin = sCompileShader(sST_Vertex,vspro,vstxt,0);
  sASSERT(bin);
  VS = Adapter->CreateShader(bin);
  delete bin;

  bin = sCompileShader(sST_Pixel,pspro,pstxt,0);
  sASSERT(bin);
  PS = Adapter->CreateShader(bin);
  delete bin;

  Format = Adapter->CreateVertexFormat(vfdesc);

  const sInt sx=64;
  const sInt sy=64;
  sU32 tex[sy][sx];
  for(sInt y=0;y<sy;y++)
    for(sInt x=0;x<sx;x++)
      tex[y][x] = ((x^y)&8) ? 0xffff8080 : 0xff80ff80;
  Tex = new sResource(Adapter,sResTexPara(sRF_BGRA8888,sx,sy,1),&tex[0][0],sizeof(tex));


  Mtrl = new sMaterial(Adapter);
  Mtrl->SetShader(VS,sST_Vertex);
  Mtrl->SetShader(PS,sST_Pixel);
  Mtrl->SetTexturePS(1,Tex,sSamplerStatePara(0,0));
  Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
  Mtrl->Prepare(Format);


  const sInt ic = 6*6;
  const sInt vc = 24;
  static const sU16 ib[ic] =
  {
     0, 1, 2, 0, 2, 3,
     4, 5, 6, 4, 6, 7,
     8, 9,10, 8,10,11,

    12,13,14,12,14,15,
    16,17,18,16,18,19,
    20,21,22,20,22,23,
  };
  struct vert
  {
    sF32 px,py,pz;
    sF32 nx,ny,nz;
    sF32 u0,v0;
  };
  static const vert vb[vc] =
  {
    { -1, 1,-1,   0, 0,-1, 0,0, },
    {  1, 1,-1,   0, 0,-1, 1,0, },
    {  1,-1,-1,   0, 0,-1, 1,1, },
    { -1,-1,-1,   0, 0,-1, 0,1, },

    { -1,-1, 1,   0, 0, 1, 0,0, },
    {  1,-1, 1,   0, 0, 1, 1,0, },
    {  1, 1, 1,   0, 0, 1, 1,1, },
    { -1, 1, 1,   0, 0, 1, 0,1, },

    { -1,-1,-1,   0,-1, 0, 0,0, },
    {  1,-1,-1,   0,-1, 0, 1,0, },
    {  1,-1, 1,   0,-1, 0, 1,1, },
    { -1,-1, 1,   0,-1, 0, 0,1, },

    {  1,-1,-1,   1, 0, 0, 0,0, },
    {  1, 1,-1,   1, 0, 0, 1,0, },
    {  1, 1, 1,   1, 0, 0, 1,1, },
    {  1,-1, 1,   1, 0, 0, 0,1, },

    {  1, 1,-1,   0, 1, 0, 0,0, },
    { -1, 1,-1,   0, 1, 0, 1,0, },
    { -1, 1, 1,   0, 1, 0, 1,1, },
    {  1, 1, 1,   0, 1, 0, 0,1, },

    { -1, 1,-1,  -1, 0, 0, 0,0, },
    { -1,-1,-1,  -1, 0, 0, 1,0, },
    { -1,-1, 1,  -1, 0, 0, 1,1, },
    { -1, 1, 1,  -1, 0, 0, 0,1, },
  };

  Geo = new sGeometry(Adapter);
  Geo->SetIndex(sResBufferPara(sRBM_Index|sRU_Static,sizeof(sU16),ic),ib);
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(vert),vc),vb);
  Geo->Prepare(Format,sGMP_Tris|sGMF_Index16);


  cbv0 = new sCBuffer<cbuffer>(Adapter,sST_Vertex,0);
}

void App::OnExit()
{
  delete Mtrl;
  delete Tex;
  delete Geo;
  delete cbv0;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
  sF32 time = sGetTimeMS()*0.01f;
  sTargetPara tp(sTAR_ClearAll,0xff405060,Screen);
  sViewport view;


  Context->BeginTarget(tp);

  view.Camera.k.w = -4;
  view.Model = sEulerXYZ(time*0.11f,time*0.13f,time*0.15f);
  view.ZoomX = 1/tp.Aspect;
  view.ZoomY = 1;
  view.Prepare(tp);

  cbv0->Map();
  cbv0->Data->MS2SS = view.MS2SS;
  cbv0->Data->ldir.Set(-view.MS2CS.k.x,-view.MS2CS.k.y,-view.MS2CS.k.z);
  cbv0->Unmap();

  Context->Draw(sDrawPara(Geo,Mtrl,cbv0));

  Context->EndTarget();
}

void App::OnKey(const sKeyData &kd)
{
  if((kd.Key&(sKEYQ_Mask|sKEYQ_Break))==27)
    sExit();
}

void App::OnDrag(const sDragData &dd)
{
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

