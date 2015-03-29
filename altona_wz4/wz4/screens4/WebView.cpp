#include "webview.hpp"

#if HAS_AWESOMIUM

#include "util/shaders.hpp"

#pragma comment(lib, "awesomium.lib")

#pragma warning (disable:4530)
#define __PLACEMENT_VEC_NEW_INLINE
#define __PLACEMENT_NEW_INLINE

#include <Awesomium/WebCore.h>
#include <Awesomium/WebSession.h>
#include <Awesomium/BitmapSurface.h>
#include <Awesomium/STLHelpers.h>

static Awesomium::WebCore *WebCore;
static Awesomium::WebSession *Session;
static sThreadLock *Locker;
static sDList<WebView, &WebView::AllViews> Views;
static int CurId;

class WebView::LoadListener : public Awesomium::WebViewListener::Load
{
public:

    sBool Finish, Fail;

    LoadListener()
    {
        Finish = sFALSE;
        Fail = sFALSE;
    }

    /// This event occurs when the page begins loading a frame.
    virtual void OnBeginLoadingFrame(Awesomium::WebView* caller, int64 frame_id, bool is_main_frame, const Awesomium::WebURL& url, bool is_error_page)
    {
        //sDPrintF(L"OnBeginLoadingFrame\n");
    }

    /// This event occurs when a frame fails to load. See error_desc
    /// for additional information.
    virtual void OnFailLoadingFrame(Awesomium::WebView* caller, int64 frame_id, bool is_main_frame, const Awesomium::WebURL& url, int error_code, const Awesomium::WebString& error_desc)
    {
        //sDPrintF(L"OnFailLoadingFrame\n");
        Fail = sTRUE;
    }

    /// This event occurs when the page finishes loading a frame.
    /// The main frame always finishes loading last for a given page load.
    virtual void OnFinishLoadingFrame(Awesomium::WebView* caller, int64 frame_id, bool is_main_frame, const Awesomium::WebURL& url)
    {
        //sDPrintF(L"OnFinishLoadingFrame\n");
        if (is_main_frame)
            Finish = sTRUE;
    }

    /// This event occurs when the DOM has finished parsing and the
    /// window object is available for JavaScript execution.
    virtual void OnDocumentReady(Awesomium::WebView* caller, const Awesomium::WebURL& url)
    {
        //sDPrintF(L"OnDocumentReady\n");
    }

};

WebView::WebView(const sChar *url, int width, int height)
{
    W = width;
    H = height;
    URL = url;
    Error = sFALSE;
    Surface = 0;
    View = 0;
    Mtrl = 0;
    Valid = sFALSE;
    Listener = new LoadListener();

    {
        sScopeLock lock(Locker);
        Views.AddTail(this);
        Id = CurId++;
    }
}

WebView::~WebView()
{
    {
        sScopeLock lock(Locker);
        Views.Rem(this);
    }

    if (View)
        View->Destroy();

    if (Mtrl)
        sDelete(Mtrl->Texture[0]);

    sDelete(Mtrl);
    sDelete(Listener);
}

void WebView::Release()
{
    delete this;
}


sMaterial *WebView::GetFrame(sFRect &uvrect)
{
    if (!Mtrl)
    {
        Mtrl = new sSimpleMaterial();
        Mtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF | sMTRL_VC_COLOR0;
        Mtrl->BlendColor = sMB_ALPHA;
        Mtrl->Texture[0] = new sTexture2D(W, H, sTEX_2D | sTEX_ARGB8888 | sTEX_NOMIPMAPS | sTEX_DYNAMIC);
        Mtrl->TFlags[0] = sMTF_CLAMP | sMTF_UV0;
        Mtrl->Prepare(sVertexFormatSingle);
    }

    if (Valid)
    {
        uvrect.Init(0, 0, 1, 1);
    }
    else
    {
        uvrect.Init(0, 0, 1, 1);
    }

    return Mtrl;

}

float WebView::GetAspect()
{
    return (float)W / (float)H;
}

void WebView::OnFrame()
{
    if (!View)
    {
        //sDPrintF(L"Creating view for %s (%dx%d)\n", URL, W, H);
        View = WebCore->CreateWebView(W, H, Session);
        View->set_load_listener(Listener);
            
        View->LoadURL(Awesomium::WebURL(Awesomium::WebString((const wchar16*)URL)));
        return;
    }

    if (Listener->Fail)
    {
        Error = sTRUE;
        return;
    }

    if (!Surface)
        Surface = (Awesomium::BitmapSurface*)View->surface();

    if (Listener->Finish && Mtrl && Surface->is_dirty())
    {
        //sDPrintF(L"Blit! %d\n", Id);
        sTexture2D *t2d = Mtrl->Texture[0]->CastTex2D();
        sVERIFY(t2d);
        sU8 *dest;
        sInt dpitch;
        t2d->BeginLoad(dest, dpitch);
        Surface->CopyTo(dest, dpitch, 4, false, false);
        t2d->EndLoad();

        Valid = sTRUE;
    }
}


static void OnFrame(void *user)
{
    WebCore->Update();
    WebView *view;
    sFORALL_LIST(Views, view)
        view->OnFrame();
}


static void InitAwe()
{
    Awesomium::WebConfig config;
    config.user_agent = Awesomium::WSLit("Mozilla / 5.0 (Windows NT 6.3; WOW64) AppleWebKit / 537.36 (KHTML, like Gecko) Chrome / 33.0.1750.117 Safari / 537.36");
    WebCore = Awesomium::WebCore::Initialize(config);

    Awesomium::WebPreferences prefs;
    prefs.user_stylesheet = Awesomium::WSLit("body { overflow:hidden; }");
    Session = WebCore->CreateWebSession(Awesomium::WSLit(""), prefs);

    sPreFlipHook->Add(OnFrame);
    Locker = new sThreadLock();
}

static void ExitAwe()
{
    sPreFlipHook->Rem(OnFrame);
    Session->Release();
    Awesomium::WebCore::Shutdown();
    WebCore = 0;
    delete Locker;
}

void AddWebView()
{
    sAddSubsystem(L"Awesomium", 0xe0, InitAwe, ExitAwe);
}

#else

WebView::WebView(const sChar *url, int width, int height)
{
    Error = sTRUE;
}

WebView::~WebView()
{
}

void WebView::Release()
{
    delete this;
}

sMaterial *WebView::GetFrame(sFRect &uvrect)
{
    uvrect.Init(0, 0, 0, 0);
    return 0;
}

float WebView::GetAspect()
{
    return 1;
}

void AddWebView()
{
}

#endif
