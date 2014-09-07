/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Doc.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sBool wFile::GetSize()
{
    sBool result = 1;
    sFile *file = sOpenFile(SourceName,sFA_Read);
    if(file)
        Size = file->GetSize();
    else
        result = 0;
    delete file;
    return result;
}

wDocument::wDocument()
{

}

wDocument::~wDocument()
{
    Files.DeleteAll();
}

/****************************************************************************/

sBool wDocument::Parse(const sChar *filename)
{
    Scan.Init(sSF_CppComment|sSF_NumberComment);
    Scan.AddDefaultTokens();
    Scan.StartFile(filename);

    sPoolString cd;
    sString<sMaxPath> buffer;

    while(!Scan.Errors && Scan.Token!=sTOK_End)
    {
        if(Scan.IfName("file"))
        {
            wFile *f = new wFile;
            Scan.ScanString(f->SourceName);
            if(Scan.IfName("as"))
                Scan.ScanString(f->PackName);
            else
                f->PackName = f->SourceName;
            Scan.Match(';');

            if(!cd.IsEmpty())
            {
                buffer.PrintF("%s/%s",cd,f->SourceName);
                f->SourceName = buffer;
            }

            if(!f->GetSize())
                Scan.Error("file not found: <%s>\n",f->SourceName);

            Files.Add(f);
        }
        else if(Scan.IfName("match"))
        {
            sPoolString path,wild;
            Scan.ScanString(wild);
            Scan.MatchName("in");
            Scan.ScanString(path);
            Scan.Match(';');
            if(!Scan.Errors)
            {
                buffer = path;
                if(!cd.IsEmpty())
                    buffer.PrintF("%s/%s",cd,path);
                sArray<sDirEntry> dir;
                if(sLoadDir(dir,buffer))
                {
                    for(auto &de : dir)
                    {
                        if(!(de.Flags & sDEF_Dir))
                        {
                            if(sMatchWildcard(wild,de.Name,0,0))
                            {
                                wFile *f = new wFile;              
                                buffer.PrintF("%s/%s",path,de.Name);
                                f->PackName = buffer;
                                if(!cd.IsEmpty())
                                    buffer.PrintF("%s/%s/%s",cd,path,de.Name);
                                f->SourceName = buffer;
                                if(!f->GetSize())
                                    Scan.Error("file not found: <%s>\n",f->SourceName);
                                Files.Add(f);
                            }
                        }
                    }
                }
                else
                {
                    Scan.Error("could not load dir %q",buffer);
                }
            }
        }
        else if(Scan.IfName("cd"))
        {
            sBool ok = 1;
            if(Scan.Token==sTOK_Name)
            {
                sPoolString platform = Scan.ScanName();
                sInt code = 0;
                if(platform=="osx") code = sConfigPlatformOSX;
                else if(platform=="win") code = sConfigPlatformWin;
                else if(platform=="linux") code = sConfigPlatformLinux;
                else Scan.Error("unknown platform code, try win, linux or osx");

                ok = sConfigPlatform==code;
            }
            if(ok)
                Scan.ScanString(cd);
            else
                Scan.ScanString();
            Scan.Match(';');
        }
        else
        {
            Scan.Error("unknown command");
        }
    }

    return Scan.Errors==0;
}

sBool wDocument::Output(const sChar *filename)
{
    sBool ok = 0;

    // count strings

    sPtr stringsize = 0;
    for(auto f : Files)
    {
        stringsize += sGetStringLen(f->PackName)+1;
    }
    stringsize = sAlign(stringsize,sPackfileAlign);

    // generate header

    sPackfileHeader ph;
    ph.Magic = sPackfileMagic;
    ph.Version = sPackfileVersion;
    ph.DirCount = Files.GetCount();
    ph.StringBytes = sU32(stringsize);
    ph.DataOffset = sU32(sizeof(sPackfileHeader) + sizeof(sPackfileEntry)*Files.GetCount() + stringsize);
    ph.pad[0] = 0;
    ph.pad[1] = 0;
    ph.pad[2] = 0;

    // generate dir

    sPtr stringused = 0;
    sChar *strings = new sChar[stringsize];
    sSetMem(strings,0,stringsize);
    sU64 offset = 0;
    sPackfileEntry *pde = new sPackfileEntry[Files.GetCount()];
    sInt n=0;
    for(auto f : Files)
    {
        pde[n].Size = f->Size;
        pde[n].PackedSize = 0;
        pde[n].Offset = offset;
        offset += sAlign(pde[n].Size,sPackfileAlign);
        pde[n].Name = sU32(stringused);
        sPtr len = sGetStringLen(f->PackName);
        sCopyMem(strings+stringused,f->PackName,len);
        stringused += len+1;
        pde[n].Flags = 0;
        n++;
    }

    // write out

    sFile *file = sOpenFile(filename,sFA_Write);
    if(file)
    {
        ok = 1;
        file->Write(&ph,sizeof(sPackfileHeader));
        file->Write(pde,sizeof(sPackfileEntry)*Files.GetCount());
        file->Write(strings,stringsize);

        sU8 *a0 = new sU8[sPackfileAlign];
        sSetMem(a0,0xaa,sPackfileAlign);

        for(auto f : Files)
        {
            sPtr readsize = 0;
            sU8 *buffer = sLoadFile(f->SourceName,readsize);
            if(buffer && sS64(readsize)==f->Size)
            {
                if(!file->Write(buffer,sPtr(f->Size)))
                    ok = 0;
                sInt padding = sPackfileAlign-(f->Size&(sPackfileAlign-1));
                if(padding!=sPackfileAlign)
                {
                    if(!file->Write(a0,padding))
                        ok = 0;
                }
            }
            else
            {
                sPrintF("problems loading <%s>",f->SourceName);
                ok = 0;
            }
            delete[] buffer;

        }
        delete[] a0;
    }
    delete[] strings;
    delete[] pde;
    if(!file->Close())
        ok = 0;
    delete file;

    return ok;
}

/****************************************************************************/

