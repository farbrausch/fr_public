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
#define REVISION 2

void Altona2::Main()
{
    sPrintF("incbin v%d.%d\n",VERSION,REVISION);

    // commandline

    sString<sMaxPath> inputfile;
    sString<sMaxPath> outputfile;
    sInt hasoutputfile = 0;
    sInt hasinputfile = 0;
    sInt x64 = 0;
    sCommandlineParser cmd;
    cmd.AddHelp("?");
    cmd.AddFile("!i",inputfile,&hasinputfile,"source *.incbin.txt file");
    cmd.AddFile("!o",outputfile,&hasoutputfile,"output *.c file");
    if(!cmd.Parse() || !hasoutputfile || !hasinputfile)
    {
        sPrint("usage:\n");
        sPrint("incbin -i=inputfile.txt -o=outputfile.c\n");
        sSetExitCode(1);
        return;
    }

    // plaform?

    const sChar *prefix = "";
    if((sConfigPlatform==sConfigPlatformWin || sConfigPlatform==sConfigPlatformOSX) && !x64)
        prefix = "_";

    // create asm file

    sTextLog out;

    sScanner Scan;
    Scan.Init(sSF_CppComment);
    Scan.AddDefaultTokens();
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
                sString<sMaxPath> Path(inputfile);
                sStringDesc p(Path.GetBuffer(), sMaxPath);

                sRemoveName(p);
                if (Path.Count())
                    Path.Add("/");

                Path.Add(File);
                sCleanPath(p);
                
                                
                uptr size;
                uint8 *d = sLoadFile(Path.Get(), size);

                if (d)
                {
                    out.PrintF("unsigned char %s[] = {\n\t",Label);
                    for (uptr i=0;i<size;i++)
                    {
                        if ((i&63)==63) out.Print("\n\t");

                        out.PrintF("%d",d[i]);

                        if (i<(size-1)) out.Print(",");

                    }
                    out.PrintF("\n\t};//End %s\n",Label);
                    delete []d;
                }
                else
                {
                    Scan.Error("file not found: %q",Path);
                }
            }
        }
        else if(Scan.IfName("utf8") || Scan.IfName("text"))
        {
            Scan.ScanName(Label);
            Scan.ScanString(File);
            Scan.Match(';');
            if(!Scan.Errors)
            {
                sString<sMaxPath> Path(inputfile);
                sStringDesc p(Path.GetBuffer(), sMaxPath);

                sRemoveName(p);
                if (Path.Count())
                    Path.Add("/");
                Path.Add(File);
                sCleanPath(p);


                const sChar *text = sLoadText(Path);
                if(text==0)
                {
                    Scan.Error("file not found: %q",Path);
                }
                else
                {
                    out.PrintF("char %s[]= { \n\t",Label);
                    for (int i=0;text[i];i++)
                    {
                        if ((i&63)==63) out.Print("\n\t");
                        out.PrintF("%d,",text[i]);                        
                    }
                    out.PrintF("0\n\t};//End %s\n",Label);
                    
                    out.Print("#if 0\n");
                    out.Print(text);
                    out.Print("#endif\n");
                    delete[] text;
                }
            }
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

    // save c file


    sString<sMaxPath> cfile;

    cfile = outputfile;
    cfile.Add(".c");
    if(!sSaveFile(cfile,out.Get(),sGetStringLen(out.Get())))
    {
        sPrint("writing c file failed\n");
        sSetExitCode(1);
        return;
    }

    // done!
}

/****************************************************************************/
