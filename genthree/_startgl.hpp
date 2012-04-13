// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __STARTGL_HPP__
#define __STARTGL_HPP__

#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/

// defines flags for sSystem::MatState() when in OpenGL Mode    
// comments are examples for the values
// high byte is number of values, including sGLCMD_???
// low byte is internal code

#define sGLCMD_END           0x0100         // no value
#define sGLCMD_ENABLE        0x0201         // sGL_TEXTURE2D
#define sGLCMD_DISABLE       0x0202         // sGL_TEXTURE2D
#define sGLCMD_TEXPARA2D     0x0303         // sGL_TEXTURE_MIN_FILTER , sGL_NEAREST
#define sGLCMD_BLENDFUNC     0x0304         // sGL_ONE , sGL_ONE
#define sGLCMD_TEXENV        0x0305         // sGL_TEXTURE_ENV_MODE , sGL_REPLACE
#define sGLCMD_DEPTHMASK     0x0206         // 0

/****************************************************************************/


#define sGL_ACCUM                          0x0100
#define sGL_LOAD                           0x0101
#define sGL_RETURN                         0x0102
#define sGL_MULT                           0x0103
#define sGL_ADD                            0x0104

/* AlphaFunction */
#define sGL_NEVER                          0x0200
#define sGL_LESS                           0x0201
#define sGL_EQUAL                          0x0202
#define sGL_LEQUAL                         0x0203
#define sGL_GREATER                        0x0204
#define sGL_NOTEQUAL                       0x0205
#define sGL_GEQUAL                         0x0206
#define sGL_ALWAYS                         0x0207

/* AttribMask */
#define sGL_CURRENT_BIT                    0x00000001
#define sGL_POINT_BIT                      0x00000002
#define sGL_LINE_BIT                       0x00000004
#define sGL_POLYGON_BIT                    0x00000008
#define sGL_POLYGON_STIPPLE_BIT            0x00000010
#define sGL_PIXEL_MODE_BIT                 0x00000020
#define sGL_LIGHTING_BIT                   0x00000040
#define sGL_FOG_BIT                        0x00000080
#define sGL_DEPTH_BUFFER_BIT               0x00000100
#define sGL_ACCUM_BUFFER_BIT               0x00000200
#define sGL_STENCIL_BUFFER_BIT             0x00000400
#define sGL_VIEWPORT_BIT                   0x00000800
#define sGL_TRANSFORM_BIT                  0x00001000
#define sGL_ENABLE_BIT                     0x00002000
#define sGL_COLOR_BUFFER_BIT               0x00004000
#define sGL_HINT_BIT                       0x00008000
#define sGL_EVAL_BIT                       0x00010000
#define sGL_LIST_BIT                       0x00020000
#define sGL_TEXTURE_BIT                    0x00040000
#define sGL_SCISSOR_BIT                    0x00080000
#define sGL_ALL_ATTRIB_BITS                0x000fffff

/* BeginMode */
#define sGL_POINTS                         0x0000
#define sGL_LINES                          0x0001
#define sGL_LINE_LOOP                      0x0002
#define sGL_LINE_STRIP                     0x0003
#define sGL_TRIANGLES                      0x0004
#define sGL_TRIANGLE_STRIP                 0x0005
#define sGL_TRIANGLE_FAN                   0x0006
#define sGL_QUADS                          0x0007
#define sGL_QUAD_STRIP                     0x0008
#define sGL_POLYGON                        0x0009

/* BlendingFactorDest */
#define sGL_ZERO                           0
#define sGL_ONE                            1
#define sGL_SRC_COLOR                      0x0300
#define sGL_ONE_MINUS_SRC_COLOR            0x0301
#define sGL_SRC_ALPHA                      0x0302
#define sGL_ONE_MINUS_SRC_ALPHA            0x0303
#define sGL_DST_ALPHA                      0x0304
#define sGL_ONE_MINUS_DST_ALPHA            0x0305

/* BlendingFactorSrc */
/*      sGL_ZERO */
/*      sGL_ONE */
#define sGL_DST_COLOR                      0x0306
#define sGL_ONE_MINUS_DST_COLOR            0x0307
#define sGL_SRC_ALPHA_SATURATE             0x0308
/*      sGL_SRC_ALPHA */
/*      sGL_ONE_MINUS_SRC_ALPHA */
/*      sGL_DST_ALPHA */
/*      sGL_ONE_MINUS_DST_ALPHA */

/* Boolean */
#define sGL_TRUE                           1
#define sGL_FALSE                          0

/* ClearBufferMask */
/*      sGL_COLOR_BUFFER_BIT */
/*      sGL_ACCUM_BUFFER_BIT */
/*      sGL_STENCIL_BUFFER_BIT */
/*      sGL_DEPTH_BUFFER_BIT */

/* ClientArrayType */
/*      sGL_VERTEX_ARRAY */
/*      sGL_NORMAL_ARRAY */
/*      sGL_COLOR_ARRAY */
/*      sGL_INDEX_ARRAY */
/*      sGL_TEXTURE_COORD_ARRAY */
/*      sGL_EDGE_FLAG_ARRAY */

/* ClipPlaneName */
#define sGL_CLIP_PLANE0                    0x3000
#define sGL_CLIP_PLANE1                    0x3001
#define sGL_CLIP_PLANE2                    0x3002
#define sGL_CLIP_PLANE3                    0x3003
#define sGL_CLIP_PLANE4                    0x3004
#define sGL_CLIP_PLANE5                    0x3005

/* ColorMaterialFace */
/*      sGL_FRONT */
/*      sGL_BACK */
/*      sGL_FRONT_AND_BACK */

/* ColorMaterialParameter */
/*      sGL_AMBIENT */
/*      sGL_DIFFUSE */
/*      sGL_SPECULAR */
/*      sGL_EMISSION */
/*      sGL_AMBIENT_AND_DIFFUSE */

/* ColorPointerType */
/*      sGL_BYTE */
/*      sGL_UNSIGNED_BYTE */
/*      sGL_SHORT */
/*      sGL_UNSIGNED_SHORT */
/*      sGL_INT */
/*      sGL_UNSIGNED_INT */
/*      sGL_FLOAT */
/*      sGL_DOUBLE */

/* CullFaceMode */
/*      sGL_FRONT */
/*      sGL_BACK */
/*      sGL_FRONT_AND_BACK */

/* DataType */
#define sGL_BYTE                           0x1400
#define sGL_UNSIGNED_BYTE                  0x1401
#define sGL_SHORT                          0x1402
#define sGL_UNSIGNED_SHORT                 0x1403
#define sGL_INT                            0x1404
#define sGL_UNSIGNED_INT                   0x1405
#define sGL_FLOAT                          0x1406
#define sGL_2_BYTES                        0x1407
#define sGL_3_BYTES                        0x1408
#define sGL_4_BYTES                        0x1409
#define sGL_DOUBLE                         0x140A

