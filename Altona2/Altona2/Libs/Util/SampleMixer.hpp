/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_SAMPLEMIXER_HPP
#define FILE_ALTONA2_LIBS_UTIL_SAMPLEMIXER_HPP

#include "Altona2/Libs/Base/Base.hpp"

struct stb_vorbis;

namespace Altona2 {

class sSampleMixerVoice;
class sSampleMixerSample;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sSampleMixerSample : public sRCObject
{
public:
    void LoadSample(const char *filename);
    virtual void Render(float *data,int samples,sSampleMixerVoice *v) = 0;
    virtual bool LoadSampleMayFail(const char *filename) = 0;
    int Frequency;
};

class sSampleMixerMemorySample : public sSampleMixerSample
{
    bool LoadWav(uint8 *mem,uptr bytesize);
    bool LoadOgg(uint8 *mem,uptr bytesize); // load entire file!

    int SampleCount;
    int Channels;
    int16 *Data;
    uint64 SampleCount32;
   
public:

    sSampleMixerMemorySample();
    ~sSampleMixerMemorySample();
    sSampleMixerMemorySample(const char *filename);
    bool LoadSampleMayFail(const char *filename);
    void Render(float *data,int samples,sSampleMixerVoice *v);
};

class sSampleMixerOgg : public sSampleMixerSample
{
    struct stb_vorbis *Handle;
    uint8 *FileData;
    uptr FileBytes;
    float *FrameData[2];
    uint FrameStart;
    uint FrameEnd;
    uint ReadSample;
    int Pos;

    int Channels;
    void Construct();
    void Destruct();

public:
    ~sSampleMixerOgg();
    sSampleMixerOgg();
    sSampleMixerOgg(const char *filename);
    bool LoadSampleMayFail(const char *filename);
    void Render(float *data,int samples,sSampleMixerVoice *v);
    void Render(float *data,int samples);
    bool SkipSilence;
};


/****************************************************************************/

class sSampleMixerVoice : public sRCObject
{
    friend class sSampleMixerMemorySample;
    friend class sSampleMixerOgg;
    friend class sSampleMixer;

    bool Loop;
    bool Inactive;
    uint64 Rate;
    uint64 FracPos;
    uint64 Loop0;
    uint64 Loop1;
    float Volume;
public:
    sSampleMixerVoice();
    ~sSampleMixerVoice();

    sSampleMixerSample *Sample;

    void SetSample(uint sample);
    void SetVolume(float volume);
    uint GetSample();
    void SetLoop(uint loop0,uint loop1);
    void EnableLoop(bool loop);
};

/****************************************************************************/

class sSampleMixer
{
    friend class sSampleMixerSample;
    friend class sSampleMixerVoice;

    sArray<sSampleMixerVoice *> Voices;    
    int Frequency;
    sThreadLock *AudioLock;
	bool OwnDevice;
public:
    sSampleMixer(int audioflags);
	sSampleMixer(sThreadLock *audiolock, int frequency);
    
	~sSampleMixer();

    void Lock();
    void Unlock();

    sSampleMixerVoice *PlaySample(sSampleMixerSample *sample,bool loop,float rate=1.0f,float volume=1.0f);
    void StopVoice(sSampleMixerVoice *);
    int GetFrequency() { return Frequency; }
	void SampleCallback(float *data,int samples);
};

/****************************************************************************/

int16 *sLoadOgg(const char *filename,int channels,int &samplerate,int &samplesloaded);
int16 *sLoadWav(const char *filename,int channels,int &samplerate,int &samplesloaded);

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_UTIL_SAMPLEMIXER_HPP
