/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_PAINTER_HPP
#define FILE_ALTONA2_LIBS_UTIL_PAINTER_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Image.hpp"
#include "Altona2/Libs/Util/GraphicsHelper.hpp"
#include "Altona2/Libs/Util/TextureFont.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Private and Helper                                                 ***/
/***                                                                      ***/
/****************************************************************************/

class sPainterFont;
class sPainterImage;
struct sPainterCommand;
class sFont;
class sFontResource;

struct sPainterFontPara
{
    sPainterFont *Font;
    float SizeX;
    float SizeY;

    sPainterFontPara();
    sPainterFontPara(sPainterFont *,float size=1.0f);
};

struct sFontPara
{
    sString<256> Name;              // name of the font
    int Size;                       // height 
    int Flags;                      // sSFF_???
    int MaxUnicode;                 // generate code up to this
    float Zoom;                     // scaling for extra-large non-df
    float Blur;                     // set to 1.0f for a blur that does correct anti-alias
    float Outline;                  //

    sFontPara();
    sFontPara(const char *name,int size,int flags = 0,int maxcode = 0x100);
};

enum sPainterFlags
{
    // font flags

    // image flags

    sPIF_Alpha          = 0x0000001,
    sPIF_AlphaOnly      = 0x0000002,
    sPIF_VertexPWT      = 0x0000004,
    sPIF_VertexPWCT     = 0x0000008,
    sPIF_Add            = 0x0000010,
    sPIF_AllowFailure   = 0x0000020,
    sPIF_Clamp          = 0x0000040,
    sPIF_PointFilter    = 0x0000080,

    // print flags

    sPPF_Left           = 0x0000001,
    sPPF_Right          = 0x0000002,
    sPPF_Top            = 0x0000004,
    sPPF_Bottom         = 0x0000008,

    sPPF_Space          = 0x0000010,
    sPPF_Baseline       = 0x0000020,
    sPPF_Integer        = 0x0000040,
    sPPF_Downwards      = 0x0000080,
    sPPF_Upwards        = 0x0000100,
    sPPF_Wrap           = 0x0004000,
};