/* DepthFunction */
/*      sGL_NEVER */
/*      sGL_LESS */
/*      sGL_EQUAL */
/*      sGL_LEQUAL */
/*      sGL_GREATER */
/*      sGL_NOTEQUAL */
/*      sGL_GEQUAL */
/*      sGL_ALWAYS */

/* DrawBufferMode */
#define sGL_NONE                           0
#define sGL_FRONT_LEFT                     0x0400
#define sGL_FRONT_RIGHT                    0x0401
#define sGL_BACK_LEFT                      0x0402
#define sGL_BACK_RIGHT                     0x0403
#define sGL_FRONT                          0x0404
#define sGL_BACK                           0x0405
#define sGL_LEFT                           0x0406
#define sGL_RIGHT                          0x0407
#define sGL_FRONT_AND_BACK                 0x0408
#define sGL_AUX0                           0x0409
#define sGL_AUX1                           0x040A
#define sGL_AUX2                           0x040B
#define sGL_AUX3                           0x040C

/* Enable */
/*      sGL_FOG */
/*      sGL_LIGHTING */
/*      sGL_TEXTURE_1D */
/*      sGL_TEXTURE_2D */
/*      sGL_LINE_STIPPLE */
/*      sGL_POLYGON_STIPPLE */
/*      sGL_CULL_FACE */
/*      sGL_ALPHA_TEST */
/*      sGL_BLEND */
/*      sGL_INDEX_LOGIC_OP */
/*      sGL_COLOR_LOGIC_OP */
/*      sGL_DITHER */
/*      sGL_STENCIL_TEST */
/*      sGL_DEPTH_TEST */
/*      sGL_CLIP_PLANE0 */
/*      sGL_CLIP_PLANE1 */
/*      sGL_CLIP_PLANE2 */
/*      sGL_CLIP_PLANE3 */
/*      sGL_CLIP_PLANE4 */
/*      sGL_CLIP_PLANE5 */
/*      sGL_LIGHT0 */
/*      sGL_LIGHT1 */
/*      sGL_LIGHT2 */
/*      sGL_LIGHT3 */
/*      sGL_LIGHT4 */
/*      sGL_LIGHT5 */
/*      sGL_LIGHT6 */
/*      sGL_LIGHT7 */
/*      sGL_TEXTURE_GEN_S */
/*      sGL_TEXTURE_GEN_T */
/*      sGL_TEXTURE_GEN_R */
/*      sGL_TEXTURE_GEN_Q */
/*      sGL_MAP1_VERTEX_3 */
/*      sGL_MAP1_VERTEX_4 */
/*      sGL_MAP1_COLOR_4 */
/*      sGL_MAP1_INDEX */
/*      sGL_MAP1_NORMAL */
/*      sGL_MAP1_TEXTURE_COORD_1 */
/*      sGL_MAP1_TEXTURE_COORD_2 */
/*      sGL_MAP1_TEXTURE_COORD_3 */
/*      sGL_MAP1_TEXTURE_COORD_4 */
/*      sGL_MAP2_VERTEX_3 */
/*      sGL_MAP2_VERTEX_4 */
/*      sGL_MAP2_COLOR_4 */
/*      sGL_MAP2_INDEX */
/*      sGL_MAP2_NORMAL */
/*      sGL_MAP2_TEXTURE_COORD_1 */
/*      sGL_MAP2_TEXTURE_COORD_2 */
/*      sGL_MAP2_TEXTURE_COORD_3 */
/*      sGL_MAP2_TEXTURE_COORD_4 */
/*      sGL_POINT_SMOOTH */
/*      sGL_LINE_SMOOTH */
/*      sGL_POLYGON_SMOOTH */
/*      sGL_SCISSOR_TEST */
/*      sGL_COLOR_MATERIAL */
/*      sGL_NORMALIZE */
/*      sGL_AUTO_NORMAL */
/*      sGL_VERTEX_ARRAY */
/*      sGL_NORMAL_ARRAY */
/*      sGL_COLOR_ARRAY */
/*      sGL_INDEX_ARRAY */
/*      sGL_TEXTURE_COORD_ARRAY */
/*      sGL_EDGE_FLAG_ARRAY */
/*      sGL_POLYGON_OFFSET_POINT */
/*      sGL_POLYGON_OFFSET_LINE */
/*      sGL_POLYGON_OFFSET_FILL */

/* ErrorCode */
#define sGL_NO_ERROR                       0
#define sGL_INVALID_ENUM                   0x0500
#define sGL_INVALID_VALUE                  0x0501
#define sGL_INVALID_OPERATION              0x0502
#define sGL_STACK_OVERFLOW                 0x0503
#define sGL_STACK_UNDERFLOW                0x0504
#define sGL_OUT_OF_MEMORY                  0x0505

/* FeedBackMode */
#define sGL_2D                             0x0600
#define sGL_3D                             0x0601
#define sGL_3D_COLOR                       0x0602
#define sGL_3D_COLOR_TEXTURE               0x0603
#define sGL_4D_COLOR_TEXTURE               0x0604

/* FeedBackToken */
#define sGL_PASS_THROUGH_TOKEN             0x0700
#define sGL_POINT_TOKEN                    0x0701
#define sGL_LINE_TOKEN                     0x0702
#define sGL_POLYGON_TOKEN                  0x0703
#define sGL_BITMAP_TOKEN                   0x0704
#define sGL_DRAW_PIXEL_TOKEN               0x0705
#define sGL_COPY_PIXEL_TOKEN               0x0706
#define sGL_LINE_RESET_TOKEN               0x0707

/* FogMode */
/*      sGL_LINEAR */
#define sGL_EXP                            0x0800
#define sGL_EXP2                           0x0801


/* FogParameter */
/*      sGL_FOG_COLOR */
/*      sGL_FOG_DENSITY */
/*      sGL_FOG_END */
/*      sGL_FOG_INDEX */
/*      sGL_FOG_MODE */
/*      sGL_FOG_START */

/* FrontFaceDirection */
#define sGL_CW                             0x0900
#define sGL_CCW                            0x0901

/* GetMapTarget */
#define sGL_COEFF                          0x0A00
#define sGL_ORDER                          0x0A01
#define sGL_DOMAIN                         0x0A02

/* GetPixelMap */
/*      sGL_PIXEL_MAP_I_TO_I */
/*      sGL_PIXEL_MAP_S_TO_S */
/*      sGL_PIXEL_MAP_I_TO_R */
/*      sGL_PIXEL_MAP_I_TO_G */
/*      sGL_PIXEL_MAP_I_TO_B */
/*      sGL_PIXEL_MAP_I_TO_A */
/*      sGL_PIXEL_MAP_R_TO_R */
/*      sGL_PIXEL_MAP_G_TO_G */
/*      sGL_PIXEL_MAP_B_TO_B */
/*      sGL_PIXEL_MAP_A_TO_A */

