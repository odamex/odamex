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
// Res_ValidatePCSpeakerSoundData
//
// Returns true if the given lump data appears to be a valid PC speaker
// sound effect. The lump starts with a 4 byte header indicating the length
// of the sound effect in bytes and then is followed by that number of bytes
// of sound data.
//
bool Res_ValidatePCSpeakerSoundData(const uint8_t* data, size_t length)
{
	int16_t* magic = (int16_t*)((uint8_t*)data + 0);
	int16_t* sample_length = (int16_t*)((uint8_t*)data + 2);
	return length >= 4 && LESHORT(*magic) == 0 && LESHORT(*sample_length) + 4 == (int16_t)length;
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
	uint16_t* magic = (uint16_t*)((uint8_t*)data + 0);
	return length >= 2 && LESHORT(*magic) == 3;
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
			const int32_t max_column_offset = length - 4;

			for (int i = 0; i < width; i++, column_offset++)
				if (*column_offset < min_column_offset || *column_offset > max_column_offset)
					return false;
			return true;
		}
	}
	return false;
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
	// if (lump_name.compare(0, 2, "D_") == 0)
	//		return music_directory_name;
	if (lump_name.compare(0, 2, "DP") == 0 && Res_ValidatePCSpeakerSoundData(data, length))
		return sounds_directory_name;
	// if (lump_name.compare(0, 2, "DS") == 0 && Res_ValidateSoundData(data, length))
	if (Res_ValidateSoundData(data, length))
		return sounds_directory_name;
	return global_directory_name;
}

