// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
//  A collection of strongly typed hash types.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "ohash.h"

/**
 * @brief Populate an OMD5Hash with the passed hex string.
 *
 * @param out FileHash to populate.
 * @param hash Hex string to populate with.
 * @return True if the hash was populated, otherwise false.
 */
bool OMD5Hash::makeFromHexStr(OMD5Hash& out, const std::string& hash)
{
	if (IsHexString(hash, 32))
	{
		out.m_hash = StdStringToUpper(hash);
		return true;
	}

	return false;
}

/**
 * @brief Populate an OCRC32Hash with the passed hex string.
 *
 * @param out FileHash to populate.
 * @param hash Hex string to populate with.
 * @return True if the hash was populated, otherwise false.
 */
bool OCRC32Sum::makeFromHexStr(OCRC32Sum& out, const std::string& hash)
{
	if (IsHexString(hash, 8))
	{
		out.m_hash = StdStringToUpper(hash);
		return true;
	}

	return false;
}
