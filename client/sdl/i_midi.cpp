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
//	MidiEvent classes, MidiSong class and midi file parsing functions.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <algorithm>
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
//static const size_t		cMaxSysexSize = 8192; //unused

static const byte drums_table[128] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x18, 0x19, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F
};

static byte variation[128][128];
static byte bank_msb[16];
static byte drum_map[16];
static bool selected[16];

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
		Printf(PRINT_WARNING, "I_ReadMidiTrack: Unexpected chunk header ID\n");
		return NULL;
	}
	
	std::list<MidiEvent*> *eventlist = new std::list<MidiEvent*>;
	
	size_t trackend = mem_ftell(mf) + chunkheader.chunk_size;
	while (mem_ftell(mf) < int(trackend))
	{
		MidiEvent *newevent = I_ReadMidiEvent(mf, track_time);
		
		if (!newevent)
		{
			Printf(PRINT_WARNING, "I_ReadMidiTrack: Unable to read MIDI event\n");
			
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
		Printf(PRINT_WARNING, "MidiSong::_ParseSong: Unexpected file header ID\n");
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
		Printf(PRINT_WARNING, "MidiSong::_ParseSong: Only type 0 or type 1 MIDI files are supported.\n");
		return;
	}
	
	// Read all the tracks and merge them into one stream of events
	for (size_t i = 0; i < fileheader.num_tracks; i++)
	{
		std::list<MidiEvent*> *eventlist = I_ReadMidiTrack(mf);
		if (!eventlist)
		{
			Printf(PRINT_WARNING, "MidiSong::_ParseSong: Error reading track %lu.\n", i + 1);
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

// ============================================================================
//
// MIDI instrument fallback support
//
// ============================================================================

static void UpdateDrumMap(const byte *data, size_t length)
{
	// GS allows drums on any channel using SysEx messages
	// The message format is F0 followed by:
	//
	// 41 10 42 12 40 <ch> 15 <map> <sum> F7
	//
	// <ch> is [11-19, 10, 1A-1F] for channels 1-16. Note the position of 10
	// <map> is 00-02 for off (normal part), drum map 1, or drum map 2
	// <sum> is checksum

	if (length == 10 &&
		data[0] == 0x41 && // Roland
		data[1] == 0x10 && // Device ID
		data[2] == 0x42 && // GS
		data[3] == 0x12 && // DT1
		data[4] == 0x40 && // Address MSB
		data[6] == 0x15 && // Address LSB
		data[9] == 0xF7)   // SysEx EOX
	{
		byte idx;
		byte checksum = 128 - ((int)data[4] + data[5] + data[6] + data[7]) % 128;

		if (data[8] != checksum)
			return;

		if (data[5] == 0x10) // Channel 10
		{
			idx = 9;
		}
		else if (data[5] < 0x1A) // Channels 1-9
		{
			idx = (data[5] & 0x0F) - 1;
		}
		else // Channels 11-16
		{
			idx = data[5] & 0x0F;
		}

		drum_map[idx] = data[7];
	}
}

static bool GetProgramFallback(byte idx, byte program, midi_fallback_t *fallback)
{
	if (drum_map[idx] == 0) // Normal channel
	{
		if (bank_msb[idx] == 0 || variation[bank_msb[idx]][program])
		{
			// Found a capital or variation for this bank select MSB
			selected[idx] = true;
			return false;
		}

		fallback->type = MIDI_FALLBACK_BANK_MSB;

		if (!selected[idx] || bank_msb[idx] > 63)
		{
			// Fall to capital when no instrument has (successfully) selected
			// this variation or if the variation is above 63
			fallback->value = 0;
			return true;
		}

		// A previous instrument used this variation but it's not valid for the
		// current instrument. Fall to the next valid "sub-capital" (next
		// variation that is a multiple of 8)
		fallback->value = (bank_msb[idx] / 8) * 8;
		while (fallback->value > 0)
		{
			if (variation[fallback->value][program])
				break;

			fallback->value -= 8;
		}
		return true;
	}
	else // Drums channel
	{
		if (program != drums_table[program])
		{
			// Use drum set from drums fallback table
			// Drums 0-63 and 127: same as original SC-55 (1.00 - 1.21)
			// Drums 64-126: standard drum set (0)
			fallback->type = MIDI_FALLBACK_DRUMS;
			fallback->value = drums_table[program];
			selected[idx] = true;
			return true;
		}
	}

	return false;
}

void I_MidiCheckFallback(MidiEvent *event, midi_fallback_t *fallback)
{
	if (I_IsMidiSysexEvent(event) && event->getEventType() == MIDI_EVENT_SYSEX)
	{
		MidiSysexEvent *sysexevent = static_cast<MidiSysexEvent*>(event);
		const byte *data = sysexevent->getData();
		size_t length = sysexevent->getDataLength();

		if (length > 0 && data[length - 1] == MIDI_EVENT_SYSEX_SPLIT)
			UpdateDrumMap(data, length);
	}
	else if (I_IsMidiControllerEvent(event))
	{
		MidiControllerEvent *ctrlevent = static_cast<MidiControllerEvent*>(event);
		byte idx = ctrlevent->getChannel();
		byte control = ctrlevent->getControllerType();
		byte param1 = ctrlevent->getParam1();

		switch (control)
		{
			case MIDI_CONTROLLER_BANK_SELECT_MSB:
				bank_msb[idx] = param1;
				selected[idx] = false;
				break;

			case MIDI_CONTROLLER_BANK_SELECT_LSB:
				selected[idx] = false;
				if (param1 > 0)
				{
					// Bank select LSB > 0 not supported. This also preserves
					// the user's current SC-XX map
					fallback->type = MIDI_FALLBACK_BANK_LSB;
					fallback->value = 0;
					return;
				}
				break;
		}
	}
	else if (I_IsMidiChannelEvent(event))
	{
		MidiChannelEvent *chanevent = static_cast<MidiChannelEvent*>(event);
		byte idx = chanevent->getChannel();
		byte param1 = chanevent->getParam1();

		if (event->getEventType() == MIDI_EVENT_PROGRAM_CHANGE)
		{
			if (GetProgramFallback(idx, param1, fallback))
				return;
		}
	}

	fallback->type = MIDI_FALLBACK_NONE;
	fallback->value = 0;
}

void I_MidiResetFallback()
{
	for (int i = 0; i < 16; i++)
	{
		bank_msb[i] = 0;
		drum_map[i] = 0;
		selected[i] = false;
	}

	// Channel 10 (index 9) is set to drum map 1 by default
	drum_map[9] = 1;
}

void I_MidiInitFallback()
{
	byte program;

	I_MidiResetFallback();

	// Capital
	for (program = 0; program < 128; program++)
		variation[0][program] = 1;

	// Variation #1
	variation[1][38] = 1;
	variation[1][57] = 1;
	variation[1][60] = 1;
	variation[1][80] = 1;
	variation[1][81] = 1;
	variation[1][98] = 1;
	variation[1][102] = 1;
	variation[1][104] = 1;
	variation[1][120] = 1;
	variation[1][121] = 1;
	variation[1][122] = 1;
	variation[1][123] = 1;
	variation[1][124] = 1;
	variation[1][125] = 1;
	variation[1][126] = 1;
	variation[1][127] = 1;

	// Variation #2
	variation[2][102] = 1;
	variation[2][120] = 1;
	variation[2][122] = 1;
	variation[2][123] = 1;
	variation[2][124] = 1;
	variation[2][125] = 1;
	variation[2][126] = 1;
	variation[2][127] = 1;

	// Variation #3
	variation[3][122] = 1;
	variation[3][123] = 1;
	variation[3][124] = 1;
	variation[3][125] = 1;
	variation[3][126] = 1;
	variation[3][127] = 1;

	// Variation #4
	variation[4][122] = 1;
	variation[4][124] = 1;
	variation[4][125] = 1;
	variation[4][126] = 1;

	// Variation #5
	variation[5][122] = 1;
	variation[5][124] = 1;
	variation[5][125] = 1;
	variation[5][126] = 1;

	// Variation #6
	variation[6][125] = 1;

	// Variation #7
	variation[7][125] = 1;

	// Variation #8
	variation[8][0] = 1;
	variation[8][1] = 1;
	variation[8][2] = 1;
	variation[8][3] = 1;
	variation[8][4] = 1;
	variation[8][5] = 1;
	variation[8][6] = 1;
	variation[8][11] = 1;
	variation[8][12] = 1;
	variation[8][14] = 1;
	variation[8][16] = 1;
	variation[8][17] = 1;
	variation[8][19] = 1;
	variation[8][21] = 1;
	variation[8][24] = 1;
	variation[8][25] = 1;
	variation[8][26] = 1;
	variation[8][27] = 1;
	variation[8][28] = 1;
	variation[8][30] = 1;
	variation[8][31] = 1;
	variation[8][38] = 1;
	variation[8][39] = 1;
	variation[8][40] = 1;
	variation[8][48] = 1;
	variation[8][50] = 1;
	variation[8][61] = 1;
	variation[8][62] = 1;
	variation[8][63] = 1;
	variation[8][80] = 1;
	variation[8][81] = 1;
	variation[8][107] = 1;
	variation[8][115] = 1;
	variation[8][116] = 1;
	variation[8][117] = 1;
	variation[8][118] = 1;
	variation[8][125] = 1;

	// Variation #9
	variation[9][14] = 1;
	variation[9][118] = 1;
	variation[9][125] = 1;

	// Variation #16
	variation[16][0] = 1;
	variation[16][4] = 1;
	variation[16][5] = 1;
	variation[16][6] = 1;
	variation[16][16] = 1;
	variation[16][19] = 1;
	variation[16][24] = 1;
	variation[16][25] = 1;
	variation[16][28] = 1;
	variation[16][39] = 1;
	variation[16][62] = 1;
	variation[16][63] = 1;

	// Variation #24
	variation[24][4] = 1;
	variation[24][6] = 1;

	// Variation #32
	variation[32][16] = 1;
	variation[32][17] = 1;
	variation[32][24] = 1;
	variation[32][52] = 1;

	// CM-64 Map (PCM)
	for (program = 0; program < 64; program++)
		variation[126][program] = 1;

	// CM-64 Map (LA)
	for (program = 0; program < 128; program++)
		variation[127][program] = 1;
}

VERSION_CONTROL (i_midi_cpp, "$Id$")
