//-----------------------------------------------------------------------------
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
//  Plays music utilizing the SDL_Mixer library and can handle a wide range
//  of music formats.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "i_musicsystem_adlmidi.h"

#include "i_sdl.h"
#include <SDL_mixer.h>
#include "adlmidi.h"

#include "i_music.h"

EXTERN_CVAR(snd_samplerate)
EXTERN_CVAR(snd_musicvolume)
EXTERN_CVAR(snd_oplcore)
EXTERN_CVAR(snd_oplpan)
EXTERN_CVAR(snd_oplchips)
EXTERN_CVAR(snd_oplbank)


extern MusicSystem* musicsystem;
extern MusicSystemType current_musicsystem_type;

CVAR_FUNC_IMPL (snd_oplcore)
{
	if (current_musicsystem_type == MS_LIBADLMIDI)
		static_cast<AdlMidiMusicSystem *>(musicsystem)->applyCVars();
}
CVAR_FUNC_IMPL (snd_oplpan)
{
	if (current_musicsystem_type == MS_LIBADLMIDI)
		static_cast<AdlMidiMusicSystem *>(musicsystem)->applyCVars();
}
CVAR_FUNC_IMPL (snd_oplchips)
{
	if (current_musicsystem_type == MS_LIBADLMIDI)
		static_cast<AdlMidiMusicSystem *>(musicsystem)->applyCVars();
}
CVAR_FUNC_IMPL (snd_oplbank)
{
	if (current_musicsystem_type == MS_LIBADLMIDI)
		static_cast<AdlMidiMusicSystem *>(musicsystem)->applyCVars();
}

static int oplCoreMap[] = {ADLMIDI_EMU_DOSBOX, ADLMIDI_EMU_NUKED_174, ADLMIDI_EMU_NUKED};
static int oplBankMap[] = {16, 14, 72};


AdlMidiMusicSystem::AdlMidiMusicSystem() : m_mutex()
{
	// Midi mapper volume can interfere with PCM volume on some windows versions, ensure that it's set properly
	I_ResetMidiVolume();

	m_midiPlayer = adl_init(snd_samplerate.asInt());
	if (!m_midiPlayer)
	{
		PrintFmt(PRINT_WARNING, "I_InitMusic: Failed to initialize libADLMIDI with sample rate {}hz", snd_samplerate.str());
		return;
	}
	else
		PrintFmt("I_InitMusic: Music playback enabled using libADLMIDI.\n");

	m_isInitialized = true;

	applyCVars();
}

AdlMidiMusicSystem::~AdlMidiMusicSystem()
{
	if (!isInitialized())
		return;

	Mix_HaltMusic();
	_StopSong();

	adl_close(m_midiPlayer);
	m_isInitialized = false;
}


// adlmidi_music_hook()
// Mix_HookMusic callback installed during OPL music playback
static void adlmidi_music_hook (void *data, byte *stream, int len)
{
	AdlMidiHookData *hdata = static_cast<AdlMidiHookData *>(data);
	if (hdata->paused)
		return;

	const std::lock_guard<std::mutex> lock(*hdata->mutex);

	ADLMIDI_AudioFormat fmt;
	fmt.type = ADLMIDI_SampleType_S16;
	fmt.containerSize = 2;
	fmt.sampleOffset = 4;

	// Generate samples to fill the music buffer
	int out_count = adl_playFormat(hdata->player, len / fmt.containerSize, stream, stream + fmt.containerSize, &fmt);
	if (out_count < 0)
		memset(stream, 0, len);

	// Samples are generated at full volume, so iterate over them and scale them by the specified volume
	for (int i = 0; i < len; i += 2)
	{
		int16_t samp;
		memcpy(&samp, stream + i, 2);
		samp = clamp(samp * hdata->volume, -32768.f, 32767.f);
		memcpy(stream + i, &samp, 2);
	}
}


void AdlMidiMusicSystem::startSong(byte* data, size_t length, bool loop)
{
	if (!isInitialized())
		return;

	stopSong();

	if (!data || !length)
		return;

	_RegisterSong(data, length);

	if (!m_isPlaying)
		return;

	adl_setLoopEnabled(m_midiPlayer, loop);
	_UpdateMidiHook();

	MusicSystem::startSong(data, length, loop);
}

//
// AdlMidiMusicSystem::_StopSong()
//
// Fades the current music out
//
void AdlMidiMusicSystem::_StopSong()
{
	if (!isInitialized() || !isPlaying())
		return;

	if (!m_isPlaying)
		return;

	Mix_HookMusic(nullptr, nullptr);
}

//
// AdlMidiMusicSystem::stopSong()
//
// [SL] 2011-12-16 - This function simply calls _StopSong().  Since stopSong()
// is a virtual function, calls to it should be avoided in ctors & dtors.  Our
// dtor calls the non-virtual function _StopSong() instead and the virtual
// function stopSong() might as well reuse the code in _StopSong().
//
void AdlMidiMusicSystem::stopSong()
{
	_StopSong();
	MusicSystem::stopSong();
}

void AdlMidiMusicSystem::pauseSong()
{
	MusicSystem::pauseSong();

	if (!m_isPlaying)
		return;

	_UpdateMidiHook();
}

void AdlMidiMusicSystem::resumeSong()
{
	MusicSystem::resumeSong();

	if (!m_isPlaying)
		return;

	_UpdateMidiHook();
}



//
// AdlMidiMusicSystem::setVolume
//
// Sanity checks the volume parameter and then sets the volume for the midi
// output mixer channel.  Note that Windows Vista/7 combines the volume control
// for the audio and midi channels and changing the midi volume also changes
// the SFX volume.
//
void AdlMidiMusicSystem::setVolume(float volume)
{
	MusicSystem::setVolume(volume);

	_UpdateMidiHook();
	Mix_VolumeMusic(int(getVolume() * MIX_MAX_VOLUME));
}


void AdlMidiMusicSystem::_UpdateMidiHook()
{
	if (!m_isPlaying)
		return;

	SDL_LockAudio();
	m_midiHookData.player = m_midiPlayer;
	m_midiHookData.paused = isPaused();
	m_midiHookData.volume = getVolume();
	m_midiHookData.mutex = &m_mutex;
	SDL_UnlockAudio();

	Mix_HookMusic(adlmidi_music_hook, &m_midiHookData);
}

//
// AdlMidiMusicSystem::_RegisterSong
//
// Determines the format of music data and allocates the memory for the music
// data if appropriate.
//
void AdlMidiMusicSystem::_RegisterSong(byte* data, size_t length)
{
	const std::lock_guard<std::mutex> lock(m_mutex);
	m_isPlaying = false;
	if (S_MusicIsMus(data, length) || S_MusicIsMidi(data, length))
	{
		adl_reset(m_midiPlayer);
		if (adl_openData(m_midiPlayer, data, length) < 0)
		{
			PrintFmt(PRINT_WARNING, "adl_openData: {}\n", adl_errorInfo(m_midiPlayer));
			return;
		}

		m_isPlaying = true;
	}
}


void AdlMidiMusicSystem::applyCVars()
{
	if (m_isPlaying)
		Mix_HookMusic(nullptr, nullptr);

	adl_switchEmulator(m_midiPlayer, oplCoreMap[snd_oplcore.asInt()]);
	adl_setSoftPanEnabled(m_midiPlayer, snd_oplpan);
	adl_setNumChips(m_midiPlayer, snd_oplchips);
	adl_setBank(m_midiPlayer, oplBankMap[snd_oplbank.asInt()]);

	if (m_isPlaying)
		_UpdateMidiHook();
}