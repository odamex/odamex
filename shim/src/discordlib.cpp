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

#include "discordlib.h"

#include <ctime>

ODiscordLib& ODiscordLib::getInstance()
{
	static ODiscordLib instance;
	return instance;
}

void ODiscordLib::Init_DiscordSdk(void)
{
	initializeDiscordSdk();
}

void ODiscordLib::Update_RichPresence(const StatusUpdate& update)
{
	updateRichPresence(update);
}

void ODiscordLib::Run_Callbacks(void)
{
	runCallbacks();
}

void ODiscordLib::initializeDiscordSdk(void)
{ 
	discord::Core* newcore{};
	auto result =	discord::Core::Create(OdamexDiscordAppId, DiscordCreateFlags_Default, &newcore);
	core.reset(newcore);
	if (!core)
	{
		printf("Failed to instantiate discord core! (error %d)\n", static_cast<int>(result));
	}

	core->SetLogHook(discord::LogLevel::Debug, [](discord::LogLevel level, const char* message)
	{
		printf("Debug: %s)\n", message);
	});

	core->SetLogHook(discord::LogLevel::Info, [](discord::LogLevel level, const char* message)
	{
		printf("Info: %s)\n", message);
	});

	core->SetLogHook(discord::LogLevel::Warn, [](discord::LogLevel level, const char* message)
	{
		printf("Warn: %s)\n", message);
	});

	core->SetLogHook(discord::LogLevel::Error, [](discord::LogLevel level, const char* message)
	{
		printf("Error: %s)\n", message);
	});

	printf("Discord core successfully initiated!\n");
}

void ODiscordLib::updateRichPresence(const StatusUpdate& update)
{
	discord::Activity activity{};

	// Set details and state
	activity.SetDetails(update.details.c_str());
	activity.SetState(update.state.c_str());

	// Set display images
	activity.GetAssets().SetSmallImage(update.small_image.c_str());
	activity.GetAssets().SetSmallText(update.small_image_text.c_str());
	activity.GetAssets().SetLargeImage(update.large_image.c_str());
	activity.GetAssets().SetLargeText(update.large_image_text.c_str());


	// Set server/party size
	if (update.current_size > 0)
		activity.GetParty().GetSize().SetCurrentSize(update.current_size);

	if (update.max_size > 0 && update.current_size > 0)
		activity.GetParty().GetSize().SetMaxSize(update.max_size);

	// Set activity start/end
	if (update.start > 0)
		activity.GetTimestamps().SetStart(static_cast<time_t>(update.start));

	if (update.end > 0)
		activity.GetTimestamps().SetEnd(static_cast<time_t>(update.end));

	// Set Party Privacy/Secrets
	activity.GetParty().SetPrivacy(update.privacy);
	activity.GetParty().SetId(update.party_id.c_str());
	activity.GetSecrets().SetJoin(update.join_secret.c_str());

	if (!update.party_id.empty())
	{
		std::string joinpath = "odamex://";
		core->ActivityManager().RegisterCommand(joinpath.c_str());
	}

	// Non-configurables
	activity.SetInstance(true);
	activity.SetType(discord::ActivityType::Playing);

	// Finally, lets log it all!
	core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
		printf("%s updating activity!\n",
		       (result == discord::Result::Ok ? "Succeeded" : "Failed"));
	});
}

void ODiscordLib::runCallbacks(void)
{
	core->RunCallbacks();
}