/* GetPointerTarget */
/*      sGL_VERTEX_ARRAY_POINTER */
/*      sGL_NORMAL_ARRAY_POINTER */
/*      sGL_COLOR_ARRAY_POINTER */
/*      sGL_INDEX_ARRAY_POINTER */
/*      sGL_TEXTURE_COORD_ARRAY_POINTER */
/*      sGL_EDGE_FLAG_ARRAY_POINTER */

/* GetTarget */
#define sGL_CURRENT_COLOR                  0x0B00
#define sGL_CURRENT_INDEX                  0x0B01
#define sGL_CURRENT_NORMAL                 0x0B02
#define sGL_CURRENT_TEXTURE_COORDS         0x0B03
#define sGL_CURRENT_RASTER_COLOR           0x0B04
#define sGL_CURRENT_RASTER_INDEX           0x0B05
#define sGL_CURRENT_RASTER_TEXTURE_COORDS  0x0B06
#define sGL_CURRENT_RASTER_POSITION        0x0B07
#define sGL_CURRENT_RASTER_POSITION_VALID  0x0B08
#define sGL_CURRENT_RASTER_DISTANCE        0x0B09
#define sGL_POINT_SMOOTH                   0x0B10
#define sGL_POINT_SIZE                     0x0B11
#define sGL_POINT_SIZE_RANGE               0x0B12
#define sGL_POINT_SIZE_GRANULARITY         0x0B13
#define sGL_LINE_SMOOTH                    0x0B20
#define sGL_LINE_WIDTH                     0x0B21
#define sGL_LINE_WIDTH_RANGE               0x0B22
#define sGL_LINE_WIDTH_GRANULARITY         0x0B23
#define sGL_LINE_STIPPLE                   0x0B24
#define sGL_LINE_STIPPLE_PATTERN           0x0B25
#define sGL_LINE_STIPPLE_REPEAT            0x0B26
#define sGL_LIST_MODE                      0x0B30
#define sGL_MAX_LIST_NESTING               0x0B31
#define sGL_LIST_BASE                      0x0B32
#define sGL_LIST_INDEX                     0x0B33
#define sGL_POLYGON_MODE                   0x0B40
#define sGL_POLYGON_SMOOTH                 0x0B41
#define sGL_POLYGON_STIPPLE                0x0B42
#define sGL_EDGE_FLAG                      0x0B43
#define sGL_CULL_FACE                      0x0B44
#define sGL_CULL_FACE_MODE                 0x0B45
#define sGL_FRONT_FACE                     0x0B46
#define sGL_LIGHTING                       0x0B50
#define sGL_LIGHT_MODEL_LOCAL_VIEWER       0x0B51
#define sGL_LIGHT_MODEL_TWO_SIDE           0x0B52
#define sGL_LIGHT_MODEL_AMBIENT            0x0B53
#define sGL_SHADE_MODEL                    0x0B54
#define sGL_COLOR_MATERIAL_FACE            0x0B55
#define sGL_COLOR_MATERIAL_PARAMETER       0x0B56
#define sGL_COLOR_MATERIAL                 0x0B57
#define sGL_FOG                            0x0B60
#define sGL_FOG_INDEX                      0x0B61
#define sGL_FOG_DENSITY                    0x0B62
#define sGL_FOG_START                      0x0B63
#define sGL_FOG_END                        0x0B64
#define sGL_FOG_MODE                       0x0B65
#define sGL_FOG_COLOR                      0x0B66
#define sGL_DEPTH_RANGE                    0x0B70
#define sGL_DEPTH_TEST                     0x0B71
#define sGL_DEPTH_WRITEMASK                0x0B72
#define sGL_DEPTH_CLEAR_VALUE              0x0B73
#define sGL_DEPTH_FUNC                     0x0B74
#define sGL_ACCUM_CLEAR_VALUE              0x0B80
#define sGL_STENCIL_TEST                   0x0B90
#define sGL_STENCIL_CLEAR_VALUE            0x0B91
#define sGL_STENCIL_FUNC                   0x0B92
#define sGL_STENCIL_VALUE_MASK             0x0B93
#define sGL_STENCIL_FAIL                   0x0B94
#define sGL_STENCIL_PASS_DEPTH_FAIL        0x0B95
#define sGL_STENCIL_PASS_DEPTH_PASS        0x0B96
#define sGL_STENCIL_REF                    0x0B97
#define sGL_STENCIL_WRITEMASK              0x0B98
#define sGL_MATRIX_MODE                    0x0BA0
#define sGL_NORMALIZE                      0x0BA1
#define sGL_VIEWPORT                       0x0BA2
#define sGL_MODELVIEW_STACK_DEPTH          0x0BA3
#define sGL_PROJECTION_STACK_DEPTH         0x0BA4
#define sGL_TEXTURE_STACK_DEPTH            0x0BA5
#define sGL_MODELVIEW_MATRIX               0x0BA6
#define sGL_PROJECTION_MATRIX              0x0BA7
#define sGL_TEXTURE_MATRIX                 0x0BA8
#define sGL_ATTRIB_STACK_DEPTH             0x0BB0
#define sGL_CLIENT_ATTRIB_STACK_DEPTH      0x0BB1
#define sGL_ALPHA_TEST                     0x0BC0
#define sGL_ALPHA_TEST_FUNC                0x0BC1
#define sGL_ALPHA_TEST_REF                 0x0BC2
#define sGL_DITHER                         0x0BD0
#define sGL_BLEND_DST                      0x0BE0
#define sGL_BLEND_SRC                      0x0BE1
#define sGL_BLEND                          0x0BE2
#define sGL_LOGIC_OP_MODE                  0x0BF0
#define sGL_INDEX_LOGIC_OP                 0x0BF1
#define sGL_COLOR_LOGIC_OP                 0x0BF2
#define sGL_AUX_BUFFERS                    0x0C00
#define sGL_DRAW_BUFFER                    0x0C01
#define sGL_READ_BUFFER                    0x0C02
#define sGL_SCISSOR_BOX                    0x0C10
#define sGL_SCISSOR_TEST                   0x0C11
#define sGL_INDEX_CLEAR_VALUE              0x0C20
#define sGL_INDEX_WRITEMASK                0x0C21
#define sGL_COLOR_CLEAR_VALUE              0x0C22
#define sGL_COLOR_WRITEMASK                0x0C23
#define sGL_INDEX_MODE                     0x0C30
#define sGL_RGBA_MODE                      0x0C31
#define sGL_DOUBLEBUFFER                   0x0C32
#define sGL_STEREO                         0x0C33
#define sGL_RENDER_MODE                    0x0C40
#define sGL_PERSPECTIVE_CORRECTION_HINT    0x0C50
#define sGL_POINT_SMOOTH_HINT              0x0C51
#define sGL_LINE_SMOOTH_HINT               0x0C52
#define sGL_POLYGON_SMOOTH_HINT            0x0C53
#define sGL_FOG_HINT                       0x0C54
#define sGL_TEXTURE_GEN_S                  0x0C60
#define sGL_TEXTURE_GEN_T                  0x0C61
#define sGL_TEXTURE_GEN_R                  0x0C62
#define sGL_TEXTURE_GEN_Q                  0x0C63
#define sGL_PIXEL_MAP_I_TO_I               0x0C70
#define sGL_PIXEL_MAP_S_TO_S               0x0C71
#define sGL_PIXEL_MAP_I_TO_R               0x0C72
#define sGL_PIXEL_MAP_I_TO_G               0x0C73
#define sGL_PIXEL_MAP_I_TO_B               0x0C74
#define sGL_PIXEL_MAP_I_TO_A               0x0C75
#define sGL_PIXEL_MAP_R_TO_R               0x0C76
#define sGL_PIXEL_MAP_G_TO_G               0x0C77
#define sGL_PIXEL_MAP_B_TO_B               0x0C78
#define sGL_PIXEL_MAP_A_TO_A               0x0C79
#define sGL_PIXEL_MAP_I_TO_I_SIZE          0x0CB0
#define sGL_PIXEL_MAP_S_TO_S_SIZE          0x0CB1
#define sGL_PIXEL_MAP_I_TO_R_SIZE          0x0CB2
#define sGL_PIXEL_MAP_I_TO_G_SIZE          0x0CB3
#define sGL_PIXEL_MAP_I_TO_B_SIZE          0x0CB4
#define sGL_PIXEL_MAP_I_TO_A_SIZE          0x0CB5
#define sGL_PIXEL_MAP_R_TO_R_SIZE          0x0CB6
#define sGL_PIXEL_MAP_G_TO_G_SIZE          0x0CB7
#define sGL_PIXEL_MAP_B_TO_B_SIZE          0x0CB8
#define sGL_PIXEL_MAP_A_TO_A_SIZE          0x0CB9
#define sGL_UNPACK_SWAP_BYTES              0x0CF0
#define sGL_UNPACK_LSB_FIRST               0x0CF1
#define sGL_UNPACK_ROW_LENGTH              0x0CF2
#define sGL_UNPACK_SKIP_ROWS               0x0CF3
#define sGL_UNPACK_SKIP_PIXELS             0x0CF4
#define sGL_UNPACK_ALIGNMENT               0x0CF5
#define sGL_PACK_SWAP_BYTES                0x0D00
#define sGL_PACK_LSB_FIRST                 0x0D01
#define sGL_PACK_ROW_LENGTH                0x0D02
#define sGL_PACK_SKIP_ROWS                 0x0D03
#define sGL_PACK_SKIP_PIXELS               0x0D04
#define sGL_PACK_ALIGNMENT                 0x0D05
#define sGL_MAP_COLOR                      0x0D10
#define sGL_MAP_STENCIL                    0x0D11
#define sGL_INDEX_SHIFT                    0x0D12
#define sGL_INDEX_OFFSET                   0x0D13
#define sGL_RED_SCALE                      0x0D14
#define sGL_RED_BIAS                       0x0D15
#define sGL_ZOOM_X                         0x0D16
#define sGL_ZOOM_Y                         0x0D17
#define sGL_GREEN_SCALE                    0x0D18
#define sGL_GREEN_BIAS                     0x0D19
#define sGL_BLUE_SCALE                     0x0D1A
#define sGL_BLUE_BIAS                      0x0D1B
#define sGL_ALPHA_SCALE                    0x0D1C
#define sGL_ALPHA_BIAS                     0x0D1D
#define sGL_DEPTH_SCALE                    0x0D1E
#define sGL_DEPTH_BIAS                     0x0D1F
#define sGL_MAX_EVAL_ORDER                 0x0D30
#define sGL_MAX_LIGHTS                     0x0D31
#define sGL_MAX_CLIP_PLANES                0x0D32
#define sGL_MAX_TEXTURE_SIZE               0x0D33
#define sGL_MAX_PIXEL_MAP_TABLE            0x0D34
#define sGL_MAX_ATTRIB_STACK_DEPTH         0x0D35
#define sGL_MAX_MODELVIEW_STACK_DEPTH      0x0D36
#define sGL_MAX_NAME_STACK_DEPTH           0x0D37
#define sGL_MAX_PROJECTION_STACK_DEPTH     0x0D38
#define sGL_MAX_TEXTURE_STACK_DEPTH        0x0D39
#define sGL_MAX_VIEWPORT_DIMS              0x0D3A
#define sGL_MAX_CLIENT_ATTRIB_STACK_DEPTH  0x0D3B
#define sGL_SUBPIXEL_BITS                  0x0D50
#define sGL_INDEX_BITS                     0x0D51
#define sGL_RED_BITS                       0x0D52
#define sGL_GREEN_BITS                     0x0D53
#define sGL_BLUE_BITS                      0x0D54
#define sGL_ALPHA_BITS                     0x0D55
#define sGL_DEPTH_BITS                     0x0D56
#define sGL_STENCIL_BITS                   0x0D57
#define sGL_ACCUM_RED_BITS                 0x0D58
#define sGL_ACCUM_GREEN_BITS               0x0D59
#define sGL_ACCUM_BLUE_BITS                0x0D5A
#define sGL_ACCUM_ALPHA_BITS               0x0D5B
#define sGL_NAME_STACK_DEPTH               0x0D70
#define sGL_AUTO_NORMAL                    0x0D80
#define sGL_MAP1_COLOR_4                   0x0D90
#define sGL_MAP1_INDEX                     0x0D91
#define sGL_MAP1_NORMAL                    0x0D92
#define sGL_MAP1_TEXTURE_COORD_1           0x0D93
#define sGL_MAP1_TEXTURE_COORD_2           0x0D94
#define sGL_MAP1_TEXTURE_COORD_3           0x0D95
#define sGL_MAP1_TEXTURE_COORD_4           0x0D96
#define sGL_MAP1_VERTEX_3                  0x0D97
#define sGL_MAP1_VERTEX_4                  0x0D98
#define sGL_MAP2_COLOR_4                   0x0DB0
#define sGL_MAP2_INDEX                     0x0DB1
#define sGL_MAP2_NORMAL                    0x0DB2
#define sGL_MAP2_TEXTURE_COORD_1           0x0DB3
#define sGL_MAP2_TEXTURE_COORD_2           0x0DB4
#define sGL_MAP2_TEXTURE_COORD_3           0x0DB5
#define sGL_MAP2_TEXTURE_COORD_4           0x0DB6
#define sGL_MAP2_VERTEX_3                  0x0DB7
#define sGL_MAP2_VERTEX_4                  0x0DB8
#define sGL_MAP1_GRID_DOMAIN               0x0DD0
#define sGL_MAP1_GRID_SEGMENTS             0x0DD1
#define sGL_MAP2_GRID_DOMAIN               0x0DD2
#define sGL_MAP2_GRID_SEGMENTS             0x0DD3
#define sGL_TEXTURE_1D                     0x0DE0
#define sGL_TEXTURE_2D                     0x0DE1
#define sGL_FEEDBACK_BUFFER_POINTER        0x0DF0
#define sGL_FEEDBACK_BUFFER_SIZE           0x0DF1
#define sGL_FEEDBACK_BUFFER_TYPE           0x0DF2
#define sGL_SELECTION_BUFFER_POINTER       0x0DF3
#define sGL_SELECTION_BUFFER_SIZE          0x0DF4
/*      sGL_TEXTURE_BINDING_1D */
/*      sGL_TEXTURE_BINDING_2D */
/*      sGL_VERTEX_ARRAY */
/*      sGL_NORMAL_ARRAY */
/*      sGL_COLOR_ARRAY */
/*      sGL_INDEX_ARRAY */
/*      sGL_TEXTURE_COORD_ARRAY */
/*      sGL_EDGE_FLAG_ARRAY */
/*      sGL_VERTEX_ARRAY_SIZE */
/*      sGL_VERTEX_ARRAY_TYPE */
/*      sGL_VERTEX_ARRAY_STRIDE */
/*      sGL_NORMAL_ARRAY_TYPE */
/*      sGL_NORMAL_ARRAY_STRIDE */
/*      sGL_COLOR_ARRAY_SIZE */
/*      sGL_COLOR_ARRAY_TYPE */
/*      sGL_COLOR_ARRAY_STRIDE */
/*      sGL_INDEX_ARRAY_TYPE */
/*      sGL_INDEX_ARRAY_STRIDE */
/*      sGL_TEXTURE_COORD_ARRAY_SIZE */
/*      sGL_TEXTURE_COORD_ARRAY_TYPE */
/*      sGL_TEXTURE_COORD_ARRAY_STRIDE */
/*      sGL_EDGE_FLAG_ARRAY_STRIDE */
/*      sGL_POLYGON_OFFSET_FACTOR */
/*      sGL_POLYGON_OFFSET_UNITS */

