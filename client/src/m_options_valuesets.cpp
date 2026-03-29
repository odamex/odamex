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

namespace
{
namespace valuesets
{
	value_t YesNo[] = {
		{0.0, "No"},
		{1.0, "Yes"},
	};

	value_t NoYes[] = {
		{0.0, "Yes"},
		{1.0, "No"},
	};

	value_t OnOff[] = {
		{0.0, "Off"},
		{1.0, "On"},
	};

	value_t HideShow[] = {
		{0.0, "Hide"},
		{1.0, "Show"},
	};

	value_t OffOn[] = {
		{0.0, "On"},
		{1.0, "Off"},
	};

	value_t OnOffAuto[] = {
		{0.0, "Off"},
		{1.0, "On"},
		{2.0, "Auto"},
	};

	value_t DemoRestrictions[] = {
		{0.0, "Restrict"},
		{1.0, "Allow"},
	};

	value_t DoomOrOdamex[] = {
		{0.0, "Odamex"},
		{1.0, "Doom"},
	};

	value_t MusSys[] = {
	{1.0, "libADLMIDI (OPL3 FM)"},
	{3.0, "PortMidi"},
	};

	value_t MidiReset[] = {
	{0.0, "None"},
	{1.0, "GM"},
	{2.0, "GS"},
	{3.0, "XG"},
	};

	value_t OplCore[] = {
	{0.0, "Fast (Dosbox)"},
	{1.0, "Balanced (Nuked 1.74)"},
	{2.0, "Accurate (Nuked 1.8)"},
	};

	value_t OplBank[] = {
	{0.0, "Doom"},
	{1.0, "Doom II"},
	{2.0, "DMXOPL3"},
	};

	value_t VoxType[] = {
	{0.0, "Off"},
	{1.0, "Team Colors"},
	{2.0, "Possessive"},
	};

	value_t ChatSndType[] = {
	{0.0, "Disabled"},
	{1.0, "Enabled"},
	{2.0, "Teamchat only"},
	};

	value_t WeapSwitch[] = {
	{0.0, "Never"},
	{1.0, "Always"},
	{2.0, "By Preference"},
	{3.0, "Attack Cancels PWO"},
	};

	value_t Wipes[] = {
	{0.0, "None"},
	{1.0, "Melt"},
	{2.0, "Burn"},
	{3.0, "Crossfade"},
	{4.0, "Auto"},
	};

	value_t Endoom[] = {
	{0.0, "Off"},
	{1.0, "On"},
	{2.0, "PWAD Only"},
	};

	value_t SecretOptions[] = {
	{0.0, "Off"},
	{1.0, "On (with sounds)"},
	{2.0, "On (w/o sounds)"},
	{3.0, "Own only"},
	};

	value_t TimerStyles[] = {
	{0.0, "No Timer"},
	{1.0, "Count Down"},
	{2.0, "Count Up"},
	};

	value_t FlagHelds[] = {
	{0.0, "Off"},
	{1.0, "Complete"},
	{2.0, "Simple"},
	};

