/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "SampleMixer.hpp"

#define STB_VORBIS_HEADER_ONLY
#include "Altona2/Libs/Util/StbVorbis.c"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   sSampleMixerSample                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sSampleMixerSample::LoadSample(const char *filename)
{
    if(LoadSampleMayFail(filename))
        return;
    sFatal("sSampleMixerSample::LoadSample(): failed");
}

/****************************************************************************/
/***                                                                      ***/
/***   sSampleMixerMemorySample                                           ***/
/***                                                                      ***/
/****************************************************************************/

sSampleMixerMemorySample::sSampleMixerMemorySample()
{
    SampleCount = 0;
    Channels = 1;
    Data = 0;
    Frequency = 48000;
}

sSampleMixerMemorySample::sSampleMixerMemorySample(const char *filename)
{
    SampleCount = 0;
    Channels = 1;
    Data = 0;
    Frequency = 48000;
    LoadSample(filename);
}

sSampleMixerMemorySample::~sSampleMixerMemorySample()
{
    delete[] Data;
}

static uint Make4(const char *x)
{
    return ((x[0]&0xff)<< 0) |
        ((x[1]&0xff)<< 8) |
        ((x[2]&0xff)<<16) |
        ((x[3]&0xff)<<24);
}

bool sSampleMixerMemorySample::LoadSampleMayFail(const char *filename)
{
    const char *ext = sExtractLastSuffix(filename);

    bool ok = 0;
    uptr bytesize = 0;
    uint8 *mem = sLoadFile(filename,bytesize);
    if(mem)
    {
        if(sCmpStringI(ext,"wav")==0)
            ok = LoadWav(mem,bytesize);
        else if(sCmpStringI(ext,"ogg")==0)
            ok = LoadOgg(mem,bytesize);
        SampleCount32 = uint64(SampleCount)<<32;
        delete[] mem;
    }
    return ok;
}

void sSampleMixerMemorySample::Render(float *data,int samples,sSampleMixerVoice *v)
{
    float amp = v->Volume / 0x8000;
    if(v->Inactive)
        return;

    uint64 l0 = 0;
    uint64 l1 = SampleCount32;
    if(v->Loop)
    {
        l0 = sClamp<uint64>(v->Loop0,0,SampleCount32);
        l1 = sClamp<uint64>(v->Loop1,0,SampleCount32);
        if(l0>=l1)
        {
            l0 = 0;
            l1 = SampleCount32;
        }
    }

    for(int i=0;i<samples;i++)
    {
        v->FracPos += v->Rate;
        while(v->FracPos >= l1)
        {
            v->FracPos -= l1-l0;
            if(!v->Loop)
            {
                v->Inactive = 1;
                return;
            }
        }
        uint p = v->FracPos>>32;
        if(Channels==2)
        {
            data[i*2+0] += Data[p*2+0]*amp;
            data[i*2+1] += Data[p*2+1]*amp;
        }
        else
        {
            float d = Data[p]*amp;
            data[i*2+0] += d;
            data[i*2+1] += d;
        }
    }
}

/****************************************************************************/

bool sSampleMixerMemorySample::LoadWav(uint8 *mem,uptr bytesize)
{
    uint *data = (uint *) mem;
    //  uint *end = (uint *)(mem+bytesize);

    // header

    if(data[0] != Make4("RIFF")) goto error;
    if(data[1] != bytesize-8) goto error;
    if(data[2] != Make4("WAVE")) goto error;
    data += 3;

    // format
    {
        int format = data[2]&0xffff;
        Channels = data[2]>>16;
        Frequency = data[3];
        int bitspersample = data[5]>>16;
        int blockalign = data[5]&0xffff;

        if(data[0] != Make4("fmt ")) goto error;
        if(data[1] != 0x10) goto error;
        if(format != 1) goto error;
        if(Channels<1 || Channels>2) goto error;
        if(Frequency<1 || Frequency>1000*1000) goto error;
        if(bitspersample != 16) goto error;
        if(blockalign != Channels*2) goto error;

        data += 6;
    }
    // data

    if(data[0] != Make4("data")) goto error;
    SampleCount = data[1]/(Channels*2);
    if(SampleCount<1 || SampleCount>0x10000000) goto error;

    Data = new int16[SampleCount*Channels];
    sCopyMem(Data,data+2,SampleCount*Channels*2);

    return 1;
error:
    return 0;
}

