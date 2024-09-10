// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Server database management.
//
//-----------------------------------------------------------------------------

#include "odalaunch.h"

#include <cassert>

#include "db.h"
#include "log.h"

#include "json/json.h"

#include "sqlite3.h"

/******************************************************************************/

const int SQL_TRIES = 3;

static const char* SERVERS_TABLE =           //
    "CREATE TABLE servers ("                 //
    "    address TEXT PRIMARY KEY NOT NULL," //
    "    servername TEXT,"                   //
    "    gametype TEXT,"                     //
    "    wads TEXT,"                         //
    "    map TEXT,"                          //
    "    numplayers INTEGER,"                //
    "    maxplayers INTEGER,"                //
    "    lives INTEGER,"                     //
    "    sides INTEGER,"                     //
    "    ping INTEGER,"                      //
    "    players BLOB,"                      //
    "    cvars BLOB,"                        //
    "    lockid INTEGER,"                    //
    "    gen INTEGER DEFAULT 0"              //
    ") STRICT;";                             //

static const char* ADD_SERVER =      //
    "REPLACE INTO servers(address) " //
    "    VALUES(:address);";         //

static const char* ADD_SERVER_INFO =                                       //
    "UPDATE servers "                                                      //
    "SET servername = :servername, gametype = :gametype, wads = :wads, "   //
    "    map = :map, numplayers = :numplayers, maxplayers = :maxplayers, " //
    "    lives = :lives, sides = :sides, players = jsonb(:players), "      //
    "    cvars = jsonb(:cvars), ping = :ping, lockid = NULL "              //
    "WHERE address = :address;";                                           //

static const char* GET_SERVER_LIST =                  //
    "SELECT "                                         //
    "    address, servername, gametype, wads, map, "  //
    "    numplayers, maxplayers, lives, sides, ping " //
    "FROM servers;";                                  //

static const char* GET_SERVER =                       //
    "SELECT "                                         //
    "    address, servername, gametype, wads, map, "  //
    "    numplayers, maxplayers, lives, sides, ping " //
    "FROM servers "                                   //
    "WHERE address = :address;";                      //

static const char* GET_SERVER_PLAYERS =               //
    "SELECT "                                         //
    "    address, servername, gametype, wads, map, "  //
    "    numplayers, maxplayers, lives, sides, ping " //
    "FROM servers "                                   //
    "WHERE address = :address;";                      //

static const char* GET_SERVER_CVARS =         //
    "SELECT key, value "                      //
    "FROM servers, json_each(servers.cvars) " //
    "WHERE servers.address = :address";       //

static const char* LOCK_SERVER =        //
    "UPDATE servers "                   //
    "SET lockid = :lockid, gen = :gen " //
    "WHERE rowid IN "                   //
    "("                                 //
    " SELECT rowid FROM servers "       //
    " WHERE lockid IS NULL "            //
    " AND gen < :gen "                  //
    " LIMIT 1 "                         //
    ");";                               //

static const char* GET_LOCKED_SERVER = //
    "SELECT address FROM servers "     //
    "WHERE lockid = :lockid;";         //

static const char* STRIKE_SERVER = //
    "DELETE FROM servers "         //
    "WHERE address = :address;";   //

static sqlite3* g_pDatabase;

/******************************************************************************/

/**
 * @brief Log an SQL error.
 * @param Unused.
 * @param errCode Error code to log.
 * @param msg Message to log.
 */
static void SQLError(void*, int errCode, const char* msg)
{
	Log_Debug("[SQLITE] {}: {}\n", errCode, msg);
}

/**
 * @brief Prepare a statement.
 *
 * @param sql SQL statement to prepare.
 * @return Pointer to prepared statement if successful, otherwise nullptr.
 */
NODISCARD static sqlite3_stmt* SQLPrepare(const char* sql)
{
	sqlite3_stmt* rvo = nullptr;
	const int res = sqlite3_prepare_v2(g_pDatabase, sql, -1, &rvo, nullptr);
	if (res != SQLITE_OK)
	{
		return nullptr;
	}
	return rvo;
}

