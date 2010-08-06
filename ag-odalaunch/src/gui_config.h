// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: game_command.h 1696 2010-08-05 03:16:10Z hypereye $
//
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	Gui Configuration
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include "typedefs.h"

#ifndef _GUI_CONFIG_H
#define _GUI_CONFIG_H

class GuiConfig
{
public:
	GuiConfig();
	~GuiConfig();

	static int Save();

	static int Write(std::string option, std::string value);
	static int Write(std::string option, int8_t value);
	static int Write(std::string option, int16_t value);
	static int Write(std::string option, int32_t value);
	static int Write(std::string option, uint8_t value);
	static int Write(std::string option, uint16_t value);
	static int Write(std::string option, uint32_t value);
	static int Write(std::string option, float value);
	static int Write(std::string option, double value);

	static int Read(std::string option, std::string &value);
	static int Read(std::string option, int8_t &value);
	static int Read(std::string option, int16_t &value);
	static int Read(std::string option, int32_t &value);
	static int Read(std::string option, uint8_t &value);
	static int Read(std::string option, uint16_t &value);
	static int Read(std::string option, uint32_t &value);
	static int Read(std::string option, float &value);
	static int Read(std::string option, double &value);
};

#endif
