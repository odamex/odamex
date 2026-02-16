// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//  Stopwatch controller
//
//-----------------------------------------------------------------------------
#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Stopwatch;

/// This class owns a collection of stopwatches, allows user code to create, fetch,
/// and record stopwatches for analysis.
class StopwatchManager
{
	public:
		typedef std::map<std::string, std::shared_ptr<Stopwatch> > WatchMap;

		/// Create a stopwatch with the given name.  If the watch already exists,
		/// it is returned without creating a new one.
		std::shared_ptr<Stopwatch> CreateStopwatch(const std::string& i_name);

		/// Fetch an existing stopwatch.  If the watch doesn't exist, an empty
		/// shared_ptr is returned.
		std::shared_ptr<Stopwatch> Get(const std::string& i_name);

		/// Return a collection of pointers to stopwatches whose names match
		/// the given regular expression.  Note that the regex language is
		/// that used by standard C++.
		std::vector<WatchMap::value_type> Find(const std::string& i_expression);

		void EnableAll();
		void DisableAll();

		/// Opens a CSV file for writing out a recording of stopwatch samples.
		/// Any stopwatches that are enabled at the time that this function is
		/// called will be included in the recording.  Any watches enabled
		/// while a recording is underway will NOT be included in the recording.
		///
		/// If the given file already exists, it is overwritten.  Returns true
		/// if the file was successfully opened for writing, false otherwise.
		bool StartRecording(const std::string& i_filepath);

		/// Write out the next line of the CSV with the current values of the
		/// enabled, recorded stopwatches.
		void RecordSamples(int i_tic);

		/// Close the recording file.
		void StopRecording();

	protected:

		WatchMap                                m_watches;
		std::unique_ptr<std::ofstream>          m_outFile;
		std::vector<std::shared_ptr<Stopwatch>> m_activeRecordingWatches;
};
