/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Tools/MakeProject/Main.hpp"
#include "Altona2/Tools/MakeProject/Doc.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
    sPrintF("makeproject (altona2) v%d.%d\n",VERSION,REVISION);

    Document *Doc = new Document;

    // cmd line

    sString<64> buildname;
    sString<64> platformname;
    sInt platformset = 0;
    sInt buildset = 0;
    sInt pauseonerror = 0;
    int WriteMpx = 0;

    sCommandlineParser cmd;

    const sChar *platform = "win32";
    const sChar *build = "vs2012";
    if(sConfigPlatform == sConfigPlatformWin)
    {
        platform = "win32";
        build = "vs2012";
        Doc->RootPath = "c:/source/altona2";
    }
    if(sConfigPlatform == sConfigPlatformLinux)
    {
        platform = "linux";
        build = "make";
        Doc->RootPath = "/home/chaos/svn3";
    }
    if(sConfigPlatform == sConfigPlatformOSX)
    {
        platform = "osx";
        build = "xcode4";
        Doc->RootPath = "c:/svn3/altona2/";
    }
    if(sConfigPlatform == sConfigPlatformIOS)
    {
        platform = "ios";
        build = "xcode4";
        Doc->RootPath = "c:/svn3/altona2/";
    }
    if(sConfigPlatform == sConfigPlatformAndroid)
    {
        platform = "android";
        build = "android";
        Doc->RootPath = "c:/svn3/altona2/";
    }

    int X86 = 0;
    Doc->X64 = 1;
    Doc->Manifest = 0;
    Doc->TargetProject = "";

    cmd.AddHelp("?");
    cmd.AddFile("r",Doc->RootPath,0,"root path (c:/svn3)");
    cmd.AddFile("tr",Doc->TargetRootPath,0,"root path as seen from the target");
    cmd.AddSwitch("c",Doc->CreateNewFiles,"create new files");
    cmd.AddSwitch("cs",Doc->SvnNewFiles,"create new files and add to svn");
    cmd.AddSwitch("forceinfo",Doc->ForceInfo,"force writing of additional files like info.plist");
    cmd.AddSwitch("p",Doc->Pretend,"pretend");
    cmd.AddString("build",buildname,&buildset,"one of: vs2013 vs2012 vs2010 vs2008 make xcode4 ndk");
    cmd.AddString("platform",platformname,&platformset,"one of: win32 linux osx ios android");
    cmd.AddSwitch("x64",Doc->X64,"add x64 projects to vs2012/13 (default)");
    cmd.AddSwitch("x86",X86,"do not generate x64 projects for vs2012/13");
    cmd.AddSwitch("pause",pauseonerror,"pause on error");
    cmd.AddSwitch("mpx",WriteMpx,"write *.mpx.txt files");
    cmd.AddSwitch("manifest",Doc->Manifest,"creates an AndroidManifest.xml (if not already exists)");
    cmd.AddSwitch("nocygwinpath",Doc->CygwinPath,"Replace dos path c:\\ to /cygrdrive/. (default false) .only on Windows and build=ndk, platform=android.");
    
    cmd.AddFile("tp",Doc->TargetProject,0,"target project (only that project will be build/rebuild)");

    if(!cmd.Parse())    
    {
        sSetExitCode(1);
        return;
    }
    if(Doc->X64 && X86)
    {
        sPrint("you may not specify -x64 and -x86 at the same time.\n");
        sSetExitCode(1);
        return;
    }
    if(X86)
        Doc->X64 = 0;

    if(Doc->SvnNewFiles)
        Doc->CreateNewFiles = 1;
    if(Doc->TargetRootPath.IsEmpty())
        Doc->TargetRootPath = Doc->RootPath;

    // process configuration file

    sBool ok = Doc->ScanConfig();

    // figure out platform

    if(ok)
    {
        // figure out native platform

        if(platformset)
            platform = platformname;
        if(buildset)
            build = buildname;

        Doc->Platform = Doc->Platforms.Find([=](mPlatform *p){return p->Name==platform;});
        if(Doc->Platform==0)
        {
            sPrintF("platform %s not found\n",platform);
            sPrintF("known platforms are:\n");
            for(auto p : Doc->Platforms)
                sPrintF("  %s\n",p->Name);
            ok = 0;
        }

        Doc->OutputKind = OK_Error;
        if (sCmpStringI(build, "vs2013") == 0)
            Doc->OutputKind = OK_VS2013;
        else if (sCmpStringI(build, "vs2012") == 0)
            Doc->OutputKind = OK_VS2012;
        else if(sCmpStringI(build,"vs2010")==0)
            Doc->OutputKind = OK_VS2010;
        else if(sCmpStringI(build,"vs2008")==0)
            Doc->OutputKind = OK_VS2008;
        else if(sCmpStringI(build,"make")==0)
            Doc->OutputKind = OK_Make;
        else if(sCmpStringI(build,"xcode4")==0)
            Doc->OutputKind = OK_XCode4;
        else if(sCmpStringI(build,"ndk")==0)
            Doc->OutputKind = OK_NDK;

        if((Doc->OutputKind != OK_VS2012 && Doc->OutputKind != OK_VS2013) && Doc->X64)
        {
            Doc->X64 = 0;
        }

        if(Doc->OutputKind==OK_Error)
        {
            sPrintF("build system %s not found\n",build);
            sPrint("supported build systems:\n");
            sPrint("  vs2013\n");
            sPrint("  vs2012\n");
            sPrint("  vs2010\n");
            sPrint("  vs2008\n");
            sPrint("  make\n");
            sPrint("  xcode4\n");
            sPrint("  ndk\n");
            ok = 0;
        }
    }

    // scan mp files and make dependencies

    if(ok)
        ok = Doc->ScanMP();
    if(ok)
        ok = Doc->FindDepends();

    // output project files

    if(ok)
    {
        switch (Doc->OutputKind)
        {
        case OK_VS2013: case OK_VS2012:
            ok = Doc->OutputVS2012();
            break;
        case OK_VS2010:
            ok = Doc->OutputVS2010();
            break; 
        case OK_VS2008:
            ok = Doc->OutputVS2008();
            break;
        case OK_Make:
            ok = Doc->OutputMake();
            break;
        case OK_XCode4:
            ok = Doc->OutputXCode4();
            break;
        case OK_NDK:
            ok = Doc->OutputNDK();
            break;
        default:
            ok = 0;
            break;
        }
    }

    // create new files (if requested)

    if(ok && Doc->CreateNewFiles)
        Doc->Create();

    // some more outputs

    if(ok && WriteMpx)
        Doc->OutputMpx();

    // done

    if(!ok)
    {
        sSetExitCode(1);
        if(pauseonerror)
        {
            sPrint("fail. press any key to continue\n");
            sWaitForKey();
        }
        else
        {
            sPrint("fail\n");
        }
    }
    else
    {
        sPrint("success\n");
        sPrintF("projects found: %d\n",Doc->Solutions.GetCount());
        sPrintF("platforms found: %d\n",Doc->Platforms.GetCount());
        sInt n = 0;
        for(auto p : Doc->Platforms)
            n += p->Configs.GetCount();
        sPrintF("configurations found: %d (%d in %s)\n",n,Doc->Platform->Configs.GetCount(),Doc->Platform->Name);
    }

    delete Doc;
}


/****************************************************************************/
