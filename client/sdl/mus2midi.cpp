// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2005 by Simon Howard
// Copyright (C) 2006 by Ben Ryves 2006
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// DESCRIPTION:
//	mus2mid.cpp - Ben Ryves 2006 - http://benryves.com - benryves@benryves.com
//	Use to convert a MUS file into a single track, type 0 MIDI file.
// 
//	Russell - Minor modifications to make it compile
//
//-----------------------------------------------------------------------------

#include "odamex.h"


#include "m_memio.h"
#include "mus2midi.h"

// MUS event codes
typedef enum 
{
	mus_releasekey = 0x00,
	mus_presskey = 0x10,
	mus_pitchwheel = 0x20,
	mus_systemevent = 0x30,
	mus_changecontroller = 0x40,
	mus_scoreend = 0x60
} musevent;

// MIDI event codes
typedef enum 
{
	midi_releasekey = 0x80,
	midi_presskey = 0x90,
	midi_aftertouchkey = 0xA0,
	midi_changecontroller = 0xB0,
	midi_changepatch = 0xC0,
	midi_aftertouchchannel = 0xD0,
	midi_pitchwheel = 0xE0
} midievent;


// Structure to hold MUS file header
typedef struct 
{
  BYTE id[4];
  unsigned short scorelength;
  unsigned short scorestart;
  unsigned short primarychannels;
  unsigned short secondarychannels;
  unsigned short instrumentcount;
} PACKEDATTR musheader;

// Standard MIDI type 0 header + track header
static BYTE midiheader[] = 
{
	'M', 'T', 'h', 'd',     // Main header
	0x00, 0x00, 0x00, 0x06, // Header size
	0x00, 0x00,             // MIDI type (0)
	0x00, 0x01,             // Number of tracks
	0x00, 0x46,             // Resolution
	'M', 'T', 'r', 'k',		// Start of track
	0x00, 0x00, 0x00, 0x00  // Placeholder for track length
};

// Cached channel velocities
static BYTE channelvelocities[] = 
{ 
	127, 127, 127, 127, 127, 127, 127, 127, 
	127, 127, 127, 127, 127, 127, 127, 127 
};

// Timestamps between sequences of MUS events

static unsigned int queuedtime = 0; 

// Counter for the length of the track

static unsigned int tracksize;

static BYTE mus2midi_translation[] = 
{ 
	0x00, 0x20, 0x01, 0x07, 0x0A, 0x0B, 0x5B, 0x5D,
	0x40, 0x43, 0x78, 0x7B, 0x7E, 0x7F, 0x79 
};

// Write timestamp to a MIDI file.

static BOOL midi_writetime(unsigned int time, MEMFILE *midioutput) 
{
	unsigned int buffer = time & 0x7F;
	BYTE writeval;

	while ((time >>= 7) != 0) 
	{
		buffer <<= 8;
		buffer |= ((time & 0x7F) | 0x80);
	}

	for (;;) 
	{
		writeval = (BYTE)(buffer & 0xFF);

		if (mem_fwrite(&writeval, 1, 1, midioutput) != 1) 
		{
			return true;
		}

		++tracksize;

		if ((buffer & 0x80) != 0) 
		{
			buffer >>= 8;
		} 
		else 
		{
			queuedtime = 0;
			return false;
		}
	}
}


// Write the end of track marker
static BOOL midi_writeendtrack(MEMFILE *midioutput) 
{
	BYTE endtrack[] = {0xFF, 0x2F, 0x00};

	if (midi_writetime(queuedtime, midioutput)) 
	{
		return true;
	}

	if (mem_fwrite(endtrack, 1, 3, midioutput) != 3) 
	{
		return true;
	}

	tracksize += 3;
	return false;
}

// Write a key press event
static BOOL midi_writepresskey(BYTE channel, BYTE key, 
                              BYTE velocity, MEMFILE *midioutput) 
{
	BYTE working = midi_presskey | channel;

	if (midi_writetime(queuedtime, midioutput)) 
	{
		return true;
	}

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	working = key & 0x7F;

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}
	
	working = velocity & 0x7F;
	
	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	tracksize += 3;

	return false;
}

// Write a key release event
static BOOL midi_writereleasekey(BYTE channel, BYTE key, 
                                MEMFILE *midioutput) 
{
	BYTE working = midi_releasekey | channel;

	if (midi_writetime(queuedtime, midioutput)) 
	{
		return true;
	}

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	working = key & 0x7F;

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	working = 0;

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	tracksize += 3;

	return false;
}

