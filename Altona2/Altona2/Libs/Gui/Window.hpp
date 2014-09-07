/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_WINDOW_HPP
#define FILE_ALTONA2_LIBS_GUI_WINDOW_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/painter.hpp"

namespace Altona2 {

class sGuiStyle;
class sWindow;
class sGuiInitializer;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sGuiAdapter
{
public:
    sGuiAdapter(sAdapter *ada,sGuiInitializer *initapp);
    sGuiAdapter();
    ~sGuiAdapter();

    sAdapter *Adapter;
    sContext *Context;
    sPainter *Painter;
    sGuiStyle *Style;

    sPainterImage *IconAtlas;
};

class sGuiScreen
{
public:
    sGuiScreen();
    virtual ~sGuiScreen()=0;

    virtual void GetSize(int &sx,int &sy)=0;
    virtual bool GetTargetPara(sTargetPara &tp)=0;
    virtual bool IsRightScreen(sScreen *)=0;
    virtual sResource *GetColorBuffer()=0;
    virtual sResource *GetDepthBuffer()=0;
    virtual int GetDefaultColorFormat()=0;
    virtual int GetDefaultDepthFormat()=0;
    virtual void WindowControl(WindowControlEnum mode)=0;
    virtual void SetDragDropCallback(sDelegate1<void,const sDragDropInfo *> del) {}
    virtual void ClearDragDropCallback() {}

    sWindow *Root;
    sGuiAdapter *GuiAdapter;
    int BufferSequence;

    sRect UpdateRect;
    sRect ScissorRect;
    sRect SequenceRects[4];
    int SequenceIndex;
};

class sRegularGuiScreen : public sGuiScreen
{
    sScreen *Screen;
public:
    sRegularGuiScreen(sScreen *);
    ~sRegularGuiScreen();

    void GetSize(int &sx,int &sy);
    bool GetTargetPara(sTargetPara &tp);
    bool IsRightScreen(sScreen *);
    sResource *GetColorBuffer() { return Screen->ColorBuffer; }
    sResource *GetDepthBuffer() { return Screen->DepthBuffer; }
    int GetDefaultColorFormat() { return Screen->GetDefaultColorFormat(); }
    int GetDefaultDepthFormat() { return Screen->GetDefaultDepthFormat(); }
    void WindowControl(WindowControlEnum mode) { Screen->WindowControl(mode); }
    void SetDragDropCallback(sDelegate1<void,const sDragDropInfo *> del) { Screen->SetDragDropCallback(del); }
    void ClearDragDropCallback() { Screen->ClearDragDropCallback(); }
};

/****************************************************************************/
/***                                                                      ***/
/***   The Message Class                                                  ***/
/***                                                                      ***/
/****************************************************************************/

class sGuiMsg
{
    struct sMessageTarget
    {
    };

