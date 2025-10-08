// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//
// Objects to create and serialize status updates of player activity to
// any external IPC processes that need them. This is mainly the Odashim.
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>
#include <sstream>

enum MatchJoinPrivacy
{
	Private = 0,
	Public = 1,
};

struct StatusUpdate
{
	int current_size;
	int max_size;

	int64_t start;
	int64_t end;

	MatchJoinPrivacy privacy;

	std::string details;
	std::string state;

	std::string large_image;
	std::string large_image_text;

	std::string small_image;
	std::string small_image_text;

	std::string party_id;
	std::string join_secret;

	std::ostringstream& status_update_serialize(std::ostringstream& stream) const
	{
		stream.write(reinterpret_cast<const char*>(&current_size), sizeof(current_size));
		stream.write(reinterpret_cast<const char*>(&max_size), sizeof(max_size));
		stream.write(reinterpret_cast<const char*>(&start), sizeof(start));
		stream.write(reinterpret_cast<const char*>(&end), sizeof(end));
		stream.write(reinterpret_cast<const char*>(&privacy), sizeof(privacy));

		stream << details << '\0';
		stream << state << '\0';
		stream << large_image << '\0';
		stream << large_image_text << '\0';
		stream << small_image << '\0';
		stream << small_image_text << '\0';
		stream << party_id << '\0';
		stream << join_secret << '\0';

		return stream;
	}

	bool operator==(const StatusUpdate& comp)
	{
		if (
				this->current_size == comp.current_size &&
				this->details == comp.details &&
				this->end == comp.end &&
				this->join_secret == comp.join_secret &&
				this->large_image == comp.large_image &&
				this->large_image_text == comp.large_image_text &&
				this->max_size == comp.max_size &&
				this->party_id == comp.party_id &&
				this->privacy == comp.privacy &&
				this->small_image == comp.small_image &&
				this->small_image_text == comp.small_image_text &&
				this->start == comp.start &&
				this->state == comp.state
			)
		{
			return true;
		}
		return false;
	}
};