/* GetTextureParameter */
/*      sGL_TEXTURE_MAG_FILTER */
/*      sGL_TEXTURE_MIN_FILTER */
/*      sGL_TEXTURE_WRAP_S */
/*      sGL_TEXTURE_WRAP_T */
#define sGL_TEXTURE_WIDTH                  0x1000
#define sGL_TEXTURE_HEIGHT                 0x1001
#define sGL_TEXTURE_INTERNAL_FORMAT        0x1003
#define sGL_TEXTURE_BORDER_COLOR           0x1004
#define sGL_TEXTURE_BORDER                 0x1005
/*      sGL_TEXTURE_RED_SIZE */
/*      sGL_TEXTURE_GREEN_SIZE */
/*      sGL_TEXTURE_BLUE_SIZE */
/*      sGL_TEXTURE_ALPHA_SIZE */
/*      sGL_TEXTURE_LUMINANCE_SIZE */
/*      sGL_TEXTURE_INTENSITY_SIZE */
/*      sGL_TEXTURE_PRIORITY */
/*      sGL_TEXTURE_RESIDENT */

/* HintMode */
#define sGL_DONT_CARE                      0x1100
#define sGL_FASTEST                        0x1101
#define sGL_NICEST                         0x1102

/* HintTarget */
/*      sGL_PERSPECTIVE_CORRECTION_HINT */
/*      sGL_POINT_SMOOTH_HINT */
/*      sGL_LINE_SMOOTH_HINT */
/*      sGL_POLYGON_SMOOTH_HINT */
/*      sGL_FOG_HINT */
/*      sGL_PHONG_HINT */

