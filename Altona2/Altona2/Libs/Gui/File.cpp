/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Gui.hpp"
#include "file.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sFileWindow::Entry::Entry(const sDirEntry *de)
{
    Name = de->Name;
    Size = de->Size;
    Flags = de->Flags;
    LastWriteTime = de->LastWriteTime;
    LoadMe = 0;
}

sFileWindow::Entry2::Entry2(const sDirEntry *de,const Entry2 *parent) : Entry(de)
{
}

/****************************************************************************/

sFileWindow::sFileWindow()
{
    Flags |= sWF_Top;

    TreeWin = new sListWindow<Entry2>(Tree,&Entry2::ListInfo,sLWF_Tree);
    TreeWin->AddScrolling(0,1);
    TreeWin->AddTree<256>("Name",150,0,&Entry2::Name);
    TreeWin->AddField("Path",150,0,&Entry2::Path);
    TreeWin->SelectMsg = sGuiMsg(this,&sFileWindow::CmdSelectTree);
    TreeWin->TreeMsg = sGuiMsg(this,&sFileWindow::CmdUpdateTree);
    /*
    TreeWin->AddField("Size",150,0,&Entry::Size);
    TreeWin->AddField("Flags",150,0,&Entry::Flags);
    TreeWin->AddField("Last Changed",150,0,&Entry::LastWriteTime);
    */
    FilesWin = new sListWindow<Entry>(Files,&Entry::ListInfo,sLWF_MultiSelect);
    FilesWin->AddScrolling(1,1);
    FilesWin->AddField("Name",150,0,&Entry::Name);
    FilesWin->AddField("Size",50,0,&Entry::Size);
    FilesWin->AddField("Flags",50,0,&Entry::Flags);
    FilesWin->AddField("Last Changed",150,0,&Entry::LastWriteTime);
    FilesWin->AddKey("Select",sKEY_LMB|sKEYQ_Double,sGuiMsg(this,&sFileWindow::CmdOk));
    FilesWin->AddHeader();
    FilesWin->SelectMsg = sGuiMsg(this,&sFileWindow::CmdSelectFile);
    sVSplitterFrame *v = new sVSplitterFrame;
    v->AddChild(TreeWin);
    v->AddChild(FilesWin);
    v->SetSize(0,200);
    AddChild(v);

    sArray<sDirEntry> dir;
    sLoadVolumes(dir);
    for(auto &de : dir)
    {
        Entry2 *e = new Entry2(&de,0);
        e->Path = de.Name;
        e->ListInfo.Opened = 0;
        Tree.Add(e);
        
        e = new Entry2(&de,e);
        e->LoadMe = 1;
        e->Name = "LoadMe";
        e->ListInfo.Opened = 0;
        e->ListInfo.Indent = 1;
        Tree.Add(e);
    }
}

sFileWindow::~sFileWindow()
{
    Tree.DeleteAll();
    Files.DeleteAll();
}

void sFileWindow::LoadTree(Entry2 *parent,const char *path)
{
    sArray<sDirEntry> dir;
    int indent = parent->ListInfo.Indent+1;
    int pos = Tree.FindEqualIndex(parent);
    sASSERT(pos>=0);
    if(Tree[pos]->ListInfo.Indent == Tree[pos+1]->ListInfo.Indent-1 && Tree[pos+1]->LoadMe)
    {
        Entry2 *e = Tree[pos+1];
        Tree.RemAtOrder(pos+1);
        if(e)
            delete e;
    }

    pos++;
    if(sLoadDir(dir,path))
    {
        for(auto &de : dir)
        {
            if(de.Flags & sDEF_Dir)
            {
                Entry2 *e = new Entry2(&de,parent);
                e->ListInfo.Indent = indent;
                e->ListInfo.Opened = 0;
                if(parent)
                {
                    e->Path = parent->Path;
                    e->Path.Add("/");
                    e->Path.Add(e->Name);
                }
                else
                {
                    e->Path = e->Name;
                }
                Tree.AddAt(e,pos++);

                e = new Entry2(&de,e);
                e->LoadMe = 1;
                e->Name = "LoadMe";
                e->ListInfo.Indent = indent+1;
                e->ListInfo.Opened = 0;
                Tree.AddAt(e,pos++);
            }
        }
    }
    TreeWin->Filter();
    TreeWin->Update();
}

