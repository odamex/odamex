// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
//
// Permission is hereby granted, free of charge,
// to any person obtaining a copy of this software and associated documentation
// files(the “Software”),
// to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and / or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all copies
// or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”,
// WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
// .IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM,
// OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// DESCRIPTION:
//
// Helper library to make the bridge between Discord SDK and the shim.
//
//-----------------------------------------------------------------------------

#pragma once

#include "discord.h"

#include <memory>
#include <string>
#include <sstream>

static constexpr uint64_t OdamexDiscordAppId = 1295042491409498222;

struct StatusUpdate
{
	int current_size;
	int max_size;

	int64_t start;
	int64_t end;

	discord::ActivityPartyPrivacy privacy;

	std::string details;
	std::string state;

	std::string large_image;
	std::string large_image_text;

	std::string small_image;
	std::string small_image_text;

	std::string party_id;
	std::string join_secret;

	void status_update_deserialize(const uint8_t* buf, unsigned int buflen)
	{
		std::istringstream iss(std::string((const char*)buf, buflen));
		iss.read(reinterpret_cast<char*>(&current_size), sizeof(current_size));
		iss.read(reinterpret_cast<char*>(&max_size), sizeof(max_size));
		iss.read(reinterpret_cast<char*>(&start), sizeof(start));
		iss.read(reinterpret_cast<char*>(&end), sizeof(end));
		iss.read(reinterpret_cast<char*>(&privacy), sizeof(privacy));
		std::getline(iss, details, '\0');
		std::getline(iss, state, '\0');
		std::getline(iss, large_image, '\0');
		std::getline(iss, large_image_text, '\0');
		std::getline(iss, small_image, '\0');
		std::getline(iss, small_image_text, '\0');
		std::getline(iss, party_id, '\0');
		std::getline(iss, join_secret, '\0');
	}
};

class ODiscordLib
{
  public:
	static ODiscordLib& getInstance(); // returns the instantiated ODiscordLib
	                                   // object
	void Init_DiscordSdk(void);
	void Update_RichPresence(const StatusUpdate& update);
	void Run_Callbacks(void);

  private:
	std::unique_ptr<discord::Core> core;

	void initializeDiscordSdk(void);
	void updateRichPresence(const StatusUpdate& update);
	void runCallbacks(void);
};