/****************************************************************************/
/***                                                                      ***/
/***   The Painter                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sPainter
{
    static const int MaxTextureId = 0x100;
    static uint IdBits[MaxTextureId/32];

    struct Cmd;

    enum Enums
    {
        HashSize = 256,
        HashMask = 255,

        MaxTex = 0x7ff,
        MaxLayer = 0xffff,
        MaxTransform = 0xffff,
        MaxClipRect = 0xffff,
    };

    sAdapter *Adapter;
    sPainterCommand *CommandHash[HashSize];

    sMemoryPool *Pool;
    sArray<sPainterCommand *> Commands;
    sArray<sMatrix33> Transforms;
    sArray<sFRect> ClipRects;

    uint16 Transform;
    uint16 ClipRect;
    uint16 Layer;
    bool Scissor;
    sPainterFontPara FontPara;
    sFont *Font;

    void Render(sContext *ctx);
    void Init(sPainterCommand *cmd,uint8 kind,int tex=MaxTex);

    sTargetPara TargetPara;
    sMaterial *MtrlFlat;
    sCBuffer<sFixedMaterialVC> *cbv0;

    void AddCommand(sPainterCommand *);
    void Text_(int flags,uint color,const sFRect &r,const char *text,uptr len=-1);

    // font management

    sArray<sFontResource *> FontResources;
    sArray<sFont *> Fonts;
    sTextureFontGenerator TexGen;
public:
    sPainter(sAdapter *adapter);
    ~sPainter();

    // Painting control

    void Begin(sTargetPara &tp);
    void End(sContext *ctx);
    void SetLayer(int n);
    int GetLayer();
    void SetFont(const sPainterFontPara &fp);
    void SetFont(sFont *);
    void SetTransform(const sMatrix33 &mat);
    void ResetTransform();
    void EnableScissor(bool enable);
    void SetClip(const sFRect &r);
    void SetClip(const sRect &r);
    void ResetClip();
    int SaveClip();
    void RestoreClip(int saved);
    bool AndClip(const sFRect &r);
    bool AndClip(const sRect &r);

    // font management, resource management

    sFont *GetFont(const sFontPara &fp);
    void ReleaseFont(sFont *);
    static int FindBit();
    static void ClearBit(int);

    // primitives (float)

    void Line(uint col,const sFRect &r);
    void Frame(uint col,const sFRect &r);
    void Rect(uint col,const sFRect &r);
    void RectC(const uint *col,const sFRect &r);
    void Image(sPainterImage *img,uint col,const sFRect &src,const sFRect &dest);
    void Image(sPainterImage *img,uint col,const sFRect &src,float dx,float dy);
    void Image(sPainterImage *img,uint col,float dx,float dy);
    void Image(sPainterImage *img,int vc,int ic,const sVertexPCT *vp,const uint16 *ip);
    void Image(sPainterImage *img,int vc,int ic,const sVertexPWT *vp,const uint16 *ip);
    void Image(sPainterImage *img,int vc,int ic,const sVertexPWCT *vp,const uint16 *ip);
    void Letter(int flags,uint color,float x,float y,int letter);
    void Text(int flags,uint color,const sFRect &r,const char *text,uptr len=-1);
    void Text(int flags,uint color,float x,float y,const char *text,uptr len=-1);

    // primitives (int)

    void Line(uint col,const sRect &r)
    { Line(col,sFRect(r)); }
    void Frame(uint col,const sRect &r)
    { Frame(col,sFRect(r)); }
    void Rect(uint col,const sRect &r)
    { Rect(col,sFRect(r)); }
    void RectC(const uint *col,const sRect &r)
    { RectC(col,sFRect(r)); }
    void Image(sPainterImage *img,uint col,const sRect &src,const sRect &dest)
    { Image(img,col,sFRect(src),sFRect(dest)); }
    void Image(sPainterImage *img,uint col,const sRect &src,int dx,int dy)
    { Image(img,col,sFRect(src),float(dx),float(dy)); }
    void Image(sPainterImage *img,uint col,int dx,int dy)
    { Image(img,col,float(dx),float(dy)); }
    void Letter(int flags,uint color,int x,int y,int letter)
    { Letter(flags|sPPF_Integer,color,float(x),float(y),letter); }
    void Text(int flags,uint color,const sRect &r,const char *text,uptr len=-1)
    { Text(flags|sPPF_Integer,color,sFRect(r),text,len); }
    void Text(int flags,uint color,int x,int y,const char *text,uptr len=-1)
    { Text(flags|sPPF_Integer,color,float(x),float(y),text,len); }

    // other helpers

    void Frame(uint col0,uint col1,const sFRect &r,float w=1);
    void Frame(uint col0,uint col1,const sRect &r,int w=1);

};

/****************************************************************************/

class sPainterImage
{
    // static uint TextureIdCounter;
    void Init(sAdapter *ada,sFImage *img,int flags,int format);
    void Init(sAdapter *ada,sImageData *img,int flags);
public:
    sPainterImage();
    sPainterImage(sAdapter *ada,sImageData *img,int flags);
    sPainterImage(sAdapter *ada,sImage *img,int flags,int format=sRF_BGRA8888);
    sPainterImage(sAdapter *ada,sFImage *img,int flags,int format=sRF_BGRA8888);
    sPainterImage(sAdapter *ada,const char *filename,int flags,int format=sRF_BGRA8888);
    sPainterImage(sMaterial *Mtrl);
    sPainterImage(sAdapter *ada,sMaterial *Mtrl,sFImage *img,sVertexFormat *fmt, const sSamplerStatePara *ssp=0);
    ~sPainterImage();
    sResource *Texture;
    sMaterial *Mtrl;
    sCBufferBase *ExtraCB;

    int SizeX;
    int SizeY;
    uint TextureId;
};

/****************************************************************************/

class sPainterFont
{
    friend class sPainter;

    struct Cell
    {
        int16 a,b,c;
        uint16 x0;
        uint16 y0;

