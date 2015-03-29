#ifndef FILE_SCREENS4_WEBVIEW_HPP
#define FILE_SCREENS4_WEBVIEW_HPP

#define HAS_AWESOMIUM 0

#include "base/types.hpp"
#include "base/graphics.hpp"
#include "util/image.hpp"

void AddWebView();

namespace Awesomium
{
    class WebView;
    class BitmapSurface;
}

class WebView
{
public:

    WebView(const sChar *url, int width, int height);
    void Release();

    sMaterial *GetFrame(sFRect &uvrect);
    float GetAspect();
    sBool Error;

    void OnFrame();
    sDNode AllViews;

private:

    class LoadListener;

    ~WebView();
    int W, H;
    const sChar *URL;
    int Id;
    sBool Valid;

    sMaterial *Mtrl;
    void Blit();

    Awesomium::WebView *View;
    Awesomium::BitmapSurface *Surface;
    LoadListener *Listener;
};


#endif