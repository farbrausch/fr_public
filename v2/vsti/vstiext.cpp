// lr: additional export functions for v2vsti

#include "stdafx.h"

HINSTANCE hInstance;

static Console *con;

// zusammengefaktes kack for automation
class v2vstiEditor;

v2vstiEditor* theAEditor;
AudioEffectX* v2vstiAEffect;

#define lrBox(LRMSG) ::MessageBox(0,LRMSG,"Farbrausch V2 VSTi",MB_OK)

#include "resource.h"

#include "../types.h"
#include "../tool/file.h"
#include "../soundsys.h"
#include "../sounddef.h"
#include "../synth.h"

#include "midi.h"

#include "v2host.h"

#include "v2vsti.h"

#include "v2api.h"

//#include "wtlblaView.h"
#include "v2view.h"

CAppModule _Module;


BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH) {
		hInstance = hInst;
		HRESULT hRes = ::CoInitialize(NULL);
	// If you are running on NT 4.0 or higher you can use the following call instead to 
	// make the EXE free threaded. This means that calls come in on a random RPC thread.
	//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
		ATLASSERT(SUCCEEDED(hRes));

	#if 0
		INITCOMMONCONTROLSEX iccx;
		iccx.dwSize = sizeof(iccx);
		iccx.dwICC = ICC_BAR_CLASSES|ICC_PROGRESS_CLASS ;	// change to support other controls
		BOOL bRet = ::InitCommonControlsEx(&iccx);
		bRet;
		ATLASSERT(bRet);
	#else
		::InitCommonControls();
	#endif

		hRes = _Module.Init(NULL, hInstance);

        V2::GUI::Initialize(); // 4 gdiplus 

		ATLASSERT(SUCCEEDED(hRes));
	}	else if(dwReason == DLL_PROCESS_DETACH) {

        V2::GUI::Uninitialize(); // 4 gdiplus

		_Module.Term();
		::CoUninitialize();
	}
	return 1;
}

class v2vstiEditor : public AEffEditor {
public:
	ERect m_rect;
	V2::GUI::View m_view;
	AudioEffect* vsti;
	V2::Synth* m_synth;

	v2vstiEditor(AudioEffect* effect)
		: AEffEditor(effect)
	{
		m_view.VSTMode();
		m_synth = V2::GetTheSynth();
		vsti = effect;
		effect->setEditor(this);
		m_synth->SetCurrentPatchIndex(0);
	}