void sFileWindow::LoadFiles()
{
    sArray<sDirEntry> dir;
    Files.DeleteAll();
    if(sLoadDir(dir,Path))
    {
        for(auto &de : dir)
        {
            Entry *e = new Entry(&de);
            Files.Add(e);
        }
    }
    FilesWin->ClearSelect();
    FilesWin->Filter();
    FilesWin->Update();
}

void sFileWindow::MakePath(int treeindex,const sStringDesc &desc)
{
    sString<sMaxPath> buffer;
    sString<sMaxPath> buffer2;

    int indent = 0;
    do
    {
        buffer2 = buffer;
        buffer = Tree[treeindex]->Name;
        if(buffer2[0])
        {
            buffer.Add("/");
            buffer.Add(buffer2);
        }
        int indent = Tree[treeindex]->ListInfo.Indent;
        if(indent==0)
            break;
        treeindex--;
        while(treeindex>0)
        {
            if(Tree[treeindex]->ListInfo.Indent<indent)
                break;
            treeindex--;
        }
    }
    while(indent>0 || treeindex>=0);

    sCopyString(desc,buffer);
}


sFileWindow::Entry2 *sFileWindow::FindDir(const char *dir)
{
    for(auto e : Tree)
        if(sCmpStringI(e->Path,dir)==0)
            return e;
    return 0;
}

/****************************************************************************/

void sFileWindow::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = 0;
}

void sFileWindow::OpenPath(const char *path_)
{
    const char *t0,*t1;
    Path = path_;
    sCleanPath(Path);

    t1 = Path;
    int indent = 0;
    while(*t1)
    {
        t0 = t1;
        while(*t1!=0 && *t1!='/')
            t1++;

        if(t0<t1 || t0==Path)
        {
            sString<sMaxPath> b;
            b.Init(Path,t1-Path);
            Entry2 *e = FindDir(b);
            if(e)
            {
                e->ListInfo.Opened = 1;
                LoadTree(e,b);      
            }
        }

        if(*t1=='/') 
            t1++;
        indent++;
    }
    Entry2 *e = FindDir(Path);
    if(e)
    {
        TreeWin->SetSelect(e);
        TreeWin->ScrollTo(e);
        LoadFiles();
    }
}

void sFileWindow::OpenPathAndFile(const char *path,const char *file)
{
    OpenPath(path);
    for(auto e : Files)
        if(sCmpStringI(e->Name,file)==0)
            FilesWin->SetSelect(e);
}

void sFileWindow::GetFile(const sStringDesc &desc)
{
    Entry *e = FilesWin->GetSelect();
    if(e)
        sCopyString(desc,e->Name);
}

void sFileWindow::GetPath(const sStringDesc &desc)
{
    sCopyString(desc,Path);
}

void sFileWindow::CmdSelectTree()
{
    Entry2 *e = TreeWin->GetSelect();
    if(e)
    {
        int n = Tree.FindEqualIndex(e);
        if(n>=0)
        {
            sString<sMaxPath> buffer;
            MakePath(n,buffer);
            if(sCmpStringP(buffer,Path)!=0)
            {
                Path = buffer;
                LoadFiles();
            }
        }
    }
    TreeMsg.Send();
}

void sFileWindow::CmdUpdateTree()
{
    TreeWin->Filter();
    for(auto e : Tree)
    {
        if(e->ListInfo.IsVisible && e->LoadMe)
        {
            int n = Tree.FindEqualIndex(e);
            sASSERT(n>0);
            sString<sMaxPath> buffer;
            Entry2 *parent = Tree[n-1];
            MakePath(n-1,buffer);
            LoadTree(parent,buffer);
            break;
        }
    }
    TreeMsg.Send();
}

void sFileWindow::CmdSelectFile()
{
    Entry *e = FilesWin->GetSelect();
    if(e)
    {
        if(e->Flags & sDEF_Dir)
        {
            sString<sMaxPath> file,path,buffer;
            GetFile(file);
            GetPath(path);
            buffer.PrintF("%s/%s",path,file);
            OpenPath(buffer);
        }
        else
        {
            FileMsg.Send();
        }
    }
}

