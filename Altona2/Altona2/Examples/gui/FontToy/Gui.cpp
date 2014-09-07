/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/GraphicsHelper.hpp"
#include "Altona2/Libs/Util/UtilShaders.hpp"
#include "Gui.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void wTextWindow::OnPaint(int layer)
{
    auto pnt = Painter();

    pnt->SetLayer(layer+0);
    pnt->Rect(0xffffffff,Client);
    pnt->SetLayer(layer+1);
    pnt->SetFont(sPainterFontPara(App->OldFont));
    pnt->Rect(0xff000000,sRect(Client.x0,Client.CenterY(),Client.x1,Client.CenterY()+1));

    auto r = Client;
    r.y1 = r.CenterY();
    pnt->Text(App->PaintFlags,0xff000000,r,App->Text.Get());

    r = Client;
    r.y0 = r.CenterY();
    pnt->SetFont(App->Font);
    pnt->Text(App->PaintFlags,0xff000000,r,App->Text.Get());
}

/****************************************************************************/


MainWindow::MainWindow()
{
    Text.Print("Today!");

    ParaWin = new sGridFrame;
    ParaWin->AddScrolling(0,1);
    ParaWin->AddBorder(new sFocusBorder);
//    TextWin = new sTextWindow(&Text);
    TextWin = new wTextWindow(this);
    TextWin->AddBorder(new sFocusBorder);

    auto v = new sVSplitterFrame;
    v->AddChild(ParaWin);
    v->AddChild(TextWin);
    v->SetSize(0,500);
    
    AddChild(v);

    AddKey("Exit",sKEY_Escape|sKEYQ_Shift,sGuiMsg(this,&MainWindow::CmdExit));

    FontSize = 24;
    FontName = "Arial";
    FontFlags = sSFF_Antialias;
    Zoom = 1.0f;
    Blur = 1.0f;
    Outline = 0.0f;
    OldFont = 0;
    Font = 0;
    PaintFlags = 0;
    SetPara();
}

void MainWindow::OnInit()
{
    CmdUpdateFontResource();
}

MainWindow::~MainWindow()
{
    delete OldFont;
}

void MainWindow::SetPara()
{
    sGridFrameHelper gh(ParaWin);

    gh.Group("Font");
    gh.DoneMsg = sGuiMsg(this,&MainWindow::CmdUpdateFontResource);
    gh.Label("Name");
    gh.String(FontName);
    gh.Label("Generated Size");
    gh.Int(FontSize,1,128,0.25f,24);
    gh.Label("Flags");
    gh.Choice(FontFlags,"-|bold:*1-|italics:*2-|Anti Alias:*16-|Distance Field");
    gh.Choice(FontFlags,"*17-|MSAA");
//    gh.Button(sGuiMsg(this,&MainWindow::CmdUpdateFont),"Generate!");
    gh.DoneMsg = sGuiMsg();

    gh.ChangeMsg = sGuiMsg(this,&MainWindow::CmdUpdateFont);
    gh.Group("Test");
    gh.Label("Zoom");
    gh.Float(Zoom,0,16,0.01f,1.0f);
    gh.Label("Blur");
    gh.Float(Blur,0,16,0.01f,1.0f);
    gh.Label("Outline");
    gh.Float(Outline,0,16,0.01f,1.0f);
    gh.Label("MSAA");
    gh.ChangeMsg = sGuiMsg();
    gh.Label("Flags");
    gh.Choice(PaintFlags,"*0Center|Left|Right:*2Center|Top|Bottom");
    gh.NextLine();
    gh.Choice(PaintFlags,"*4-|Space:*5-|Baseline:*6-|Integer");
    gh.NextLine();
    gh.Choice(PaintFlags,"*7Horizontal|Downwards|Upwards");


    gh.ExtendRight();
    gh.FullWidth();
    gh.Text(Text);

}

void MainWindow::CmdUpdateFontResource()
{
    sDelete(OldFont);
    OldFont = new sPainterFont(Adapter(),FontName,FontFlags,FontSize);
    CmdUpdateFont();
}

void MainWindow::CmdUpdateFont()
{
    auto old = Font;

    sFontPara para;
    para.Name = FontName;
    para.Size = FontSize;
    para.Zoom = Zoom;
    para.Blur = Blur;
    para.Outline = Outline;
    para.Flags = FontFlags;
    Font = Painter()->GetFont(para);

    Painter()->ReleaseFont(old);
}

/****************************************************************************/
