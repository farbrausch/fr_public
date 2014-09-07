/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Painter.hpp"
#include "TextureFont.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***   Using the font                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sTextureFontPrintPara::sTextureFontPrintPara()
{
    Blur = 1.0f;
    Zoom = 1.0f;
    Outline = 0.0f;
    Multisample = false;
    AlphaBlend = false;
    VZoomX = 1.0f;
    VZoomY = 1.0f;
    SpacingX = 0.0f;
    SpacingY = 1.0f;
    IgnorePartialInCheck = false;
    ZOn = false;
}

/****************************************************************************/

sTextureFont::sTextureFont()
{
    Adapter = 0;
    Context = 0;
    Texture = 0;
    cbv0 = 0;
    cbp0 = 0;
    sClear(DistMtrl);
    TexMtrl = 0;

    TexZoom = 1.0f;
    Advance = 0;
    Baseline = 0;
    MaxOuter = 0;
    DistScale = 1.0f;
    DistAdd = 0.0f;
    SizeX = 1;
    SizeY = 1;
    SizeA = 1;
    TextureData = 0;
    TextureDataSize = 0;
    TextureFormat = 0;

    Mul = 1; 
    IMul = 1;
    Sub = 0;

    LetterFinder = 0;

    // init block text to not used
    BlockData.Lines = 0;
    BlockData.BufferSize = 0;
    BlockData.BufferUsage = 0;
}

sTextureFont::~sTextureFont()
{
    delete Texture;
    delete cbv0;
    delete cbp0;

	for (int i=0;i<8;i++)
		delete DistMtrl[i];

    delete TexMtrl;
    sDelete(TextureData);
    if(LetterFinder)
    {
        for(int i=0;i<256;i++)
            delete[] LetterFinder[i];
        delete[] LetterFinder;
    }

    if( BlockData.Lines )
    {
        delete[] BlockData.Lines;
    }
}

void sTextureFont::CreateTexture(sAdapter *ada)
{
    Adapter = ada;
    Context = Adapter->ImmediateContext;

    sASSERT(!Texture);
    int sx = SizeX;
    int sy = SizeY;

    Texture = new sResource(Adapter,sResTexPara(TextureFormat,sx,sy,1),TextureData,TextureDataSize);

    CreateCharTable();
}

void sTextureFont::CreateMtrl()
{
    cbv0 = new sCBuffer<sFixedMaterialVC>(Adapter,sST_Vertex,0);
    cbp0 = new sCBuffer<sDistanceFieldShader_cbps>(Adapter,sST_Pixel,0);

    if(CreationFlags & sSFF_DistanceField)
    {
        for(int i=0;i<8;i++)
        {
			bool alpha = (i&3)<2;
			int df = i>3 ? sMTRL_DepthOn:sMTRL_DepthOff;
            DistMtrl[i] = new sMaterial(Adapter);
            DistMtrl[i]->SetShaders(sDistanceFieldShader.Get(i&3));
            DistMtrl[i]->SetTexturePS(0,Texture,sSamplerStatePara(sTF_Tile|sTF_Linear));			
            DistMtrl[i]->SetState(sRenderStatePara(sMTRL_CullOff|df,alpha ? sMB_PMAlpha:sMB_Alpha));
            DistMtrl[i]->Prepare(Adapter->FormatPCT);
        }
    }
    else
    {
        TexMtrl = new sFixedMaterial(Adapter);
        TexMtrl->SetTexturePS(0,Texture,sSamplerStatePara(sTF_Tile|sTF_Linear));
        TexMtrl->SetState(sRenderStatePara(sMTRL_CullOff|sMTRL_DepthOff,sMB_PMAlpha));
        TexMtrl->FMFlags = sFMF_TexRedAlpha;
        TexMtrl->Prepare(Adapter->FormatPCT);
    }
}

void sTextureFont::CreateCharTable()
{
    sASSERT(!LetterFinder);

    LetterFinder = new sTextureFontLetter **[256];
    for(int i=0;i<256;i++)
        LetterFinder[i] = 0;
    
    for(auto &l : Letters)
    {
        int lo = l.Code & 255;
        int hi = (l.Code>>8) & 255;
        if(LetterFinder[hi]==0)
        {
            LetterFinder[hi] = new sTextureFontLetter *[256];
            for(int i=0;i<256;i++)
                LetterFinder[hi][i] = 0;
        }
        LetterFinder[hi][lo] = &l;
    }

    KernPairs.InsertionSort([](const sTextureFontKernPair &a,const sTextureFontKernPair &b){ return a.CodeA < b.CodeA; });

    for(auto &l : Letters)
    {
        l.FirstKern = KernPairs.GetCount();
        for(int i=0;i<KernPairs.GetCount();i++)
        {
            if(KernPairs[i].CodeA == l.Code)
            {
                l.FirstKern = i;
                break;
            }
        }
    }
}



const sTextureFontLetter* sTextureFont::GetLetter(int code) const 
{
    int lo = code & 255;
    int hi = (code>>8) & 255;
    if(LetterFinder[hi] && LetterFinder[hi][lo])
        return LetterFinder[hi][lo];

    return 0;
}

float sTextureFont::GetKerning(int codea,int codeb) const
{
    auto l = GetLetter(codea);
    if(l)
    {
        for(int i=l->FirstKern;i<KernPairs.GetCount() && KernPairs[i].CodeA==codea;i++)
        {
            if(KernPairs[i].CodeB==codeb)
                return KernPairs[i].Kern;
        }
    }
    return 0;
}

float sTextureFont::GetWidth(float size,const char *text,uptr len)
{
    float sx = 0;
    if(len==~0)
        len = sGetStringLen(text);
    const char *end = text+len;
    size *= TexZoom;

    while(text<end)
    {
        auto l = GetLetter(sReadUTF8(text));
        if(l)
            sx += size * l->AdvanceX;
    }
    return sx;
}

/****************************************************************************/

void sTextureFont::Print(float x,float y,const sTextureFontPrintPara &para,uint color,const char *text,uptr len)
{
    Begin(para);
    Print(0,x,y,0,color,text,len);
    End();
}

void sTextureFont::Begin(const sTextureFontPrintPara &para)
{

    float outline = para.Outline;
    float blur = para.Blur;
    Zoom = para.Zoom * TexZoom;
    VZoomX = para.VZoomX;
    VZoomY = para.VZoomY;
    SpacingX = para.SpacingX;
    SpacingY = para.SpacingY;
    IgnorePartialInCheck = para.IgnorePartialInCheck;	
    PaintSize = para.Zoom;

    float limit = (MaxOuter-0.5f)*Zoom;
    if(blur>limit*2)
        blur = limit*2;
    if(outline+blur*0.5f > limit)
        outline = limit-blur*0.5f;

    float iblur = blur>0 ? 1.0f/blur : 1e30f;

    float sub = Sub - IMul/Zoom*(outline+blur*0.5f);
    float mul = Mul*Zoom*iblur;

    ScaleX = 1.0f/float(SizeX);
    ScaleY = 1.0f/float(SizeY);
    Factor = sMax(1.0f,(1.0f+outline+blur*0.5f)/Zoom);

    cbv0->Map();
    cbv0->Data->MS2SS = para.MS2SS;
    cbv0->Unmap();
    if(CreationFlags & sSFF_DistanceField)
    {
        int ix = (para.Multisample?1:0)+(para.AlphaBlend?2:0)+(para.ZOn?4:0);
        cbp0->Map();
        cbp0->Data->Para.Set(sub,mul,-ScaleX/Zoom*0.25f,-ScaleY/Zoom*0.25f);
        cbp0->Unmap();
		Mtrl = DistMtrl[ix];		
    }
    else
    {
        Mtrl = TexMtrl;
    }

    int LastChar = 0;
    VertexCount = 0;

    auto dg = Adapter->DynamicGeometry;
    VertexMax = sMin(dg->GetMaxVertex(4096,Adapter->FormatPCT),0x10000);
    dg->Begin(Adapter->FormatPCT,2,Mtrl,VertexMax,0,sGMP_Quads);
    dg->BeginVB(&VertexPtr);
}

