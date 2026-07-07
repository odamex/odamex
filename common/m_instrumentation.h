// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jim Thoenen.
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
//  Singleton for coordinating the enablement and recording of multiple timing
//  instrumentation Stopwatches.
//
//-----------------------------------------------------------------------------

#pragma once

#include "StopwatchManager.h"

#include "Stopwatch.h"

class TimingInstr
{
	public:
		static TimingInstr& Get()
		{
			static TimingInstr s_instance;
			return s_instance;
		}

		std::shared_ptr<Stopwatch> CreateStopwatch(const std::string& i_name);

		size_t EnableStopwatches(const std::string& i_regex);
		size_t DisableStopwatches(const std::string& i_regex);

		bool StartRecording(const std::string& i_filename);
		void ManageRecording(int i_tic);
		bool StopRecording();

		const std::string& GetFilename();

	protected:
		TimingInstr() = default;

		StopwatchManager m_manager;
};
