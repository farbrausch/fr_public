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
/***   makefile                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sBool Document::OutputMake(mProject *pro)
{
    sBool ok = 1;

    // prepare

    Set("rootpath",(const sChar *)TargetRootPath);
    Set("library",pro->Library ? "1" : "0");
    Set("compiler","make");

    sArray<mFile *> files;
    OutputFolder(pro->Root,files);

    // output header

    sln.Clear();
    sln.PrintF("### %s\n",pro->Name);
    sln.Print("\n");
    sln.Print("### header\n");
    sln.Print("\n");
    sln.Print("ifndef CONFIG\n");
    sln.PrintF("CONFIG = %s\n",pro->Configs[0]->Name);
    sln.Print("endif\n");
    sln.Print("\n");
    sln.Print("objdir = $(CONFIG)\n");
    sln.PrintF("root = %s\n",TargetRootPath);

    // global options

    sln.Print("\n");
    sln.Print("### options\n");
    sln.Print("\n");

    for(auto con : pro->Configs)
    {
        sln.PrintF("ifeq ($(CONFIG),%s)\n",con->Name);
        for(auto gr : con->VSGroups)
        {
            if(gr->Name=="" && gr->Compiler==OK_Make)
            {
                for(auto it : gr->VSItems)
                {
                    sln.PrintF("%s = %s\n",it->Key,it->Value);
                }
            }
        }
        sln.Print("endif\n");
    }

    // output libs / links

    sln.Print("\n");
    sln.Print("### files\n");
    sln.Print("\n");

    sln.Print("depends =\n");
    for(auto dep : pro->Depends)
        sln.PrintF("depends += %s/$(CONFIG)/%s.a\n",dep->Project->TargetSolutionPath,dep->Project->Name);
    sln.Print("\n");

    // output objects

    sln.Print("objects =\n");
    for(auto file : files)
    {
        if(file->ToolId==mTI_cpp || file->ToolId==mTI_c || file->ToolId==mTI_incbin || file->ToolId==mTI_asc || file->ToolId==mTI_ops)
        {
            if(file->Options.GetCount()==0)
            {
                sln.PrintF("objects += $(objdir)/%s.o\n",file->NameWithoutExtension);
            }
            else
            {
                for(auto con : pro->Configs)
                {
                    sBool exclude = 0;
                    for(auto opt : file->Options) 
                    {
                        sBool ok = 0;
                        if(opt->Name[0]=='!')
                            ok = !sMatchWildcard(opt->Name+1,con->Name,0,1);
                        else
                            ok = sMatchWildcard(opt->Name,con->Name,0,1);
                        if(ok)
                            if(opt->Excluded)
                                exclude = 1;
                    }
                    if(!exclude)
                    {
                        sln.PrintF("ifeq ($(CONFIG),%s)\n",con->Name);
                        sln.PrintF("objects += $(objdir)/%s.o\n",file->NameWithoutExtension);
                        sln.Print("endif\n");
                    }
                }
            }
        }
    }

    if(0)
    {
        sln.Print("\n");
        sln.Print("headers =\n");
        for(auto file : files)
        {
            if(file->ToolId==mTI_hpp)
            {
                sln.PrintF("headers += %s\n",file->Name);
            }
        }
    }

    sln.Print("\n");
    sln.Print("### project rules\n");
    sln.Print("\n");

    if(pro->Library)
    {
        sln.PrintF("all : prereqs $(objdir)/%s.a\n",pro->Name);
        sln.PrintF("prereqs :\n");
        //    sln.PrintF("\techo $(CONFIG) $(cflags)\n");
        sln.PrintF("\tmkdir -p $(objdir)\n");
        sln.PrintF("$(objdir)/%s.a : $(objects)\n",pro->Name);
        sln.PrintF("\tar -rcs $(CONFIG)/%s.a $(objects)\n",pro->Name);
    }
    else
    {
        sln.PrintF("all : prereqs makelibs $(objdir)/%s.out\n",pro->Name);
        sln.PrintF("prereqs :\n");
        //    sln.PrintF("\techo $(CONFIG) $(cflags)\n");
        sln.PrintF("\tmkdir -p $(objdir)\n");
        sln.Print("makelibs :\n");
        for(auto dep : pro->Depends)
            sln.PrintF("\t(cd %s; $(MAKE))\n",dep->Project->TargetSolutionPath);
        sln.PrintF("$(objdir)/%s.out : $(objects) $(depends)\n",pro->Name);
        sln.PrintF("\tg++ -o$(objdir)/%s.out $(cflags) $(objects) $(depends) $(libs)\n",pro->Name);
        sln.Print("clean : makelibs\n");
        sln.PrintF("\trm $(objdir)/%s.out\n",pro->Name);
        sln.Print("\trm $(objdir)/*.o\n");
        for(auto dep : pro->Depends)
        {
            sln.PrintF("\trm %s/$(objdir)/%s.a\n",dep->Project->TargetSolutionPath,dep->Project->Name);
            sln.PrintF("\trm %s/$(objdir)/*.o\n",dep->Project->TargetSolutionPath);
        }
    }

    sln.Print("\n");
    sln.Print("### dependencies\n");
    sln.Print("\n");

    for(auto file : files)
    {
        if(file->ToolId == mTI_cpp || file->ToolId==mTI_c)
        {
            sln.PrintF("$(objdir)/%s.o :",file->NameWithoutExtension);

            sString<sMaxPath> buffer;
            buffer.PrintF("%s/%s",pro->SolutionPath,file->Name);
            PrintIncludes(sPoolString(buffer),sln);
            sln.PrintF("\n");
        }
    }


    sln.Print("\n");
    sln.Print("### rules\n");
    sln.Print("\n");

    sln.Print("$(objdir)/%.o : %.cpp\n");
    sln.Print("\tg++ $(cflags) -c $< -o $@\n");
    sln.Print("\n");
    sln.Print("$(objdir)/%.o : %.c\n");
    sln.Print("\tg++ $(cflags) -c $< -o $@\n");
    sln.Print("\n");
    sln.Print("$(objdir)/%.o : %.incbin\n");
    sln.Print("\tincbin -i=$< -o=$@ -x64\n");
    sln.Print("\n");

    sln.Print("$(objdir)/%.o %.hpp : %.asc\n");
    //  sln.Print("\tasc -i=$< -h=$@ -o=$* -p=$(renderer)\n");
    sln.Print("\tasc -i=$< -o=$(<D)/$(objdir)/$(basename $(<F)).o -h=$*.hpp -p=$(renderer) -x64\n");
    sln.Print("\n");

    sln.Print("\n");
    sln.Print("### end\n");
    sln.Print("\n");

    // done

    if(!Pretend)
    {
        if(!sSaveTextAnsi("Makefile",sln.Get(),0)) ok = 0;
    }
    if(pro->Dump)
    {
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint(sln.Get());
        sDPrint("------------------------------------------------------------------------------\n");
    }

    return ok;
}

sBool Document::OutputMake()
{
    sBool ok = 1;
    for(auto sol : Solutions)
    {
        sChangeDir(sol->Path);
        for(auto pro : sol->Projects)
            if(!OutputMake(pro))
                ok = 0;
    }
    return ok;
}

/****************************************************************************/

