/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_SCREENS4_CONFIG_HPP
#define FILE_SCREENS4_CONFIG_HPP

#include "base/types.hpp"
#include "base/types2.hpp"
#include "util/scanner.hpp"

/****************************************************************************/

class Config
{
public:

  struct Resolution
  {
    sInt Width, Height, RefreshRate;
  };

  struct Note
  {
    sInt Pitch, Velocity;
    sF32 Duration;
  };

  enum EventType
  {
    PLAYSOUND, STOPSOUND, SETRESOLUTION, FULLSCREEN, MIDINOTE, DIM
  };

  struct KeyEvent
  {
    sU32 Key;
    EventType Type;
    sPoolString ParaStr;
    Resolution ParaRes;
    Note ParaNote;
  };

  sInt Port;
  sInt HttpPort;
  Resolution DefaultResolution;
  sBool DefaultFullscreen;
  sArray<KeyEvent> Keys;
  sF32 BarAnimTime;
  sF32 BarAnimSpread;
  sF32 MovieVolume; 
  sPoolString SlidePrefix;
  sPoolString TransPrefix;
  sPoolString DefaultType;
  sBool LockWhenDimmed;
  sBool Callbacks;

  sInt MidiDevice;
  sInt MidiChannel;

  sPoolString PngOut;

  Config()
  {
    Scan=0;
    sClear(DefaultResolution);
    DefaultFullscreen = sRELEASE;
    Port = 1234;
    HttpPort = 8080;
    BarAnimTime = 9;
    BarAnimSpread = 0.5;
    MovieVolume = 1.0;
    SlidePrefix = L"slide";
    TransPrefix = L"trans";
    DefaultType = L"";
    MidiDevice = -1;
    MidiChannel = 1;
    PngOut = L"";
    LockWhenDimmed = sFALSE;
		Callbacks = sFALSE;
  }

  sBool Read(const sChar *filename)
  {
    Scan = new sScanner;
    Scan->Init();
    Scan->DefaultTokens();
    if (!Scan->StartFile(filename)) return sFALSE;
    sBool ret = _Doc();
    if (!ret) ErrorMsg = Scan->ErrorMsg;
    delete Scan;
    return ret;
  }

  const sChar *GetError() { return ErrorMsg; }
 
private:

  sScanner *Scan;
  sString<1024> ErrorMsg;

  // parser functions
  sBool _Doc()
  {
    while (!Scan->Errors && Scan->Token!=sTOK_END)
    {
      if (Scan->IfName(L"defaults"))
        _Defaults();
      else if (Scan->IfName(L"keys"))
        _Keys();
    }
    return !Scan->Errors;
  }

  void _Resolution(Resolution &res)
  {
    res.Width = Scan->ScanInt();
    Scan->Match(',');
    res.Height = Scan->ScanInt();
    res.RefreshRate = Scan->IfToken(',') ? Scan->ScanInt() : 0;
  }

  void _Note(Note &n)
  {
    n.Pitch = sClamp(Scan->ScanInt(),0,127);
    Scan->Match(',');
    n.Velocity = sClamp(Scan->ScanInt(),0,127);
    Scan->Match(',');
    n.Duration = Scan->ScanFloat();
  }

  void _Defaults()
  {
    Scan->Match('{');
    while (!Scan->Errors && !Scan->IfToken('}'))
    {
      if (Scan->IfName(L"resolution"))
        _Resolution(DefaultResolution);
      else if (Scan->IfName(L"fullscreen"))
        DefaultFullscreen = Scan->ScanInt();
      else if (Scan->IfName(L"port"))
        Port = Scan->ScanInt();
      else if (Scan->IfName(L"httpport"))
        HttpPort = Scan->ScanInt();
      else if (Scan->IfName(L"bartime"))
        BarAnimTime = sClamp(Scan->ScanFloat(),0.1f,100.0f);
      else if (Scan->IfName(L"barspread"))
        BarAnimSpread = sClamp(Scan->ScanFloat(),0.0f,1.0f);
      else if (Scan->IfName(L"movievolume"))
        MovieVolume = sFPow(10.0f,sClamp(Scan->ScanFloat(),-100.0f,12.0f)/20.0f);
      else if (Scan->IfName(L"slideprefix"))
        Scan->ScanString(SlidePrefix);
      else if (Scan->IfName(L"transprefix"))
        Scan->ScanString(TransPrefix);
      else if (Scan->IfName(L"defaulttype"))
        Scan->ScanString(DefaultType);
      else if (Scan->IfName(L"mididevice"))
        MidiDevice = Scan->ScanInt();
      else if (Scan->IfName(L"midichannel"))
        MidiChannel = sClamp(Scan->ScanInt(),1,16);
      else if (Scan->IfName(L"pngout"))
        Scan->ScanString(PngOut);
      else if (Scan->IfName(L"lockwhendimmed"))
        LockWhenDimmed = Scan->ScanInt();
			else if (Scan->IfName(L"callbacks"))
			  Callbacks = Scan->ScanInt();
	  else
        Scan->Error(L"syntax error");
    }
  }