	~v2vstiEditor()
	{
		if (m_view.m_hWnd)
			m_view.DestroyWindow();
	}
	virtual long getRect(ERect **rect)      // CHAOS: cubase fix. this is called once before CreateWindow!
	{
		RECT clientRect;
    if(m_view.m_hWnd!=0)
    {
		  m_view.GetClientRect(&clientRect);
		  m_rect.left = (short)clientRect.left;
		  m_rect.top = (short)clientRect.top;
		  m_rect.right = (short)clientRect.right;
		  m_rect.bottom = (short)clientRect.bottom;
    }
    else
    {
		  m_rect.left = 0;
		  m_rect.top = 0;
		  m_rect.right = 0;
		  m_rect.bottom = 0;
    }
		*rect = &m_rect;
		return 0;
	}	
	virtual long open(void *ptr)
	{
		AEffEditor::open(ptr);			
		if(!m_view.m_hWnd)
		{
			m_synth->SetCurrentPatchIndex(vsti->getProgram());
			HWND m_hWndClient = m_view.Create((HWND)ptr); //, CWindow::rcDefault, "v2 frame", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		}
		//if (m_view.m_hWnd) m_view.SetWindowPos(0,0,0,420/*+10*/+280 -10,570+110,SWP_NOZORDER|SWP_SHOWWINDOW);
		return (long)m_view.m_hWnd;
	}
	virtual void close()
	{
		if (m_view.m_hWnd)
			m_view.ShowWindow(SW_HIDE);
	}
	virtual void update()
	{
		if (m_view.m_hWnd) 
			m_view.UpdateWindow();
	}
	//effect->setParameterAutomated (tag, control->getValue ());
	void SetProgram(UINT nr)
	{
		m_synth->SetCurrentPatchIndex(nr);
		if (m_view.m_hWnd) 
		{
			m_view.PostPatchChanged();
		}
	}
};

CMessageLoop* theLoop = NULL;

void __declspec(dllexport) v2vstiSetSampleRate(int newrate)
{
	/*
	char str[1024];
	sprintf(str,"changing SR to %i\n",newrate);
	lrBox(str);*/

#ifdef RONAN
	msSetSampleRate(newrate,soundmem,globals,(const char **)speechptrs);
#else
	msSetSampleRate(newrate,soundmem,globals);
#endif
}


extern "C" extern sU32 pbsize;
extern "C" extern sU8 presetbank[];

static void Init()
{
	theLoop = new CMessageLoop;
	_Module.AddMessageLoop(theLoop);

	con=0;
//	con = new Console;
	printf("an!\n");

	sdInit();
	msInit();

	printf("loading bank at %08x, size %d\n",presetbank,pbsize);

//	fileM presetbank;
//	printf("open: %d\n",presetbank.open(IDR_BANK1));
//	printf("load: %d\n",sdLoad(presetbank));
//	presetbank.close();

	fileM pb;
	pb.open(presetbank,pbsize);
	printf("load: %d\n",sdLoad(pb));

	printf("start audio\n");
#ifdef RONAN
	msStartAudio(0,soundmem,globals,(const char **)speechptrs);
#else
	msStartAudio(0,soundmem,globals);
#endif	

	printf("done\n");

}

static void Shutdown()
{
    // es gibt so ein paar orte, da macht man besser kein printf.
	// printf("ShutDown\n");
	msStopAudio();

	msClose();

    if (con)
	    delete con;

	_Module.RemoveMessageLoop();
}

__declspec(dllexport) AEffEditor* v2vstiInit(AudioEffectX* effect)
{
	v2vstiAEffect = effect;

    Init();

	return theAEditor = new v2vstiEditor(effect); 
}

__declspec(dllexport) void v2buzzInit()
{
    Init();
}

void v2vstiReset()
{
	printf("reset\n");
	V2::GetTheSynth()->Reset();
}


#define	 VPE_PARAMCHG		 0xF3

void  v2vstiUpdateUI(int program, int index)
{
	printf("UpdateUI %d,%d\n",program,index);
	if (theAEditor->m_view.m_hWnd) ::SendMessage(theAEditor->m_view,VPE_PARAMCHG,(WPARAM)program,(LPARAM)index);
}


void  __declspec(dllexport) v2vstiShutdown(AEffEditor* editor)
{
	printf("ShutDown\n");
	if(editor)
    {
		v2vstiEditor* ed = (v2vstiEditor*)editor;
		delete ed;
	}
}

void  __declspec(dllexport) v2buzzShutdown()
{
    Shutdown();
}

extern sU8 *theSynth;

static unsigned char* chunk = NULL;
long  __declspec(dllexport) v2vstiGetChunk(void** data, bool isPreset)
{
	printf("getChunk %08x,%d\n",data,(int)isPreset);

	//char buf[256];
	//sprintf(buf,"%08x %d\n",data,(int)isPreset);
	//MessageBox(0,buf,"getChunk",MB_OK);

	if(chunk) delete[] chunk;

	fileMTmp temp;
	temp.open();

	temp.putsU32('hund');
	int version = 1;
	temp.write(version);
	temp.putsU32('PGM0');
	int pgm[16];
	synthGetPgm(theSynth,pgm);
	temp.write(pgm,16*4);

	sdSaveBank(temp);

	V2::GUI::GetTheAppearance()->SaveAppearance(temp);

	int chunksize;
	temp.seek(0);
	chunksize=temp.size();
	chunk = new sU8[chunksize];
	temp.read(chunk,chunksize);
	temp.close();

	*data = chunk;
	return chunksize;
}

long  __declspec(dllexport) v2vstiSetChunk(void* data, long byteSize, bool isPreset)
{
	printf("setChunk %08x,%d,%d\n",data,byteSize,(int)isPreset);

	fileM chunk;
	chunk.open(data,byteSize);

	int pgm[16];
	int version = 0;

	sU32 pgm0=chunk.getsU32();
	if (pgm0=='hund')
	{
		chunk.read(version);
		pgm0 = chunk.getsU32();
	}
	if (pgm0=='PGM0')
		chunk.read(pgm,4*16);
	else
		chunk.seekcur(-4);

	sdLoad(chunk);

	if (version >= 1)
	{
		V2::GUI::GetTheAppearance()->LoadAppearance(chunk);
	}

#ifdef RONAN
	msStartAudio(0,soundmem,globals,(const char **)speechptrs);
#else
	msStartAudio(0,soundmem,globals);
#endif

	if (pgm0=='PGM0')
	{
#ifdef SINGLECHN
		msProcessEvent(0,(pgm[0]<<8)|0xc0);
#else
		for (sU32 i=0; i<16; i++)
			msProcessEvent(0,(pgm[i]<<8)|0xc0|i);
#endif
	}

	return 0;
}

void  __declspec(dllexport) v2vstiGetParameters(unsigned char** psoundmem, char** pglobals)
{
	*psoundmem = soundmem;
	*pglobals = (char*)globals;
}

__declspec(dllexport) char* v2vstiGetPatchNames()
{
	return (char*)patchnames;
}

void  __declspec(dllexport) v2vstiGetParameterDefs
	(V2TOPIC** topics, int* ntopics, V2PARAM** parms, int* nparms, 
	 V2TOPIC** gtopics, int* ngtopics, V2PARAM** gparms, int* ngparms)
{
	*topics = (V2TOPIC*)v2topics;
	*ntopics = v2ntopics;
	*parms = (V2PARAM*)v2parms;
	*nparms = v2nparms;
	*gtopics = (V2TOPIC*)v2gtopics;
	*ngtopics = v2ngtopics;
	*gparms = (V2PARAM*)v2gparms;
	*ngparms = v2ngparms;
}

void __stdcall msAudioCallback(float *left, float *right, unsigned int len, int mixwithsource);

void  __declspec(dllexport) v2vstiProcess(float** inputs, float** outputs, long sampleframes)
{
	msAudioCallback(outputs[0],outputs[1],sampleframes,1);
}

void  __declspec(dllexport) v2vstiProcessReplacing(float** inputs, float** outputs, long sampleframes)
{
	msAudioCallback(outputs[0],outputs[1],sampleframes,0);
}

long  __declspec(dllexport) v2vstiProcessEvents(VstEvents* events)
{
	for(int i=0;i<events->numEvents;i++) 
	{
		VstMidiEvent* me = (VstMidiEvent*)events->events[i];
		if(me->type == kVstMidiType)
		{
			msProcessEvent(me->deltaFrames,*(DWORD*)me->midiData);
		}
	}

	return 1;
}

void __declspec(dllexport) v2buzzProcessEvent(DWORD dwOffset, DWORD dwMidiData)
{    
    msProcessEvent(dwOffset,dwMidiData);
}

// stereo
void __declspec(dllexport) v2buzzWork(float* pSamples, int nSampleCount)
{
    float* pL = new float[nSampleCount];
    float* pR = new float[nSampleCount];

    float* pLPtr = pL;
    float* pRPtr = pR;
    msAudioCallback(pL,pR,(DWORD)nSampleCount,0);
    do
    {
        *pSamples++ = (*pLPtr++) * 32767.0f;
        *pSamples++ = (*pRPtr++) * 32767.0f;
    } while (--nSampleCount);

    delete[] pL;
    delete[] pR;
}

void __declspec(dllexport) v2buzzDestroyEditor()
{
	if(V2::g_BuzzEditor)
    {
        if(*V2::g_BuzzEditor)
            V2::g_BuzzEditor->DestroyWindow();
        delete V2::g_BuzzEditor;
        V2::g_BuzzEditor = NULL;
    }
}

void __declspec(dllexport) v2buzzShowEditor(HWND hWndParent)
{
    if (!V2::g_BuzzEditor)
    {
        V2::g_BuzzEditor = new V2::GUI::Frame();
    }

    if (*V2::g_BuzzEditor)
    {
        V2::g_BuzzEditor->ShowWindow(SW_SHOW);
    }
    else
    {
		V2::g_BuzzEditor->Create(hWndParent);
    }
}

void __declspec(dllexport) v2vstiSetProgram(AEffEditor* editor, UINT nr)
{
	printf("setProgram %d\n",nr);
    if (V2::g_BuzzEditor)
        V2::g_BuzzEditor->SetProgram((int)nr);
    else
	    ((v2vstiEditor*)editor)->SetProgram(nr);
}

void __declspec(dllexport) v2buzzSetChannel(int channel)
{
	if (V2::g_BuzzEditor)
		V2::g_BuzzEditor->SetChannel(channel);
}

// heh, sobald man irgendwas C++ spezifisches benutzt, 
// wird der exportname sofort dekoriert. daher:
extern "C"
{
	__declspec(dllexport) V2::IHost& v2GetHost()
	{
		return V2::g_theHost;
	}
}