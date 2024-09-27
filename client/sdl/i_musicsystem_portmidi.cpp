// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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

#include <math.h>

#include "i_musicsystem_portmidi.h"

#include "portmidi.h"

#include "i_midi.h"
#include "i_system.h"

EXTERN_CVAR(snd_musicdevice)
EXTERN_CVAR(snd_midireset)
EXTERN_CVAR(snd_mididelay)
EXTERN_CVAR(snd_midisysex)

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
	: MidiMusicSystem(), sysex_buffer(), m_channelVolume(), m_volumeScale(0.0f),
	  m_isInitialized(false), m_isPlaying(false), m_outputDevice(-1), m_stream(NULL)
{
	const int output_buffer_size = 1024;
	const PmDeviceInfo *info;

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
		info = Pm_GetDeviceInfo(i);
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

	// Initialize tracked channel volumes
	for (int i = 0; i < NUM_CHANNELS; i++)
		m_channelVolume[i] = DEFAULT_VOLUME;

	// Initialize SysEx buffer
	memset(sysex_buffer, 0, sizeof(sysex_buffer));
	sysex_buffer[0] = MIDI_EVENT_SYSEX;

	// Initialize instrument fallback support
	_InitFallback();

	Printf(PRINT_HIGH, "I_InitMusic: Music playback enabled using PortMidi.\n");
	m_isInitialized = true;
}

PortMidiMusicSystem::~PortMidiMusicSystem()
{
	if (!isInitialized())
		return;

	m_isInitialized = false;
	m_isPlaying = false;

	if (m_stream)
	{
		_ResetDevice(false);

		// Sleep to prevent PortMidi callback deadlock
		I_Sleep(I_ConvertTimeFromMs(cLatency * 2));

		Pm_Close(m_stream);
		Pm_Terminate();
		m_stream = NULL;
	}
}

void PortMidiMusicSystem::writeControl(int time, byte channel, byte control, byte value)
{
	PmMessage msg;

	// MS GS Wavetable Synth resets volume if "reset all controllers" value isn't zero.
	if (control == MIDI_CONTROLLER_RESET_ALL_CTRLS)
		msg = Pm_Message(MIDI_EVENT_CONTROLLER | channel, control, 0);
	else
		msg = Pm_Message(MIDI_EVENT_CONTROLLER | channel, control, value);

	Pm_WriteShort(m_stream, time, msg);
}

void PortMidiMusicSystem::writeChannel(int time, byte channel, byte status, byte param1, byte param2)
{
	PmMessage msg = Pm_Message(status | channel, param1, param2);
	Pm_WriteShort(m_stream, time, msg);
}

void PortMidiMusicSystem::writeSysEx(int time, const byte *data, size_t length)
{
	if (!snd_midisysex)
		return;

	// Ignore messages that are too long
	if (length > PM_DEFAULT_SYSEX_BUFFER_SIZE - 1)
		return;

	// Copy data to buffer due to PortMidi not using a const pointer
	memcpy(&sysex_buffer[1], data, length);
	Pm_WriteSysEx(m_stream, time, sysex_buffer);

	if (_IsSysExReset(data, length))
	{
		// SysEx reset also resets volume. Take the default channel volumes
		// and scale them by the user's volume slider
		for (int i = 0; i < NUM_CHANNELS; i++)
			writeVolume(0, i, DEFAULT_VOLUME);

		// Disable instrument fallback and give priority to MIDI file. Fallback
		// assumes GS (SC-55 level) and the MIDI file could be GM, GM2, XG, or
		// GS (SC-88 or higher). Preserve the composer's intent
		_DisableFallback();
	}
}

void PortMidiMusicSystem::writeVolume(int time, byte channel, byte volume)
{
	byte scaled = (byte)(volume * m_volumeScale + 0.5f) & 0x7F;
	writeControl(time, channel, MIDI_CONTROLLER_VOLUME_MSB, scaled);
	m_channelVolume[channel] = volume & 0x7F;
}

