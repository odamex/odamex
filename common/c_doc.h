// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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
//   Dumps a document with a list of all cvars
//
//-----------------------------------------------------------------------------

#pragma once

// Where a generated document should be sent.
enum cvardocdest_t
{
	CVARDOC_FILE,
	CVARDOC_STDOUT,
};

void C_WriteCvarDoc(cvardocdest_t dest);
void C_WriteCvarDocJSON(cvardocdest_t dest);