void sTextureFont::RestartGeometry(int vc)
{
    if(VertexCount+vc>VertexMax)
    {
        auto dg = Adapter->DynamicGeometry;
        dg->EndVB(VertexCount);
        dg->End(cbv0,cbp0);

        VertexMax = sMin(dg->GetMaxVertex(4096,Adapter->FormatPCT),0x10000);
        dg->Begin(Adapter->FormatPCT,2,Mtrl,VertexMax,0,sGMP_Quads);
        dg->BeginVB(&VertexPtr);
    }
}

float sTextureFont::CalcWordLength( const char* text, const char* end )
{
    float d = 0.0f;

    if( 0 == text ) return d;

    int last = 0;
    
    while( text < end )
    {
        int code = sReadUTF8( text );
        
        if( ( code == 0x20 ) || ( code == 0xa ) ) break;

        auto l = GetLetter( code );
        if(l)
            d += l->AdvanceX;
        
        //d += GetKerning( last, code ) + SpacingX;

        last = code;
    }

    return d * Zoom;
}

float sTextureFont::CalcWordLength( const char* text, const char* end, const char*& newText, uptr& len )
{
    float d = 0.0f;

    if( 0 == text ) return d;

    int last = 0;
    len = 0;
    
    while( text < end )
    {
        const char* tmp = text;
        int code = sReadUTF8( tmp );
        
        if( ( code == 0x20 ) || ( code == 0xa ) ) break;

        auto li = GetLetter( code );
        if(li)
        {
            /*float x0 = (li->OffsetX - Factor) * Zoom * VZoomX;
            float x1 = x0 + (li->Cell.SizeX() + 2 * Factor) * Zoom * VZoomX;
            d += x1 - x0;*/

            d += li->AdvanceX + ( SpacingX  * Zoom );
            
            ++len;
        }
        text = tmp;
        
        //d += GetKerning( last, code ) + SpacingX;

        last = code;
    }

    newText = text;

    return d;
}

void sTextureFont::CalcArea( int flags, float x, float y, sFRect *clip, const char *text, uptr len, sFRect &area )
{
    float ox = x;
    float maxx = 0.0f;
    if(len==~0)
        len = sGetStringLen(text);
    const char *end = text+len;
    int cur = 0;
    bool newword = true;

    area.x0 = x;
    area.y0 = y;

    while(text<end)
    {
        // word wrap
        if( ( flags & sPPF_Wrap ) && clip )
        {
            if( newword )
            {
                newword = false;
                float d = CalcWordLength( text, end );

                if( ( x + d ) >= ( clip->x1 - clip->x0 ) )
                {
                    auto li = GetLetter('W');
                    y -= (li->OffsetY - Factor)*Zoom*SpacingY;
                    x = ox;
                }
            }
        }

        int code = sReadUTF8(text);
        if(code>0x20)
        {
            float kern = GetKerning(LastChar,code);
			x += kern*Zoom+SpacingX*Zoom;

            sFRect screen,uv;
            auto li = GetLetter(code);
            if(li)
            {
				x += li->AdvanceX*Zoom;
                if( x > maxx ) maxx = x;
            }
        }
        else if(code==0x20)
        {
            newword = true;
            auto li = GetLetter(code);
            if(li)
            {
                x += li->AdvanceX*Zoom;
                if( x > maxx ) maxx = x;
            }
        }
		else if(code==0xa)
		{
            newword = true;
			auto li = GetLetter('W');
			y -= (li->OffsetY - Factor)*Zoom*SpacingY;
			x = ox;
		}

        LastChar = code;
    }

    {
        auto li = GetLetter('W');
        y -= (li->OffsetY - Factor)*Zoom*SpacingY;
        area.y1 = y;
    }

    if( clip )
    {
        area.x1 = clip->x1;
    }
    else
    {
        area.x1 = maxx;
    }
}

void sTextureFont::Print(int flags, float x,float y,sFRect *clip,uint color,const char *text,uptr len,uint* cols,int numcols)
{
    float ox = x;
    if(len==~0)
        len = sGetStringLen(text);
    const char *end = text+len;
    int cur = 0;
    bool newword = true;

    RestartGeometry(int(len*4));

    while(text<end)
    {
        // word wrap
        if( ( flags & sPPF_Wrap ) && clip )
        {
            if( newword )
            {
                newword = false;
                float d = CalcWordLength( text, end );

                if( ( x + d ) >= clip->x1 )
                {
                    auto li = GetLetter('W');
                    y -= (li->OffsetY - Factor)*Zoom*SpacingY;
                    x = ox;
                }
            }
        }

        int code = sReadUTF8(text);
        if(code>0x20)
        {
            float kern = GetKerning(LastChar,code);
            x += kern*Zoom+SpacingX*Zoom;

            sFRect screen,uv;
            auto li = GetLetter(code);
            if(li)
            {
                screen.x0 = x + (li->OffsetX - Factor)*Zoom;
                screen.y0 = y + (li->OffsetY - Factor)*Zoom;
                screen.x1 = screen.x0 + (li->Cell.SizeX() + 2*Factor)*Zoom;
                screen.y1 = screen.y0 + (li->Cell.SizeY() + 2*Factor)*Zoom;

                uv.x0 = float(li->Cell.x0-Factor)*ScaleX;
                uv.y0 = float(li->Cell.y0-Factor)*ScaleY;
                uv.x1 = float(li->Cell.x1+Factor)*ScaleX;
                uv.y1 = float(li->Cell.y1+Factor)*ScaleY;

                x += li->AdvanceX*Zoom;

                if(clip)
                {
                    const sFRect &cr = *clip;
                    if(!screen.IsPartiallyInside(cr))
                        goto weiter;
                    if(screen.x0<clip->x0)
                    {
                        uv.x0 = uv.x0+(cr.x0-screen.x0)*uv.SizeX()/screen.SizeX(); 
                        screen.x0 = clip->x0;
                    }
                    if(screen.x1>clip->x1)
                    {
                        uv.x1=uv.x1+(cr.x1-screen.x1)*uv.SizeX()/screen.SizeX();
                        screen.x1 = clip->x1;
                    }
                    if(screen.y0<clip->y0)
                    {
                        uv.y0=uv.y0+(cr.y0-screen.y0)*uv.SizeY()/screen.SizeY();
                        screen.y0 = clip->y0;
                    }
                    if(screen.y1>clip->y1)
                    {
                        uv.y1=uv.y1+(cr.y1-screen.y1)*uv.SizeY()/screen.SizeY();
                        screen.y1 = clip->y1;
                    }
                }

                if( cols && numcols )
                {
                    color = cols[ cur++ % numcols ];
                }

                if(sConfigRender==sConfigRenderDX9)
                    screen.Move(-0.5f,-0.5f);

                VertexPtr[VertexCount+0].Set(screen.x0*VZoomX,screen.y0*VZoomY,0,color,uv.x0,uv.y0);
                VertexPtr[VertexCount+1].Set(screen.x1*VZoomX,screen.y0*VZoomY,0,color,uv.x1,uv.y0);
                VertexPtr[VertexCount+2].Set(screen.x1*VZoomX,screen.y1*VZoomY,0,color,uv.x1,uv.y1);
                VertexPtr[VertexCount+3].Set(screen.x0*VZoomX,screen.y1*VZoomY,0,color,uv.x0,uv.y1);
                VertexCount+=4;

            }
        }
        else if(code==0x20)
        {
            newword = true;
            auto li = GetLetter(code);
            if(li)
                x += li->AdvanceX*Zoom;
        }
		else if(code==0xa)
		{
            newword = true;
			auto li = GetLetter('W');
			y -= (li->OffsetY - Factor)*Zoom*SpacingY;
			x = ox;
		}
    weiter:
        LastChar = code;
    }
}

