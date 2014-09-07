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
#include "Doc.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

MainWindow::MainWindow()
{
}

void MainWindow::OnInit()
{
    Font = 0;
    FontGen = 0;
    sVSplitterFrame *vbox = new sVSplitterFrame;
    sHSplitterFrame *hbox = new sHSplitterFrame;
    ParaWin = new sGridFrame();
    FontWin = new FontWindow(this);
    FontWin->Fps = 0;
    FontWin->MsCount = 0;
    TextWin = new TextWindow(this);

    sToolBorder *tb = new sToolBorder;
    tb->AddChild(new sButtonControl(sGuiMsg(this,&MainWindow::CmdEdit),"Edit",sBS_Menu));

    hbox->AddChild(FontWin);
    hbox->AddChild(TextWin);
    vbox->AddChild(ParaWin);
    vbox->AddChild(hbox);
    vbox->SetSize(0,400);
    AddChild(vbox);
    AddBorder(tb);

    AddKey("Exit",sKEY_Escape|sKEYQ_Shift,sGuiMsg(this,&sWindow::CmdExit));

    FontNames.Add("Arial");
    FontNames.Add("New Times Roman");
    FontNames.Add("Courier New");
    FontNames.Add("Marlet");
    FontNames.Add("Verdana");
    FontNames.Add("Tahoma");
    FontNames.Add("Traditional Arabic");
    FontNames.Add("MS Gothic");
    FontNames.Add("Batang");
    FontNames.Add("SimSun");

    Text = "@WsO?";
    Para.FontName = "Times New Roman";
    Para.SizeY = 64;
    Para.Inside = 10;
    Para.Outside = 5;
    Para.Intra = 1;
    Para.MaxSizeX = 0x4000;
    Para.MaxSizeY = 0x4000;
    Para.FontFlags = sSFF_DistanceField;
    Para.Format = sTextureFontPara::F32;
    Para.Generate = sTextureFontPara::Images;
    LastChar = 0x200;

    FontGen = new sTextureFontGenerator();

    CmdSetPara();
    CmdUpdate();
}

MainWindow::~MainWindow()
{
    delete Font;
    delete FontGen;
}

void MainWindow::CmdEdit()
{
    sMenuFrame *mf = new sMenuFrame(this);
    mf->AddItem(sGuiMsg(this,&MainWindow::CmdUpdate),0,"Update");
    mf->AddSpacer();
    mf->AddItem(sGuiMsg(this,&MainWindow::CmdExit),0,"Exit");
    mf->StartMenu();
}

void MainWindow::CmdUpdate()
{
    sDelete(Font);

    int t0 = sGetTimeMS();

    sTextureFontPara pc(Para);

    if(!(pc.FontFlags & sSFF_DistanceField))
        pc.FontSize = pc.SizeY;

//    for(int i=0;i<1000;i++)
    {
        sDelete(Font);
        FontGen->Begin(pc);
        for(int i=0x20;i<LastChar;i++)
            FontGen->AddLetter(i);
        Font = FontGen->End();
    }
    int t1 = sGetTimeMS();
    sDPrintF("time: %d\n",t1-t0);

    Font->CreateTexture(Adapter(),Para);
    Font->CreateMtrl();
}

void MainWindow::CmdSetPara()
{
    ParaWin->Clear();
    ParaWin->GridX = 9;
    sGridFrameHelper gh(ParaWin);

    gh.Left = 2;
    gh.Normal = 2;
    gh.Wide = 6;
    gh.Right = 9;

    gh.Group("Font Generation");
    gh.Label("Font");
    gh.String(Para.FontName);
    gh.Box(sGuiMsg(this,&MainWindow::CmdFont),"...");
    gh.Label("Font Modifier");
    gh.Choice(Para.FontFlags,"*0-|bold:*1-|italics:*2-|antialias:*16-|distance field");
    gh.Label("Text");
    gh.String(Text);
    gh.Label("Size");
    gh.Int(Para.SizeY,1,0x4000,1);
    gh.Label("DF Inside");
    gh.Float(Para.Inside,0,32,0.25f);
    gh.Label("DF Outside");
    gh.Float(Para.Outside,0,32,0.25f);
    gh.Label("Intra");
    gh.Int(Para.Intra,0,0x4000,1);
    gh.Label("Last Char");
    gh.Choice(LastChar,"128 0x80 (US)|256 0x100 (Western Europe)|512 0x200|4096 0x1000 (Europe)|16384 0x4000 (Europe+some Japanese)|65536 0x10000 (All)");
    gh.Label("Quality");
    gh.Choice(Para.Format," 8 bit| 16 bit| 32 bit float");
    gh.NextLine();
    gh.Button(sGuiMsg(this,&MainWindow::CmdUpdate),"Generate!");

    gh.Group("Rendering");
    gh.Label("BackColor");
    gh.Color(FontWin->BackColor);
    gh.Label("Flags");
    gh.Choice(FontWin->Fps,"-|FPS");
    gh.Label("Multisample");
    gh.ChangeMsg = sGuiMsg(this,&MainWindow::CmdSetPara);

    sString<2048> buffer = "0 off";
    for(int i=1;i<4;i++)
        if(FontWin->GetMsQuality(1<<i)>0)
            buffer.AddF("|%d %d",1<<i,1<<i);
    gh.Choice(FontWin->MsCount,sPoolString(buffer));

    int ms = FontWin->GetMsQuality(FontWin->MsCount);
    buffer = "0 0";
    for(int i=1;i<ms;i++)
        buffer.AddF("|%d %d",i,i);
    gh.Choice(FontWin->MsQuality,sPoolString(buffer));

    gh.Group("Font Styling");

    gh.ChangeMsg = sGuiMsg();
    gh.Label("Outline");
    gh.Float(TextWin->Para.Outline,-255.0f,255.0f,0.01f);
    gh.Label("Blur");
    gh.Float(TextWin->Para.Blur,0.0f,255.0f,0.01f,1.0f);
//    gh.Label("Correct Zoom");
//    gh.Choice(FontWin->CorrectZoom,"Blur|Correct");
    gh.Label("Multisample");
    gh.Choice(TextWin->Para.Multisample,"-|MS");

    gh.FullWidth();
    gh.ExtendRight();
    gh.Text(TextWin->Text);
}

