/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_FILE_HPP
#define FILE_ALTONA2_LIBS_GUI_FILE_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Window.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sFileWindow : public sPassthroughFrame
{
    struct Entry
    {
        sString<256> Name;
        int64 Size;
        int Flags;
        uint64 LastWriteTime;
        sListWindowElementInfo ListInfo;
        bool LoadMe;

        Entry(const sDirEntry *de);
    };
    struct Entry2 : public Entry
    {
        sString<sMaxPath> Path;

        Entry2(const sDirEntry *de,const Entry2 *parent);
    };

    sArray<Entry2 *> Tree;
    sArray<Entry *> Files;
    sListWindow<Entry2> *TreeWin;
    sListWindow<Entry> *FilesWin;

    void LoadTree(Entry2 *parent,const char *path);
    void LoadFiles();
    void MakePath(int treeindex,const sStringDesc &desc);
    Entry2 *FindDir(const char *dir);
public:
    sFileWindow();
    ~sFileWindow();

    void OpenPath(const char *path);
    void OpenPathAndFile(const char *path,const char *file);
    void GetFile(const sStringDesc &desc);
    void GetPath(const sStringDesc &desc);

    void OnCalcSize();

    void CmdSelectTree();
    void CmdUpdateTree();
    void CmdSelectFile();
    void CmdOk();

    sGuiMsg FileMsg;
    sGuiMsg TreeMsg;
    sGuiMsg OkMsg;

    sString<sMaxPath> Path;
};

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

enum sFileDialogFlags
{
    sFDF_ModeMask       = 0x000f,
    sFDF_ModeLoad       = 0x0001,
    sFDF_ModeSave       = 0x0002,
    sFDF_ModeDir        = 0x0003,
    sFDF_Exists         = 0x0010,
    sFDF_AllowRelative  = 0x0020,
    sFDF_ForceRelative  = 0x0040
};

struct sFileDialogPara
{
    sStringDesc Filename;
    sStringDesc BaseDir;
    int Flags;
    const char *Extensions;
    const char *Headline;
    sGuiMsg OkMsg;
    sGuiMsg CancelMsg;

    sFileDialogPara();
    void Init();
    sOBSOLETE void Start();
	void Start(sWindow *master);
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_FILE_HPP

