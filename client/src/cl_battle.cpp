// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2022-2022 by DoomBattle.Zone.
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
//   Query host to connect to battle
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "c_dispatch.h"
#include "cl_battle.h"
#include "cl_main.h"
#include "git_describe.h"
#include "i_system.h"
#include "m_fileio.h"

#include "curl/curl.h"

//-----------------------------------------------------------------------------

#define RETRY_SECONDS 5

//-----------------------------------------------------------------------------

CURL* http_handle = NULL;
CURLM* multi_handle = NULL;
int still_running = 0;
std::string readBuffer;
uint64_t nextCurlTime = 0;
std::string battle_url;

typedef enum client_battle_status_e {
	CBS_NONE,
	CBS_ACTIVE,
	CBS_LOST,
	CBS_WON,
	CBS_MAX
} client_battle_status_t;

client_battle_status_t cl_battle_status = CBS_NONE;

std::string cl_battleover_hud_markup = "";
std::string cl_battleover_client_message = "";

// indicates that disconnect/quit should try to reconnect
bool cl_battle_restart = false;

//-----------------------------------------------------------------------------

void CL_BattleInit()
{
	Printf("CL_BattleInit: Init Battle subsystem (libcurl %d.%d.%d)\n",
		LIBCURL_VERSION_MAJOR, LIBCURL_VERSION_MINOR, LIBCURL_VERSION_PATCH);
}

//-----------------------------------------------------------------------------

void CleanupHandles()
{
	if (multi_handle)
	{
		if (http_handle)
			curl_multi_remove_handle(multi_handle, http_handle);

		curl_multi_cleanup(multi_handle);
		multi_handle = NULL;
	}

	if (http_handle)
	{
		curl_easy_cleanup(http_handle);
		http_handle = NULL;
	}

	still_running = 0;
	nextCurlTime = 0;
}

//-----------------------------------------------------------------------------

void CL_BattleShutdown()
{
	CleanupHandles();
}

//-----------------------------------------------------------------------------

bool CL_IsBattleActive()
{
	return cl_battle_status == CBS_ACTIVE;
}

//-----------------------------------------------------------------------------

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

//-----------------------------------------------------------------------------

void UpdateMultiHandle(bool removeHttpHandle)
{
	if (removeHttpHandle)
		curl_multi_remove_handle(multi_handle, http_handle);

	readBuffer.clear();
	curl_multi_add_handle(multi_handle, http_handle);
}

//-----------------------------------------------------------------------------

std::string parse_uri(char const* uri, char const* protocol, bool return_ticket);

//-----------------------------------------------------------------------------