    int Type;
    union
    {
        int Code;
        void *Data;
    };
    sMessageTarget *Target;
    union
    {
        void (sMessageTarget::*Func01)();
        void (sMessageTarget::*Func02)(int);
        void (sMessageTarget::*Func03)(void *);
        void (sMessageTarget::*Func11)(const sDragData &);
        void (sMessageTarget::*Func12)(const sDragData &,int);
        void (sMessageTarget::*Func13)(const sDragData &,void *);
        void (sMessageTarget::*Func21)(const sKeyData &);
        void (sMessageTarget::*Func22)(const sKeyData &,int);
        void (sMessageTarget::*Func23)(const sKeyData &,void *);
        void (*Func31)();
        void (*Func32)(int);
        void (*Func33)(void*);
        void (*Func41)(const sDragData &);
        void (*Func42)(const sDragData &,int);
        void (*Func43)(const sDragData &,void*);
        void (*Func51)(const sKeyData &);
        void (*Func52)(const sKeyData &,int);
        void (*Func53)(const sKeyData &,void*);
    };

public:
    sGuiMsg() { Type = 0; Data = 0; Target = 0; Func01 = 0; }
    template<class T, class TBase> sGuiMsg(T *target,void (TBase::*func)()                                     ) { Type=0x01; Data=0;    Target=(sMessageTarget*)(target); Func01 =(void (sMessageTarget::*)()                        )func; }
    template<class T, class TBase> sGuiMsg(T *target,void (TBase::*func)(int)                    ,int  code=0) { Type=0x02; Code=code; Target=(sMessageTarget*)(target); Func02 =(void (sMessageTarget::*)(int)                    )func; }
    template<class T, class TBase> sGuiMsg(T *target,void (TBase::*func)(void *)                  ,void* data=0) { Type=0x03; Data=data; Target=(sMessageTarget*)(target); Func03 =(void (sMessageTarget::*)(void *)                  )func; }
    template<class T, class TBase> sGuiMsg(T *target,void (TBase::*func)(const sDragData &)                    ) { Type=0x11; Data=0;    Target=(sMessageTarget*)(target); Func11 =(void (sMessageTarget::*)(const sDragData &)       )func; }
    template<class T, class TBase> sGuiMsg(T *target,void (TBase::*func)(const sDragData &,int)  ,int  code=0) { Type=0x12; Code=code; Target=(sMessageTarget*)(target); Func12 =(void (sMessageTarget::*)(const sDragData &,int)  )func; }
    template<class T, class TBase> sGuiMsg(T *target,void (TBase::*func)(const sDragData &,void *),void* data=0) { Type=0x13; Data=data; Target=(sMessageTarget*)(target); Func13 =(void (sMessageTarget::*)(const sDragData &,void *))func; }
    template<class T, class TBase> sGuiMsg(T *target,void (TBase::*func)(const sKeyData &)                     ) { Type=0x21; Data=0;    Target=(sMessageTarget*)(target); Func21 =(void (sMessageTarget::*)(const sKeyData &)        )func; }
    template<class T, class TBase> sGuiMsg(T *target,void (TBase::*func)(const sKeyData &,int)   ,int  code=0) { Type=0x22; Code=code; Target=(sMessageTarget*)(target); Func22 =(void (sMessageTarget::*)(const sKeyData &,int)   )func; }
    template<class T, class TBase> sGuiMsg(T *target,void (TBase::*func)(const sKeyData &,void *), void* data=0) { Type=0x23; Data=data; Target=(sMessageTarget*)(target); Func23 =(void (sMessageTarget::*)(const sKeyData &,void *) )func; }
    sGuiMsg(void (*func)()                                                                                     ) { Type=0x31; Data=0;    Target=0;                         Func31 =                                                    func; }
    sGuiMsg(void (*func)(int)                                                                    ,int  code=0) { Type=0x32; Code=code; Target=0;                         Func32 =                                                    func; }
    sGuiMsg(void (*func)(void*)                                                                   ,void* data=0) { Type=0x33; Data=data; Target=0;                         Func33 =                                                    func; }
    sGuiMsg(void (*func)(const sDragData &)                                                                    ) { Type=0x31; Data=0;    Target=0;                         Func41 =                                                    func; }
    sGuiMsg(void (*func)(const sDragData &,int)                                                  ,int  code=0) { Type=0x32; Code=code; Target=0;                         Func42 =                                                    func; }
    sGuiMsg(void (*func)(const sDragData &,void*)                                                 ,void* data=0) { Type=0x33; Data=data; Target=0;                         Func43 =                                                    func; }
    sGuiMsg(void (*func)(const sKeyData &)                                                                     ) { Type=0x31; Data=0;    Target=0;                         Func51 =                                                    func; }
    sGuiMsg(void (*func)(const sKeyData &,int)                                                   ,int  code=0) { Type=0x32; Code=code; Target=0;                         Func52 =                                                    func; }
    sGuiMsg(void (*func)(const sKeyData &,void*)                                                  ,void* data=0) { Type=0x33; Data=data; Target=0;                         Func53 =                                                    func; }

    void SetCode(int code) { Code = code; }
    void *GetTarget() { return (void *) Target; }

    void Send();
    void Send(const sDragData &dd);
    void Send(const sKeyData &dd);
    void SendCode(int code);
    void SendCode(const sDragData &dd,int code);
    void SendCode(const sKeyData &dd,int code);
    void Post();
    bool IsEmpty() const { return Func01==0; }

    bool operator==(const sGuiMsg &b)
    {
        if(Type!=b.Type) return 0;
        if(Data!=b.Data) return 0;
        if(Func01!=b.Func01) return 0;
        return 1;
    }
};


/****************************************************************************/
/***                                                                      ***/
/***   Drag & Drop                                                        ***/
/***                                                                      ***/
/****************************************************************************/

enum sDragDropIconKind
{
    sDIK_Error,
    sDIK_PushButton,
    sDIK_Image,
};

struct sDragDropIcon
{
    // Drawing

    sDragDropIconKind Kind;
    const char *Text;
    sPainterImage *Image;
    sRect ImageRect;
    sRect Client;

    // Identification

    sWindow *SourceWin;
    int Command;
    int Index;
    sGCObject *GcObject;
    sRCObject *RcObject;

    // Constructors

