/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_BORDERS_HPP
#define FILE_ALTONA2_LIBS_GUI_BORDERS_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Window.hpp"
#include "Altona2/Libs/Gui/Style.hpp"
#include "Altona2/Libs/Gui/Manager.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Decorative Borders                                                 ***/
/***                                                                      ***/
/****************************************************************************/

class sNiceBorder : public sBorder
{
public:
    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
};

class sFocusBorder : public sBorder
{
public:
    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
};

class sThinBorder : public sBorder
{
public:
    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
};

/****************************************************************************/
/***                                                                      ***/
/***   Toolbar (horizontal)                                               ***/
/***                                                                      ***/
/****************************************************************************/

class sToolBorder : public sBorder
{
    bool Top;
public:
    sToolBorder(bool top = 1);
    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
};

/****************************************************************************/
/***                                                                      ***/
/***   Status Border (Bottom)                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sStatusBorder : public sBorder
{
    struct Item
    {
        sString<sFormatBuffer> Text;
        int Size;
        sRect r;

        Item() { Size = 0; }
        Item(int s) { Size = s; }
    };

    int Height;
    sArray<Item> Items;
public:
    sStatusBorder();
    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);

    int AddItem(int s);
    void Print(int n,const char *text);

    sPRINTING1(PrintF,sFormatStringBuffer buf=sFormatStringBase(Items[arg1].Text,format);buf,Update();,int)
};

/****************************************************************************/
/***                                                                      ***/
/***   For overlapped frames                                              ***/
/***                                                                      ***/
/****************************************************************************/

class sSizeBorder : public sBorder
{
    int DragMode;
    sRect DragStart;
public:
    sSizeBorder();

    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
    void Drag(const sDragData &dd);
};

class sTitleBorder : public sBorder
{
    const char *Text;
    int DragMode;
    sRect DragStart;
public:
    sTitleBorder();
    sTitleBorder(const char *text);

    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
    void Drag(const sDragData &dd);
};

/****************************************************************************/
/***                                                                      ***/
/***   Scrolling                                                          ***/
/***                                                                      ***/
/****************************************************************************/

class sScrollBorder : public sBorder
{
    int DragStartX;
    int DragStartY;
    int DragMode;
    bool EnableX;                  // enable scrolling leftright
    bool EnableY;                  // enable scrolling updown

    sRect RectX;                    // leftright scrollbar at bottom side
    sRect RectY;                    // updown scrollbar at right side
    sRect RectC;                    // corner rect if both x and y
    sRect BarX;                     // leftright knop
    sRect BarY;                     // updown knop
public:
    sScrollBorder(bool x,bool y);

    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
    void Drag(const sDragData &dd);
};

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct sTabElementInfo
{
    const char *Name;
    sRect Client;
    bool HoverDelete;

    sTabElementInfo() { Name = "xxx"; HoverDelete = false; }
};

struct sTabDummyElement
{
};

class sTabBorderBase : public sWindow
{
    sArray<sTabDummyElement *> *Array;
    sTabElementInfo sTabDummyElement::*Info;
    sTabDummyElement **WindowSelect;
    sTabDummyElement **GlobalSelect;
    void Construct();
    int FindDragDrop(const sDragDropIcon &icon,int mx,int my);
    bool BoxesDirty;
protected:
    sTabDummyElement *Selected;
    bool Pressed;
    bool Delete;
    bool DelayedDelete;
public:
    sGuiMsg SelectMsg;
    sGuiMsg DeleteMsg;
    sRTTIBASE(sTabBorderBase);
    void *DropGroup;

    sTabBorderBase();
    sTabBorderBase(sArray<sTabDummyElement *> &a,sTabElementInfo sTabDummyElement::*info,sTabDummyElement **windowselect,sTabDummyElement **globalselect);
    void SetArray(sArray<sTabDummyElement *> &a,sTabElementInfo sTabDummyElement::*info);
    void SetWindowSelect(sTabDummyElement **e);
    void SetGlobalSelect(sTabDummyElement **e);

    void OnCalcSize();
    void OnLayoutChilds();
    void UpdateElements();

    void OnPaint(int layer);
    void OnHover(const sDragData &dd);
    void OnEnterFocus() { BorderParent->OnEnterFocus(); }
    void OnLeaveFocus() { BorderParent->OnLeaveFocus(); }
    void DragSelect(const sDragData &dd);
    void OnDragDropFrom(const sDragDropIcon &icon,bool remove);
    bool OnDragDropTo(const sDragDropIcon &icon,int &mx,int &my,bool checkonly);
};

template <class T> class sTabBorder : public sTabBorderBase
{
public:
    sTabBorder()
        : sTabBorderBase() {}
    sTabBorder(sArray<T *> &a,sTabElementInfo T::*info,T **windowselect,T **globalselect)
        : sTabBorderBase((sArray<sTabDummyElement *> &)a,(sTabElementInfo sTabDummyElement::*)info,(sTabDummyElement **)windowselect,(sTabDummyElement **)globalselect) {}
    void SetArray(sArray<sTabDummyElement *> &a,sTabElementInfo sTabDummyElement::*info)
    { sTabBorderBase::SetArray((sArray<sTabDummyElement *> &)a,(sTabElementInfo sTabDummyElement::*)info); }
    void SetWindowSelect(T **e)
    { sTabBorderBase::SetWindowSelect((sTabDummyElement **)e); }
    void SetGlobalSelect(T **e)
    { sTabBorderBase::SetGlobalSelect((sTabDummyElement **)e); }
    T *GetSelected()
    { return (T *) Selected; }
};

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_BORDERS_HPP

