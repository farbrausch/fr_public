/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/GraphicsHelper.hpp"
#include "Altona2/Libs/Util/Config.hpp"
#include "Painter.hpp"

namespace Altona2 {

/****************************************************************************/

enum sPainterCommandKind
{
    sPC_Line = 1,
    sPC_Frame,
    sPC_Rect,
    sPC_RectC,
    sPC_Image,
    sPC_ImagePCT,
    sPC_ImagePWT,
    sPC_ImagePWCT,
    sPC_Text,
    sPC_Text2,
};

struct sPainterCommand
{
    uint Sort;                    // llllllll llllllll tttttttt tttscccc : l=layer, t=tex, s=scissor, c=command
    sPainterCommand *NextHash;
    sPainterCommand *NextSort;

    uint16 Transform;
    uint16 ClipRect;

    sPainterCommandKind GetKind() { return sPainterCommandKind(Sort&15); }
    bool GetScissor() { return Sort & 0x10 ? 1 : 0; }
};

struct sPainterCommandSimple : public sPainterCommand
{
    uint Color;
    sFRect Rect;
};

struct sPainterCommandRectC : public sPainterCommand
{
    uint Color[4];
    sFRect Rect;
};

struct sPainterCommandImage : public sPainterCommand
{
    uint Color;
    sPainterImage *Image;
    sFRect SRect;
    sFRect DRect;
};

struct sPainterCommandImagePCT : public sPainterCommand
{
    sPainterImage *Image;
    int vc;
    int ic;
    sVertexPCT *vp;
    uint16 *ip;
};

struct sPainterCommandImagePWT : public sPainterCommand
{
    sPainterImage *Image;
    int vc;
    int ic;
    sVertexPWT *vp;
    uint16 *ip;
};

struct sPainterCommandImagePWCT : public sPainterCommand
{
    sPainterImage *Image;
    int vc;
    int ic;
    sVertexPWCT *vp;
    uint16 *ip;
};

struct sPainterCommandText : public sPainterCommand
{
    sPainterFontPara FontPara;
    int Flags;
    uint Color;
    const char *Text;
    int Length;
    sFRect Rect;
};

struct sPainterCommandText2 : public sPainterCommand
{
    sFont *Font;
    int Flags;
    uint Color;
    const char *Text;
    int Length;
    sFRect Rect;
};

/****************************************************************************/

sPainterFontPara::sPainterFontPara()
{
    Font = 0;
    SizeX = 1.0f;
    SizeY = 1.0f;
}

sPainterFontPara::sPainterFontPara(sPainterFont *f,float size)
{
    Font = f;
    SizeX = SizeY = size;
}

/****************************************************************************/
/***                                                                      ***/
/***   sPainterImage                                                      ***/
/***                                                                      ***/
/****************************************************************************/

//uint sPainterImage::TextureIdCounter = 1;
uint sPainter::IdBits[MaxTextureId/32] = { 1 };

int sPainter::FindBit()
{
    for(int i=0;i<MaxTextureId/32;i++)
    {
        if(IdBits[i]!=0xffffffff)
        {
            uint bits = IdBits[i];
            for(int j=0;j<32;j++)
            {
                if(!(bits & (1<<j)))
                {
                    IdBits[i] |= 1<<j;
                    return i*32+j;
                }
            }
        }
    }
    sASSERT0();
    return 0;
}

void sPainter::ClearBit(int i)
{
    IdBits[i/32] &= ~(1<<(i&31));
}

/****************************************************************************/

sPainterImage::sPainterImage()
{
    SizeX = 0;
    SizeY = 0;
    Texture = 0;
    Mtrl = 0;
    TextureId = 0;
    ExtraCB = 0;
}

void sPainterImage::Init(sAdapter *ada,sFImage *fimg,int flags,int format)
{
    sPrepareTexture2D prep;
    prep.Load(fimg);
    prep.Format = format;
    prep.Mipmaps = 1;
    prep.Process();

    sImageData *imgd = prep.GetImageData();
    Init(ada,imgd,flags);
    delete imgd;
}

void sPainterImage::Init(sAdapter *ada,sImageData *imgd,int flags)
{
    TextureId = sPainter::FindBit();  
    // sASSERT(TextureIdCounter<0xfe);
    ExtraCB = 0;

    Texture = imgd->MakeTexture(ada);
    SizeX = imgd->SizeX;
    SizeY = imgd->SizeY;

    sSamplerStatePara ssp((flags & sPIF_Clamp?sTF_Clamp:sTF_Tile) | (flags & sPIF_PointFilter ? sTF_Point : sTF_Linear),0);
    sRenderStatePara rsp(sMTRL_DepthOff|sMTRL_CullOff,sMB_Off);
    if(flags & (sPIF_Alpha | sPIF_AlphaOnly))
        rsp.BlendColor[0] = sMB_Alpha;
    if(flags & (sPIF_Add))
        rsp.BlendColor[0] = sMB_Add;

    sFixedMaterial *mtrl = new sFixedMaterial;
    Mtrl = mtrl;

    sVertexFormat *fmt = ada->FormatPCT;
    if(flags & sPIF_AlphaOnly)
        mtrl->FMFlags = sFMF_TexAlphaOnly;
    if(flags & sPIF_VertexPWT)
    {
        fmt = ada->FormatPWT;
        //    rsp.Flags = sMTRL_DepthOn | sMTRL_CullOff;
        //    ssp.Flags = sTF_Tile|sTF_Aniso;
        ssp.MipLodBias = -1;
    }
    if(flags & sPIF_VertexPWCT)
    {
        fmt = ada->FormatPWCT;
        //    rsp.Flags = sMTRL_DepthOn | sMTRL_CullOff;
        //    ssp.Flags = sTF_Tile|sTF_Aniso;
        ssp.MipLodBias = -1;
    }

    mtrl->SetTexturePS(0,Texture,ada->CreateSamplerState(ssp));
    mtrl->SetState(ada->CreateRenderState(rsp));
    mtrl->Prepare(ada,fmt);
}

sPainterImage::sPainterImage(sAdapter *ada,sFImage *img,int flags,int format)
{
    Init(ada,img,flags,format);
}

sPainterImage::sPainterImage(sAdapter *ada,sImageData *img,int flags)
{
    Init(ada,img,flags);
}

sPainterImage::sPainterImage(sAdapter *ada,sImage *src,int flags,int format)
{
    sFImage *fimg = new sFImage;
    fimg->CopyFrom(src);
    Init(ada,fimg,flags,format);
    delete fimg;
}

sPainterImage::sPainterImage(sAdapter *ada,const char *filename,int flags,int format)
{
    sImage src;
    if(!src.Load(filename))
    {
        if(flags & sPIF_AllowFailure)
        {
            src.Init(16,16);
            src.Clear(0xffff0000);
        }
        else
        { 
            sFatalF("failed to load sPainterImage <%s>",filename);
        }
    }

    sFImage *fimg = new sFImage;
    fimg->CopyFrom(&src);
    Init(ada,fimg,flags,format);

    delete fimg;
}

sPainterImage::sPainterImage(sMaterial *mtrl)
{
    TextureId = sPainter::FindBit();  
    SizeX = 0;
    SizeY = 0;
    Texture = 0;
    TextureId = 0;
    ExtraCB = 0;
    Mtrl = mtrl;
}

// the material must be initializsed except for the texture,
// and ist must not be prepared.
// this function will create a texture, set it in the material
// and then prepare the material.

sPainterImage::sPainterImage(sAdapter *ada,sMaterial *mtrl,sFImage *img,sVertexFormat *fmt, const sSamplerStatePara *ssp)
{
    TextureId = sPainter::FindBit();  
    ExtraCB = 0;

    sPrepareTexture2D prep;
    prep.Load(img);
    prep.Format = (sConfigRender==sConfigRenderDX9) ? sRF_BGRA8888 : sRF_RGBA8888;
    prep.Mipmaps = 1;
    prep.Process();

    sImageData *imgd = prep.GetImageData();

    Texture = imgd->MakeTexture(ada);
    SizeX = imgd->SizeX;
    SizeY = imgd->SizeY;
    delete imgd;

    sSamplerStatePara tempssp(sTF_Tile|sTF_Linear,0);
    if (!ssp)
        ssp = &tempssp;
    mtrl->SetTexturePS(0,Texture,ada->CreateSamplerState(*ssp));
    mtrl->Prepare(ada,fmt);
    Mtrl = mtrl;
}

sPainterImage::~sPainterImage()
{
    sPainter::ClearBit(TextureId);
    delete Texture;
    delete Mtrl;
    delete ExtraCB;
}

/****************************************************************************/
/***                                                                      ***/
/***   sPainterFont                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sPainterFont::ParseLetterRanges(sPainterFont::ParseLetterInfo &li,const char *letters)
{
    li.Ranges.Clear();
    li.Ranges.HintSize(32);

    const char *l = letters;
    uint lastval = 0;
    bool range = 0;
    while(*l)
    {
        uint val = 0;
        if(l[0]=='/' && l[1]=='-')
        {
            val = l[1];
            l+=2;
        }
        else if(l[0]=='/' && l[1]>='0' && l[1]<='9')
        {
            val = l[1];
            l+=2;
        }
        else if(l[0]=='/' && l[1]=='u')
        {
            l+=2;
            while(sIsHex(*l))
            {
                if(*l>='0' && *l<'9')
                    val = val * 16 + *l -'0';
                else
                    val = val * 16 + (*l&15) + 9;
                l++;
            }
        }
        else
        {
            val = *l++;
        }

        if(range)
        {
            LetterRange lr;
            lr.l0 = lastval;
            lr.l1 = val;
            li.Ranges.Add(lr);
            lastval = 0;
            range = 0;
        }
        else
        {
            if(val=='-')
            {
                range = 1;
            }
            else
            {
                if(lastval)
                {
                    LetterRange lr;
                    lr.l0 = lastval;
                    lr.l1 = lastval;
                    li.Ranges.Add(lr);
                }
                lastval = val;
            }
        }
    }
    if(lastval)
    {
        LetterRange lr;
        lr.l0 = lastval;
        lr.l1 = lastval;
        li.Ranges.Add(lr);
    }

    li.LetterCount = 0;
    li.MaxLetter = 0;
    for(auto &lr : li.Ranges)
    {
        li.LetterCount += lr.l1 - lr.l0+1;
        li.MaxLetter = sMax(li.MaxLetter,lr.l1);
    }
}

/****************************************************************************/

static const char *sDefaultLetters = 
    "/u20-/u7e/ua0-/uff" // latin-1
    "/u0100-/u017f"   // eastern europe