    void Clear();
    sDragDropIcon();
    sDragDropIcon(const sRect &client,int cmd,int index,const char *text);
    sDragDropIcon(const sRect &client,int cmd,int index,const char *text,sGCObject *obj);
    sDragDropIcon(const sRect &client,int cmd,int index,const char *text,sRCObject *obj);
    sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img);
    sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img,sGCObject *obj);
    sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img,sRCObject *obj);
    sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img,const char *text);
    sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img,const char *text,sGCObject *obj);
    sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img,const char *text,sRCObject *obj);
};

/****************************************************************************/
/***                                                                      ***/
/***   Window Root Class                                                  ***/
/***                                                                      ***/
/****************************************************************************/

enum sWindowFlags
{
    // set by the manager or layout: state of the window

    sWF_Focus         = 0x00000001, // focus
    sWF_ChildFocus    = 0x00000002, // a child of this window has focus
    sWF_Hover         = 0x00000004, // this window is below the mouse cursor
    sWF_Disable       = 0x00000008, // set this during OnLayout()
    sWF_Static        = 0x00000010, // will not receive keys/drags and looks grayed out
    sWF_Dirty         = 0x00000020, // window requires redraw
    sWF_Kill          = 0x00000040, // kill this window during next layout

    // set when window is created

    sWF_Border        = 0x00010000, // set by sBorder::sBorder()
    sWF_ReceiveHover  = 0x00020000, // will need repainting when hovering in or out
    sWF_Back          = 0x00040000, // as child of sOverlappedFrame, fill background
    sWF_Top           = 0x00080000, // as child of sOverlappedFrame, on top of everything else
    sWF_Overlapped    = 0x00100000, // child windows may overlap. use different renderpass allocation
    sWF_ScrollX       = 0x00200000, // automatically set by AddScrolling
    sWF_ScrollY       = 0x00400000, // automatically set by AddScrolling
    sWF_ManyLayers    = 0x00800000, // allow for 0x1000 layers (not just 16)
    sWF_Keyboard      = 0x01000000, // on IOS, display virtual keyboard when in primary focus
    sWF_Paint3d       = 0x02000000, // enable 3d drawing
    sWF_GiveSpace     = 0x04000000, // layouting hint for toolborder
    sWF_AutoKill      = 0x08000000, // kill window automatically when it looses focus
};

/****************************************************************************/

enum sWindowKeyInfoKind
{
    sWKIK_Key = 1,
    sWKIK_AllKeys,
    sWKIK_SpecialKeys,
};

struct sWindowKeyInfo
{
    sWindowKeyInfoKind Kind;
    uint Key;
    const char *Label;
    sGuiMsg Message;
};

enum sWindowDragInfoKind
{
    sWDIK_Drag = 1,
    sWDIK_DragHit,
    sWDIK_DragMiss,
    sWDIK_DragAll,
};

struct sWindowToolInfo
{
    uint Key;
    const char *Label;
    sGuiMsg Message;
};

struct sWindowDragInfo
{
    sWindowDragInfoKind Kind;
    uint Key;
    const char *Label;
    int Hit;                       // DragWithHit: what to hit or miss?
    sGuiMsg Message;
    sWindowToolInfo *Tool;
};

/****************************************************************************/

class sUpdatePool;
class sUpdateReceiver
{
    sArray<sUpdatePool *> Pools;
public: 
    sUpdateReceiver();
    ~sUpdateReceiver();
    sDelegate<void> Call;
    //  void Update();
    void Rem(sUpdatePool *);
    void Add(sUpdatePool *);
};

class sUpdatePool
{
    sArray<sUpdateReceiver *> Receivers;
public:
    sUpdatePool();
    ~sUpdatePool();
    void Update();
    void Rem(sUpdateReceiver *);
    void Add(sUpdateReceiver *);
};

void sUpdateConnection(sUpdateReceiver *,sUpdatePool *);

/****************************************************************************/

class sWindow
{
    friend class sGuiManager;

    int ScrollStartX;
    int ScrollStartY;
    sArray<sDragDropIcon> DragDropIcons;
public:
    sRTTIBASE(sWindow);

    // Main Members

    sGuiScreen *Screen;             // Set before OnInit() call
    sRect Client;                   // client area, with scrolling
    sRect Inner;                    // window through which the client area is visible
    sRect Outer;                    // outer rect, including borders (decoration)
    sHardwareCursorImage MouseCursor; // display this cursor when hovering
    sUpdateReceiver UpdateReceiver; // for auto-update notifications

    int ReqSizeX;                  // required size - from OnCalcSize()
    int ReqSizeY;
    int DecoratedSizeX;
    int DecoratedSizeY;
    int ScrollX;
    int ScrollY;

