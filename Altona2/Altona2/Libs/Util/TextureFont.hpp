/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_TEXTUREFONT_HPP
#define FILE_ALTONA2_LIBS_UTIL_TEXTUREFONT_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Image.hpp"
#include "Altona2/Libs/Util/UtilShaders.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   TextureFont, ready for use                                         ***/
/***                                                                      ***/
/****************************************************************************/

struct sTextureFontPara
{
    sTextureFontPara();

    sString<sMaxPath> FontName;     // name of the system font
    int SizeY;                      // height in texture, in pixels
    float Inside;                   // how many pixels the distance field should go inside.
    float Outside;                  // how many pixels the distance field should go outside.
    int Intra;                      // how many empty pixels between letters?
    int MaxSizeX;                   // maximum allowed width
    int MaxSizeY;                   // maximum allowed height
    int FontFlags;                  // sSFF_Bold, sSFF_Italics
    int FontSize;                   // height of the system font

    enum FormatEnum
    {
        I8,                         // 8 bit integer
        I16,                        // 16 bit integer
        F32,                        // 32 bit float, no scaling for Inside/Outside is done
    } Format;                       // pixel format of the font
    enum GenerateBits
    {
        Texture = 1,                // generate textures
        Images = 2,                 // generate images
        Array = 4,                  // allow generation of multiple texture / image pages
    } Generate;                     // combination of the flags abive
};

struct sTextureFontLetter
{
    int Code;                       // code this letter represents
    int Page;                       // if multiple textures were created, the texture number
    sRect Cell;                     // position on texture, in pixels
    float OffsetX;                  // x-offset from cursor to top-left cell
    float OffsetY;                  // y-offset from cursor (on baseline) to top-left cell
    float AdvanceX;                 // advance cursor in X
    int FirstKern;                  // first kerning pair or KernPairs.GetCount().
};

struct sTextureFontKernPair
{
    int CodeA;
    int CodeB;
    float Kern;
};

struct sTextureFontPrintPara
{
    sTextureFontPrintPara();

    sMatrix44 MS2SS;
    float Blur;
    float Zoom;
    float Outline;
    bool Multisample;
	bool AlphaBlend;
	float VZoomX;
	float VZoomY;	
	float SpacingX;
	float SpacingY;
	bool IgnorePartialInCheck;
    bool ZOn;
};

struct sTextLine
{
    int     Length;             // Length in characters (not bytes!)
    float   Width;              // Width in base pixels
    int     StartIndex;         // Line's start index in string (relative to text in bytes)
};

struct sTextBlockData
{
    const char*         Text;               // Text to be used
    sTextLine          *Lines;              // Array of lines
    int                 BufferSize;         // size of lines array
    int                 BufferUsage;        // used lines in lines array
    float               LineHeigth;         // Hieght of a line
    float               BlockHeight;        // The text block's total height
};

class sTextureFont
{
    // this block of variables are used for the Begin()/End() api
    int VertexCount;
    int VertexMax;
    sVertexPCT *VertexPtr;
    float Factor;
    float ScaleX;
    float ScaleY;
    float Zoom;
	float VZoomX;
	float VZoomY;
	float SpacingX;
	float SpacingY;
	bool IgnorePartialInCheck;

    // Data for block text
    sTextBlockData      BlockData;

    float PaintSize;
    int LastChar;
    sMaterial *Mtrl;
    void RestartGeometry(int vc);
    void CreateCharTable();
public:
    sTextureFont();
    ~sTextureFont();
    void CreateTexture(sAdapter *ada);
    void CreateMtrl();

    float Advance;                  // distance top to next line top
    float Baseline;                 // distance top to baseline
    float MaxOuter;                 // maximum allowed positive distance field offset (in pixel)
    
    float DistScale,DistAdd;        // dist = (texld() + DistAdd) * DistScale;  this is 1;0 for float

    sArray<sTextureFontLetter> Letters;
    sArray<sTextureFontKernPair> KernPairs;

    sTextureFontLetter ***LetterFinder;
    const sTextureFontLetter* GetLetter(int code) const;
    float GetKerning(int codea,int codeb) const;

    int SizeX;                      // width of texture (all same)
    int SizeY;                      // height of texture (all same)
    int SizeA;                      // number of pages (1 if no array texture was generated)
    sResource * Texture;            // texture resource
    void *TextureData;              // data blob
    int TextureDataSize;
    int TextureFormat;              // sRF_???

    float Mul;                      // shader parameters
    float IMul;
    float Sub;
    float TexZoom;
    int CreationFlags;

    sAdapter *Adapter;
    sCBuffer<sFixedMaterialVC> *cbv0;
    sCBuffer<sDistanceFieldShader_cbps> *cbp0;
    sMaterial *DistMtrl[8];
    sFixedMaterial *TexMtrl;
    sContext *Context;

    // simple draw api

    float GetWidth(float size,const char *text,uptr len=-1);
    void Print(float x,float y,const sTextureFontPrintPara &para,uint color,const char *text,uptr len=-1);

    // optimized api

    float CalcWordLength( const char* text, const char* end );
    float CalcWordLength( const char* text, const char* end, const char*& newText, uptr& len );
    void CalcArea( int flags, float x, float y, sFRect *clip, const char *text, uptr len, sFRect &area );
    void Begin(const sTextureFontPrintPara &para);
    void Print(int flags, float x,float y,sFRect *clip,uint color,const char *text,uptr len=-1,uint* cols=0,int numcols=0);
    void PrintUp(float x,float y,sFRect *clip,uint color,const char *text,uptr len=-1);
    void PrintDown(float x,float y,sFRect *clip,uint color,const char *text,uptr len=-1);
    void Print(int flags,sFRect &client,sFRect *clip,uint color,const char *text,uptr len=-1,uint* cols=0,int numcols=0);
    void End();

    // block text api
    void GrowLineBuffer();
    float PrepareBlock( sFRect* area, const char* text, const sTextureFontPrintPara &para );
    void  PrintBlock( sFRect* area, float scroll, uint color );

    template <class Ser> void Serialize(Ser &s);
};
sReader& operator| (sReader &s,sTextureFont &v);
sWriter& operator| (sWriter &s,sTextureFont &v);

/****************************************************************************/
/***                                                                      ***/
/***   Font Generator                                                     ***/
/***                                                                      ***/
/****************************************************************************/


class sTextureFontGenerator
{
public:
    struct Bezier;
    struct ShapeSegment;
    class DistGen;
    class Shape;
    class Letter;
private:
    sMemoryPool Pool;
    sSystemFont *Font; 
    sTextureFontPara Para;
    sArray<Letter *> Letters;
    sFMonoImage DistX;
    sFMonoImage DistY;
    sArray<const sSystemFontInfo *> FontDir;
    
    bool Puzzle(int TexSizeX,int TexSizeY);
    int AAFactor;
public:
    sTextureFontGenerator();
    ~sTextureFontGenerator();

    void Begin(const sTextureFontPara &para);
    void AddLetter(int code);
    sTextureFont *End();
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_UTIL_TEXTUREFONT_HPP

