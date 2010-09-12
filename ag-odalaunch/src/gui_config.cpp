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

void GuiConfig::Unset(std::string option)
{
	if(option.size())
		AG_Unset(agConfig, option.c_str());
}

int GuiConfig::Write(std::string option, std::string value)
{
	if(!option.size() || !value.size())
		return -1;

	if(AG_SetString(agConfig, option.c_str(), value.c_str()) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(std::string option, int8_t value)
{
	if(!option.size())
		return -1;

	if(AG_SetSint8(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(std::string option, int16_t value)
{
	if(!option.size())
		return -1;

	if(AG_SetSint16(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(std::string option, int32_t value)
{
	if(!option.size())
		return -1;

	if(AG_SetSint32(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(std::string option, uint8_t value)
{
	if(!option.size())
		return -1;

	if(AG_SetUint8(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(std::string option, uint16_t value)
{
	if(!option.size())
		return -1;

	if(AG_SetUint16(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(std::string option, uint32_t value)
{
	if(!option.size())
		return -1;

	if(AG_SetUint32(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(std::string option, float value)
{
	if(!option.size())
		return -1;

	if(AG_SetFloat(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Write(std::string option, double value)
{
	if(!option.size())
		return -1;

	if(AG_SetDouble(agConfig, option.c_str(), value) == NULL)
		return -1;

	return 0;
}

int GuiConfig::Read(std::string option, std::string &value)
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

int GuiConfig::Read(std::string option, int8_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetSint8(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(std::string option, int16_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetSint16(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(std::string option, int32_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetSint32(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(std::string option, uint8_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetUint8(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(std::string option, uint16_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetUint16(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(std::string option, uint32_t &value)
{
	if(!option.size())
		return -1;

	value = AG_GetUint32(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(std::string option, float &value)
{
	if(!option.size())
		return -1;

	value = AG_GetFloat(agConfig, option.c_str());

	return 0;
}

int GuiConfig::Read(std::string option, double &value)
{
	if(!option.size())
		return -1;

	value = AG_GetDouble(agConfig, option.c_str());

	return 0;
}