    "/u20ac"          // euro
    "/u2122"          // trademark
    "/u2012-/u2015"   // various dashes
    "/u2018-/u201f"   // various quotes
    "/u2032-/u2034"   // primes, for geolocations (minutes, seconds)
    //  "/u0400-/u0523"
    //  "/u0374-/u03ff"
    ;

sPainterFont::sPainterFont(sAdapter *adapter)
{
    Adapter = adapter;
    Init();
}

sPainterFont::sPainterFont(sAdapter *adapter,sSystemFontId id,int flags,int size,const char *letters)
{
    Adapter = adapter;
    Init();
    if(letters == 0) letters = sDefaultLetters;
    const sSystemFontInfo *sfi = sGetSystemFont(id);
    sSystemFont *sfont = sLoadSystemFont(sfi,0,size,flags);
    Init(sfont,letters);
    delete sfont;
    delete sfi;
}

sPainterFont::sPainterFont(sAdapter *adapter,sSystemFont *sfont,const char *letters)
{
    Adapter = adapter;
    Init();
    if(letters == 0) letters = sDefaultLetters;
    Init(sfont,letters);
}

sPainterFont::sPainterFont(sAdapter *adapter,const char *fontname,int flags,int size,const char *letters)
{
    Adapter = adapter;
    Init();
#if sConfigPlatform==sConfigPlatformIOS || sConfigPlatform==sConfigPlatformOSX
    sReader s("data/save_font.pfont");
    s | *this;
#else
    if(letters == 0) letters = sDefaultLetters;
    sArray<const sSystemFontInfo *> dir;
    bool ok = sLoadSystemFontDir(dir);
    sASSERT(ok);
    for(auto sfi : dir)
    {
        if(sCmpStringI(sfi->Name,fontname)==0)
        {
            sSystemFont *sfont = sLoadSystemFont(sfi,0,size,flags);
            Init(sfont,letters);
            delete sfont;
            dir.DeleteAll();
            return;
        }
    }
    dir.DeleteAll();
    sASSERT0();
#endif
}


void sPainterFont::Init()
{
    Image = 0;
    Advance = 0;
    Baseline = 0;
    CellHeight = 0;
    Bitmap = 0;
    BitmapSizeX = 0;
    BitmapSizeY = 0;
    Pages = new Page*[MaxPages];
    for(int i=0;i<MaxPages;i++)
        Pages[i] = 0;
    DefaultPage = new Page;
}

struct templetterinfo
{
    int letter;
    int a,b,c;
};

void sPainterFont::Init(sSystemFont *sfont,const char *letters)
{
    Advance = sfont->GetAdvance();
    Baseline = sfont->GetBaseline();
    CellHeight = sfont->GetCellHeight();

    // parse letter sting

    ParseLetterInfo li;
    ParseLetterRanges(li,letters);

    // get list of all characters and thier abc values

    sArray<templetterinfo> temp;
    temp.HintSize(li.LetterCount);

    for(auto &lr : li.Ranges)
    {
        for(int c=lr.l0;c<=lr.l1;c++)
        {
            templetterinfo tli;
            tli.letter = c;
            sfont->GetInfo(c,&tli.a);
            temp.Add(tli);
        }
    }

    // get size of texture

    int padding = 1;
    int sx=0,sy=0;
    {
        int linear = 0;
        int widest = 0;
        int chary = sfont->GetCellHeight()+padding*2;

        for(auto &tli : temp)
        {
            linear += tli.b + padding*2;
            widest = sMax(widest,tli.b + padding*2);
        }

        int area = linear * chary;
        int bits = sFindHigherPower(area);

        sx = sMax(1<<((bits)/2),64);
        sx = sMax(sx,sFindHigherPower(widest));

        int cursory = 0;
        int cursorx = 0;
        for(auto &tli : temp)
        {
            if(cursorx + tli.b + padding*2 > sx)
            {
                cursorx = 0;
                cursory += chary;
            }
            if(Pages[tli.letter/CellsPerPage]==0)
            {
                Pages[tli.letter/CellsPerPage] = new Page;
                sClear(*Pages[tli.letter/CellsPerPage]);
            }
            Cell *cell = GetCellX(tli.letter);
            cell->a = int16(tli.a);
            cell->b = int16(tli.b);
            cell->c = int16(tli.c);
            cell->x0 = cursorx + padding;
            cell->y0 = cursory + padding;
            cursorx += tli.b + padding*2;
        }
        sy = 1<<sFindHigherPower(cursory);
        sLogF("util","create font %dx%d , %d lines unused",sx,sy,sy-cursory);
    }

    // fill unused cells with dummy

    {
        Cell DefaultChar = *GetCell('?');
        if(DefaultChar.a+DefaultChar.b+DefaultChar.c==0)
            DefaultChar = *GetCell(' ');
        if(DefaultChar.a+DefaultChar.b+DefaultChar.c==0)
            sFatal("font must contain ' ' or '?'");

        for(int i=0;i<CellsPerPage;i++)
            DefaultPage->Cells[i] = DefaultChar;
        for(int i=0;i<MaxPages;i++)
        {
            if(Pages[i]==0)
            {
                Pages[i] = DefaultPage;
            }
            else
            {
                for(int j=0;j<CellsPerPage;j++)
                {
                    const Cell &cell = Pages[i]->Cells[j];
                    if(cell.a+cell.b+cell.c==0)
                        Pages[i]->Cells[j] = DefaultChar;
                }
            }
        }
    }

    // paint for real

    {
        BitmapSizeX = sx;
        BitmapSizeY = sy;
        Bitmap = new uint8[sx*sy];
        sfont->BeginRender(sx,sy,Bitmap);
        for(auto &tli : temp)
        {
            const Cell *cell = GetCell(tli.letter);
            sfont->RenderChar(tli.letter,cell->x0-cell->a,cell->y0);
        }
        sfont->EndRender();

        MakeImage();
    }
}

void sPainterFont::MakeImage()
{
    sImage *img= new sImage(BitmapSizeX,BitmapSizeY);
    uint *i32 = img->GetData();
    for(int i=0;i<BitmapSizeX*BitmapSizeY;i++)
        i32[i] = (Bitmap[i]<<24) | 0x00ffffff;

    Image = new sPainterImage(Adapter,img,sPIF_AlphaOnly,sRFC_A|sRFB_8|sRFT_UNorm);
    //  Image = new sPainterImage(img,0,sRF_RGBA8888);

    if(0)
    {
        int static n = 0;
        for(int i=0;i<BitmapSizeX*BitmapSizeY;i++)
            i32[i] = Bitmap[i] | (Bitmap[i]<<8) | (Bitmap[i]<<16);
        sString<256> buffer;
        buffer.PrintF("d:/chaos/temp/font_%02d.bmp",n++);
        img->Save(buffer);
    }

    delete img;
}

sPainterFont::~sPainterFont()
{
    delete Image;
    if(Pages)
    {
        for(int i=0;i<MaxPages;i++)
            if(Pages[i]!=DefaultPage)
                delete Pages[i];
        delete[] Pages;
    }
    delete DefaultPage;
    delete[] Bitmap;
}

int sPainterFont::GetWidth(const char *text,uptr len)
{
    if(len==-1) len = sGetStringLen(text);
    const char *str = text;
    const char *end = text+len;
    int x = 0;
    while(str<end)
    {
        int c = sReadUTF8(str);
        int abc[3];
        GetCell(c,abc);
        x += abc[0] + abc[1] + abc[2];
    }
    return x;
}

bool sPainterFont::CheckLetters(const char *text,uptr len)
{
    if(len==-1) len = sGetStringLen(text);
    const char *str = text;
    const char *end = text+len;
    while(str<end)
    {
        int c = sReadUTF8(str);
        if(c!='?')
        {
            const Cell *cell = GetCell(c);
            if(DefaultChar.x0==cell->x0 && DefaultChar.y0 == cell->y0)
                return 0;
        }
    }
    return 1;
}


int sPainterFont::GetWidth(int c)
{
    int abc[3];
    GetCell(c,abc);
    return abc[0] + abc[1] + abc[2];
}

template <class Ser> 
void sPainterFont::Serialize(Ser &s)
{
    int version = s.Begin(sSerId::sPainterFont,2,2);
    if(version)
    {
        s.Check(1024);
        s | Baseline | Advance | CellHeight;
        s | BitmapSizeX | BitmapSizeY;

        DefaultChar.Serialize(s);

        if(s.IsReading())
            for(int i=0;i<CellsPerPage;i++)
                DefaultPage->Cells[i] = DefaultChar;

        int bitmapbytes = BitmapSizeX * BitmapSizeY * sizeof(uint8);
        if(s.IsReading())
        {
            Bitmap = new uint8[bitmapbytes];
        }
        sASSERT(Bitmap);

        s.LargeData(bitmapbytes,Bitmap);
        for(int i=0;i<MaxPages;i++)
        {
            s.Check(sizeof(Page)+1024);
            if(s.If(Pages[i]==DefaultPage))
            {
                if(s.IsReading())
                    Pages[i] = DefaultPage;
            }
            else
            {
                if(s.IsReading())
                    Pages[i] = new Page;
                for(int j=0;j<CellsPerPage;j++)
                    Pages[i]->Cells[j].Serialize(s);
            }
        }
        s.End();

        if(s.IsReading())
            MakeImage();
    }
}

sReader& operator| (sReader &s,sPainterFont &v)  { v.Serialize(s); return s; }
sWriter& operator| (sWriter &s,sPainterFont &v)  { v.Serialize(s); return s; }


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   sPainter                                                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sPainter::sPainter(sAdapter *adapter)
{
    Adapter = adapter;
    Pool = new sMemoryPool();
    Transform = 0;
    ClipRect = 0;
    Scissor = 0;
    Font = 0;

    MtrlFlat = new sFixedMaterial();
    MtrlFlat->SetState(Adapter->CreateRenderState(sRenderStatePara(sMTRL_DepthOff|sMTRL_CullOff,sMB_Alpha)));
    MtrlFlat->Prepare(adapter,Adapter->FormatPC);

    cbv0 = new sCBuffer<sFixedMaterialVC>(Adapter,sST_Vertex,0);
}

sPainter::~sPainter()
{
    while(!Fonts.IsEmpty())
        ReleaseFont(Fonts[0]);
    sASSERT(FontResources.IsEmpty());

    delete cbv0;

    delete MtrlFlat;
    delete Pool;
}

void sPainter::Begin(sTargetPara &tp)
{
    Pool->Reset();
    Commands.Clear();
    Transforms.Clear();
    ClipRects.Clear();
    for(int i=0;i<HashSize;i++)
        CommandHash[i] = 0;
    Transform = 0;
    ClipRect = 0;
    Scissor = 0;

    sMatrix33 mat;
    SetTransform(mat);

    SetClip(sFRect(0,0,0,0));

    FontPara.Font = 0;
    FontPara.SizeX = 1;
    FontPara.SizeY = 1;

    TargetPara = tp;
}

void sPainter::End(sContext *ctx)
{
    Render(ctx);
    Pool->Reset();
    Commands.Clear();
    Transforms.Clear();
}

void sPainter::SetLayer(int n)
{
    Layer = (uint16)sClamp<int>(n+0x1000,0,MaxLayer);
}

int sPainter::GetLayer()
{
    return Layer-0x1000;
}

void sPainter::SetFont(const sPainterFontPara &tp)
{
    FontPara = tp;
    Font = 0;
}

void sPainter::SetFont(sFont *font)
{
    Font = font;
    sClear(FontPara);
}

void sPainter::SetTransform(const sMatrix33 &mat)
{
    sASSERT(Transforms.GetCount()<=MaxTransform);
    Transform = uint16(Transforms.GetCount());
    Transforms.Add(mat);
}

void sPainter::ResetTransform()
{
    Transform = 0;    // this is always a null transform
}

void sPainter::EnableScissor(bool enable)
{
    Scissor = enable;
}

void sPainter::SetClip(const sFRect &r)
{
    if(ClipRect>0 && ClipRects[ClipRect]==r)
        return;
    sASSERT(ClipRects.GetCount()<=MaxClipRect);
    ClipRect = uint16(ClipRects.GetCount());
    ClipRects.Add(r);
}

void sPainter::SetClip(const sRect &r)
{
    SetClip(sFRect(r));
}

void sPainter::ResetClip()
{
    ClipRect = 0;
}

int sPainter::SaveClip()
{
    return ClipRect;
}

void sPainter::RestoreClip(int saved)
{
    ClipRect = saved;
}

bool sPainter::AndClip(const sFRect &r)
{
    if(ClipRect==0)
    {
        SetClip(r);
        return !r.IsEmpty();
    }
    else
    {
        sFRect rr(r);
        rr.And(ClipRects[ClipRect]);
        SetClip(rr);
        return !rr.IsEmpty();
    }
}

bool sPainter::AndClip(const sRect &r)
{
    return AndClip(sFRect(r));
}

/****************************************************************************/
/****************************************************************************/

sFont *sPainter::GetFont(const sFontPara &fp)
{
    sFontResource *res = 0;
    sFont *font = 0;

    // Find fitting font resource

    for(auto fr : FontResources)
    {
        if(fr->Para.Flags != fp.Flags)
            continue;
        if(fr->Para.MaxUnicode != fp.MaxUnicode)
            continue;
        if(fr->Para.Name != fp.Name)
            continue;
        if(!(fp.Flags & sSFF_DistanceField) && fr->Para.Size != fp.Size)
            continue;
        res = fr;
        break;
    }

    // create one if nothing fits

    if(!res)
    {
        res = new sFontResource(Adapter,fp);

        sTextureFontPara para;

        para.Format = sTextureFontPara::I8;
        para.FontName = fp.Name;
        para.FontFlags = fp.Flags;
        if(fp.Flags & sSFF_DistanceField)
        {
            para.SizeY = 128;
            para.Inside = 6;
            para.Outside = 12;
            para.Intra = 12;
            para.FontSize = 0x2000;
        }
        else
        {
            para.SizeY = fp.Size;
            para.Inside = 1.5f;
            para.Outside = 1.5f;
            para.Intra = 0;
            para.FontSize = fp.Size;
        }

        // try to load font

        sString<sMaxPath> file;
        const char *ConfigPath = sGetConfigPath();
        if(ConfigPath)
        {
            int size = (fp.Flags & sSFF_DistanceField) ? 0 : fp.Size;
            file.PrintF("%s/FontCache/%08x_%04x_%04x_%s.bin",ConfigPath,fp.Flags&~sSFF_Multisample,fp.MaxUnicode-1,size,fp.Name);
            if(sCheckFile(file))
            {
                res->TFont = new sTextureFont;
                if(!sLoadObject(res->TFont,file))
                    sDelete(res->TFont);
            }
        }

        // can't load, so create

        if(!res->TFont)
        {
            TexGen.Begin(para);
            for(int i=32;i<fp.MaxUnicode;i++)
                TexGen.AddLetter(i);

            TexGen.AddLetter(0x2022);               // bullet point
            TexGen.AddLetter(0x20ac);               // euro

            res->TFont = TexGen.End();

            // put in font cache

            if(ConfigPath)
                sSaveObject(res->TFont,file);
        }

        // creaete dx resources, no matter if we are from cache or freshly created.
        // this should delete TextureData, but doesn't yet.

        res->TFont->CreateTexture(Adapter);
        res->TFont->CreateMtrl();

        // and add to pool

        FontResources.Add(res);
    }

    // find a fitting font

    for(auto f : Fonts)
    {
        if(f->Font != res)
            continue;
        if((f->Para.Flags & sSFF_Multisample) != (fp.Flags & sSFF_Multisample))
            continue;
        if(f->Para.Zoom != fp.Zoom)
            continue;
        if(f->Para.Blur != fp.Blur)
            continue;
        if(f->Para.Outline != fp.Outline)
            continue;
        if(f->Para.Size != fp.Size)
            continue;
        font = f;
        font->RefCount++;
        break;       
    }

    // create one if not fount

    if(!font)
    {
        res->RefCount++;
        font = new sFont(res,fp);
        Fonts.Add(font);
    }

    // done

    return font;
}

void sPainter::ReleaseFont(sFont *font)
{
    if(font)
    {
        font->RefCount--;
        sASSERT(font->RefCount>=0);
        if(font->RefCount==0)
        {
            Fonts.Rem(font);
            font->Font->RefCount--;
            sASSERT(font->Font->RefCount>=0);
            if(font->Font->RefCount==0)
            {
                FontResources.Rem(font->Font);
                delete font->Font;
            }
            delete font;
        }
    }
}

/****************************************************************************/
/****************************************************************************/

void sPainter::Init(sPainterCommand *cmd,uint8 kind,int tex)
{
    cmd->Transform = Transform;
    cmd->ClipRect = ClipRect;
    cmd->NextHash = 0;
    cmd->NextSort = 0;

    sASSERT(tex>=0 && tex<=MaxTex);
    sASSERT(kind>=0 && kind<=0xf)
        cmd->Sort = (Layer<<16) | (tex<<5) | (Scissor ? 0x10 : 0x00) | (kind);
}

void sPainter::AddCommand(sPainterCommand *pc)
{
    uint hash = pc->Sort;
    hash = hash ^ (hash>>16);
    hash = hash ^ (hash>>8);
    hash = hash & HashMask;

    sPainterCommand *h = CommandHash[hash];
    while(h)
    {
        if(h->Sort==pc->Sort)
        {
            pc->NextSort = h->NextSort;
            h->NextSort = pc;
            return;
        }
        h = h->NextHash;
    }

    pc->NextHash = CommandHash[hash];
    CommandHash[hash] = pc;
    Commands.AddTail(pc);
}

/****************************************************************************/

void sPainter::Line(uint col,const sFRect &r)
{
    if(!ClipRect || ClipRects[ClipRect].IsInside(r))
    {
        sPainterCommandSimple *pc = Pool->Alloc<sPainterCommandSimple>();
        Init(pc,sPC_Line,0x07fe);
        pc->Color = col;
        pc->Rect = r;
        AddCommand(pc);
    }
}

void sPainter::Frame(uint col,const sFRect &r)
{
    if(!ClipRect || ClipRects[ClipRect].IsInside(r))
    {
        sPainterCommandSimple *pc = Pool->Alloc<sPainterCommandSimple>();
        Init(pc,sPC_Frame);
        pc->Color = col;
        pc->Rect = r;
        AddCommand(pc);
    }
}

void sPainter::Rect(uint col,const sFRect &r)
{
    if(!ClipRect || ClipRects[ClipRect].IsInside(r))
    {
        sPainterCommandSimple *pc = Pool->Alloc<sPainterCommandSimple>();
        Init(pc,sPC_Rect);
        pc->Color = col;
        pc->Rect = r;
        AddCommand(pc);
    }
}

void sPainter::RectC(const uint *col,const sFRect &r)
{
    if(!ClipRect || ClipRects[ClipRect].IsInside(r))
    {
        sPainterCommandRectC*pc = Pool->Alloc<sPainterCommandRectC>();
        Init(pc,sPC_RectC);
        pc->Color[0] = col[0];
        pc->Color[1] = col[1];
        pc->Color[2] = col[2];
        pc->Color[3] = col[3];
        pc->Rect = r;
        AddCommand(pc);
    }
}

void sPainter::Image(sPainterImage *img,uint col,const sFRect &src,const sFRect &dest)
{
    if(!ClipRect || ClipRects[ClipRect].IsInside(dest))
    {
        sPainterCommandImage *pc = Pool->Alloc<sPainterCommandImage>();
        Init(pc,sPC_Image,img->TextureId);
        pc->Color = col;
        pc->Image = img;
        pc->SRect = src;
        pc->DRect = dest;
        AddCommand(pc);
    }
}

void sPainter::Image(sPainterImage *img,uint col,const sFRect &src,float dx,float dy)
{
    Image(img,col,src,sFRect(dx,dy,dx+src.SizeX(),dy+src.SizeY()));
}

void sPainter::Image(sPainterImage *img,uint col,float dx,float dy)
{
    sFRect src(0,0,float(img->SizeX),float(img->SizeY));
    sFRect dest(dx,dy,dx+float(img->SizeX),dy+float(img->SizeY));
    Image(img,col,src,dest);
}

void sPainter::Image(sPainterImage *img,int vc,int ic,const sVertexPCT *vp,const uint16 *ip)
{
    sPainterCommandImagePCT *pc = Pool->Alloc<sPainterCommandImagePCT>();
    Init(pc,sPC_ImagePCT,img->TextureId);
    pc->Image = img;
    pc->vc = vc;
    pc->ic = ic;
    pc->vp = Pool->Alloc<sVertexPCT>(vc);
    pc->ip = Pool->Alloc<uint16>(ic);

    sCopyMem(pc->vp,vp,sizeof(sVertexPCT)*vc);
    sCopyMem(pc->ip,ip,sizeof(uint16)*ic);

    AddCommand(pc);
}

void sPainter::Image(sPainterImage *img,int vc,int ic,const sVertexPWT *vp,const uint16 *ip)
{
    sPainterCommandImagePWT *pc = Pool->Alloc<sPainterCommandImagePWT>();
    Init(pc,sPC_ImagePWT,img->TextureId);
    pc->Image = img;
    pc->vc = vc;
    pc->ic = ic;
    pc->vp = Pool->Alloc<sVertexPWT>(vc);
    pc->ip = Pool->Alloc<uint16>(ic);

    sCopyMem(pc->vp,vp,sizeof(sVertexPWT)*vc);
    sCopyMem(pc->ip,ip,sizeof(uint16)*ic);

    AddCommand(pc);
}

void sPainter::Image(sPainterImage *img,int vc,int ic,const sVertexPWCT *vp,const uint16 *ip)
{
    sPainterCommandImagePWCT *pc = Pool->Alloc<sPainterCommandImagePWCT>();
    Init(pc,sPC_ImagePWCT,img->TextureId);
    pc->Image = img;
    pc->vc = vc;
    pc->ic = ic;
    pc->vp = Pool->Alloc<sVertexPWCT>(vc);
    pc->ip = Pool->Alloc<uint16>(ic);

    sCopyMem(pc->vp,vp,sizeof(sVertexPWCT)*vc);
    sCopyMem(pc->ip,ip,sizeof(uint16)*ic);

    AddCommand(pc);
}

void sPainter::Letter(int flags,uint color,float x,float y,int letter)
{
    char *text = Pool->Alloc<char>(8);
    char *d = text;
    sWriteUTF8(d,letter);
    *d++ = 0;

    Text_(flags,color,sFRect(x,y,x,y),text,-1);
}

void sPainter::Text(int flags,uint color,const sFRect &r,const char *stext,uptr len)
{
    int old = SaveClip();

    if(AndClip(r))
        Text_(flags,color,r,stext,len);

    RestoreClip(old);
}

void sPainter::Text(int flags,uint color,float x,float y,const char *text,uptr len)
{
    Text_(flags,color,sFRect(x,y,x,y),text,len);
}

void sPainter::Text_(int flags,uint color,const sFRect &r,const char *stext,uptr len)
{
    if(len==-1) len = sGetStringLen(stext);
    char *text = Pool->Alloc<char>(len+1);
    sCopyMem(text,stext,len);
    text[len] = 0;

    if(FontPara.Font)
    {
        sPainterCommandText *pc = Pool->Alloc<sPainterCommandText>();
        Init(pc,sPC_Text,FontPara.Font->GetImage()->TextureId);
        pc->Flags = flags;  
        pc->Color = color;
        pc->FontPara = FontPara;
        pc->Text = text;
        pc->Length = int(len);
        pc->Rect = r;
        AddCommand(pc);
    }
    else if(Font)
    {
        sPainterCommandText2 *pc = Pool->Alloc<sPainterCommandText2>();
        Init(pc,sPC_Text2,Font->TextureId);
        pc->Flags = flags;  
        pc->Color = color;
        pc->Font = Font;
        pc->Text = text;
        pc->Length = int(len);
        pc->Rect = r;
        AddCommand(pc);
    }
}

/****************************************************************************/

void sPainter::Frame(uint col0,uint col1,const sFRect &r,float w)
{
    if(r.SizeX()>0 && r.SizeY()>0)
    {
        Rect(col0,sFRect(r.x0  ,r.y0  ,r.x1  ,r.y0+w));
        Rect(col0,sFRect(r.x0  ,r.y0+w,r.x0+w,r.y1-w));
        Rect(col1,sFRect(r.x1-w,r.y0+w,r.x1  ,r.y1-w));
        Rect(col1,sFRect(r.x0  ,r.y1-w,r.x1  ,r.y1  ));
    }
}

void sPainter::Frame(uint col0,uint col1,const sRect &r_,int w_)
{
    sFRect r(r_);
    float w = float(w_);
    if(r.SizeX()>0 && r.SizeY()>0)
    {
        Rect(col0,sFRect(r.x0  ,r.y0  ,r.x1  ,r.y0+w));
        Rect(col0,sFRect(r.x0  ,r.y0+w,r.x0+w,r.y1-w));
        Rect(col1,sFRect(r.x1-w,r.y0+w,r.x1  ,r.y1-w));
        Rect(col1,sFRect(r.x0  ,r.y1-w,r.x1  ,r.y1  ));
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Painter Backend                                                    ***/
/***                                                                      ***/
/****************************************************************************/

void sPainter::Render(sContext *ctx)
{
    sAdapter *ada = ctx->Adapter;

    // sort commands


    Commands.QuickSort([](sPainterCommand *a,sPainterCommand *b){return a->Sort<b->Sort;});
    //  sHeapSort(sAll(Commands),sMemPtrLess(&sPainterCommand::Sort));

    // begin painting

    // this discards the vb/ib way to often. use nooverwrite !

    TargetPara.Flags = 0;
    sViewport view;
    view.Mode = sVM_Pixels;
    if(0)
        view.Model.i.x = view.Model.j.y = 0.5f;
    view.Prepare(TargetPara);
    cbv0->Map(ctx);
    cbv0->Data->Set(view);
    cbv0->Unmap(ctx);

    sCBufferBase *ActiveExtraCB = 0;
    sMaterial *ActiveMtrl = 0;
    sFont *ActiveFont = 0;
    sVertexFormat *ActiveFormat = 0;
    int ActiveMode = -1;
    bool ActiveScissor = 0;
    int ic=0;
    uint16 *ip=0;
    int vc=0;
    float o = 0.0f;
    if(sConfigRender==sConfigRenderDX9)
        o = -0.5f;
    union
    {
        sVertexPC *vfp;
        sVertexPCT *vtp;
        sVertexPWT *vpwtp;
        sVertexPWCT *vpwctp;
    };
    const float z = 0.5f;
    union
    {
        sPainterCommand *cmd;
        sPainterCommandSimple *cmds;
        sPainterCommandImage *cmdi;
        sPainterCommandImagePCT *cmdpct;
        sPainterCommandImagePWT *cmdpwt;
        sPainterCommandImagePWCT *cmdpwct;
        sPainterCommandText *cmdt;
        sPainterCommandText2 *cmdt2;
        sPainterCommandRectC *cmdrc;
    };
    int MaxIndex = 0;
    int MaxVertex = 0;
    bool text2 = false;

    for(auto cmd0 : Commands)
    {
        sPainterCommand *cmd1 = cmd0;
        while(cmd1)
        {
            cmd = cmd1;

            int rectcount = 1;
            sMaterial *nextmtrl=0;
            sFont *nextfont=0;
            sVertexFormat *nextformat = 0;
            int nextmode = 0;
            bool nextscissor = cmd->Sort & 0x10 ? 1 : 0;

            switch(cmd->GetKind())
            {
            case sPC_Text:
                rectcount = int(sGetStringLen(((sPainterCommandText *)cmd)->Text));
                nextmtrl = cmdt->FontPara.Font->GetImage()->Mtrl;
                nextformat = Adapter->FormatPCT;
                nextmode = sGMP_Tris|sGMF_Index16;
                break;

            case sPC_Text2:
                rectcount = int(sGetStringLen(((sPainterCommandText2 *)cmd)->Text));
                nextfont = cmdt2->Font;
                nextformat = Adapter->FormatPCT;
                nextmode = sGMP_Tris|sGMF_Index16;
                break;

            case sPC_Rect:
            case sPC_RectC:
                nextmtrl = MtrlFlat;
                nextformat = Adapter->FormatPC;
                nextmode = sGMP_Tris|sGMF_Index16;
                break;

            case sPC_Frame:
                rectcount = 4;
                nextmtrl = MtrlFlat;
                nextformat = Adapter->FormatPC;
                nextmode = sGMP_Tris|sGMF_Index16;
                break;

            case sPC_Line:
                nextmtrl = MtrlFlat;
                nextformat = Adapter->FormatPC;
                nextmode = sGMP_Lines|sGMF_Index16;
                break;

            case sPC_Image:
                nextmtrl = cmdi->Image->Mtrl;
                nextformat = Adapter->FormatPCT;
                nextmode = sGMP_Tris|sGMF_Index16;
                break;

            case sPC_ImagePCT:
                nextmtrl = cmdpwt->Image->Mtrl;
                nextformat = Adapter->FormatPCT;
                nextmode = sGMP_Tris|sGMF_Index16;
                rectcount = sMax(cmdpct->ic/6+1,cmdpct->vc/4+1);
                break;

            case sPC_ImagePWT:
                nextmtrl = cmdpwt->Image->Mtrl;
                nextformat = Adapter->FormatPWT;
                nextmode = sGMP_Tris|sGMF_Index16;
                rectcount = sMax(cmdpwt->ic/6+1,cmdpwt->vc/4+1);
                break;

            case sPC_ImagePWCT:
                nextmtrl = cmdpwct->Image->Mtrl;
                nextformat = Adapter->FormatPWCT;
                nextmode = sGMP_Tris|sGMF_Index16;
                rectcount = sMax(cmdpwct->ic/6+1,cmdpwct->vc/4+1);
                break;

            default:
                sASSERT0();
            }

            if(ActiveMode!=nextmode || ActiveFormat!=nextformat || ActiveScissor!=nextscissor ||
                ActiveMtrl!=nextmtrl || ic+rectcount*6>MaxIndex || vc+rectcount*4>MaxVertex ||
                ActiveFont!=nextfont)
            {
                if(ActiveFont)
                    ActiveFont->Font->TFont->End();
                if(ActiveMtrl)
                {
                    ada->DynamicGeometry->EndVB(vc);
                    ada->DynamicGeometry->EndIB(ic);
                    ada->DynamicGeometry->End(cbv0,ActiveExtraCB);
                }
                ActiveExtraCB = 0;
                ActiveMtrl = 0;
                ActiveFont = 0;
                ActiveFormat = 0;
                ActiveMode = 0;
            }

            sFRect cr(ClipRects[cmd->ClipRect]);

            if(!ActiveMtrl && !ActiveFont)
            {
                ic = vc = 0;
                ActiveFormat = nextformat;
                ActiveMtrl = nextmtrl;
                ActiveFont = nextfont;
                ActiveMode = nextmode;
                ActiveScissor = nextscissor;

                Adapter->ImmediateContext->SetScissor(ActiveScissor,sRect(cr));

                MaxIndex = ada->DynamicGeometry->GetMaxIndex(rectcount*6,2);
                MaxVertex = ada->DynamicGeometry->GetMaxVertex(rectcount*4,ActiveFormat);
                //      sDPrintF("draw %d %d -> %d %d\n",rectcount*6,rectcount*4,MaxIndex,MaxVertex);
                if(ActiveMtrl)
                {
                    ada->DynamicGeometry->Begin(ctx,ActiveFormat,2,ActiveMtrl,MaxVertex,MaxIndex,ActiveMode);
                    ada->DynamicGeometry->BeginVB(&vfp);
                    ada->DynamicGeometry->BeginIB(&ip);
                }
                if(ActiveFont)
                {
                    ActiveFont->PaintPara.MS2SS = view.MS2SS;
                    ActiveFont->Font->TFont->Begin(ActiveFont->PaintPara);
                }
            }

            switch(cmd->GetKind())
            {
            case sPC_Rect:
                if(cmd->ClipRect && !cmds->Rect.IsCompletelyInside(cr))
                {
                    uint col = sVERTEXCOLOR(cmds->Color);
                    sFRect r(cmds->Rect);
                    r.x0 = sMax(r.x0,cr.x0)+o;
                    r.y0 = sMax(r.y0,cr.y0)+o;
                    r.x1 = sMin(r.x1,cr.x1)+o;
                    r.y1 = sMin(r.y1,cr.y1)+o;
                    vfp[0].Init(r.x0,r.y0,z,col);
                    vfp[1].Init(r.x1,r.y0,z,col);
                    vfp[2].Init(r.x1,r.y1,z,col);
                    vfp[3].Init(r.x0,r.y1,z,col);
                }
                else
                {
                    uint col = sVERTEXCOLOR(cmds->Color);
                    vfp[0].Init(cmds->Rect.x0+o,cmds->Rect.y0+o,z,col);
                    vfp[1].Init(cmds->Rect.x1+o,cmds->Rect.y0+o,z,col);
                    vfp[2].Init(cmds->Rect.x1+o,cmds->Rect.y1+o,z,col);
                    vfp[3].Init(cmds->Rect.x0+o,cmds->Rect.y1+o,z,col);
                }
                sQuad(ip,vc+0,0,1,2,3);
                ic+=6;
                vc+=4;
                vfp+=4;
                break;

            case sPC_RectC:
                if(cmd->ClipRect && !cmdrc->Rect.IsCompletelyInside(cr))
                {
                    sFRect r(cmdrc->Rect);
                    r.x0 = sMax(r.x0,cr.x0)+o;
                    r.y0 = sMax(r.y0,cr.y0)+o;
                    r.x1 = sMin(r.x1,cr.x1)+o;
                    r.y1 = sMin(r.y1,cr.y1)+o;
                    vfp[0].Init(r.x0,r.y0,z,sVERTEXCOLOR(cmdrc->Color[0]));
                    vfp[1].Init(r.x1,r.y0,z,sVERTEXCOLOR(cmdrc->Color[1]));
                    vfp[2].Init(r.x1,r.y1,z,sVERTEXCOLOR(cmdrc->Color[3]));
                    vfp[3].Init(r.x0,r.y1,z,sVERTEXCOLOR(cmdrc->Color[2]));
                }
                else
                {
                    vfp[0].Init(cmdrc->Rect.x0+o,cmdrc->Rect.y0+o,z,sVERTEXCOLOR(cmdrc->Color[0]));
                    vfp[1].Init(cmdrc->Rect.x1+o,cmdrc->Rect.y0+o,z,sVERTEXCOLOR(cmdrc->Color[1]));
                    vfp[2].Init(cmdrc->Rect.x1+o,cmdrc->Rect.y1+o,z,sVERTEXCOLOR(cmdrc->Color[3]));
                    vfp[3].Init(cmdrc->Rect.x0+o,cmdrc->Rect.y1+o,z,sVERTEXCOLOR(cmdrc->Color[2]));
                }
                sQuad(ip,vc+0,0,1,2,3);
                ic+=6;
                vc+=4;
                vfp+=4;
                break;

            case sPC_Frame:
                if(cmd->ClipRect && !cmds->Rect.IsCompletelyInside(cr))
                {
                    uint col = sVERTEXCOLOR(cmds->Color);
                    sFRect rr(cmds->Rect);
                    sFRect r;

                    r.x0 = sMax(rr.x0,cr.x0)+o;
                    r.y0 = sMax(rr.y0,cr.y0)+o;
                    r.x1 = sMin(rr.x1,cr.x1)+o;
                    r.y1 = sMin(rr.y1,cr.y1)+o;
                    vfp[ 0].Init(r.x0,r.y0,z,col);
                    vfp[ 1].Init(r.x1,r.y0,z,col);
                    vfp[ 2].Init(r.x1,r.y1,z,col);
                    vfp[ 3].Init(r.x0,r.y1,z,col);

                    rr.y0 += 1.0f;
                    rr.y1 -= 1.0f;
                    r.x0 = sMax(rr.x0,cr.x0)+o;
                    r.y0 = sMax(rr.y0,cr.y0)+o;
                    r.x1 = sMin(rr.x1,cr.x1)+o;
                    r.y1 = sMin(rr.y1,cr.y1)+o;
                    vfp[ 4].Init(r.x0,r.y0,z,col);
                    vfp[ 5].Init(r.x1,r.y0,z,col);
                    vfp[ 6].Init(r.x1,r.y1,z,col);
                    vfp[ 7].Init(r.x0,r.y1,z,col);

                    rr.x0 += 1.0f;
                    rr.x1 -= 1.0f;
                    r.x0 = sMax(rr.x0,cr.x0)+o;
                    r.y0 = sMax(rr.y0,cr.y0)+o;
                    r.x1 = sMin(rr.x1,cr.x1)+o;
                    r.y1 = sMin(rr.y1,cr.y1)+o;
                    vfp[ 8].Init(r.x0,r.y0,z,col);
                    vfp[ 9].Init(r.x1,r.y0,z,col);
                    vfp[10].Init(r.x1,r.y1,z,col);
                    vfp[11].Init(r.x0,r.y1,z,col);
                }
                else
                {
                    uint col = sVERTEXCOLOR(cmds->Color);
                    sFRect r(cmds->Rect);
                    vfp[ 0].Init(r.x0+o,r.y0+o,z,col);
                    vfp[ 1].Init(r.x1+o,r.y0+o,z,col);
                    vfp[ 2].Init(r.x1+o,r.y1+o,z,col);
                    vfp[ 3].Init(r.x0+o,r.y1+o,z,col);
                    r.y0 += 1.0f;
                    r.y1 -= 1.0f;
                    vfp[ 4].Init(r.x0+o,r.y0+o,z,col);
                    vfp[ 5].Init(r.x1+o,r.y0+o,z,col);
                    vfp[ 6].Init(r.x1+o,r.y1+o,z,col);
                    vfp[ 7].Init(r.x0+o,r.y1+o,z,col);
                    r.x0 += 1.0f;
                    r.x1 -= 1.0f;
                    vfp[ 8].Init(r.x0+o,r.y0+o,z,col);
                    vfp[ 9].Init(r.x1+o,r.y0+o,z,col);
                    vfp[10].Init(r.x1+o,r.y1+o,z,col);
                    vfp[11].Init(r.x0+o,r.y1+o,z,col);
                }
                sQuad(ip,vc+0,0,1,5,4);
                sQuad(ip,vc+0,7,6,2,3);
                sQuad(ip,vc+0,4,8,11,7);
                sQuad(ip,vc+0,9,5,6,10);
                ic+=6*4;
                vc+=4*4;
                vfp+=4*4;
                break;

            case sPC_Line:
                {
                    uint col = sVERTEXCOLOR(cmds->Color);
                    vfp[0].Init(cmds->Rect.x0+o,cmds->Rect.y0+o,z,col);
                    vfp[1].Init(cmds->Rect.x1+o,cmds->Rect.y1+o,z,col);
                    ip[0] = vc+0;
                    ip[1] = vc+1;
                    ic+=2;
                    vc+=2;
                    ip+=2;
                    vfp+=2;
                }
                break;

            case sPC_Image:
                ActiveExtraCB = cmdi->Image->ExtraCB;
                if(cmd->ClipRect && !cmdi->DRect.IsCompletelyInside(cr))
                {
                    uint col = sVERTEXCOLOR(cmdi->Color);
                    sFRect r(cmdi->DRect);
                    sFRect uv(cmdi->SRect);
                    uv.Scale(1.0f/cmdi->Image->SizeX,1.0f/cmdi->Image->SizeY);

                    if(r.x0<cr.x0) { uv.x0=uv.x0+(cr.x0-r.x0)*uv.SizeX()/r.SizeX(); r.x0=cr.x0; }
                    if(r.x1>cr.x1) { uv.x1=uv.x1+(cr.x1-r.x1)*uv.SizeX()/r.SizeX(); r.x1=cr.x1; }
                    if(r.y0<cr.y0) { uv.y0=uv.y0+(cr.y0-r.y0)*uv.SizeY()/r.SizeY(); r.y0=cr.y0; }
                    if(r.y1>cr.y1) { uv.y1=uv.y1+(cr.y1-r.y1)*uv.SizeY()/r.SizeY(); r.y1=cr.y1; }

                    vtp[0].Init(r.x0+o,r.y0+o,z,col,uv.x0,uv.y0);
                    vtp[1].Init(r.x1+o,r.y0+o,z,col,uv.x1,uv.y0);
                    vtp[2].Init(r.x1+o,r.y1+o,z,col,uv.x1,uv.y1);
                    vtp[3].Init(r.x0+o,r.y1+o,z,col,uv.x0,uv.y1);
                }
                else
                {
                    uint col = sVERTEXCOLOR(cmdi->Color);
                    sFRect uv(cmdi->SRect);
                    uv.Scale(1.0f/cmdi->Image->SizeX,1.0f/cmdi->Image->SizeY);
                    vtp[0].Init(cmdi->DRect.x0+o,cmdi->DRect.y0+o,z,col,uv.x0,uv.y0);
                    vtp[1].Init(cmdi->DRect.x1+o,cmdi->DRect.y0+o,z,col,uv.x1,uv.y0);
                    vtp[2].Init(cmdi->DRect.x1+o,cmdi->DRect.y1+o,z,col,uv.x1,uv.y1);
                    vtp[3].Init(cmdi->DRect.x0+o,cmdi->DRect.y1+o,z,col,uv.x0,uv.y1);
                }
                sQuad(ip,vc+0,0,1,2,3);
                ic+=6;
                vc+=4;
                vtp+=4;
                break;

            case sPC_ImagePCT:
                ActiveExtraCB = cmdpwt->Image->ExtraCB;
                sASSERT(cmdpct->ic>0 && cmdpct->vc>0);
                sCopyMem(vtp,cmdpct->vp,sizeof(sVertexPCT)*cmdpct->vc);
                for(int i=0;i<cmdpct->ic;i++)
                    ip[i] = cmdpct->ip[i] + vc;
                vc+=cmdpct->vc;
                vtp+=cmdpct->vc;
                ic+=cmdpct->ic;
                ip+=cmdpct->ic;
                break;


            case sPC_ImagePWT:
                ActiveExtraCB = cmdpwt->Image->ExtraCB;
                sASSERT(cmdpwt->ic>0 && cmdpwt->vc>0);
                sCopyMem(vpwtp,cmdpwt->vp,sizeof(sVertexPWT)*cmdpwt->vc);
                for(int i=0;i<cmdpwt->ic;i++)
                    ip[i] = cmdpwt->ip[i] + vc;
                vc+=cmdpwt->vc;
                vpwtp+=cmdpwt->vc;
                ic+=cmdpwt->ic;
                ip+=cmdpwt->ic;
                break;

            case sPC_ImagePWCT:
                ActiveExtraCB = cmdpwct->Image->ExtraCB;
                sASSERT(cmdpwct->ic>0 && cmdpwct->vc>0);
                sCopyMem(vpwctp,cmdpwct->vp,sizeof(sVertexPWCT)*cmdpwct->vc);
                for(int i=0;i<cmdpwct->ic;i++)
                    ip[i] = cmdpwct->ip[i] + vc;
                vc+=cmdpwct->vc;
                vpwctp+=cmdpwct->vc;
                ic+=cmdpwct->ic;
                ip+=cmdpwct->ic;
                break;

            case sPC_Text2:
                cmdt2->Font->Font->TFont->Print(cmdt2->Flags,cmdt2->Rect,&cr,cmdt2->Color,cmdt2->Text,cmdt2->Length);
                break;

            case sPC_Text:
                ActiveExtraCB = cmdt->FontPara.Font->GetImage()->ExtraCB;
                if(!(cmdt->Flags & (sPPF_Downwards|sPPF_Upwards)))
                {
                    sFRect rect = cmdt->Rect;
                    float cx = 0;
                    float cy = 0;
                    float scalex = 1.0f/cmdt->FontPara.Font->GetImage()->SizeX;
                    float scaley = 1.0f/cmdt->FontPara.Font->GetImage()->SizeY;
                    int cellheight = cmdt->FontPara.Font->CellHeight;
                    if(cmdt->Flags & sPPF_Space)
                    {
                        rect.x0 += cmdt->FontPara.Font->CellHeight/2;
                        rect.x1 -= cmdt->FontPara.Font->CellHeight/2;
                    }
                    if(!(cmdt->Flags & sPPF_Left))
                    {
                        int width = 0;
                        const char *str = cmdt->Text;
                        const char *end = cmdt->Text+cmdt->Length;
                        while(str<end)
                        {
                            int c = sReadUTF8(str);
                            const sPainterFont::Cell *cell = cmdt->FontPara.Font->GetCell(c);
                            width += cell->a+cell->b+cell->c;
                        }
                        if(cmdt->Flags & sPPF_Right)
                            cx = rect.x1-width;
                        else
                            cx = rect.CenterX()-width/2;
                    }
                    else
                    {
                        cx = rect.x0;
                    }
                    if(cmdt->Flags & sPPF_Top)
                    {
                        cy = rect.y0;
                    }
                    else if(cmdt->Flags & sPPF_Bottom)
                    {
                        cy = rect.y1 - cmdt->FontPara.Font->CellHeight;
                    }
                    else
                    {
                        cy = rect.CenterY() - cmdt->FontPara.Font->CellHeight/2;
                    }

                    if(cmdt->Flags & sPPF_Baseline)
                        cy -= cmdt->FontPara.Font->Baseline;

                    if(cmdt->Flags & sPPF_Integer)
                    {
                        cx = sRoundDown(cx+0.5f);
                        cy = sRoundDown(cy+0.5f);
                    }

                    if(cmd->ClipRect)
                    {
                        const char *str = cmdt->Text;
                        const char *end = cmdt->Text+cmdt->Length;
                        uint col = sVERTEXCOLOR(cmdt->Color);
                        while(str<end)
                        {
                            int c = sReadUTF8(str);
                            const sPainterFont::Cell *cell = cmdt->FontPara.Font->GetCell(c);

                            sFRect uv,r;
                            cx += cell->a;
                            uv.x0 = cell->x0*scalex;
                            uv.y0 = cell->y0*scaley;
                            uv.x1 = (cell->x0+cell->b)*scalex;
                            uv.y1 = (cell->y0+cellheight)*scaley;
                            r.x0 = cx;
                            r.y0 = cy;
                            r.x1 = cx+cell->b;
                            r.y1 = cy+cellheight;
                            cx += cell->b;
                            cx += cell->c;

                            if(r.IsPartiallyInside(cr))
                            {
                                if(r.IsCompletelyInside(cr))
                                {
                                    r.x0 += o; r.y0 += o;
                                    r.x1 += o; r.y1 += o;

                                    vtp[0].Init(r.x0,r.y0,z,col,uv.x0,uv.y0);
                                    vtp[1].Init(r.x1,r.y0,z,col,uv.x1,uv.y0);
                                    vtp[2].Init(r.x1,r.y1,z,col,uv.x1,uv.y1);
                                    vtp[3].Init(r.x0,r.y1,z,col,uv.x0,uv.y1);
                                }
                                else
                                {
                                    if(r.x0<cr.x0) { uv.x0=uv.x0+(cr.x0-r.x0)*uv.SizeX()/r.SizeX(); r.x0=cr.x0; }
                                    if(r.x1>cr.x1) { uv.x1=uv.x1+(cr.x1-r.x1)*uv.SizeX()/r.SizeX(); r.x1=cr.x1; }
                                    if(r.y0<cr.y0) { uv.y0=uv.y0+(cr.y0-r.y0)*uv.SizeY()/r.SizeY(); r.y0=cr.y0; }
                                    if(r.y1>cr.y1) { uv.y1=uv.y1+(cr.y1-r.y1)*uv.SizeY()/r.SizeY(); r.y1=cr.y1; }

                                    r.x0 += o; r.y0 += o;
                                    r.x1 += o; r.y1 += o;

                                    vtp[0].Init(r.x0,r.y0,z,col,uv.x0,uv.y0);
                                    vtp[1].Init(r.x1,r.y0,z,col,uv.x1,uv.y0);
                                    vtp[2].Init(r.x1,r.y1,z,col,uv.x1,uv.y1);
                                    vtp[3].Init(r.x0,r.y1,z,col,uv.x0,uv.y1);
                                }
                                sQuad(ip,vc+0,0,1,2,3);
                                ic+=6;
                                vc+=4;
                                vtp+=4;
                            }
                        }
                    }
                    else
                    {
                        const char *str = cmdt->Text;
                        const char *end = cmdt->Text+cmdt->Length;
                        uint col = sVERTEXCOLOR(cmdt->Color);
                        while(str<end)
                        {
                            int c = sReadUTF8(str);
                            const sPainterFont::Cell *cell = cmdt->FontPara.Font->GetCell(c);

                            sFRect uv,r;
                            cx += cell->a;
                            uv.x0 = cell->x0*scalex;
                            uv.y0 = cell->y0*scaley;
                            uv.x1 = (cell->x0+cell->b)*scalex;
                            uv.y1 = (cell->y0+cellheight)*scaley;
                            r.x0 = cx+o;
                            r.y0 = cy+o;
                            r.x1 = cx+o+cell->b;
                            r.y1 = cy+o+cellheight;
                            cx += cell->b;
                            cx += cell->c;


                            vtp[0].Init(r.x0,r.y0,z,col,uv.x0,uv.y0);
                            vtp[1].Init(r.x1,r.y0,z,col,uv.x1,uv.y0);
                            vtp[2].Init(r.x1,r.y1,z,col,uv.x1,uv.y1);
                            vtp[3].Init(r.x0,r.y1,z,col,uv.x0,uv.y1);
                            sQuad(ip,vc+0,0,1,2,3);
                            ic+=6;
                            vc+=4;
                            vtp+=4;
                        }
                    }
                }
                else      // downwards...
                {
                    sFRect rect = cmdt->Rect;
                    float cx = 0;
                    float cy = 0;
                    float scalex = 1.0f/cmdt->FontPara.Font->GetImage()->SizeX;
                    float scaley = 1.0f/cmdt->FontPara.Font->GetImage()->SizeY;
                    int cellheight = cmdt->FontPara.Font->CellHeight;
                    uint col = sVERTEXCOLOR(cmdt->Color);

                    int width = 0;
                    for(int i=0;i<cmdt->Length;i++)
                    {
                        const sPainterFont::Cell *cell = cmdt->FontPara.Font->GetCell(cmdt->Text[i]);
                        width += cell->a+cell->b+cell->c;
                    }

                    if(cmdt->Flags & sPPF_Space)
                    {
                        rect.y0 += cmdt->FontPara.Font->CellHeight/2;
                        rect.y1 -= cmdt->FontPara.Font->CellHeight/2;
                    }
                    if(!(cmdt->Flags & sPPF_Left))
                    {
                        if(cmdt->Flags & sPPF_Right)
                            cy = rect.y1-width;
                        else
                            cy = rect.CenterY()-width/2;
                    }
                    else
                    {
                        cy = rect.y0;
                    }
                    if(cmdt->Flags & sPPF_Top)
                    {
                        cx = rect.x1 - cmdt->FontPara.Font->CellHeight;
                    }
                    else if(cmdt->Flags & sPPF_Bottom)
                    {
                        cx = rect.x0;
                    }
                    else
                    {
                        cx = rect.CenterX() - cmdt->FontPara.Font->CellHeight/2;
                    }

                    if(cmdt->Flags & sPPF_Baseline)
                        cx -= cmdt->FontPara.Font->Baseline;

                    if(cmdt->Flags & sPPF_Integer)
                    {
                        cx = sRoundDown(cx+0.5f);
                        cy = sRoundDown(cy+0.5f);
                    }

                    if(cmdt->Flags & sPPF_Downwards)
                    {
                        if(cmd->ClipRect)
                        {
                            const char *str = cmdt->Text;
                            const char *end = cmdt->Text+cmdt->Length;
                            while(str<end)
                            {
                                int c = sReadUTF8(str);
                                const sPainterFont::Cell *cell = cmdt->FontPara.Font->GetCell(c);

                                sFRect uv,r;
                                cy += cell->a;
                                uv.x0 = cell->x0*scalex;
                                uv.y0 = cell->y0*scaley;
                                uv.x1 = (cell->x0+cell->b)*scalex;
                                uv.y1 = (cell->y0+cellheight)*scaley;
                                r.x0 = cx;
                                r.y0 = cy;
                                r.x1 = cx+cellheight;
                                r.y1 = cy+cell->b;
                                cy += cell->b;
                                cy += cell->c;

                                if(r.IsPartiallyInside(cr))
                                {
                                    if(r.IsCompletelyInside(cr))
                                    {
                                        r.x0 += o; r.y0 += o;
                                        r.x1 += o; r.y1 += o;

                                        vtp[0].Init(r.x0,r.y0,z,col,uv.x0,uv.y1);
                                        vtp[1].Init(r.x1,r.y0,z,col,uv.x0,uv.y0);
                                        vtp[2].Init(r.x1,r.y1,z,col,uv.x1,uv.y0);
                                        vtp[3].Init(r.x0,r.y1,z,col,uv.x1,uv.y1);
                                    }
                                    else
                                    {
                                        if(r.x0<cr.x0) { uv.y1=uv.y1-(cr.x0-r.x0)*uv.SizeY()/r.SizeX(); r.x0=cr.x0; }
                                        if(r.x1>cr.x1) { uv.y0=uv.y0-(cr.x1-r.x1)*uv.SizeY()/r.SizeX(); r.x1=cr.x1; }
                                        if(r.y0<cr.y0) { uv.x0=uv.x0+(cr.y0-r.y0)*uv.SizeX()/r.SizeY(); r.y0=cr.y0; }
                                        if(r.y1>cr.y1) { uv.x1=uv.x1+(cr.y1-r.y1)*uv.SizeX()/r.SizeY(); r.y1=cr.y1; }

                                        r.x0 += o; r.y0 += o;
                                        r.x1 += o; r.y1 += o;

                                        vtp[0].Init(r.x0,r.y0,z,col,uv.x0,uv.y1);
                                        vtp[1].Init(r.x1,r.y0,z,col,uv.x0,uv.y0);
                                        vtp[2].Init(r.x1,r.y1,z,col,uv.x1,uv.y0);
                                        vtp[3].Init(r.x0,r.y1,z,col,uv.x1,uv.y1);
                                    }
                                    sQuad(ip,vc+0,0,1,2,3);
                                    ic+=6;
                                    vc+=4;
                                    vtp+=4;
                                }
                            }
                        }
                        else
                        {
                            const char *str = cmdt->Text;
                            const char *end = cmdt->Text+cmdt->Length;
                            while(str<end)
                            {
                                int c = sReadUTF8(str);
                                const sPainterFont::Cell *cell = cmdt->FontPara.Font->GetCell(c);

                                sFRect uv,r;
                                cy += cell->a;
                                uv.x0 = cell->x0*scalex;
                                uv.y0 = cell->y0*scaley;
                                uv.x1 = (cell->x0+cell->b)*scalex;
                                uv.y1 = (cell->y0+cellheight)*scaley;
                                r.x0 = cx;
                                r.y0 = cy;
                                r.x1 = cx+cellheight;
                                r.y1 = cy+cell->b;
                                cy += cell->b;
                                cy += cell->c;

                                r.x0 += o; r.y0 += o;
                                r.x1 += o; r.y1 += o;

                                vtp[0].Init(r.x0,r.y0,z,col,uv.x0,uv.y1);
                                vtp[1].Init(r.x1,r.y0,z,col,uv.x0,uv.y0);
                                vtp[2].Init(r.x1,r.y1,z,col,uv.x1,uv.y0);
                                vtp[3].Init(r.x0,r.y1,z,col,uv.x1,uv.y1);

                                sQuad(ip,vc+0,0,1,2,3);
                                ic+=6;
                                vc+=4;
                                vtp+=4;
                            }
                        }
                    }
                    else    // upwards
                    {
                        cy += width;
                        if(cmd->ClipRect)
                        {
                            const char *str = cmdt->Text;
                            const char *end = cmdt->Text+cmdt->Length;
                            while(str<end)
                            {
                                int c = sReadUTF8(str);
                                const sPainterFont::Cell *cell = cmdt->FontPara.Font->GetCell(c);

                                sFRect uv,r;
                                cy -= cell->a;
                                uv.x0 = cell->x0*scalex;
                                uv.y0 = cell->y0*scaley;
                                uv.x1 = (cell->x0+cell->b)*scalex;
                                uv.y1 = (cell->y0+cellheight)*scaley;
                                r.x0 = cx;
                                r.y1 = cy;
                                r.x1 = cx+cellheight;
                                cy -= cell->b;
                                r.y0 = cy;
                                cy -= cell->c;

                                if(r.IsPartiallyInside(cr))
                                {
                                    if(r.IsCompletelyInside(cr))
                                    {
                                        r.x0 += o; r.y0 += o;
                                        r.x1 += o; r.y1 += o;

                                        vtp[0].Init(r.x1,r.y1,z,col,uv.x0,uv.y1);
                                        vtp[1].Init(r.x0,r.y1,z,col,uv.x0,uv.y0);
                                        vtp[2].Init(r.x0,r.y0,z,col,uv.x1,uv.y0);
                                        vtp[3].Init(r.x1,r.y0,z,col,uv.x1,uv.y1);
                                    }
                                    else
                                    {
                                        if(r.x0<cr.x0) { uv.y0=uv.y0+(cr.x0-r.x0)*uv.SizeY()/r.SizeX(); r.x0=cr.x0; }
                                        if(r.x1>cr.x1) { uv.y1=uv.y1+(cr.x1-r.x1)*uv.SizeY()/r.SizeX(); r.x1=cr.x1; }
                                        if(r.y0<cr.y0) { uv.x1=uv.x1-(cr.y0-r.y0)*uv.SizeX()/r.SizeY(); r.y0=cr.y0; }
                                        if(r.y1>cr.y1) { uv.x0=uv.x0-(cr.y1-r.y1)*uv.SizeX()/r.SizeY(); r.y1=cr.y1; }

                                        r.x0 += o; r.y0 += o;
                                        r.x1 += o; r.y1 += o;

                                        vtp[0].Init(r.x1,r.y1,z,col,uv.x0,uv.y1);
                                        vtp[1].Init(r.x0,r.y1,z,col,uv.x0,uv.y0);
                                        vtp[2].Init(r.x0,r.y0,z,col,uv.x1,uv.y0);
                                        vtp[3].Init(r.x1,r.y0,z,col,uv.x1,uv.y1);
                                    }
                                    sQuad(ip,vc+0,0,1,2,3);
                                    ic+=6;
                                    vc+=4;
                                    vtp+=4;
                                }
                            }
                        }
                        else
                        {
                            const char *str = cmdt->Text;
                            const char *end = cmdt->Text+cmdt->Length;
                            while(str<end)
                            {
                                int c = sReadUTF8(str);
                                const sPainterFont::Cell *cell = cmdt->FontPara.Font->GetCell(c);

                                sFRect uv,r;
                                cy -= cell->a;
                                uv.x0 = cell->x0*scalex;
                                uv.y0 = cell->y0*scaley;
                                uv.x1 = (cell->x0+cell->b)*scalex;
                                uv.y1 = (cell->y0+cellheight)*scaley;
                                r.x0 = cx;
                                r.y1 = cy;
                                r.x1 = cx+cellheight;
                                cy -= cell->b;
                                r.y0 = cy;
                                cy -= cell->c;

                                r.x0 += o; r.y0 += o;
                                r.x1 += o; r.y1 += o;

                                vtp[0].Init(r.x1,r.y1,z,col,uv.x0,uv.y1);
                                vtp[1].Init(r.x0,r.y1,z,col,uv.x0,uv.y0);
                                vtp[2].Init(r.x0,r.y0,z,col,uv.x1,uv.y0);
                                vtp[3].Init(r.x1,r.y0,z,col,uv.x1,uv.y1);

                                sQuad(ip,vc+0,0,1,2,3);
                                ic+=6;
                                vc+=4;
                                vtp+=4;
                            }
                        }
                    }
                }

                break;

            default:
                sASSERT0();
            }

            cmd1 = cmd1->NextSort;
        }
    }

    if(ActiveMtrl)
    {
        ada->DynamicGeometry->EndVB(vc);
        ada->DynamicGeometry->EndIB(ic);
        ada->DynamicGeometry->End(cbv0,ActiveExtraCB);
    }
    if(ActiveFont)
    {
        ActiveFont->Font->TFont->End();
    }

    // end painting

    ada->DynamicGeometry->Flush();
}


/****************************************************************************/
/***                                                                      ***/
/***   Multiline Text Format Helper                                       ***/
/***                                                                      ***/
/****************************************************************************/

sPainterMultiline::sPainterMultiline()
{
    LastWidth = -1;
    Text = "";
    Font = 0;
}

sPainterMultiline::~sPainterMultiline()
{
}

void sPainterMultiline::Init(const char *text,sFont *font)
{
    Text = text;
    Font = font;
    SizeX = 0;
}

int sPainterMultiline::Layout(int w)
{
    LastWidth = w;
    SizeX = 0;
    const char *t0 = Text;
    Lines.Clear();
    while(1)
    {
        while(*t0==' ' || *t0=='\n') t0++;

        if(*t0==0) break;

        const char *t1 = t0;
        const char *scan = t0;
        //    while(*scan==' ') *scan++;
        while(1)
        {
            t1 = scan;
            if(*scan==0)
                break;
            while(*scan!=0 && *scan!=' ' && *scan!='\n') 
                scan++;
            int wx = Font->GetWidth(t0,scan-t0);
            if(wx>w)
            {
                if(t1==t0)
                    t1 = scan;
                break;
            }
            SizeX = sMax(SizeX,wx);
            const char *fit = scan;
            while(*scan==' ')  
                scan++;
            if(*scan==0 || *scan=='\n')
            {
                t1 = fit;
                break;
            }
        }

        if(t1>t0)
        {
            Line *tl = Lines.Add();
            tl->Start = t0-Text;
            tl->Count = t1-t0;
        }
        t0 = t1;
    }

    return int(Lines.GetCount()*Font->CellHeight);
}

void sPainterMultiline::Print(sPainter *pnt,const sRect &r,uint col,int underline,int align)
{
    pnt->SetFont(Font);
    if(LastWidth!=r.SizeX())
        Layout(r.SizeX());

    int x = r.x0;
    int y = r.y0;
    for(auto &l : Lines)
    {
        if(l.Count>0)
        {
            int w = Font->GetWidth(Text+l.Start,l.Count);
            if(align==1)
                x = r.CenterX() - w/2;
            else if(align==2)
                x = r.x1 - w;
            pnt->Text(sPPF_Left|sPPF_Top,col,x,y,Text+l.Start,l.Count);
            if(underline)
                pnt->Rect(col,sRect(x,y+Font->CellHeight-underline,x+w,y+Font->CellHeight));
        }
        y += Font->CellHeight;
    } 
}


/****************************************************************************/
/***                                                                      ***/
/***   Font System                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sFontPara::sFontPara()
{
    Name = "Arial";
    Size = 16;
    Flags = 0;
    MaxUnicode = 0x100;
    Zoom = 1.0f;
    Blur = 1.0f;
    Outline = 0.0f;
    int Baeline = 0;
}

sFontPara::sFontPara(const char *name,int size,int flags,int maxcode)
{
    Name = name;
    Size = size;
    Flags = flags;
    MaxUnicode = maxcode;
    Zoom = 1.0f;
    Blur = (flags & sSFF_Multisample) ? 0.5f : 1.0f;
    Outline = 0.0f;
    int Baeline = 0;
}

/****************************************************************************/

sFontResource::sFontResource(sAdapter *ada,const sFontPara &fp)
{
    Adapter = ada;
    Para = fp;
    RefCount = 0;           // font resources start with refcount==0. The refcount gets increased when the font resource is set into a font.
    TFont = 0;
}

sFontResource::~sFontResource()
{
    delete TFont;
}

/****************************************************************************/

sFont::sFont(sFontResource *font,const sFontPara &fp)
{
    Font = font;
    Para = fp;
    RefCount = 1;
    PaintPara.Zoom = float(fp.Size) * fp.Zoom;
    PaintPara.Outline = fp.Outline;
    PaintPara.Blur = fp.Blur;
    PaintPara.Multisample = (fp.Flags & sSFF_Multisample) ? 1 : 0;
    TextureId = sPainter::FindBit();

    // due to roundng, Adcance and Baseline are often like 13.00001f

    Advance = (int)sRoundUp(Font->TFont->Advance*fp.Size-0.001f);
    CellHeight = fp.Size;
    Baseline = (int)sRoundUp(Font->TFont->Baseline*fp.Size-0.001f);
}

sFont::~sFont()
{
    sPainter::ClearBit(TextureId);
}

int sFont::GetWidth(int c)
{
    const auto l = Font->TFont->GetLetter(c);
    if(!l)
        return 0;
    return int(0.5f + l->AdvanceX * PaintPara.Zoom * Font->TFont->TexZoom);
}

int sFont::GetWidth(const char *s,uptr len)
{
    return int(0.5f + Font->TFont->GetWidth((float)PaintPara.Zoom,s,len));
}

float sFont::GetWidthF(int c)
{
    const auto l = Font->TFont->GetLetter(c);
    if(!l)
        return 0;
    return l->AdvanceX * PaintPara.Zoom * Font->TFont->TexZoom;
}

float sFont::GetWidthF(const char *s,uptr len)
{
    return Font->TFont->GetWidth((float)PaintPara.Zoom,s,len);
}

bool sFont::CheckLetters(const char *text,uptr len)
{
    if(len==-1) len = sGetStringLen(text);
    const char *str = text;
    const char *end = text+len;
    while(str<end)
    {
        int c = sReadUTF8(str);
        if(c!='?')
        {
            auto const l = Font->TFont->GetLetter(c);
            if(l==0)
                return 0;
        }
    }
    return 1;
}

/****************************************************************************/
/****************************************************************************/
}
