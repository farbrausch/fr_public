/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Gui.hpp"
#include "helpers.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sDocumentBase::sDocumentBase(sWindow *master)
{
    Changed = 0;
	Master = master;
}

sDocumentBase::~sDocumentBase()
{
}

void sDocumentBase::Tag()
{
    sGCObject::Tag();
}

void sDocumentBase::Change()
{
    Changed = 1;
}

void sDocumentBase::AddFileMenu(sMenuFrame *mf)
{
    mf->AddItem(sGuiMsg(this,&sDocumentBase::CmdOpen),0,"Open ...");
    mf->AddItem(sGuiMsg(this,&sDocumentBase::CmdNew),0,"New");
    mf->AddItem(sGuiMsg(this,&sDocumentBase::CmdSave),'s'|sKEYQ_Ctrl,"Save");
    mf->AddItem(sGuiMsg(this,&sDocumentBase::CmdSaveAs),0,"Save As ...");
    mf->AddSpacer();
    mf->AddItem(sGuiMsg(this,&sDocumentBase::CmdExit),sKEY_Escape|sKEYQ_Shift,"Exit");
}

void sDocumentBase::AddGlobals(sWindow *w)
{
    w->AddKey("Exit",sKEY_Escape|sKEYQ_Shift,sGuiMsg(this,&sDocumentBase::CmdExit));
    w->AddKey("Save",'s'|sKEYQ_Ctrl,sGuiMsg(this,&sDocumentBase::CmdSave));
}

bool sDocumentBase::Load()
{
    sReader s(Filename);
    Serialize(s);
    return s.Finish();
}

bool sDocumentBase::Save()
{
    sWriter s(Filename);
    Serialize(s);
    return s.Finish();
}

void sDocumentBase::CmdOpen()
{
    sFileDialogPara para;
    para.Headline = "Open Project";
    para.Filename = Filename;
    para.OkMsg = sGuiMsg(this,&sDocumentBase::CmdOpen2);
    para.Start(Master);
}

void sDocumentBase::CmdOpen2()
{
    if(!Load())
    {
        sDialogPara dp;
        dp.Init("Error","Failed to open project");
        dp.InitOk(sGuiMsg());
        dp.Start(Master);
    }
    ResetGui();
}

void sDocumentBase::CmdNew()
{
    Clear();
    Changed = 0;
    ResetGui();
}

void sDocumentBase::CmdSaveAs()
{
    sFileDialogPara para;
    para.Headline = "Save Project";
    para.Filename = Filename;
    para.OkMsg = sGuiMsg(this,&sDocumentBase::CmdSave);
    para.Start(Master);
}


void sDocumentBase::CmdSave()
{
    if(!Save())
    {
        sDialogPara dp;
        dp.Init("Error","Failed to save project");
        dp.InitOk(sGuiMsg());
        dp.Start(Master);
    }
    else
    {
        Gui->QuickMessage("Save Ok");
        Changed = 0;
    }
}

void sDocumentBase::CmdSave2()
{
}

void sDocumentBase::CmdExit()
{
    if(Changed)
    {
        sDialogPara dp;
        dp.Init("Do you really want to quit?","You will lose changes");
        dp.InitSaveDiscardCancel(sGuiMsg(this,&sDocumentBase::CmdSaveAndExit),sGuiMsg(this,&sDocumentBase::CmdExit2),sGuiMsg());
        dp.Start(Master);
    }
    else
    {
        CmdExit2();
    }
}

void sDocumentBase::CmdExit2()
{
    sExit();
}

void sDocumentBase::CmdSaveAndExit()
{
    CmdSave();
    CmdExit2();
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/
}
