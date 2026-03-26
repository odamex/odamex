// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
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
//   Options menu value sets.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "m_options_valuesets.h"

#include <array>

value_t YesNo[2] = {
	{0.0, "No"},
	{1.0, "Yes"},
};

value_t NoYes[2] = {
	{0.0, "Yes"},
	{1.0, "No"},
};

value_t OnOff[2] = {
	{0.0, "Off"},
	{1.0, "On"},
};

value_t HideShow[2] = {
	{0.0, "Hide"},
	{1.0, "Show"},
};

value_t OffOn[2] = {
	{0.0, "On"},
	{1.0, "Off"},
};

value_t OnOffAuto[3] = {
	{0.0, "Off"},
	{1.0, "On"},
	{2.0, "Auto"},
};

value_t DemoRestrictions[2] = {
	{0.0, "Restrict"},
	{1.0, "Allow"},
};

value_t DoomOrOdamex[2] = {
	{0.0, "Odamex"},
	{1.0, "Doom"},
};

static value_t MusSys[] = {
	{1.0, "libADLMIDI (OPL3 FM)"},
	{3.0, "PortMidi"},
};

static value_t MidiReset[] = {
	{0.0, "None"},
	{1.0, "GM"},
	{2.0, "GS"},
	{3.0, "XG"},
};

static value_t OplCore[] = {
	{0.0, "Fast (Dosbox)"},
	{1.0, "Balanced (Nuked 1.74)"},
	{2.0, "Accurate (Nuked 1.8)"},
};

static value_t OplBank[] = {
	{0.0, "Doom"},
	{1.0, "Doom II"},
	{2.0, "DMXOPL3"},
};

static value_t VoxType[] = {
	{0.0, "Off"},
	{1.0, "Team Colors"},
	{2.0, "Possessive"},
};

static value_t ChatSndType[] = {
	{0.0, "Disabled"},
	{1.0, "Enabled"},
	{2.0, "Teamchat only"},
};

static value_t WeapSwitch[] = {
	{0.0, "Never"},
	{1.0, "Always"},
	{2.0, "By Preference"},
	{3.0, "Attack Cancels PWO"},
};

static value_t Wipes[] = {
	{0.0, "None"},
	{1.0, "Melt"},
	{2.0, "Burn"},
	{3.0, "Crossfade"},
	{4.0, "Auto"},
};

static value_t Endoom[] = {
	{0.0, "Off"},
	{1.0, "On"},
	{2.0, "PWAD Only"},
};

static value_t SecretOptions[] = {
	{0.0, "Off"},
	{1.0, "On (with sounds)"},
	{2.0, "On (w/o sounds)"},
	{3.0, "Own only"},
};

static value_t TimerStyles[] = {
	{0.0, "No Timer"},
	{1.0, "Count Down"},
	{2.0, "Count Up"},
};

static value_t FlagHelds[] = {
	{0.0, "Off"},
	{1.0, "Complete"},
	{2.0, "Simple"},
};

static value_t Crosshairs[] = {
	{0.0, "None"},
	{1.0, "Cross 1"},
	{2.0, "Cross 2"},
	{3.0, "X"},
	{4.0, "Diamond"},
	{5.0, "Dot"},
	{6.0, "Box"},
	{7.0, "Angle"},
	{8.0, "Big Thing"},
};

static value_t ExtendedHudStyles[] = {
	{0.0, "Off"},
	{1.0, "Horizontal 1"},
	{2.0, "Horizontal 2"},
	{3.0, "Vertical 1"},
	{4.0, "Vertical 2"},
};

static value_t TextColors[] = {
	{CR_BRICK, "brick"},
	{CR_TAN, "tan"},
	{CR_GRAY, "gray"},
	{CR_GREEN, "green"},
	{CR_BROWN, "brown"},
	{CR_GOLD, "gold"},
	{CR_RED, "red"},
	{CR_BLUE, "blue"},
	{CR_ORANGE, "orange"},
	{CR_WHITE, "white"},
	{CR_YELLOW, "yellow"},
	{CR_BLACK, "black"},
	{CR_LIGHTBLUE, "light blue"},
	{CR_CREAM, "cream"},
	{CR_OLIVE, "olive"},
	{CR_DARKGREEN, "dark green"},
	{CR_DARKRED, "dark red"},
	{CR_DARKBROWN, "dark brown"},
	{CR_PURPLE, "purple"},
	{CR_DARKGRAY, "dark gray"},
	{CR_CYAN, "cyan"},
};

static value_t ScaleFactors[] = {
	{0.0, "Auto"},
	{1.0, "1X"},
	{2.0, "2X"},
	{3.0, "3X"},
	{4.0, "4X"},
	{5.0, "5X"},
};

static value_t ClassicMapStringTypes[] = {
	{0.0, "Odamex"},
	{1.0, "Classic"},
};

