/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Gui.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

extern sPainterFont *LoadPainterFont(const char *name,sAdapter *ada);

sGuiStyle_Flat::sGuiStyle_Flat(sGuiAdapter *gada,bool tiny) 
{
    GuiAdapter = gada;
    Adapter = gada->Adapter;
    Painter = gada->Painter;
    Tiny = tiny;

    if(!tiny)
    {
/*
#if sConfigPlatform==sConfigPlatformIOS || sConfigPlatform==sConfigPlatformOSX
        Font = LoadPainterFont("data/save_font.pfont",Adapter);
        SmallFont = LoadPainterFont("data/save_font.pfont",Adapter);
        BoldFont = LoadPainterFont("data/save_font.pfont",Adapter);
        FixedFont = LoadPainterFont("data/save_font.pfont",Adapter);
#else
*/
        sFontPara  fp("Tahoma",16,sSFF_Multisample|sSFF_DistanceField);
        sFontPara sfp("Tahoma",14,sSFF_Multisample|sSFF_DistanceField);
        sFontPara bfp("Tahoma",16,sSFF_Multisample|sSFF_DistanceField|sSFF_Bold);
        sFontPara ffp("Consolas",14,sSFF_Multisample|sSFF_DistanceField);
        
        Font = Painter->GetFont(fp);
        SmallFont = Painter->GetFont(sfp);
        BoldFont = Painter->GetFont(bfp);
        FixedFont = Painter->GetFont(ffp);
//#endif
    }
    else
    {
/*
#if sConfigPlatform==sConfigPlatformIOS || sConfigPlatform==sConfigPlatformOSX
        Font = LoadPainterFont("data/save_font.pfont",Adapter);
        SmallFont = LoadPainterFont("data/save_font.pfont",Adapter);
        BoldFont = LoadPainterFont("data/save_font.pfont",Adapter);
        FixedFont = LoadPainterFont("data/save_font.pfont",Adapter);
#else
*/
        sFontPara  fp("Tahoma",14,sSFF_Multisample|sSFF_DistanceField);
        sFontPara sfp("Tahoma",12,sSFF_Multisample|sSFF_DistanceField);
        sFontPara bfp("Tahoma",14,sSFF_Multisample|sSFF_DistanceField|sSFF_Bold);
        sFontPara ffp("Consolas",14,sSFF_Multisample|sSFF_DistanceField);
        
        Font = Painter->GetFont(fp);
        SmallFont = Painter->GetFont(sfp);
        BoldFont = Painter->GetFont(bfp);
        FixedFont = Painter->GetFont(ffp);
//#endif
    }

    SplitterKnopSize = 5;
    GeneralSize = 1;
    TitleBorderSize = Font->Advance+6;
    BevelBorderSize = 0;

    SetColors();
}

/****************************************************************************/

static void Hilo(sPainter *painter,const sRect &r,uint col0,uint col1,int w=1)
{
    painter->Rect(col0,sRect(r.x0  ,r.y0  ,r.x1  ,r.y0+w));
    painter->Rect(col0,sRect(r.x0  ,r.y0+w,r.x0+w,r.y1-w));
    painter->Rect(col1,sRect(r.x1-w,r.y0+w,r.x1  ,r.y1-w));
    painter->Rect(col1,sRect(r.x0  ,r.y1-w,r.x1  ,r.y1  ));
}

void sGuiStyle_Flat::SetColors()
{
    Colors[sGC_Back             ] = 0xff424142;
    Colors[sGC_Doc              ] = 0xff292929;
    Colors[sGC_Button           ] = 0xff636869;
    Colors[sGC_Text             ] = 0xffffffff;
    Colors[sGC_Draw             ] = 0xff06182d;
    Colors[sGC_Select           ] = 0xff7a85a2;
    Colors[sGC_High             ] = 0xffc3e8f6;
    Colors[sGC_Low              ] = 0xff000000;
    Colors[sGC_High2            ] = 0xff000000;
    Colors[sGC_Low2             ] = 0xff3a3a94;
    Colors[sGC_Cursor           ] = 0xff85e1ff;
    Colors[sGC_Error            ] = 0xffff0000;
    Colors[sGC_InactiveSelect   ] = 0xff5a6070;
    Colors[sGC_HoverFeedback    ] = 0xffffffff;
}

