/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Scanner.hpp"

using namespace Altona2;

#define VERSION 1
#define REVISION 3



void Output(sTextLog &out,const char *prefix,const char *Label,const char *text,uptr size)
{
    out.PrintF("        global  %s%s\n",prefix,Label);
    out.PrintF("%s%s:",prefix,Label);
    for(uint i=0;i<size;i++)
    {
        if(i%16==0)
            out.Print("\n        db      ");
        else
            out.Print(",");
        out.PrintF("%3d",uint(text[i])&255);
    }
    out.Print("\n        db      0\n");
}

void Altona2::Main()
{
    sPrintF("incbin v%d.%d\n",VERSION,REVISION);

    // commandline

    sString<sMaxPath> inputfile;
    sString<sMaxPath> outputfile;
    int hasoutputfile = 0;
    int hasinputfile = 0;
    int x64 = 0;
    sCommandlineParser cmd;
    cmd.AddHelp("?");
    cmd.AddFile("!i",inputfile,&hasinputfile,"source *.incbin.txt file");
    cmd.AddFile("!o",outputfile,&hasoutputfile,"output *.obj file");
    cmd.AddSwitch("x64",x64,"64 bit");
    if(!cmd.Parse() || !hasoutputfile || !hasinputfile)
    {
        sPrint("usage:\n");
        sPrint("incbin -i=inputfile.txt -o=outputfile.obj\n");
        sSetExitCode(1);
        return;
    }

    // plaform?

    const sChar *prefix = "";
    if((sConfigPlatform==sConfigPlatformWin || sConfigPlatform==sConfigPlatformOSX) && !x64)
        prefix = "_";

    // create asm file

    sTextLog out;
    out.Print("        section .text\n");
    out.PrintF("        bits    %d\n",x64?64:32);
    out.Print("\n");

    sScanner Scan;
    Scan.Init(sSF_CppComment);
    Scan.AddDefaultTokens();
    Scan.AddTokens("@");
    Scan.StartFile(inputfile);


    while(!Scan.Errors && Scan.Token!=sTOK_End)
    {
        sString<sMaxPath> File;
        sString<sFormatBuffer> Label;
        if(Scan.IfName("binary"))
        {
            Scan.ScanName(Label);
            Scan.ScanString(File);
            Scan.Match(';');
            if(!Scan.Errors)
            {
                out.PrintF("        global  %s%s\n",prefix,Label);
                out.PrintF("%s%s:\n",prefix,Label);
                out.PrintF("        incbin  \"%s\"\n",File);
            }
        }
        else if(Scan.IfName("text"))
        {
            Scan.ScanName(Label);
            Scan.ScanString(File);
            Scan.Match(';');
            if(!Scan.Errors)
            {
                out.PrintF("        global  %s%s\n",prefix,Label);
                out.PrintF("%s%s:\n",prefix,Label);
                out.PrintF("        incbin  \"%s\"\n",File);
                out.PrintF("        db      0\n");
            }
        }
        else if(Scan.IfName("utf8"))
        {
            Scan.ScanName(Label);
            Scan.ScanString(File);
            Scan.Match(';');
            if(!Scan.Errors)
            {
                const sChar *text = sLoadText(File);
                if(text==0)
                {
                    Scan.Error("file not found: %q",File);
                }
                else
                {
                    Output(out,prefix,Label,text,sGetStringLen(text));
                    delete[] text;
                }
            }
        }
        else if(Scan.IfName("inline"))
        {
            Scan.ScanName(Label);
            sTextLog tb;
            Scan.ScanRaw(tb,'@','@');
            Scan.Match(';');
            Output(out,prefix,Label,tb.Get(),tb.GetCount());
        }
        else
        {
            Scan.Error("unknown command");
        }
    }
    if(Scan.Errors)
    {
        sSetExitCode(1);
        return;
    }

    // save asm file

    sString<sMaxPath> asmfile;

    asmfile = inputfile;
    asmfile.Add(".asm");
    if(!sSaveFile(asmfile,out.Get(),sGetStringLen(out.Get())))
    {
        sPrint("writing asm file failed\n");
        sSetExitCode(1);
        return;
    }

    // yasm

    if(!sAssemble(asmfile,outputfile,sConfigPlatform,x64!=0))
    {
        sPrint("yasm/nasm failed\n");
        sSetExitCode(1);
        return;
    }

    // done!
}

/****************************************************************************/
