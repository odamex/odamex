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
//	MidiEvent classes, MidiSong class and midi file parsing functions.
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdlib.h>
#include <list>
#include "m_memio.h"

static byte gm_system_on[] = {
	0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7
};

static byte gs_reset[] = {
	0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7
};

static byte xg_system_on[] = {
	0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7
};

typedef enum
{
	MIDI_RESET_NONE,
	MIDI_RESET_GM,
	MIDI_RESET_GS,
	MIDI_RESET_XG,
} midi_reset_t;

typedef enum
{
	MIDI_FALLBACK_NONE,
	MIDI_FALLBACK_BANK_MSB,
	MIDI_FALLBACK_BANK_LSB,
	MIDI_FALLBACK_DRUMS,
} midi_fallback_type_t;

typedef struct
{
	midi_fallback_type_t type;
	byte value;
} midi_fallback_t;

typedef enum
{
	MIDI_EVENT_NOTE_OFF        = 0x80,
	MIDI_EVENT_NOTE_ON         = 0x90,
	MIDI_EVENT_AFTERTOUCH      = 0xA0,
	MIDI_EVENT_CONTROLLER      = 0xB0,
	MIDI_EVENT_PROGRAM_CHANGE  = 0xC0,
	MIDI_EVENT_CHAN_AFTERTOUCH = 0xD0,
	MIDI_EVENT_PITCH_BEND      = 0xE0,

	MIDI_EVENT_SYSEX           = 0xF0,
	MIDI_EVENT_SYSEX_SPLIT     = 0xF7,
	MIDI_EVENT_META            = 0xFF,
} midi_event_type_t;

typedef enum
{
	MIDI_CONTROLLER_BANK_SELECT_MSB = 0x00,
	MIDI_CONTROLLER_MODULATION      = 0x01,
	MIDI_CONTROLLER_BREATH_CONTROL  = 0x02,
	MIDI_CONTROLLER_FOOT_CONTROL    = 0x04,
	MIDI_CONTROLLER_PORTAMENTO      = 0x05,
	MIDI_CONTROLLER_DATA_ENTRY_MSB  = 0x06,
	MIDI_CONTROLLER_VOLUME_MSB      = 0x07,
	MIDI_CONTROLLER_PAN             = 0x0A,

	MIDI_CONTROLLER_BANK_SELECT_LSB = 0x20,
	MIDI_CONTROLLER_DATA_ENTRY_LSB  = 0x26,
	MIDI_CONTROLLER_VOLUME_LSB      = 0X27,

	MIDI_CONTROLLER_REVERB          = 0x5B,
	MIDI_CONTROLLER_CHORUS          = 0x5D,

	MIDI_CONTROLLER_RPN_LSB         = 0x64,
	MIDI_CONTROLLER_RPN_MSB         = 0x65,

	MIDI_CONTROLLER_ALL_SOUND_OFF   = 0x78,
	MIDI_CONTROLLER_RESET_ALL_CTRLS = 0x79,
	MIDI_CONTROLLER_ALL_NOTES_OFF   = 0x7B,
} midi_controller_t;

typedef enum
{
	MIDI_META_SEQUENCE_NUMBER    = 0x00,

	MIDI_META_TEXT               = 0x01,
	MIDI_META_COPYRIGHT          = 0x02,
	MIDI_META_TRACK_NAME         = 0x03,
	MIDI_META_INSTR_NAME         = 0x04,
	MIDI_META_LYRICS             = 0x05,
	MIDI_META_MARKER             = 0x06,
	MIDI_META_CUE_POINT          = 0x07,

	MIDI_META_CHANNEL_PREFIX     = 0x20,
	MIDI_META_END_OF_TRACK       = 0x2F,

	MIDI_META_SET_TEMPO          = 0x51,
	MIDI_META_SMPTE_OFFSET       = 0x54,
	MIDI_META_TIME_SIGNATURE     = 0x58,
	MIDI_META_KEY_SIGNATURE      = 0x59,
	MIDI_META_SEQUENCER_SPECIFIC = 0x7F,
} midi_meta_event_type_t;


class MidiEvent
{
public:
	MidiEvent(unsigned int time, midi_event_type_t type) :
		mTime(time), mType(type)
	{ }
	
	MidiEvent(const MidiEvent &other) :
		mTime(other.mTime), mType(other.mType)
	{ }
	
