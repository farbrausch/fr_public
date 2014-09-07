/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "Altona2/Libs/Util/GraphicsHelper.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
    sRunApp(new App,sScreenMode(sSM_FullWindowed*0,"Audio",1280,720));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
    DPaint = 0;
}

App::~App()
{
}

void App::OnInit()
{
    Screen = sGetScreen();
    Adapter = Screen->Adapter;
    Context = Adapter->ImmediateContext;

    DPaint = new sDebugPainter(Adapter);

    Sample = 0;
    SampleRate = sStartAudioOut(44100,sDelegate2<void,float *,int>(this,&App::AudioCallback),sAOF_LatencyLow);
}

void App::OnExit()
{
    sStopAudioOut();
    delete DPaint;
}

void App::AudioCallback(float *data,int samples)
{
    for(int i=0;i<samples;i++)
    {
        *data++ = sSin(float(Sample)*439.9f/float(SampleRate)*s2Pi);
        *data++ = sSin(float(Sample)*440.1f/float(SampleRate)*s2Pi);
        Sample++;
    }
}


void App::OnFrame()
{
}

void App::OnPaint()
{
    sTargetPara tp(sTAR_ClearAll,0xff405060,Screen);
    Context->BeginTarget(tp);

    DPaint->PrintFPS();
    DPaint->PrintStats();
    DPaint->Draw(tp);

    Context->EndTarget();
}

void App::OnKey(const sKeyData &kd)
{
    if((kd.Key&(sKEYQ_Mask|sKEYQ_Break))==27)
        sExit();
}

void App::OnDrag(const sDragData &dd)
{
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