/* IndexPointerType */
/*      sGL_SHORT */
/*      sGL_INT */
/*      sGL_FLOAT */
/*      sGL_DOUBLE */

/* LightModelParameter */
/*      sGL_LIGHT_MODEL_AMBIENT */
/*      sGL_LIGHT_MODEL_LOCAL_VIEWER */
/*      sGL_LIGHT_MODEL_TWO_SIDE */

/* LightName */
#define sGL_LIGHT0                         0x4000
#define sGL_LIGHT1                         0x4001
#define sGL_LIGHT2                         0x4002
#define sGL_LIGHT3                         0x4003
#define sGL_LIGHT4                         0x4004
#define sGL_LIGHT5                         0x4005
#define sGL_LIGHT6                         0x4006
#define sGL_LIGHT7                         0x4007

/* LightParameter */
#define sGL_AMBIENT                        0x1200
#define sGL_DIFFUSE                        0x1201
#define sGL_SPECULAR                       0x1202
#define sGL_POSITION                       0x1203
#define sGL_SPOT_DIRECTION                 0x1204
#define sGL_SPOT_EXPONENT                  0x1205
#define sGL_SPOT_CUTOFF                    0x1206
#define sGL_CONSTANT_ATTENUATION           0x1207
#define sGL_LINEAR_ATTENUATION             0x1208
#define sGL_QUADRATIC_ATTENUATION          0x1209

/* InterleavedArrays */
/*      sGL_V2F */
/*      sGL_V3F */
/*      sGL_C4UB_V2F */
/*      sGL_C4UB_V3F */
/*      sGL_C3F_V3F */
/*      sGL_N3F_V3F */
/*      sGL_C4F_N3F_V3F */
/*      sGL_T2F_V3F */
/*      sGL_T4F_V4F */
/*      sGL_T2F_C4UB_V3F */
/*      sGL_T2F_C3F_V3F */
/*      sGL_T2F_N3F_V3F */
/*      sGL_T2F_C4F_N3F_V3F */
/*      sGL_T4F_C4F_N3F_V4F */

/* ListMode */
#define sGL_COMPILE                        0x1300
#define sGL_COMPILE_AND_EXECUTE            0x1301

/* ListNameType */
/*      sGL_BYTE */
/*      sGL_UNSIGNED_BYTE */
/*      sGL_SHORT */
/*      sGL_UNSIGNED_SHORT */
/*      sGL_INT */
/*      sGL_UNSIGNED_INT */
/*      sGL_FLOAT */
/*      sGL_2_BYTES */
/*      sGL_3_BYTES */
/*      sGL_4_BYTES */

/* LogicOp */
#define sGL_CLEAR                          0x1500
#define sGL_AND                            0x1501
#define sGL_AND_REVERSE                    0x1502
#define sGL_COPY                           0x1503
#define sGL_AND_INVERTED                   0x1504
#define sGL_NOOP                           0x1505
#define sGL_XOR                            0x1506
#define sGL_OR                             0x1507
#define sGL_NOR                            0x1508
#define sGL_EQUIV                          0x1509
#define sGL_INVERT                         0x150A
#define sGL_OR_REVERSE                     0x150B
#define sGL_COPY_INVERTED                  0x150C
#define sGL_OR_INVERTED                    0x150D
#define sGL_NAND                           0x150E
#define sGL_SET                            0x150F

