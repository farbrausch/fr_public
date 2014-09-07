/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Main.hpp"
#include "Doc.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
    sCommandlineParser cmd;

    sPrintF("asc2 v%d.%d%s.%d\n",APP_VERSION,APP_REVISION,APP_SUFFIX,APP_BUILD);

    sString<sMaxPath> infile,objfile,cppfile,platformstring,dumpfile,headerfile;
    sInt dump=0;
    sInt isobjfile=0;
    sInt iscppfile=0;
    sInt platform=0;
    sInt x64bit=0;

    cmd.AddHelp("?");
    cmd.AddFile("!i",infile,0,"input file (xxx.asc)");
    cmd.AddFile("!h",headerfile,0,"output header file (xxx.hpp)");
    cmd.AddFile("o",objfile,&isobjfile,"output object file (final_null_shell_win32/xxx.o)");
    cmd.AddFile("cpp",cppfile,&iscppfile,"output cpp file (xxx.cpp)");
    cmd.AddString("!p",platformstring,0,"platform: dx9 dx11 blank gl2 gles2");
    cmd.AddFile("d",dumpfile,&dump,"dump debug info into file");
    cmd.AddSwitch("x64",x64bit,"compile for 64 bit");

  // if (!cmd.Parse("asc -i=/Users/bastianzuhlke/svn3/altona2/altona2/Examples/graphics/cube/shader.asc -h=/Users/bastianzuhlke/svn3/altona2/altona2/Examples/graphics/cube/shader.hpp -cpp=/Users/bastianzuhlke/svn3/altona2/altona2/Examples/graphics/cube/shader.cpp -d=/Users/bastianzuhlke/svn3/altona2/altona2/Examples/graphics/cube/shader.log -p=gles2"))
    if(!cmd.Parse())
    {
        sSetExitCode(1);
        return;
    }
    if(isobjfile+iscppfile!=1)
    {
        sPrint("you must specify either the -cpp option or the -o options,\nnot both or none\n");
        sSetExitCode(1);
        return;
    }

    sBool ok = 1;

    platform = 0;
    if(platformstring=="dx9")
        platform = sConfigRenderDX9;
    if(platformstring=="dx11")
        platform = sConfigRenderDX11;
    if(platformstring=="gl2")
        platform = sConfigRenderGL2;
    if(platformstring=="gles2")
        platform = sConfigRenderGLES2;
    if(platformstring=="null")
        platform = sConfigRenderNull;
    if(platform==0)
    {
        sPrintF("unknown platform %s\n",platformstring);
        sSetExitCode(1);
        return;
    }

    wDocument *doc = new wDocument;
    doc->Bits64 = x64bit?1:0;

    sInt plat = sConfigPlatform;
    if(sConfigRender==sConfigRenderGLES2)
        plat = sConfigPlatformIOS;

    if(!doc->Parse(infile,platform,dump))
        ok = 0;

    if(ok)
    {
        sSaveTextAnsi(headerfile,doc->GetHpp(infile),1);

        if(isobjfile)
        {
            // save output

            sString<sMaxPath> asmfile;
            asmfile.PrintF("%s.asm",objfile);
            sSaveTextAnsi(asmfile,doc->GetAsm(),0);

            if(!sAssemble(asmfile,objfile,plat,doc->Bits64))
            {
                sPrint("yasm failed\n");
                ok = 0;
            }
        }

        if(iscppfile)
        {
            sSaveTextAnsi(cppfile,doc->GetCpp(),0);
        }
    }

    if(dump)
        sSaveTextAnsi(dumpfile,doc->GetDump(),1);

    if(ok && !Doc->ErrorFlag)
    {
        sPrint("ok\n");
    }
    else
    {
        sPrint("failed\n");
        sSetExitCode(1);
    }
    delete doc;
}


/****************************************************************************/

