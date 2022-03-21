// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//   Intermissions.
// 
//-----------------------------------------------------------------------------

#pragma once
#include "oscanner.h"
#include "s_sound.h" // MAX_SNDNAME

struct OIntermissionPatch
{
	std::string condition;
	OLumpName name;
	int x;
	int y;
};

struct OCastSound
{
	uint8_t sequence;
	uint8_t index;
	char sound[MAX_SNDNAME + 1];
};

struct OIntermissionAction
{
	OLumpName music;
	int musicOrder;
	bool musicLooping;
	int duration;
	OLumpName background;
	bool flatFill;
	OLumpName backgroundPalette;
	char sound[MAX_SNDNAME + 1];
	std::vector<OIntermissionPatch> overlays;

	OIntermissionAction();
	virtual ~OIntermissionAction();
	virtual bool ParseKey(OScanner& os);
};

struct OIntermissionActionFader : OIntermissionAction
{
	enum FadeType
	{
		FadeIn,
		FadeOut,
	};

	FadeType fadeType;

	OIntermissionActionFader();
	bool ParseKey(OScanner& os);
};

struct OIntermissionActionWiper : OIntermissionAction
{
	gamestate_t wipeType;

	OIntermissionActionWiper();
	bool ParseKey(OScanner& os);
};

struct OIntermissionActionTextscreen : OIntermissionAction
{
	std::string text;
	int textDelay;
	int textSpeed;
	int textX;
	int textY;

	OIntermissionActionTextscreen();
	bool ParseKey(OScanner& os);
};

struct OIntermissionActionCast : OIntermissionAction
{
	std::string name;
	std::string castClass;
	std::vector<OCastSound> castSounds;

	OIntermissionActionCast();
	bool ParseKey(OScanner& os);
};

struct OIntermissionActionScroller : OIntermissionAction
{
	enum SrollType
	{
		ScrollLeft,
		ScrollRight,
		ScrollUp,
		ScrollDown,
	};

	OLumpName secondBackground;
	int scrollDelay;
	int scrollTime;
	int scrollDir;

	OIntermissionActionScroller();
	bool ParseKey(OScanner& os);
};

void ParseIntermissionInfo(OScanner& os);
