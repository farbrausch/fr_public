/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"

#if sCONFIG_OPTION_NPP

#include "base/types2.hpp"
#include "base/system.hpp"
#include "base/input2.hpp"
#include "base/sound.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <npapi.h>
#include <npupp.h>

/****************************************************************************/
/****************************************************************************/

// NP_* implementation

NPNetscapeFuncs NPNFuncs;

NPError OSCALL NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
  if(pFuncs == NULL)
	{
    return NPERR_INVALID_FUNCTABLE_ERROR;
	}

  pFuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
	pFuncs->size					= sizeof(NPPluginFuncs);
  pFuncs->newp          = NPP_New;
  pFuncs->destroy       = NPP_Destroy;
  pFuncs->setwindow     = NPP_SetWindow;
  pFuncs->newstream     = NPP_NewStream;
  pFuncs->destroystream = NPP_DestroyStream;
  pFuncs->asfile        = NPP_StreamAsFile;
  pFuncs->writeready    = NPP_WriteReady;
  pFuncs->write         = NPP_Write;
  pFuncs->print         = NPP_Print;
  pFuncs->event         = NPP_HandleEvent;
  pFuncs->urlnotify     = NPP_URLNotify;
  pFuncs->getvalue      = NPP_GetValue;
  pFuncs->setvalue      = NPP_SetValue;
  pFuncs->javaClass     = NULL;

  return NPERR_NO_ERROR;
}

