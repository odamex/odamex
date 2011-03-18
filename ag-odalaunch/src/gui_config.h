// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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

#ifdef _WIN32
#define PATH_DELIMITER ';'
#else
#define PATH_DELIMITER ':'
#endif

class GuiConfig
{
public:
	GuiConfig();
	~GuiConfig();

	static int Save();

	static void Unset(const std::string &option);

	static int Write(const std::string &option, const std::string &value);
	static int Write(const std::string &option, const int8_t &value);
	static int Write(const std::string &option, const int16_t &value);
	static int Write(const std::string &option, const int32_t &value);
	static int Write(const std::string &option, const uint8_t &value);
	static int Write(const std::string &option, const uint16_t &value);
	static int Write(const std::string &option, const uint32_t &value);
	static int Write(const std::string &option, const float &value);
	static int Write(const std::string &option, const double &value);

	static int Read(const std::string &option, std::string &value);
	static int Read(const std::string &option, int8_t &value);
	static int Read(const std::string &option, int16_t &value);
	static int Read(const std::string &option, int32_t &value);
	static int Read(const std::string &option, uint8_t &value);
	static int Read(const std::string &option, uint16_t &value);
	static int Read(const std::string &option, uint32_t &value);
	static int Read(const std::string &option, float &value);
	static int Read(const std::string &option, double &value);
};

#endif