void sGuiStyle_Flat::Rect(int layer,sWindow *w,sGuiStyleKind style,const sRect &r_,int modifier,const char *text,uptr len,const char *extrastring)
{
    sRect r(r_);
    sRect rr(r);
    uint col = 0;

    int backid = w && (w->Flags & sWF_ChildFocus) ? sGC_Doc : sGC_Back;

    switch(style)
    {

        // misc

    case sSK_Back:
    case sSK_Doc:
        Painter->SetLayer(layer);
        Painter->Rect(Colors[backid],r);
        break;

    case sSK_Dummy:
        Painter->SetLayer(layer);
        Painter->Rect(Colors[backid],r);
        if(text)
        {
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(0,Colors[sGC_Text],r,text,len);
        }
        break;

    case sSK_GridLabel:
        Painter->SetLayer(layer+2);
        Painter->SetFont(Font);
        Painter->Text(sPPF_Right|sPPF_Space,Colors[sGC_Text],r,text,len);
        break;

    case sSK_GridGroup:
        {
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(0,Colors[sGC_Text],r,text,len);
            int w = Font->GetWidth(text,len);
            int h = Font->CellHeight/2;
            w = (r.SizeX()-w)/2 - 2*h;
            if(w>0)
            {
                uint col = Colors[sGC_Draw];
                Painter->SetLayer(layer+1);
                int n = modifier&3;
                for(int i=0;i<n;i++)
                {
                    int y = r.CenterY() - 3*(n-1)/2 + 3*i;
                    Painter->Rect(col,sRect(r.x0+h,y,r.x0+h+w,y+1));
                    Painter->Rect(col,sRect(r.x1-h-w,y,r.x1-h,y+1));
                }
            }
        }
        break;

    case sSK_ToolTip:
        {
            int clip = Painter->SaveClip();
            Painter->ResetClip();
            Painter->SetFont(Font);
            int w = Font->GetWidth(text,len);
            int h = Font->CellHeight;
            sRect rr(r);
            rr.x0 = rr.x0-1;
            rr.y0 = rr.CenterY()-h/2-1;
            rr.x1 = rr.x0 + w + Font->CellHeight*1 + 2;
            rr.y1 = rr.y0 + h + 2;
            /*
            if(rr.x1 > Gui->Root->Client.x1)
            {
            int d = rr.x1 - Gui->Root->Client.x1;
            rr.x0 -= d;
            rr.x1 -= d;
            }
            if(rr.x0 < Gui->Root->Client.x0)
            {
            int d = Gui->Root->Client.x0 - rr.x0;
            rr.x0 += d;
            rr.x1 += d;
            }
            */

            Painter->SetLayer(120);
            Painter->Frame(Colors[sGC_Draw],rr);
            rr.Extend(-1);
            Painter->Rect(Colors[sGC_Select],rr);
            //      rr.y0 += 2;
            Painter->SetLayer(121);
            Painter->Text(0/*sPPF_Left|sPPF_Top*/,Colors[sGC_Text],rr,text,len);

            Painter->RestoreClip(clip);
        }
        break;
    case sSK_BevelBorder:
        if(modifier & sSM_Pressed)
        {
            Painter->SetLayer(layer+15);
            Painter->Frame(Colors[sGC_Select],r);
        }
        break;

        // frames

    case sSK_VSplitter:
        Painter->SetLayer(layer);
        if(modifier & sSM_Pressed)
        {
            Painter->Rect(Colors[sGC_Select],r);
        }
        else
        {
            //      Painter->Rect(Colors[sGC_Draw],r);
            Painter->Rect(Colors[sGC_Back],sRect(r.x0  ,r.y0,r.x0+2,r.y1));
            Painter->Rect(Colors[sGC_Draw],sRect(r.x0+2,r.y0,r.x1-2,r.y1));
            Painter->Rect(Colors[sGC_Back],sRect(r.x1-2,r.y0,r.x1  ,r.y1));
        }
        break;

    case sSK_HSplitter:
        Painter->SetLayer(layer);
        if(modifier & sSM_Pressed)
        {
            Painter->Rect(Colors[sGC_Select],r);
        }
        else
        {
            //      Painter->Rect(Colors[sGC_Draw],r);
            Painter->Rect(Colors[sGC_Back],sRect(r.x0,r.y0  ,r.x1,r.y0+2));
            Painter->Rect(Colors[sGC_Draw],sRect(r.x0,r.y0+2,r.x1,r.y1-2));
            Painter->Rect(Colors[sGC_Back],sRect(r.x0,r.y1-2,r.x1,r.y1  ));
        }
        break;

    case sSK_HSeperator:
    case sSK_VSeperator:
        Painter->SetLayer(layer);
        Painter->Rect(Colors[sGC_Draw],r);
        break;

        // borders

    case sSK_FocusBorder:
        Painter->SetLayer(layer);
        if(modifier & sSM_Pressed)
            Hilo(Painter,r,Colors[sGC_Select],Colors[sGC_Select]);
        else
            Hilo(Painter,r,Colors[sGC_Back],Colors[sGC_Back]);
        break;

    case sSK_ThinBorder:
        Painter->SetLayer(layer);
        Hilo(Painter,r,Colors[sGC_Select],Colors[sGC_Select]);
        break;

    case sSK_NiceBorder:
        Painter->SetLayer(layer);
        Hilo(Painter,r,Colors[sGC_Select],Colors[sGC_Select],NiceBorderSize);
        break;

    case sSK_ToolBorder_Top:
        Painter->SetLayer(layer);
        r.y1--;
        Painter->Rect(Colors[sGC_Back],r);
        r = r_;
        r.y0 = r.y1-1;
        Painter->Rect(Colors[sGC_Draw],r);
        break;
    case sSK_ToolBorder_Bottom:
        Painter->SetLayer(layer);
        r.y0++;
        Painter->Rect(Colors[sGC_Back],r);
        r = r_;
        r.y1 = r.y0+1;
        Painter->Rect(Colors[sGC_Draw],r);
        break;

    case sSK_SizeBorder:
        Painter->SetLayer(layer);
        Hilo(Painter,r,Colors[sGC_Back],Colors[sGC_Back],1);
        r.Extend(-1);
        Hilo(Painter,r,Colors[sGC_Select],Colors[sGC_Select],1);
        r.Extend(-1);
        Hilo(Painter,r,Colors[sGC_Button],Colors[sGC_Button],SizeBorderSize-2);
        r.Extend(-(SizeBorderSize-2));
        Hilo(Painter,r,Colors[sGC_Select],Colors[sGC_Select]);
        break;

    case sSK_TitleBorder:
        Painter->SetLayer(layer);
        r.y1 = r.y0 + TitleBorderSize-1;
        Painter->Rect(Colors[sGC_Button],r);
        if(text)
        {
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(sPPF_Left|sPPF_Space,Colors[sGC_Text],r,text,len);
            Painter->SetLayer(layer);
        }
        r.y0 = r.y1;
        r.y1 = r.y0+1;
        Painter->Rect(Colors[sGC_Select],r);
        break;

    case sSK_StatusBorder:
        Painter->SetLayer(layer+0);
        Painter->Rect(Colors[sGC_Draw],r);
        Painter->SetFont(Font);
        Painter->SetLayer(layer+1);
        Painter->Rect(Colors[sGC_Back],r);
        Painter->SetLayer(layer+2);
        Painter->Rect(Colors[sGC_Draw],sRect(r.x0,r.y0,r.x0+1,r.y1));
        r.y0--;
        Painter->Text(sPPF_Left|sPPF_Top|sPPF_Space,Colors[sGC_Text],r,text);
        break;

        // buttons

    case sSK_Button_Push:
    case sSK_Button_Toggle:
    case sSK_Button_Radio:
        Painter->SetLayer(layer);
        if(modifier & sSM_Pressed)
        {
            //      Hilo(Painter,r,Colors[sGC_Back],Colors[sGC_Back]);
            r.Extend(-1);
            Hilo(Painter,r,Colors[sGC_Select],Colors[sGC_Select]);
        }
        else
        {
            //      Hilo(Painter,r,Colors[sGC_Back],Colors[sGC_Back]);
            r.Extend(-1);
            //      Hilo(Painter,r,Colors[sGC_Back],Colors[sGC_Back]);
        }
        r.Extend(-1);
        Painter->Rect(OverrideColors ? OverrideColorsBack : Colors[sGC_Button],r);
        OverrideColors = 0;

        if(text)
        {
            int align = 0;
            int width = Font->GetWidth(text,len);
            if(width > r.SizeX())
            {
                align = sPPF_Left;
                if((w->Flags & sWF_Hover) && width > r.SizeX() )
                {
                    sRect rr(r);
                    rr.x0 -= Font->CellHeight/2;
                    Rect(layer,w,sSK_ToolTip,rr,modifier,text,len,extrastring);
                }
            }
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(align,Colors[(modifier & sSM_Grayed) ? sGC_Low : sGC_Text],r,text,len);
        }
        break;

    case sSK_Button_Label:
        Painter->SetLayer(layer);
        Painter->Rect(OverrideColors ? OverrideColorsBack : Colors[sGC_Back],r);
        OverrideColors = 0;

        if(text)
        {
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(sPPF_Left|sPPF_Space,Colors[sGC_Text],r,text,len);
        }
        break;

    case sSK_Button_Drop:
        Painter->SetLayer(layer);
        Painter->Rect(OverrideColors ? OverrideColorsBack : (modifier & sSM_Pressed) ? Colors[sGC_High2] :  Colors[sGC_Back],r);
        OverrideColors = 0;

        if(text)
        {
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(sPPF_Left|sPPF_Space,Colors[sGC_Text],r,text,len);
        }
        break;

    case sSK_Button_ErrorMsg:
        Painter->SetLayer(layer);
        //    Painter->Rect(Colors[sGC_Back],r);

        if(text)
        {
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(sPPF_Left|sPPF_Space,0xffff0000,r,text,len);
        }
        break;

    case sSK_Button_Menu:
    case sSK_Button_MenuCheck2:
        {
            int sel = (style==sSK_Button_Menu) ? (w->Flags & sWF_Hover) : (w->Flags & sWF_Hover) || (modifier & sSM_Pressed);
            Painter->SetLayer(layer);
            if(OverrideColors)
            {
                if(sel)
                {
                    Painter->Rect(OverrideColorsBack,r);
                }
                else
                {
                    uint col0 = ((Colors[sGC_Back]>>1)&0x7f7f7f)
                        + ((Colors[sGC_Back]>>2)&0x3f3f3f);
                    uint col1 = ((OverrideColorsBack>>2)&0x3f3f3f);
                    uint colx = 0xff000000 | (col0+col1);
                    Painter->Rect(colx ,r);
                }
                OverrideColors = 0;
            }
            else
            {
                Painter->Rect(Colors[sel ? sGC_Select : sGC_Back],r);
            }

            if(text)
            {
                Painter->SetLayer(layer+2);
                Painter->SetFont(Font);
                Painter->Text(sPPF_Left|sPPF_Space,Colors[sGC_Text],r,text,len);
                if(extrastring)
                    Painter->Text(sPPF_Right|sPPF_Space,Colors[sel?sGC_Draw : sGC_Text],r,extrastring);
            }
        }
        break;

    case sSK_Button_MenuUnclickable:
        Painter->SetLayer(layer);
        col = OverrideColors ? OverrideColorsBack : Colors[sGC_Back];
        Painter->Rect(col,r);
        OverrideColors = 0;

        if(text)
        {
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(sPPF_Left|sPPF_Space,Colors[sGC_Text],r,text,len);
            if(extrastring)
                Painter->Text(sPPF_Right|sPPF_Space,Colors[(w->Flags & sWF_Hover)?sGC_Draw : sGC_Text],r,extrastring);
        }
        break;

    case sSK_Button_MenuHeader:
        Painter->SetLayer(layer);
        col = OverrideColors ? OverrideColorsBack : Colors[sGC_Button];
        Painter->Rect(col,r);
        OverrideColors = 0;

        if(text)
        {
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(sPPF_Left|sPPF_Space,Colors[sGC_Text],r,text,len);
            if(extrastring)
                Painter->Text(sPPF_Right|sPPF_Space,Colors[(w->Flags & sWF_Hover)?sGC_Draw : sGC_Text],r,extrastring);
        }
        break;

    case sSK_Button_MenuCheck:
        Painter->SetLayer(layer);
        //    Painter->Rect(Colors[(w->Flags & sWF_Hover)?sGC_Select:sGC_Back],r);
        if(w->Flags & sWF_Hover)
            Painter->Rect(Colors[sGC_Select],r);

        Painter->SetLayer(layer+1);
        rr.x0 += Font->CellHeight / 2;
        rr.x1 = rr.x0+rr.SizeY();
        rr.Extend(-2);
        Painter->Frame(Colors[sGC_Draw],rr);
        rr.Extend(-2);
        if(modifier & sSM_Pressed)
            Painter->Rect(Colors[sGC_Draw],rr);

        if(text)
        {
            r.x0 += r.SizeY();
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(sPPF_Left|sPPF_Space,Colors[sGC_Text],r,text,len);
            if(extrastring)
                Painter->Text(sPPF_Right|sPPF_Space,Colors[(w->Flags & sWF_Hover)?sGC_Draw : sGC_Text],r,extrastring);
        }
        break;

    case sSK_String:
        Painter->SetLayer(layer);
        //    Hilo(Painter,r,Colors[sGC_Back],Colors[sGC_Back],2);
        r.Extend(-1);
        if(OverrideColors)
            Painter->Rect(OverrideColorsBack,r);
        else
            Painter->Rect(Colors[sGC_Button],r);

        if(text)
        {
            uint textcolor = Colors[(modifier & sSM_Error) ? sGC_Error : sGC_Text];
            if(OverrideColors)
                textcolor = OverrideColorsText;

            if(len==-1)
                len = sGetStringLen(text);
            if(!(modifier & sSM_Pressed))
            {
                int cx = Font->GetWidth(text,sClamp<uptr>(CursorPos,0U,len));
                int climit = r.SizeX()*3/4;
                CursorScroll = 0;
                if(cx>climit)
                {
                    CursorScroll = climit-cx;
                    int w = Font->GetWidth(text,len);
                    int rw = r.SizeX()-Font->CellHeight;
                    if(CursorScroll+w < rw)
                        CursorScroll = rw-w;
                    if(CursorScroll>0)
                        CursorScroll = 0;
                }
            }
            int clipsave = 0;
            if(CursorScroll)
            {
                clipsave = Painter->SaveClip();
                Painter->AndClip(r);
            }

            r.x0 += CursorScroll;

            Painter->SetLayer(layer+3);
            Painter->SetFont(Font);
            Painter->Text(sPPF_Left|sPPF_Space,textcolor,r,text,len);
            if(w->Flags & sWF_Focus)
            {
                uptr c0,c1;

                if(SelectStart!=SelectEnd)
                {
                    c0 = sClamp<uptr>(sMin(SelectStart,SelectEnd),0U,len+1);
                    c1 = sClamp<uptr>(sMax(SelectStart,SelectEnd),0U,len+1);

                    int x0 = Font->GetWidth(text,c0) + Font->CellHeight/2 + r.x0;
                    int x1 = x0+Font->GetWidth(text+c0,c1-c0);

                    Painter->SetLayer(layer+1);
                    Painter->Rect(Colors[sGC_Select],sRect(x0,r.y0+1,x1,r.y1-1));
                }

                c1 = c0 = sClamp<uptr>(CursorPos,0U,len);
                int x0 = Font->GetWidth(text,c0) + Font->CellHeight/2 + r.x0;
                int x1 = x0;
                if(CursorOverwrite && SelectStart==SelectEnd)
                {
                    c1++;
                    if(c1>len)
                        x1 = x0+Font->GetWidth(text+c0,len-c0)+Font->CellHeight/2;
                    else
                        x1 = x0+Font->GetWidth(text+c0,c1-c0);
                }
                else
                {
                    x0--;
                    x1++;
                }
                Painter->SetLayer(layer+1);
                Painter->Rect(Colors[sGC_Cursor],sRect(x0,r.y0+1,x1,r.y1-1));
            }

            if(CursorScroll)
                Painter->RestoreClip(clipsave);
        }
        CursorPos = 0;
        CursorOverwrite = 0;
        SelectStart = 0;
        SelectEnd = 0;
        OverrideColors = 0;
        break;  

    case sSK_ListField:
        Painter->SetLayer(layer);
        if(OverrideColors && !(modifier & sSM_Pressed))
            Painter->Rect(OverrideColorsBack,r);
        else
            Painter->Rect(Colors[(modifier & sSM_Pressed)?sGC_Select:backid],r);
        OverrideColors = 0;

        if(text)
        {
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            Painter->Text(sPPF_Left|sPPF_Space,Colors[sGC_Text],r,text,len);
            if(extrastring)
                Painter->Text(sPPF_Right|sPPF_Space,Colors[sGC_Text],r,extrastring);
        }
        break;

    case sSK_TreeField:
        Painter->SetLayer(layer);
        if(OverrideColors && !(modifier & sSM_Pressed))
            Painter->Rect(OverrideColorsBack,r);
        else
            Painter->Rect(Colors[(modifier & sSM_Pressed)?sGC_Select:backid],r);
        OverrideColors = 0;

        if(text)
        {
            int xd = (TreeIndent+1)*Font->CellHeight;
            Painter->SetLayer(layer+2);
            Painter->SetFont(Font);
            if(TreeHasChilds)
                Painter->Text(sPPF_Left,Colors[sGC_Text],sRect(r.x0+xd-10,r.y0,r.x1,r.y1),TreeOpened ? "-" : "+",len);
            Painter->Text(sPPF_Left,Colors[sGC_Text],sRect(r.x0+xd,r.y0,r.x1,r.y1),text,len);
            if(extrastring)
                Painter->Text(sPPF_Right|sPPF_Space,Colors[sGC_Text],r,extrastring);
        }

        TreeIndent = 0;
        TreeOpened = 0;
        TreeHasChilds = 0;
        break;


    case sSK_TabBack:
        Painter->SetLayer(layer);
        r.y1--;
        Painter->Rect(Colors[sGC_Back],r);
        r = r_;
        r.y0 = r.y1-1;
        Painter->SetLayer(layer+15);
        Painter->Rect(Colors[sGC_Draw],r);
        break;
    case sSK_TabItem:
        {
            Painter->SetLayer(layer+1);
            r.y0+=2;
            Painter->Rect(Colors[sGC_Draw],sRect(r.x0,r.y0,r.x1,r.y0+1));
            Painter->Rect(Colors[sGC_Draw],sRect(r.x0,r.y0+1,r.x0+1,r.y1));
            Painter->Rect(Colors[sGC_Draw],sRect(r.x1-1,r.y0+1,r.x1,r.y1));
            r.y0++;
            r.x0++;
            r.x1--;
            uint col = Colors[sGC_Back];
            if(modifier & sSM_WindowSel)
                col = Colors[sGC_InactiveSelect];
            if(modifier & sSM_GlobalSel)
                col = Colors[sGC_Select];
            Painter->Rect(OverrideColors ? OverrideColorsBack : col,r);
            OverrideColors = 0;

            r.x0 += 2;
            r.x1 -= 2;
            Painter->SetLayer(layer+2);
            if(text)
            {
                Painter->SetFont(Font);
                Painter->Text(sPPF_Left,Colors[(modifier & sSM_Grayed) ? sGC_Low : sGC_Text],r,text,len);
            }
            Painter->SetFont(Font);
            col = Colors[(modifier & sSM_Grayed) ? sGC_Low : sGC_Draw];
            if(modifier & sSM_HoverIcon)
                col = Colors[sGC_HoverFeedback];
            Painter->SetFont(BoldFont);
            Painter->Text(sPPF_Right,col,r,"X");
            Painter->SetFont(Font);
        }
        break;

    case sSK_DragDropIcon:
        if(DragDropIcon)
        {
            int colt = 0xffffffff;
            int cold = 0xff000000;
            int colm = 0xffffffff & ((modifier && sSM_Grayed) ? 0x7fffffff : 0xffffffff);

            switch(DragDropIcon->Kind)
            {
            case sDIK_Image:
                Painter->SetLayer(layer+1);
                if(DragDropIcon->Image)
                    Painter->Image(DragDropIcon->Image,0xffffffff&colm,DragDropIcon->ImageRect,r);
                else
                    Painter->Rect(0xffff0000&colm,r);


                r.Extend(-2);

                if(Font->GetWidth(DragDropIcon->Text) > r.SizeX())
                {
                    int clip = Painter->SaveClip();
                    Painter->AndClip(r);
                    Painter->SetLayer(layer+3);
                    Painter->Text(sPPF_Bottom|sPPF_Left,colt,r,text);
                    Painter->SetLayer(layer+2);
                    r.Move(-1, 0);
                    Painter->Text(sPPF_Bottom|sPPF_Left,cold,r,text);
                    r.Move(+2, 0);
                    Painter->Text(sPPF_Bottom|sPPF_Left,cold,r,text);
                    r.Move(-1,-1);
                    Painter->Text(sPPF_Bottom|sPPF_Left,cold,r,text);
                    r.Move( 0,+2);
                    Painter->Text(sPPF_Bottom|sPPF_Left,cold,r,text);
                    Painter->RestoreClip(clip);
                }
                else
                {
                    Painter->SetLayer(layer+3);
                    Painter->Text(sPPF_Bottom,colt,r,text);
                    Painter->SetLayer(layer+2);
                    r.Move(-1, 0);
                    Painter->Text(sPPF_Bottom,cold,r,text);
                    r.Move(+2, 0);
                    Painter->Text(sPPF_Bottom,cold,r,text);
                    r.Move(-1,-1);
                    Painter->Text(sPPF_Bottom,cold,r,text);
                    r.Move( 0,+2);
                    Painter->Text(sPPF_Bottom,cold,r,text);
                }
                break;
            case sDIK_PushButton:
                Painter->SetLayer(layer);
                Painter->Rect((OverrideColors ? OverrideColorsBack : Colors[sGC_Button]) & colm,r);
                OverrideColors = 0;

                if(text)
                {
                    int align = 0;
                    Painter->SetLayer(layer+2);
                    Painter->SetFont(Font);
                    Painter->Text(sPPF_Left|sPPF_Space,Colors[sGC_Text]&colm,r,text,len);
                }
                break;
            }
        }
        DragDropIcon = 0;
        break;

    default:
        sGuiStyle::Rect(layer,w,style,r_,modifier,text,len,extrastring);
        break;
    }
    /*
    if(text)
    {
    Painter->SetLayer(layer+2);
    Painter->SetFont(sPainterFontPara(Font));
    Painter->Text(0,Colors[sGC_Text],r,text,len);
    }
    */
}

/****************************************************************************/
}
