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

#include "db.h"
#include "log.h"

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
    "    players INTEGER,"                   //
    "    maxplayers INTEGER,"                //
    "    ping INTEGER,"                      //
    "    serverupdate REAL,"                 //
    "    masterupdate REAL NOT NULL,"        //
    "    lockid INTEGER) STRICT;";           //

static const char* ADD_SERVER =                    //
    "REPLACE INTO servers(address, masterupdate) " //
    "    VALUES(:address, julianday('now'));";     //

static const char* ADD_SERVER_INFO =                                     //
    "UPDATE servers "                                                    //
    "SET servername = :servername, gametype = :gametype, wads = :wads, " //
    "    serverupdate = julianday('now'), lockid = NULL "                //
    "WHERE address = :address";                                          //

static const char* GET_SERVER_LIST =                             //
    "SELECT "                                                    //
    "    address, servername, gametype, wads, map, "             //
    "    players, maxplayers, ping, serverupdate, masterupdate " //
    "FROM servers;";                                             //

static const char* LOCK_SERVER =                            //
    "UPDATE servers "                                       //
    "SET lockid = :lockid WHERE rowid IN"                   //
    "(SELECT rowid FROM servers "                           //
    "WHERE lockid IS NULL ORDER BY serverupdate LIMIT 1);"; //

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

	if (!SQLBindText(stmt, ":address", server.GetAddress().c_str()))
		return;

	if (!SQLBindText(stmt, ":servername", server.Info.Name.c_str()))
		return;

	if (!SQLBindInt(stmt, ":gametype", server.Info.GameType))
		return;

	for (size_t i = 1; i < server.Info.Wads.size(); i++)
	{
		wads += server.Info.Wads[i].Name + ":";
	}
	wads.erase(wads.length() - 1);
	if (!SQLBindText(stmt, ":wads", wads.c_str()))
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
			const char* gametype = (const char*)(sqlite3_column_text(stmt, 2));
			const char* wads = (const char*)(sqlite3_column_text(stmt, 3));
			const char* map = (const char*)(sqlite3_column_text(stmt, 4));
			const int players = sqlite3_column_int(stmt, 5);
			const int maxplayers = sqlite3_column_int(stmt, 6);
			const int ping = sqlite3_column_int(stmt, 7);

			newRow.address = address ? address : "";
			newRow.servername = servername ? servername : "";
			newRow.gametype = gametype ? gametype : "";
			newRow.wads = wads ? wads : "";
			newRow.map = map ? map : "";

			if (maxplayers != 0)
			{
				snprintf(buffer, ARRAY_LENGTH(buffer), "%d/%d", players, maxplayers);
				newRow.players = buffer;
			}

			snprintf(buffer, ARRAY_LENGTH(buffer), "%d", ping);
			newRow.ping = ping;

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

NODISCARD bool DB_LockAddressForServerInfo(const uint64_t id, std::string& outAddress)
{
	sqlite3_stmt* stmt = SQLPrepare(::LOCK_SERVER);
	if (!stmt)
		return false;

	auto onExit = makeScopeExit([stmt] { sqlite3_finalize(stmt); });

	if (!SQLBindInt64(stmt, ":lockid", id))
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