NPError OSCALL NP_Initialize(NPNetscapeFuncs* pFuncs)
{
  if(pFuncs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if(HIBYTE(pFuncs->version) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  NPNFuncs = *pFuncs;
  return NPP_Initialize();
}

NPError OSCALL NP_Shutdown()
{
  NPP_Shutdown();
  return NPERR_NO_ERROR;
}

/****************************************************************************/
/****************************************************************************/


extern HINSTANCE WInstance;
extern void sInitEmergencyThread();
extern void sInitKeys();
extern void SetXSIModeD3D(sBool enable);
extern LRESULT WINAPI MsgProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam);
extern void sCollector(sBool exit);
extern sInt sSystemFlags;
extern void ExitGFX();
extern void ResizeGFX(sInt x,sInt y);
extern HWND sHWND, sExternalWindow;
extern sApp *sAppPtr;
extern sInt sExitFlag;
extern sInt sFatalFlag;
extern sBool sAllocMemEnabled;
extern sInt DXMayRestore;
extern HDC sGDIDC;
extern void Render3D();
extern void sPingSound();

#define INPUT2_EVENT_QUEUE_SIZE 64
extern sLockQueue<sInput2Event, INPUT2_EVENT_QUEUE_SIZE>* sInput2EventQueue;

LONG OldMsgProc=0;

static sThread *AltonaThread = 0;
static sThreadEvent *ExitEvent = 0;

static void AltonaThreadFunc(sThread *thread, void *user)
{
  // initialize
  //  WConOut = GetStdHandle(STD_OUTPUT_HANDLE);
  //  WConIn  = GetStdHandle(STD_INPUT_HANDLE);
  //  sParseCmdLine(sGetCommandLine());

  sInitKeys();
  sSetRunlevel(0x80);
  SetXSIModeD3D(sTRUE);

  sMain();

  while (thread->CheckTerminate())
  {
    sFrameHook->Call();
    if (sInput2EventQueue) {
      sInput2Event event;
      while (sInput2PopEvent(event, sGetTime())) {
#if !sSTRIPPED
        sBool skip = sFALSE;
        sInputHook->Call(event, skip);
        if (!skip)
#endif
          sAppPtr->OnInput(event);
      }
    }

    sBool app_fullpaint = sFALSE;
#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11
    DXMayRestore = 1;
    if(sGetApp())
      app_fullpaint = sGetApp()->OnPaint();
    DXMayRestore = 0;
#endif

    if(!app_fullpaint && sGetApp())
      sGetApp()->OnPrepareFrame();

    /*
#if sRENDERER==sRENDER_DX11   // mixed GDI & DX11. complicated.
    if((sSystemFlags & sISF_2D) && (sSystemFlags & sISF_3D) && !sExitFlag && !app_fullpaint && sAppPtr)
    {
      PAINTSTRUCT ps;
      BeginPaint(win,&ps);
      sRect update(ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
      sRect client;
      GetClientRect(win,(RECT *) &client);
      EndPaint(win,&ps);

      DXMayRestore = 1;
      if(sRender3DBegin())
      {
        if(!client.IsEmpty())
        {
          sRender2DBegin();
          sAppPtr->OnPaint2D(client,update);
          sRender2DEnd();
        }
        if(sGetApp())
          sGetApp()->OnPaint3D();

        sRender3DEnd();
      }
      DXMayRestore = 0;
      sCollector();
    }
    else
#endif
    */
    {     
      if(!app_fullpaint && (sSystemFlags & sISF_3D))
      {
#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11
        DXMayRestore = 1;
        Render3D();
        DXMayRestore = 0;
#else
        Render3D();
#endif
      }
    }
    sPingSound();

    if (sExitFlag) thread->Terminate();
  }

  sDelete(sAppPtr);
  sCollect();
  sCollector(sTRUE);
  sSetRunlevel(0x80);

  if(sSystemFlags & sISF_3D)
    ExitGFX();

  sSetRunlevel(0x00);
}

/****************************************************************************/

LRESULT WINAPI NppMsgProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam)
{

  //  sDPrintF(L"msg(%0x8) %08x %08x %08x\n",sGetTime(),msg,wparam,lparam);
  //sInt i,t;
  //sBool mouse=0;

  switch(msg)
  {
  case WM_CLOSE:
    break;

  case WM_DESTROY:
    break;

  case WM_CREATE:     
    break;

  case WM_PAINT:
    if((sSystemFlags & sISF_2D) && !sExitFlag)  // sExitFlag is checked in case of windows crash during destruction
    {
      PAINTSTRUCT ps;

      BeginPaint(win,&ps);
      sGDIDC = ps.hdc;
      sRect update(ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
      sRect client;
      GetClientRect(win,(RECT *) &client);
      if(sAppPtr && !sFatalFlag)
        sAppPtr->OnPaint2D(client,update);
      EndPaint(win,&ps);
      sGDIDC = 0;
      //      sPingSound();
      sCollector(sFALSE);
    }
    else
    {
      ValidateRect(win,0);
    }
    break;

    /*
  case WM_PAINT:
    {
      if(!AltonaInited) // not fully initialized yet. just paint a black rect!
      {
        PAINTSTRUCT ps;
        BeginPaint(win,&ps);
        FillRect(ps.hdc,&ps.rcPaint,(HBRUSH) GetStockObject(BLACK_BRUSH));
        EndPaint(win,&ps);
        return 0;
      }

      if (sInPaint>1) break;
      if (sInPaint) 
        sDPrintF(L"warning: recursive paint detected\n");
      sInPaint++;

      win = sExternalWindow ? sExternalWindow : win;              // use external window if available
     
#if !sCONFIG_OPTION_NPP
      if((sSystemFlags & sISF_CONTINUOUS) && !sWin32::ModalDialogActive || sExternalWindow)
      {
        InvalidateRect(win,0,0);
      }
#endif

      sInPaint=0;
    }
    break;
  case WM_TIMER:
    if(!sExitFlag && !sFatalFlag)
    {
#if sCONFIG_OPTION_NPP
      if (wparam==0x1234)
      {
        InvalidateRect(win,0,0);
        break;
      }
#endif
      sPingSound();
      sAppPtr->OnEvent(sAE_TIMER);
      if(TimerEventTime)
        SetTimer(win,1,TimerEventTime,0);
    }
    break;
    */

  case WM_COMMAND:
    if((wparam & 0x8000) && !sFatalFlag)
      sAppPtr->OnEvent(wparam&0x7fff);
    break;

  case WM_ACTIVATE:
    sInput2ClearQueue();
    if(sSystemFlags & sISF_FULLSCREEN)
      sActivateHook->Call((wparam&0xffff)!=WA_INACTIVE);
    else
      sActivateHook->Call(!(wparam&0xffff0000));
    return DefWindowProc(win,msg,wparam,lparam);

  case WM_SIZE:
    if(wparam == SIZE_MINIMIZED && sActivateHook)
    {
      // We only need to send a deactivate event,
      // because activation is handled in WM_ACTIVATE.
      sActivateHook->Call(sFALSE);
    }

    if(!(sSystemFlags&sISF_FULLSCREEN))
    {
      sInt x = sInt(lparam&0xffff);
      sInt y = sInt(lparam>>16);
      ResizeGFX(x,y);
    }
    InvalidateRect(win,0,0);
    break;

  default:
    return MsgProc(win,msg,wparam,lparam);
  }

  return 1;
}

/****************************************************************************/
/****************************************************************************/

char* NPP_GetMIMEDescription()
{
  return "application/x-cube-plugin";
}


NPError NPP_Initialize()
{
  WInstance = GetModuleHandle(0);

  sVERIFY(sizeof(sChar)==2);
  sInitEmergencyThread();

  sInitMem0();
  sGetMemHandler(sAMF_HEAP)->MakeThreadSafe();

  ExitEvent = new sThreadEvent;

  //CoInitialize(NULL);
  sFrameHook = new sHooks;
  sNewDeviceHook = new sHooks;
  sActivateHook = new sHooks1<sBool>;
  sCrashHook = new sHooks1<sStringDesc>;
  sAltF4Hook = new sHooks1<sBool &>;
  sCheckCapsHook = new sHooks;
#if !sSTRIPPED
  sInputHook = new sHooks2<const sInput2Event &,sBool &>;
  sDebugOutHook = new sHooks1<const sChar*>;
#endif

  return NPERR_NO_ERROR;
}

void NPP_Shutdown()
{
  sDelete(sFrameHook);
  sDelete(sNewDeviceHook);
  sDelete(sActivateHook);
  sDelete(sCrashHook);
  sDelete(sAltF4Hook);
  sDelete(sCheckCapsHook);
#if !sSTRIPPED
  sDelete(sDebugOutHook);
  sDelete(sInputHook);
#endif

  sDelete(ExitEvent);

  sExitMem0();
}

/*
string GetCurrentUrl (NPP _npp)
{
  NPObject *pWindowObj;

  // Get the window object.
  if (NPN_GetValue (_npp, NPNVWindowNPObject, &pWindowObj) != NPERR_NO_ERROR)
  {
    return "ERR_WINDOWOBJECT";
  }

  // Create a "location" identifier.
  NPIdentifier identifier = NPN_GetStringIdentifier( "location" );

  // Declare a local variant value.
  NPVariant variantValue;

  // Get the location property from the window object (which is another object).
  bool b1 = NPN_GetProperty (_npp, pWindowObj, identifier, &variantValue );

  NPN_ReleaseObject(pWindowObj);


  if (!b1)
  {
    return "ERR_LOCATIONPROP";
  }

  if (!NPVARIANT_IS_OBJECT (variantValue))
  {
    return "ERR_VARIANTISNOTOBJECT";
  }

  // Get a pointer to the "location" object.
  NPObject *locationObj = variantValue.value.objectValue;

  // Create a "href" identifier.
  identifier = NPN_GetStringIdentifier( "href" );

  NPVariant locationValue;

  // Get the location property from the location object.
  bool b2 = NPN_GetProperty (_npp, locationObj, identifier, &locationValue );

  NPN_ReleaseVariantValue (&variantValue);

  if (!b2)
  {
    return "ERR_LOCATIONVALUE";
  }

  if (!NPVARIANT_IS_STRING (locationValue))
  {
    NPN_ReleaseVariantValue (&locationValue);
    return "ERR_LOCATIONVALUEISNOTSTRING";
  }

  std::string ret = NPVARIANT_TO_STRING(locationValue).utf8characters;
  NPN_ReleaseVariantValue (&locationValue);

  return ret;
}
*/


volatile static sU32 nInstances = 0;

// here the plugin creates an instance of our CPlugin object which 
// will be associated with this newly created plugin instance and 
// will do all the neccessary job
NPError NPP_New(NPMIMEType pluginType,
                NPP instance,
                uint16 mode,
                int16 argc,
                char* argn[],
                char* argv[],
                NPSavedData* saved)
{ 
  if(instance == NULL) 
    return NPERR_INVALID_INSTANCE_ERROR;

  sU32 i=sAtomicSwap(&nInstances,1);
  if (i)
    return NPERR_MODULE_LOAD_FAILED_ERROR;

  //if (!bAcceptURL)
  //{
  //    return NPERR_INVALID_URL;			
  // }

  NPError rv = NPERR_NO_ERROR;
  instance->pdata = (void *)(sDInt)0xc007babe;
  return rv;
}


// here is the place to clean up and destroy the CPlugin object
NPError NPP_Destroy (NPP instance, NPSavedData** save)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  if(instance->pdata != (void *)(sDInt)0xc007babe)
    return NPERR_NO_ERROR;

  if (sExternalWindow && OldMsgProc)
  {
    SetWindowLong(sExternalWindow,GWL_WNDPROC,OldMsgProc);
  }
  sExternalWindow=0;
  OldMsgProc=0;

  AltonaThread->Terminate();
  sDelete(AltonaThread);

 
  NPError rv = NPERR_NO_ERROR;

  sU32 i=sAtomicSwap(&nInstances,0);
  if (!i)
    rv = NPERR_MODULE_LOAD_FAILED_ERROR;

  return rv;
}

