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

const int SQL_TRIES = 3;

const char* SERVERS_TABLE =                  //
    "CREATE TABLE servers ("                 //
    "    address TEXT PRIMARY KEY NOT NULL," //
    "    servername TEXT,"                   //
    "    gametype TEXT,"                     //
    "    wads TEXT,"                         //
    "    map TEXT,"                          //
    "    players INTEGER,"                   //
    "    maxplayers INTEGER,"                //
    "    ping INTEGER,"                      //
    "    serverupdate NUMERIC,"              //
    "    masterupdate NUMERIC NOT NULL);";   //

const char* ADD_SERVER =                           //
    "REPLACE INTO servers(address, masterupdate) " //
    "    VALUES(:address, date('now'));";          //

const char* GET_SERVER_LIST =                                    //
    "SELECT "                                                    //
    "    address, servername, gametype, wads, map, "             //
    "    players, maxplayers, ping, serverupdate, masterupdate " //
    "FROM servers;";                                             //

sqlite3* db;

/**
 * @brief Open and create database.
 *
 * @return True if the database was initialized properly.
 */
bool DB_Init()
{
	if (::db != NULL)
	{
		Log_Debug("Database is not NULL.\n");
		return false;
	}

	if (sqlite3_open(":memory:", &::db) != SQLITE_OK)
	{
		Log_Debug("Could not open database (%s).\n", sqlite3_errmsg(db));
		return false;
	}

	if (sqlite3_exec(::db, ::SERVERS_TABLE, NULL, NULL, NULL) != SQLITE_OK)
	{
		Log_Debug("Could not init database (%s).\n", sqlite3_errmsg(db));
		return false;
	}

	return true;
}

/**
 * @brief Add a server address to the database.
 *        If the server already exists, just touch the masterupdate field.
 */
void DB_AddServer(const std::string& address, const uint16_t port)
{
	int res, addressIDX;
	char buffer[64];

	sqlite3_stmt* stmt = NULL;
	res = sqlite3_prepare_v2(::db, ::ADD_SERVER, -1, &stmt, NULL);
	if (res != SQLITE_OK)
	{
		Log_Debug("Could not prepare statement (%s:%s).\n", sqlite3_errstr(res),
		          sqlite3_errmsg(::db));
		goto cleanup;
	}

	addressIDX = sqlite3_bind_parameter_index(stmt, ":address");
	if (addressIDX == 0)
	{
		Log_Debug("Unknown parameter \"address\".\n");
		goto cleanup;
	}

	snprintf(buffer, ARRAY_LENGTH(buffer), "%s:%u", address.c_str(), port);
	res = sqlite3_bind_text(stmt, addressIDX, buffer, -1, SQLITE_TRANSIENT);
	if (res != SQLITE_OK)
	{
		Log_Debug("Could not bind parameter (%s:%s).\n", sqlite3_errstr(res),
		          sqlite3_errmsg(::db));
		goto cleanup;
	}

	for (size_t tries = 0; tries < ::SQL_TRIES; tries++)
	{
		switch (sqlite3_step(stmt))
		{
		case SQLITE_DONE:
			Log_Debug("Added server %s.\n", buffer);
			goto cleanup;
		case SQLITE_BUSY:
			continue;
		default:
			Log_Debug("Tried to add server %s (%s:%s).\n", buffer, sqlite3_errstr(res),
			          sqlite3_errmsg(::db));
			goto cleanup;
		}
	}

cleanup:
	sqlite3_finalize(stmt);
	return;
}

/**
 * @brief Get a list of servers.
 */
void DB_GetServerList(serverRows_t& rows)
{
	int res;
	size_t newRowCount;
	sqlite3_stmt* stmt = NULL;

	res = sqlite3_prepare_v2(::db, ::GET_SERVER_LIST, -1, &stmt, NULL);
	if (res != SQLITE_OK)
	{
		Log_Debug("Could not prepare statement (%s:%s).\n", sqlite3_errstr(res),
		          sqlite3_errmsg(::db));
		goto cleanup;
	}

	res = sqlite3_column_count(stmt);

	newRowCount = 0;
	for (size_t tries = 0; tries < ::SQL_TRIES;)
	{
		switch (sqlite3_step(stmt))
		{
		case SQLITE_ROW: {
			char buffer[64];
			serverRow_t newRow;

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
			goto cleanup;
		default:
			tries++;
			break;
		}
	}

cleanup:
	sqlite3_finalize(stmt);
	return;
}

/**
 * @brief Close database.
 */
void DB_DeInit()
{
	if (db == NULL)
		return;

	sqlite3_close(db);
	db = NULL;
}