void MainWindow::CmdFont()
{
    auto mf = new sMenuFrame(this);
    int n = 0;
    for(auto s : FontNames)
        mf->AddItem(sGuiMsg(this,&MainWindow::CmdFont2,n++),0,s);
    mf->StartMenu();
}

void MainWindow::CmdFont2(int n)
{
    if(FontNames.IsIndex(n))
    {
        Para.FontName = FontNames[n];
        Update();
    }
}

/****************************************************************************/
/****************************************************************************/

FontWindow::FontWindow(MainWindow *app)
{
    App = app;

    AddDrag("Zoom",sKEY_LMB,sGuiMsg(this,&FontWindow::DragZoom));
    AddDrag("Scroll",sKEY_MMB,sGuiMsg(this,&FontWindow::DragScroll));
    AddKey("Reset",'r',sGuiMsg(this,&FontWindow::CmdReset));
    AddKey("Distance Field Rendering",'d',sGuiMsg(this,&FontWindow::CmdDist));
    AddHelp();

    ZoomLin = 1.0f;
    Zoom = 1.0f;
    ScrollX = 0.0f;
    ScrollY = 0.0f;

    UseDist = 0;
    CorrectZoom = 1;

    Context = 0;
    cbv0 = 0;
    cbp0 = 0;
    FlatMtrl = 0;
    DistMtrl = 0;
}

void FontWindow::OnInit()
{
    s3dWindow::OnInit();
    Context = Adapter()->ImmediateContext;
    cbv0 = new sCBuffer<sFixedMaterialVC>(Adapter(),sST_Vertex,0);
    cbp0 = new sCBuffer<sDistanceFieldShader_cbps>(Adapter(),sST_Pixel,0);
    FlatMtrl = new sFixedMaterial(Adapter());
    FlatMtrl->SetTexturePS(0,0,sSamplerStatePara(sTF_Tile|sTF_Point));
    FlatMtrl->SetState(sRenderStatePara(sMTRL_CullOff|sMTRL_DepthOff,sMB_Off));
    FlatMtrl->FMFlags = sFMF_TexMonochrome;
    FlatMtrl->Prepare(Adapter()->FormatPCT);
    DistMtrl = new sMaterial(Adapter());
    DistMtrl->SetShaders(sDistanceFieldShader.Get(0));
    DistMtrl->SetTexturePS(0,0,sSamplerStatePara(sTF_Tile|sTF_Linear));
    DistMtrl->SetState(sRenderStatePara(sMTRL_CullOff|sMTRL_DepthOff,0));//sMB_PMAlpha));
    DistMtrl->Prepare(Adapter()->FormatPCT);

    Outline = 0.0f;
    Blur = 1.0f;
}

FontWindow::~FontWindow()
{
    delete cbv0;
    delete cbp0;
    delete FlatMtrl;
    delete DistMtrl;
}

void FontWindow::Render(const sTargetPara &tp)
{

    float iblur = Blur>0 ? 1.0f/Blur : 1e30f;
    float zoom = CorrectZoom ? Zoom : 1.0f;

    float sub = App->Font->Sub - App->Font->IMul/zoom*(Outline+Blur*0.5f);
    float mul = App->Font->Mul*zoom*iblur;

    cbv0->Map();
    cbv0->Data->MS2SS.SetViewportPixels(tp.SizeX,tp.SizeY);
    cbv0->Unmap();
    cbp0->Map();
    cbp0->Data->Para.Set(sub,mul,0,0);
    cbp0->Unmap();

    sMaterial *mtrl = UseDist ? DistMtrl : FlatMtrl;
    mtrl->UpdateTexturePS(0,App->Font->Texture);

    sVertexPCT vb[2];
    vb[0].Set(0,0,0,0xffffffff,0,0);
    vb[1].Set((float)App->Font->SizeX,(float)App->Font->SizeY,0,0xffffffff,1,1);
    Adapter()->DynamicGeometry->Quad(mtrl,vb[0],vb[1],cbv0,cbp0);
}

