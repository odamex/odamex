// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
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
//	Stacktrace for more useful error messages
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "m_stacktrace.h"

#include "cpptrace/cpptrace.hpp"
#include "cpptrace/formatting.hpp"

std::string M_GetStacktrace(std::string header)
{
	auto formatter = cpptrace::formatter{}
		.header(header)
		.colors(cpptrace::formatter::color_mode::none)
		.addresses(cpptrace::formatter::address_mode::none)
		.paths(cpptrace::formatter::path_mode::basename)
		.columns(false)
		.snippets(false)
		.filter([](const auto& frame)
			{ return frame.symbol.find("M_GetStacktrace") == std::string::npos; })
		.filtered_frame_placeholders(false);
	return formatter.format(cpptrace::generate_trace());
}
