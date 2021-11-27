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
//  Plays music utilizing OSX's Audio Unit system, which is the default for
//  OSX.  On non-OSX systems, the AuMusicSystem will not output any sound
//  and should not be selected.
//
//-----------------------------------------------------------------------------

#pragma once

#ifdef OSX

#include "i_musicsystem.h"

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

	virtual bool isInitialized() const { return mIsInitialized; }

	// Only plays midi-type music
	virtual bool isMusCapable() const { return true; }
	virtual bool isMidiCapable() const { return true; }

  private:
	bool mIsInitialized;

	MusicPlayer mPlayer;
	MusicSequence mSequence;
	AUGraph mGraph;
	AUNode mSynth;
	AUNode mOutput;
	AudioUnit mUnit;
	CFDataRef mCfd;

	void _StopSong();
	void _RegisterSong(byte* data, size_t length);
	void _UnregisterSong();
};

#endif // OSX
