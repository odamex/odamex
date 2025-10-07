// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2025 by The Odamex Team.
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

#pragma once

#ifdef PORTMIDI

#include "i_musicsystem.h"

#include "portmidi.h"

/**
 * @brief Plays music utilizing the PortMidi music library.
 */
class PortMidiMusicSystem : public MidiMusicSystem
{
  public:
	PortMidiMusicSystem();
	~PortMidiMusicSystem() override;

	void startSong(byte *data, size_t length, bool loop) override;
	void stopSong() override;
	void pauseSong() override;
	void restartSong() override;

	void setVolume(float volume) override;

	void writeVolume(int time, byte channel, byte volume) override;
	void writeControl(int time, byte channel, byte control, byte value) override;
	void writeChannel(int time, byte channel, byte status, byte param1, byte param2 = 0) override;
	void writeSysEx(int time, const byte *data, size_t length = 0) override;
	void allNotesOff() override;
	void allSoundOff() override;

	bool isInitialized() const override { return m_isInitialized; }

  private:
	static constexpr int cLatency = 80;
	static constexpr byte DEFAULT_VOLUME = 100;
	byte sysex_buffer[PM_DEFAULT_SYSEX_BUFFER_SIZE];
	byte m_channelVolume[NUM_CHANNELS];
	float m_volumeScale;
	bool m_isInitialized;
	bool m_isPlaying;

	PmDeviceID m_outputDevice;
	PmStream* m_stream;

	void _ResetAllControllers();
	void _ResetCommonControllers();
	void _ResetPitchBendSensitivity();
	void _ResetDevice(bool playing);
	bool _IsSysExReset(const byte *data, size_t length);
};

#endif // PORTMIDI