bool CL_StartBattle(std::string const& uri)
{
	if (http_handle || multi_handle)
	{
		Printf("Battle already started. Use 'stop' to clear it [%s].\n", uri.c_str());
		return false;
	}

	char const* dbz_protocol = "doombattlezone://";
	std::string query_ticket = parse_uri(uri.c_str(), dbz_protocol, true);

	if (query_ticket.empty())
	{
		Printf("Battle uri missing ticket [%s].\n", uri.c_str());
		return false;
	}

	http_handle = curl_easy_init();

	if (http_handle)
	{
		// init a multi stack
		multi_handle = curl_multi_init();

		if (!multi_handle)
		{
			Printf("Failed to initialize curl multi_handle [%s].\n", uri.c_str());
			CleanupHandles();
			return false;
		}

		ticket = query_ticket;
		battle_uri = uri;

		std::string protocol = "https";
		if (uri.find("://local") != std::string::npos)
			protocol = "http";

		battle_url = protocol + uri.substr(uri.find_first_of(':')) + "&version=" DOTVERSIONSTR "-" BATTLEVERSION;

		Printf("URL: [%s] TICKET:[%s]\n", battle_url.c_str(), ticket.c_str());

		curl_easy_setopt(http_handle, CURLOPT_URL, battle_url.c_str());
		curl_easy_setopt(http_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(http_handle, CURLOPT_WRITEDATA, &readBuffer);

		UpdateMultiHandle(false);

		return true;
	}
	else
	{
		Printf("Failed to initialize http_handle [%s].\n", uri.c_str());
	}

	return false;
}

//-----------------------------------------------------------------------------

void CL_RestartBattle()
{
	if (!cl_battle_restart)
		return;

	cl_battle_restart = false;

	// battle reconnect
	Printf(PRINT_HIGH, "QuitNetGame: Connecting to another battle [%s].\n", battle_uri.c_str());
	CL_ClearBattle();
	AddCommandString("battle \"" + battle_uri + "\"");
}

//-----------------------------------------------------------------------------

void CL_StopBattle()
{
	CleanupHandles();
}

//-----------------------------------------------------------------------------

std::string WriteReadBufferToFile()
{
	std::string error_file = M_GetUserFileName("odamex-battle-error.html");

	FILE* fh = fopen(error_file.c_str(), "w");
	if (!fh)
		exit(EXIT_FAILURE);

	const int ok = fwrite(readBuffer.data(), sizeof(char), readBuffer.size(), fh);
	if (!ok)
		exit(EXIT_FAILURE);

	fclose(fh);

	readBuffer.clear();

	return error_file;
}

//-----------------------------------------------------------------------------

bool CL_HandleBattleResponse()
{
	if (readBuffer.length() > 512)
	{
		Printf("Invalid length. See file [%s].\n", WriteReadBufferToFile().c_str());
		return true;
	}

	std::vector<std::string> response = StdStringSplit(readBuffer, ",");

	if (response.size() < 2)
	{
		Printf("Invalid response size. See file [%s].\n", WriteReadBufferToFile().c_str());
		return true;
	}

	if (response[0] == "CONNECT")
	{
		AddCommandString("connect " + response[1]);
		Printf("CONNECT [%s]\n", response.size() >= 2 ? response[2].c_str() : "?");
		cl_battle_status = CBS_ACTIVE;
	}
	else if (response[0] == "UPDATE")
	{
		Printf("UPDATE [%s]\n", response[1].c_str());
	}
	else if (response[0] == "RELOG")
	{
		Printf("RELOG [%s]\n", response[1].c_str());
	}
	else if (response[0] == "WAIT")
	{
		nextCurlTime = I_MSTime() + 1000 * RETRY_SECONDS;
		Printf("WAIT [%s] (trying again in %d seconds)\n", response[1].c_str(), RETRY_SECONDS);
		UpdateMultiHandle(true);
		return false;
	}
	else
	{
		Printf("Invalid Response [%s]\n", readBuffer.c_str());
	}

	return true;
}

//-----------------------------------------------------------------------------

void CL_BattleTick()
{
	if (!http_handle || !multi_handle)
		return;

	if (nextCurlTime && I_MSTime() < nextCurlTime)
		return;

	CURLMcode mc = curl_multi_perform(multi_handle, &still_running);

	if (mc)
		Printf("curl_multi_perform() failed, code %d.\n", (int)mc);

	// done?
	if (still_running == 0 && CL_HandleBattleResponse())
		CleanupHandles();
}

//-----------------------------------------------------------------------------

static void BattleHelp()
{
	Printf("battle - Starts a battle\n\n"
		"Usage:\n"
		"  ] battle uri\n"
		"  Connects to a battle using the provided uri.\n"
		"  ] battle stop\n"
		"  Stop an in-progress battle connection."
	);
}

//-----------------------------------------------------------------------------

void CL_SetBattleOver(bool winner, std::string const& hud_markup, std::string const& client_message)
{
	cl_battle_status = winner ? CBS_WON : CBS_LOST;
	cl_battleover_hud_markup = hud_markup;
	cl_battleover_client_message = client_message;
	// try one restart after disconnect
	cl_battle_restart = true;
}

//-----------------------------------------------------------------------------

void CL_ClearBattle()
{
	cl_battle_status = CBS_NONE;
	cl_battleover_hud_markup = "";
	cl_battleover_client_message = "";
}

//-----------------------------------------------------------------------------

bool CL_IsBattleOver()
{
	return cl_battle_status == CBS_WON || cl_battle_status == CBS_LOST;
}

//-----------------------------------------------------------------------------

bool CL_IsBattleWinner()
{
	return cl_battle_status == CBS_WON;
}

//-----------------------------------------------------------------------------

std::string const& CL_GetBattleOverHudMarkup()
{
	return cl_battleover_hud_markup;
}

//-----------------------------------------------------------------------------

std::string const& CL_GetBattleOverClientMessage()
{
	return cl_battleover_client_message;
}

//-----------------------------------------------------------------------------

BEGIN_COMMAND(battle)
{
	if (argc < 2)
	{
		BattleHelp();
		return;
	}

	if (stricmp(argv[1], "stop") == 0)
	{
		if (nextCurlTime)
		{
			CL_StopBattle();
			Printf(PRINT_WARNING, "Battle cancelled.\n");
		}
		else
		{
			Printf(PRINT_WARNING, "No battle connection in progress.\n");
		}
		return;
	}

	CL_StartBattle(argv[1]);
}
END_COMMAND(battle)

//-----------------------------------------------------------------------------

VERSION_CONTROL(cl_battle_cpp, "$Id$")
