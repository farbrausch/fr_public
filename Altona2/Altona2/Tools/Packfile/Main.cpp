/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Doc.hpp"


#define VERSION 1
#define REVISION 3

void Altona2::Main()
{
    sPrintF("packfile v%d.%d\n",VERSION,REVISION);

    // commandline

    sString<sMaxPath> inputfile;
    sString<sMaxPath> outputfile;
    sCommandlineParser cmd;
    cmd.AddHelp("?");
    cmd.AddFile("!i",inputfile);
    cmd.AddFile("!o",outputfile);

    // do it

    if(cmd.Parse())
    {
        wDocument Doc;
        if(Doc.Parse(inputfile))
        {
            if(Doc.Output(outputfile))
            {
                return;
            }
        }
    }
    sSetExitCode(1);
}

/****************************************************************************/