        template <class Ser> void Serialize(Ser &s)
        { s | a | b | c | x0 | y0; }
    };
    enum sPainterFontEnum
    {
        CellsPerPage = 128,
        MaxPages = 512,
    };
    struct Page
    {
        Cell Cells[CellsPerPage];
    };
    struct LetterRange
    {
        int l0,l1;
    };
    struct ParseLetterInfo
    {
        sArray<LetterRange> Ranges;
        int LetterCount;
        int MaxLetter;
    };
    Cell DefaultChar;
    Page *DefaultPage;
    Page **Pages;
    uint8 *Bitmap;
    int BitmapSizeX;
    int BitmapSizeY;

    void ParseLetterRanges(sPainterFont::ParseLetterInfo &li,const char *letters);
    sPainterImage *Image;

    const Cell *GetCell(int c) const { return &Pages[(c/CellsPerPage)&(MaxPages-1)]->Cells[c&(CellsPerPage-1)]; }
    Cell *GetCellX(int c) { return &Pages[(c/CellsPerPage)&(MaxPages-1)]->Cells[c&(CellsPerPage-1)]; }

    void Init();
    void Init(sSystemFont *sfont,const char *letters);
    void MakeImage();
    sAdapter *Adapter;
public:
    sPainterFont(sAdapter *);
    sPainterFont(sAdapter *,sSystemFont *sfont,const char *letters=0);
    sPainterFont(sAdapter *,const char *fontname,int flags,int size,const char *letters=0);
    sPainterFont(sAdapter *,sSystemFontId id,int flags,int size,const char *letters=0);
    ~sPainterFont();

    int Baseline;
    int Advance;
    int CellHeight;

    sPainterImage *GetImage() { return Image; }
    int GetWidth(const char *text,uptr len=-1);
    int GetWidth(int c);
    void GetCell(int c,int *abc) const
    {
        const Cell *cell = GetCell(c); 
        abc[0] = cell->a; abc[1] = cell->b; abc[2] = cell->c;
    }
    template <class Ser> void Serialize(Ser &s);
    bool CheckLetters(const char *text,uptr len=-1);
};
sReader& operator| (sReader &s,sPainterFont &v);
sWriter& operator| (sWriter &s,sPainterFont &v);

/****************************************************************************/
/***                                                                      ***/
/***   Font System                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sFontResource
{
    friend class sPainter;
    int RefCount;
    sFontResource(sAdapter *ada,const sFontPara &fp);
    ~sFontResource();
public:
    sAdapter *Adapter;
    sFontPara Para;
    sTextureFont *TFont;
};

class sFont
{
    friend class sPainter;
    int RefCount;
    int TextureId;

    sFont(sFontResource *font,const sFontPara &fp);
    ~sFont();
public:
    sFontResource *Font;
    sFontPara Para;
    sTextureFontPrintPara PaintPara;

    int Advance;
    int CellHeight;
    int Baseline;
    int GetWidth(int c);
    int GetWidth(const char *s,uptr len=-1);
    float GetWidthF(int c);
    float GetWidthF(const char *s,uptr len=-1);
    bool CheckLetters(const char *text,uptr len=-1);
};

/****************************************************************************/
/***                                                                      ***/
/***   Multiline Text Format Helper                                       ***/
/***                                                                      ***/
/****************************************************************************/

class sPainterMultiline
{
    struct Line
    {
        uptr Start;
        uptr Count;
    };
    sArray<Line> Lines;
    int LastWidth;
    const char *Text;
    sFont *Font;
    int SizeX;

public:
    sPainterMultiline();
    ~sPainterMultiline();

    void Init(const char *text,sFont *font);
    int Layout(int width);
    void Print(sPainter *pnt,const sRect &r,uint col,int underline=0,int align=0);
    int GetSizeX() { return SizeX; }
    int GetSizeY() { return Lines.GetCount()*Font->Advance; }
    uptr GetLines() { return Lines.GetCount(); }
};

/****************************************************************************/
}

#endif  // FILE_ALTONA2_LIBS_UTIL_PAINTER_HPP

