// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include "resources/res_identifier.h"
#include "resources/res_main.h"

//
// Res_ValidateWadData
//
// Returns true if the given lump data appears to be a valid WAD file.
// TODO: test more than just identifying string.
//
bool Res_ValidateWadData(const uint8_t* data, size_t length)
{
	if (length >= 4)
	{
		uint32_t magic = LELONG(*(uint32_t*)((uint8_t*)data + 0));
		return magic == ('I' | ('W' << 8) | ('A' << 16) | ('D' << 24)) ||
				magic == ('P' | ('W' << 8) | ('A' << 16) | ('D' << 24));
	}
	return false;
}


//
// Res_ValidateDehackedData
//
// Returns true if the given lump data appears to be a valid DeHackEd file.
//
bool Res_ValidateDehackedData(const uint8_t* data, size_t length)
{
	const char magic_str[] = "Patch File for DeHackEd v";
	uint32_t magic_len = strlen(magic_str);

	return length >= magic_len && strnicmp((const char*)data, magic_str, magic_len) == 0;
}


//
// Res_ValidatePatchData
//
// Returns true if the given lump data appears to be a valid patch_t graphic.
//
bool Res_ValidatePatchData(const uint8_t* data, size_t length)
{
	if (length > 8)
	{
		const int16_t width = LESHORT(*(int16_t*)(data + 0));
		const int16_t height = LESHORT(*(int16_t*)(data + 2));

		const uint32_t column_table_offset = 8;
		const uint32_t column_table_length = sizeof(int32_t) * width;

		if (width > 0 && height > 0 && length >= column_table_offset + column_table_length)
		{
			const int32_t* column_offset = (const int32_t*)(data + column_table_offset);
			const int32_t min_column_offset = column_table_offset + column_table_length;
			const int32_t max_column_offset = length - 1;

			for (int i = 0; i < width; i++, column_offset++)
				if (*column_offset < min_column_offset || *column_offset > max_column_offset)
					return false;
			return true;
		}
	}
	return false;
}


//
// Res_ValidatePCSpeakerSoundData
//
// Returns true if the given lump data appears to be a valid PC speaker
// sound effect. The lump starts with a 4 byte header indicating the length
// of the sound effect in bytes and then is followed by that number of bytes
// of sound data.
//
bool Res_ValidatePCSpeakerSoundData(const uint8_t* data, size_t length)
{
	if (length > 4)
	{
		int16_t magic = LESHORT(*(int16_t*)((uint8_t*)data + 0));
		int16_t sample_length = LESHORT(*(int16_t*)((uint8_t*)data + 2));
		return magic == 0x0000 && (size_t)sample_length + 4 == length;
	}
	return false;
}


//
// Res_ValidateSoundData
//
// Returns true if the given lump data appears to be a valid DMX sound effect.
// The lump starts with a two byte version number (0x0003).
//
// User-created tools to convert to the DMX sound effect format typically
// do not use the correct number of padding bytes in the header and as such
// cannot be used to validate a sound lump.
//
bool Res_ValidateSoundData(const uint8_t* data, size_t length)
{
	if (length > 8)
	{
		uint16_t magic = LESHORT(*(uint16_t*)((uint8_t*)data + 0));
		return magic == 0x0003;
	}
	return false;
}


//
// Res_MusicIsMus()
//
// Determines if a music lump is in the MUS format based on its header.
//
bool Res_MusicIsMus(const uint8_t* data, size_t length)
{
	if (length > 4 && data[0] == 'M' && data[1] == 'U' &&
		data[2] == 'S' && data[3] == 0x1A)
		return true;

	return false;
}


//
// Res_MusicIsMidi()
//
// Determines if a music lump is in the MIDI format based on its header.
//
bool Res_MusicIsMidi(const uint8_t* data, size_t length)
{
	if (length > 4 && data[0] == 'M' && data[1] == 'T' &&
		data[2] == 'h' && data[3] == 'd')
		return true;

	return false;
}


//
// Res_MusicIsOgg()
//
// Determines if a music lump is in the OGG format based on its header.
//
bool Res_MusicIsOgg(const uint8_t* data, size_t length)
{
	if (length > 4 && data[0] == 'O' && data[1] == 'g' &&
		data[2] == 'g' && data[3] == 'S')
		return true;

	return false;
}


//
// Res_MusicIsMp3()
//
// Determines if a music lump is in the MP3 format based on its header.
//
bool Res_MusicIsMp3(const uint8_t* data, size_t length)
{
	// ID3 tag starts the file
	if (length > 4 && data[0] == 'I' && data[1] == 'D' &&
		data[2] == '3' && data[3] == 0x03)
		return true;

	// MP3 frame sync starts the file
	if (length > 2 && data[0] == 0xFF && (data[1] & 0xE0))
		return true;

	return false;
}


//
// Res_MusicIsWave()
//
// Determines if a music lump is in the WAVE/RIFF format based on its header.
//
bool Res_MusicIsWave(const uint8_t* data, size_t length)
{
	if (length > 4 && data[0] == 'R' && data[1] == 'I' &&
		data[2] == 'F' && data[3] == 'F')
		return true;

	return false;
}


//
// Res_ValidateMusicData
//
// Returns true if the given lump data appears to be a supported
// music resource format.
//
bool Res_ValidateMusicData(const uint8_t* data, size_t length)
{
	return Res_MusicIsMus(data, length) || Res_MusicIsMidi(data, length) ||
			Res_MusicIsOgg(data, length) || Res_MusicIsMp3(data, length);
}




// ============================================================================
//
// WadResourceIdentifer class implementation
//
// ============================================================================

const ResourcePath& WadResourceIdentifier::identifyByContents(const ResourcePath& path, const uint8_t* data, size_t length) const
{
	const OString& lump_name = path.last();
	if (Res_ValidatePatchData(data, length))
		return patches_directory_name;
	if (Res_ValidateMusicData(data, length))
		return music_directory_name;
	if (lump_name.compare(0, 2, "DP") == 0 && Res_ValidatePCSpeakerSoundData(data, length))
		return sounds_directory_name;
	if (Res_ValidateSoundData(data, length))
		return sounds_directory_name;
	return global_directory_name;
}