	value_t Crosshairs[] = {
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

	value_t ExtendedHudStyles[] = {
	{0.0, "Off"},
	{1.0, "Horizontal 1"},
	{2.0, "Horizontal 2"},
	{3.0, "Vertical 1"},
	{4.0, "Vertical 2"},
	};

	value_t TextColors[] = {
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

	value_t ScaleFactors[] = {
	{0.0, "Auto"},
	{1.0, "1X"},
	{2.0, "2X"},
	{3.0, "3X"},
	{4.0, "4X"},
	{5.0, "5X"},
	};

	value_t ClassicMapStringTypes[] = {
	{0.0, "Odamex"},
	{1.0, "Classic"},
	};

	value_t AutomapScales[] = {
	{0.0, "Auto"},
	{1.0, "1X"},
	{2.0, "2X"},
	{3.0, "3X"},
	{4.0, "4X"},
	{5.0, "5X"},
	{6.0, "6X"},
	};

	value_t MinimapLocations[] = {
	{0.0, "Left Top"},
	{1.0, "Left Middle"},
	{2.0, "Left Bottom"},
	{3.0, "Right Top"},
	{4.0, "Right Middle"},
	{5.0, "Right Bottom"},
	};

	value_t Overlays[] = {
	{0.0, "Off"},
	{1.0, "Standard"},
	{2.0, "Full"},
	{3.0, "Full Only"},
	};
} // namespace valuesets

	struct optionsvalueset_t
	{
		std::string_view id;
		value_t* values;
		int count;
	};

	const std::array optionsValueSets = {
		optionsvalueset_t{"OnOff", valuesets::OnOff, static_cast<int>(ARRAY_LENGTH(valuesets::OnOff))},
		optionsvalueset_t{"YesNo", valuesets::YesNo, static_cast<int>(ARRAY_LENGTH(valuesets::YesNo))},
		optionsvalueset_t{"OffOn", valuesets::OffOn, static_cast<int>(ARRAY_LENGTH(valuesets::OffOn))},
		optionsvalueset_t{"HideShow", valuesets::HideShow, static_cast<int>(ARRAY_LENGTH(valuesets::HideShow))},
		optionsvalueset_t{"OnOffAuto", valuesets::OnOffAuto, static_cast<int>(ARRAY_LENGTH(valuesets::OnOffAuto))},
		optionsvalueset_t{"DemoRestrictions", valuesets::DemoRestrictions,
		                  static_cast<int>(ARRAY_LENGTH(valuesets::DemoRestrictions))},
		optionsvalueset_t{"DoomOrOdamex", valuesets::DoomOrOdamex,
		                  static_cast<int>(ARRAY_LENGTH(valuesets::DoomOrOdamex))},
		optionsvalueset_t{"MusSys", valuesets::MusSys, static_cast<int>(ARRAY_LENGTH(valuesets::MusSys))},
		optionsvalueset_t{"MidiReset", valuesets::MidiReset, static_cast<int>(ARRAY_LENGTH(valuesets::MidiReset))},
		optionsvalueset_t{"OplCore", valuesets::OplCore, static_cast<int>(ARRAY_LENGTH(valuesets::OplCore))},
		optionsvalueset_t{"OplBank", valuesets::OplBank, static_cast<int>(ARRAY_LENGTH(valuesets::OplBank))},
		optionsvalueset_t{"VoxType", valuesets::VoxType, static_cast<int>(ARRAY_LENGTH(valuesets::VoxType))},
		optionsvalueset_t{"ChatSndType", valuesets::ChatSndType, static_cast<int>(ARRAY_LENGTH(valuesets::ChatSndType))},
		optionsvalueset_t{"WeapSwitch", valuesets::WeapSwitch, static_cast<int>(ARRAY_LENGTH(valuesets::WeapSwitch))},
		optionsvalueset_t{"Wipes", valuesets::Wipes, static_cast<int>(ARRAY_LENGTH(valuesets::Wipes))},
		optionsvalueset_t{"Endoom", valuesets::Endoom, static_cast<int>(ARRAY_LENGTH(valuesets::Endoom))},
		optionsvalueset_t{"SecretOptions", valuesets::SecretOptions,
		                  static_cast<int>(ARRAY_LENGTH(valuesets::SecretOptions))},
		optionsvalueset_t{"TimerStyles", valuesets::TimerStyles, static_cast<int>(ARRAY_LENGTH(valuesets::TimerStyles))},
		optionsvalueset_t{"FlagHelds", valuesets::FlagHelds, static_cast<int>(ARRAY_LENGTH(valuesets::FlagHelds))},
		optionsvalueset_t{"Crosshairs", valuesets::Crosshairs, static_cast<int>(ARRAY_LENGTH(valuesets::Crosshairs))},
		optionsvalueset_t{"ExtendedHudStyles", valuesets::ExtendedHudStyles,
		                  static_cast<int>(ARRAY_LENGTH(valuesets::ExtendedHudStyles))},
		optionsvalueset_t{"TextColors", valuesets::TextColors, static_cast<int>(ARRAY_LENGTH(valuesets::TextColors))},
		optionsvalueset_t{"ScaleFactors", valuesets::ScaleFactors,
		                  static_cast<int>(ARRAY_LENGTH(valuesets::ScaleFactors))},
		optionsvalueset_t{"ClassicMapStringTypes", valuesets::ClassicMapStringTypes,
		                  static_cast<int>(ARRAY_LENGTH(valuesets::ClassicMapStringTypes))},
		optionsvalueset_t{"AutomapScales", valuesets::AutomapScales,
		                  static_cast<int>(ARRAY_LENGTH(valuesets::AutomapScales))},
		optionsvalueset_t{"MinimapLocations", valuesets::MinimapLocations,
		                  static_cast<int>(ARRAY_LENGTH(valuesets::MinimapLocations))},
		optionsvalueset_t{"Overlays", valuesets::Overlays, static_cast<int>(ARRAY_LENGTH(valuesets::Overlays))},
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
