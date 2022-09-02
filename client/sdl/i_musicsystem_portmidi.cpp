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
static int I_PortMidiTime(void* time_info = NULL)
{
	return I_MSTime();
}

PortMidiMusicSystem::PortMidiMusicSystem()
    : MidiMusicSystem(), m_isInitialized(false), m_outputDevice(-1), m_stream(NULL)
{
	const int output_buffer_size = 1024;

	if (Pm_Initialize() != pmNoError)
	{
		Printf(PRINT_WARNING, "I_InitMusic: PortMidi initialization failed.\n");
		return;
	}

	m_outputDevice = Pm_GetDefaultOutputDeviceID();
	std::string prefdevicename(snd_musicdevice.cstring());

	// List PortMidi devices
	for (int i = 0; i < Pm_CountDevices(); i++)
	{
		const PmDeviceInfo* info = Pm_GetDeviceInfo(i);
		if (!info || !info->output)
			continue;

		std::string curdevicename(info->name);
		if (!prefdevicename.empty() && iequals(prefdevicename, curdevicename))
			m_outputDevice = i;

		Printf(PRINT_HIGH, "%d: %s, %s\n", i, info->interf, info->name);
	}

	if (m_outputDevice == pmNoDevice)
	{
		Printf(PRINT_WARNING, "I_InitMusic: No PortMidi output devices available.\n");
		Pm_Terminate();
		return;
	}

	if (Pm_OpenOutput(&m_stream, m_outputDevice, NULL, output_buffer_size, I_PortMidiTime,
	                  NULL, cLatency) != pmNoError)
	{
		Printf(PRINT_WARNING, "I_InitMusic: Failure opening PortMidi output device %d.\n",
		       m_outputDevice);
		return;
	}

	if (!m_stream)
		return;

	Printf(PRINT_HIGH, "I_InitMusic: Music playback enabled using PortMidi.\n");
	m_isInitialized = true;
}

PortMidiMusicSystem::~PortMidiMusicSystem()
{
	if (!isInitialized())
		return;

	_StopSong();
	m_isInitialized = false;

	if (m_stream)
	{
		// Sleep to allow the All-Notes-Off events to be processed
		I_Sleep(I_ConvertTimeFromMs(cLatency * 2));

		Pm_Close(m_stream);
		Pm_Terminate();
		m_stream = NULL;
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

		// pitch bend range (+/- 2 semitones)
		MidiControllerEvent event_rpn_msb_start(0, MIDI_CONTROLLER_RPN_MSB, i);
		_PlayEvent(&event_rpn_msb_start);
		MidiControllerEvent event_rpn_lsb_start(0, MIDI_CONTROLLER_RPN_LSB, i);
		_PlayEvent(&event_rpn_lsb_start);
		MidiControllerEvent event_data_msb(0, MIDI_CONTROLLER_DATA_ENTRY_MSB, i, 2);
		_PlayEvent(&event_data_msb);
		MidiControllerEvent event_data_lsb(0, MIDI_CONTROLLER_DATA_ENTRY_LSB, i);
		_PlayEvent(&event_data_lsb);
		MidiControllerEvent event_rpn_lsb_end(0, MIDI_CONTROLLER_RPN_LSB, i, 127);
		_PlayEvent(&event_rpn_lsb_end);
		MidiControllerEvent event_rpn_msb_end(0, MIDI_CONTROLLER_RPN_MSB, i, 127);
		_PlayEvent(&event_rpn_msb_end);

		// channel volume and panning
		MidiControllerEvent event_volume(0, MIDI_CONTROLLER_MAIN_VOLUME, i, 100);
		_PlayEvent(&event_volume);
		MidiControllerEvent event_pan(0, MIDI_CONTROLLER_PAN, i, 64);
		_PlayEvent(&event_pan);

		// effect controllers
		MidiControllerEvent event_reverb(0, MIDI_CONTROLLER_REVERB, i, 40);
		_PlayEvent(&event_reverb);
		MidiControllerEvent event_tremolo(0, MIDI_CONTROLLER_TREMOLO, i);
		_PlayEvent(&event_tremolo);
		MidiControllerEvent event_chorus(0, MIDI_CONTROLLER_CHORUS, i);
		_PlayEvent(&event_chorus);
		MidiControllerEvent event_detune(0, MIDI_CONTROLLER_DETUNE, i);
		_PlayEvent(&event_detune);
		MidiControllerEvent event_phaser(0, MIDI_CONTROLLER_PHASER, i);
		_PlayEvent(&event_phaser);
	}
}

//
// PortMidiMusicSystem::playEvent
//
// Virtual wrapper-function for the non-virtual _PlayEvent.  We provide the
// non-virtual version so that it can be safely called by ctors and dtors.
//
void PortMidiMusicSystem::playEvent(MidiEvent* event, int time)
{
	if (event)
		_PlayEvent(event, time);
}

void PortMidiMusicSystem::_PlayEvent(MidiEvent* event, int time)
{
	if (!event)
		return;

	// play at the current time if user specifies time 0
	if (time == 0)
		time = _GetLastEventTime();

	if (I_IsMidiMetaEvent(event))
	{
		MidiMetaEvent* metaevent = static_cast<MidiMetaEvent*>(event);
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
		MidiControllerEvent* ctrlevent = static_cast<MidiControllerEvent*>(event);
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
		Pm_WriteShort(m_stream, time, msg);
	}
	else if (I_IsMidiChannelEvent(event))
	{
		MidiChannelEvent* chanevent = static_cast<MidiChannelEvent*>(event);
		byte channel = chanevent->getChannel();
		byte param1 = chanevent->getParam1();
		byte param2 = chanevent->getParam2();

		PmMessage msg = Pm_Message(event->getEventType() | channel, param1, param2);
		Pm_WriteShort(m_stream, time, msg);
	}
}

#endif // PORTMIDI
