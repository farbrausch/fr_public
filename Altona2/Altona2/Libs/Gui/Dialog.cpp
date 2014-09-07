/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Gui.hpp"
#include "Altona2/Libs/gui/dialog.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sDialogWindow : public sWindow
{
    friend struct sDialogPara;
    sDialogPara Para;
    sButtonControl *Buttons[3];
    sStringControl *Strings[2];
    sRect LabelRect[2];

    void CmdButton(int n);
    sPainterMultiline TextML;
public:
    sDialogWindow(const sDialogPara &para);
    ~sDialogWindow();  
    void OnInit();

    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
};

/****************************************************************************/

sDialogWindow::sDialogWindow(const sDialogPara &para)
{
    Para = para;
    sClear(Buttons);
    sClear(Strings);

    AddBorder(new sTitleBorder(Para.Title)); 
    AddBorder(new sSizeBorder()); 

    for(int i=0;i<Para.ButtonCount;i++)
    {
        Buttons[i] = new sButtonControl(sGuiMsg(this,&sDialogWindow::CmdButton,i),Para.ButtonLabel[i]);
        AddChild(Buttons[i]);
    }
    for(int i=0;i<Para.StringCount;i++)
    {
        Strings[i] = new sStringControl(Para.StringDesc[i]);
        AddChild(Strings[i]);
        if(i==Para.StringCount-1)
            Strings[i]->AddKey("Ok",sKEY_Enter,sGuiMsg(this,&sDialogWindow::CmdButton,0));
    }

    if(Para.ButtonCount>=1)
        AddKey(Para.ButtonLabel[0],sKEY_Enter,sGuiMsg(this,&sDialogWindow::CmdButton,0));
    if(Para.ButtonCount>=2)
        AddKey(Para.ButtonLabel[1],sKEY_Escape,sGuiMsg(this,&sDialogWindow::CmdButton,1));
    AddKey("Help",sKEY_RMB,sGuiMsg(this,&sWindow::CmdHelp));
}

void sDialogWindow::OnInit()
{
    TextML.Init(Para.Text,Style()->Font);
}

sDialogWindow::~sDialogWindow()
{
}

void sDialogWindow::CmdButton(int n)
{
    if(n>=0 && n<Para.ButtonCount)
    {
        CmdKill();
        Para.ButtonMsg[n].Post();
    }
}

/****************************************************************************/

void sDialogWindow::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = 0;
}

void sDialogWindow::OnLayoutChilds()
{
    int h = Style()->Font->CellHeight+4;
    int y = Client.y1;
    if(Para.ButtonCount>0)
    {
        int y1 = y; y-=h;
        int y0 = y;

        static const int pos[3] = { 2,0,1 };

        for(int i=0;i<Para.ButtonCount;i++)
        {
            Buttons[i]->Outer.x0 = Client.x0 + (pos[i]  )*Client.SizeX() / 3;
            Buttons[i]->Outer.x1 = Client.x0 + (pos[i]+1)*Client.SizeX() / 3;
            Buttons[i]->Outer.y0 = y0;
            Buttons[i]->Outer.y1 = y1;
        }
    }
    for(int i=Para.StringCount-1;i>=0;i--)
    {
        int y1 = y; y-=h;
        int y0 = y;
        int x = Client.x0 + Client.SizeX()/4;
        LabelRect[i].Set(Client.x0,y0,x,y1);
        Strings[i]->Outer.Set(x,y0,Client.x1,y1);
    }
}

void sDialogWindow::OnPaint(int layer)
{
    Painter()->SetLayer(layer);
    Painter()->Rect(Style()->Colors[sGC_Back],Client);

    Painter()->SetLayer(layer+2);
    sRect r(Client);
    uint col = Style()->Colors[sGC_Text];
    r.Extend(-Style()->Font->CellHeight/2);
    TextML.Print(Painter(),r,col);

    for(int i=0;i<Para.StringCount;i++)
        Painter()->Text(sPPF_Left,col,LabelRect[i],Para.StringLabel[i]);
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sDialogPara::sDialogPara()
{
    Title = "";
    Text = "";
    ButtonCount = 0;
    StringCount = 0;
    for(int i=0;i<sCOUNTOF(ButtonLabel);i++)
        ButtonLabel[i] = "";
    for(int i=0;i<sCOUNTOF(StringLabel);i++)
        StringLabel[i] = "";
}

void sDialogPara::Init(const char *title,const char *text)
{
    Title = title;
    Text = text;
}

void sDialogPara::InitOk(const sGuiMsg &ok)
{
    ButtonCount = 1;
    ButtonMsg[0] = ok;
    ButtonLabel[0] = "ok";
}

void sDialogPara::InitOkCancel(const sGuiMsg &ok,const sGuiMsg &cancel)
{
    ButtonCount = 2;
    ButtonMsg[0] = ok;
    ButtonLabel[0] = "ok";
    ButtonMsg[1] = cancel;
    ButtonLabel[1] = "cancel";
}

void sDialogPara::InitYesNo(const sGuiMsg &yes,const sGuiMsg &no)
{
    ButtonCount = 2;
    ButtonMsg[0] = yes;
    ButtonLabel[0] = "yes";
    ButtonMsg[1] = no;
    ButtonLabel[1] = "no";
}

void sDialogPara::InitSaveDiscardCancel(const sGuiMsg &save,const sGuiMsg &discard,const sGuiMsg &cancel)
{
    ButtonCount = 3;
    ButtonMsg[0] = save;
    ButtonLabel[0] = "save";
    ButtonMsg[1] = discard;
    ButtonLabel[1] = "discard";
    ButtonMsg[2] = cancel;
    ButtonLabel[2] = "cancel";
}

void sDialogPara::AddString(const char *label,const sStringDesc &desc)
{
    sASSERT(StringCount<sCOUNTOF(StringDesc));
    StringDesc[StringCount] = desc;
    StringLabel[StringCount] = label;
    StringCount++;
}

void sDialogPara::AddButton(const char *label,const sGuiMsg &msg)
{
    sASSERT(ButtonCount<3);
    ButtonMsg[ButtonCount] = msg;
    ButtonLabel[ButtonCount] = label;
    ButtonCount++;
}

sWindow *sDialogPara::Start()
{
    return Start(0);
}

sWindow *sDialogPara::Start(sWindow *master)
{
    sDialogWindow *dlg = new sDialogWindow(*this);
    dlg->IndirectParent = master;
    Gui->AddDialog(dlg,sRect(100,100,400,300));
    if(StringCount>=1)
    {
        Gui->SetFocus(dlg->Strings[StringCount-1]);
        dlg->Strings[StringCount-1]->CmdStartEditing();
    }
    return dlg;
}

/****************************************************************************/



/****************************************************************************/
}
