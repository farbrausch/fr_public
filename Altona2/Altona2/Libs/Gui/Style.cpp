/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Gui.hpp"
#include "Altona2/Libs/Util/Config.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sGuiStyle::sGuiStyle()
{
    Font = 0;
    BoldFont = 0;
    FixedFont = 0;
    GeneralSize = 2;
    SplitterKnopSize = 5;
    NiceBorderSize = 4;
    FocusBorderSize = 1;
    ThinBorderSize = 1;
    ToolBorderSize = 1;
    SizeBorderSize = 6;
    TitleBorderSize = 20;
    ScrollBorderSize = 12;
    ScrollBorderFrameSize = 1;
    BevelBorderSize = 2;

    CursorPos = 0;
    CursorOverwrite = 0;
    SelectStart = 0;
    SelectEnd = 0;
    CursorScroll = 0;
    TreeIndent = 0;
    TreeOpened = 0;
    TreeHasChilds = 0;
    DragDropIcon = 0;

    OverrideColors = 0;
    OverrideColorsText = 0;
    OverrideColorsBack = 0;

    SetColors();
}

sGuiStyle::~sGuiStyle() 
{
    // can't release because painter is already destroyed.
    // will not leak since painter will kill remaining fonts.
    if(0)
    {
        Painter->ReleaseFont(Font);
        Painter->ReleaseFont(SmallFont);
        Painter->ReleaseFont(BoldFont);
        Painter->ReleaseFont(FixedFont);
    }
}

void sGuiStyle::OnFrame()
{
    if(sGUI_ColorUpdateDebug)
    {
        sVector4 m;
        int time = sGetTimeMS();
        m.x = sSin(time*0.01f)*0.1f;
        m.y = sCos(time*0.01f)*0.1f;
        m.z =-sCos(time*0.01f)*0.1f;

        SetColors();
        for(int i=0;i<sGC_Max;i++)
        {
            sVector4 c; c.SetColor(Colors[i]);
            Colors[i] = (c+m).GetColor();
        }
    }
}

void sGuiStyle::SetColors()
{
    Colors[sGC_Back             ] = 0xffc0c0c0;
    Colors[sGC_Doc              ] = 0xffd0d0d0;
    Colors[sGC_Button           ] = 0xff909090;
    Colors[sGC_Text             ] = 0xff000000;
    Colors[sGC_Draw             ] = 0xff000000;
    Colors[sGC_Select           ] = 0xffff8080;
    Colors[sGC_High             ] = 0xffe0e0e0;
    Colors[sGC_Low              ] = 0xff606060;
    Colors[sGC_High2            ] = 0xffffffff;
    Colors[sGC_Low2             ] = 0xff000000;
    Colors[sGC_Cursor           ] = 0xff404040;
    Colors[sGC_Error            ] = 0xffff0000;
    Colors[sGC_InactiveSelect   ] = 0xff804040;
    Colors[sGC_HoverFeedback    ] = 0xffffffff;
}

void sGuiStyle::Rect(int layer,sWindow *w,sGuiStyleKind style,const sRect &r,int modifier,const char *text,uptr len,const char *extrastring)
{
    Painter->SetLayer(layer);
    Painter->Rect(Colors[sGC_Back],r);
    if(text)
    {
        Painter->SetLayer(layer+1);
        Painter->SetFont(Font);
        Painter->Text(0,Colors[sGC_Text],r,text,len);
    }
}


/****************************************************************************/

sGuiStyle *sGetDefaultStyle(sGuiAdapter *gada)
{
    sEnableAltonaConfig();

    const char *style = sGetConfigString("GuiStyle");
    if(!style)
        style = "";

    if(sCmpStringI(style,"Flat")==0)
        return new sGuiStyle_Flat(gada);
    if(sCmpStringI(style,"FlatSmall")==0)
        return new sGuiStyle_Flat(gada,true);
    if(sCmpStringI(style,"OldSchool")==0)
        return new sGuiStyle_Small(gada);

    // use oldschool as default because the df-generator is not optimized yet.

    return new sGuiStyle_Small(gada);
}


const char *sGetDefaultStyleName(int n)
{
    static const char *names[] = 
    {
        "Flat",
        "FlatSmall",
        "OldSchool",
    };

    if(n>=0 && n<sCOUNTOF(names))
        return names[n];
    return 0;
}


/****************************************************************************/

}