void sTextureFont::PrintDown(float x,float y,sFRect *clip,uint color,const char *text,uptr len)
{
    if(len==-1)
        len = sGetStringLen(text);
    const char *end = text+len;

    RestartGeometry(int(len*4));

    sSwap(x,y);
	float ox = x;

    while(text<end)
    {
        int code = sReadUTF8(text);
        if(code>0x20)
        {
            float kern = GetKerning(LastChar,code);            
			x += kern*Zoom+SpacingX*Zoom;

            sFRect screen,uv;
            auto li = GetLetter(code);
            if(li)
            {
                screen.x0 = x + (li->OffsetX - Factor)*Zoom;
                screen.y0 = y - (li->OffsetY - Factor)*Zoom;
                screen.x1 = screen.x0 + (li->Cell.SizeX() + 2*Factor)*Zoom;
                screen.y1 = screen.y0 - (li->Cell.SizeY() + 2*Factor)*Zoom;

                uv.x0 = float(li->Cell.x0-Factor)*ScaleX;
                uv.y0 = float(li->Cell.y0-Factor)*ScaleY;
                uv.x1 = float(li->Cell.x1+Factor)*ScaleX;
                uv.y1 = float(li->Cell.y1+Factor)*ScaleY;

                x += li->AdvanceX*Zoom;

                if(sConfigRender==sConfigRenderDX9)
                    screen.Move(-0.5f,-0.5f);

                VertexPtr[VertexCount+0].Set(screen.y1*VZoomX,screen.x0*VZoomY,0,color,uv.x0,uv.y1);
                VertexPtr[VertexCount+1].Set(screen.y1*VZoomX,screen.x1*VZoomY,0,color,uv.x1,uv.y1);
                VertexPtr[VertexCount+2].Set(screen.y0*VZoomX,screen.x1*VZoomY,0,color,uv.x1,uv.y0);
                VertexPtr[VertexCount+3].Set(screen.y0*VZoomX,screen.x0*VZoomY,0,color,uv.x0,uv.y0);
                VertexCount+=4;
            }
        }
        else if(code==0x20)
        {
            auto li = GetLetter(code);
            if(li)
                x += li->AdvanceX*Zoom;
        }
		else if(code==0xa)
		{
			auto li = GetLetter('W');
			y += (li->OffsetY - Factor)*Zoom*SpacingY;
			x = ox;
		}
        LastChar = code;
    }
}

void sTextureFont::PrintUp(float x,float y,sFRect *clip,uint color,const char *text,uptr len)
{
    if(len==-1)
        len = sGetStringLen(text);
    const char *end = text+len;

    RestartGeometry(int(len*4));

    sSwap(x,y);
	float ox = x;

    while(text<end)
    {
        int code = sReadUTF8(text);
        if(code>0x20)
        {
            float kern = GetKerning(LastChar,code);

            sFRect screen,uv;
            auto li = GetLetter(code);
            if(li)
            {                
				x -= kern*Zoom+SpacingX*Zoom;

                screen.x0 = x - (li->OffsetX - Factor)*Zoom;
                screen.y0 = y + (li->OffsetY - Factor)*Zoom;
                screen.x1 = screen.x0 - (li->Cell.SizeX() + 2*Factor)*Zoom;
                screen.y1 = screen.y0 + (li->Cell.SizeY() + 2*Factor)*Zoom;

                x -= li->AdvanceX*Zoom;

                uv.x0 = float(li->Cell.x0-Factor)*ScaleX;
                uv.y0 = float(li->Cell.y0-Factor)*ScaleY;
                uv.x1 = float(li->Cell.x1+Factor)*ScaleX;
                uv.y1 = float(li->Cell.y1+Factor)*ScaleY;

                if(sConfigRender==sConfigRenderDX9)
                    screen.Move(-0.5f,-0.5f);

                VertexPtr[VertexCount+0].Set(screen.y1*VZoomX,screen.x1*VZoomY,0,color,uv.x1,uv.y1);
                VertexPtr[VertexCount+1].Set(screen.y1*VZoomX,screen.x0*VZoomY,0,color,uv.x0,uv.y1);
                VertexPtr[VertexCount+2].Set(screen.y0*VZoomX,screen.x0*VZoomY,0,color,uv.x0,uv.y0);
				VertexPtr[VertexCount+3].Set(screen.y0*VZoomX,screen.x1*VZoomY,0,color,uv.x1,uv.y0);
                VertexCount+=4;
            }
        }
        else if(code==0x20)
        {
            auto li = GetLetter(code);
            if(li)
                x -= li->AdvanceX*Zoom;
        }
		else if(code==0xa)
		{
			auto li = GetLetter('W');
			y -= (li->OffsetY - Factor)*Zoom*SpacingY;
			x = ox;
		}
        LastChar = code;
    }
}

void sTextureFont::Print(int flags,sFRect &client,sFRect *clip,uint color,const char *text,uptr len,uint* cols,int numcols)
{
    if(len==-1)
        len = sGetStringLen(text);

    float x = 0;
    float y = 0;
    float sx = GetWidth(PaintSize,text,len);
    float sy = PaintSize;
    if(flags & (sPPF_Downwards | sPPF_Upwards))
        sSwap(sx,sy);

    // alignment

    if(!(flags & sPPF_Baseline))
    {
        if(flags & sPPF_Top)
        {
            y = client.y0;
            if((flags & sPPF_Space) && (flags & sPPF_Downwards))
                y += PaintSize*0.5f;
        }
        else if(flags & sPPF_Bottom)
        {
            y = client.y1-sy;
            if((flags & sPPF_Space) && (flags & sPPF_Upwards))
                y -= PaintSize*0.5f;
        }
        else
        {
            y = client.CenterY()-sy*0.5f;
        }
    }
    else
    {
        y = client.CenterY();
    }

    if(flags & sPPF_Left)
    {
        x = client.x0;
        if((flags & sPPF_Space) && !(flags & (sPPF_Downwards | sPPF_Upwards)))
            x += PaintSize*0.5f;
    }
    else if(flags & sPPF_Right)
    {
        x = client.x1 - sx;
        if((flags & sPPF_Space) && !(flags & (sPPF_Downwards | sPPF_Upwards)))
            x -= PaintSize*0.5f;
    }
    else
    {
        x = client.CenterX() - sx*0.5f;
    }

    // baseline adjustment and rounding

    float cx = x;                             // clipping need top-left corner
    float cy = y;                             // rendering needs baseline-left corner
    if(flags & sPPF_Downwards)
    {
        if(!(flags & sPPF_Baseline))
            x -= Baseline * PaintSize;
        cx = x + Baseline * PaintSize;
        x += sx;
        if(!(CreationFlags & sSFF_DistanceField))
        {
            x = sRoundDown(x);
            y = sRoundDown(y);
        }
    }
    else if(flags & sPPF_Upwards)
    {
        if(!(flags & sPPF_Baseline))
            x += Baseline * PaintSize;
        cx = x - Baseline * PaintSize;
        y += sy;
        if(!(CreationFlags & sSFF_DistanceField))
        {
            x = sRoundDown(x);
            y = sRoundDown(y);
        }
    }
    else
    {
        if(!(flags & sPPF_Baseline))
            y += Baseline * PaintSize;
        cy = y - Baseline * PaintSize;
        if(!(CreationFlags & sSFF_DistanceField))
        {
            x = sRoundDown(x);
            y = sRoundDown(y);
        }
    }

    // clip & cull

    if(clip)
    {
        sFRect r(cx,cy,cx+sx,cy+sy);
        if(r.IsCompletelyInside(*clip))
            clip=0;
        else if(!IgnorePartialInCheck && !r.IsPartiallyInside(*clip))
            return;
    }

    // render

    if(flags & sPPF_Downwards)
    {
        PrintDown(x,y,clip,color,text,len);
    }
    else if(flags & sPPF_Upwards)
    {
        PrintUp(x,y,clip,color,text,len);
    }
    else
    {
        Print(flags,x,y,clip,color,text,len);
    }
}

