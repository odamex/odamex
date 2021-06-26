// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Multiplayer properties (?)
//
//-----------------------------------------------------------------------------


#ifndef __D_NETINFO_H__
#define __D_NETINFO_H__

#include "doomdef.h"
#include "cmdlib.h"
#include "c_cvars.h"
#include "teaminfo.h"

#include "base64.h"

#define MAXPLAYERNAME	15

class picon_t
{
  public:
	static const size_t PICON_STRIDE = 7;
	static const size_t PICON_PIXELS = (PICON_STRIDE * PICON_STRIDE);

  private:
	static const size_t PICON_HEADER_BITS = 4;
	static const size_t PICON_HEADER_MASK = 0x0F;

	bool m_data[PICON_PIXELS];
	graphic_t m_graphic;

  public:
	picon_t()
	{
		ArrayInit(m_data, false);
	}

	bool& at(const int x, const int y)
	{
		return m_data[y * PICON_STRIDE + x];
	}

	const graphic_t& getGraphic(const palindex_t fg, const palindex_t bg)
	{
		m_graphic.width = PICON_STRIDE;
		m_graphic.height = PICON_STRIDE;
		m_graphic.data.resize(PICON_PIXELS);
		for (size_t i = 0; i < ARRAY_LENGTH(m_data); i++)
		{
			m_graphic.data[i] = m_data[i] ? fg : bg;
		}
		return m_graphic;
	}

	bool fromString(const std::string& str)
	{
		std::vector<byte> data;
		if (!M_Base64Decode(data, str) || data.size() != 7)
		{
			return false;
		}

		// "I" indicates a 7x7 icon.
		if ((data.at(0) & PICON_HEADER_MASK) != 0x08)
		{
			return false;
		}

		for (size_t i = 0; i < PICON_PIXELS; i++)
		{
			size_t iPlus = i + PICON_HEADER_BITS;
			size_t dataI = iPlus / 8;
			size_t dataBit = iPlus % 8;

			if (data.at(dataI) & BIT(dataBit))
			{
				m_data[i] = true;
			}
			else
			{
				m_data[i] = false;
			}
		}

		return true;
	}

	std::string toString()
	{
		std::vector<byte> data;

		// First four bits is format.  We use a nibble instead of a byte to
		// save a byte later (and thus construct a shorter base64 token).
		byte scratch = 0x08;
		for (size_t i = 0; i < ARRAY_LENGTH(m_data); i++)
		{
			// Start writing bytes after the first nibble.
			size_t bit = (i + PICON_HEADER_BITS) % 8;

			if (bit == 0 && i != 0)
			{
				// Byte is done, go to the next one.
				data.push_back(scratch);
				scratch = 0;
			}

			if (m_data[i])
			{
				scratch |= BIT(bit);
			}
		}

		// Push the last byte.
		data.push_back(scratch);

		return M_Base64Encode(data);
	}
};

enum gender_t
{
	GENDER_MALE,
	GENDER_FEMALE,
	GENDER_NEUTER,
	
	NUMGENDER
};

enum weaponswitch_t
{
	WPSW_NEVER,
	WPSW_ALWAYS,
	WPSW_PWO,
	WPSW_PWO_ALT,	// PWO but never switch if holding +attack

	WPSW_NUMTYPES
};

struct UserInfo
{
	std::string		netname;
	team_t			team; // [Toke - Teams]
	fixed_t			aimdist;
	bool			predict_weapons;
	byte			color[4];
	gender_t		gender;
	weaponswitch_t	switchweapon;
	byte			weapon_prefs[NUMWEAPONS];
	picon_t			icon;
	byte			icon_color[4];

	static const byte weapon_prefs_default[NUMWEAPONS];

	UserInfo()
	    : team(TEAM_NONE), aimdist(0), predict_weapons(true), gender(GENDER_MALE),
	      switchweapon(WPSW_ALWAYS), icon()
	{
		// default doom weapon ordering when player runs out of ammo
		ArrayCopy(weapon_prefs, UserInfo::weapon_prefs_default);
		ArrayInit(color, 0x00);
		ArrayInit(icon_color, 0x00);
	}
};

FArchive &operator<< (FArchive &arc, UserInfo &info);
FArchive &operator>> (FArchive &arc, UserInfo &info);

void D_SetupUserInfo (void);

void D_UserInfoChanged (cvar_t *info);

void D_SendServerInfoChange (const cvar_t *cvar, const char *value);
void D_DoServerInfoChange (byte **stream);

void D_WriteUserInfoStrings (int player, byte **stream, bool compact=false);
void D_ReadUserInfoStrings (int player, byte **stream, bool update);

#endif //__D_NETINFO_H__