/**
 * @brief Bind a 32-bit integer.
 *
 * @param stmt Prepared statement.
 * @param param Param to bind.
 * @param data Data to bind.
 * @return True if data was bound successfully, otherwise false.
 */
NODISCARD static bool SQLBindInt(sqlite3_stmt* stmt, const char* param, const int data)
{
	const int idx = sqlite3_bind_parameter_index(stmt, param);
	if (idx == 0)
	{
		Log_Debug("Unknown parameter \"{}\".\n", param);
		return false;
	}

	const int res = sqlite3_bind_int(stmt, idx, data);
	if (res != SQLITE_OK)
	{
		return false;
	}

	return true;
}

/**
 * @brief Bind a 64-bit integer.
 *
 * @param stmt Prepared statement.
 * @param param Param to bind.
 * @param data Data to bind.
 * @return True if data was bound successfully, otherwise false.
 */
NODISCARD static bool SQLBindInt64(sqlite3_stmt* stmt, const char* param,
                                   const int64_t data)
{
	const int idx = sqlite3_bind_parameter_index(stmt, param);
	if (idx == 0)
	{
		Log_Debug("Unknown parameter \"{}\".\n", param);
		return false;
	}

	const int res = sqlite3_bind_int64(stmt, idx, data);
	if (res != SQLITE_OK)
	{
		return false;
	}

	return true;
}

/**
 * @brief Bind a text parameter.
 *
 * @param stmt Prepared statement.
 * @param param Param to bind.
 * @param data Data to bind.
 * @return True if data was bound successfully, otherwise false.
 */
NODISCARD static bool SQLBindText(sqlite3_stmt* stmt, const char* param, const char* data)
{
	const int idx = sqlite3_bind_parameter_index(stmt, param);
	if (idx == 0)
	{
		Log_Debug("Unknown parameter \"{}\".\n", param);
		return false;
	}

	const int res = sqlite3_bind_text(stmt, idx, data, -1, SQLITE_TRANSIENT);
	if (res != SQLITE_OK)
	{
		return false;
	}

	return true;
}

/******************************************************************************/

static Json::Value WadsToJSON(const std::vector<odalpapi::Wad_t>& ncWads)
{
	Json::Value root;

	for (auto& wad : ncWads)
	{
		Json::Value json;

		json["name"] = wad.Name;
		json["hash"] = wad.Hash;

		root.append(std::move(json));
	}

	return root;
}

/******************************************************************************/

static Json::Value PlayersToJSON(const std::vector<odalpapi::Player_t>& ncPlayers)
{
	Json::Value root;

	for (auto& player : ncPlayers)
	{
		Json::Value json;

		json["name"] = player.Name;
		json["color"] = player.Colour;
		json["kills"] = player.Kills;
		json["deaths"] = player.Deaths;
		json["time"] = player.Time;
		json["frags"] = player.Frags;
		json["ping"] = player.Ping;
		json["team"] = player.Team;
		json["spectator"] = player.Spectator;

		root.append(std::move(json));
	}

	return root;
}

/******************************************************************************/

static Json::Value CvarsToJSON(const std::vector<odalpapi::Cvar_t>& ncCvars)
{
	Json::Value root;

	for (auto& cvar : ncCvars)
	{
		root[cvar.Name] = cvar.Value;
	}

	return root;
}

/******************************************************************************/

NODISCARD bool DB_Init()
{
	if (::g_pDatabase != nullptr)
	{
		Log_Debug("Database is not NULL.\n");
		return false;
	}

	if (sqlite3_config(SQLITE_CONFIG_LOG, SQLError, nullptr) != SQLITE_OK)
	{
		Log_Debug("Could not config database ({}).\n", sqlite3_errmsg(g_pDatabase));
		return false;
	}

	if (sqlite3_open(":memory:", &::g_pDatabase) != SQLITE_OK)
	{
		Log_Debug("Could not open database ({}).\n", sqlite3_errmsg(g_pDatabase));
		return false;
	}

	if (sqlite3_exec(::g_pDatabase, "PRAGMA integrity_check;", nullptr, nullptr, nullptr))
	{
		return false;
	}

	if (sqlite3_exec(::g_pDatabase, ::SERVERS_TABLE, nullptr, nullptr, nullptr) !=
	    SQLITE_OK)
	{
		return false;
	}

	return true;
}

