// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#ifdef UNIX
#include <ctype.h>
#ifndef O_BINARY
#define O_BINARY		0
#endif
#endif

#ifdef _WIN32
#include <io.h>
#else
#define strcmpi	strcasecmp
#endif

#include <fcntl.h>

#include "crc32.h"

#include "m_fileio.h"
#include "i_system.h"
#include "z_zone.h"
#include "cmdlib.h"
#include "m_argv.h"
#include "md5.h"

#include "farmhash.h"

#include "w_wad.h"

#include <sstream>
#include <algorithm>
#include <iomanip>


//
// W_ReadChunk
//
// denis - for wad downloading
//
unsigned W_ReadChunk(const char* file, unsigned offs, unsigned len, void* dest, unsigned& filelen)
{
	FILE* fp = fopen(file, "rb");
	unsigned read = 0;

	if (fp)
	{
		filelen = M_FileLength(fp);

		fseek(fp, offs, SEEK_SET);
		read = fread(dest, 1, len, fp);
		fclose(fp);
	}
	else filelen = 0;

	return read;
}

/*
 * @brief Creates a 128-bit fingerprint for a map via FarmHash.
 *
 * However, it encodes the fingerprint into a 16-byte array to be read later.
 *
 * @param lumpdata byte array pointer to the lump (or lumps) that needs to be
 * fingerprinted.
 * @param size of the byte array pointer in bytes.
 * @return fhfprint_s - struct containing 16-byte array of fingerprint.
 */
fhfprint_s W_FarmHash128(const byte* lumpdata, int length)
{
	fhfprint_s fhfngprnt;

	if (!lumpdata)
		return fhfngprnt;

	util::uint128_t fingerprint128 = util::Fingerprint128((const char*)lumpdata, length);

	// Store the bytes of the hashes in the array.
	fhfngprnt.fingerprint[0] = fingerprint128.first >> 8 * 0;
	fhfngprnt.fingerprint[1] = fingerprint128.first >> 8 * 1;
	fhfngprnt.fingerprint[2] = fingerprint128.first >> 8 * 2;
	fhfngprnt.fingerprint[3] = fingerprint128.first >> 8 * 3;
	fhfngprnt.fingerprint[4] = fingerprint128.first >> 8 * 4;
	fhfngprnt.fingerprint[5] = fingerprint128.first >> 8 * 5;
	fhfngprnt.fingerprint[6] = fingerprint128.first >> 8 * 6;
	fhfngprnt.fingerprint[7] = fingerprint128.first >> 8 * 7;

	fhfngprnt.fingerprint[8] = fingerprint128.second >> 8 * 0;
	fhfngprnt.fingerprint[9] = fingerprint128.second >> 8 * 1;
	fhfngprnt.fingerprint[10] = fingerprint128.second >> 8 * 2;
	fhfngprnt.fingerprint[11] = fingerprint128.second >> 8 * 3;
	fhfngprnt.fingerprint[12] = fingerprint128.second >> 8 * 4;
	fhfngprnt.fingerprint[13] = fingerprint128.second >> 8 * 5;
	fhfngprnt.fingerprint[14] = fingerprint128.second >> 8 * 6;
	fhfngprnt.fingerprint[15] = fingerprint128.second >> 8 * 7;

	return fhfngprnt;
}

// denis - Standard MD5SUM
OMD5Hash W_MD5(const std::string& filename)
{
	OMD5Hash rvo;

	const int file_chunk_size = 8192;
	FILE* fp = fopen(filename.c_str(), "rb");

	if (!fp)
		return rvo;

	md5_state_t state;
	md5_init(&state);

	unsigned n = 0;
	unsigned char buf[file_chunk_size];

	while ((n = fread(buf, 1, sizeof(buf), fp)))
		md5_append(&state, (unsigned char*)buf, n);

	md5_byte_t digest[16];
	md5_finish(&state, digest);

	fclose(fp);

	std::stringstream hashStr;

	for (int i = 0; i < 16; i++)
		hashStr << std::setw(2) << std::setfill('0') << std::hex << std::uppercase
		        << (short)digest[i];

	OMD5Hash::makeFromHexStr(rvo, hashStr.str());
	return rvo; // bubble up failure
}



VERSION_CONTROL (w_wad_cpp, "$Id$")
