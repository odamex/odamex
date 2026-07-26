// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
//   UI to game scene command channel.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "ui/ui_command.h"

#include <vector>

#include "cl_main.h"
#include "d_event.h"
#include "g_game.h"
#include "g_level.h"

static std::vector<UICommand> ui_commands;

void UI_PostCommand(const UICommand& cmd)
{
	ui_commands.push_back(cmd);
}

static void UI_ExecCommand(const UICommand& cmd)
{
	switch (cmd.type)
	{
	case UICommand::CMD_NEW_GAME:
		G_DeferedInitNew(OLumpName(cmd.str));
		break;

	case UICommand::CMD_LOAD_GAME:
		G_LoadGame(cmd.str);
		break;

	case UICommand::CMD_SAVE_GAME:
		G_SaveGame(cmd.slot, cmd.str);
		break;

	case UICommand::CMD_END_GAME:
		CL_QuitNetGame(NQ_SILENT);
		break;

	case UICommand::CMD_FINALE_ADVANCE:
		gameaction = ga_worlddone;
		break;

	case UICommand::CMD_NONE:
	default:
		break;
	}
}

void UI_DrainCommands()
{
	if (ui_commands.empty())
		return;

	// Swap the list first: executing a command may post another one,
	// and that should land on the next drain rather than extend this loop.
	std::vector<UICommand> pending;
	pending.swap(ui_commands);

	for (size_t i = 0; i < pending.size(); i++)
		UI_ExecCommand(pending[i]);
}