/* MapTarget */
/*      sGL_MAP1_COLOR_4 */
/*      sGL_MAP1_INDEX */
/*      sGL_MAP1_NORMAL */
/*      sGL_MAP1_TEXTURE_COORD_1 */
/*      sGL_MAP1_TEXTURE_COORD_2 */
/*      sGL_MAP1_TEXTURE_COORD_3 */
/*      sGL_MAP1_TEXTURE_COORD_4 */
/*      sGL_MAP1_VERTEX_3 */
/*      sGL_MAP1_VERTEX_4 */
/*      sGL_MAP2_COLOR_4 */
/*      sGL_MAP2_INDEX */
/*      sGL_MAP2_NORMAL */
/*      sGL_MAP2_TEXTURE_COORD_1 */
/*      sGL_MAP2_TEXTURE_COORD_2 */
/*      sGL_MAP2_TEXTURE_COORD_3 */
/*      sGL_MAP2_TEXTURE_COORD_4 */
/*      sGL_MAP2_VERTEX_3 */
/*      sGL_MAP2_VERTEX_4 */

/* MaterialFace */
/*      sGL_FRONT */
/*      sGL_BACK */
/*      sGL_FRONT_AND_BACK */

/* MaterialParameter */
#define sGL_EMISSION                       0x1600
#define sGL_SHININESS                      0x1601
#define sGL_AMBIENT_AND_DIFFUSE            0x1602
#define sGL_COLOR_INDEXES                  0x1603
/*      sGL_AMBIENT */
/*      sGL_DIFFUSE */
/*      sGL_SPECULAR */

/* MatrixMode */
#define sGL_MODELVIEW                      0x1700
#define sGL_PROJECTION                     0x1701
#define sGL_TEXTURE                        0x1702

/* MeshMode1 */
/*      sGL_POINT */
/*      sGL_LINE */

/* MeshMode2 */
/*      sGL_POINT */
/*      sGL_LINE */
/*      sGL_FILL */

/* NormalPointerType */
/*      sGL_BYTE */
/*      sGL_SHORT */
/*      sGL_INT */
/*      sGL_FLOAT */
/*      sGL_DOUBLE */

/* PixelCopyType */
#define sGL_COLOR                          0x1800
#define sGL_DEPTH                          0x1801
#define sGL_STENCIL                        0x1802

/* PixelFormat */
#define sGL_COLOR_INDEX                    0x1900
#define sGL_STENCIL_INDEX                  0x1901
#define sGL_DEPTH_COMPONENT                0x1902
#define sGL_RED                            0x1903
#define sGL_GREEN                          0x1904
#define sGL_BLUE                           0x1905
#define sGL_ALPHA                          0x1906
#define sGL_RGB                            0x1907
#define sGL_RGBA                           0x1908
#define sGL_LUMINANCE                      0x1909
#define sGL_LUMINANCE_ALPHA                0x190A

/* PixelMap */
/*      sGL_PIXEL_MAP_I_TO_I */
/*      sGL_PIXEL_MAP_S_TO_S */
/*      sGL_PIXEL_MAP_I_TO_R */
/*      sGL_PIXEL_MAP_I_TO_G */
/*      sGL_PIXEL_MAP_I_TO_B */
/*      sGL_PIXEL_MAP_I_TO_A */
/*      sGL_PIXEL_MAP_R_TO_R */
/*      sGL_PIXEL_MAP_G_TO_G */
/*      sGL_PIXEL_MAP_B_TO_B */
/*      sGL_PIXEL_MAP_A_TO_A */

/* PixelStore */
/*      sGL_UNPACK_SWAP_BYTES */
/*      sGL_UNPACK_LSB_FIRST */
/*      sGL_UNPACK_ROW_LENGTH */
/*      sGL_UNPACK_SKIP_ROWS */
/*      sGL_UNPACK_SKIP_PIXELS */
/*      sGL_UNPACK_ALIGNMENT */
/*      sGL_PACK_SWAP_BYTES */
/*      sGL_PACK_LSB_FIRST */
/*      sGL_PACK_ROW_LENGTH */
/*      sGL_PACK_SKIP_ROWS */
/*      sGL_PACK_SKIP_PIXELS */
/*      sGL_PACK_ALIGNMENT */

/* PixelTransfer */
/*      sGL_MAP_COLOR */
/*      sGL_MAP_STENCIL */
/*      sGL_INDEX_SHIFT */
/*      sGL_INDEX_OFFSET */
/*      sGL_RED_SCALE */
/*      sGL_RED_BIAS */
/*      sGL_GREEN_SCALE */
/*      sGL_GREEN_BIAS */
/*      sGL_BLUE_SCALE */
/*      sGL_BLUE_BIAS */
/*      sGL_ALPHA_SCALE */
/*      sGL_ALPHA_BIAS */
/*      sGL_DEPTH_SCALE */
/*      sGL_DEPTH_BIAS */

/* PixelType */
#define sGL_BITMAP                         0x1A00
/*      sGL_BYTE */
/*      sGL_UNSIGNED_BYTE */
/*      sGL_SHORT */
/*      sGL_UNSIGNED_SHORT */
/*      sGL_INT */
/*      sGL_UNSIGNED_INT */
/*      sGL_FLOAT */

/* PolygonMode */
#define sGL_POINT                          0x1B00
#define sGL_LINE                           0x1B01
#define sGL_FILL                           0x1B02

/* ReadBufferMode */
/*      sGL_FRONT_LEFT */
/*      sGL_FRONT_RIGHT */
/*      sGL_BACK_LEFT */
/*      sGL_BACK_RIGHT */
/*      sGL_FRONT */
/*      sGL_BACK */
/*      sGL_LEFT */
/*      sGL_RIGHT */
/*      sGL_AUX0 */
/*      sGL_AUX1 */
/*      sGL_AUX2 */
/*      sGL_AUX3 */

/* RenderingMode */
#define sGL_RENDER                         0x1C00
#define sGL_FEEDBACK                       0x1C01
#define sGL_SELECT                         0x1C02

/* ShadingModel */
#define sGL_FLAT                           0x1D00
#define sGL_SMOOTH                         0x1D01


/* StencilFunction */
/*      sGL_NEVER */
/*      sGL_LESS */
/*      sGL_EQUAL */
/*      sGL_LEQUAL */
/*      sGL_GREATER */
/*      sGL_NOTEQUAL */
/*      sGL_GEQUAL */
/*      sGL_ALWAYS */