// during this call we know when the plugin window is ready or
// is about to be destroyed so we can do some gui specific
// initialization and shutdown

NPError NPP_SetWindow (NPP instance, NPWindow* pNPWindow)
{ 
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;

  if(pNPWindow == NULL)
    return NPERR_GENERIC_ERROR;

  if (pNPWindow)
  {
    sDPrintF(L"an\n");
    if (!sExternalWindow)
    {
      sExternalWindow = (HWND)pNPWindow->window;
    
      AltonaThread = new sThread(AltonaThreadFunc,0,131072);

      // subclass window
      OldMsgProc=SetWindowLong(sExternalWindow,GWL_WNDPROC,(LONG)(sDInt)NppMsgProc);
    }
  }
  else
  {
    sDPrintF(L"aus\n");
  }

  return rv;
}


NPError	NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;

  if(instance == NULL)
    return NPERR_GENERIC_ERROR;

  switch (variable) 
  {
  case NPPVpluginWindowBool:
    *((PRBool *)value) = PR_TRUE;
    break;

  case NPPVpluginNameString:
    *((char **)value) = "Boilerplate Plugin";
    break;

  case NPPVpluginDescriptionString:
    *((char **)value) = "Boilerplate web plugin";
    break;

    /*
  case NPPVpluginScriptableNPObject:

    if (!plugin->isInitialized())
    {
      return NPERR_GENERIC_ERROR;
    }

    *((NPObject **)value) = plugin->GetScriptableObject();

    break;
*/
  default:
    rv = NPERR_GENERIC_ERROR;
    break;
  }

  return rv;
}


