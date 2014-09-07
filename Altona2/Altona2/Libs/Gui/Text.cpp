/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/painter.hpp"
#include "Altona2/Libs/gui/text.hpp"
#include "Altona2/Libs/gui/manager.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sTextWindowEditorHelper::sTextWindowEditorHelper()
{
    Text = 0;
}

void sTextWindowEditorHelper::Delete(uptr pos)
{
    if(Static) return;

    AllocUndo();
    const char *b = Text->Get();
    int n = 1;
    while((b[pos+n]&0xc0)==0x80) n++;
    Undo->Delete(pos,b+pos,n);

    Text->Delete(pos);
    if(Cursor>pos)
        Cursor--;
}

void sTextWindowEditorHelper::Delete(uptr start,uptr end)
{
    if(Static) return;

    AllocUndo();
    Undo->Delete(start,Text->Get()+start,end-start);

    Text->Delete(start,end);
    if(Cursor>start && Cursor<end)
        Cursor = start;
    else if(Cursor>=end)
        Cursor -= end-start;
}

bool sTextWindowEditorHelper::Insert(uptr pos,int c)
{
    if(Static) return false;
    bool ok = Text->Insert(pos,c);

    AllocUndo();
    const char *b = Text->Get();
    int n = 1;
    while((b[pos+n]&0xc0)==0x80) n++;
    Undo->Insert(pos,b+pos,n);

    return ok;
}

bool sTextWindowEditorHelper::Insert(uptr pos,const char *text,uptr len)
{
    if(Static) return false;
    if(len==~uptr(0))
        len = sGetStringLen(text);

    AllocUndo();
    Undo->Insert(pos,text,len);

    return Text->Insert(pos,text,len);
}

void sTextWindowEditorHelper::ScrollToCursor()
{
    sTextWindow *tw = (sTextWindow *) Window;
    tw->ScrollToCursor();
}

/****************************************************************************/

sTextWindow::sTextWindow()
{
    Construct();
}

sTextWindow::sTextWindow(sTextBuffer *t)
{
    Editor.Text = t;
    Construct();
}

sTextWindow::~sTextWindow()
{
}


void sTextWindow::Set(sTextBuffer *t)
{
    Editor.Text = t;
    Editor.NewText();
}

void sTextWindow::Set(sTextBuffer *t,sStringEditorUndo *undo)
{
    Editor.Text = t;
    Editor.NewText();
    Editor.Undo = undo;
}

void sTextWindow::Construct()
{
    AddHelp();

    LineNumbers = 0;
    LineNumberMargin = 0;
    DragMode = 0;
    ControlStyle = 0;
    Wrapping = 0;
    OverrideBackCol = 0;
    AlwaysDisplayCursor = false;

    Font = 0;

    Editor.Multiline = 1;
    Editor.Register(this);
    //  AddAllKeys("...",sGuiMsg(this,&sTextWindow::CmdKey));
    AddDrag("click",sKEY_LMB,sGuiMsg(this,&sTextWindow::DragClick));
}


uptr sTextWindow::FindClick(int xpos,int ypos)
{
    UpdateLines();

    xpos -= Client.x0+1;
    ypos -= Client.y0+1;
    if(ControlStyle)
    {
        xpos--;
        ypos--;
    }
    if(!Editor.Text) return 0;

    int line = 0;
    if(ypos>=0)
        line = ypos / Style()->FixedFont->CellHeight;
    if(line>=Lines.GetCount())
        return Editor.Text->GetCount();

    int x0 = LineNumberMargin;
    int x1;

    if(xpos<=x0) return 0;
    const char *str = Editor.Text->Get() + Lines[line].Start;
    const char *end = Editor.Text->Get() + Lines[line].End;
    while(str<end)
    {
        const char *oldstr = str;
        int c = sReadUTF8(str);
        int w = Style()->FixedFont->GetWidth(c);
        x1 = x0+w;

        int xm = (x0+x1)/2;
        if(xpos>=x0 && xpos<xm) 
            return oldstr-Editor.Text->Get();
        else if(xpos>=xm && xpos<x1)
            return str-Editor.Text->Get();

        x0 = x1;
    }

    return str-Editor.Text->Get();
}


/****************************************************************************/

void sTextWindow::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = 0;
    LineNumberMargin = 0;
    if(LineNumbers)
        LineNumberMargin = Style()->FixedFont->GetWidth("00000 ");
    UpdateLines();
}

