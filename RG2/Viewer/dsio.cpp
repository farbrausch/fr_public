// This code is in the public domain. See LICENSE for details.

#include "dsio.h"
#include "rtlib.h"
#include "types.h"
#include "windows.h"

#define NUMBLOCKS					16
#define SAMPLESPERBLOCK		2048

static DSIOCALLBACK*	g_callback;
static HWAVEOUT				g_hWaveOut;
static WAVEHDR				g_headers[NUMBLOCKS];
static sS16						g_buffers[NUMBLOCKS][SAMPLESPERBLOCK * 2];
static sF32						g_renderBuffer[SAMPLESPERBLOCK];
static sInt						g_bufferCounter;
static sBool					g_closing;
static sF32						g_volume;

static void convertAndClamp(sS16* dest, const sF32* src, sInt nSamples)
{
	sF32 vol = g_volume;

	while (nSamples--)
	{
		sInt value = *src++ * vol;
		*dest++ = (value < -32600) ? -32600 : (value > 32600) ? 32600 : value;
	}
}

static void __stdcall waveOutCallback(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance,
	DWORD dwParam1, DWORD dwParam2)
{
	if (uMsg == WOM_DONE && !g_closing) // block done
	{
		WAVEHDR* header = (WAVEHDR*) dwParam1;

		waveOutUnprepareHeader(hwo, header, sizeof(WAVEHDR));
		g_callback(g_renderBuffer, header->dwBufferLength / 4);
		header->lpData = (LPSTR) g_buffers[g_bufferCounter];
		g_bufferCounter = (g_bufferCounter + 1) & (NUMBLOCKS - 1);
		convertAndClamp((sS16*) header->lpData, g_renderBuffer, header->dwBufferLength / 4);
		waveOutPrepareHeader(hwo, header, sizeof(WAVEHDR));
		waveOutWrite(hwo, header, sizeof(WAVEHDR));
	}
}

int __stdcall dsInit(DSIOCALLBACK* callback, void* hWnd)
{
	static WAVEFORMATEX fmt = { WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0 };

	g_callback = callback;
	g_closing = sFALSE;
	g_bufferCounter = 0;
	waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &fmt, (DWORD_PTR) waveOutCallback, 0,
		CALLBACK_FUNCTION);

	memset(g_buffers, 0, sizeof(sS16) * NUMBLOCKS * SAMPLESPERBLOCK * 2);
	memset(g_headers, 0, sizeof(WAVEHDR) * NUMBLOCKS);
	dsSetVolume(1.0f);

	for (sInt i = 0; i < NUMBLOCKS; i++)
	{
		g_headers[i].lpData = (LPSTR) g_buffers[i];
		g_headers[i].dwBufferLength = sizeof(g_buffers[i]);
		waveOutPrepareHeader(g_hWaveOut, g_headers + i, sizeof(WAVEHDR));
		waveOutWrite(g_hWaveOut, g_headers + i, sizeof(WAVEHDR));
	}

	return 1;
}

void __stdcall dsClose()
{
	g_closing = sTRUE;
	waveOutReset(g_hWaveOut);
	waveOutClose(g_hWaveOut);
	g_hWaveOut = 0;
}

int __stdcall dsGetCurSmp()
{
	MMTIME time;

	time.wType = TIME_SAMPLES;
	waveOutGetPosition(g_hWaveOut, &time, sizeof(MMTIME));

	return time.u.sample;
}

void __stdcall dsSetVolume(sF32 vol)
{
	g_volume = vol * 32768.0f;
}

void __stdcall dsFadeOut(int time)
{
}

void __stdcall dsTick()
{
}