/* StencilOp */
/*      sGL_ZERO */
#define sGL_KEEP                           0x1E00
#define sGL_REPLACE                        0x1E01
#define sGL_INCR                           0x1E02
#define sGL_DECR                           0x1E03
/*      sGL_INVERT */

/* StringName */
#define sGL_VENDOR                         0x1F00
#define sGL_RENDERER                       0x1F01
#define sGL_VERSION                        0x1F02
#define sGL_EXTENSIONS                     0x1F03

/* TextureCoordName */
#define sGL_S                              0x2000
#define sGL_T                              0x2001
#define sGL_R                              0x2002
#define sGL_Q                              0x2003

/* TexCoordPointerType */
/*      sGL_SHORT */
/*      sGL_INT */
/*      sGL_FLOAT */
/*      sGL_DOUBLE */

/* TextureEnvMode */
#define sGL_MODULATE                       0x2100
#define sGL_DECAL                          0x2101
/*      sGL_BLEND */
/*      sGL_REPLACE */

/* TextureEnvParameter */
#define sGL_TEXTURE_ENV_MODE               0x2200
#define sGL_TEXTURE_ENV_COLOR              0x2201

/* TextureEnvTarget */
#define sGL_TEXTURE_ENV                    0x2300

/* TextureGenMode */
#define sGL_EYE_LINEAR                     0x2400
#define sGL_OBJECT_LINEAR                  0x2401
#define sGL_SPHERE_MAP                     0x2402

/* TextureGenParameter */
#define sGL_TEXTURE_GEN_MODE               0x2500
#define sGL_OBJECT_PLANE                   0x2501
#define sGL_EYE_PLANE                      0x2502

/* TextureMagFilter */
#define sGL_NEAREST                        0x2600
#define sGL_LINEAR                         0x2601

/* TextureMinFilter */
/*      sGL_NEAREST */
/*      sGL_LINEAR */
#define sGL_NEAREST_MIPMAP_NEAREST         0x2700
#define sGL_LINEAR_MIPMAP_NEAREST          0x2701
#define sGL_NEAREST_MIPMAP_LINEAR          0x2702
#define sGL_LINEAR_MIPMAP_LINEAR           0x2703

/* TextureParameterName */
#define sGL_TEXTURE_MAG_FILTER             0x2800
#define sGL_TEXTURE_MIN_FILTER             0x2801
#define sGL_TEXTURE_WRAP_S                 0x2802
#define sGL_TEXTURE_WRAP_T                 0x2803
/*      sGL_TEXTURE_BORDER_COLOR */
/*      sGL_TEXTURE_PRIORITY */

/* TextureTarget */
/*      sGL_TEXTURE_1D */
/*      sGL_TEXTURE_2D */
/*      sGL_PROXY_TEXTURE_1D */
/*      sGL_PROXY_TEXTURE_2D */

/* TextureWrapMode */
#define sGL_CLAMP                          0x2900
#define sGL_REPEAT                         0x2901

/* VertexPointerType */
/*      sGL_SHORT */
/*      sGL_INT */
/*      sGL_FLOAT */
/*      sGL_DOUBLE */

/* ClientAttribMask */
#define sGL_CLIENT_PIXEL_STORE_BIT         0x00000001
#define sGL_CLIENT_VERTEX_ARRAY_BIT        0x00000002
#define sGL_CLIENT_ALL_ATTRIB_BITS         0xffffffff

/* polygon_offset */
#define sGL_POLYGON_OFFSET_FACTOR          0x8038
#define sGL_POLYGON_OFFSET_UNITS           0x2A00
#define sGL_POLYGON_OFFSET_POINT           0x2A01
#define sGL_POLYGON_OFFSET_LINE            0x2A02
#define sGL_POLYGON_OFFSET_FILL            0x8037

/* texture */
#define sGL_ALPHA4                         0x803B
#define sGL_ALPHA8                         0x803C
#define sGL_ALPHA12                        0x803D
#define sGL_ALPHA16                        0x803E
#define sGL_LUMINANCE4                     0x803F
#define sGL_LUMINANCE8                     0x8040
#define sGL_LUMINANCE12                    0x8041
#define sGL_LUMINANCE16                    0x8042
#define sGL_LUMINANCE4_ALPHA4              0x8043
#define sGL_LUMINANCE6_ALPHA2              0x8044
#define sGL_LUMINANCE8_ALPHA8              0x8045
#define sGL_LUMINANCE12_ALPHA4             0x8046
#define sGL_LUMINANCE12_ALPHA12            0x8047
#define sGL_LUMINANCE16_ALPHA16            0x8048
#define sGL_INTENSITY                      0x8049
#define sGL_INTENSITY4                     0x804A
#define sGL_INTENSITY8                     0x804B
#define sGL_INTENSITY12                    0x804C
#define sGL_INTENSITY16                    0x804D
#define sGL_R3_G3_B2                       0x2A10
#define sGL_RGB4                           0x804F
#define sGL_RGB5                           0x8050
#define sGL_RGB8                           0x8051
#define sGL_RGB10                          0x8052
#define sGL_RGB12                          0x8053
#define sGL_RGB16                          0x8054
#define sGL_RGBA2                          0x8055
#define sGL_RGBA4                          0x8056
#define sGL_RGB5_A1                        0x8057
#define sGL_RGBA8                          0x8058
#define sGL_RGB10_A2                       0x8059
#define sGL_RGBA12                         0x805A
#define sGL_RGBA16                         0x805B
#define sGL_TEXTURE_RED_SIZE               0x805C
#define sGL_TEXTURE_GREEN_SIZE             0x805D
#define sGL_TEXTURE_BLUE_SIZE              0x805E
#define sGL_TEXTURE_ALPHA_SIZE             0x805F
#define sGL_TEXTURE_LUMINANCE_SIZE         0x8060
#define sGL_TEXTURE_INTENSITY_SIZE         0x8061
#define sGL_PROXY_TEXTURE_1D               0x8063
#define sGL_PROXY_TEXTURE_2D               0x8064

/* texture_object */
#define sGL_TEXTURE_PRIORITY               0x8066
#define sGL_TEXTURE_RESIDENT               0x8067
#define sGL_TEXTURE_BINDING_1D             0x8068
#define sGL_TEXTURE_BINDING_2D             0x8069