void sFileWindow::CmdOk()
{
    OkMsg.Send();
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sFileDialogWindow : public sPassthroughFrame
{
    sFileWindow *FileWin;
    sFileDialogPara Para;

    sString<sMaxPath> Path;
    sString<sMaxPath> File;
    bool MakeRelative;
public:
    sFileDialogWindow(const sFileDialogPara &para);
    void SplitPath(const char *name);
    void CmdButton(int);
    void CmdUpdate();
};

/****************************************************************************/

sFileDialogWindow::sFileDialogWindow(const sFileDialogPara &para)
{
    Para = para;
    Flags = sWF_Top;

    SplitPath(para.Filename.Buffer);

    sHBoxFrame *h = new sHBoxFrame;
    sGridFrame *grid = new sGridFrame;
    FileWin = new sFileWindow;
    h->AddChild(FileWin);
    h->AddChild(grid);
    h->SetWeight(0,1);
    h->SetWeight(1,0);

    AddBorder(new sTitleBorder(Para.Headline ? Para.Headline : "File Dialog"));
    AddBorder(new sSizeBorder());    

    int w = 9;
    grid->GridX = w;
    grid->GridY = 3;
    grid->AddWindow(0 ,0,w,1,new sStringControl(Path));
    grid->AddWindow(0 ,1,w,1,new sStringControl(File));
    
    MakeRelative = (para.Flags&(sFDF_AllowRelative|sFDF_ForceRelative))!=0;

    if (para.Flags&sFDF_AllowRelative)
        grid->AddWindow(0 ,2,1,1,new sBoolToggle(MakeRelative,"Absolute Path","Relative Path"));
    
    grid->AddWindow(w-2,2,1,1,new sButtonControl(sGuiMsg(this,&sFileDialogWindow::CmdButton,0),"Cancel"));
    grid->AddWindow(w-1,2,1,1,new sButtonControl(sGuiMsg(this,&sFileDialogWindow::CmdButton,1),"Ok"));
    
    AddChild(h);

    FileWin->OpenPathAndFile(Path,File);
    FileWin->FileMsg = sGuiMsg(this,&sFileDialogWindow::CmdUpdate);
    FileWin->TreeMsg = sGuiMsg(this,&sFileDialogWindow::CmdUpdate);
    FileWin->OkMsg = sGuiMsg(this,&sFileDialogWindow::CmdButton,1);
}

void sFileDialogWindow::SplitPath(const char *name)
{
    File = name;
    sCleanPath(File);
    sMakeAbsolutePath(Path,File);
    File = sExtractName(Path);
    sRemoveName(Path);
}

void sFileDialogWindow::CmdButton(int n)
{
    if(n)
    {
        sString<sMaxPath> b;
        if (MakeRelative && sIsAbsolutePath(Path))
        {
            sMakeRelativePath(b, (const char *)Para.BaseDir.Buffer, Path);
            if (b.Count())
                b.Add("/");
        }
        else
        {
            b = Path;
        }        

        if (b.Count())
        {
            if (b.GetBuffer()[b.Count()-1]!='/')
                b.Add("/");
        }

        if(!(Para.Flags & sFDF_ModeDir))
            b.Add(File);
        sCopyString(Para.Filename,b);
        Para.OkMsg.Send();
    }
    else
    {
        Para.CancelMsg.Send();
    }
    CmdKill();
}

void sFileDialogWindow::CmdUpdate()
{
    FileWin->GetPath(Path);
    FileWin->GetFile(File);
}

/****************************************************************************/
/****************************************************************************/

sFileDialogPara::sFileDialogPara()
{
    Init();
}

void sFileDialogPara::Init()
{
    Filename.Buffer = 0;
    Filename.Size = 0;
    Flags = 0;
    Extensions = "";
    Headline = "";
    OkMsg = sGuiMsg();
    CancelMsg = sGuiMsg();
}

void sFileDialogPara::Start()
{
    Start(0);
}

void sFileDialogPara::Start(sWindow *master)
{
    sWindow *win = new sFileDialogWindow(*this);
    win->IndirectParent = master;
    Gui->AddDialog(win,sRect(10,10,810,610));
}

/****************************************************************************/
}
