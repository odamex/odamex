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

#include "odamex.h"

#include "cl_cheat.h"

#include "am_map.h"
#include "c_dispatch.h"
#include "cl_main.h"
#include "g_gametype.h"
#include "gstrings.h"
#include "m_cheat.h"
#include "stringenums.h"

extern bool automapactive;

extern bool simulated_connection;
EXTERN_CVAR(sv_allowcheats)

void C_DoCommand(std::string_view cmd, uint32_t key = 0);

//
// CHEAT SEQUENCE PACKAGE
//

namespace cheat
{

//-------------
// Smashing Pumpkins Into Small Piles Of Putrid Debris.
bool AutoMap(cheatseq_t* cheat)
{
	if (automapactive)
	{
		if (not multiplayer
		    or G_IsCoopGame()
		    or netdemo.isInPlayback())
		{
			am_cheating = (am_cheating + 1) % 3;
		}

		return true;
	}
	return false;
}

bool ChangeLevel(cheatseq_t* cheat)
{
	std::string buf;

	// What were you trying to achieve?
	if (multiplayer)
		return false;

	// [ML] Chex mode: always set the episode number to 1.
	// FIXME: This is probably a horrible hack, it sure looks like one at least
	// And why is there only a newline for non-chex?
	if (gamemode == retail_chex)
		buf = fmt::format("map 1{:c}", cheat->Args[1]);
	else
		buf = fmt::format("map {:c}{:c}\n", cheat->Args[0], cheat->Args[1]);

	AddCommandString(buf);
	return true;
}

bool IdMyPos(cheatseq_t* cheat)
{
	C_DoCommand("toggle idmypos", 0);
	return true;
}

bool BeholdMenu(cheatseq_t* cheat)
{
	PrintFmt(PRINT_HIGH, "{}\n", GStrings(STSTR_BEHOLD));
	return false;
}

bool ChangeMusic(cheatseq_t* cheat)
{
	char buf[9] = "idmus xx";

	buf[6] = cheat->Args[0];
	buf[7] = cheat->Args[1];
	C_DoCommand(buf, 0);
	return true;
}

//
// Sets clientside the new cheat flag
// and also requests its new status serverside
//
bool SetGeneric(cheatseq_t* cheat)
{
	if (!AreCheatsEnabled())
		return true;

	if (cheat->Args[0] == CHT_NOCLIP)
	{
		if (cheat->Args[1] == 0 && gamemode != shareware && gamemode != registered &&
		    gamemode != retail && gamemode != retail_bfg)
			return true;
		else if (cheat->Args[1] == 1 && gamemode != commercial &&
		         gamemode != commercial_bfg)
			return true;
	}

	DoCheat(consoleplayer(), static_cast<CheatEnum>(cheat->Args[0]));
	CL_SendCheat(static_cast<CheatEnum>(cheat->Args[0]));

	return true;
}

// [RH] Actually handle the cheat. The cheat code in st_stuff.c now just
// writes some bytes to the network data stream, and the network code
// later calls us.

bool AddKey(cheatseq_t* cheat, unsigned char key, bool* eat)
{
	if (cheat->Pos == nullptr)
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
	}
	if (*cheat->Pos == 0)
	{
		*eat = true;
		cheat->Args[cheat->CurrentArg++] = key;
		cheat->Pos++;
	}
	else if (key == *cheat->Pos)
	{
		cheat->Pos++;
	}
	else
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
	}
	if (*cheat->Pos == 0xff)
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
		return true;
	}
	return false;
}

} // namespace cheat

BEGIN_COMMAND(tntem)
{
	if (!cheat::AreCheatsEnabled())
		return;

	if (multiplayer && !G_IsCoopGame())
		return;

	cheat::DoCheat(consoleplayer(), CHT_MASSACRE);
	CL_SendCheat(CHT_MASSACRE);
}
END_COMMAND(tntem)

BEGIN_COMMAND(summon)
{
	if (!cheat::AreCheatsEnabled())
		return;

	if (argc < 2)
		return;

	const std::string mobname = C_ArgCombine(argc - 1, const_cast<const char**>(argv + 1));

	if (!cheat::ValidSummonActor(mobname))
	{
		PrintFmt(PRINT_HIGH, "Invalid summon argument: {}. Please use `dumpactors` for a valid list of actor names.\n", mobname);
		return;
	}

	cheat::Summon(consoleplayer(), mobname, false);
	CL_SendSummonCheat(mobname.c_str());
 }
END_COMMAND(summon)

BEGIN_COMMAND(summonfriend)
{
	if (!cheat::AreCheatsEnabled())
		return;

	if (argc < 2)
		return;

	const std::string mobname = C_ArgCombine(argc - 1, const_cast<const char**>(argv + 1));

	if (!cheat::ValidSummonActor(mobname.c_str()))
	{
		PrintFmt(PRINT_HIGH,
		         "Invalid summon argument: {}. Please use `dumpactors` for a valid list of "
		         "actor names.\n",
		         mobname);
		return;
	}

	cheat::Summon(consoleplayer(), mobname.c_str(), true);
	CL_SendSummonFriendCheat(mobname.c_str());
}
END_COMMAND(summonfriend)

BEGIN_COMMAND(mdk)
{
	if (!cheat::AreCheatsEnabled())
		return;

	if (multiplayer && !G_IsCoopGame())
		return;

	cheat::DoCheat(consoleplayer(), CHT_MDK);
	CL_SendCheat(CHT_MDK);
}
END_COMMAND(mdk)