/******************************************************************************/

void DB_DeInit()
{
	if (g_pDatabase == nullptr)
		return;

	sqlite3_close(g_pDatabase);
	g_pDatabase = nullptr;
}

/******************************************************************************/

void DB_AddServer(const std::string& address)
{
	sqlite3_stmt* stmt = SQLPrepare(::ADD_SERVER);
	if (stmt == nullptr)
		return;

	auto onExit = makeScopeExit([stmt] { sqlite3_finalize(stmt); });

	if (!SQLBindText(stmt, ":address", address.c_str()))
		return;

	for (size_t tries = 0; tries < ::SQL_TRIES; tries++)
	{
		if (sqlite3_step(stmt) != SQLITE_DONE)
			continue;

		break;
	}
}

/******************************************************************************/

void DB_AddServerInfo(const odalpapi::Server& server)
{
	std::string wads;

	sqlite3_stmt* stmt = SQLPrepare(::ADD_SERVER_INFO);
	if (stmt == nullptr)
		return;

	auto onExit = makeScopeExit([stmt] { sqlite3_finalize(stmt); });

	Json::StreamWriterBuilder builder;
	Json::String buffer;

	if (!SQLBindText(stmt, ":address", server.GetAddress().c_str()))
		return;

	if (!SQLBindText(stmt, ":servername", server.Info.Name.c_str()))
		return;

	if (!SQLBindInt(stmt, ":gametype", server.Info.GameType))
		return;

	buffer = Json::writeString(builder, WadsToJSON(server.Info.Wads));
	if (!SQLBindText(stmt, ":wads", buffer.c_str()))
		return;

	if (!SQLBindText(stmt, ":map", server.Info.CurrentMap.c_str()))
		return;

	if (!SQLBindInt64(stmt, ":numplayers", server.Info.Players.size()))
		return;

	if (!SQLBindInt(stmt, ":maxplayers", server.Info.MaxPlayers))
		return;

	if (!SQLBindInt(stmt, ":lives", server.Info.Lives))
		return;

	if (!SQLBindInt(stmt, ":sides", server.Info.Sides))
		return;

	buffer = Json::writeString(builder, PlayersToJSON(server.Info.Players));
	if (!SQLBindText(stmt, ":players", buffer.c_str()))
		return;

	buffer = Json::writeString(builder, CvarsToJSON(server.Info.Cvars));
	if (!SQLBindText(stmt, ":cvars", buffer.c_str()))
		return;

	if (!SQLBindInt(stmt, ":ping", server.GetPing()))
		return;

	for (size_t tries = 0; tries < ::SQL_TRIES; tries++)
	{
		const int res = sqlite3_step(stmt);
		if (res != SQLITE_DONE)
		{
			continue;
		}

		break;
	}
}

/******************************************************************************/

