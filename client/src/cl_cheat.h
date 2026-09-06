// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//  Client cheat sequence management
//
//-----------------------------------------------------------------------------

#pragma once

//
// CHEAT SEQUENCE PACKAGE
//

struct cheatseq_t
{
	unsigned char*  Sequence;
	unsigned char*  Pos;
	bool            DontCheck;
	bool            AllowInNetdemoPlayback;
	unsigned char   CurrentArg;
	unsigned char   Args[2];
	bool (*Handler)(cheatseq_t*);
};

namespace cheat
{
	// keycheat handlers
	bool AddKey(cheatseq_t* cheat, unsigned char key, bool* eat);

	bool AutoMap    (cheatseq_t* cheat);
	bool ChangeLevel(cheatseq_t* cheat);
	bool IdMyPos    (cheatseq_t* cheat);
	bool BeholdMenu (cheatseq_t* cheat);
	bool ChangeMusic(cheatseq_t* cheat);
	bool SetGeneric (cheatseq_t* cheat);
}