  void _Keys()
  {
    Scan->Match('{');
    while (!Scan->Errors && !Scan->IfToken('}'))
    {
      sU32 key = 0, qual = 0;

      // scan for qualifiers
      for (;;)
      {
        if (Scan->IfName(L"SHIFT"))
          qual|=sKEYQ_SHIFT;
        else if (Scan->IfName(L"SHIFT"))
          qual|=sKEYQ_CTRL;
        else if (Scan->IfName(L"ALT"))
          qual|=sKEYQ_ALT;
        else break;
      }

      // scan for key name
      if(Scan->Token==sTOK_CHAR)
      {
        key = Scan->ScanChar();
      }
      else if(Scan->Token==sTOK_NAME)
      {
             if(Scan->Name == L"UP")          key = sKEY_UP;
        else if(Scan->Name == L"DOWN")        key = sKEY_DOWN;
        else if(Scan->Name == L"LEFT")        key = sKEY_LEFT;
        else if(Scan->Name == L"RIGHT")       key = sKEY_RIGHT;
        else if(Scan->Name == L"HOME")        key = sKEY_HOME;
        else if(Scan->Name == L"END")         key = sKEY_END;
        else if(Scan->Name == L"PAGEUP")      key = sKEY_PAGEUP;
        else if(Scan->Name == L"PAGEDOWN")    key = sKEY_PAGEDOWN;
        else if(Scan->Name == L"INSERT")      key = sKEY_INSERT;
        else if(Scan->Name == L"DELETE")      key = sKEY_DELETE;
        else if(Scan->Name == L"BACKSPACE")   key = sKEY_BACKSPACE;
        else if(Scan->Name == L"TAB")         key = sKEY_TAB;
        else if(Scan->Name == L"ENTER")       key = sKEY_ENTER;
        else if(Scan->Name == L"ESCAPE")      key = sKEY_ESCAPE;
        else if(Scan->Name == L"ESC")         key = sKEY_ESCAPE;
        else if(Scan->Name == L"PAUSE")       key = sKEY_PAUSE;
        else if(Scan->Name == L"MENU")        key = sKEY_WINM;
        else if(Scan->Name == L"SPACE")      key = ' ';

        else if(Scan->Name == L"F1") key = sKEY_F1;
        else if(Scan->Name == L"F2") key = sKEY_F2;
        else if(Scan->Name == L"F3") key = sKEY_F3;
        else if(Scan->Name == L"F4") key = sKEY_F4;
        else if(Scan->Name == L"F5") key = sKEY_F5;
        else if(Scan->Name == L"F6") key = sKEY_F6;
        else if(Scan->Name == L"F7") key = sKEY_F7;
        else if(Scan->Name == L"F8") key = sKEY_F8;
        else if(Scan->Name == L"F9") key = sKEY_F9;
        else if(Scan->Name == L"F10") key = sKEY_F10;
        else if(Scan->Name == L"F11") key = sKEY_F11;
        else if(Scan->Name == L"F12") key = sKEY_F12;
        if(key==0)    
          Scan->Error(L"key expected");
        else
          Scan->Scan();
        if(key==1)
          key = 0;
      }
      else 
      {
        Scan->Error(L"key expected");
      }

      Scan->Match(':');

      KeyEvent ev;
      ev.Key = key|qual;

      if (Scan->IfName(L"playsound"))
      {
        ev.Type = PLAYSOUND;
        Scan->ScanString(ev.ParaStr);
      }
      else if (Scan->IfName(L"stopsound"))
      {
        ev.Type = STOPSOUND;
      }
      else if (Scan->IfName(L"resolution"))
      {
        ev.Type = SETRESOLUTION;
        _Resolution(ev.ParaRes);
      }
      else if (Scan->IfName(L"togglefullscreen"))
      {
        ev.Type = FULLSCREEN;
      }
      else if (Scan->IfName(L"toggledim"))
      {
        ev.Type = DIM;
      }
      else if (Scan->IfName(L"sendmidinote"))
      {
        ev.Type = MIDINOTE;
        _Note(ev.ParaNote);
      }
      else
        Scan->Error(L"event type expected");
      
      if (!Scan->Errors)
        Keys.AddTail(ev);
    }
  }
};


/****************************************************************************/

#endif // FILE_SCREENS4_CONFIG_HPP