void PortMidiMusicSystem::setVolume(float volume)
{
	MidiMusicSystem::setVolume(volume);

	// Mimic the volume curve of midiOutSetVolume, as used by SDL_Mixer
	m_volumeScale = sqrtf(MusicSystem::getVolume());

	if (!isInitialized() || !isPlaying())
		return;

	for (int i = 0; i < NUM_CHANNELS; i++)
		writeVolume(0, i, m_channelVolume[i]);
}

void PortMidiMusicSystem::allNotesOff()
{
	for (int i = 0; i < NUM_CHANNELS; i++)
		writeControl(0, i, MIDI_CONTROLLER_ALL_NOTES_OFF, 0);
}

void PortMidiMusicSystem::allSoundOff()
{
	for (int i = 0; i < NUM_CHANNELS; i++)
		writeControl(0, i, MIDI_CONTROLLER_ALL_SOUND_OFF, 0);
}

void PortMidiMusicSystem::_ResetAllControllers()
{
	for (int i = 0; i < NUM_CHANNELS; i++)
	{
		writeControl(0, i, MIDI_CONTROLLER_RESET_ALL_CTRLS, 0);
	}
}

void PortMidiMusicSystem::_ResetCommonControllers()
{
	for (int i = 0; i < NUM_CHANNELS; i++)
	{
		// Reset commonly used controllers not covered by "Reset All Controllers"
		writeControl(0, i, MIDI_CONTROLLER_PAN, 64);
		writeControl(0, i, MIDI_CONTROLLER_REVERB, 40);
		writeControl(0, i, MIDI_CONTROLLER_CHORUS, 0);
		writeControl(0, i, MIDI_CONTROLLER_BANK_SELECT_MSB, 0);
		writeControl(0, i, MIDI_CONTROLLER_BANK_SELECT_LSB, 0);
		writeChannel(0, i, MIDI_EVENT_PROGRAM_CHANGE, 0);
	}
}

void PortMidiMusicSystem::_ResetPitchBendSensitivity()
{
	for (int i = 0; i < NUM_CHANNELS; i++)
	{
		// Set RPN MSB/LSB to pitch bend sensitivity
		writeControl(0, i, MIDI_CONTROLLER_RPN_MSB, 0);
		writeControl(0, i, MIDI_CONTROLLER_RPN_LSB, 0);

		// Reset pitch bend sensitivity to +/- 2 semitones and 0 cents
		writeControl(0, i, MIDI_CONTROLLER_DATA_ENTRY_MSB, 2);
		writeControl(0, i, MIDI_CONTROLLER_DATA_ENTRY_LSB, 0);

		// Set RPN MSB/LSB to null value after data entry
		writeControl(0, i, MIDI_CONTROLLER_RPN_MSB, 127);
		writeControl(0, i, MIDI_CONTROLLER_RPN_LSB, 127);
	}
}

void PortMidiMusicSystem::_ResetDevice(bool playing)
{
	int midireset = snd_midireset.asInt();

	allNotesOff();
	allSoundOff();

	switch (midireset)
	{
		case MIDI_RESET_NONE:
			_ResetAllControllers();
			_ResetCommonControllers();
			break;

		case MIDI_RESET_GM:
			Pm_WriteSysEx(m_stream, 0, gm_system_on);
			break;

		case MIDI_RESET_XG:
			Pm_WriteSysEx(m_stream, 0, xg_system_on);
			break;

		default:
			Pm_WriteSysEx(m_stream, 0, gs_reset);
			break;
	}

	_ResetPitchBendSensitivity();

	if (midireset == MIDI_RESET_GS)
		_EnableFallback();

	// Reset tracked channel volumes
	for (int i = 0; i < NUM_CHANNELS; i++)
		m_channelVolume[i] = DEFAULT_VOLUME;

	// Reset to default volume on shutdown if no SysEx reset selected
	if (!playing && midireset == MIDI_RESET_NONE)
	{
		m_volumeScale = 1.0f;
		for (int i = 0; i < NUM_CHANNELS; i++)
			writeVolume(0, i, DEFAULT_VOLUME);
	}

	m_useResetDelay = (snd_mididelay > 0);
}