    sWindow *LayoutShortcut;        // during layout, this window is replaced. for tab-fullscreen
    sArray<sWindow *> Childs;    // child windows
    sArray<sWindow *> Borders;   // decoration child windows
    sWindow *FocusParent;           // this may be 0. it is set for the focus window chain. a window may have multiple parents!
    sWindow *BorderParent;          // in a border, first non-border parent.
    sWindow *IndirectParent;        // An indirect child, like the choice control that opened a menu frame. set manually

    int Flags;                     // sWF_???
    int Temp;                      // use as you like for temporary operations

    sArray<sWindowKeyInfo *> KeyInfos;    // user interface wiring
    sArray<sWindowDragInfo *> DragInfos;  // user interface wiring
    sArray<sWindowToolInfo *> ToolInfos;  // user interface wiring
    sWindowToolInfo *CurrentTool;

    const char *ClassName;
    const char *ToolTipText;
    int ToolTipTextLen;

    // Main Methods

    sWindow();
    virtual ~sWindow();

    virtual void OnInit() {}      // here you have access to the Screen ptr to create Adapter resources
    virtual void OnCalcSize() {}
    virtual void OnLayoutChilds() {}
    virtual int OnCheckHit(const sDragData &dd) { return 0; }
    virtual void OnPaint3d() {}
    virtual bool OnPaintToolTip(int layer,int tx,int ty,int time);
    virtual void OnPaint(int layer) {}
    virtual void OnHover(const sDragData &dd) {}
    virtual void OnEnterFocus() {}
    virtual void OnLeaveFocus() {}
    virtual void OnLayoutOverlapped(const sRect &box) {}
    virtual bool OnKey(const sKeyData &kd) { return 0; }
    virtual bool OnDrag(const sDragData &dd) { return 0; }
    virtual void OnDragDropFrom(const sDragDropIcon &icon,bool remove) {}
    virtual bool OnDragDropTo(const sDragDropIcon &icon,int &mx,int &my,bool checkonly) { return false; }
    virtual void Tag() {}

    // helper methods

    void AddChild(sWindow *);
    void AddBorder(sWindow *);
    void AddScrolling(bool x,bool y);
    void AddHelp();
    void SetScreen(sGuiScreen *scr);
    void ClearDragDrop();
    void AddDragDrop(const sDragDropIcon &icon);
    void PaintDragDropIcon(int layer,const sDragDropIcon &icon,int dx,int dy);
    void PaintDragDrop(int layer);
    void ResetTooltipTimer();

    sWindowKeyInfo *AddKey(const char *label,uint key,const sGuiMsg &msg);
    sWindowKeyInfo *AddAllKeys(const char *label,const sGuiMsg &msg);
    sWindowKeyInfo *AddSpecialKeys(const char *label,const sGuiMsg &msg);
    sWindowToolInfo *AddTool(const char *label,uint key,const sGuiMsg &msg);

    sWindowDragInfo *AddDrag(const char *label,uint key,const sGuiMsg &msg,sWindowToolInfo *tool=0);
    sWindowDragInfo *AddDragHit(const char *label,uint key,const sGuiMsg &msg,int checkhit,sWindowToolInfo *tool=0);
    sWindowDragInfo *AddDragMiss(const char *label,uint key,const sGuiMsg &msg,int checkhit,sWindowToolInfo *tool=0);
    sWindowDragInfo *AddDragAll(const char *label,const sGuiMsg &msg,sWindowToolInfo *tool=0);

    void RemKey(const sGuiMsg &msg);

    // helper message handlers

    void CmdKill();                 // kill this window
    void CmdExit();                 // exit whole app (no questions asked)
    void CmdScrollWheelUp();
    void CmdScrollWheelDown();
    void CmdHelp();
    void DragScroll(const sDragData &dd);

    // other helpers

    void Update();
    void ScrollTo(const sRect &area);   // make this area visible
    void ScrollTo(int x,int y);
    sAdapter *Adapter() { return Screen->GuiAdapter->Adapter; }
    sGuiStyle *Style() { return Screen->GuiAdapter->Style; }
    sPainter *Painter() { return Screen->GuiAdapter->Painter; }
};

class sBorder : public sWindow
{
public:
    sBorder() { Flags |= sWF_Border; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Multitouch Emulation                                               ***/
/***                                                                      ***/
/****************************************************************************/

class sMultitoucher
{
    enum ModeEnum
    {
        sMM_Off = 0,
        sMM_Centered,
        sMM_MirrorY,
    };
    sDragData DragOut;
    int Mode;
    int ScreenX;
    int ScreenY;
public:
    sMultitoucher();
    const sDragData &Convert(const sDragData &dd);
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_WINDOW_HPP

