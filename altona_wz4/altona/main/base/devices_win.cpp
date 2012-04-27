/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"

#if sCONFIG_SYSTEM_WINDOWS

#include "base/devices.hpp"
#include "base/system.hpp"
#include <windows.h>

static sBool sMidiLog;

/****************************************************************************/
/****************************************************************************/

// there may be only one cosumer thread and one producer thread

template <class T,int max> class sLocklessQueue2
{
  T Data[max];
  volatile sInt Write;
  volatile sInt Read;
public:
  sLocklessQueue2()   { Write=0;Read=0;sVERIFY(sIsPower2(max)); }
  ~sLocklessQueue2()  {}

  // may be called by producer

  sBool IsFull()      { return Write >= Read+max; }
  void AddTail(const T &e)  { sInt i=Write; sVERIFY(i < Read+max); Data[i&(max-1)] = e; sWriteBarrier(); Write=i+1; }

  // may be called by consumer

  sBool IsEmpty()     { return Read >= Write; }
  sBool RemHead(T &e)        { sInt i=Read; if(i>=Write) return 0; e=Data[i&(max-1)]; sWriteBarrier(); Read=i+1; return 1; }

  // this is an approximation!

  sInt GetUsed()      { return Write-Read; }
};

/****************************************************************************/
/****************************************************************************/

struct sMidiIn
{
  HMIDIIN Handle;
  sString<128> Name;
};

struct sMidiOut
{
  HMIDIOUT Handle;
  sString<128> Name;
};

void CALLBACK sMidiInProc (HMIDIIN  in ,UINT msg,DWORD inst,DWORD p0,DWORD p1);
void CALLBACK sMidiOutProc(HMIDIOUT out,UINT msg,DWORD inst,DWORD p0,DWORD p1);

class sMidiHandlerWin : public sMidiHandler_
{
  friend void CALLBACK sMidiInProc (HMIDIIN  in ,UINT msg,DWORD inst,DWORD p0,DWORD p1);
  sArray<sMidiIn> In;
  sArray<sMidiOut> Out;
  sLocklessQueue2<sMidiEvent,1024> InQueue;
public:
  sMidiHandlerWin();
  ~sMidiHandlerWin();
  const sChar *GetDeviceName(sBool out,sInt dev);

  sBool HasInput();
  sBool GetInput(sMidiEvent &e);
  void Output(sMidiEvent &e);
  void Output(sU8 dev,sU8 chan,sU8 msg,sU8 val);
};

sMidiHandler_ *sMidiHandler;

/****************************************************************************/

void CALLBACK sMidiInProc(HMIDIIN in,UINT msg,DWORD inst,DWORD p0,DWORD p1)
{
  if(sMidiLog)
    sLogF(L"midi",L"in  %08x %08x %08x %08x %08x\n",sPtr(in),sU32(msg),sU32(inst),sU32(p0),sU32(p1));
  if(sMidiHandler)
  {
    sMidiHandlerWin *mh = (sMidiHandlerWin *) sMidiHandler;
    sMidiEvent ev;
    ev.TimeStamp = timeGetTime();
    ev.Device = inst;
    ev.Status = (p0>>0) & 0xff;
    ev.Value1 = (p0>>8) & 0xff;
    ev.Value2 = (p0>>16) & 0xff;
    if(!mh->InQueue.IsFull())
      mh->InQueue.AddTail(ev);
    if(mh->InputMsg.Target)
      mh->InputMsg.PostASync();
  }
}

void CALLBACK sMidiOutProc(HMIDIOUT out,UINT msg,DWORD inst,DWORD p0,DWORD p1)
{
  if(sMidiLog)
    sLogF(L"midi",L"out %08x %08x %08x %08x %08x\n",sPtr(out),sU32(msg),sU32(inst),sU32(p0),sU32(p1));
}


sMidiHandlerWin::sMidiHandlerWin()
{
  sInt max;
  sMidiIn *in;
  sMidiOut *out;
  sString<128> str;

  max = midiInGetNumDevs();
  In.HintSize(max);
  for(sInt i=0;i<max;i++)
  {
    HMIDIIN hnd;
    MIDIINCAPSW caps;
    sInt n = In.GetCount();
    MMRESULT r = midiInOpen(&hnd,i,(DWORD_PTR)sMidiInProc,n,CALLBACK_FUNCTION);
    if(r==MMSYSERR_NOERROR)
    {

      in = In.AddMany(1);
      str.PrintF(L"(%d)",n);
      if(midiInGetDevCapsW(i,&caps,sizeof(caps))==MMSYSERR_NOERROR)
        str = caps.szPname;

      in->Name = str;
      in->Handle = hnd;

      sLogF(L"midi",L"midi in %d = %q\n",n,str);

      midiInStart(hnd);
    }
  }

  max = midiOutGetNumDevs();
  Out.HintSize(max);
  for(sInt i=0;i<max;i++)
  {
    HMIDIOUT hnd;
    MIDIOUTCAPSW caps;
    sInt n = Out.GetCount();
    MMRESULT r = midiOutOpen(&hnd,i,(DWORD_PTR)sMidiOutProc,n,CALLBACK_FUNCTION);
    if(r==MMSYSERR_NOERROR)
    {
      out = Out.AddMany(1);
      str.PrintF(L"(%d)",n);
      if(midiOutGetDevCapsW(i,&caps,sizeof(caps))==MMSYSERR_NOERROR)
        str = caps.szPname;

      out->Name = str;
      out->Handle = hnd;

      sLogF(L"midi",L"midi out %d = %q\n",n,str);
    }
  }
}

const sChar *sMidiHandlerWin::GetDeviceName(sBool out,sInt dev)
{
  if(out)
  {
    if(dev>=0 && dev<Out.GetCount())
      return Out[dev].Name;
    else 
      return 0;
  }
  else
  {
    if(dev>=0 && dev<In.GetCount())
      return In[dev].Name;
    else 
      return 0;
  }
}

sMidiHandlerWin::~sMidiHandlerWin()
{
  sMidiIn *in;
  sMidiOut *out;

  sFORALL(In,in)
    midiInClose(in->Handle);
  sFORALL(Out,out)
    midiOutClose(out->Handle);

}

/****************************************************************************/

sBool sMidiHandlerWin::HasInput()
{
  return !InQueue.IsEmpty();
}

sBool sMidiHandlerWin::GetInput(sMidiEvent &e)
{
  return InQueue.RemHead(e);
}

void sMidiHandlerWin::Output(sU8 dev,sU8 stat,sU8 val1,sU8 val2)
{
  midiOutShortMsg(Out[dev].Handle,stat|(val1<<8)|(val2<<16));
}

/****************************************************************************/

void sInitMidi()
{
  sMidiHandler = new sMidiHandlerWin;
}

void sExitMidi()
{
  delete sMidiHandler;
}


void sAddMidi(sBool logging)
{
  sMidiLog = logging;
  sAddSubsystem(L"midi",0x81,sInitMidi,sExitMidi);
}

/****************************************************************************/
/****************************************************************************/

#endif

/****************************************************************************/
