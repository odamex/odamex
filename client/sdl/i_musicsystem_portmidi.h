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
	virtual ~PortMidiMusicSystem();

	virtual void stopSong();
	virtual bool isInitialized() const { return m_isInitialized; }

	virtual void playEvent(MidiEvent* event, int time = 0);

  private:
	static constexpr int cLatency = 80;

	bool m_isInitialized;

	PmDeviceID m_outputDevice;
	PmStream* m_stream;

	void _PlayEvent(MidiEvent* event, int time = 0);
	void _StopSong();
};

#endif // PORTMIDI