/****************************************************************************/

void FontWindow::DragZoom(const sDragData &dd)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        DragStartX = ScrollX;
        DragStartY = ScrollY;
        DragStart = ZoomLin;
        break;
    case sDM_Drag:
        ZoomLin = sMax(0.0f,DragStart + dd.DeltaY*0.01f);
        Zoom = sPow(ZoomLin,2.0f);
        break;
    case sDM_Stop:
        break;
    }
}

void FontWindow::DragScroll(const sDragData &dd)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        DragStartX = ScrollX;
        DragStartY = ScrollY;
        break;
    case sDM_Drag:
        ScrollX = DragStartX + float(dd.DeltaX);
        ScrollY = DragStartY + float(dd.DeltaY);
        break;
    case sDM_Stop:
        break;
    }
}

void FontWindow::CmdReset()
{
    ScrollX = 0.0f;
    ScrollY = 0.0f;
    ZoomLin = 1.0f;
    Zoom = 1.0f;
}

/****************************************************************************/
/****************************************************************************/

TextWindow::TextWindow(MainWindow *app)
{
    AddDrag("Zoom",sKEY_LMB,sGuiMsg(this,&TextWindow::DragZoom));
    AddDrag("Scroll",sKEY_MMB,sGuiMsg(this,&TextWindow::DragScroll,1));
    AddDrag("Scroll Slow",sKEY_MMB|sKEYQ_Shift,sGuiMsg(this,&TextWindow::DragScroll,10));
    AddKey("Reset",'r',sGuiMsg(this,&TextWindow::CmdReset));

    App = app;
    Context = 0;
    cbv0 = 0;
    cbp0 = 0;
    DistMtrl = 0;
    
    CmdReset();

    Text.Print("Maxwell's equations are a set of partial differential equations that, together with the Lorentz force law, form the foundation of classical electrodynamics, classical optics, and electric circuits.\n");
    Text.Print("These fields in turn underlie modern electrical and communications technologies.\n");
    Text.Print("Maxwell's equations describe how electric and magnetic fields are generated and altered by each other and by charges and currents.\n");
    Text.Print("They are named after the Scottish physicist and mathematician James Clerk Maxwell who published an early form of those equations between 1861 and 1862.\n");

    Para.Blur = 1.0f;
    Para.Outline = 0.0f;
}

void TextWindow::OnInit()
{
    s3dWindow::OnInit();
    Context = Adapter()->ImmediateContext;
    cbv0 = new sCBuffer<sFixedMaterialVC>(Adapter(),sST_Vertex,0);
    cbp0 = new sCBuffer<sDistanceFieldShader_cbps>(Adapter(),sST_Pixel,0);
    DistMtrl = new sMaterial(Adapter());
    DistMtrl->SetShaders(sDistanceFieldShader.Get(0));
    DistMtrl->SetTexturePS(0,0,sSamplerStatePara(sTF_Tile|sTF_Linear));
    DistMtrl->SetState(sRenderStatePara(sMTRL_CullOff|sMTRL_DepthOff,sMB_PMAlpha));
    DistMtrl->Prepare(Adapter()->FormatPCT);
}

TextWindow::~TextWindow()
{
    delete cbv0;
    delete cbp0;
    delete DistMtrl;
}

/****************************************************************************/

void TextWindow::DragZoom(const sDragData &dd)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        DragStartX = ScrollX;
        DragStartY = ScrollY;
        DragStart = ZoomLin;
        break;
    case sDM_Drag:
        ZoomLin = sMax(0.0f,DragStart + dd.DeltaY*0.01f);
        Zoom = sPow(ZoomLin,2.0f)*64;
        break;
    case sDM_Stop:
        break;
    }
}

void TextWindow::DragScroll(const sDragData &dd,int speed)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        DragStartX = ScrollX;
        DragStartY = ScrollY;
        break;
    case sDM_Drag:
        ScrollX = DragStartX + float(dd.DeltaX) / float(speed);
        ScrollY = DragStartY + float(dd.DeltaY) / float(speed);
        break;
    case sDM_Stop:
        break;
    }
}

void TextWindow::CmdReset()
{
    ScrollX = 0.0f;
    ScrollY = 0.0f;
    ZoomLin = 1.0f;
    Zoom = 64.0f;
}

/****************************************************************************/

void TextWindow::Render(const sTargetPara &tp)
{
    float Blur = 1.0f;
    float Outline = 0.0f;
    bool CorrectZoom = true;

    Para.Zoom = Zoom;
    Para.MS2SS.SetViewportPixels(tp.SizeX,tp.SizeY);

    const char *text = Text.Get();

    float y = App->Font->Baseline * Para.Zoom;
    while(*text)
    {
        const char *start = text;
        while(*text!='\n' && *text!=0)
            text++;
        App->Font->Print(0,y,Para,0xffffffff,start,text-start);

        if(*text=='\n')
            text++;

        y += App->Font->Advance * Para.Zoom;
    }
}

/****************************************************************************/
