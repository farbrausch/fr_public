/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_STYLE_HPP
#define FILE_ALTONA2_LIBS_GUI_STYLE_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Painter.hpp"

namespace Altona2 {

using namespace Altona2;

class sGuiAdapter;
struct sDragDropIcon;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

enum sGuiStyleColors
{
    sGC_Back = 0,                           // normal background
    sGC_Doc,                                // document background, like in a text editor (usually white)
    sGC_Button,                             // button background
    sGC_Text,                               // general text
    sGC_Draw,                               // general drawing (lines & markers)
    sGC_Select,                             // selected item background
    sGC_High,                               // bevelled rect : upper left (usually bright)
    sGC_Low,                                // bevelled rect : lower right (usually dim)
    sGC_High2,                              // double bevelled rect : upper left inner (usually white)
    sGC_Low2,                               // double bevelled rect : lower right inner (usually black)
    sGC_Cursor,                             // line or box cursor color
    sGC_Error,                              // mark something as error (usually red)
    sGC_InactiveSelect,                     // background for selected item in a window without focus
    sGC_HoverFeedback,                      // feedback color for hovering (usually white)
    sGC_Max,
};

enum sGuiStyleKind
{
    sSK_Back = 0,
    sSK_Doc,
    sSK_Dummy,
    sSK_GridLabel,
    sSK_GridGroup,
    sSK_ToolTip,
    sSK_BevelBorder,

    sSK_HSeperator,
    sSK_VSeperator,
    sSK_VSplitter,
    sSK_HSplitter,

    sSK_NiceBorder,
    sSK_FocusBorder,
    sSK_ThinBorder,
    sSK_ToolBorder_Top,
    sSK_ToolBorder_Bottom,
    sSK_SizeBorder,
    sSK_TitleBorder,
    sSK_StatusBorder,

    sSK_Button_Label,
    sSK_Button_Push,
    sSK_Button_Toggle,
    sSK_Button_Radio,
    sSK_Button_Menu,
    sSK_Button_MenuHeader,
    sSK_Button_MenuUnclickable,
    sSK_Button_MenuCheck,
    sSK_Button_MenuCheck2,
    sSK_Button_ErrorMsg,
    sSK_Button_Drop,
    sSK_String,
    sSK_ListField,
    sSK_TreeField,

    sSK_TabBack,
    sSK_TabItem,
    sSK_DragDropIcon,
};

enum sGuiStyleModifier
{
    sSM_Special         = 0x0000ffff,

    // provided through "modifier" parameter

    sSM_Pressed         = 0x00010000,
    sSM_Error           = 0x00020000,
    sSM_Grayed          = 0x00040000,
    sSM_WindowSel       = 0x00080000,   // used by TabItem
    sSM_GlobalSel       = 0x00100000,   // used by TabItem
    sSM_HoverIcon       = 0x00200000,   // used by TabItem
    sSM_Hover           = 0x00400000,

    // automatically derived from window

    sSM_Static          =  0x01000000,
};

class sGuiStyle
{
    virtual void SetColors();
public:
    sGuiStyle();
    virtual ~sGuiStyle();
    virtual void OnFrame();
    virtual void Rect(int layer,class sWindow *w,sGuiStyleKind style,const sRect &r,int modifier=0,const char *text=0,uptr len=-1,const char *extrastring=0);
    virtual void Update() {}

    sGuiAdapter *GuiAdapter;
    sPainter *Painter;
    sAdapter *Adapter;

    // style constants (read only, please)

    uint Colors[sGC_Max];
    sFont *Font;
    sFont *SmallFont;
    sFont *BoldFont;
    sFont *FixedFont;

    // border size of 2 means add/subtract 2 from all of x0,x1,y0,y1.

    int GeneralSize;
    int SplitterKnopSize;
    int NiceBorderSize;
    int FocusBorderSize;
    int ThinBorderSize;
    int ToolBorderSize;
    int SizeBorderSize;
    int TitleBorderSize;
    int ScrollBorderSize;
    int ScrollBorderFrameSize;
    int BevelBorderSize;

    // style extra parameters

    uptr CursorPos;                 // sSK_String
    bool CursorOverwrite;          // sSK_String
    uptr SelectStart;               // sSK_String
    uptr SelectEnd;                 // sSK_String
    int CursorScroll;
    int TreeIndent;                // sSK_Tree
    bool TreeOpened;               // sSK_Tree
    bool TreeHasChilds;            // sSK_Tree
    const sDragDropIcon *DragDropIcon;

    bool OverrideColors;           // sSK_String
    uint OverrideColorsText;        // sSK_String
    uint OverrideColorsBack;        // sSK_String
};

sGuiStyle *sGetDefaultStyle(sGuiAdapter *gada);
const char *sGetDefaultStyleName(int n);

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_STYLE_HPP