static value_t AutomapScales[] = {
	{0.0, "Auto"},
	{1.0, "1X"},
	{2.0, "2X"},
	{3.0, "3X"},
	{4.0, "4X"},
	{5.0, "5X"},
	{6.0, "6X"},
};

static value_t MinimapLocations[] = {
	{0.0, "Left Top"},
	{1.0, "Left Middle"},
	{2.0, "Left Bottom"},
	{3.0, "Right Top"},
	{4.0, "Right Middle"},
	{5.0, "Right Bottom"},
};

static value_t Overlays[] = {
	{0.0, "Off"},
	{1.0, "Standard"},
	{2.0, "Full"},
	{3.0, "Full Only"},
};

namespace
{
	struct optionsvalueset_t
	{
		std::string_view id;
		value_t* values;
		int count;
	};

	const std::array optionsValueSets = {
		optionsvalueset_t{"OnOff", OnOff, static_cast<int>(ARRAY_LENGTH(OnOff))},
		optionsvalueset_t{"YesNo", YesNo, static_cast<int>(ARRAY_LENGTH(YesNo))},
		optionsvalueset_t{"OffOn", OffOn, static_cast<int>(ARRAY_LENGTH(OffOn))},
		optionsvalueset_t{"HideShow", HideShow, static_cast<int>(ARRAY_LENGTH(HideShow))},
		optionsvalueset_t{"OnOffAuto", OnOffAuto, static_cast<int>(ARRAY_LENGTH(OnOffAuto))},
		optionsvalueset_t{"DemoRestrictions", DemoRestrictions,
		                  static_cast<int>(ARRAY_LENGTH(DemoRestrictions))},
		optionsvalueset_t{"DoomOrOdamex", DoomOrOdamex,
		                  static_cast<int>(ARRAY_LENGTH(DoomOrOdamex))},
		optionsvalueset_t{"MusSys", MusSys, static_cast<int>(ARRAY_LENGTH(MusSys))},
		optionsvalueset_t{"MidiReset", MidiReset, static_cast<int>(ARRAY_LENGTH(MidiReset))},
		optionsvalueset_t{"OplCore", OplCore, static_cast<int>(ARRAY_LENGTH(OplCore))},
		optionsvalueset_t{"OplBank", OplBank, static_cast<int>(ARRAY_LENGTH(OplBank))},
		optionsvalueset_t{"VoxType", VoxType, static_cast<int>(ARRAY_LENGTH(VoxType))},
		optionsvalueset_t{"ChatSndType", ChatSndType, static_cast<int>(ARRAY_LENGTH(ChatSndType))},
		optionsvalueset_t{"WeapSwitch", WeapSwitch, static_cast<int>(ARRAY_LENGTH(WeapSwitch))},
		optionsvalueset_t{"Wipes", Wipes, static_cast<int>(ARRAY_LENGTH(Wipes))},
		optionsvalueset_t{"Endoom", Endoom, static_cast<int>(ARRAY_LENGTH(Endoom))},
		optionsvalueset_t{"SecretOptions", SecretOptions,
		                  static_cast<int>(ARRAY_LENGTH(SecretOptions))},
		optionsvalueset_t{"TimerStyles", TimerStyles, static_cast<int>(ARRAY_LENGTH(TimerStyles))},
		optionsvalueset_t{"FlagHelds", FlagHelds, static_cast<int>(ARRAY_LENGTH(FlagHelds))},
		optionsvalueset_t{"Crosshairs", Crosshairs, static_cast<int>(ARRAY_LENGTH(Crosshairs))},
		optionsvalueset_t{"ExtendedHudStyles", ExtendedHudStyles,
		                  static_cast<int>(ARRAY_LENGTH(ExtendedHudStyles))},
		optionsvalueset_t{"TextColors", TextColors, static_cast<int>(ARRAY_LENGTH(TextColors))},
		optionsvalueset_t{"ScaleFactors", ScaleFactors,
		                  static_cast<int>(ARRAY_LENGTH(ScaleFactors))},
		optionsvalueset_t{"ClassicMapStringTypes", ClassicMapStringTypes,
		                  static_cast<int>(ARRAY_LENGTH(ClassicMapStringTypes))},
		optionsvalueset_t{"AutomapScales", AutomapScales,
		                  static_cast<int>(ARRAY_LENGTH(AutomapScales))},
		optionsvalueset_t{"MinimapLocations", MinimapLocations,
		                  static_cast<int>(ARRAY_LENGTH(MinimapLocations))},
		optionsvalueset_t{"Overlays", Overlays, static_cast<int>(ARRAY_LENGTH(Overlays))},
	};
}

value_t* M_OptionValueSet(std::string_view name, int& count)
{
	for (const optionsvalueset_t& set : optionsValueSets)
	{
		if (set.id == name)
		{
			count = set.count;
			return set.values;
		}
	}

	count = 0;
	return nullptr;
}
