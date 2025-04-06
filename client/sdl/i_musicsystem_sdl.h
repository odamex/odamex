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
//  Plays music utilizing the SDL_Mixer library and can handle a wide range
//  of music formats.
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_midi.h"
#include "i_music.h"
#include "i_musicsystem.h"

/**
 * @brief Plays music utilizing the SDL_Mixer library and can handle a wide
 *        range of music formats.
 */
class SdlMixerMusicSystem : public MusicSystem
{
  public:
	SdlMixerMusicSystem();
	~SdlMixerMusicSystem() override;

	void startSong(byte* data, size_t length, bool loop) override ;
	void stopSong() override ;
	void pauseSong() override ;
	void resumeSong() override ;
	void playChunk()  override { }
	void setVolume(float volume) override ;

	bool isInitialized() const override { return m_isInitialized; }

	bool isMusCapable() const override { return true; }
	bool isMidiCapable() const override { return true; }
	bool isOggCapable() const override { return true; }
	bool isMp3Capable() const override { return true; }
	bool isModCapable() const override { return true; }
	bool isWaveCapable() const override { return true; }

  private:
	bool m_isInitialized;
	MusicHandler_t m_registeredSong;

	void _StopSong();
	void _RegisterSong(byte* data, size_t length);
	void _UnregisterSong();
};