void DB_GetServerList(serverRows_t& rows)
{
	int res;
	size_t newRowCount;

	sqlite3_stmt* stmt = SQLPrepare(::GET_SERVER_LIST);
	if (!stmt)
		return;

	auto onExit = makeScopeExit([stmt] { sqlite3_finalize(stmt); });

	Json::CharReaderBuilder builder;
	newRowCount = 0;
	for (size_t tries = 0; tries < ::SQL_TRIES;)
	{
		res = sqlite3_step(stmt);
		switch (res)
		{
		case SQLITE_ROW: {
			char buffer[64];
			serverRow_s newRow;

			// Extract the row into intermediate representation.
			const char* address = (const char*)(sqlite3_column_text(stmt, 0));
			const char* servername = (const char*)(sqlite3_column_text(stmt, 1));
			const int gametype = sqlite3_column_int(stmt, 2);
			const char* wads = (const char*)(sqlite3_column_text(stmt, 3));
			const size_t wadsLen = sqlite3_column_bytes(stmt, 3);

			Json::Value wadsJson;
			if (wads != nullptr)
			{
				const bool ok = builder.newCharReader()->parse(wads, wads + wadsLen,
				                                               &wadsJson, nullptr);
				assert(ok);
			}

			const char* map = (const char*)(sqlite3_column_text(stmt, 4));
			const int numplayers = sqlite3_column_int(stmt, 5);
			const int maxplayers = sqlite3_column_int(stmt, 6);
			const int lives = sqlite3_column_int(stmt, 7);
			const int sides = sqlite3_column_int(stmt, 8);
			const int ping = sqlite3_column_int(stmt, 9);

			newRow.address = address ? address : "";
			newRow.servername = servername ? servername : "";

			constexpr int GT_Cooperative = 0;
			constexpr int GT_Deathmatch = 1;
			constexpr int GT_TeamDeathmatch = 2;
			constexpr int GT_CaptureTheFlag = 3;
			constexpr int GT_Horde = 4;

			if (gametype == GT_Cooperative && lives)
				newRow.gametype = "Survival";
			else if (gametype == GT_Cooperative && maxplayers <= 1)
				newRow.gametype = "Single-player";
			else if (gametype == GT_Cooperative)
				newRow.gametype = "Cooperative";
			else if (gametype == GT_Deathmatch && lives)
				newRow.gametype = "Last Marine Standing";
			else if (gametype == GT_Deathmatch && maxplayers <= 2)
				newRow.gametype = "Duel";
			else if (gametype == GT_Deathmatch)
				newRow.gametype = "Deathmatch";
			else if (gametype == GT_TeamDeathmatch && lives)
				newRow.gametype = "Team Last Marine Standing";
			else if (gametype == GT_TeamDeathmatch)
				newRow.gametype = "Team Deathmatch";
			else if (gametype == GT_CaptureTheFlag && sides)
				newRow.gametype = "Attack & Defend CTF";
			else if (gametype == GT_CaptureTheFlag && lives)
				newRow.gametype = "LMS Capture The Flag";
			else if (gametype == GT_CaptureTheFlag)
				newRow.gametype = "Capture The Flag";
			else if (gametype == GT_Horde && lives)
				newRow.gametype = "Survival Horde";
			else if (gametype == GT_Horde)
				newRow.gametype = "Horde";
			else
				newRow.gametype = "Unknown";

			newRow.wads.clear();
			for (auto& wad : wadsJson)
			{
				auto name = wad.get("name", Json::String{});
				newRow.wads.push_back(name.asString());
			}
			newRow.map = map ? map : "";

			if (maxplayers != 0)
			{
				auto it = fmt::format_to_n(buffer, ARRAY_LENGTH(buffer), "{}/{}",
				                           numplayers, maxplayers);
				*it.out = '\0';
				newRow.players = buffer;
			}

			auto it = fmt::format_to_n(buffer, ARRAY_LENGTH(buffer), "{}", ping);
			*it.out = '\0';
			newRow.ping = buffer;

			// Insert the new server into the existing server data vector.
			newRowCount += 1;
			if (newRowCount > rows.size())
			{
				rows.resize(newRowCount);
			}
			rows[newRowCount - 1] = newRow;
			break;
		}
		case SQLITE_DONE:
			// Resize the vector down - in case it's now too big.
			rows.resize(newRowCount);
			return;
		default:
			tries++;
			break;
		}
	}
}

/******************************************************************************/