	virtual ~MidiEvent() { }

	unsigned int getMidiClockTime() const { return mTime; }
	virtual midi_event_type_t getEventType() const { return mType; }
	
private:
	unsigned int		mTime;
	midi_event_type_t	mType;
};


class MidiMetaEvent : public MidiEvent
{
public:
	MidiMetaEvent(unsigned int time, midi_meta_event_type_t meta, byte* data, size_t length) :
		MidiEvent(time, MIDI_EVENT_META), mMetaType(meta),	mData(NULL), mLength(length)
	{
		if (mLength)
		{
			mData = new byte[mLength];
			memcpy(mData, data, mLength);
		}
	}
	
	virtual ~MidiMetaEvent()
	{
		if (mData)
			delete mData;
	}
	
	virtual size_t getDataLength() const { return mLength; }
	virtual const byte* getData() const	{ return mData; }
	virtual midi_meta_event_type_t getMetaType() const { return mMetaType; }

private:
	midi_meta_event_type_t	mMetaType;
	byte*					mData;
	size_t					mLength;
};


class MidiSysexEvent : public MidiEvent
{
public:
	MidiSysexEvent(unsigned int time, byte* data, size_t length) :
		MidiEvent(time, MIDI_EVENT_SYSEX), mData(NULL), mLength(length)
	{
		if (mLength)
		{
			mData = new byte[mLength];
			memcpy(mData, data, mLength);
		}
	}
	
	virtual ~MidiSysexEvent()
	{
		if (mData)
			delete mData;
	}
	
	virtual size_t getDataLength() const { return mLength; }
	virtual const byte* getData() const	{ return mData; }

private:
	byte*				mData;
	size_t				mLength;
};


class MidiChannelEvent : public MidiEvent
{
public:
	MidiChannelEvent(unsigned int time, midi_event_type_t type, byte channel,
					 byte param1, byte param2 = 0) :
		MidiEvent(time, type), mChannel(channel), mParam1(param1), mParam2(param2)
	{ }
	
	virtual ~MidiChannelEvent() {}
	
	byte getChannel() const { return mChannel; }
	byte getParam1() const { return mParam1; }
	byte getParam2() const { return mParam2; }
	
private:
	byte				mChannel;
	byte				mParam1;
	byte				mParam2;
};


class MidiControllerEvent : public MidiChannelEvent
{
public:
	MidiControllerEvent(unsigned int time, midi_controller_t controllertype,
						byte channel, byte param1 = 0) :
		MidiChannelEvent(time, MIDI_EVENT_CONTROLLER, channel, param1, 0),
		mControllerType(controllertype)
	{ }
	
	virtual ~MidiControllerEvent() {}
	
	midi_controller_t getControllerType() const { return mControllerType; }
	
private:
	midi_controller_t	mControllerType;
};


class MidiSong
{
public:
	MidiSong(byte* data, size_t length);
	~MidiSong();
	
	unsigned short getTimeDivision() { return mTimeDivision; }
	
	typedef std::list<MidiEvent*>::iterator iterator;
	typedef std::list<MidiEvent*>::const_iterator const_iterator;
	
	// Allow iteration through a song's events
	iterator begin() { return mEvents.begin(); }
	const_iterator begin() const { return mEvents.begin(); }
	iterator end() { return mEvents.end(); }
	const_iterator end() const { return mEvents.end(); }

private:
	std::list<MidiEvent*>	mEvents;
	unsigned short			mTimeDivision;
	
	void _ParseSong(MEMFILE *mf);
};


bool I_IsMidiChannelEvent(midi_event_type_t event);
bool I_IsMidiChannelEvent(MidiEvent *event);
bool I_IsMidiControllerEvent(midi_event_type_t event);
bool I_IsMidiControllerEvent(MidiEvent *event);
bool I_IsMidiSysexEvent(midi_event_type_t event);
bool I_IsMidiSysexEvent(MidiEvent *event);
bool I_IsMidiMetaEvent(midi_event_type_t event);
bool I_IsMidiMetaEvent(MidiEvent *event);
double I_GetTempoChange(MidiMetaEvent *event);

void I_MidiCheckFallback(MidiEvent *event, midi_fallback_t *fallback);
void I_MidiResetFallback();
void I_MidiInitFallback();