void sTextWindow::UpdateLines()
{
    auto font = Font ? Font : Style()->FixedFont;

    int w = Inner.SizeX();
    w -= 2;
    w -= LineNumberMargin;
    if(ControlStyle)
        w -= 2;
    w -=  font->Advance/2;

    const char *tstart = Editor.Text->Get();
    const char *s = tstart;

    int maxx = 0;

    Lines.Clear();
    bool finalline = 1;
    int x = Client.x0 + 1 + LineNumberMargin;
    int y = Client.y0 + 1;
    CursorRect.Init(0,0,0,0);
    while(*s)
    {
        LineInfo li;
        li.Start = s-tstart;
        if(Wrapping)
        {
            int x = 0;
            const char *stop = 0;
            while(*s!=0 && *s!='\n')
            {
                int c = sReadUTF8(s);
                x += font->GetWidth(c);
                if(x>w && stop)
                {
                    s = stop;
                    break;
                }
                if(c==' ')
                    stop = s;
            }
        }
        else
        {
            while(*s!=0 && *s!='\n')
                s++;
        }
        li.End = s-tstart;
        maxx = sMax(maxx,font->GetWidth(tstart+li.Start,li.End-li.Start));
        Lines.Add(li);

        if(Editor.Cursor>=li.Start && Editor.Cursor<=li.End)
        {
            int w = font->GetWidth(tstart+li.Start,Editor.Cursor-li.Start);
            int x0 = x + w;
            int x1 = x0;
            if(Editor.Overwrite)
            {
                if(Editor.Cursor==li.End)
                    x1 = x0 + font->CellHeight/2;
                else
                    x1 = x0 + font->GetWidth(Editor.Text->Get()+Editor.Cursor,1);
            }
            else
            {
                x0--;
                x1++;
            }
            CursorRect.Init(x0,y,x1,y+font->CellHeight);
        }
        y += font->Advance;

        finalline = 0;
        if(*s=='\n')
        {
            s++;
            finalline = 1;
        }
    }
    if(finalline)
    {
        LineInfo li;
        li.End = li.Start = s-tstart;
        Lines.Add(li);
        if(Editor.Cursor>=li.Start)
        {
            if(Editor.Overwrite)
                CursorRect.Init(x,y,x+font->CellHeight/2,y+font->CellHeight);
            else
                CursorRect.Init(x-1,y,x+1,y+font->CellHeight);
        }
    }

    ReqSizeX = maxx + 2 + font->Advance/2 + LineNumberMargin;
    ReqSizeY = int(2 + font->Advance * Lines.GetCount());
    if(ControlStyle)
    {
        ReqSizeX += 2;
        ReqSizeY += 2;
    }
}

void sTextWindow::OnPaint(int layer)
{
    UpdateLines();

    sPainter *pnt = Painter();
    sGuiStyle *sty = Style();
    auto font = Font ? Font : sty->FixedFont;

    uint bcol = sty->Colors[sGC_Back];
    sRect cr(Client);

    if(ControlStyle)
    {
        pnt->SetLayer(layer+3);
        pnt->Frame(sty->Colors[sGC_Draw],Inner);
        cr.Extend(-1);
        bcol = sty->Colors[sGC_Button];
    }
    if(OverrideBackCol)
        bcol = OverrideBackCol;
    pnt->SetLayer(layer);

    pnt->Rect(bcol,cr);

    if(Editor.Text)
    {
        int x = cr.x0 + 1 + LineNumberMargin;
        int y = cr.y0 + 1;

        const char *tstart = Editor.Text->Get();
        const char *s0 = tstart;
        const char *s1 = tstart;
        if(Editor.SelectMode)
        {
            s0 += Editor.Cursor;
            s1 += Editor.SelectStart;
            if(s0>s1)
                sSwap(s0,s1);
        }

        pnt->SetLayer(layer+3);
        pnt->SetFont(font);
        uint col = sty->Colors[sGC_Text];

        int line = 1;
        for(auto &li : Lines)
        {
            uptr c0 = li.Start;
            uptr c1 = li.End;
            const char *t0 = tstart + c0;
            const char *t1 = tstart + c1;

            if(t1!=t0)
                pnt->Text(sPPF_Top|sPPF_Left,col,x,y,t0,t1-t0);
            if(LineNumbers)
            {
                sString<8> buffer;
                buffer.PrintF("%05d",line);
                pnt->Text(sPPF_Top|sPPF_Left,sty->Colors[sGC_Cursor],cr.x0+1,y,buffer);
            }

            if(Editor.Cursor>=c0 && Editor.Cursor<=c1 && ((Flags & sWF_Focus) || AlwaysDisplayCursor))
            {
                /*
                int w = font->GetWidth(t0,Editor.Cursor-c0);
                int x0 = x + w;
                int x1 = x0;
                if(Editor.Overwrite)
                {
                    if(Editor.Cursor==c1)
                        x1 = x0 + font->CellHeight/2;
                    else
                        x1 = x0 + font->GetWidth(Editor.Text->Get()+Editor.Cursor,1);
                }
                else
                {
                    x0--;
                    x1++;
                }
                CursorRect.Init(x0,y,x1,y+font->CellHeight);
                */
                pnt->SetLayer(layer+2);
                pnt->Rect(sty->Colors[sGC_Cursor],CursorRect);
                pnt->SetLayer(layer+3);
            }
            if(Editor.SelectMode && s0<t1 && s1>t0 )
            {
                const char *m0 = s0;
                if(m0<t0) m0 = t0;
                const char *m1 = s1;
                if(m1>t1) m1 = t1;
                int x0 = cr.x0+1+LineNumberMargin+font->GetWidth(t0,m0-t0);
                int x1 = x0 + font->GetWidth(m0,m1-m0);
                if(m1<s1 && *m1=='\n') 
                    x1 += font->CellHeight/2;
                if(x0<x1)
                {
                    pnt->SetLayer(layer+1);
                    pnt->Rect(sty->Colors[sGC_Select],sRect(x0,y,x1,y+font->CellHeight));
                    pnt->SetLayer(layer+3);
                }
            }
            line++;
            y += font->Advance;
        }
    }
}