void DB_GetServer(serverRow_s& row, const std::string& address)
{
	int res;

	sqlite3_stmt* stmt = SQLPrepare(::GET_SERVER);
	if (!stmt)
		return;

	auto onExit = makeScopeExit([stmt] { sqlite3_finalize(stmt); });

	if (!SQLBindText(stmt, ":address", address.c_str()))
		return;

	Json::CharReaderBuilder builder;
	for (size_t tries = 0; tries < ::SQL_TRIES;)
	{
		res = sqlite3_step(stmt);
		switch (res)
		{
		case SQLITE_ROW: {
			char buffer[64];

			// Extract the row into intermediate representation.
			const char* address = (const char*)(sqlite3_column_text(stmt, 0));
			const char* servername = (const char*)(sqlite3_column_text(stmt, 1));
			const int gametype = sqlite3_column_int(stmt, 2);
			const char* wads = (const char*)(sqlite3_column_text(stmt, 3));
			const size_t wadsLen = sqlite3_column_bytes(stmt, 3);

			Json::Value wadsJson;
			if (wads != nullptr)
			{
				const bool ok = builder.newCharReader()->parse(wads, wads + wadsLen,
				                                               &wadsJson, nullptr);
				assert(ok);
			}

			const char* map = (const char*)(sqlite3_column_text(stmt, 4));
			const int numplayers = sqlite3_column_int(stmt, 5);
			const int maxplayers = sqlite3_column_int(stmt, 6);
			const int lives = sqlite3_column_int(stmt, 7);
			const int sides = sqlite3_column_int(stmt, 8);
			const int ping = sqlite3_column_int(stmt, 9);

			row.address = address ? address : "";
			row.servername = servername ? servername : "";

			constexpr int GT_Cooperative = 0;
			constexpr int GT_Deathmatch = 1;
			constexpr int GT_TeamDeathmatch = 2;
			constexpr int GT_CaptureTheFlag = 3;
			constexpr int GT_Horde = 4;

			if (gametype == GT_Cooperative && lives)
				row.gametype = "Survival";
			else if (gametype == GT_Cooperative && maxplayers <= 1)
				row.gametype = "Single-player";
			else if (gametype == GT_Cooperative)
				row.gametype = "Cooperative";
			else if (gametype == GT_Deathmatch && lives)
				row.gametype = "Last Marine Standing";
			else if (gametype == GT_Deathmatch && maxplayers <= 2)
				row.gametype = "Duel";
			else if (gametype == GT_Deathmatch)
				row.gametype = "Deathmatch";
			else if (gametype == GT_TeamDeathmatch && lives)
				row.gametype = "Team Last Marine Standing";
			else if (gametype == GT_TeamDeathmatch)
				row.gametype = "Team Deathmatch";
			else if (gametype == GT_CaptureTheFlag && sides)
				row.gametype = "Attack & Defend CTF";
			else if (gametype == GT_CaptureTheFlag && lives)
				row.gametype = "LMS Capture The Flag";
			else if (gametype == GT_CaptureTheFlag)
				row.gametype = "Capture The Flag";
			else if (gametype == GT_Horde && lives)
				row.gametype = "Survival Horde";
			else if (gametype == GT_Horde)
				row.gametype = "Horde";
			else
				row.gametype = "Unknown";

			row.wads.clear();
			for (auto& wad : wadsJson)
			{
				auto name = wad.get("name", Json::String{});
				row.wads.push_back(name.asString());
			}
			row.map = map ? map : "";

			if (maxplayers != 0)
			{
				auto it = fmt::format_to_n(buffer, ARRAY_LENGTH(buffer), "{}/{}",
				                           numplayers, maxplayers);
				*it.out = '\0';
				row.players = buffer;
			}

			auto it = fmt::format_to_n(buffer, ARRAY_LENGTH(buffer), "{}", ping);
			*it.out = '\0';
			row.ping = buffer;
			break;
		}
		case SQLITE_DONE:
			// Resize the vector down - in case it's now too big.
			return;
		default:
			tries++;
			break;
		}
	}
}

/******************************************************************************/

void DB_GetServerPlayers(serverPlayers_t& players, const std::string& address)
{
	int res;
	size_t newRowCount;

	sqlite3_stmt* stmt = SQLPrepare(::GET_SERVER_PLAYERS);
	if (!stmt)
		return;

	auto onExit = makeScopeExit([stmt] { sqlite3_finalize(stmt); });

	if (!SQLBindText(stmt, ":address", address.c_str()))
		return;

	newRowCount = 0;
	for (size_t tries = 0; tries < ::SQL_TRIES;)
	{
		res = sqlite3_step(stmt);
		switch (res)
		{
		case SQLITE_ROW: {
		}
		case SQLITE_DONE:
			// Resize the vector down - in case it's now too big.
			players.resize(newRowCount);
			return;
		default:
			tries++;
			break;
		}
	}
}

