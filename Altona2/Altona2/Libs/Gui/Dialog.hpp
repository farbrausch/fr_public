/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_DIALOG_HPP
#define FILE_ALTONA2_LIBS_GUI_DIALOG_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Window.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct sDialogPara
{
    const char *Title;
    const char *Text;

    int ButtonCount;
    sGuiMsg ButtonMsg[3];
    const char *ButtonLabel[3];

    int StringCount;
    sStringDesc StringDesc[2];
    const char *StringLabel[2];

    sDialogPara();
    void Init(const char *title,const char *text);
    void InitOk(const sGuiMsg &ok);
    void InitOkCancel(const sGuiMsg &ok,const sGuiMsg &cancel);
    void InitYesNo(const sGuiMsg &yes,const sGuiMsg &no);
    void InitSaveDiscardCancel(const sGuiMsg &save,const sGuiMsg &discard,const sGuiMsg &cancel);
    void AddString(const char *label,const sStringDesc &desc);
    void AddButton(const char *label,const sGuiMsg &msg);
    sOBSOLETE sWindow *Start();
    sWindow *Start(sWindow *master);      // use this to give back focus after dialog is closed.
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_DIALOG_HPP

