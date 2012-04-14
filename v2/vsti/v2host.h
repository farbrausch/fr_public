// lr: v2 host interface implementation and global object
#pragma once

#include "v2interfaces.h"
#include <cassert>
#include "v2api.h"
#include "v2appearance.h"
#include "v2view.h"

namespace V2
{
	__declspec(selectany) GUI::Frame* g_BuzzEditor = NULL;

	struct Host : InteractsWithIClientImpl<IHost>
	{	
		virtual void OnSetClient(IClient& client)
		{
			GUI::GetTheAppearance()->SetClient(client);
			GetTheSynth()->SetClient(client);
		}

		virtual void Release()
		{			
			m_client = NULL;
		}

		// returns the GUI interface
		virtual IGUI* GetGUI()
		{
			return GUI::GetTheAppearance();
		}

		// returns the Synth interface
		virtual ISynth* GetSynth()
		{
			return GetTheSynth();
		}

		// updates current program for synth and for GUI
		virtual void SetCurrentPatchIndex(int index)
		{
			if (g_BuzzEditor)
			{				
				g_BuzzEditor->SetProgram(index,true);
			}
		}
	};

	__declspec(selectany) Host g_theHost;

} // namespace V2