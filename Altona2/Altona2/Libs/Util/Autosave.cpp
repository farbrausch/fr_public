/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Autosave.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sAutosaveRotation(const sStringDesc &filename,const char *ext)
{
    sString<sMaxPath> fold,fnew;
    if(!sCheckDir("backup"))
    {
        sMakeDir("backup");
    }

    fold.PrintF("backup/autosave_9.%s",ext);
    if(sCheckFile(fold))
        sDeleteFile(fold);

    for(int i=8;i>=0;i--)
    {
        fold.PrintF("backup/autosave_%d.%s",i,ext);
        fnew.PrintF("backup/autosave_%d.%s",i+1,ext);
        if(sCheckFile(fold))
            sRenameFile(fold,fnew);
    }
    fold.PrintF("backup/autosave_0.%s",ext);
    sCopyString(filename,fold);
}

/****************************************************************************/

sAutosave::sAutosave()
{
    Changed = 0;
    AutosaveTime = 0;
    AutosaveTimeout = 90*1000;
}

void sAutosave::SetChanged()
{
    Changed = 1;
    if(AutosaveTime == 0)
        AutosaveTime = sGetTimeMS()+AutosaveTimeout;
}

bool sAutosave::GetChanged()
{
    return Changed;
}

void sAutosave::ClearChanged()
{
    Changed = 0;
    AutosaveTime = 0;
}

bool sAutosave::NeedAutosave()
{
    return Changed && AutosaveTime!=0 && sGetTimeMS()>=AutosaveTime;
}

void sAutosave::AutosaveDone()
{
    AutosaveTime = 0;
}

/****************************************************************************/
}
