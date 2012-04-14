// lr: v2 wrapper/host interfeces (zwischenscheisse)
#pragma once

#include <cassert>

namespace V2
{

	// functions provided by v2 client (wrappers)
	struct IClient
	{
		virtual void CurrentParameterChanged(int parameterIndex) = 0;

		// the gui has changed the current channel
		virtual void CurrentChannelChanged() {}

		// the gui has changed the current patch
		virtual void CurrentPatchChanged() {}

		// return a local channel name if available
		virtual const char* GetChannelName(int channelIndex) { return NULL; }
	};

	struct IGUI
	{
		// return a pointer to gui settings (not deleted by client)
		virtual void* SaveSettings(int& size, int& version) = 0;

		// load gui settings from pointer
		virtual void LoadSettings(void* data, int size, int version) = 0;
	};

	struct ISynth
	{
		// returns the currently selected channel
		virtual int GetCurrentChannelIndex() = 0;

		// returns the currently selected patch
		virtual int GetCurrentPatchIndex() = 0;
	};

	// functions provided by v2 host (player and gui)
	struct IHost
	{
		// must be called by client immediately after pointer to IHost is obtained
		virtual void SetClient(IClient& client) = 0;

		// called by client before unload, no further calls to IHost may be made
		virtual void Release() = 0;

		// returns the GUI interface
		virtual IGUI* GetGUI() = 0;

		// returns the Synth interface
		virtual ISynth* GetSynth() = 0;

		// updates current program for synth and for GUI
		virtual void SetCurrentPatchIndex(int index) = 0;
	};

	// function is not a factory but returns
	// an existing instance. 
	typedef IHost& (__cdecl *GetHostFunc)();


	/// templates

	template<typename BaseType>
	struct InteractsWithIClientImpl : BaseType
	{
		IClient* m_client;

		// local functions

		IClient* GetClient()
		{
			assert(m_client);
			return m_client;
		}

		void SetClient(IClient& client)
		{
			m_client = &client;
			OnSetClient(client);
		}

		virtual void OnSetClient(IClient& client)
		{
		}
	};

	struct NoBase
	{
	};

	struct InteractsWithIClient : InteractsWithIClientImpl<NoBase> {};

} // namespace V2