/* vertex_array */
#define sGL_VERTEX_ARRAY                   0x8074
#define sGL_NORMAL_ARRAY                   0x8075
#define sGL_COLOR_ARRAY                    0x8076
#define sGL_INDEX_ARRAY                    0x8077
#define sGL_TEXTURE_COORD_ARRAY            0x8078
#define sGL_EDGE_FLAG_ARRAY                0x8079
#define sGL_VERTEX_ARRAY_SIZE              0x807A
#define sGL_VERTEX_ARRAY_TYPE              0x807B
#define sGL_VERTEX_ARRAY_STRIDE            0x807C
#define sGL_NORMAL_ARRAY_TYPE              0x807E
#define sGL_NORMAL_ARRAY_STRIDE            0x807F
#define sGL_COLOR_ARRAY_SIZE               0x8081
#define sGL_COLOR_ARRAY_TYPE               0x8082
#define sGL_COLOR_ARRAY_STRIDE             0x8083
#define sGL_INDEX_ARRAY_TYPE               0x8085
#define sGL_INDEX_ARRAY_STRIDE             0x8086
#define sGL_TEXTURE_COORD_ARRAY_SIZE       0x8088
#define sGL_TEXTURE_COORD_ARRAY_TYPE       0x8089
#define sGL_TEXTURE_COORD_ARRAY_STRIDE     0x808A
#define sGL_EDGE_FLAG_ARRAY_STRIDE         0x808C
#define sGL_VERTEX_ARRAY_POINTER           0x808E
#define sGL_NORMAL_ARRAY_POINTER           0x808F
#define sGL_COLOR_ARRAY_POINTER            0x8090
#define sGL_INDEX_ARRAY_POINTER            0x8091
#define sGL_TEXTURE_COORD_ARRAY_POINTER    0x8092
#define sGL_EDGE_FLAG_ARRAY_POINTER        0x8093
#define sGL_V2F                            0x2A20
#define sGL_V3F                            0x2A21
#define sGL_C4UB_V2F                       0x2A22
#define sGL_C4UB_V3F                       0x2A23
#define sGL_C3F_V3F                        0x2A24
#define sGL_N3F_V3F                        0x2A25
#define sGL_C4F_N3F_V3F                    0x2A26
#define sGL_T2F_V3F                        0x2A27
#define sGL_T4F_V4F                        0x2A28
#define sGL_T2F_C4UB_V3F                   0x2A29
#define sGL_T2F_C3F_V3F                    0x2A2A
#define sGL_T2F_N3F_V3F                    0x2A2B
#define sGL_T2F_C4F_N3F_V3F                0x2A2C
#define sGL_T4F_C4F_N3F_V4F                0x2A2D

/* Extensions */
#define sGL_EXT_vertex_array               1
#define sGL_EXT_bgra                       1
#define sGL_EXT_paletted_texture           1
#define sGL_WIN_swap_hint                  1
#define sGL_WIN_draw_range_elements        1
// #define sGL_WIN_phong_shading              1
// #define sGL_WIN_specular_fog               1

/* EXT_vertex_array */
#define sGL_VERTEX_ARRAY_EXT               0x8074
#define sGL_NORMAL_ARRAY_EXT               0x8075
#define sGL_COLOR_ARRAY_EXT                0x8076
#define sGL_INDEX_ARRAY_EXT                0x8077
#define sGL_TEXTURE_COORD_ARRAY_EXT        0x8078
#define sGL_EDGE_FLAG_ARRAY_EXT            0x8079
#define sGL_VERTEX_ARRAY_SIZE_EXT          0x807A
#define sGL_VERTEX_ARRAY_TYPE_EXT          0x807B
#define sGL_VERTEX_ARRAY_STRIDE_EXT        0x807C
#define sGL_VERTEX_ARRAY_COUNT_EXT         0x807D
#define sGL_NORMAL_ARRAY_TYPE_EXT          0x807E
#define sGL_NORMAL_ARRAY_STRIDE_EXT        0x807F
#define sGL_NORMAL_ARRAY_COUNT_EXT         0x8080
#define sGL_COLOR_ARRAY_SIZE_EXT           0x8081
#define sGL_COLOR_ARRAY_TYPE_EXT           0x8082
#define sGL_COLOR_ARRAY_STRIDE_EXT         0x8083
#define sGL_COLOR_ARRAY_COUNT_EXT          0x8084
#define sGL_INDEX_ARRAY_TYPE_EXT           0x8085
#define sGL_INDEX_ARRAY_STRIDE_EXT         0x8086
#define sGL_INDEX_ARRAY_COUNT_EXT          0x8087
#define sGL_TEXTURE_COORD_ARRAY_SIZE_EXT   0x8088
#define sGL_TEXTURE_COORD_ARRAY_TYPE_EXT   0x8089
#define sGL_TEXTURE_COORD_ARRAY_STRIDE_EXT 0x808A
#define sGL_TEXTURE_COORD_ARRAY_COUNT_EXT  0x808B
#define sGL_EDGE_FLAG_ARRAY_STRIDE_EXT     0x808C
#define sGL_EDGE_FLAG_ARRAY_COUNT_EXT      0x808D
#define sGL_VERTEX_ARRAY_POINTER_EXT       0x808E
#define sGL_NORMAL_ARRAY_POINTER_EXT       0x808F
#define sGL_COLOR_ARRAY_POINTER_EXT        0x8090
#define sGL_INDEX_ARRAY_POINTER_EXT        0x8091
#define sGL_TEXTURE_COORD_ARRAY_POINTER_EXT 0x8092
#define sGL_EDGE_FLAG_ARRAY_POINTER_EXT    0x8093
#define sGL_DOUBLE_EXT                     sGL_DOUBLE

/* EXT_bgra */
#define sGL_BGR_EXT                        0x80E0
#define sGL_BGRA_EXT                       0x80E1

/* EXT_paletted_texture */

/* These must match the sGL_COLOR_TABLE_*_SGI enumerants */
#define sGL_COLOR_TABLE_FORMAT_EXT         0x80D8
#define sGL_COLOR_TABLE_WIDTH_EXT          0x80D9
#define sGL_COLOR_TABLE_RED_SIZE_EXT       0x80DA
#define sGL_COLOR_TABLE_GREEN_SIZE_EXT     0x80DB
#define sGL_COLOR_TABLE_BLUE_SIZE_EXT      0x80DC
#define sGL_COLOR_TABLE_ALPHA_SIZE_EXT     0x80DD
#define sGL_COLOR_TABLE_LUMINANCE_SIZE_EXT 0x80DE
#define sGL_COLOR_TABLE_INTENSITY_SIZE_EXT 0x80DF

#define sGL_COLOR_INDEX1_EXT               0x80E2
#define sGL_COLOR_INDEX2_EXT               0x80E3
#define sGL_COLOR_INDEX4_EXT               0x80E4
#define sGL_COLOR_INDEX8_EXT               0x80E5
#define sGL_COLOR_INDEX12_EXT              0x80E6
#define sGL_COLOR_INDEX16_EXT              0x80E7

/* WIN_draw_range_elements */
#define sGL_MAX_ELEMENTS_VERTICES_WIN      0x80E8
#define sGL_MAX_ELEMENTS_INDICES_WIN       0x80E9

/* WIN_phong_shading */
#define sGL_PHONG_WIN                      0x80EA 
#define sGL_PHONG_HINT_WIN                 0x80EB 

/* WIN_specular_fog */
#define sGL_FOG_SPECULAR_TEXTURE_WIN       0x80EC

#endif

