// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2011 by The Odamex Team.
// Copyright (C) 2012 by Alex Mayfield.
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
//   Serverside banlist handling.
//
//-----------------------------------------------------------------------------

#ifndef __SV_BANLIST__
#define __SV_BANLIST__

#include <sstream>
#include <string>
#include <vector>

#include "json/json.h"

#include "d_player.h"
#include "i_net.h"

class IPRange
{
private:
	byte ip[4];
	bool mask[4];
public:
	IPRange(void);
	bool check(const netadr_t &address);
	bool check(const std::string &input);
	void set(const netadr_t &address);
	bool set(const std::string &input);
	std::string string(void);
};

struct Ban
{
	Ban(void) : expire(0) { };
	time_t expire;
	std::string name;
	IPRange range;
	std::string reason;
};

struct Exception
{
	std::string name;
	IPRange range;
};

typedef std::pair<size_t, Ban*> banlist_result_t;
typedef std::vector<banlist_result_t> banlist_results_t;
typedef std::pair<size_t, Exception*> exceptionlist_result_t;
typedef std::vector<exceptionlist_result_t> exceptionlist_results_t;

class Banlist
{
public:
	size_t size();
	bool add(const std::string &address, const time_t expire = 0,
	         const std::string &name = std::string(),
	         const std::string &reason = std::string());
	bool add(player_t &player, const time_t expire = 0,
	         const std::string &reason = std::string());
	bool add_exception(const std::string &address,
	                   const std::string &name = std::string());
	bool add_exception(player_t &player);
	bool check(const netadr_t &address, Ban &baninfo);
	bool query(banlist_results_t &result);
	bool query(const std::string &query, banlist_results_t &result);
	bool query_exception(exceptionlist_results_t &result);
	bool query_exception(const std::string &query,
	                     exceptionlist_results_t &result);
	bool remove(size_t index);
	bool remove_exception(size_t index);
	void clear();
	void clear_exceptions();
	bool json(Json::Value &json_bans);
	bool json_replace(const Json::Value &json_bans);
	void json_exceptions();
private:
	std::vector<Ban> banlist;
	std::vector<Exception> exceptionlist;
};

void SV_InitBanlist();
bool SV_BanCheck(client_t* cl, int n);
void SV_BanlistTics();

#endif