bool sSampleMixerMemorySample::LoadOgg(uint8 *mem,uptr bytesize)
{
    bool ok = 0;
    //  stb_vorbis_alloc memhandler;
    //  memhandler.alloc_buffer_length_in_bytes = 512*1024;
    //  memhandler.alloc_buffer = (char *) new uint8[memhandler.alloc_buffer_length_in_bytes];
    stb_vorbis *handle = stb_vorbis_open_memory(mem,int(bytesize),0,0);
    stb_vorbis_info info;
    if(handle)
    {
        info = stb_vorbis_get_info(handle);
        Channels = 2;
        if(info.channels==2 || info.channels==1)
        {
            Frequency = info.sample_rate;
            SampleCount = stb_vorbis_stream_length_in_samples(handle);
            sASSERT(SampleCount>0);
            Data = new int16[SampleCount*Channels];

            int pos = 0;
            while(pos<SampleCount)
            {
                int n = stb_vorbis_get_frame_short_interleaved(handle,2,Data+pos*2,(SampleCount-pos)*2);
                pos += n;
                if(n==0)
                    break;
            }
            sASSERT(pos==SampleCount);
            ok = 1;
        }
        stb_vorbis_close(handle);
    }

    //  sDelete(memhandler.alloc_buffer);
    return ok;
}

/****************************************************************************/
/***                                                                      ***/
/***   sSampleMixerOgg                                                    ***/
/***                                                                      ***/
/****************************************************************************/

void sSampleMixerOgg::Construct()
{
    Handle = 0;
    Channels = 0;
    Frequency = 0;

    FileBytes = 0;
    FileData = 0;
    FrameData[0] = 0;
    FrameData[1] = 0;
    FrameStart = 0;

    FrameStart = 0;
    FrameEnd = 0;
    ReadSample = 0;
}

void sSampleMixerOgg::Destruct()
{
    if(Handle)
        stb_vorbis_close(Handle);
    Handle = 0;
    Channels = 0;
    Frequency = 0;

    FileBytes = 0;
    sDelete(FileData);
}

sSampleMixerOgg::sSampleMixerOgg()
{
    Construct();
}

sSampleMixerOgg::~sSampleMixerOgg()
{
    Destruct();
}

sSampleMixerOgg::sSampleMixerOgg(const char *filename)
{
    Construct();
    LoadSample(filename);
}

bool sSampleMixerOgg::LoadSampleMayFail(const char *filename)
{
    stb_vorbis_info info;

    FileData = sLoadFile(filename,FileBytes);
    if(!FileData)
        goto error;

    Handle = stb_vorbis_open_memory(FileData,int(FileBytes),0,0);
    if(!Handle)
        goto error;
    info = stb_vorbis_get_info(Handle);

    Channels = info.channels;
    if(Channels!=2)
        goto error;

    Frequency = info.sample_rate;
    FrameStart = 0;
    FrameEnd = 0;
    ReadSample = 0;
    Pos = 0;

    return 1;
error:
    Destruct();
    return 0;
}

void sSampleMixerOgg::Render(float *data,int samples,sSampleMixerVoice *v)
{
    if(v->Inactive)
        return;

    float amp = v->Volume;

    for(int i=0;i<samples;i++)
    {
        v->FracPos += v->Rate;
loadfirstblock:
        uint p = v->FracPos>>32;

        while(p >= FrameEnd)
        {
            FrameStart = FrameEnd;
            float **data;
skipsilence:
            int n = stb_vorbis_get_frame_float(Handle,0,&data);
            if(SkipSilence)
            {
                bool ok = 1;
                for(int i=0;i<n;i++)
                {
                    if(sAbs(data[0][i])>0.001f || sAbs(data[1][i])>0.001f) 
                    { 
                        ok = 0;
                        break;
                    }
                }
                if(ok)
                    goto skipsilence;
                else
                    SkipSilence = 0;
            }

            FrameData[0] = data[0];
            FrameData[1] = data[1];
            FrameEnd = FrameStart + n;
            if(n==0)
            {
                v->FracPos = 0;
                p = 0;
            }
        }
        if(p < FrameStart)
        {
            FrameStart = FrameEnd = 0;
            stb_vorbis_seek_start(Handle);
            goto loadfirstblock;
        }

        sASSERT(p>=FrameStart && p<FrameEnd);
        data[i*2+0] += FrameData[0][p-FrameStart]*amp;
        data[i*2+1] += FrameData[1][p-FrameStart]*amp;
    }
}