#define BLOCK_TEXT_GROW_SIZE        10

void sTextureFont::GrowLineBuffer()
{
    // init alloc
    if( BlockData.Lines == 0 )
    {
        BlockData.BufferSize = BLOCK_TEXT_GROW_SIZE;
        BlockData.Lines = new sTextLine[ BlockData.BufferSize ];
    }
    // grow
    else if( BlockData.BufferUsage == BlockData.BufferSize )
    {
        // new memory
        int newSize = BlockData.BufferSize + BLOCK_TEXT_GROW_SIZE;
        sTextLine* newLines = new sTextLine[ newSize ];

        if( newLines )
        {
            // copy old stuff
            sCopyMem( newLines, BlockData.Lines, sizeof( sTextLine ) * BlockData.BufferSize );

            // get rid of old stuff
            delete[] BlockData.Lines;

            // store new stuff
            BlockData.Lines = newLines;
            BlockData.BufferSize = newSize;
        }
        else
        {
            sDPrint( "sTextureFont::GrowLineBuffer(): Memory allocation failed\n" );
        }
    }
}

float sTextureFont::PrepareBlock( sFRect* area, const char* text, const sTextureFontPrintPara &para  )
{
    // make sure there is a line buffer
    GrowLineBuffer();

    BlockData.Text = text;
    uptr len = sGetStringLen( text );
    const char* end = text + len;
    const char* cursor = text;

    // store last char for kerning
    int LastChar = 0;

    // get first line
    sTextLine* line = BlockData.Lines;
    line->Length = 0;
    line->Width = 0.0f;
    line->StartIndex = 0;
    BlockData.BufferUsage = 1;

    // some preparation
    {
        Zoom = para.Zoom * TexZoom;
        VZoomX = para.VZoomX;
        VZoomY = para.VZoomY;
        SpacingX = para.SpacingX;
        SpacingY = para.SpacingY;
        IgnorePartialInCheck = para.IgnorePartialInCheck;	
        PaintSize = para.Zoom;

        float outline = para.Outline;
        float blur = para.Blur;

        ScaleX = 1.0f/float(SizeX);
        ScaleY = 1.0f/float(SizeY);
        Factor = sMax(1.0f,(1.0f+outline+blur*0.5f)/Zoom);

        auto li = GetLetter( 'W' );
        //Zoom = size / li->AdvanceX;
	    BlockData.LineHeigth = -( li->OffsetY - Factor ) * Zoom;
    }

    do
    {
        // peek at next char
        const char* tmp = cursor;
        int code = sReadUTF8( tmp );

        bool br = false;

        // some visible char
        if( code > 0x20 )
        {
            // calc width for next word
            const char* newCursor;
            uptr wordlen;
            float wordwidth = CalcWordLength( cursor, end, newCursor, wordlen ) * Zoom;

            // fits or has to fit (in case the word is too long for a line)
            if( !line->Length || ( ( line->Width + wordwidth ) < ( area->x1 - area->x0 ) ) )
            {
                line->Width += wordwidth;
                line->Length += (int)wordlen;
                cursor = newCursor;
            }
            // doesn't fit
            else
            {
                // line break
                br = true;
            }
        }
        // space
        else if( code == 0x20 )
        {
            auto li = GetLetter( code );
            if(li)
            {
                sReadUTF8( cursor );
                line->Width += li->AdvanceX * Zoom;
                ++line->Length;
            }
        }
        // line break
		else if( code == 0xa )
		{
			br = true;
            sReadUTF8( cursor );
		}
        // unknwon, skip
        else
        {
            sReadUTF8( cursor );
        }

        if( br )
        {
            GrowLineBuffer();
            line = &BlockData.Lines[ BlockData.BufferUsage ];
            line->Length = 0;
            line->Width = 0.0f;
            line->StartIndex = int(cursor - text);
            
            ++BlockData.BufferUsage;
        }
        

    } while( cursor != end );
 
    BlockData.BlockHeight = BlockData.LineHeigth * BlockData.BufferUsage;

    return BlockData.BlockHeight;
    
}

void sTextureFont::PrintBlock( sFRect* area, float scroll, uint color )
{
    if( !BlockData.Lines )
    {
        return;
    }

    int len = 0;

    int first = (int)( scroll / ( BlockData.LineHeigth * Zoom * SpacingY ) );
    int last = first + (int)( area->y1 / ( BlockData.LineHeigth * Zoom * SpacingY ) + 0.999f );

    // count chars
    for( int l = first; ( l < BlockData.BufferUsage ) && ( l < last ); ++l )
    {
        len += BlockData.Lines[ l ].Length;
    }

    RestartGeometry( len * 4 );

    for( int l = first; ( l < BlockData.BufferUsage ) && ( l < last ); ++l )
    {
        sTextLine* line = &BlockData.Lines[ l ];
        
        const char* text = BlockData.Text + line->StartIndex;
        float x = 0.0f;
        float y = -scroll + ( l + 1 ) * BlockData.LineHeigth * Zoom * SpacingY;
    
        // ???
        Factor = 1.0f;

        for( int i = 0; i < line->Length; ++i )
        {
            int code = sReadUTF8(text);
        
            if( code > 0x20 )
            {
                float kern = GetKerning(LastChar,code);
			    x += kern * Zoom + SpacingX * Zoom;

                sFRect screen, uv;
                auto li = GetLetter(code);
                if(li)
                {
                    screen.x0 = x + (li->OffsetX - Factor)*Zoom;
                    screen.y0 = y + (li->OffsetY - Factor)*Zoom;
                    screen.x1 = screen.x0 + (li->Cell.SizeX() + 2*Factor)*Zoom;
                    screen.y1 = screen.y0 + (li->Cell.SizeY() + 2*Factor)*Zoom;

                    uv.x0 = float(li->Cell.x0-Factor)*ScaleX;
                    uv.y0 = float(li->Cell.y0-Factor)*ScaleY;
                    uv.x1 = float(li->Cell.x1+Factor)*ScaleX;
                    uv.y1 = float(li->Cell.y1+Factor)*ScaleY;

                    sFRect cr = *area;
                    if( screen.IsPartiallyInside( cr ) )
                    {

                        if( screen.x0 < area->x0 )
                        {
                            uv.x0 = uv.x0+ ( area->x0 - screen.x0 ) * uv.SizeX() / screen.SizeX(); 
                            screen.x0 = area->x0;
                        }
                        if( screen.x1 > area->x1 )
                        {
                            uv.x1 = uv.x1 + ( area->x1 - screen.x1 ) * uv.SizeX() / screen.SizeX();
                            screen.x1 = area->x1;
                        }
                        if( screen.y0 < area->y0 )
                        {
                            uv.y0 = uv.y0 + ( area->y0 - screen.y0 ) * uv.SizeY() / screen.SizeY();
                            screen.y0 = area->y0;
                        }
                        if( screen.y1 > area->y1 )
                        {
                            uv.y1 = uv.y1 + ( area->y1 - screen.y1 ) * uv.SizeY() / screen.SizeY();
                            screen.y1 = area->y1;
                        }

                        VertexPtr[VertexCount+0].Set(screen.x0*VZoomX,screen.y0*VZoomY,0,color,uv.x0,uv.y0);
                        VertexPtr[VertexCount+1].Set(screen.x1*VZoomX,screen.y0*VZoomY,0,color,uv.x1,uv.y0);
                        VertexPtr[VertexCount+2].Set(screen.x1*VZoomX,screen.y1*VZoomY,0,color,uv.x1,uv.y1);
                        VertexPtr[VertexCount+3].Set(screen.x0*VZoomX,screen.y1*VZoomY,0,color,uv.x0,uv.y1);
                        VertexCount+=4;

                    }

                    x += li->AdvanceX*Zoom;
                }
            }
            // space
            else if( code == 0x20 )
            {
                auto li = GetLetter( code );
                if(li)
                {
                    x += li->AdvanceX * Zoom;
                }
            }
        }
    }
}

