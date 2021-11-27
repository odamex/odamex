// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  Plays music utilizing the PortMidi music library.
//
//-----------------------------------------------------------------------------

#ifdef PORTMIDI

#include "odamex.h"

#include "i_musicsystem_portmidi.h"

#include "portmidi.h"

#include "i_midi.h"
#include "i_system.h"

EXTERN_CVAR(snd_musicdevice)

// ============================================================================
// Partially based on an implementation from prboom-plus by Nicholai Main (Natt).
// ============================================================================

//
// I_PortMidiTime()
//
// A wrapper function for I_MSTime() so that PortMidi can use a function
// pointer to I_MSTime() for its event scheduling needs.
//
static int I_PortMidiTime(void *time_info = NULL)
{
	return I_MSTime();
} 

PortMidiMusicSystem::PortMidiMusicSystem() :
	MidiMusicSystem(), mIsInitialized(false),
	mOutputDevice(-1), mStream(NULL)
{
	const int output_buffer_size = 1024;
	
	if (Pm_Initialize() != pmNoError)
	{
		Printf(PRINT_WARNING, "I_InitMusic: PortMidi initialization failed.\n");
		return;
	}

 	mOutputDevice = Pm_GetDefaultOutputDeviceID();
 	std::string prefdevicename(snd_musicdevice.cstring());
  	
	// List PortMidi devices
	for (int i = 0; i < Pm_CountDevices(); i++)
	{
		const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		if (!info || !info->output)
			continue;
			
		std::string curdevicename(info->name);
		if (!prefdevicename.empty() && iequals(prefdevicename, curdevicename))
			mOutputDevice = i;

		Printf(PRINT_HIGH, "%d: %s, %s\n", i, info->interf, info->name);
    }
    
    if (mOutputDevice == pmNoDevice)
	{
		Printf(PRINT_WARNING, "I_InitMusic: No PortMidi output devices available.\n");
		Pm_Terminate ();
		return;
	}
	
	if (Pm_OpenOutput(&mStream,	mOutputDevice, NULL, output_buffer_size, I_PortMidiTime, NULL, cLatency) != pmNoError)
	{
		Printf(PRINT_WARNING, "I_InitMusic: Failure opening PortMidi output device %d.\n", mOutputDevice);
		return;
	} 
                  
	if (!mStream)
		return;
		
	Printf(PRINT_HIGH, "I_InitMusic: Music playback enabled using PortMidi.\n");
	mIsInitialized = true;
}

PortMidiMusicSystem::~PortMidiMusicSystem()
{
	if (!isInitialized())
		return;
	
	_StopSong();
	mIsInitialized = false;
	
	if (mStream)
	{
		// Sleep to allow the All-Notes-Off events to be processed
		I_Sleep(I_ConvertTimeFromMs(cLatency * 2));
		
		Pm_Close(mStream);
		Pm_Terminate();
		mStream = NULL;
	}
}

void PortMidiMusicSystem::stopSong()
{
	_StopSong();
	MidiMusicSystem::stopSong();
}

void PortMidiMusicSystem::_StopSong()
{
	// non-virtual version of _AllNotesOff()
	for (int i = 0; i < _GetNumChannels(); i++)
	{
		MidiControllerEvent event_noteoff(0, MIDI_CONTROLLER_ALL_NOTES_OFF, i);
		_PlayEvent(&event_noteoff);
		MidiControllerEvent event_reset(0, MIDI_CONTROLLER_RESET_ALL, i);
		_PlayEvent(&event_reset);
	}
}

//
// PortMidiMusicSystem::playEvent
//
// Virtual wrapper-function for the non-virtual _PlayEvent.  We provide the
// non-virtual version so that it can be safely called by ctors and dtors.
//
void PortMidiMusicSystem::playEvent(MidiEvent *event, int time)
{
	if (event)
		_PlayEvent(event, time);
}

void PortMidiMusicSystem::_PlayEvent(MidiEvent *event, int time)
{
	if (!event)
		return;
		
	// play at the current time if user specifies time 0
	if (time == 0)
		time = _GetLastEventTime();
	
	if (I_IsMidiMetaEvent(event))
	{
		MidiMetaEvent *metaevent = static_cast<MidiMetaEvent*>(event);
		if (metaevent->getMetaType() == MIDI_META_SET_TEMPO)
		{
			double tempo = I_GetTempoChange(metaevent);
			setTempo(tempo);
		}
		
		//	Just ignore other meta events for now
	}
	else if (I_IsMidiSysexEvent(event))
	{
		// Just ignore sysex events for now
	}
	else if (I_IsMidiControllerEvent(event))
	{
		MidiControllerEvent *ctrlevent = static_cast<MidiControllerEvent*>(event);
		byte channel = ctrlevent->getChannel();
		byte controltype = ctrlevent->getControllerType();
		byte param1 = ctrlevent->getParam1();
		
		if (controltype == MIDI_CONTROLLER_MAIN_VOLUME)
		{
			// store the song's volume for the channel
			_SetChannelVolume(channel, param1);
			
			// scale the channel's volume by the master music volume
			param1 *= _GetScaledVolume();
		}
			
		PmMessage msg = Pm_Message(event->getEventType() | channel, controltype, param1);
		Pm_WriteShort(mStream, time, msg);
	}
	else if (I_IsMidiChannelEvent(event))
	{
		MidiChannelEvent *chanevent = static_cast<MidiChannelEvent*>(event);
		byte channel = chanevent->getChannel();
		byte param1 = chanevent->getParam1();
		byte param2 = chanevent->getParam2();
		
		PmMessage msg = Pm_Message(event->getEventType() | channel, param1, param2);
		Pm_WriteShort(mStream, time, msg);
	}
}

#endif	// PORTMIDI
