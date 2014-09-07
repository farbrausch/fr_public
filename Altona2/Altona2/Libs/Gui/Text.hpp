/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_TEXT_HPP
#define FILE_ALTONA2_LIBS_GUI_TEXT_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Window.hpp"
#include "Altona2/Libs/Gui/Buttons.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sTextWindowEditorHelper : public sStringEditorHelper
{
public:
    sTextBuffer *Text;

    sTextWindowEditorHelper();
    const char *GetBuffer() { return Text->Get(); }
    uptr GetBufferUsed() { return Text->GetCount(); }
    uptr GetBufferSize() { return ~uptr(0); }
    void Delete(uptr pos);
    void Delete(uptr start,uptr end);
    bool Insert(uptr pos,int c);
    bool Insert(uptr pos,const char *text,uptr len=~uptr(0));
    void ScrollToCursor();
};


class sTextWindow : public sControl
{
protected:
    sTextWindowEditorHelper Editor;
    bool DragMode;
    int LineNumberMargin;
    sRect CursorRect;

    struct LineInfo
    {
        uptr Start;
        uptr End;
    };

    sArray<LineInfo> Lines;

    sFont *Font;
    void Construct();
    uptr FindClick(int mx,int my);
public:
    sTextWindow();
    sTextWindow(sTextBuffer *);
    ~sTextWindow();

    bool ControlStyle;
    uint OverrideBackCol;
    bool AlwaysDisplayCursor;
    bool LineNumbers;
    bool Wrapping;

    void Set(sTextBuffer *);
    void Set(sTextBuffer *,sStringEditorUndo *);
    void SetFont(sFont *font) { Font = font; }
    void SetStatic(bool s) { Editor.Static = s; }

    void OnCalcSize();
    void OnPaint(int layer);
    void OnLeaveFocus() { Editor.ChangeDone(); }
    void DragClick(const sDragData &dd);
    void SetCursor(uptr cursor) { Editor.SetCursor(cursor); }
    void SetSelect(uptr t0,uptr t1) { Editor.SetSelect(t0,t1); }
    uptr GetCursor() { return Editor.GetCursor(); }
    void ScrollToCursor();
    void UpdateLines();
};

/****************************************************************************/

class sTextControl : public sTextWindow
{
    sStringDesc Desc;
    sPoolString *Pool;
    sTextBuffer Text;
    void UpdateTarget();
    void Construct();
public:
    sTextControl();
    ~sTextControl();
    sTextControl(sPoolString &pool);
    sTextControl(const sStringDesc &desc);
    void SetBuffer(sPoolString &pool);
    void SetBuffer(const sStringDesc &desc);
    void SetText(const char *);
};

/****************************************************************************/

class sStaticTextControl : public sTextWindow
{
    sPainterMultiline *MultiPainter;
    int OldWidth;
public:
    sStaticTextControl();
    sStaticTextControl(const char *t);
    ~sStaticTextControl();
    void Init(const char *t);
    void OnPaint(int layer);
};

/****************************************************************************/

}

#endif  // FILE_ALTONA2_LIBS_GUI_TEXT_HPP