void sTextureFont::End()
{
    Adapter->DynamicGeometry->EndVB(VertexCount);
    Adapter->DynamicGeometry->End(cbv0,cbp0);
}


/****************************************************************************/

template <class Ser> void sTextureFont::Serialize(Ser &s)
{
    int version = s.Begin(sSerId::sTextureFont,1,1);
    if(version>0)
    {
        s.Check(1024);
        s | Advance | Baseline | MaxOuter | DistScale | DistAdd;
        s | SizeX | SizeY | SizeA;
        s | Mul | IMul | Sub | TexZoom | CreationFlags;
        s | TextureDataSize | TextureFormat;

        s.Array(Letters);
        s.Array(KernPairs);

        for(auto &l : Letters)
        {
            s.Check(1024);
            s | l.Code | l.Page | l.Cell | l.OffsetX | l.OffsetY | l.AdvanceX | l.FirstKern;
        }
        for(auto &k : KernPairs)
        {
            s.Check(1024);
            s | k.CodeA | k.CodeB | k.Kern;
        }

        if(s.IsReading())
        {
            sASSERT(!TextureData);
            TextureData = new uint8[TextureDataSize];
        }
        s.LargeData(TextureDataSize,TextureData);
    }
    s.End();
}

sReader& Altona2::operator| (sReader &s,sTextureFont &v) { v.Serialize(s); return s; }
sWriter& Altona2::operator| (sWriter &s,sTextureFont &v) { v.Serialize(s); return s; }

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Font Generator                                                     ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Parameter                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sTextureFontPara::sTextureFontPara()
{
    SizeY = 10;
    Inside = 1;
    Outside = 1;
    Intra = 0;
    MaxSizeX = 0x4000;
    MaxSizeY = 0x4000;
    FontFlags = 0;
    Format = F32;
    Generate = Images;
    FontSize = 0x2000;
}

/****************************************************************************/
/***                                                                      ***/
/***   Helper Functions                                                   ***/
/***                                                                      ***/
/****************************************************************************/

static float IntersectY(const sVector2 &a,const sVector2 &b,float y)
{
    return a.x + (b.x-a.x)*(y-a.y)/(b.y-a.y);
}

/****************************************************************************/
/***                                                                      ***/
/***   new DistGen                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sTextureFontGenerator::DistGen
{
public:
    struct Segment
    {
        sVector2 a,b;
    };
private:
    struct QNode
    {
        Segment **FirstSeg;
        int SegCount;
        sAABBox2 BBox;
        QNode *Nodes[4];
        float TempBBoxDistSq;
    };
    
    sArray<Segment *> Segments[4];
    QNode *Root;

    QNode* MakeQNode(int bin,sMemoryPool &Pool);
    void MakeQTreeR(QNode *,sMemoryPool &Pool);
    static bool Intersects(const sFRect &box,const sVector2 &a,const sVector2 &b);
public:
    DistGen();
    ~DistGen();

    void Set(sOutline *,float zoom,sMemoryPool &Pool);
    void Clear();
    void AddLine(const sVector2 &a,const sVector2 &b,sMemoryPool &Pool);
    void AddBezier(const sBezier2 &bez,float errsq,sMemoryPool &Pool);
    void MakeQTree(sMemoryPool &Pool);
    float FindDist(const sVector2 &p,float bestsq);
    float FindDistSq(const sVector2 &p,float bestsq,const QNode *);

    sArray<Segment *> AllSegments;
    sAABBox2 BBox;
};

/****************************************************************************/

sTextureFontGenerator::DistGen::DistGen()
{
}

sTextureFontGenerator::DistGen::~DistGen()
{
}

void sTextureFontGenerator::DistGen::Clear()
{
    Segments[0].Clear();
    Segments[1].Clear();
    Segments[2].Clear();
    Segments[3].Clear();
    AllSegments.Clear();
    Root = 0;
}

void sTextureFontGenerator::DistGen::Set(sOutline *outline,float zoom,sMemoryPool &Pool)
{
    Clear();
    for(auto c : outline->Contours)
    {
        auto start = c;
        auto seg = start;
        do
        {
            sVector2 a = seg->P0->Pos*zoom;
            sVector2 b = seg->P1->Pos*zoom;
            if(a!=b)
            {
                if(seg->C0 && seg->C1)
                {
                    sVector2 c0 = seg->C0->Pos * zoom;
                    sVector2 c1 = seg->C1->Pos * zoom;
                    sBezier2 bez;
                    bez.c0 = a;
                    bez.c1 = c0;
                    bez.c2 = c1;
                    bez.c3 = b;
                    AddBezier(bez,0.25f*0.25f,Pool);
                }
                else
                {
                    AddLine(a,b,Pool);
                }
            }
            
            seg = seg->Next;
        }
        while(seg!=start);
    }

    MakeQTree(Pool);
}

void sTextureFontGenerator::DistGen::AddLine(const sVector2 &a,const sVector2 &b,sMemoryPool &Pool)
{
    Segment *seg = Pool.Alloc<Segment>();
    seg->a = a;
    seg->b = b;
    AllSegments.Add(seg);
}

void sTextureFontGenerator::DistGen::AddBezier(const sBezier2 &bez,float errsq,sMemoryPool &Pool)
{
    float err1 = sDistSqToLine(bez.c0,bez.c3,bez.c1);
    float err2 = sDistSqToLine(bez.c0,bez.c3,bez.c2);

    if(err1<errsq && err2<errsq)
    {
        AddLine(bez.c0,bez.c3,Pool);
    }
    else
    {
        sBezier2 b0,b1;
        bez.Divide(b0,b1);
        AddBezier(b0,errsq,Pool);
        AddBezier(b1,errsq,Pool);
    }
}

void sTextureFontGenerator::DistGen::MakeQTree(sMemoryPool &Pool)
{
    Segments[0].Clear();
    Segments[0].Add(AllSegments);
    Root = MakeQNode(0,Pool);

    BBox.Clear();
    for(int i=0;i<Root->SegCount;i++)
    {
        Segment *seg = Root->FirstSeg[i];

        BBox.Add(seg->a);
        BBox.Add(seg->b);
    }
    Root->BBox = BBox;

    MakeQTreeR(Root,Pool);
}