void sSampleMixerOgg::Render(float *data,int samples)
{
    float amp = 1.0f;

    for(int i=0;i<samples;i++)
    {
loadfirstblock:
        uint p = Pos;
        Pos++;

        while(p >= FrameEnd)
        {
            FrameStart = FrameEnd;
            float **data;
skipsilence:
            int n = stb_vorbis_get_frame_float(Handle,0,&data);
            if(SkipSilence)
            {
                bool ok = 1;
                for(int i=0;i<n;i++)
                {
                    if(sAbs(data[0][i])>0.001f || sAbs(data[1][i])>0.001f) 
                    { 
                        ok = 0;
                        break;
                    }
                }
                if(ok)
                    goto skipsilence;
                else
                    SkipSilence = 0;
            }

            if(n==0)
            {
                Pos = 0;
                p = 0;
            }
            else
            {
                FrameData[0] = data[0];
                FrameData[1] = data[1];
                FrameEnd = FrameStart + n;
            }
        }
        if(p < FrameStart)
        {
            FrameStart = FrameEnd = 0;
            stb_vorbis_seek_start(Handle);
            goto loadfirstblock;
        }

        sASSERT(p>=FrameStart && p<FrameEnd);
        data[i*2+0] += FrameData[0][p-FrameStart]*amp;
        data[i*2+1] += FrameData[1][p-FrameStart]*amp;
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   sSampleMixerVoice                                                  ***/
/***                                                                      ***/
/****************************************************************************/

sSampleMixerVoice::sSampleMixerVoice()
{
    Loop = 0;
    Inactive = 0;
    Sample = 0;
    Rate = 0;
    FracPos = 0;
    Loop0 = 0;
    Loop1 = 0;
}

sSampleMixerVoice::~sSampleMixerVoice()
{
    sRelease(Sample);
}

void sSampleMixerVoice::SetSample(uint sample)
{
    FracPos = uint64(sample)<<32;
}

uint sSampleMixerVoice::GetSample()
{
    return (FracPos>>32) - sGetSamplesQueuedOut();
}

void sSampleMixerVoice::SetLoop(uint loop0,uint loop1)
{
    Loop0 = uint64(loop0)<<32;
    Loop1 = uint64(loop1)<<32;
}

void sSampleMixerVoice::EnableLoop(bool loop)
{
    Loop = loop;
}

void sSampleMixerVoice::SetVolume(float vol)
{
    Volume = vol;
}

/****************************************************************************/
/***                                                                      ***/
/***   sSampleMixer                                                       ***/
/***                                                                      ***/
/****************************************************************************/

sSampleMixer::sSampleMixer(int audioflags)
{
    Frequency = sStartAudioOut(48000,sDelegate2<void,float *,int>(this,&sSampleMixer::SampleCallback),audioflags);
    if(!Frequency)
        Frequency = sStartAudioOut(44100,sDelegate2<void,float *,int>(this,&sSampleMixer::SampleCallback),audioflags);
    if(!Frequency)
        sFatal("could not start audio");
    sLogF("util","audio at %d Hz",Frequency);

	AudioLock = new sThreadLock();
	OwnDevice = true;
}

sSampleMixer::sSampleMixer(sThreadLock *audiolock, int frequency)
{
	AudioLock = audiolock;
	Frequency = frequency;
	OwnDevice = false;
}

sSampleMixer::~sSampleMixer()
{
	if (OwnDevice)
		sStopAudioOut();

    Lock();
    Voices.ReleaseAll();
    Unlock();

	if (OwnDevice)
		delete AudioLock;
}

void sSampleMixer::Lock()
{
    AudioLock->Lock();
}

void sSampleMixer::Unlock()
{
    AudioLock->Unlock();
}

sSampleMixerVoice *sSampleMixer::PlaySample(sSampleMixerSample *sample,bool loop,float rate,float volume)
{
    sSampleMixerVoice *v = new sSampleMixerVoice;
    v->Sample = sample;
    v->Rate = uint64(sample->Frequency*rate/Frequency*0x10000*0x10000);
    v->Loop = loop;
    v->Volume = volume;
    v->Sample->AddRef();

    Lock();
    Voices.Add(v);
    Unlock();
    v->AddRef();
    return v;
}

void sSampleMixer::StopVoice(sSampleMixerVoice *v)
{
    if(v)
    {
        Lock();
        Voices.Rem(v);
        Unlock();
        v->Release();
    }
}

void sSampleMixer::SampleCallback(float *data,int samples)
{
    Lock();
	if (OwnDevice)
    for(int i=0;i<samples;i++)
    {
        data[i*2+0] = 0;
        data[i*2+1] = 0;
    }
    for(auto v : Voices)
    {
        if(!v->Inactive)
            v->Sample->Render(data,samples,v);
    }

    Voices.ReleaseIfOrder([](sSampleMixerVoice *v){ return v->Inactive; });
    Unlock();
}

/****************************************************************************/
/****************************************************************************/

int16 *sLoadOgg(const char *filename,int channels,int &samplerate,int &samplesloaded)
{
    uptr bytesize;
    int16 *Data = 0;

    uint8 *mem = sLoadFile(filename,bytesize);
    if(mem)
    {
        stb_vorbis *handle = stb_vorbis_open_memory(mem,int(bytesize),0,0);
        stb_vorbis_info info;

        if(handle)
        {
            info = stb_vorbis_get_info(handle);
            if(info.channels==2 || info.channels==1)
            {
                samplerate = info.sample_rate;
                samplesloaded = stb_vorbis_stream_length_in_samples(handle);
                sASSERT(samplesloaded>0);
                Data = new int16[samplesloaded*channels];//*samplerate];

                int pos = 0;
                while(pos<samplesloaded)
                {
                    int n = stb_vorbis_get_frame_short_interleaved(handle,2,Data+pos*2,(samplesloaded-pos)*2);
                    pos += n;
                    if(n==0)
                        break;
                }
                sASSERT(pos==samplesloaded);
            }
            stb_vorbis_close(handle);
        }
        delete[] mem;
    }

    return Data;
}


int16 *sLoadWav(const char *filename,int channels,int &samplerate,int &samplesloaded)
{
    uptr bytesize;
    int16 *Data = 0;

    uint *mem = (uint *) sLoadFile(filename,bytesize);
    uint *del = mem;
    if(mem)
    {
        // header

        if(mem[0] != Make4("RIFF")) goto error;
        if(mem[1] != bytesize-8) goto error;
        if(mem[2] != Make4("WAVE")) goto error;
        mem += 3;

        // format

        int format = mem[2]&0xffff;
        int Channels = mem[2]>>16;
        samplerate = mem[3];
        int bitspersample = mem[5]>>16;
        int blockalign = mem[5]&0xffff;

        if(mem[0] != Make4("fmt ")) goto error;
        if(mem[1] != 0x10) goto error;
        if(format != 1) goto error;
        if(Channels<1 || Channels>2) goto error;
        if(samplerate<1 || samplerate>1000*1000) goto error;
        if(bitspersample != 16) goto error;
        if(blockalign != Channels*2) goto error;

        mem += 6;

        // data

        if(mem[0] != Make4("data")) goto error;
        samplesloaded = mem[1]/(Channels*sizeof(int16));
        if(samplesloaded<1 || samplesloaded>0x10000000) goto error;

        mem+=2;
        Data = new int16[samplesloaded*2];

        if(Channels==2)
        {
            sCopyMem(Data,mem,samplesloaded*Channels*2);
        }
        else
        {
            for(int i=0;i<samplesloaded;i++)
                Data[i*2+1] = Data[i*2+0] = ((int16 *)mem)[i];
        }
    }
error:
    delete[] del;
    return Data;
}

/****************************************************************************/
}