NPError NPP_NewStream(NPP instance,
                      NPMIMEType type,
                      NPStream* stream, 
                      NPBool seekable,
                      uint16* stype)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;
  return rv;
}

int32 NPP_WriteReady (NPP instance, NPStream *stream)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  int32 rv = 0x0fffffff;
  return rv;
}

int32 NPP_Write (NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{   
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  int32 rv = len;
  return rv;
}

NPError NPP_DestroyStream (NPP instance, NPStream *stream, NPError reason)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;
  return rv;
}

void NPP_StreamAsFile (NPP instance, NPStream* stream, const char* fname)
{
  if(instance == NULL)
    return;
}

void NPP_Print (NPP instance, NPPrint* printInfo)
{
  if(instance == NULL)
    return;
}

void NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
  if(instance == NULL)
    return;
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;
  return rv;
}

int16	NPP_HandleEvent(NPP instance, void* event)
{
  if(instance == NULL)
    return 0;
  int16 rv = NPERR_NO_ERROR;
  /*
  CPlugin * pPlugin = (CPlugin *)instance->pdata;
  if (pPlugin)
    rv = pPlugin->handleEvent(event);
    */

  return rv;
}

/****************************************************************************/
/****************************************************************************/

// wrappers for NPN_* functions

void NPN_Version(int* plugin_major, int* plugin_minor, int* netscape_major, int* netscape_minor)
{
  *plugin_major   = NP_VERSION_MAJOR;
  *plugin_minor   = NP_VERSION_MINOR;
  *netscape_major = HIBYTE(NPNFuncs.version);
  *netscape_minor = LOBYTE(NPNFuncs.version);
}

NPError NPN_GetURLNotify(NPP instance, const char *url, const char *target, void* notifyData)
{
  return NPNFuncs.geturlnotify(instance, url, target, notifyData);
}

NPError NPN_GetURL(NPP instance, const char *url, const char *target)
{
  return NPNFuncs.geturl(instance, url, target);
}

NPError NPN_PostURLNotify(NPP instance, const char* url, const char* window, uint32 len, const char* buf, NPBool file, void* notifyData)
{
  return NPNFuncs.posturlnotify(instance, url, window, len, buf, file, notifyData);
}

NPError NPN_PostURL(NPP instance, const char* url, const char* window, uint32 len, const char* buf, NPBool file)
{
  return NPNFuncs.posturl(instance, url, window, len, buf, file);
} 

NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
  return NPNFuncs.requestread(stream, rangeList);
}

NPError NPN_NewStream(NPP instance, NPMIMEType type, const char* target, NPStream** stream)
{ 
  return NPNFuncs.newstream(instance, type, target, stream);
}

int32 NPN_Write(NPP instance, NPStream *stream, int32 len, void *buffer)
{
  return NPNFuncs.write(instance, stream, len, buffer);
}

NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
  return NPNFuncs.destroystream(instance, stream, reason);
}

void NPN_Status(NPP instance, const char *message)
{
  NPNFuncs.status(instance, message);
}

const char* NPN_UserAgent(NPP instance)
{
  return NPNFuncs.uagent(instance);
}