sTextureFontGenerator::DistGen::QNode* sTextureFontGenerator::DistGen::MakeQNode(int bin,sMemoryPool &Pool)
{
    QNode *node = Pool.Alloc<QNode>();
    node->SegCount = Segments[bin].GetCount();
    node->FirstSeg = Pool.Alloc<Segment *>(node->SegCount);
    sClear(node->Nodes);
    node->BBox.Clear();

    int n = 0;
    for(auto seg : Segments[bin])
        node->FirstSeg[n++] = seg;

    return node;
}


bool sTextureFontGenerator::DistGen::Intersects(const sFRect &box,const sVector2 &a_,const sVector2 &b_)
{
    sVector2 a(a_);
    sVector2 b(b_);
    while(true)
    {
        int bita = 0;
        int bitb = 0;

        if(a.x < box.x0) bita |= 1; // left
        if(a.x > box.x1) bita |= 2; // right
        if(a.y < box.y0) bita |= 4; // top
        if(a.y > box.y1) bita |= 8; // bottom
        if(b.x < box.x0) bitb |= 1;
        if(b.x > box.x1) bitb |= 2;
        if(b.y < box.y0) bitb |= 4;
        if(b.y > box.y1) bitb |= 8;

        if(!(bita | bitb))         // at least one is in
            return true;
        if(bita & bitb)         // both are out at the same side
            return false;

        // clip one part

        sVector2 c;
        int bit = bita ? bita : bitb;

        if(bit & 1) // left
        {
            c.x = box.x0;
            c.y = a.y + (b.y-a.y) * (box.x0-a.x) / (b.x-a.x);
        }
        else if(bit & 2) // right
        {
            c.x = box.x1;
            c.y = a.y + (b.y-a.y) * (box.x1-a.x) / (b.x-a.x);
        }
        else if(bit & 4) // top
        {
            c.x = a.x + (b.x-a.x) * (box.y0-a.y) / (b.y-a.y);
            c.y = box.y0;
        }
        else if(bit & 8) // bottom
        {
            c.x = a.x + (b.x-a.x) * (box.y1-a.y) / (b.y-a.y);
            c.y = box.y1;
        }

        // and continue

        if(bit==bita)
            a = c;
        else
            b = c;
    }
}

void sTextureFontGenerator::DistGen::MakeQTreeR(sTextureFontGenerator::DistGen::QNode *node,sMemoryPool &Pool)
{
    Segments[0].Clear();
    Segments[1].Clear();
    Segments[2].Clear();
    Segments[3].Clear();
    sFRect boxes[4];

    float cx = (node->BBox.Min.x + node->BBox.Max.x)*0.5f;
    float cy = (node->BBox.Min.y + node->BBox.Max.y)*0.5f;

    boxes[0].Set(node->BBox.Min.x,node->BBox.Min.y,node->BBox.Max.x,node->BBox.Max.y);
    boxes[1] = boxes[0];
    boxes[2] = boxes[0];
    boxes[3] = boxes[0];

    boxes[0].x1 = cx;
    boxes[0].y1 = cy;
    boxes[1].x0 = cx;
    boxes[1].y1 = cy;
    boxes[2].x1 = cx;
    boxes[2].y0 = cy;
    boxes[3].x0 = cx;
    boxes[3].y0 = cy;

    for(int i=0;i<node->SegCount;i++)
    {
        Segment *seg = node->FirstSeg[i];

        for(int j=0;j<4;j++)
            if(Intersects(boxes[j],seg->a,seg->b))
                Segments[j].Add(seg);
    }

    if(node->SegCount>=9)
    {
        if(Segments[0].GetCount() + Segments[1].GetCount() + Segments[2].GetCount() + Segments[3].GetCount() < node->SegCount*2)
        {
            for(int j=0;j<4;j++)
            {
                node->Nodes[j] = MakeQNode(j,Pool);
                node->Nodes[j]->BBox.Min.x = boxes[j].x0;
                node->Nodes[j]->BBox.Min.y = boxes[j].y0;
                node->Nodes[j]->BBox.Max.x = boxes[j].x1;
                node->Nodes[j]->BBox.Max.y = boxes[j].y1;
            }
            for(int j=0;j<4;j++)
                MakeQTreeR(node->Nodes[j],Pool);
        }
    }
}

float sTextureFontGenerator::DistGen::FindDist(const sVector2 &p,float bestsq)
{
    return sSqrt(FindDistSq(p,bestsq,Root));
}

