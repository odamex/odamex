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

#pragma once

#include <string>

struct UICommand
{
	enum Type
	{
		CMD_NONE = 0,
		CMD_NEW_GAME,       // str = map name
		CMD_LOAD_GAME,      // str = savegame name
		CMD_SAVE_GAME,      // slot + str = description
		CMD_END_GAME,
		CMD_FINALE_ADVANCE,
	};

	UICommand() : type(CMD_NONE), slot(0) {}
	explicit UICommand(Type t) : type(t), slot(0) {}

	Type type;
	int slot;
	std::string str;
};

// Post a command from a UI layer.
void UI_PostCommand(const UICommand& cmd);

// Execute and clear everything posted since the last drain.
void UI_DrainCommands();
