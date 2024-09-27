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
//  Plays music utilizing OSX's Audio Unit system, which is the default for
//  OSX.
//
//-----------------------------------------------------------------------------

#pragma once

#ifdef OSX

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include "i_musicsystem.h"

/**
 * @brief Plays music utilizing OSX's Audio Unit system, which is the default
 *        for OSX.
 *
 * @detail On non-OSX systems, the AuMusicSystem will not output any sound
 *         and should not be selected.
 */
class AuMusicSystem : public MusicSystem
{
  public:
	AuMusicSystem();
	virtual ~AuMusicSystem();

	virtual void startSong(byte* data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	virtual void playChunk() { }
	virtual void setVolume(float volume);

	virtual bool isInitialized() const { return m_isInitialized; }

	// Only plays midi-type music
	virtual bool isMusCapable() const { return true; }
	virtual bool isMidiCapable() const { return true; }

  private:
	bool m_isInitialized;

	MusicPlayer m_player;
	MusicSequence m_sequence;
	AUGraph m_graph;
	AUNode m_synth;
	AUNode m_output;
	AudioUnit m_unit;
	CFDataRef m_cfd;

	void _StopSong();
	void _RegisterSong(byte* data, size_t length);
	void _UnregisterSong();
};

#endif // OSX