float sTextureFontGenerator::DistGen::FindDistSq(const sVector2 &p,float bestsq,const sTextureFontGenerator::DistGen::QNode *node)
{
    if(node->Nodes[0])
    {
        const QNode *nodes[4];
        int ncount = 0;

        for(int j=0;j<4;j++)
        {
            const auto n = node->Nodes[j];
            float dx = 0;
            float dy = 0;
            if(p.x<n->BBox.Min.x) dx = n->BBox.Min.x-p.x;
            else if(p.x>n->BBox.Max.x) dx = p.x-n->BBox.Max.x;
            if(p.y<n->BBox.Min.y) dy = n->BBox.Min.y-p.y;
            else if(p.y>n->BBox.Max.y) dy = p.y-n->BBox.Max.y;

            float bds = dx*dx + dy*dy;
            if(bds < bestsq)
            {
                n->TempBBoxDistSq = bds;
                nodes[ncount++] = n;
            }
        }

#define SHUFFLE(i,j) if(nodes[i]->TempBBoxDistSq > nodes[j]->TempBBoxDistSq) sSwap(nodes[i],nodes[j]);
        switch(ncount)
        {
        case 2:
            SHUFFLE(0,1);
            break;
        case 3:
            SHUFFLE(1,2);
            SHUFFLE(0,2);
            SHUFFLE(0,1);
            break;
        case 4:
            SHUFFLE(0,1);
            SHUFFLE(2,3);
            SHUFFLE(0,2);
            SHUFFLE(1,3);
            SHUFFLE(1,2);
            break;
        }
#undef SHUFFLE

        for(int j=0;j<ncount;j++)
        {
            auto n = nodes[j];
            if(n->TempBBoxDistSq > bestsq)
                break;
            bestsq = FindDistSq(p,bestsq,n);
        }
        return bestsq;
    }
    else
    {
        for(int i=0;i<node->SegCount;i++)
        {
            Segment *seg = node->FirstSeg[i];

            sVector2 ab = seg->b - seg->a;
            sVector2 ap = p - seg->a;
            float absq = sDot(ab,ab);
            float dun = sCross(ap,ab);
            float dsq = dun*dun/absq;           // distance to line
            if(dsq<bestsq)
            {
                float t = sDot(ap,ab) / absq;   // check endpoints
                if(t<=0)
                {
                    dsq = sDistSquared(p,seg->a);
                    if(dsq<bestsq)
                        bestsq = dsq;
                }
                else if(t>=1)
                {
                    dsq = sDistSquared(p,seg->b);
                    if(dsq<bestsq)
                        bestsq = dsq;
                }
                else
                {
                    bestsq = dsq;
                }
            }
        }
        return bestsq;
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Letter                                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sTextureFontGenerator::Letter
{
public:
    Letter()
    {
        SizeX = 0;
        SizeY = 0;
        Page = 0;
        Outline = 0;
        Dist = 0;
    }

    ~Letter()
    {
        delete Dist;
        delete Outline;
    }

    DistGen *Dist;
    sOutline *Outline;
    int Code;                               // unicode
    int SizeX;                              // width of the Shape, rounded up to full pixels
    int SizeY;                              // height of the Shape, rounded up to full pixels
    int Page;                               // page number
    sRect Pos;                              // position on page
    sFRect BBox;                            // bounding box
};

/****************************************************************************/
/***                                                                      ***/
/***   Font Generator                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sTextureFontGenerator::sTextureFontGenerator()
{
    Font = 0;
    sLoadSystemFontDir(FontDir);
}

sTextureFontGenerator::~sTextureFontGenerator()
{
    sASSERT(!Font);
    Letters.DeleteAll();
    FontDir.DeleteAll();
}

void sTextureFontGenerator::Begin(const sTextureFontPara &para)
{
    Letters.DeleteAll();
    Pool.Reset();

    Para = para;

    const sSystemFontInfo *fi = 0;
    for(auto i : FontDir)
        if(sCmpStringI(i->Name,para.FontName)==0)
            fi = i;

    AAFactor = (Para.FontFlags & sSFF_Antialias) && !(Para.FontFlags & sSFF_DistanceField) ? 4 : 1;
    if(fi)
    {
        Font = sLoadSystemFont(fi,0,Para.FontSize*AAFactor,Para.FontFlags&~sSFF_Antialias);
    }
    else
    {
        fi = sGetSystemFont(sSFI_Serif);
        Font = sLoadSystemFont(fi,0,Para.FontSize*AAFactor,Para.FontFlags&~sSFF_Antialias);
        delete fi;
    }
}

void sTextureFontGenerator::AddLetter(int code)
{
    if(Font->CodeExists(code))
    {
        sString<16> buffer;
        buffer.PrintF("%c",code);
        auto outline = Font->GetOutline(buffer);
        auto letter = new Letter;
        letter->Outline = outline;
        letter->Dist = new DistGen;
        letter->Dist->Set(outline,float(Para.SizeY)/float(Para.FontSize*AAFactor),Pool);
        letter->BBox.x0 = letter->Dist->BBox.Min.x;
        letter->BBox.y0 = letter->Dist->BBox.Min.y;
        letter->BBox.x1 = letter->Dist->BBox.Max.x;
        letter->BBox.y1 = letter->Dist->BBox.Max.y;
        if(!(Para.FontFlags & sSFF_DistanceField))
        {
            letter->BBox.x0 = sRoundDown(letter->BBox.x0);
            letter->BBox.x1 = sRoundUp(letter->BBox.x1);
            letter->BBox.y0 = sRoundDown(letter->BBox.y0);
            letter->BBox.y1 = sRoundUp(letter->BBox.y1);
        }
        letter->SizeX = sMax(1,(int) sRoundUp(letter->BBox.SizeX()));
        letter->SizeY = sMax(1,(int) sRoundUp(letter->BBox.SizeY()));
        letter->Code = code;

        Letters.Add(letter);
    }
}

bool sTextureFontGenerator::Puzzle(int TexSizeX,int TexSizeY)
{
    int x = 0;
    int y0 = 0;
    int y1 = 0;

    int IntOutside = (int)sRoundUp(Para.Outside);

    for(auto l : Letters)
    {
        int sx = l->SizeX+IntOutside*2+Para.Intra;
        int sy = l->SizeY+IntOutside*2+Para.Intra;

        if(sy > y1-y0 || x+sx>TexSizeX)
        {
            y0 = y1;
            y1 = y0+sy;
            x = 0;
            if(y1>TexSizeY)
                return 0;
        }

        l->Pos.x0 = x + IntOutside;
        l->Pos.x1 = l->Pos.x0 + l->SizeX;
        l->Pos.y0 = y0 + IntOutside;
        l->Pos.y1 = l->Pos.y0 + l->SizeY;

        x += sx;
    }

    return 1;
}

sTextureFont *sTextureFontGenerator::End()
{
    // sort by size

    Letters.Sort([](const Letter *a,const Letter *b) { return a->SizeY > b->SizeY; });

    // calculate minimal area and starting point for puzzling

    int IntOutside = (int)sRoundUp(Para.Outside);
    int minarea = 0;
    int minx = 1;
    int miny = 1;
    for(auto l : Letters)
    {
        int sx = l->SizeX+IntOutside*2+Para.Intra;
        int sy = l->SizeY+IntOutside*2+Para.Intra;
        minarea += sx*sy;
        minx = sMax(minx,sx);
        miny = sMax(miny,sy);
    }

    int texsizex = 1<<sFindHigherPower(minx);
    int texsizey = 1<<sFindHigherPower(miny);
    DistX.Init(texsizex,texsizey);
    DistY.Init(texsizey,texsizex);

    // try to puzzle

    while(1)
    {
        if(texsizex * texsizey > minarea)
        {
            if(Puzzle(texsizex,texsizey))
                break;
        }
        if(texsizex <= texsizey)
            texsizex *= 2;
        else
            texsizey *= 2;
    }

    // create font

    sTextureFont *tf = new sTextureFont;
    float zoom = float(Para.SizeY) / float(Para.FontSize);
    
    tf->TexZoom = 1.0f/float(Para.SizeY);
    tf->Advance = float(Font->GetAdvance()) * zoom * tf->TexZoom / float(AAFactor);
    tf->Baseline = float(Font->GetBaseline()) * zoom * tf->TexZoom / float(AAFactor);
    tf->MaxOuter = float(IntOutside);
    tf->DistScale = 1.0f;
    tf->DistAdd = 0.0f;
    tf->SizeX = texsizex;
    tf->SizeY = texsizey;
    tf->SizeA = 1;
    tf->Letters.HintSize(Letters.GetCount());
    tf->CreationFlags = Para.FontFlags;

    switch(Para.Format)
    {
    case sTextureFontPara::I8:
        tf->TextureDataSize = tf->SizeX*tf->SizeY*tf->SizeA*sizeof(uint8);
        tf->TextureFormat = sRFB_8|sRFC_R|sRFT_UNorm;
        break;

    case sTextureFontPara::I16:
        tf->TextureDataSize = tf->SizeX*tf->SizeY*tf->SizeA*sizeof(uint16);
        tf->TextureFormat = sRFB_16|sRFC_R|sRFT_UNorm;
        break;

    case sTextureFontPara::F32:
        tf->TextureDataSize = tf->SizeX*tf->SizeY*tf->SizeA*sizeof(float);
        tf->TextureFormat = sRFB_32|sRFC_R|sRFT_Float;
        break;
    }
    tf->TextureData = new uint8[tf->TextureDataSize];


    // create distance field

    auto img = new sFMonoImage(texsizex,texsizey);
    img->Clear(-1e30f);
    float *dest = img->GetData();
    const float bestsq = (tf->MaxOuter+0.1f) * (tf->MaxOuter+0.1f);

    if(Para.FontFlags & sSFF_DistanceField)
    {
        int time0 = sGetTimeMS();

        for(auto l : Letters)
        {
            // letter info

            int abc[3];
            Font->GetInfo(l->Code,abc);
            sTextureFontLetter tl;
            tl.Code = l->Code;
            tl.Page = 0;
            tl.Cell = l->Pos;
            tl.OffsetX = l->BBox.x0;
            tl.OffsetY = l->BBox.y0 - tf->Baseline / tf->TexZoom;
            tl.AdvanceX = float(abc[0]+abc[1]+abc[2]) * zoom;
            tf->Letters.Add(tl);

            int sx = l->SizeX + IntOutside*2;
            int sy = l->SizeY + IntOutside*2;
            
            // sort segments by y

            l->Dist->AllSegments.Sort([](const DistGen::Segment *a,const DistGen::Segment *b){ return sMin(a->a.y,a->b.y) < sMin(b->a.y,b->b.y); });
                
            // calculate distance field4

            for(int y=0;y<sy;y++)
            {
                float py = float(y)+l->BBox.y0-IntOutside;
                sArray<float> Intersections;

                for(const auto seg : l->Dist->AllSegments)
                {
                    float miny = sMin(seg->a.y,seg->b.y);
                    float maxy = sMax(seg->a.y,seg->b.y);
                    if(py>=miny && py<maxy)
                    {
                        Intersections.Add(IntersectY(seg->a,seg->b,py));
                    }
                }
                sASSERT((Intersections.GetCount() & 1)==0)
                Intersections.Add(-1e30f);
                Intersections.Add(1e30f);
                Intersections.Sort([](float a,float b){ return a<b; });
                int index = 1;
                float sign = -1;

                // scan out line

                for(int x=0;x<sx;x++)
                {
                    sVector2 p(float(x)+l->BBox.x0-IntOutside,
                               float(y)+l->BBox.y0-IntOutside);

                    while(Intersections[index]<=p.x)
                    {
                        index++;
                        sign = -sign;
                    }
                    float mindist = sMin(p.x-Intersections[index-1],Intersections[index]-p.x);
                    float dist = l->Dist->FindDist(p,mindist*mindist);
                    dest[(l->Pos.y0+y-IntOutside)*texsizex + l->Pos.x0+x-IntOutside] = dist*sign;
                }
            }
        }

        int time1 = sGetTimeMS();
        sLogF("util","TextureFont generation tool %s ms",time1-time0);
    }
    else    // no, normal texture font
    {
        uint8 *data8 = new uint8[texsizex*texsizey*AAFactor*AAFactor];
        Font->BeginRender(texsizex*AAFactor,texsizey*AAFactor,data8);

        for(auto l : Letters)
        {
            // letter info

            int abc[3];
            Font->GetInfo(l->Code,abc);
            sTextureFontLetter tl;
            tl.Code = l->Code;
            tl.Page = 0;
            tl.Cell = l->Pos;
            tl.OffsetX = l->BBox.x0;
            tl.OffsetY = l->BBox.y0 - tf->Baseline / tf->TexZoom;
            tl.AdvanceX = sRoundDown(0.5f+(abc[0]+abc[1]+abc[2]) * zoom /AAFactor);
            tf->Letters.Add(tl);

         
            Font->RenderChar(l->Code,(int)(l->Pos.x0-l->BBox.x0)*AAFactor,(int)(l->Pos.y0-l->BBox.y0)*AAFactor);
        }
        Font->EndRender();
        
        if(AAFactor==1)
        {
            for(int y=0;y<texsizey;y++)
            {
                for(int x=0;x<texsizex;x++)
                {
                    float d = float(data8[y*texsizex*AAFactor*AAFactor+x*AAFactor])/255.0f;
                    dest[y*texsizex + x] = d;
                }
            }
        }
        else if(AAFactor==4)
        {
            for(int y=0;y<texsizey;y++)
            {
                for(int x=0;x<texsizex;x++)
                {
                    int d = 0;
                    for(int yy=0;yy<4;yy++)
                        for(int xx=0;xx<4;xx++)
                            d += data8[(y*4+yy)*(texsizex*4) + x*AAFactor + xx];
                    dest[y*texsizex + x] = d/float(255*4*4);
                }
            }
        }
        else
        {
            sASSERT0();
        }
        delete[] data8;
    }

    // get range

    if(tf->CreationFlags & sSFF_DistanceField)
    {
        float min = 1;
        float max = 0;
        for(int i=0;i<texsizex*texsizey;i++)
        {
            if(dest[i]>-1e25)
                min = sMin(dest[i],min);
            max = sMax(dest[i],max);
        }

        min = sMax(min,-Para.Outside);
        max = sMin(max,Para.Inside);
        tf->Mul = (max-min);
        tf->IMul = 1.0f/tf->Mul;
        tf->Sub = -(min)/(max-min);
    }
    else
    {
        tf->Mul = 1.0f;
        tf->IMul = 1.0f;
        tf->Sub = 0.0f;
    }

    // copy to texture

    switch(Para.Format)
    {
    case sTextureFontPara::I8:
        {
            uint8 *data = (uint8 *)tf->TextureData;
            for(int y=0;y<texsizey;y++)
            {
                for(int x=0;x<texsizex;x++)
                {
                    float d = dest[y*texsizex+x] * tf->IMul + tf->Sub;
                    data[y*texsizex+x] = uint8(sClamp(int(d*255),0,255));
                }
            }
        }
        break;

    case sTextureFontPara::I16:
        {
            uint16 *data = (uint16 *)tf->TextureData;
            for(int y=0;y<texsizey;y++)
            {
                for(int x=0;x<texsizex;x++)
                {
                    float d = dest[y*texsizex+x] * tf->IMul + tf->Sub;
                    data[y*texsizex+x] = uint16(sClamp(int(d*0xffff),0,0xffff));
                }
            }
        }
        break;

    case sTextureFontPara::F32:
        {
            float *data = (float *)tf->TextureData;
            for(int y=0;y<texsizey;y++)
            {
                for(int x=0;x<texsizex;x++)
                {
                    float d = dest[y*texsizex+x] * tf->IMul + tf->Sub;
                    data[y*texsizex+x] = d;
                }
            }
        }
        break;
    }

    // get kerning info

    tf->KernPairs.SetSize(Font->KernPairs.GetCount());
    for(int i=0;i<Font->KernPairs.GetCount();i++)
    {
        tf->KernPairs[i].CodeA = Font->KernPairs[i].CodeA;
        tf->KernPairs[i].CodeB = Font->KernPairs[i].CodeB;
        tf->KernPairs[i].Kern = float(Font->KernPairs[i].Kern)*zoom/float(AAFactor);
    }

    // some debugging

    if(0)
    {
        sImage *img= new sImage(texsizex,texsizey);
        uint *i32 = img->GetData();
        int static n = 0;
        for(int i=0;i<texsizex*texsizey;i++)
        {
            int v = sClamp(int(dest[i]*255.0f * tf->IMul + tf->Sub),0,255);
            i32[i] = v | (v<<8) | (v<<16);
        }
        sString<256> buffer;
        buffer.PrintF("c:/chaos/temp/font.bmp",n++);
        img->Save(buffer);
        delete img;

        sDPrintF("    Mul = %f;\n",tf->Mul);
        sDPrintF("    Sub = %f;\n",tf->Sub);
        sDPrintF("    Advance = %f;\n",tf->Advance);
        sDPrintF("    Baseline = %f;\n",tf->Baseline);
        sDPrintF("    MaxOuter = %f;\n",tf->MaxOuter);
        sDPrintF("    DistScale = %f;\n",tf->DistScale);
        sDPrintF("    DistAdd = %f;\n",tf->DistAdd);
        for(auto &l : tf->Letters)
        {
            sDPrintF("    AddLetter('%c', %3d,%3d,%3d,%3d ,%3d,%3d ,%3d);\n",
                l.Code,
                l.Cell.x0,l.Cell.y0,l.Cell.x1,l.Cell.y1,
                l.OffsetX,l.OffsetY,l.AdvanceX);
        }
    }

    // done

    sDelete(Font);
    Pool.Reset();
    delete img;

    return tf;
}

/****************************************************************************/
