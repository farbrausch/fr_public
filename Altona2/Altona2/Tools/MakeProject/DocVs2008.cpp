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
/***   output vs2008                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Document::OutputVS2008Folder(mFolder *fol,mProject *pro)
{
    for(auto folder : fol->Folders)
    {
        vcx.PrintF("\t\t<Filter Name=\"%s\" Filter=\"\">\r\n",folder->Name);
        OutputVS2008Folder(folder,pro);
        vcx.PrintF("\t\t</Filter>\r\n");
    }
    for(auto file : fol->Files)
    {
        vcx.PrintF("\t\t<File RelativePath=\".\\%P\">\r\n",file->Name);

        if(file->Options.GetCount()>0 || file->ToolId==mTI_incbin || file->ToolId==mTI_asc || file->ToolId==mTI_packfile || file->ToolId==mTI_ops)
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


                if(exclude || file->ToolId==mTI_incbin || file->ToolId==mTI_asc || file->ToolId==mTI_packfile || file->ToolId==mTI_ops)
                {
                    vcx.PrintF("\t\t\t<FileConfiguration Name=\"%s|Win32\"\r\n",con->Name);
                    if(exclude)
                    {
                        vcx.PrintF("\t\t\t\tExcludedFromBuild=\"true\"\r\n");
                    }
                    vcx.PrintF("\t\t\t>\r\n");

                    sPoolString toolname;
                    if(file->ToolId==mTI_incbin) toolname = "incbin";
                    if(file->ToolId==mTI_asc) toolname = "asc";
                    if(file->ToolId==mTI_ops) toolname = "ops";
                    if(file->ToolId==mTI_para) toolname = "para";
                    if(file->ToolId==mTI_packfile) toolname = "packfile";
                    if(!toolname.IsEmpty())
                    {
                        vcx.Print("\t\t\t<Tool\r\n");
                        vcx.Print("\t\t\t\tName=\"VCCustomBuildTool\"\r\n");

                        for(auto gr : con->VSGroups)
                        {
                            if(gr->Name==toolname && gr->Compiler==OK_VS2008)
                            {
                                for(auto it : gr->VSItems)
                                {
                                    Resolve(LargeBuffer,it->Value);
                                    vcx.PrintF("\t\t\t\t%s=\"%s\"\r\n",it->Key,LargeBuffer);
                                }
                            }
                        }
                        if(!file->AdditionalDependencies.IsEmpty())
                            vcx.PrintF("\t\t\t\tAdditionalDependencies=\"%s\"\r\n",file->AdditionalDependencies);
                        vcx.Print("\t\t\t/>\r\n");
                    }
                    vcx.Print("\t\t\t</FileConfiguration>\r\n");
                }
                /*
                sFORALL(pro->Configs,con)
                {
                sBool ok = 0;
                if(opt->Name[0]=='!')
                ok = !sMatchWildcard(opt->Name+1,con->Name,0,1);
                else
                ok = sMatchWildcard(opt->Name,con->Name,0,1);
                if(ok)
                {


                vcx.PrintF("\t\t\t<FileConfiguration Name=\"%s|Win32\"\r\n",con->Name);

                sFORALL(con->VSGroups,gr)
                {
                if(gr->Name=="" && gr->Compiler==OK_VS2008)
                {
                sFORALL(gr->VSItems,it)
                {
                Resolve(LargeBuffer,it->Value);
                vcx.PrintF("\t\t\t\t%s=\"%s\"\r\n",it->Key,LargeBuffer);
                }
                }
                }

                vcx.PrintF("\t\t\t>\n");
                sFORALL(con->VSGroups,gr)
                {
                if(gr->Name!="" && gr->Compiler==OK_VS2008)
                {
                vcx.PrintF("\t\t\t\t<Tool Name=\"%s\"\r\n",gr->Name);
                sFORALL(gr->VSItems,it)
                {
                Resolve(LargeBuffer,it->Value);
                vcx.PrintF("\t\t\t\t\t%s=\"%s\"\r\n",it->Key,LargeBuffer);
                }
                vcx.Print("\t\t\t\t/>\r\n");
                }
                }
                vcx.Print("\t\t\t</FileConfiguration>\r\n");
                }
                }
                */
            }
        }
        vcx.Print("\t\t</File>\r\n");
    }
}

