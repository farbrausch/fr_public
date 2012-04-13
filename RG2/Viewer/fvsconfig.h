// This code is in the public domain. See LICENSE for details.

// fvs² configuration. fit to your needs.

#ifndef __fvs_fvsconfig_h_
#define __fvs_fvsconfig_h_

#undef  FVS_SUPPORT_CUBEMAPS
#ifndef _DEBUG
#undef  FVS_SUPPORT_WINDOWED
#else
#define FVS_SUPPORT_WINDOWED
#endif
#undef  FVS_SUPPORT_GAMMA
#undef  FVS_SUPPORT_ANIM_TEXTURES
#undef  FVS_SUPPORT_RESOURCE_MANAGEMENT
#undef  FVS_SUPPORT_RENDER_TO_TEXTURE
#define FVS_TINY_ERROR_CHECKING
#define FVS_TINY_TEXTURE_CODE
#define FVS_RGBA_TEXTURES_ONLY
#define FVS_ONLY_32B_VERTEX_FORMATS
#define FVS_MINIMAL_CRT

#endif
