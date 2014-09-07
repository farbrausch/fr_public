/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_HELPERS_HPP
#define FILE_ALTONA2_LIBS_GUI_HELPERS_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Frames.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   open, load, save, and exit with respect to change of the document  ***/
/***                                                                      ***/
/****************************************************************************/

class sDocumentBase : public sGCObject
{
    void CmdOpen();
    void CmdOpen2();                // call this to load "Filename"
    void CmdNew();
    void CmdSaveAs();
    void CmdSave();
    void CmdSave2();
    void CmdExit();
    void CmdExit2();
    void CmdSaveAndExit();
	sWindow *Master;
protected:
    ~sDocumentBase();
public:

    // establishing the class

    sDocumentBase(sWindow *master);
    void Tag();
    virtual void Clear()=0;
    virtual void ResetGui()=0;
    bool Load();
    bool Save();

    // user interface

    void AddFileMenu(sMenuFrame *mf);
    void AddGlobals(sWindow *w);
    void Change();

    // serialization

    virtual void Serialize(sReader &s)=0;
    virtual void Serialize(sWriter &s)=0;

    // state

    bool Changed;
    sString<sMaxPath> Filename;
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_HELPERS_HPP

