/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Config.hpp"
#include "Gui.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

MainWindow::MainWindow()
{
    ParaWin = new sGridFrame;
    ParaWin->AddScrolling(0,1);
    ParaWin->AddBorder(new sFocusBorder);
    

    AddChild(ParaWin);

    AddKey("Exit",sKEY_Escape|sKEYQ_Shift,sGuiMsg(this,&MainWindow::CmdExit));

    UseCustomPath = false;
    sGetSystemPath(sSP_User,UserPath);
    UserPath.Add("/altona2");
    CustomPath = UserPath;
    DefaultStyle = 0;

    sEnableAltonaConfig();

    CustomPath = sGetConfigPath();
    UseCustomPath = (sCmpStringP(CustomPath,UserPath)!=0) && CustomPath[0]!=0;
    const char *style = sGetConfigString("GuiStyle");
    if(style==0) style = "";

    for(int i=0;;i++)
    {
        const char *s = sGetDefaultStyleName(i);
        if(!s)
            break;
        if(i!=0)
            StyleChoice.Print("|");
        StyleChoice.Print(s);

        if(sCmpStringI(s,style)==0)
            DefaultStyle = i;
    }


    SetPara();
}

void MainWindow::OnInit()
{
}

MainWindow::~MainWindow()
{
}

void MainWindow::SetPara()
{
    sGridFrameHelper gh(ParaWin);
    gh.Normal = 6;
    gh.Label("Installation Dir");
    gh.Choice(UseCustomPath,"home dir|custom dir")->ChangeMsg = sGuiMsg(this,&MainWindow::SetPara);
    if(UseCustomPath)
        gh.String(CustomPath);
    else
        gh.ConstString(UserPath);
    gh.Label("Gui Style");
    gh.Choice(DefaultStyle,StyleChoice.Get());

    gh.NextLine();
    gh.NextLine();
    gh.Normal = 3;
    gh.Button(sGuiMsg(this,&MainWindow::CmdExit),"Cancel");
    gh.Button(sGuiMsg(this,&MainWindow::CmdAction),"Ok");
}

void MainWindow::CmdAction()
{
    sString<sMaxPath> user;
    sString<sMaxPath> path;
    sString<sMaxPath> file;
    sTextBuffer config;

    if(UseCustomPath)
        path = CustomPath;
    else
        path = UserPath;

    config.PrintF("GuiStyle = %s;\n",sGetDefaultStyleName(DefaultStyle));

    if(!sCheckDir(path))
        if(!sMakeDir(path)) 
            return;

    file.PrintF("%s/FontCache",path);
    if(!sCheckDir(file))
        if(!sMakeDir(file)) 
            return;

    file.PrintF("%s/Altona2Config.txt",path);
    sSaveTextUTF8(file,config.Get());

    sGetSystemPath(sSP_User,user);
    file.PrintF("%s/Altona2.txt",user);
    sSaveTextUTF8(file,path);
}

/****************************************************************************/