void sTextWindow::ScrollToCursor()
{
    UpdateLines();
    if(!CursorRect.IsEmpty())
    {
        sRect r(CursorRect);
        r.Extend(Style()->FixedFont->Advance);
        ScrollTo(r);
    }
}

/****************************************************************************/

void sTextWindow::DragClick(const sDragData &dd)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        Editor.SelectStart = Editor.Cursor = FindClick(dd.StartX,dd.StartY);
        Editor.SelectMode = 0;
        DragMode = 1;
        Update();
        break;
    case sDM_Drag:
        Editor.Cursor = FindClick(dd.PosX,dd.PosY);
        if(Editor.SelectStart!=Editor.Cursor)
            Editor.SelectMode = 1;
        Update();
        break;
    case sDM_Stop:
        DragMode = 0;
        break;
    default:
        break;
    }
}

/****************************************************************************/
/****************************************************************************/

sTextControl::~sTextControl()
{
}

void sTextControl::Construct()
{
    Pool = 0;
    sTextWindow::Set(&Text);
    Editor.UpdateTargetMsg = sGuiMsg(this,&sTextControl::UpdateTarget);
}

sTextControl::sTextControl()
{
    Construct();
}

sTextControl::sTextControl(sPoolString &pool)
{
    Construct();
    Pool = &pool;
    Text.Set(*Pool);
}

sTextControl::sTextControl(const sStringDesc &desc)
{
    Construct();
    Desc = desc;
    Text.Set(Desc.Buffer);
}

void sTextControl::SetBuffer(sPoolString &pool)
{
    Construct();
    Pool = &pool;
    Text.Set(*Pool);
}

void sTextControl::SetBuffer(const sStringDesc &desc)
{
    Construct();
    Desc = desc;
    Text.Set(Desc.Buffer);
}

void sTextControl::SetText(const char *str)
{
    Text.Set(str);
    sTextWindow::Set(&Text);
}

void sTextControl::UpdateTarget()
{
    if(Desc.Buffer)
        sCopyString(Desc,Text.Get());
    if(Pool)
        *Pool = Text.Get();
}

/****************************************************************************/

sStaticTextControl::sStaticTextControl()
{
    MultiPainter = new sPainterMultiline();
    OldWidth = -1;
}

sStaticTextControl::sStaticTextControl(const char *t)
{
    MultiPainter = new sPainterMultiline();
    OldWidth = -1;
    MultiPainter->Init(t,Style()->Font);
}

sStaticTextControl::~sStaticTextControl()
{
    delete MultiPainter;
}

void sStaticTextControl::Init(const char *t)
{
    MultiPainter->Init(t,Style()->Font);
}

void sStaticTextControl::OnPaint(int layer)
{
    if(OldWidth!=Client.SizeX())
    {
        OldWidth = Client.SizeX();
        MultiPainter->Layout(OldWidth);
    }
    Painter()->SetLayer(layer);
    Painter()->Rect(Style()->Colors[sGC_Back],Client);
    Painter()->SetLayer(layer+1);
    sRect r(Client);
    r.x0 += Style()->Font->CellHeight/2;
    r.x1 -= Style()->Font->CellHeight/2;
    MultiPainter->Print(Painter(),r,Style()->Colors[sGC_Text]);
}

/****************************************************************************/
}
