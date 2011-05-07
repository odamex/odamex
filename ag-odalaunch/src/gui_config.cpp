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

#include <iostream>
#include <string>

#include <agar/core.h>

#include "gui_config.h"

using namespace std;

namespace agOdalaunch {

GuiConfig::GuiConfig()
{

}

GuiConfig::~GuiConfig()
{
	Save();
}

int GuiConfig::Save()
{
	return AG_ConfigSave();
}

void GuiConfig::Unset(const string &option)
{
	if(option.size())
		AG_Unset(agConfig, option.c_str());
}

int GuiConfig::Write(const string &option, const string &value)
{
	if(!option.size() || !value.size())
		return -1;

	if(AG_SetString(agConfig, option.c_str(), value.c_str()) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(const string &option, const int8_t &value)
{
	if(!option.size())
		return -1;

	if(AG_SetSint8(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(const string &option, const int16_t &value)
{
	if(!option.size())
		return -1;

	if(AG_SetSint16(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(const string &option, const int32_t &value)
{
	if(!option.size())
		return -1;

	if(AG_SetSint32(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(const string &option, const uint8_t &value)
{
	if(!option.size())
		return -1;

	if(AG_SetUint8(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(const string &option, const uint16_t &value)
{
	if(!option.size())
		return -1;

	if(AG_SetUint16(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(const string &option, const uint32_t &value)
{
	if(!option.size())
		return -1;

	if(AG_SetUint32(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(const string &option, const float &value)
{
	if(!option.size())
		return -1;

	if(AG_SetFloat(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(const string &option, const double &value)
{
	if(!option.size())
		return -1;

	if(AG_SetDouble(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Read(const string &option, string &value)
{
	char *str = NULL;

	if(!option.size())
		return -1;

	if((str = AG_GetStringDup(agConfig, option.c_str())) == NULL)
		return -1;

	value = str;

	free(str);

	return 0;
}

int GuiConfig::Read(const string &option, int8_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetSint8(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(const string &option, int16_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetSint16(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(const string &option, int32_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetSint32(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(const string &option, uint8_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetUint8(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(const string &option, uint16_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetUint16(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(const string &option, uint32_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetUint32(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(const string &option, float &value)
{
	if(!option.size())
		return -1;

	value = AG_GetFloat(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(const string &option, double &value)
{
	if(!option.size())
		return -1;

	value = AG_GetDouble(agConfig, option.c_str());

	return 0;
}

} // namespace
