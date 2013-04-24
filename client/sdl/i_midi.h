// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_midi.h 1788 2010-08-24 04:42:57Z russellrice $
//
// Copyright (C) 2006-2013 by The Odamex Team.
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

#ifndef __I_MIDI_H__
#define __I_MIDI_H__

#include <stdlib.h>
#include <cstring>
#include <list>
#include <vector>
#include "doomtype.h"
#include "m_memio.h"

typedef enum
{
    MIDI_EVENT_NOTE_OFF        = 0x80,
    MIDI_EVENT_NOTE_ON         = 0x90,
    MIDI_EVENT_AFTERTOUCH      = 0xa0,
    MIDI_EVENT_CONTROLLER      = 0xb0,
    MIDI_EVENT_PROGRAM_CHANGE  = 0xc0,
    MIDI_EVENT_CHAN_AFTERTOUCH = 0xd0,
    MIDI_EVENT_PITCH_BEND      = 0xe0,

    MIDI_EVENT_SYSEX           = 0xf0,
    MIDI_EVENT_SYSEX_SPLIT     = 0xf7,
    MIDI_EVENT_META            = 0xff
} midi_event_type_t;

typedef enum
{
    MIDI_CONTROLLER_BANK_SELECT     = 0x0,
    MIDI_CONTROLLER_MODULATION      = 0x1,
    MIDI_CONTROLLER_BREATH_CONTROL  = 0x2,
    MIDI_CONTROLLER_FOOT_CONTROL    = 0x3,
    MIDI_CONTROLLER_PORTAMENTO      = 0x4,
    MIDI_CONTROLLER_DATA_ENTRY      = 0x5,

    MIDI_CONTROLLER_MAIN_VOLUME     = 0x7,
    MIDI_CONTROLLER_PAN             = 0xa,
    MIDI_CONTROLLER_RESET_ALL		= 0x79,
    MIDI_CONTROLLER_ALL_NOTES_OFF	= 0x7b
} midi_controller_t;

typedef enum
{
    MIDI_META_SEQUENCE_NUMBER       = 0x0,

    MIDI_META_TEXT                  = 0x1,
    MIDI_META_COPYRIGHT             = 0x2,
    MIDI_META_TRACK_NAME            = 0x3,
    MIDI_META_INSTR_NAME            = 0x4,
    MIDI_META_LYRICS                = 0x5,
    MIDI_META_MARKER                = 0x6,
    MIDI_META_CUE_POINT             = 0x7,

    MIDI_META_CHANNEL_PREFIX        = 0x20,
    MIDI_META_END_OF_TRACK          = 0x2f,

    MIDI_META_SET_TEMPO             = 0x51,
    MIDI_META_SMPTE_OFFSET          = 0x54,
    MIDI_META_TIME_SIGNATURE        = 0x58,
    MIDI_META_KEY_SIGNATURE         = 0x59,
    MIDI_META_SEQUENCER_SPECIFIC    = 0x7f
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

#endif	// __I_MIDI_H__

