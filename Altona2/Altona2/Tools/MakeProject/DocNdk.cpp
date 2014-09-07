/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/
#include "Main.hpp"
#include "Doc.hpp"

extern "C" sChar androidmanifest_txt[];

/****************************************************************************/
/***                                                                      ***/
/***   ndk                                                                ***/
/***                                                                      ***/
/****************************************************************************/

sBool Document::OutputNDK(mProject *pro)
{
    sBool ok = 1;
    sArray<mFile *> files;
    sArray<mFile *> depfiles;
    mFile *file;
    

    if(!pro->Library && pro->Configs.GetCount()==0) 
        return 1;


    sString<1024> trp;
    if (TargetRootPath[1]==':' && !CygwinPath )
    {
        sString<1024> t = (char *)TargetRootPath;
        t[1] = '/';
        sCleanPath(t);
        sSPrintF(trp,"/cygdrive/%s",(char *)t);            
    }
    else
    {
        trp = (char *)TargetRootPath;            
    }  

    // prepare
    Set("rootpath",(const sChar *)trp);
    Set("library",pro->Library ? "1" : "0");
    Set("compiler","ndk");
    Set("CONFIG","$(CONFIG)");
    Set("PROJECT",pro->Name);

    OutputFolder(pro->Root,files);

    if (!pro->Library)
    {
        OutputFolder(pro->Root,depfiles);

        for(auto dep : pro->Depends)
            OutputFolder(dep->Project->Root,depfiles);

        for(sInt i=0;i<depfiles.GetCount();)
        {
            file = depfiles[i];
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
                    depfiles[i] = depfiles[depfiles.GetCount()-1];
                    depfiles.RemTail();
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


    sln.Clear();  
    sln.PrintF("# makeproject (%d:%d) (%s) \n\n", VERSION, REVISION,__DATE__);

    if(pro->Library)
    {
        sln.PrintF("LOCAL_PATH := $(call my-dir)\n");
    }
    else
    {
        sln.PrintF("LOCAL_PATH := $(call my-dir)/..\n");
    }

    sln.PrintF("include $(CLEAR_VARS)\n");  
    sln.PrintF("LOCAL_MODULE := %s\n",pro->Name);  
    sln.PrintF("LOCAL_C_INCLUDES += %s\n",(char *)trp);

    for(auto file : files)
    {
        if(file->ToolId==mTI_cpp || file->ToolId==mTI_asc || file->ToolId==mTI_c || file->ToolId==mTI_incbin)
        {

            if (file->ToolId==mTI_asc)
                sln.PrintF("LOCAL_SRC_FILES += %s.cpp \n",file->NameWithoutExtension);
            else if (file->ToolId==mTI_incbin)
                sln.PrintF("LOCAL_SRC_FILES += %s.c \n",file->Name);
            else
                sln.PrintF("LOCAL_SRC_FILES += %s \n",file->Name);
        }        
    }

    for(auto dep : pro->Depends)
    {        
        sln.PrintF("LOCAL_STATIC_LIBRARIES += %s\n",dep->Project->Name);        
    }
    
    sln.Print("LOCAL_STATIC_LIBRARIES += android_native_app_glue\n");

    if(pro->Library)
    {
        sln.Print("include $(BUILD_STATIC_LIBRARY)\n");
    }
    else
    {        
        sln.Print("include $(BUILD_SHARED_LIBRARY)\n");
    }

    sInt rlen = TargetRootPath.Count();

    for(auto dep : pro->Depends)
    {    
        const sChar *mod = dep->Project->TargetSolutionPath.Get();
        sln.PrintF("$(call import-module,%s)\n",&mod[rlen]);
    }

    if(!pro->Library)
    {
        sln.Print("$(call import-module,android/native_app_glue)\n\n");

        sln.Print("prereqs:\n");

        for(auto file : depfiles)
        {
            sString<sMaxPath> path = file->FullPath;
            sRemoveName(path);            

            sString<sMaxPath> pak = file->Name.Get();
            sRemoveLastSuffix(pak);
            pak.Add(".pak");

            if(file->ToolId==mTI_asc)
            {
                sString<sMaxPath> cpp = file->FullPath;
                sRemoveLastSuffix(cpp);
                cpp.Add(".cpp");

                sString<sMaxPath> hpp = file->FullPath;
                sRemoveLastSuffix(hpp);
                hpp.Add(".hpp");

                sln.PrintF("\tasc2 -i=%s -cpp=%s -h=%s -p=gles2\n",file->FullPath,cpp.Get(),hpp.Get());
            }    
            else if(file->ToolId==mTI_ops)
            {
                sString<sMaxPath> cpp = file->FullPath;
                sRemoveLastSuffix(cpp);
                cpp.Add(".ops.cpp");

                sString<sMaxPath> hpp = file->FullPath;
                sRemoveLastSuffix(hpp);
                hpp.Add(".ops.hpp");

                sln.PrintF("\twz5ops -i=%s -c=%s -h=%s\n",file->FullPath,cpp.Get(),hpp.Get());
            }
            else if(file->ToolId==mTI_packfile)
            {
                sln.PrintF("\tpackfile -i=%s -o=%s/assets/%s \n",file->FullPath,path.Get(),pak.Get());
            }
            else if (file->ToolId==mTI_incbin)
            {
                sln.PrintF("\tincbin2asc -i=%s -o=%s \n",file->FullPath, file->FullPath);
            }
        }    
    }

    
    sTextLog app;
    sTextLog xml;

    if(!pro->Library)
    {                         
        app.Clear();
        app.PrintF("# makeproject (%d:%d) (%s) \n\n", VERSION, REVISION,__DATE__);
        app.PrintF("export NDK_MODULE_PATH=%s\n\n",(char *)trp);        
        app.PrintF("ifndef CONFIG\nCONFIG = %s\nendif\n\n",pro->Configs[0]->Name);  //First config entry is default

        for(auto con : pro->Configs)
        {        
            app.PrintF("ifeq ($(CONFIG),%s)\n",con->Name);
            for(auto gr : con->VSGroups)
            {
                if(gr->Name=="General" && gr->Compiler==OK_NDK)
                {                 
                    for(auto it : gr->VSItems)
                        app.PrintF("%s := %s\n",it->Key,it->Value);                        
                }
                else if(gr->Name=="" && gr->Compiler==OK_NDK)
                {                 
                    for(auto it : gr->VSItems)
                        app.PrintF("%s += %s\n",it->Key,it->Value);                        
                }
            }
            app.Print("endif\n\n");
        }
        LargeBuffer.Clear();
        Resolve(LargeBuffer,app.Get());
        app.Clear();
        app.Print(LargeBuffer.Get());
    
        LargeBuffer.Clear();
        Resolve(LargeBuffer,androidmanifest_txt);
        xml.Clear();
        xml.Print(LargeBuffer.Get());

        sDPrint(xml.Get());
    }
    
    if(!Pretend)
    {
        if(!pro->Library)
        {
            if (!sCheckDir("jni"))
                sMakeDir("jni");

            if (!sCheckDir("assets"))
                sMakeDir("assets");

            if (Manifest && !sCheckFile("AndroidManifest.xml"))
            {
                if(!sSaveTextAnsi("AndroidManifest.xml",xml.Get(),0)) ok = 0;
            }

            if(!sSaveTextAnsi("jni/Application.mk",app.Get(),0)) ok = 0;
            if(!sSaveTextAnsi("jni/Android.mk",sln.Get(),0)) ok = 0;
        }
        else
        {
            if(!sSaveTextAnsi("Android.mk",sln.Get(),0)) ok = 0;
        }
    }
    if(pro->Dump)
    {
        int i=0;
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint(app.Get());
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint(sln.Get());
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint("------------------------------------------------------------------------------\n");

    }

    return ok;
}

sBool Document::OutputNDK()
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
                if(!OutputNDK(pro))
                    ok = 0;
        }
    }
    return ok;
}

/****************************************************************************/