sBool Document::OutputVS2008(mProject *pro)
{
    sln.Clear();
    vcx.Clear();
    sBool ok = 1;

    // set environment

    Environment.Clear();
    Set("rootpath",(const sChar *)TargetRootPath);
    Set("library",pro->Library ? "1" : "0");
    Set("compiler","vs2008");

    // solution

    sln.Print("\r\n");
    sln.Print("Microsoft Visual Studio Solution File, Format Version 10.00\r\n");
    sln.Print("# Visual C++ Express 2008\r\n");

    OutputVSSln(pro,OK_VS2008);

    // vcproj
    vcx.Print("<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n");
    vcx.Print("<VisualStudioProject\r\n");
    vcx.Print("\tProjectType=\"Visual C++\"\r\n");
    vcx.Print("\tVersion=\"9,00\"\r\n");
    vcx.PrintF("\tName=\"%s\"\r\n",pro->Name);
    vcx.PrintF("\tProjectGUID=\"%s\"\r\n",pro->Guid);
    vcx.PrintF("\tRootNamespace=\"%s\"\r\n",pro->Name);
    vcx.Print("\tKeyword=\"Win32Proj\"\r\n");
    vcx.Print("\t>\r\n");
    vcx.Print("\t<Platforms>\r\n");
    vcx.Print("\t\t<Platform Name=\"Win32\"/>\r\n");
    //  vcx.Print("\t\t<Platform Name=\"x64\"/>\r\n");
    vcx.Print("\t</Platforms>\r\n");

    vcx.Print("\r\n");
    vcx.Print("\t<ToolFiles>\r\n");
    vcx.Print("\t</ToolFiles>\r\n");
    vcx.Print("\r\n");

    vcx.Print("\t<Configurations>\r\n");

    for(auto con : pro->Configs)
    {
        vcx.Print("\t\t<Configuration\r\n");
        vcx.PrintF("\t\t\tName=\"%s|Win32\"\r\n",con->Name);
        /*
        vcx.Print("\t\t\tOutputDirectory=\"$(SolutionDir)$(ConfigurationName)_Win32\"\r\n");
        vcx.Print("\t\t\tIntermediateDirectory=\"$(ConfigurationName)_Win32\"\r\n");
        vcx.Print("\t\t\tConfigurationType=\"1\"\r\n");
        vcx.Print("\t\t\tCharacterSet=\"1\"\r\n");
        */
        for(auto gr : con->VSGroups)
        {
            if(gr->Name=="" && gr->Compiler==OK_VS2008)
            {
                for(auto it : gr->VSItems)
                {
                    Resolve(LargeBuffer,it->Value);
                    vcx.PrintF("\t\t\t\t%s=\"%s\"\r\n",it->Key,LargeBuffer);
                }
            }
        }
        vcx.Print("\t\t\t>\r\n");
        for(auto gr : con->VSGroups)
        {
            if(gr->Name!="" && gr->Compiler==OK_VS2008)
            {
                vcx.PrintF("\t\t\t<Tool Name=\"%s\"\r\n",gr->Name);
                for(auto it : gr->VSItems)
                {
                    Resolve(LargeBuffer,it->Value);
                    vcx.PrintF("\t\t\t\t%s=\"%s\"\r\n",it->Key,LargeBuffer);
                }
                vcx.Print("\t\t\t/>\r\n");
            }
        }
        vcx.Print("\t\t</Configuration>\r\n");
    }
    vcx.Print("\t</Configurations>\r\n");

    vcx.Print("\r\n");
    vcx.Print("\t<References>\r\n");
    vcx.Print("\t</References>\r\n");
    vcx.Print("\r\n");

    vcx.Print("\r\n");
    vcx.Print("\t<Files>\r\n");
    OutputVS2008Folder(pro->Root,pro);
    vcx.Print("\t</Files>\r\n");
    vcx.Print("\r\n");

    vcx.Print("\r\n");
    vcx.Print("\t<Globals>\r\n");
    vcx.Print("\t</Globals>\r\n");
    vcx.Print("\r\n");

    vcx.Print("</VisualStudioProject>\r\n");


    // write out

    if(!Pretend)
    {
        sString<sMaxPath> name;
        name.PrintF("%s.sln",pro->Name);
        if(!sSaveTextUTF8(name,sln.Get())) ok = 0;

        name.PrintF("%s.vcproj",pro->Name);
        if(!sSaveTextUTF8(name,vcx.Get())) ok = 0;
    }
    if(pro->Dump)
    {
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint(sln.Get());
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint(vcx.Get());
        sDPrint("------------------------------------------------------------------------------\n");
    }

    return ok;
}

sBool Document::OutputVS2008()
{
    sBool ok = 1;
    for(auto sol : Solutions)
    {
        sChangeDir(sol->Path);
        for(auto pro : sol->Projects)
            if(!OutputVS2008(pro))
                ok = 0;
    }
    return ok;
}


/****************************************************************************/

