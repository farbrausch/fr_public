/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "altona2/libs/util/samplemixer.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

MainWindow::MainWindow()
{ 
    AddKey("Exit",sKEY_Escape|sKEYQ_Shift,sGuiMsg(this,&sWindow::CmdExit));
    AddHelp();

    // grid

    sGridFrame *g = new sGridFrame(); g->AddBorder(new sFocusBorder); g->AddScrolling(0,1);
    AddChild(g);

    sGridFrameHelper gh(g);
    gh.Label("Buttons");
    gh.UpdatePool = 0;
    gh.Button(sGuiMsg(this,&MainWindow::CmdPlay,0),"A");
    gh.Button(sGuiMsg(this,&MainWindow::CmdPlay,1),"B");
    gh.Button(sGuiMsg(this,&MainWindow::CmdPlay,2),"C");
    gh.Button(sGuiMsg(this,&MainWindow::CmdPlay,3),"D");

    InitSound();
}

MainWindow::~MainWindow()
{
    ExitSound();
}

/****************************************************************************/

void MainWindow::InitSound()
{
    Mixer = new sSampleMixer(sAOF_LatencyLow);

    const sChar *samplefiles[SampleCount] =
    {
        "C:/Windows/Media/tada.wav",
        "C:/Windows/Media/chimes.wav",
        "C:/Windows/Media/chord.wav",
    };

    for(sInt i=0;i<SampleCount;i++)
    {
        if(samplefiles[i])
        {
            Samples[i] = new sSampleMixerMemorySample(samplefiles[i]);
            //      sDPrintF("sample %d Hz %d channels %d samples\n",Samples[i]->Frequency,Samples[i]->Channels,Samples[i]->SampleCount);
        }
        else
        {
            Samples[i] = 0;
        }
    }
}

void MainWindow::ExitSound()
{
    for(sInt i=0;i<SampleCount;i++)
        Samples[i]->Release();

    delete Mixer;
}

void MainWindow::CmdPlay(sInt n)
{
    if(Samples[n])
        Mixer->PlaySample(Samples[n],0,1.0)->Release();
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class AppInit : public sGuiInitializer
{
public:
    sWindow *CreateRoot() { return new MainWindow; }
    void GetTitle(const sStringDesc &desc) { sSPrintF(desc,"altona2 gui showcase"); }
};

void Altona2::Main()
{
    sRunGui(new AppInit);
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

