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
// Identifiying resource types
//
//-----------------------------------------------------------------------------

#ifndef __RES_IDENTIFIER_H__
#define __RES_IDENTIFIER_H__

#include <stdint.h>
#include "resources/res_resourcepath.h"

bool Res_ValidateWadData(const uint8_t* data, size_t length);
bool Res_ValidateDehackedData(const uint8_t* data, size_t length);
bool Res_ValidatePCSpeakerSoundData(const uint8_t* data, size_t length);
bool Res_ValidateSoundData(const uint8_t* data, size_t length);
bool Res_ValidatePatchData(const uint8_t* data, size_t length);

bool Res_MusicIsMus(const uint8_t* data, size_t length);
bool Res_MusicIsMidi(const uint8_t* data, size_t length);
bool Res_MusicIsOgg(const uint8_t* data, size_t length);
bool Res_MusicIsMp3(const uint8_t* data, size_t length);


// ============================================================================
//
// WadResourceIdentifer class interface
//
// ============================================================================

class WadResourceIdentifier
{
public:
	WadResourceIdentifier() {}

	const ResourcePath& identifyByContents(const ResourcePath& path, const uint8_t* data, size_t length) const;

private:
};

#endif // __RES_IDENTIFIER_H__