// Write a pitch wheel/bend event
static BOOL midi_writepitchwheel(BYTE channel, short wheel, 
                                MEMFILE *midioutput) 
{
	BYTE working = midi_pitchwheel | channel;

	if (midi_writetime(queuedtime, midioutput)) 
	{
		return true;
	}

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	working = wheel & 0x7F;

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	working = (wheel >> 7) & 0x7F;

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	tracksize += 3;
	return false;
}

// Write a patch change event
static BOOL midi_writechangepatch(BYTE channel, BYTE patch,
                                 MEMFILE *midioutput) 
{
	BYTE working = midi_changepatch | channel;
	
	if (midi_writetime(queuedtime, midioutput)) 
	{
		return true;
	}

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	working = patch & 0x7F;

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	tracksize += 2;

	return false;
}



// Write a valued controller change event
static BOOL midi_writechangecontroller_valued(BYTE channel, 
                                             BYTE control, 
                                             BYTE value, 
                                             MEMFILE *midioutput) 
{
	BYTE working = midi_changecontroller | channel;

	if (midi_writetime(queuedtime, midioutput)) 
	{
		return true;
	}

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	working = control & 0x7F;

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}
	// Quirk in vanilla DOOM? MUS controller values should be 
	// 7-bit, not 8-bit.

	working = value;// & 0x7F; 

	// Fix on said quirk to stop MIDI players from complaining that 
	// the value is out of range:

	if (working & 0x80) 
	{
		working = 0x7F;
	}

	if (mem_fwrite(&working, 1, 1, midioutput) != 1) 
	{
		return true;
	}

	tracksize += 3;

	return false;
}

// Write a valueless controller change event
static BYTE midi_writechangecontroller_valueless(BYTE channel, 
                                                BYTE control, 
                                                MEMFILE *midioutput) 
{
	return midi_writechangecontroller_valued(channel, control, 0, 
			 			 midioutput);
}

static BOOL read_musheader(MEMFILE *file, musheader *header)
{
	BOOL result;

	result = (mem_fread(&header->id, sizeof(BYTE), 4, file) == 4)
              && (mem_fread(&header->scorelength, sizeof(short), 1, file) == 1)
              && (mem_fread(&header->scorestart, sizeof(short), 1, file) == 1)
              && (mem_fread(&header->primarychannels, sizeof(short), 1, file) == 1)
              && (mem_fread(&header->secondarychannels, sizeof(short), 1, file) == 1)
              && (mem_fread(&header->instrumentcount, sizeof(short), 1, file) == 1);

	if (result)
	{
		header->scorelength = LESHORT(header->scorelength);
		header->scorestart = LESHORT(header->scorestart);
		header->primarychannels = LESHORT(header->primarychannels);
		header->secondarychannels = LESHORT(header->secondarychannels);
		header->instrumentcount = LESHORT(header->instrumentcount);
	}

	return result;
}


// Read a MUS file from a stream (musinput) and output a MIDI file to 
// a stream (midioutput).
//
// Returns 0 on success or 1 on failure.