/******************************************************************************/

void DB_GetServerCvars(serverCvars_t& cvars, const std::string& address)
{
	int res;
	size_t newRowCount;

	sqlite3_stmt* stmt = SQLPrepare(::GET_SERVER_CVARS);
	if (!stmt)
		return;

	auto onExit = makeScopeExit([stmt] { sqlite3_finalize(stmt); });

	if (!SQLBindText(stmt, ":address", address.c_str()))
		return;

	newRowCount = 0;
	for (size_t tries = 0; tries < ::SQL_TRIES;)
	{
		res = sqlite3_step(stmt);
		switch (res)
		{
		case SQLITE_ROW: {
			// Extract the row into intermediate representation.
			const char* key = (const char*)(sqlite3_column_text(stmt, 0));
			const char* value = (const char*)(sqlite3_column_text(stmt, 1));
			if (strlen(value) == 0)
				break;

			serverCvar_s newRow;
			newRow.key = key;
			newRow.value = value;

			// Insert the new cvar into the existing cvar data vector.
			newRowCount += 1;
			if (newRowCount > cvars.size())
			{
				cvars.resize(newRowCount);
			}
			cvars[newRowCount - 1] = newRow;
			break;
		}
		case SQLITE_DONE:
			// Resize the vector down - in case it's now too big.
			cvars.resize(newRowCount);
			return;
		default:
			tries++;
			break;
		}
	}
}

/******************************************************************************/

static bool GetLockedAddress(const uint64_t id, std::string& outAddress)
{
	int res;
	sqlite3_stmt* stmt = SQLPrepare(::GET_LOCKED_SERVER);
	if (!stmt)
		return false;

	auto onExit = makeScopeExit([stmt] { sqlite3_finalize(stmt); });

	if (!SQLBindInt64(stmt, ":lockid", id))
		return false;

	for (size_t tries = 0; tries < ::SQL_TRIES;)
	{
		res = sqlite3_step(stmt);
		switch (res)
		{
		case SQLITE_ROW: {
			const char* address = (const char*)(sqlite3_column_text(stmt, 0));
			if (address == nullptr)
			{
				Log_Debug("Could not find address for lock {}\n",
				          static_cast<uint32_t>(id));
				return false;
			}

			outAddress = address;
			break;
		}
		case SQLITE_DONE:
			return true;
		default:
			tries++;
			break;
		}
	}

	return false;
}

/******************************************************************************/

NODISCARD bool DB_LockAddressForServerInfo(const uint64_t id, const uint64_t gen,
                                           std::string& outAddress)
{
	sqlite3_stmt* stmt = SQLPrepare(::LOCK_SERVER);
	if (!stmt)
		return false;

	auto onExit = makeScopeExit([stmt] { sqlite3_finalize(stmt); });

	if (!SQLBindInt64(stmt, ":lockid", id))
		return false;

	if (!SQLBindInt64(stmt, ":gen", gen))
		return false;

	for (size_t tries = 0; tries < ::SQL_TRIES; tries++)
	{
		if (sqlite3_step(stmt) != SQLITE_DONE)
		{
			continue;
		}

		if (!sqlite3_changes(::g_pDatabase))
		{
			Log_Debug("Lock not found for {}.\n", static_cast<uint32_t>(id));
			return false;
		}

		if (!GetLockedAddress(id, outAddress))
		{
			return false;
		}

		return true;
	}

	return false;
}

/******************************************************************************/

void DB_StrikeServer(const std::string& address)
{
	sqlite3_stmt* stmt = SQLPrepare(::STRIKE_SERVER);
	if (stmt == nullptr)
		return;

	auto onExit = makeScopeExit([stmt] { sqlite3_finalize(stmt); });

	if (!SQLBindText(stmt, ":address", address.c_str()))
		return;

	for (size_t tries = 0; tries < ::SQL_TRIES; tries++)
	{
		if (sqlite3_step(stmt) != SQLITE_DONE)
			continue;

		break;
	}
}
