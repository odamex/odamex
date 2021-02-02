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
//
// Resource file identification
//
//-----------------------------------------------------------------------------

#ifndef __W_IDENT_H__
#define __W_IDENT_H__

#include <string>

#include "m_ostring.h"
#include "m_resfile.h"

void W_SetupFileIdentifiers();
void W_ConfigureGameInfo(const OResFile& iwad);
bool W_IsKnownIWAD(const OWantFile& file);
bool W_IsIWAD(const OResFile& file);
bool W_IsFilenameCommercialIWAD(const std::string& filename);
bool W_IsFilehashCommercialIWAD(const std::string& filename);
bool W_IsFileCommercialIWAD(const std::string& filename);
bool W_IsIWADDeprecated(const OResFile& file);
std::vector<OString> W_GetIWADFilenames();

#endif // __W_IDENT_H__