void* NPN_MemAlloc(uint32 size)
{
  return NPNFuncs.memalloc(size);
}

void NPN_MemFree(void* ptr)
{
  NPNFuncs.memfree(ptr);
}

uint32 NPN_MemFlush(uint32 size)
{
  return NPNFuncs.memflush(size);
}

void NPN_ReloadPlugins(NPBool reloadPages)
{
  NPNFuncs.reloadplugins(reloadPages);
}

JRIEnv* NPN_GetJavaEnv(void)
{
  return NPNFuncs.getJavaEnv();
}

jref NPN_GetJavaPeer(NPP instance)
{
  return NPNFuncs.getJavaPeer(instance);
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value)
{
  return NPNFuncs.getvalue(instance, variable, value);
}

NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value)
{
  return NPNFuncs.setvalue(instance, variable, value);
}

void NPN_InvalidateRect(NPP instance, NPRect *invalidRect)
{
  NPNFuncs.invalidaterect(instance, invalidRect);
}

void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
{
  NPNFuncs.invalidateregion(instance, invalidRegion);
}

void NPN_ForceRedraw(NPP instance)
{
  NPNFuncs.forceredraw(instance);
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name)
{
  return NPNFuncs.getstringidentifier(name);
}

void NPN_GetStringIdentifiers(const NPUTF8 **names, uint32_t nameCount, NPIdentifier *identifiers)
{
  return NPNFuncs.getstringidentifiers(names, nameCount, identifiers);
}

NPIdentifier NPN_GetIntIdentifier(int32_t intid)
{
  return NPNFuncs.getintidentifier(intid);
}

bool NPN_IdentifierIsString(NPIdentifier identifier)
{
  return NPNFuncs.identifierisstring(identifier);
}

NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier identifier)
{
  return NPNFuncs.utf8fromidentifier(identifier);
}

int32_t NPN_IntFromIdentifier(NPIdentifier identifier)
{
  return NPNFuncs.intfromidentifier(identifier);
}

NPObject *NPN_CreateObject(NPP npp, NPClass *aClass)
{
  return NPNFuncs.createobject(npp, aClass);
}

NPObject *NPN_RetainObject(NPObject *obj)
{
  return NPNFuncs.retainobject(obj);
}

void NPN_ReleaseObject(NPObject *obj)
{
  return NPNFuncs.releaseobject(obj);
}

bool NPN_Invoke(NPP npp, NPObject* obj, NPIdentifier methodName, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
  return NPNFuncs.invoke(npp, obj, methodName, args, argCount, result);
}

bool NPN_InvokeDefault(NPP npp, NPObject* obj, const NPVariant *args,
                       uint32_t argCount, NPVariant *result)
{
  return NPNFuncs.invokeDefault(npp, obj, args, argCount, result);
}

bool NPN_Evaluate(NPP npp, NPObject* obj, NPString *script, NPVariant *result)
{
  return NPNFuncs.evaluate(npp, obj, script, result);
}

bool NPN_GetProperty(NPP npp, NPObject* obj, NPIdentifier propertyName, NPVariant *result)
{
  return NPNFuncs.getproperty(npp, obj, propertyName, result);
}

bool NPN_SetProperty(NPP npp, NPObject* obj, NPIdentifier propertyName, const NPVariant *value)
{
  return NPNFuncs.setproperty(npp, obj, propertyName, value);
}

bool NPN_RemoveProperty(NPP npp, NPObject* obj, NPIdentifier propertyName)
{
  return NPNFuncs.removeproperty(npp, obj, propertyName);
}

bool NPN_Enumerate(NPP npp, NPObject *obj, NPIdentifier **identifier, uint32_t *count)
{
  return NPNFuncs.enumerate(npp, obj, identifier, count);
}

bool NPN_Construct(NPP npp, NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
  return NPNFuncs.construct(npp, obj, args, argCount, result);
}

bool NPN_HasProperty(NPP npp, NPObject* obj, NPIdentifier propertyName)
{
  return NPNFuncs.hasproperty(npp, obj, propertyName);
}

bool NPN_HasMethod(NPP npp, NPObject* obj, NPIdentifier methodName)
{
  return NPNFuncs.hasmethod(npp, obj, methodName);
}

void NPN_ReleaseVariantValue(NPVariant *variant)
{
  NPNFuncs.releasevariantvalue(variant);
}

void NPN_SetException(NPObject* obj, const NPUTF8 *message)
{
  NPNFuncs.setexception(obj, message);
}


/****************************************************************************/
/****************************************************************************/

#endif // sCONFIG_OPTION_NPP