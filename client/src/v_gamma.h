// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: v_palette.cpp 3798 2013-04-24 03:09:33Z dr_sean $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	Gamma correction behavior for the Doom and ZDoom gamma types
//
//-----------------------------------------------------------------------------

#ifndef __V_GAMMA_H__
#define __V_GAMMA_H__

#include "doomtype.h"
#include <math.h>

enum {
	GAMMA_DOOM = 0,
	GAMMA_ZDOOM = 1
};

//
// GammaStrategy
//
// Encapsulate the differences of the Doom and ZDoom gamma types with
// a strategy pattern. Provides a common interface for generation of gamma
// tables.
//
class GammaStrategy
{
public:
	virtual float min() const = 0;
	virtual float max() const = 0;
	virtual float increment(float level) const = 0;
	virtual void generateGammaTable(byte* table, float level) const = 0;
};

class DoomGammaStrategy : public GammaStrategy
{
public:
	float min() const
	{
		return 0.0f;
	}

	float max() const
	{
		return 7.0f;
	}

	float increment(float level) const
	{
		level += 1.0f;
		if (level > max())
			level = min();
		return level;
	}

	void generateGammaTable(byte* table, float level) const
	{
		// [SL] Use vanilla Doom's gamma table
		//
		// This was derived from the original Doom gammatable after some
		// trial and error and several beers.  The +0.5 is used to round
		// while the 255/256 is to scale to ensure 255 isn't exceeded.
		// This generates a 1:1 match with the original gammatable but also
		// allows for intermediate values.

		const double basefac = pow(2.0, level) * (255.0/256.0);
		const double exp = 1.0 - 0.125 * level;

		for (int i = 0; i < 256; i++)
			table[i] = (byte)(0.5 + basefac * pow(double(i) + 1.0, exp));
	}
};

class ZDoomGammaStrategy : public GammaStrategy
{
public:
	float min() const
	{
		return 0.5f;
	}

	float max() const
	{
		return 3.0f;
	}

	float increment(float level) const
	{
		level += 0.1f;
		if (level > max())
			level = min();
		return level;
	}

	void generateGammaTable(byte* table, float level) const
	{
		// [SL] Use ZDoom 1.22 gamma correction

		// [RH] I found this formula on the web at
		// http://panda.mostang.com/sane/sane-gamma.html

		double invgamma = 1.0 / level;

		for (int i = 0; i < 256; i++)
			table[i] = (byte)(255.0 * pow(double(i) / 255.0, invgamma));
	}
};

#endif	// #define __V_GAMMA_H__

