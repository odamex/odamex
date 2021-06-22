// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//  Public Domain base64 implementation from Wikibooks
//  https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64
//
//-----------------------------------------------------------------------------

#include "base64.h"

// Lookup table for encoding
// If you want to use an alternate alphabet, change the characters here
static const char encodeLookup[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char PADDING_CHAR = '=';

std::string M_Base64Encode(std::vector<byte> inputBuffer)
{
	std::string encodedString;
	encodedString.reserve(((inputBuffer.size() / 3) + (inputBuffer.size() % 3 > 0)) * 4);
	uint32_t temp;

	std::vector<byte>::iterator cursor = inputBuffer.begin();
	for (size_t idx = 0; idx < inputBuffer.size() / 3; idx++)
	{
		temp = (*cursor++) << 16; // Convert to big endian
		temp += (*cursor++) << 8;
		temp += (*cursor++);
		encodedString.append(1, encodeLookup[(temp & 0x00FC0000) >> 18]);
		encodedString.append(1, encodeLookup[(temp & 0x0003F000) >> 12]);
		encodedString.append(1, encodeLookup[(temp & 0x00000FC0) >> 6]);
		encodedString.append(1, encodeLookup[(temp & 0x0000003F)]);
	}

	switch (inputBuffer.size() % 3)
	{
	case 1:
		temp = (*cursor++) << 16; // Convert to big endian
		encodedString.append(1, encodeLookup[(temp & 0x00FC0000) >> 18]);
		encodedString.append(1, encodeLookup[(temp & 0x0003F000) >> 12]);
		encodedString.append(2, PADDING_CHAR);
		break;

	case 2:
		temp = (*cursor++) << 16; // Convert to big endian
		temp += (*cursor++) << 8;
		encodedString.append(1, encodeLookup[(temp & 0x00FC0000) >> 18]);
		encodedString.append(1, encodeLookup[(temp & 0x0003F000) >> 12]);
		encodedString.append(1, encodeLookup[(temp & 0x00000FC0) >> 6]);
		encodedString.append(1, PADDING_CHAR);
		break;
	}

	return encodedString;
}

bool M_Base64Decode(std::vector<byte>& decodedBytes, const std::string& input)
{
	if (input.length() % 4) // Sanity check
		return false;

	size_t padding = 0;
	if (input.length())
	{
		if (input[input.length() - 1] == PADDING_CHAR)
			padding++;
		if (input[input.length() - 2] == PADDING_CHAR)
			padding++;
	}

	// Setup a vector to hold the result
	decodedBytes.clear();
	decodedBytes.reserve(((input.length() / 4) * 3) - padding);
	uint32_t temp = 0; // Holds decoded quanta

	std::string::const_iterator cursor = input.begin();
	while (cursor < input.end())
	{
		for (size_t quantumPosition = 0; quantumPosition < 4; quantumPosition++)
		{
			temp <<= 6;
			if (*cursor >= 'A' && *cursor <= 'Z') // This area will need tweaking if
				temp |= *cursor - 0x41;           // you are using an alternate alphabet
			else if (*cursor >= 'a' && *cursor <= 'z')
				temp |= *cursor - 0x47;
			else if (*cursor >= '0' && *cursor <= '9')
				temp |= *cursor + 0x04;
			else if (*cursor == '+')
				temp |= 0x3E; // change to 0x2D for URL alphabet
			else if (*cursor == '/')
				temp |= 0x3F; // change to 0x5F for URL alphabet
			else if (*cursor == PADDING_CHAR) // pad
			{
				switch (input.end() - cursor)
				{
				case 1: // One pad character
					decodedBytes.push_back((temp >> 16) & 0x000000FF);
					decodedBytes.push_back((temp >> 8) & 0x000000FF);
					return true;

				case 2: // Two pad characters
					decodedBytes.push_back((temp >> 10) & 0x000000FF);
					return true;

				default:
					return false;
				}
			}
			else
			{
				return false;
			}

			cursor++;
		}

		decodedBytes.push_back((temp >> 16) & 0x000000FF);
		decodedBytes.push_back((temp >> 8) & 0x000000FF);
		decodedBytes.push_back((temp)&0x000000FF);
	}

	return true;
}