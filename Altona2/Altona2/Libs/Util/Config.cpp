/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Scanner.hpp"
#include "Config.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/


static class sAltona2ConfigSubsystem : public sSubsystem
{
public:
    sAltona2ConfigSubsystem() : sSubsystem() { Initialized = 0; }

    struct ConfigString
    {
        sPoolString Key;
        sPoolString Value;

        ConfigString() {}
        ConfigString(sPoolString k,sPoolString v) { Key = k; Value = v; }
    };

    void Init()
    {
        sString<sMaxPath> user;
        sString<sMaxPath> path;
        sString<sMaxPath> file;

        sGetSystemPath(sSP_User,user);
        file.PrintF("%s/Altona2.txt",user);

        auto s = sLoadText(file);
        if(s)
        {
            Path = s;
            delete s;

            sScanner Scan;
            Scan.Init(sSF_CppComment);
            Scan.AddDefaultTokens();
            file.PrintF("%s/Altona2Config.txt",Path);
            Scan.StartFile(file);
            while(!Scan.Errors && Scan.Token!=sTOK_End)
            {
                sPoolString k = Scan.ScanName();
                Scan.Match('=');
                sPoolString v = Scan.ScanNameOrString();
                Scan.Match(';');
                if(!Scan.Errors)
                    ConfigStrings.Add(ConfigString(k,v));
            }

            if(!Scan.Errors)
            {
                sLogF("config","Altona2Config at %q",Path);
                for(auto s : ConfigStrings)
                    sLogF("config","%s=%q",s.Key,s.Value);
                Initialized = true;
            }
            else
            {
                sLogF("config","error in config file");
            }
        }
        else
        {
            sLogF("config","missing altona2 config %q",file);
        }
    }
    void Exit()
    {
        ConfigStrings.FreeMemory();
    }

    sArray<ConfigString> ConfigStrings;
    sString<sMaxPath> Path;
    bool Initialized;
} Altona2ConfigSubsystem;



void Altona2::sEnableAltonaConfig()
{
    Altona2ConfigSubsystem.Register("Altona2Config",0x70);
}

const char *Altona2::sGetConfigString(const char *key)
{
    auto k = sPoolString(key);
    auto e = Altona2ConfigSubsystem.ConfigStrings.FindValue([=](const sAltona2ConfigSubsystem::ConfigString &e){ return e.Key==k; });
    if(e)
        return e->Value;
    else
        return 0;
}

const char *Altona2::sGetConfigPath()
{
    if(Altona2ConfigSubsystem.Initialized)
        return Altona2ConfigSubsystem.Path;
    return 0;
}

/****************************************************************************/

