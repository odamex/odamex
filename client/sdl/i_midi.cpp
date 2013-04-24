// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_muidi.h 1788 2010-08-24 04:42:57Z russellrice $
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

#include <algorithm>
#include "doomtype.h"
#include "m_memio.h"
#include "i_midi.h"

#ifndef ntohl
#ifdef WORDS_BIGENDIAN
#define ntohl
#define ntohs
#else // WORDS_BIGENDIAN

#define ntohl(x) \
        (/*(long int)*/((((unsigned long int)(x) & 0x000000ffU) << 24) | \
                             (((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
                             (((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
                             (((unsigned long int)(x) & 0xff000000U) >> 24)))

#define ntohs(x) \
        (/*(short int)*/((((unsigned short int)(x) & 0x00ff) << 8) | \
                              (((unsigned short int)(x) & 0xff00) >> 8)))
#endif // WORDS_BIGENDIAN
#endif // ntohl

typedef struct
{
    uint32_t		chunk_id;
    uint32_t		chunk_size;
} midi_chunk_header_t;

typedef struct
{
	uint16_t			format_type;
	uint16_t			num_tracks;
	uint16_t			time_division;
} midi_header_t;

typedef struct
{
	uint32_t		data_len;	// length in bytes

	MidiEvent		*events;
	uint32_t		num_events;
	uint32_t		num_event_mem;	// NSM track size of structure
} midi_track_t;

typedef struct
{
    midi_header_t header;

    midi_track_t	*tracks;
    unsigned int	num_tracks;

    // Data buffer used to store data read for SysEx or meta events:
    byte			*buffer;
    unsigned int	buffer_size;
}  midi_file_t;

static const uint32_t	cHeaderChunkId = 0x4D546864;
static const uint32_t	cTrackChunkId = 0x4D54726B;
static const size_t		cHeaderSize = 6;
static const size_t		cTrackHeaderSize = 8;
static const size_t		cMaxSysexSize = 8192;

// ============================================================================
//
// Non-member MidiSong helper functions
//
// Based partially on an implementation from Chocolate Doom by Simon Howard.
// ============================================================================

//
// I_ReadVariableSizeInt()
//
// Will read an int from a MEMFILE of unknown length.  For each byte we read,
// the highest bit will idicate if the next byte is to be read.  Returns the
// value or -1 if there was a read error.
//
static int I_ReadVariableSizeInt(MEMFILE *mf)
{
	if (!mf)
		return -1;
		
	// In midi files the variable can use between 1 and 5 bytes
	// if the high bit is set, the next byte is used
	int var = 0;
	for (size_t i = 0; i < 5; i++)
	{
		byte curbyte = 0;
		size_t res = mem_fread(&curbyte, sizeof(curbyte), 1, mf);
		if (!res)
			return -1;
			
		// Insert the bottom seven bits from this byte.			
        var = (var << 7) | (curbyte & 0x7f);
        
		// If the top bit is not set, this is the end.
        if ((curbyte & 0x80) == 0)
			break;
	}
	
	return var;
}


//
// I_ReadDataBlock()
//
// Returns a handle to the internal storage of a MEMFILE and moves the read
// position of the MEMFILE forward to the end of the block.
//
static byte* I_ReadDataBlock(MEMFILE *mf, size_t length)
{
	if (!mf)
		return NULL;
		
	size_t memfileoffset = mem_ftell(mf);
	if (mem_fsize(mf) < memfileoffset + length)
		return NULL;
	
	byte* data = (byte*)mem_fgetbuf(mf) + memfileoffset;
	mem_fseek(mf, length, MEM_SEEK_CUR);

	return data;
}

bool I_IsMidiChannelEvent(midi_event_type_t event)
{
	return ((event & 0xF0) == MIDI_EVENT_PROGRAM_CHANGE ||
			(event & 0xF0) == MIDI_EVENT_CHAN_AFTERTOUCH ||
			(event & 0xF0) == MIDI_EVENT_NOTE_OFF ||
			(event & 0xF0) == MIDI_EVENT_NOTE_ON ||
			(event & 0xF0) == MIDI_EVENT_AFTERTOUCH ||
			(event & 0xF0) == MIDI_EVENT_CONTROLLER ||
			(event & 0xF0) == MIDI_EVENT_PITCH_BEND);
}

bool I_IsMidiChannelEvent(MidiEvent *event)
{
	if (!event)
		return false;
	
	return I_IsMidiChannelEvent(event->getEventType());
}

bool I_IsMidiControllerEvent(midi_event_type_t event)
{
	return (event & 0xF0) == MIDI_EVENT_CONTROLLER;
}

bool I_IsMidiControllerEvent(MidiEvent *event)
{
	if (!event)
		return false;
		
	return I_IsMidiControllerEvent(event->getEventType());
}

bool I_IsMidiSysexEvent(midi_event_type_t event)
{
	return (event == MIDI_EVENT_SYSEX || event == MIDI_EVENT_SYSEX_SPLIT);
}

bool I_IsMidiSysexEvent(MidiEvent *event)
{
	if (!event)
		return false;
	
	return I_IsMidiSysexEvent(event->getEventType());
}

bool I_IsMidiMetaEvent(midi_event_type_t event)
{
	return (event == MIDI_EVENT_META);
}

bool I_IsMidiMetaEvent(MidiEvent *event)
{
	if (!event)
		return false;
	
	return I_IsMidiMetaEvent(event->getEventType());
}

static bool I_CompareMidiEventTimes(MidiEvent *a, MidiEvent *b)
{
	if (!a || !b)
		return true;
		
	return a->getMidiClockTime() < b->getMidiClockTime();
}

//
// I_ReadMidiEvent()
//
// Parses a MEMFILE and will create and return a MidiEvent object of the
// next midi event stored in the MEMFILE.  It will return NULL if there is
// an error.
//
static MidiEvent* I_ReadMidiEvent(MEMFILE *mf, unsigned int start_time)
{
	if (!mf)
		return NULL;
	
	int delta_time = I_ReadVariableSizeInt(mf);
	if (delta_time == -1)
		return NULL;
		
	unsigned int event_time = start_time + delta_time;
	
	// Read event type
	byte val;
	if (!mem_fread(&val, sizeof(val), 1, mf))
		return NULL;
		
	midi_event_type_t eventtype = static_cast<midi_event_type_t>(val);
	static midi_event_type_t prev_eventtype = eventtype;
	
	// All event types have their top bit set.  Therefore, if
	// the top bit is not set, it is because we are using the "same
	// as previous event type" shortcut to save a byte.  Skip back
	// a byte so that we read this byte again.
	if ((eventtype & 0x80) == 0)
	{
		eventtype = prev_eventtype;
		mem_fseek(mf, -1, MEM_SEEK_CUR);	// put the byte back in the memfile
	}
	else
		prev_eventtype = eventtype;
	
	if (I_IsMidiSysexEvent(eventtype))
	{
		int length = I_ReadVariableSizeInt(mf);
		if (length == -1)
			return NULL;
		
		byte* data = I_ReadDataBlock(mf, length);
		if (!data)
			return NULL;

		return new MidiSysexEvent(event_time, data, length);
	}
	
	if (I_IsMidiMetaEvent(eventtype))
	{
		byte val;
		if (!mem_fread(&val, sizeof(val), 1, mf))
			return NULL;
		
		midi_meta_event_type_t metatype = static_cast<midi_meta_event_type_t>(val);
		
		int length = I_ReadVariableSizeInt(mf);
		if (length == -1)
			return NULL;

		byte* data = I_ReadDataBlock(mf, length);
		if (!data)
			return NULL;
			
		return new MidiMetaEvent(event_time, metatype, data, length);
	}

	// Channel Events only use the highest 4 bits to denote the type
	// Lower four bits denote the channel
	int channel = eventtype & 0x0F;
	eventtype = static_cast<midi_event_type_t>(int(eventtype) & 0xF0);
	
	if (I_IsMidiControllerEvent(eventtype))
	{
		byte val, param1 = 0;
		
		if (!mem_fread(&val, sizeof(val), 1, mf))
			return NULL;

		midi_controller_t controllertype = static_cast<midi_controller_t>(val);
		
		if (!mem_fread(&param1, sizeof(param1), 1, mf))
			return NULL;
		
		return new MidiControllerEvent(event_time, controllertype, channel, param1);
	}
	
	if (I_IsMidiChannelEvent(eventtype))
	{
		byte param1 = 0, param2 = 0;
		
		if (!mem_fread(&param1, sizeof(param1), 1, mf))
			return NULL;
			
		if (eventtype != MIDI_EVENT_PROGRAM_CHANGE && 
			eventtype != MIDI_EVENT_CHAN_AFTERTOUCH)
		{
			// this is an event that uses two parameters
			if (!mem_fread(&param2, sizeof(param2), 1, mf))
				return NULL;
		}
		
		return new MidiChannelEvent(event_time, eventtype, channel, param1, param2);
	}
		
	// none of the above?
	return NULL;
}

static void I_ClearMidiEventList(std::list<MidiEvent*> *eventlist)
{
	if (!eventlist)
		return;
		
	while (!eventlist->empty())
	{
		if (eventlist->front())
			delete eventlist->front();
		eventlist->pop_front();
	}
}

//
// I_ReadMidiTrack()
//
// Reads an entire midi track from memory and creates a list of MidiEvents
// pointers where the event's start time is the time since the beginning of the
// track.  It is the caller's responsibility to delete the list returned
// by this function.
//
static std::list<MidiEvent*> *I_ReadMidiTrack(MEMFILE *mf)
{
	if (!mf)
		return NULL;
		
	midi_chunk_header_t chunkheader;
	unsigned int track_time = 0;
	
    size_t res = mem_fread(&chunkheader, cTrackHeaderSize, 1, mf); 
	if (!res)
		return NULL;
		
	chunkheader.chunk_id = ntohl(chunkheader.chunk_id);
	chunkheader.chunk_size = ntohl(chunkheader.chunk_size);
		
	if (chunkheader.chunk_id != cTrackChunkId)
	{
		Printf(PRINT_HIGH, "I_ReadMidiTrack: Unexpected chunk header ID\n");
		return NULL;
	}
	
	std::list<MidiEvent*> *eventlist = new std::list<MidiEvent*>;
	
	size_t trackend = mem_ftell(mf) + chunkheader.chunk_size;
	while (mem_ftell(mf) < int(trackend))
	{
		MidiEvent *newevent = I_ReadMidiEvent(mf, track_time);
		
		if (!newevent)
		{
			Printf(PRINT_HIGH, "I_ReadMidiTrack: Unable to read MIDI event\n");
			
			I_ClearMidiEventList(eventlist);
			delete eventlist;
			
			return NULL;
		}
		
		eventlist->push_back(newevent);
		track_time = newevent->getMidiClockTime();
	}
	
	return eventlist;
}


double I_GetTempoChange(MidiMetaEvent *event)
{
	if (event->getMetaType() == MIDI_META_SET_TEMPO)
	{
		size_t length = event->getDataLength();
		if (length == 3)
		{
			const byte* data = event->getData();
			static const double microsecondsperminute = 60.0 * 1000000.0;
			double microsecondsperbeat =	int(data[0]) << 16 | 
											int(data[1]) << 8 |
											int(data[2]);
			return microsecondsperminute / microsecondsperbeat;
		}
	}
		
	return 0.0;
}


// ============================================================================
//
// MidiSong implementation
//
// ============================================================================

MidiSong::MidiSong(byte* data, size_t length) :
	mEvents(), mTimeDivision(96)
{
	if (data && length)
	{
		MEMFILE *mf = mem_fopen_read(data, length);
		_ParseSong(mf);
	}
}

MidiSong::~MidiSong()
{
	I_ClearMidiEventList(&mEvents);
}

//
// MidiSong::_ParseSong()
//
// Loads a midi song stored in the MEMFILE, parses the header, and reads
// all of the midi events from the song.  If the midi song has multiple
// tracks, they are merged into a single stream of events.
//
// Based partially on an implementation from Chocolate Doom by Simon Howard.
//
void MidiSong::_ParseSong(MEMFILE *mf)
{
	if (!mf)
		return;
		
	I_ClearMidiEventList(&mEvents);
	
	mem_fseek(mf, 0, MEM_SEEK_SET);
		
	midi_chunk_header_t chunkheader;
    if (!mem_fread(&chunkheader, cTrackHeaderSize, 1, mf))
		return;
		
	chunkheader.chunk_id = ntohl(chunkheader.chunk_id);
	chunkheader.chunk_size = ntohl(chunkheader.chunk_size);
		
	if (chunkheader.chunk_id != cHeaderChunkId)
	{
		Printf(PRINT_HIGH, "MidiSong::_ParseSong: Unexpected file header ID\n");
		return;
	}

	midi_header_t fileheader;
	if (!mem_fread(&fileheader, cHeaderSize, 1, mf))
		return;
	
	fileheader.format_type = ntohs(fileheader.format_type);
	fileheader.num_tracks = ntohs(fileheader.num_tracks);
	fileheader.time_division = ntohs(fileheader.time_division);
	mTimeDivision = fileheader.time_division;

	if (fileheader.format_type != 0 && fileheader.format_type != 1)
	{
		Printf(PRINT_HIGH, "MidiSong::_ParseSong: Only type 0 or type 1 MIDI files are supported.\n");
		return;
	}
	
	// Read all the tracks and merge them into one stream of events
	for (size_t i = 0; i < fileheader.num_tracks; i++)
	{
		std::list<MidiEvent*> *eventlist = I_ReadMidiTrack(mf);
		if (!eventlist)
		{
			Printf(PRINT_HIGH, "MidiSong::_ParseSong: Error reading track %d.\n", i + 1);
			return;
		}
		
		// add this track's list of events to the song's list
		while (!eventlist->empty())
		{
			mEvents.push_back(eventlist->front());
			eventlist->pop_front();
		}
		
		delete eventlist;
	}

	// sort the stream of events by start time 
	// (only needed if we're merging multiple tracks)
	if (fileheader.num_tracks > 1)
		mEvents.sort(I_CompareMidiEventTimes);
}

VERSION_CONTROL (i_midi_cpp, "$Id: i_midi.cpp 2671 2011-12-19 00:20:32Z dr_sean $")