void PortMidiMusicSystem::startSong(byte *data, size_t length, bool loop)
{
	m_isPlaying = false;
	_ResetDevice(true);
	MidiMusicSystem::startSong(data, length, loop);
	m_isPlaying = true;
}

void PortMidiMusicSystem::stopSong()
{
	if (m_isPlaying)
	{
		allNotesOff();
		allSoundOff();
		m_isPlaying = false;
	}
	MidiMusicSystem::stopSong();
}

void PortMidiMusicSystem::pauseSong()
{
	allNotesOff();
	allSoundOff();
	MidiMusicSystem::pauseSong();
}

void PortMidiMusicSystem::restartSong()
{
	allNotesOff();
	_ResetAllControllers();
	MidiMusicSystem::restartSong();
}

bool PortMidiMusicSystem::_IsSysExReset(const byte *data, size_t length)
{
	if (length < 5)
	{
		return false;
	}

	switch (data[0])
	{
		case 0x41: // Roland
			switch (data[2])
			{
				case 0x42: // GS
					switch (data[3])
					{
						case 0x12: // DT1
							if (length == 10 &&
								data[4] == 0x00 &&  // Address MSB
								data[5] == 0x00 &&  // Address
								data[6] == 0x7F &&  // Address LSB
							  ((data[7] == 0x00 &&  // Data     (MODE-1)
								data[8] == 0x01) || // Checksum (MODE-1)
							   (data[7] == 0x01 &&  // Data     (MODE-2)
								data[8] == 0x00)))  // Checksum (MODE-2)
							{
								// SC-88 System Mode Set
								// 41 <dev> 42 12 00 00 7F 00 01 F7 (MODE-1)
								// 41 <dev> 42 12 00 00 7F 01 00 F7 (MODE-2)
								return true;
							}
							else if (length == 10 &&
									 data[4] == 0x40 && // Address MSB
									 data[5] == 0x00 && // Address
									 data[6] == 0x7F && // Address LSB
									 data[7] == 0x00 && // Data (GS Reset)
									 data[8] == 0x41)   // Checksum
							{
								// GS Reset
								// 41 <dev> 42 12 40 00 7F 00 41 F7
								return true;
							}
							break;
					}
					break;
			}
			break;

		case 0x43: // Yamaha
			switch (data[2])
			{
				case 0x2B: // TG300
					if (length == 9 &&
						data[3] == 0x00 && // Start Address b20 - b14
						data[4] == 0x00 && // Start Address b13 - b7
						data[5] == 0x7F && // Start Address b6 - b0
						data[6] == 0x00 && // Data
						data[7] == 0x01)   // Checksum
					{
						// TG300 All Parameter Reset
						// 43 <dev> 2B 00 00 7F 00 01 F7
						return true;
					}
					break;

				case 0x4C: // XG
					if (length == 8 &&
						data[3] == 0x00 &&  // Address High
						data[4] == 0x00 &&  // Address Mid
					   (data[5] == 0x7E ||  // Address Low (System On)
						data[5] == 0x7F) && // Address Low (All Parameter Reset)
						data[6] == 0x00)    // Data
					{
						// XG System On, XG All Parameter Reset
						// 43 <dev> 4C 00 00 7E 00 F7
						// 43 <dev> 4C 00 00 7F 00 F7
						return true;
					}
					break;
			}
			break;

		case 0x7E: // Universal Non-Real Time
			switch (data[2])
			{
				case 0x09: // General Midi
					if (length == 5 &&
					   (data[3] == 0x01 || // GM System On
						data[3] == 0x02 || // GM System Off
						data[3] == 0x03))  // GM2 System On
					{
						// GM System On/Off, GM2 System On
						// 7E <dev> 09 01 F7
						// 7E <dev> 09 02 F7
						// 7E <dev> 09 03 F7
						return true;
					}
					break;
			}
			break;
	}
	return false;
}

#endif // PORTMIDI
