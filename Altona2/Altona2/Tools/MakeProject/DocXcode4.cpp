/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Doc.hpp"

extern "C" sChar plist_txt[];
extern "C" sChar osxplist_txt[];

/****************************************************************************/
/***                                                                      ***/
/***   makefile                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sBool Document::OutputXCode4(mProject *pro)
{
    sBool ok = 1;
    mFile *file;
    cVSGroup *gr;
    sArray<sPoolString> Frameworks;

    // prepare

    const sBool COOLLIBS = 0;
    
    bool nosetup = false;
    bool shell = false;
    for(auto con : pro->Configs)
    {
        nosetup = nosetup || (sFindFirstString(con->Name,"nosetup")!=-1);
        shell = shell || (sFindFirstString(con->Name,"shell")!=-1);
    }
        
    if(pro->Library && !nosetup) return 1;
    if(pro->Configs.GetCount()==0) return 1;

    Set("rootpath",(const sChar *)TargetRootPath);
    Set("library",pro->Library ? "1" : "0");
    Set("compiler","make");
    Set("solutiondir",pro->Name);

    sArray<mFile *> files;
    OutputFolder(pro->Root,files);
    if(!COOLLIBS)
    {
        for(auto dep : pro->Depends)
            OutputFolder(dep->Project->Root,files);
    }
    sString<sMaxPath> basepath = (const sChar *) pro->SolutionPath;
    sRemoveName(basepath);


    // gather frameworks

    for(auto con : pro->Configs)
    {
        for(auto gr : con->VSGroups)
        {
            if(gr->Compiler==OK_XCode4 && gr->Name.IsEmpty())
            {
                for(auto it : gr->VSItems)
                {
                    if(it->Key=="Frameworks")
                    {
                        const sChar *s = it->Value;
                        while(*s==' ') s++;
                        while(*s)
                        {
                            const sChar *ss = s;
                            while(*s!=' ' && *s!=0) s++;
                            sPoolString name(ss,s-ss);
                            if(!Frameworks.FindEqual(name))
                                Frameworks.Add(name);
                            while(*s==' ') s++;
                        }
                    }
                }
            }
        }
    }
    if(0)
    {
        for(auto pp : Frameworks)
            sDPrintF("(%s) ",pp);
        sDPrintF("\n");
    }

    // gather files
    
    sArray<mFile *> AdditionalFiles;
    for(auto file : files)
    {
        if(file->ToolId == mTI_packfile)
        {
            sString<sMaxPath> buffer;
            buffer.PrintF("%s.pak",file->NameWithoutExtension);
            sASSERT(sCmpString(file->Name,file->OriginalName)==0);

            mFile *af = new mFile;
            af->Name = buffer;
            af->OriginalName = buffer;
            af->NameWithoutExtension = file->NameWithoutExtension;
            af->NoNew = 1;
            af->ToolId = mTI_pak;
            af->Project = file->Project;
            af->FullPath.PrintF("%s/%s",af->Project->SolutionPath,af->Name);

            AdditionalFiles.Add(af);
        }
    }
    for(auto file : AdditionalFiles)
        files.Add(file);

    sInt i = 1;
    for(auto file : files)
    {
        file->XCodeRefGuid .PrintF("%08X000000A0%08X",pro->GuidHash,i);
        file->XCodeFileGuid.PrintF("%08X000000A1%08X",pro->GuidHash,i);

        i++;
    }

    for(sInt i=0;i<files.GetCount();)
    {
        file = files[i];
        if(file->Options.GetCount())
        {
            sInt cfound = 0;
            sInt cexcld = 0;
            for(auto con : pro->Configs)
            {
                for(auto opt : file->Options)
                {
                    sBool ok = 0;
                    if(opt->Name[0]=='!')
                        ok = !sMatchWildcard(opt->Name+1,con->Name,0,1);
                    else
                        ok = sMatchWildcard(opt->Name,con->Name,0,1);
                    if(ok)
                    {
                        cfound++;
                        if(opt->Excluded)
                            cexcld++;
                    }
                }
            }
            if(cfound>0 && cfound==cexcld)
            {
                files[i] = files[files.GetCount()-1];
                files.RemTail();
            }
            else
            {
                i++;
            }
        }
        else
        {
            i++;
        }
    }

    // prepare guids

    sString<32> guidApp            ; guidApp            .PrintF("%08X000000E000000001",pro->GuidHash);
    sString<32> guidPhaseFrameworks; guidPhaseFrameworks.PrintF("%08X000000E000000002",pro->GuidHash);
    sString<32> guidPhaseSources   ; guidPhaseSources   .PrintF("%08X000000E000000003",pro->GuidHash);
    sString<32> guidCopyFiles      ; guidCopyFiles      .PrintF("%08X000000E000000004",pro->GuidHash);
    sString<32> guidProducts       ; guidProducts       .PrintF("%08X000000E000000005",pro->GuidHash);
    sString<32> guidMyProject      ; guidMyProject      .PrintF("%08X000000E000000006",pro->GuidHash);
    sString<32> guidmaingroup      ; guidmaingroup      .PrintF("%08X000000E000000007",pro->GuidHash);
    sString<32> guidTarget         ; guidTarget         .PrintF("%08X000000E000000008",pro->GuidHash);
    sString<32> guidProject        ; guidProject        .PrintF("%08X000000E000000009",pro->GuidHash);
    sString<32> guidBuildTarget    ; guidBuildTarget    .PrintF("%08X000000E00000000A",pro->GuidHash);
    sString<32> guidBuildProject   ; guidBuildProject   .PrintF("%08X000000E00000000B",pro->GuidHash);
    sString<32> guidProductRef     ; guidProductRef     .PrintF("%08X000000E00000000C",pro->GuidHash);
    sString<32> guidGroupFrameworks; guidGroupFrameworks.PrintF("%08X000000E00000000D",pro->GuidHash);
    sString<32> guidPhaseResources ; guidPhaseResources .PrintF("%08X000000E00000000E",pro->GuidHash);

    sString<32> guidRuleIncbin     ; guidRuleIncbin     .PrintF("%08X000000E000000201",pro->GuidHash);
    sString<32> guidRuleAsc        ; guidRuleAsc        .PrintF("%08X000000E000000202",pro->GuidHash);
    sString<32> guidRulePackfile   ; guidRulePackfile   .PrintF("%08X000000E000000203",pro->GuidHash);
    sString<32> guidRuleOps        ; guidRuleOps        .PrintF("%08X000000E000000204",pro->GuidHash);

    // output project file (this is loooooong)

    sln.Clear();
    sln.Print("// !$*UTF8*$!\n");
    sln.Print("{\n");
    sln.Print("  archiveVersion = 1;\n");
    sln.Print("  classes = {\n");
    sln.Print("  };\n");
    sln.Print("  objectVersion = 46;\n");
    sln.Print("  objects = {\n");
    sln.Print("\n");
    sln.Print("/* Begin PBXBuildFile section */\n");
    
    if (!pro->Library && !shell)
    {
	    if(Platform->Name=="ios")
    	 	sln.Print("		566655C616F0BE660016BA8A /* ios.info.plist in Resources */ = {isa = PBXBuildFile; fileRef = 566655C516F0BE660016BA8A /* ios.info.plist */; };\n");
	    else
    	 	sln.Print("		566655C616F0BE660016BA8A /* osx.info.plist in Resources */ = {isa = PBXBuildFile; fileRef = 566655C516F0BE660016BA8A /* osx.info.plist */; };\n");
    }
    for(auto file : files)
    {
        switch(file->ToolId)
        {
        case mTI_cpp:
        case mTI_c:
        case mTI_m:
        case mTI_mm:
        case mTI_incbin:
        case mTI_packfile:
        case mTI_xib:
        case mTI_pak:
        case mTI_asc:
        case mTI_ops:
        case mTI_dat:
        case mTI_xml:
            sln.PrintF("    %s = {isa = PBXBuildFile; fileRef = %s /* %s */; }; \n",file->XCodeRefGuid,file->XCodeFileGuid,file->FullPath);
            break;
        default:
            break;
        }
    }
    i = 1;
    for(auto pp : Frameworks)
    {
        sln.PrintF("    %08X000000A2%08X = {isa = PBXBuildFile; fileRef = %08X000000A3%08X /* %s.framework */; }; \n",pro->GuidHash,i,pro->GuidHash,i,pp);
        i++;
    }
    if(COOLLIBS)
    {
        for(auto dep : pro->Depends)
            sln.PrintF("    %08X000000B8%08X = {isa = PBXBuildFile; fileRef = %08X000000B9%08X /* lib%s.a */; };\n",pro->GuidHash,dep->Project->GuidHash,pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
    }
    sln.Print ("/* End PBXBuildFile section */\n");

    sln.Print ("\n");

    sln.Print ("/* Begin PBXBuildRule section */\n");

    sln.PrintF("    %s /* PBXBuildRule */ = {\n",guidRuleIncbin);
    sln.Print ("      isa = PBXBuildRule;\n");
    sln.Print ("      compilerSpec = com.apple.compilers.proxy.script;\n");
    sln.Print ("      filePatterns = \"*.incbin\";\n");
    sln.Print ("      fileType = pattern.proxy;\n");
    sln.Print ("      isEditable = 1;\n");
    sln.Print ("      outputFiles = (\n");
    sln.Print ("        \"${DERIVED_FILE_DIR}/${INPUT_FILE_BASE}.c\",\n");
    sln.Print ("      );\n");
    sln.PrintF ("      script = \""
        "cd ${INPUT_FILE_DIR}\\n"
        "echo %saltona2/Bin/osx/incbinasc -i=${INPUT_FILE_PATH} -o=${DERIVED_FILE_DIR}/${INPUT_FILE_BASE}\\n"
        "%saltona2/Bin/osx/incbinasc -i=${INPUT_FILE_PATH} -o=${DERIVED_FILE_DIR}/${INPUT_FILE_BASE}"
        "\";\n", (const char*)TargetRootPath,(const char*)TargetRootPath);
    sln.Print ("    };\n");

    sln.PrintF("    %s /* PBXBuildRule */ = {\n",guidRulePackfile);
    sln.Print ("      isa = PBXBuildRule;\n");
    sln.Print ("      compilerSpec = com.apple.compilers.proxy.script;\n");
    sln.Print ("      filePatterns = \"*.packfile\";\n");
    sln.Print ("      fileType = pattern.proxy;\n");
    sln.Print ("      isEditable = 1;\n");
    sln.Print ("      outputFiles = (\n");
    sln.Print ("        \"${INPUT_FILE_DIR}/${INPUT_FILE_BASE}.pak\",\n");
    sln.Print ("      );\n");
    sln.PrintF ("      script = \""
        "cd ${INPUT_FILE_DIR}\\n"
        "echo %saltona2/Bin/osx/packfile -i=${INPUT_FILE_PATH} ${INPUT_FILE_DIR}/${INPUT_FILE_BASE}.pak\\n"
        "%saltona2/Bin/osx/packfile -i=${INPUT_FILE_PATH} -o=${INPUT_FILE_DIR}/${INPUT_FILE_BASE}.pak"
        "\";\n", (const char*)TargetRootPath,(const char*)TargetRootPath);
    sln.Print ("    };\n");

    sln.PrintF("    %s /* PBXBuildRule */ = {\n",guidRuleAsc);
    sln.Print ("      isa = PBXBuildRule;\n");
    sln.Print ("      compilerSpec = com.apple.compilers.proxy.script;\n");
    sln.Print ("      filePatterns = \"*.asc\";\n");
    sln.Print ("      fileType = pattern.proxy;\n");
    sln.Print ("      isEditable = 1;\n");
    sln.Print ("      outputFiles = (\n");
    sln.Print ("        \"${DERIVED_FILE_DIR}/${INPUT_FILE_BASE}.cpp\",\n");
    sln.Print ("        \"${INPUT_FILE_DIR}/${INPUT_FILE_BASE}.hpp\",\n");
    sln.Print ("      );\n");
    if(Platform->Name=="ios")
    {
        sln.PrintF ("      script = \""
            "cd ${INPUT_FILE_DIR}\\n"
            "echo %saltona2/Bin/osx/asc -i=${INPUT_FILE_PATH} -cpp=${DERIVED_FILE_DIR}/${INPUT_FILE_BASE}.cpp -h=${INPUT_FILE_BASE}.hpp -p=gles2\\n"
            "%saltona2/Bin/osx/asc -i=${INPUT_FILE_PATH} -cpp=${DERIVED_FILE_DIR}/${INPUT_FILE_BASE}.cpp -h=${INPUT_FILE_BASE}.hpp -p=gles2"
            "\";\n", (const char*)TargetRootPath,(const char*)TargetRootPath);
    }
    else
    {
        sln.PrintF ("      script = \""
            "cd \\${INPUT_FILE_DIR}\\n"
            "echo %saltona2/Bin/osx/asc -i=${INPUT_FILE_PATH} -cpp=${DERIVED_FILE_DIR}/${INPUT_FILE_BASE}.cpp -h=${INPUT_FILE_BASE}.hpp -p=gl2\\n"
            "%saltona2/Bin/osx/asc -i=${INPUT_FILE_PATH} -cpp=${DERIVED_FILE_DIR}/${INPUT_FILE_BASE}.cpp -h=${INPUT_FILE_BASE}.hpp -p=gl2"
            "\";\n", (const char*)TargetRootPath,(const char*)TargetRootPath);
    }
    sln.Print ("    };\n");
    sln.Print ("/* End PBXBuildRule section */\n");


    sln.PrintF("    %s /* PBXBuildRule */ = {\n",guidRuleOps);
    sln.Print ("      isa = PBXBuildRule;\n");
    sln.Print ("      compilerSpec = com.apple.compilers.proxy.script;\n");
    sln.Print ("      filePatterns = \"*.ops\";\n");
    sln.Print ("      fileType = pattern.proxy;\n");
    sln.Print ("      isEditable = 1;\n");
    sln.Print ("      outputFiles = (\n");
    sln.Print ("        \"${INPUT_FILE_DIR}/${INPUT_FILE_BASE}.ops.hpp\",\n");
    sln.Print ("      );\n");
    sln.PrintF ("      script = \""
        "cd ${INPUT_FILE_DIR}\\n"
        "echo %saltona2/Bin/osx/wz5ops -i=${INPUT_FILE_PATH} -c=${INPUT_FILE_BASE}.ops.cpp -h=${INPUT_FILE_BASE}.ops.hpp \\n"
        "%saltona2/Bin/osx/wz5ops -i=${INPUT_FILE_PATH} -c=${INPUT_FILE_BASE}.ops.cpp -h=${INPUT_FILE_BASE}.ops.hpp"
        "\";\n", (const char*)TargetRootPath,(const char*)TargetRootPath);
    sln.Print ("    };\n");
    sln.Print ("/* End PBXBuildRule section */\n");

    sln.Print ("\n");


    if(COOLLIBS)
    {
        sln.Print ("/* Begin PBXContainerItemProxy section */\n");
        for(auto dep : pro->Depends)
        {
            sln.PrintF("    %08X000000B1%08X /* PBXContainerItemProxy */ = {\n",pro->GuidHash,dep->Project->GuidHash);
            sln.Print ("      isa = PBXContainerItemProxy;\n");
            sln.PrintF("      containerPortal = %08X000000BA%08X /* xcodeproj */;\n",pro->GuidHash,dep->Project->GuidHash);
            sln.Print ("      proxyType = 2;\n");
            sln.PrintF("      remoteGlobalIDString = %08X000000E000000001;\n",dep->Project->GuidHash);
            sln.PrintF("      remoteInfo = %s;\n",dep->Project->Name);
            sln.Print ("    };\n");
            sln.PrintF("    %08X000000B3%08X /* PBXContainerItemProxy */ = {\n",pro->GuidHash,dep->Project->GuidHash);
            sln.Print ("      isa = PBXContainerItemProxy;\n");
            sln.PrintF("      containerPortal = %08X000000BA%08X /* xcodeproj */;\n",pro->GuidHash,dep->Project->GuidHash);
            sln.Print ("      proxyType = 1;\n");
            sln.PrintF("      remoteGlobalIDString = %08X000000E000000008;\n",dep->Project->GuidHash);
            sln.PrintF("      remoteInfo = %s;\n",dep->Project->Name);
            sln.Print ("    };\n");
        }
        sln.Print ("/* End PBXContainerItemProxy section */\n");

        sln.Print ("\n");
    }

    sln.Print ("/* Begin PBXCopyFilesBuildPhase section */\n");
    /*
    sln.PrintF("    %s = {\n",guidCopyFiles);
    sln.Print ("      isa = PBXCopyFilesBuildPhase;\n");
    sln.Print ("      buildActionMask = 2147483647;\n");
    sln.Print ("      dstPath = /usr/share/man/man1/;\n");
    sln.Print ("      dstSubfolderSpec = 0;\n");
    sln.Print ("      files = (\n");
    sln.Print ("      );\n");
    sln.Print ("      runOnlyForDeploymentPostprocessing = 1;\n");
    sln.Print ("    };\n");
    */
    sln.Print ("/* End PBXCopyFilesBuildPhase section */\n");

    sln.Print ("\n");

    sln.Print ("/* Begin PBXFileReference section */\n");
    
    if (!pro->Library && !shell)
    {
	    if(Platform->Name=="ios")
			sln.Print ("		566655C516F0BE660016BA8A /* ios.info.plist */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = text.plist.xml; path = ios.info.plist; sourceTree = \"<group>\"; };\n");
	    else
			sln.Print ("		566655C516F0BE660016BA8A /* osx.info.plist */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = text.plist.xml; path = osx.info.plist; sourceTree = \"<group>\"; };\n");
    }
    i = 1;
    for(auto pp : Frameworks)
    {
        sln.PrintF("    %08X000000A3%08X = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = %s.framework; path = System/Library/Frameworks/%s.framework; sourceTree = SDKROOT; };\n",pro->GuidHash,i,pp,pp);
        i++;
    }
    if(pro->Library)
        sln.PrintF("    %s = {isa = PBXFileReference; explicitFileType = archive.ar; includeInIndex = 0; path = lib%s.a; sourceTree = BUILT_PRODUCTS_DIR; };\n",guidApp,sExtractName(pro->SolutionPath));
    else
        sln.PrintF("    %s = {isa = PBXFileReference; explicitFileType = \"compiled.mach-o.executable\"; includeInIndex = 0; path = %s; sourceTree = BUILT_PRODUCTS_DIR; };\n",guidApp,sExtractName(pro->SolutionPath));
    for(auto file : files)
    {
        sString<sMaxPath> buffer,from;
        from = pro->SolutionPath;
        //    sRemoveName(from);

        sMakeRelativePath(buffer,from,file->FullPath);
        switch(file->ToolId)
        {
        case mTI_cpp:
            sln.PrintF("    %s /* %s */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.cpp.cpp; path = \"%s\"; sourceTree = \"<group>\"; };\n",file->XCodeFileGuid,file->Name,file->OriginalName);
            break;
        case mTI_c:
            sln.PrintF("    %s /* %s */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.c; path = \"%s\"; sourceTree = \"<group>\"; };\n",file->XCodeFileGuid,file->Name,file->OriginalName);
            break;
        case mTI_m:
            sln.PrintF("    %s /* %s */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.objc; path = \"%s\"; sourceTree = \"<group>\"; };\n",file->XCodeFileGuid,file->Name,file->OriginalName);
            break;
        case mTI_mm:
            sln.PrintF("    %s /* %s */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.cpp.objc; path = \"%s\"; sourceTree = \"<group>\"; };\n",file->XCodeFileGuid,file->Name,file->OriginalName);
            break;
        case mTI_xib:
            sln.PrintF("    %s /* %s */ = {isa = PBXFileReference; lastKnownFileType = file.xib; path = \"%s\"; sourceTree = \"<group>\"; };\n",file->XCodeFileGuid,file->Name,file->OriginalName);
            break;
        case mTI_hpp:
            sln.PrintF("    %s /* %s */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.h; path = \"%s\"; sourceTree = \"<group>\"; };\n",file->XCodeFileGuid,file->Name,file->OriginalName);
            break;
        case mTI_dat:
            sln.PrintF("    %s /* %s */ = {isa = PBXFileReference; lastKnownFileType = file; path = \"%s\"; sourceTree = \"<group>\"; };\n",file->XCodeFileGuid,file->Name,file->OriginalName);
            break;
        case mTI_xml:
            sln.PrintF("    %s /* %s */ = {isa = PBXFileReference; lastKnownFileType = text.xml; path = \"%s\"; sourceTree = \"<group>\"; };\n",file->XCodeFileGuid,file->Name,file->OriginalName);
            break;
        default:
            sln.PrintF("    %s /* %s */ = {isa = PBXFileReference; lastKnownFileType = text; path = \"%s\"; sourceTree = \"<group>\"; };\n",file->XCodeFileGuid,file->Name,file->OriginalName);
            break;
        }
    }
    if(COOLLIBS)
    {
        for(auto dep : pro->Depends)
        {
            sString<sMaxPath> buffer,from,to;

            buffer = dep->Project->SolutionPath;
            sRemoveName(buffer);
            to.PrintF("%s/%s.xcodeproj",buffer,dep->Project->Name);
            from = pro->SolutionPath;
            sRemoveName(from);
            sMakeRelativePath(buffer,from,to);
            sln.PrintF("    %08X000000BA%08X = {isa = PBXFileReference; lastKnownFileType = \"wrapper.pb-project\"; name = %s.xcodeproj; path = %s; sourceTree = \"<group>\"; };\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name,buffer);
            sln.PrintF("    %08X000000B9%08X = {isa = PBXFileReference; explicitFileType = archive.ar; path = lib%s.a; sourceTree = SOURCE_ROOT; };\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
        }
    }
    sln.Print ("/* End PBXFileReference section */\n");


    sln.Print ("\n");
    sln.Print ("/* Begin PBXFrameworksBuildPhase section */\n");
    sln.PrintF("    %s /* Frameworks */ = {\n",guidPhaseFrameworks);
    sln.Print ("      isa = PBXFrameworksBuildPhase;\n");
    sln.Print ("      buildActionMask = 2147483647;\n");
    sln.Print ("      files = (\n");
    i = 1;
    for(auto pp : Frameworks)
    {
        sln.PrintF("        %08X000000A2%08X /* %s.framework */,\n",pro->GuidHash,i,pp);
        i++;
    }
    if(COOLLIBS)
    {
        for(auto dep : pro->Depends)
            sln.PrintF("        %08X000000B8%08X /* lib%s.a */,\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
    }
    sln.Print ("      );\n");
    sln.Print ("      runOnlyForDeploymentPostprocessing = 0;\n");
    sln.Print ("    };\n");
    sln.Print ("/* End PBXFrameworksBuildPhase section */\n");

    sln.Print ("\n");

    sln.Print ("/* Begin PBXGroup section */\n");
    sln.PrintF("    %s = {\n",guidmaingroup);
    sln.Print ("      isa = PBXGroup;\n");
    sln.Print ("      children = (\n");
    if(COOLLIBS)
    {
        for(auto dep : pro->Depends)
        {
            sln.PrintF("        %08X000000B9%08X /* library %s */,\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
            sln.PrintF("        %08X000000BA%08X /* library %s */,\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
        }
    }
    else
    {
        for(auto dep : pro->Depends)
            sln.PrintF("        %08X000000BF%08X /* library %s */,\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
    }
    sln.PrintF("        %s /* MyProject */,\n",guidMyProject);
    sln.PrintF("        %s /* Products */,\n",guidProducts);
    sln.PrintF("        %s /* Framworks */,\n",guidGroupFrameworks);
    sln.Print ("      );\n");
    sln.Print ("      sourceTree = \"<group>\";\n");
    sln.Print ("    };\n");

    sln.PrintF("    %s /* Products */ = {\n",guidProducts);
    sln.Print ("      isa = PBXGroup;\n");
    sln.Print ("      children = (\n");
    sln.PrintF("        %s /* app */,\n",guidApp);
    sln.Print ("      );\n");
    sln.Print ("      name = Products;\n");
    sln.Print ("      sourceTree = \"<group>\";\n");
    sln.Print ("    };\n");

    sln.PrintF("    %s /* MyProject */ = {\n",guidMyProject);
    sln.Print ("      isa = PBXGroup;\n");
    sln.Print ("      children = (\n");
    
    if (!pro->Library && !shell)
    {
    	if(Platform->Name=="ios")
        	sln.Print ("				566655C516F0BE660016BA8A /* ios.info.plist */,\n");
    	else
        	sln.Print ("				566655C516F0BE660016BA8A /* osx.info.plist */,\n");
    }
    
    for(auto file : files)
    {
        if(file->Project==pro)
        {
            sln.PrintF("        %s /* %s */,\n",file->XCodeFileGuid,file->Name);
        }
    }
    sln.Print ("      );\n");
    sln.PrintF("      path = %s;\n",sExtractName(pro->SolutionPath));
    sln.Print ("      sourceTree = \"<group>\";\n");
    sln.Print ("    };\n");

    if(!COOLLIBS)
    {
        for(auto dep : pro->Depends)
        {
            sln.PrintF("    %08X000000BF%08X /* library %s */ = {\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
            sln.Print ("      isa = PBXGroup;\n");
            sln.Print ("      children = (\n");
                        
            for(auto file : files)
            {
                if(file->Project==dep->Project)
                {
                    sln.PrintF("        %s /* %s */,\n",file->XCodeFileGuid,file->Name);
                }
            }
            sln.Print ("      );\n");
            sln.PrintF("      name = %s;\n",dep->Project->Name);
            sString<sMaxPath> buffer;
            sMakeRelativePath(buffer,basepath,dep->Project->PathName);
            sRemoveName(buffer);
            sln.PrintF("      path = %s;\n",buffer);
            sln.Print ("      sourceTree = \"<group>\";\n");
            sln.Print ("    };\n");
        }
    }
    if(COOLLIBS && !pro->Depends.IsEmpty())
    {
        sln.PrintF("    %s /* Products */ = {\n",guidProductRef);
        sln.Print ("      isa = PBXGroup;\n");
        sln.Print ("      children = (\n");
        for(auto dep : pro->Depends)
        {
            sln.PrintF("        %08X000000BC%08X /* lib%s.a */,\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
        }
        sln.Print ("      );\n");
        sln.Print ("      name = Products;\n");
        sln.Print ("      sourceTree = \"<group>\";\n");
        sln.Print ("    };  \n");
    }
    sln.PrintF("    %s /* Frameworks */ = {\n",guidGroupFrameworks);
    sln.Print ("      isa = PBXGroup;\n");
    sln.Print ("      children = (\n");
    i = 1;
    for(auto pp : Frameworks)
    {
        sln.PrintF("        %08X000000A3%08X /* %s.framework */,\n",pro->GuidHash,i,pp);
        i++;
    }
    sln.Print ("      );\n");
    sln.Print ("      name = Frameworks;\n");
    sln.Print ("      sourceTree = \"<group>\";\n");
    sln.Print ("    };  \n");

    sln.Print ("/* End PBXGroup section */\n");

    sln.Print ("\n");

    sln.Print ("/* Begin PBXNativeTarget section */\n");
    sln.PrintF("    %s /* target */ = {\n",guidTarget);
    sln.Print ("      isa = PBXNativeTarget;\n");
    sln.PrintF("      buildConfigurationList = %s /* Build configuration list for PBXNativeTarget \"%s\" */;\n",guidBuildTarget,pro->Name);
    sln.Print ("      buildPhases = (\n");
    sln.PrintF("        %s /* Sources */,\n",guidPhaseSources);
    sln.PrintF("        %s /* Frameworks */,\n",guidPhaseFrameworks);
    sln.PrintF("        %s /* Resources */,\n",guidPhaseResources);
    //  sln.PrintF("        %s /* CopyFiles */,\n",guidCopyFiles);
    sln.Print ("      );\n");
    sln.Print ("      buildRules = (\n");
    sln.PrintF("        %s /* incbin */,\n",guidRuleIncbin);
    sln.PrintF("        %s /* packfile */,\n",guidRulePackfile);
    sln.PrintF("        %s /* asc */,\n",guidRuleAsc);
    sln.PrintF("        %s /* ops */,\n",guidRuleOps);
    sln.Print ("      );\n");
    sln.Print ("      dependencies = (\n");
    if(COOLLIBS)
    {
        for(auto dep : pro->Depends)
            sln.PrintF("				%08X000000BD%08X /* lib%s.a */,\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
    }
    sln.Print ("      );\n");
    sln.PrintF("      name = %s;\n",pro->Name);
    sln.PrintF("      productName = %s;\n",pro->Name);
    sln.PrintF("      productReference = %s /* app */;\n",guidApp);
    
    if(pro->Library)
        sln.Print ("      productType = \"com.apple.product-type.library.static\";\n");  // OR APPLICATION
    else
    {
    	if (shell)
	        sln.Print ("      productType = \"com.apple.product-type.tool\";\n");  // OR shell application
        else
        	sln.Print ("      productType = \"com.apple.product-type.application\";\n");  // OR APPLICATION
              }
    sln.Print ("    };\n");
    sln.Print ("/* End PBXNativeTarget section */\n");

    sln.Print ("\n");

    sln.Print ("/* Begin PBXProject section */\n");
    sln.PrintF("    %s /* Project object */ = {\n",guidProject);
    sln.Print ("      isa = PBXProject;\n");
    sln.Print ("      attributes = {\n");
    sln.Print ("        ORGANIZATIONNAME = Farbrausch;\n");
    sln.Print ("      };\n");
    sln.PrintF("      buildConfigurationList = %s /* Build configuration list for PBXProject */;\n",guidBuildProject);
    sln.Print ("      compatibilityVersion = \"Xcode 3.2\";\n");
    sln.Print ("      developmentRegion = English;\n");
    sln.Print ("      hasScannedForEncodings = 0;\n");
    sln.Print ("      knownRegions = (\n");
    sln.Print ("        en,\n");
    sln.Print ("      );\n");
    sln.PrintF("      mainGroup = %s;\n",guidmaingroup);
    sln.PrintF("      productRefGroup = %s /* Products */;\n",guidProducts);
    sln.Print ("      projectDirPath = \"\";\n");
    if(COOLLIBS)
    {
        sln.Print ("      projectReferences = (\n");
        for(auto dep : pro->Depends)
        {
            sln.Print ("        {\n");
            sln.PrintF("          ProductGroup = %s /* Productsref */;\n",guidProductRef);
            sln.PrintF("          ProjectRef = %08X000000BA%08X /* %s.xcodeproj */;\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
            sln.Print ("        },\n");
        }
        sln.Print ("      );\n");
    }
    sln.Print ("      projectRoot = \"\";\n");

    sln.Print ("      targets = (\n");
    sln.PrintF("        %s /* target */,\n",guidTarget);
    sln.Print ("      );\n");
    sln.Print ("    };\n");
    sln.Print ("/* End PBXProject section */\n");
    sln.Print ("\n");

    sln.Print ("\n");

    if(COOLLIBS)
    {
        sln.Print ("/* Begin PBXReferenceProxy section */\n");
        for(auto dep : pro->Depends)
        {
            sln.PrintF("    %08X000000BC%08X /* lib%s.a */  = {\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
            sln.Print ("      isa = PBXReferenceProxy;\n");
            sln.Print ("      fileType = archive.ar;\n");
            sln.PrintF("      path = lib%s.a;\n",dep->Project->Name);
            sln.PrintF("      remoteRef = %08X000000B1%08X /* PBXContainerItemProxy */;\n",pro->GuidHash,dep->Project->GuidHash);
            sln.Print ("      sourceTree = BUILT_PRODUCTS_DIR;\n");
            sln.Print ("    };\n");
        }
        sln.Print ("/* End PBXReferenceProxy section */\n");

        sln.Print ("\n");
    }

    sln.Print ("/* Begin PBXResourcesBuildPhase section */\n");
    sln.PrintF("    %s /* Resources */ = {\n",guidPhaseResources);
    sln.Print ("      isa = PBXResourcesBuildPhase;\n");
    sln.Print ("      buildActionMask = 2147483647;\n");
    sln.Print ("      files = (\n");
    for(auto file : files)
    {
        if(file->ToolId==mTI_xib || file->ToolId==mTI_pak || file->ToolId==mTI_dat || file->ToolId==mTI_xml)
            sln.PrintF("        %s /* %s */,\n",file->XCodeRefGuid,file->Name);
    }
    sln.Print ("      );\n");
    sln.Print ("      runOnlyForDeploymentPostprocessing = 0;\n");
    sln.Print ("    };\n");
    sln.Print ("/* End PBXResourcesBuildPhase section */\n");
    sln.Print ("\n");

    sln.Print ("/* Begin PBXSourcesBuildPhase section */\n");
    sln.PrintF("    %s /* Sources */ = {\n",guidPhaseSources);
    sln.Print ("      isa = PBXSourcesBuildPhase;\n");
    sln.Print ("      buildActionMask = 2147483647;\n");
    sln.Print ("      files = (\n");
    for(auto file : files)
    {
        switch(file->ToolId)
        {
        case mTI_cpp:
        case mTI_c:
        case mTI_m:
        case mTI_mm:
        case mTI_incbin:
        case mTI_packfile:
        case mTI_asc:
        case mTI_ops:
            //    case mTI_xib:
            sln.PrintF("        %s /* %s */,\n",file->XCodeRefGuid,file->Name);
            break;
        default:
            break;
        }
    }
    sln.Print ("      );\n");
    sln.Print ("      runOnlyForDeploymentPostprocessing = 0;\n");
    sln.Print ("    };\n");
    sln.Print ("/* End PBXSourcesBuildPhase section */\n");

    sln.Print ("\n");

    if(COOLLIBS)
    {
        sln.Print ("/* Begin PBXTargetDependency section */\n");
        for(auto dep : pro->Depends)
        {
            sln.PrintF("    %08X000000BD%08X /* lib%s.a */  = {\n",pro->GuidHash,dep->Project->GuidHash,dep->Project->Name);
            sln.Print ("      isa = PBXTargetDependency;\n");
            sln.PrintF("      name = %s;\n",dep->Project->Name);
            sln.PrintF("      targetProxy = %08X000000B3%08X /* PBXContainerItemProxy */;\n",pro->GuidHash,dep->Project->GuidHash);
            sln.Print ("    };\n");
        }
        sln.Print ("/* End PBXTargetDependency section */\n");

        sln.Print ("\n");
    }

    sln.Print ("/* Begin XCBuildConfiguration section */\n");
    sInt n;
    n = 1;
    for(auto con : pro->Configs)
    {
        sPoolString PbxPro("PBXProject");
        gr = con->VSGroups.Find([=](cVSGroup *g){return g->Name==PbxPro;});
        if(gr)
        {
            sln.PrintF("    %08X000000D1%08X /* %s */ = {\n",pro->GuidHash,n,con->Name);
            sln.Print ("      isa = XCBuildConfiguration;\n");
            sln.Print ("      buildSettings = {\n");
            for(auto it : gr->VSItems)
            {
                Resolve(LargeBuffer,it->Value);
                sln.PrintF("        %Q = %Q;\n",it->Key,LargeBuffer);
            }
            sln.Print ("      };\n");
            sln.PrintF("      name = %s;\n",con->Name);
            sln.Print ("    };\n");
        }
        sPoolString PbxNav("PBXNativeTarget");
        gr = con->VSGroups.Find([=](cVSGroup *g){return g->Name==PbxNav;});
        if(gr)
        {
            sln.PrintF("    %08X000000D2%08X /* %s */ = {\n",pro->GuidHash,n,con->Name);
            sln.Print ("      isa = XCBuildConfiguration;\n");
            sln.Print ("      buildSettings = {\n");
            for(auto it : gr->VSItems)
            {
                Resolve(LargeBuffer,it->Value);
                sln.PrintF("        %Q = %Q;\n",it->Key,LargeBuffer);
            }
            sln.Print ("      };\n");
            sln.PrintF("      name = %s;\n",con->Name);
            sln.Print ("    };\n");
        }
        n++;
    }
    sln.Print ("/* End XCBuildConfiguration section */\n");

    sln.Print ("\n");

    sln.Print ("/* Begin XCConfigurationList section */\n");
    sln.PrintF("    %s /* Build configuration list for PBXProject  */ = {\n",guidBuildProject);
    sln.Print ("      isa = XCConfigurationList;\n");
    sln.Print ("      buildConfigurations = (\n");
    n = 1;
    for(auto con : pro->Configs)
        sln.PrintF("        %08X000000D1%08X /* %s */,\n",pro->GuidHash,n++,con->Name);
    sln.Print ("      );\n");
    sln.Print ("      defaultConfigurationIsVisible = 0;\n");
    sln.Print ("    };\n");
    sln.PrintF("    %s /* Build configuration list for PBXNativeTarget  */ = {\n",guidBuildTarget);
    sln.Print ("      isa = XCConfigurationList;\n");
    sln.Print ("      buildConfigurations = (\n");
    n = 1;
    for(auto con : pro->Configs)
        sln.PrintF("        %08X000000D2%08X /* %s */,\n",pro->GuidHash,n++,con->Name);
    sln.Print ("      );\n");
    sln.Print ("      defaultConfigurationIsVisible = 0;\n");
    sln.Print ("    };\n");
    sln.Print ("/* End XCConfigurationList section */\n");

    sln.Print ("\n");

    sln.Print ("  };\n");
    sln.PrintF("  rootObject = %s /* Project object */;\n",guidProject);
    sln.Print ("}\n");

    // done

    if(!Pretend)
    {
        sString<sMaxPath> buffer;
        buffer.PrintF("%s/%s_%s.xcodeproj",basepath,pro->Name,Platform->Name);
        sMakeDir(buffer);
        buffer.PrintF("%s/%s_%s.xcodeproj/project.pbxproj",basepath,pro->Name,Platform->Name);
        if(!sSaveTextAnsi(buffer,sln.Get(),0)) ok = 0;
      
        if (!pro->Library && !shell)
        {
        	if(Platform->Name=="ios")
        	{
          		buffer.PrintF("%s/%s/ios.info.plist",basepath,pro->Name);
          		if(!sCheckFile(buffer) || ForceInfo)
            		sSaveTextUTF8(buffer,plist_txt);
        	}
            else
        	{
          		buffer.PrintF("%s/%s/osx.info.plist",basepath,pro->Name);
          		if(!sCheckFile(buffer) || ForceInfo)
            		sSaveTextUTF8(buffer,osxplist_txt);
            }
        }
    }
    if(pro->Dump)
    {
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint(sln.Get());
        sDPrint("------------------------------------------------------------------------------\n");
    }


    AdditionalFiles.DeleteAll();

    return ok;
}

sBool Document::OutputXCode4()
{
    sBool ok = 1;
    for(auto sol : Solutions)
    {
        sChangeDir(sol->Path);
        for(auto pro : sol->Projects)
        {
        	for(auto dep : pro->Depends)
            {
              	for (auto opt : dep->Project->Root->Options)
                {
                    for(auto conf : pro->Configs)
                    {
                        if (sFindFirstString(conf->Name,opt->Name)!=-1)
          				{
           					AddOptions(conf, opt);
           				}
                    }
                }
            }
            
    		for (auto opt : pro->Root->Options)
            {
                for(auto conf : pro->Configs)
                {
                    if (sFindFirstString(conf->Name,opt->Name)!=-1)
          			{
                    	AddOptions(conf, opt);
                    }
                }
            }
            
            if (TargetProject.IsEmpty() || sCmpString(TargetProject.Get(),pro->Name.Get())==0)
                if(!OutputXCode4(pro))
                    ok = 0;
        }
        
    }
    return ok;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/



/****************************************************************************/