QWORD mus2mid(MEMFILE *musinput, MEMFILE *midioutput) 
{
	// Header for the MUS file
	musheader musfileheader;         

	// Descriptor for the current MUS event
	BYTE eventdescriptor;   
	int channel; // Channel number
	musevent event; 
	

	// Bunch of vars read from MUS lump
	BYTE key;
	BYTE controllernumber;
	BYTE controllervalue;

	// Buffer used for MIDI track size record
	BYTE tracksizebuffer[4]; 

	// Flag for when the score end marker is hit.
	int hitscoreend = 0; 

	// Temp working byte
	BYTE working; 
	// Used in building up time delays
	unsigned int timedelay; 

	// Grab the header

	if (!read_musheader(musinput, &musfileheader))
	{
		return 1;
	}

	// Check MUS header
	if (musfileheader.id[0] != 'M' 
	 || musfileheader.id[1] != 'U' 
	 || musfileheader.id[2] != 'S' 
	 || musfileheader.id[3] != 0x1A) 
	{
		// [Russell] - write it out anyway, saves mucking around
		mem_fwrite(mem_fgetbuf(musinput), 1, mem_fsize(musinput), midioutput);
		return 2;
	}

	// Seek to where the data is held
	if (mem_fseek(musinput, (long)musfileheader.scorestart, MEM_SEEK_SET) != 0) 
	{
		return 1;
	}

	// So, we can assume the MUS file is faintly legit. Let's start 
	// writing MIDI data...

	mem_fwrite(midiheader, 1, sizeof(midiheader), midioutput);
	tracksize = 0;

	// Now, process the MUS file:
	while (!hitscoreend) 
	{
		// Handle a block of events:

		while (!hitscoreend) 
		{
			// Fetch channel number and event code:

			if (mem_fread(&eventdescriptor, 1, 1, musinput) != 1) 
			{
				return 1;
			}

			channel = eventdescriptor & 0x0F;
			event = (musevent)(eventdescriptor & 0x70);

			// Swap channels 15 and 9.
			// MIDI channel 9 = percussion.
			// MUS channel 15 = percussion.

			if (channel == 15) 
			{
				channel = 9;
			} 
			else if (channel == 9) 
			{
				channel = 15;
			}
			
			switch (event) 
			{
				case mus_releasekey:
					if (mem_fread(&key, 1, 1, musinput) != 1) 
					{
						return 1;
					}

					if (midi_writereleasekey(channel, key, midioutput)) 
					{
						return 1;
					}

					break;
				
				case mus_presskey:
					if (mem_fread(&key, 1, 1, musinput) != 1) 
					{
						return 1;
					}

					if (key & 0x80) 
					{
						if (mem_fread(&channelvelocities[channel], 1, 1, musinput) != 1) 
						{
							return 1;
						}

						channelvelocities[channel] &= 0x7F;
					}

					if (midi_writepresskey(channel, key, channelvelocities[channel], midioutput)) 
					{
						return 1;
					}

					break;

				case mus_pitchwheel:
					if (mem_fread(&key, 1, 1, musinput) != 1) 
					{
						break;
					}
					if (midi_writepitchwheel(channel, (short)(key * 64), midioutput)) 
					{
						return 1;
					}

					break;
				
				case mus_systemevent:
					if (mem_fread(&controllernumber, 1, 1, musinput) != 1) 
					{
						return 1;
					}
					if (controllernumber < 10 || controllernumber > 14) 
					{
						return 1;
					}

					if (midi_writechangecontroller_valueless(channel, mus2midi_translation[controllernumber], midioutput)) 
					{
						return 1;
					}

					break;
				
				case mus_changecontroller:
					if (mem_fread(&controllernumber, 1, 1, musinput) != 1) 
					{
						return 1;
					}

					if (mem_fread(&controllervalue, 1, 1, musinput) != 1) 
					{
						return 1;
					}

					if (controllernumber == 0) 
					{
						if (midi_writechangepatch(channel, controllervalue, midioutput)) 
						{
							return 1;
						}
					} 
					else 
					{
						if (controllernumber < 1 || controllernumber > 9) 
						{
							return 1;
						}

						if (midi_writechangecontroller_valued(channel, mus2midi_translation[controllernumber], controllervalue, midioutput)) 
						{
							return 1;  
						}
					}

					break;
				
				case mus_scoreend:
					hitscoreend = 1;
					break;

				default:
					return 1;
					break;
			}

			if (eventdescriptor & 0x80) 
			{
				break;
			}
		}
		// Now we need to read the time code:
		if (!hitscoreend) 
		{
			timedelay = 0;
			for (;;) 
			{
				if (mem_fread(&working, 1, 1, musinput) != 1) 
				{
					return 1;
				}

				timedelay = timedelay * 128 + (working & 0x7F);
				if ((working & 0x80) == 0) 
				{
					break;
				}
			}
			queuedtime += timedelay;
		}
	}

	// End of track
	if (midi_writeendtrack(midioutput)) 
	{
		return 1;
	}

	// Write the track size into the stream
	if (mem_fseek(midioutput, 18, MEM_SEEK_SET)) 
	{
		return 1;
	}

    tracksizebuffer[0] = (tracksize >> 24) & 0xff;
    tracksizebuffer[1] = (tracksize >> 16) & 0xff;
    tracksizebuffer[2] = (tracksize >> 8) & 0xff;
    tracksizebuffer[3] = tracksize & 0xff;

	if (mem_fwrite(tracksizebuffer, 1, 4, midioutput) != 4) 
	{
		return 1;
	}

	return 0;
}


VERSION_CONTROL (mus2midi_cpp, "$Id$")
