// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2025 by The Odamex Team.
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


#include "odamex.h"

#include <ctime>
#include <sstream>

#include "win32inc.h"

#include "i_system.h"

#include "json/json.h"

#include "c_dispatch.h"
#include "cmdlib.h"
#include "d_player.h"
#include "m_fileio.h"
#include "sv_banlist.h"
#include "sv_main.h"

EXTERN_CVAR(sv_email)
EXTERN_CVAR(sv_banfile)
EXTERN_CVAR(sv_banfile_reload)

Banlist banlist;

//// IPRange ////

// Constructor
IPRange::IPRange()
{
	for (byte i = 0; i < 4; i++)
	{
		this->ip[i] = (byte)0;
		this->mask[i] = false;
	}
}

// Check a given address against the ip + range in the object.
bool IPRange::check(const netadr_t &address) const
{
	for (byte i = 0; i < 4; i++)
	{
		if (!(this->ip[i] == address.ip[i] || this->mask[i] == true))
		{
			return false;
		}
	}

	return true;
}

// Check a given string address against the ip + range in the object.
bool IPRange::check(const std::string &address) const
{
	StringTokens tokens = TokenizeString(address, ".");

	// An IP address contains 4 octets
	if (tokens.size() != 4)
	{
		return false;
	}

	for (byte i = 0; i < 4; i++)
	{
		// * means that octet is masked and we will accept any byte
		if (tokens[i].compare("*") == 0)
		{
			continue;
		}

		// Convert string into byte.
		unsigned short octet = 0;
		std::istringstream buffer(tokens[i]);
		buffer >> octet;
		if (!buffer)
		{
			return false;
		}

		if (!(this->ip[i] == octet || this->mask[i] == true))
		{
			return false;
		}
	}

	return true;
}

// Set the object's range to a specific address.
void IPRange::set(const netadr_t &address)
{
	for (byte i = 0; i < 4; i++)
	{
		this->ip[i] = address.ip[i];
		this->mask[i] = false;
	}
}

// Set the object's range against the given address in string form.
bool IPRange::set(const std::string &input)
{
	StringTokens tokens = TokenizeString(input, ".");

	// An IP address contains 4 octets
	if (tokens.size() != 4)
	{
		return false;
	}

	for (byte i = 0; i < 4; i++)
	{
		// * means that octet is masked
		if (tokens[i].compare("*") == 0)
		{
			this->mask[i] = true;
			continue;
		}
		this->mask[i] = false;

		// Convert string into byte.
		unsigned short octet = 0;
		std::istringstream buffer(tokens[i]);
		buffer >> octet;
		if (!buffer)
		{
			return false;
		}

		this->ip[i] = (byte)octet;
	}

	return true;
}

// Return the range as a string, with stars representing masked octets.
std::string IPRange::string() const
{
	std::ostringstream buffer;

	for (byte i = 0; i < 4; i++)
	{
		if (mask[i])
		{
			buffer << '*';
		}
		else
		{
			buffer << (unsigned short)this->ip[i];
		}

		if (i < 3)
		{
			buffer << '.';
		}
	}

	return buffer.str();
}

//// Banlist ////

size_t Banlist::size()
{
	return this->banlist.size();
}

bool Banlist::add(const std::string &address, const time_t expire,
                  const std::string &name, const std::string &reason)
{
	Ban ban;

	// Did we pass a valid address?
	if (!ban.range.set(address))
	{
		return false;
	}

	// Fill in the rest of the ban information
	ban.expire = expire;
	ban.name = name;
	ban.reason = reason;

	// Add the ban to the banlist
	this->banlist.push_back(ban);

	return true;
}

// We have a specific client that we want to add to the banlist.
bool Banlist::add(player_t &player, const time_t expire,
                  const std::string &reason)
{
	// Player must be valid.
	if (!validplayer(player))
	{
		return false;
	}

	// Fill in ban info
	Ban ban;
	ban.expire = expire;
	ban.name = player.userinfo.netname;
	ban.range.set(player.client.address);
	ban.reason = reason;

	// Add the ban to the banlist
	this->banlist.push_back(ban);

	return true;
}

// Add an exception to the banlist by address.
bool Banlist::add_exception(const std::string &address, const std::string &name)
{
	Exception exception;

	// Did we pass a valid address?
	if (!exception.range.set(address))
	{
		return false;
	}

	// Add the exception to the banlist.
	exception.name = name;
	this->exceptionlist.push_back(exception);

	return true;
}

// We have a specific client that we want to add as an exception.
bool Banlist::add_exception(player_t &player)
{
	// Player must be valid.
	if (!validplayer(player))
	{
		return false;
	}

	// Fill in exception info
	Exception exception;
	exception.name = player.userinfo.netname;
	exception.range.set(player.client.address);

	// Add the exception to the banlist.
	this->exceptionlist.push_back(exception);

	return true;
}

// Check a given address against the exception and banlist.  Sets
// baninfo and returns true if passed address is banned, otherwise
// returns false.
bool Banlist::check(const netadr_t &address, Ban &baninfo)
{
	// Check against exception list.
	for (const auto& exception : this->exceptionlist)
	{
		if (exception.range.check(address))
		{
			return false;
		}
	}

	// Check against banlist.
	for (const auto& ban : this->banlist)
	{
		if (ban.range.check(address) && (ban.expire == 0 ||
		                                 ban.expire > time(NULL)))
		{
			baninfo = ban;
			return true;
		}
	}

	return false;
}

// Return a complete list of bans.
bool Banlist::query(banlist_results_t &results)
{
	// No banlist?  Return an error state.
	if (this->banlist.empty())
	{
		return false;
	}

	results.reserve(this->banlist.size());
	for (size_t i = 0; i < banlist.size(); i++)
	{
		results.emplace_back(i, &(this->banlist[i]));
	}

	return true;
}

// Run a query on the banlist and return a list of all matching bans.  Matches
// against IP address/range and partial name.
bool Banlist::query(const std::string &query, banlist_results_t &results)
{
	// No banlist?  Return an error state.
	if (this->banlist.empty())
	{
		return false;
	}

	// No query?  Return everything.
	if (query.empty())
	{
		return this->query(results);
	}

	std::string pattern = "*" + (query) + "*";
	for (size_t i = 0; i < this->banlist.size(); i++)
	{
		bool f_ip = this->banlist[i].range.check(query);
		bool f_name = CheckWildcards(pattern.c_str(),
		                             this->banlist[i].name.c_str());
		if (f_ip || f_name)
		{
			results.emplace_back(i, &(this->banlist[i]));
		}
	}
	return true;
}

// Return a complete list of exceptions.
bool Banlist::query_exception(exceptionlist_results_t &results)
{
	// No banlist?  Return an error state.
	if (this->exceptionlist.empty())
	{
		return false;
	}

	results.reserve(this->exceptionlist.size());
	for (size_t i = 0; i < exceptionlist.size(); i++)
	{
		results.emplace_back(i, &(this->exceptionlist[i]));
	}

	return true;
}

// Run a query on the exceptionlist and return a list of all matching
// exceptions.  Matches against IP address/range and partial name.
bool Banlist::query_exception(const std::string &query,
                              exceptionlist_results_t &results)
{
	// No exceptionlist?  Return an error state.
	if (this->exceptionlist.empty())
	{
		return false;
	}

	// No query?  Return everything.
	if (query.empty())
	{
		return this->query_exception(results);
	}

	std::string pattern = "*" + (query) + "*";
	for (size_t i = 0; i < this->exceptionlist.size(); i++)
	{
		bool f_ip = this->exceptionlist[i].range.check(query);
		bool f_name = CheckWildcards(pattern.c_str(),
		                             this->exceptionlist[i].name.c_str());
		if (f_ip || f_name)
		{
			results.emplace_back(i, &(this->exceptionlist[i]));
		}
	}
	return true;
}

// Remove a ban from the banlist by index.
bool Banlist::remove(size_t index)
{
	// No banlist?  Return an error state.
	if (this->banlist.empty())
	{
		return false;
	}

	// Invalid index?  Return an error state.
	if (this->banlist.size() <= index)
	{
		return false;
	}

	this->banlist.erase(this->banlist.begin() + index);
	return true;
}

// Remove a exception from the banlist by index.
bool Banlist::remove_exception(size_t index)
{
	// No exception list?  Return an error state.
	if (this->exceptionlist.empty())
	{
		return false;
	}

	// Invalid index?  Return an error state.
	if (this->exceptionlist.size() <= index)
	{
		return false;
	}

	this->exceptionlist.erase(this->exceptionlist.begin() + index);
	return true;
}

// Clear the banlist.
void Banlist::clear()
{
	this->banlist.clear();
}

// Clear the exceptionlist.
void Banlist::clear_exceptions()
{
	this->exceptionlist.clear();
}

// Fills a JSON array with bans.
bool Banlist::json(Json::Value &json_bans)
{
	std::string expire;
	tm* tmp;

	for (const auto& ban : this->banlist)
	{
		Json::Value json_ban(Json::objectValue);
		json_ban["range"] = ban.range.string();
		// Expire time is optional.
		if (ban.expire != 0)
		{
			tmp = gmtime(&ban.expire);
			if (StrFormatISOTime(expire, tmp))
			{
				json_ban["expire"] = expire;
			}
		}
		// Name is optional.
		if (!ban.name.empty())
			json_ban["name"] = ban.name;
		// Reason is optional.
		if (!ban.reason.empty())
			json_ban["reason"] = ban.reason;
		json_bans.append(json_ban);
	}

	return true;
}

// Replace the current banlist with the contents of a JSON array.
bool Banlist::json_replace(const Json::Value &json_bans)
{
	tm tmp;

	// Must be an array or null root node
	if (!(json_bans.isArray() || json_bans.isNull()))
		return false;

	this->clear();

	// No bans to parse?
	if (json_bans.isNull() || json_bans.empty())
		return true;

	for (const auto& json_ban : json_bans)
	{
		Ban ban;
		Json::Value value;

		// Range
		value = json_ban.get("range", Json::Value::null);
		if (value.isNull())
			continue;
		else
			ban.range.set(json_ban.get("range", false).asString());

		// Expire time
		value = json_ban.get("expire", Json::Value::null);
		if (!value.isNull())
		{
			if (StrParseISOTime(value.asString(), &tmp))
				ban.expire = timegm(&tmp);
		}

		// Name
		value = json_ban.get("name", Json::Value::null);
		if (!value.isNull())
			ban.name = value.asString();

		// Reason
		value = json_ban.get("reason", Json::Value::null);
		if (!value.isNull())
			ban.reason = value.asString();

		this->banlist.push_back(ban);
	}

	return true;
}

//// Console commands ////

// Ban bans a player by player id.
BEGIN_COMMAND(ban)
{
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// We need at least one argument.
	if (arguments.size() < 1)
	{
		PrintFmt(PRINT_HIGH, "Usage: ban <player id> [ban length] [reason].\n");
		return;
	}

	size_t pid;
	std::istringstream buffer(arguments[0]);
	buffer >> pid;
	if (!buffer)
	{
		PrintFmt(PRINT_HIGH, "ban: need a player id.\n");
		return;
	}

	player_t &player = idplayer(pid);
	if (!validplayer(player))
	{
		PrintFmt(PRINT_HIGH, "ban: {} is not a valid player id.\n", pid);
		return;
	}

	// If a length is specified, turn the length into an expire time.
	time_t tim;
	if (arguments.size() > 1)
	{
		if (!StrToTime(arguments[1], tim))
		{
			PrintFmt(PRINT_HIGH, "ban: invalid ban time (try a period of time like \"2 hours\" or \"permanent\")\n");
			return;
		}
	}
	else
	{
		// Default is a permaban.
		tim = 0;
	}

	// If a reason is specified, add it too.
	std::string reason;
	if (arguments.size() > 2)
	{
		// Account for people who forget their double-quotes.
		arguments.erase(arguments.begin(), arguments.begin() + 2);
		reason = JoinStrings(arguments, " ");
	}

	// Add the ban and kick the player.
	banlist.add(player, tim, reason);
	PrintFmt(PRINT_HIGH, "ban: ban added.\n");

	// If we have a banfile, save the banlist.
	if (sv_banfile.cstring()[0] != 0)
	{
		Json::Value json_bans(Json::arrayValue);
		if (!(banlist.json(json_bans) && M_WriteJSON(sv_banfile.cstring(), json_bans, true)))
			PrintFmt(PRINT_HIGH, "ban: banlist could not be saved.\n");
	}

	SV_KickPlayer(player, reason);
}
END_COMMAND(ban)

// addban adds a ban by IP address.
BEGIN_COMMAND(addban)
{
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// We need at least one argument.
	if (arguments.size() < 1)
	{
		PrintFmt(PRINT_HIGH, "Usage: addban <ip address or ip range> [ban length] [player name] [reason]\n");
		return;
	}

	std::string address = arguments[0];

	// If a length is specified, turn the length into an expire time.
	time_t tim;
	if (arguments.size() > 1)
	{
		if (!StrToTime(arguments[1], tim))
		{
			PrintFmt(PRINT_HIGH, "addban: invalid ban time (try a period of time like \"2 hours\" or \"permanent\")\n");
			return;
		}
	}
	else
	{
		// Default is a permaban.
		tim = 0;
	}

	// If the player's name is specified, add it too.
	std::string name;
	if (arguments.size() > 2)
	{
		name = arguments[2];
	}

	// If a reason is specified, add it too.
	std::string reason;
	if (arguments.size() > 3)
	{
		// Account for people who forget their double-quotes.
		arguments.erase(arguments.begin(), arguments.begin() + 3);
		reason = JoinStrings(arguments, " ");
	}

	if (!banlist.add(address, tim, name, reason))
	{
		PrintFmt(PRINT_HIGH, "addban: invalid address or range.\n");
		return;
	}
	PrintFmt(PRINT_HIGH, "addban: ban added.\n");

	// If we have a banfile, save the banlist.
	if (sv_banfile.cstring()[0] != 0)
	{
		Json::Value json_bans(Json::arrayValue);
		if (!(banlist.json(json_bans) && M_WriteJSON(sv_banfile.cstring(), json_bans, true)))
			PrintFmt(PRINT_HIGH, "addban: banlist could not be saved.\n");
	}
}
END_COMMAND(addban)

// Add an exception for a player by player id.
BEGIN_COMMAND(except)
{
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// We need at least one argument.
	if (arguments.size() < 1)
	{
		PrintFmt(PRINT_HIGH, "Usage: except <player id>.\n");
		return;
	}

	size_t pid;
	std::istringstream buffer(arguments[0]);
	buffer >> pid;
	if (!buffer)
	{
		PrintFmt(PRINT_HIGH, "except: need a player id.\n");
		return;
	}

	player_t &player = idplayer(pid);
	if (!validplayer(player))
	{
		PrintFmt(PRINT_HIGH, "except: {} is not a valid player id.\n", pid);
		return;
	}

	// Add the exception.
	banlist.add_exception(player);
}
END_COMMAND(except)

// addexception adds an exception by IP address.
BEGIN_COMMAND(addexception)
{
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// We need at least one argument.
	if (arguments.size() < 1)
	{
		PrintFmt(PRINT_HIGH, "Usage: addexception <ip address or ip range> [player name]\n");
		return;
	}

	std::string address = arguments[0];

	// If the player's name is specified, add it.
	std::string name;
	if (arguments.size() > 1)
	{
		name = arguments[1];
	}

	banlist.add_exception(address, name);
}
END_COMMAND(addexception)

// Delete a ban
BEGIN_COMMAND(delban)
{
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// We need at least one argument.
	if (arguments.size() < 1)
	{
		PrintFmt(PRINT_HIGH, "Usage: delban <banlist index>\n");
		return;
	}

	size_t bid;
	std::istringstream buffer(arguments[0]);
	buffer >> bid;
	if (!buffer || bid == 0)
	{
		PrintFmt(PRINT_HIGH, "delban: banlist index must be a nonzero number.\n");
		return;
	}

	if (!banlist.remove(bid - 1))
	{
		PrintFmt(PRINT_HIGH, "delban: banlist index does not exist.\n");
		return;
	}

	PrintFmt(PRINT_HIGH, "delban: ban deleted.\n");

	// If we have a banfile, save the banlist.
	if (sv_banfile.cstring()[0] != 0)
	{
		Json::Value json_bans(Json::arrayValue);
		if (!(banlist.json(json_bans) && M_WriteJSON(sv_banfile.cstring(), json_bans, true)))
			PrintFmt(PRINT_HIGH, "delban: banlist could not be saved.\n");
	}
}
END_COMMAND(delban)

BEGIN_COMMAND(delexception)
{
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// We need at least one argument.
	if (arguments.size() < 1)
	{
		PrintFmt(PRINT_HIGH, "delexception: delban <banlist index>\n");
		return;
	}

	size_t bid;
	std::istringstream buffer(arguments[0]);
	buffer >> bid;
	if (!buffer || bid == 0)
	{
		PrintFmt(PRINT_HIGH, "delexception: exception index must be a nonzero number.\n");
		return;
	}

	if (!banlist.remove_exception(bid - 1))
	{
		PrintFmt(PRINT_HIGH, "delexception: exception index does not exist.\n");
		return;
	}
}
END_COMMAND(delexception)

BEGIN_COMMAND(banlist)
{
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	banlist_results_t results;
	if (!banlist.query(JoinStrings(arguments, " "), results))
	{
		PrintFmt(PRINT_HIGH, "banlist: banlist is empty.\n");
		return;
	}

	if (results.empty())
	{
		PrintFmt(PRINT_HIGH, "banlist: no results found.\n");
		return;
	}

	char expire[20];
	tm* tmp;

	for (const auto& [num, ban] : results)
	{
		std::ostringstream buffer;
		buffer << num + 1 << ". " << ban->range.string();

		if (ban->expire == 0)
		{
			strncpy(expire, "Permanent", 19);
		}
		else
		{
			tmp = localtime(&(ban->expire));
			if (!strftime(expire, 20, "%Y-%m-%d %H:%M:%S", tmp))
			{
				strncpy(expire, "???", 19);
			}
		}
		buffer << " " << expire;

		bool has_name = !ban->name.empty();
		bool has_reason = !ban->reason.empty();
		if (has_name || has_reason)
		{
			buffer << " (";
			if (!has_name)
			{
				buffer << "\"" << ban->reason << "\"";
			}
			else if (!has_reason)
			{
				buffer << ban->name;
			}
			else
			{
				buffer << ban->name << ": \"" << ban->reason << "\"";
			}
			buffer << ")";
		}

		PrintFmt(PRINT_HIGH, "{}", buffer.str());
	}
}
END_COMMAND(banlist)

BEGIN_COMMAND(clearbanlist)
{
	banlist.clear();

	PrintFmt(PRINT_HIGH, "clearbanlist: banlist cleared.\n");

	// If we have a banfile, save the banlist.
	if (sv_banfile.cstring()[0] != 0)
	{
		Json::Value json_bans(Json::arrayValue);
		if (!(banlist.json(json_bans) && M_WriteJSON(sv_banfile.cstring(), json_bans, true)))
			PrintFmt(PRINT_HIGH, "clearbanlist: banlist could not be saved.\n");
	}
}
END_COMMAND(clearbanlist)

BEGIN_COMMAND(savebanlist)
{
	std::string banfile;
	if (argc > 1)
		banfile = argv[1];
	else
		banfile = sv_banfile.cstring();

	Json::Value json_bans(Json::arrayValue);
	if (banlist.json(json_bans) && M_WriteJSON(banfile.c_str(), json_bans, true))
		PrintFmt(PRINT_HIGH, "savebanlist: banlist saved to {}.\n", banfile);
	else
		PrintFmt(PRINT_HIGH, "savebanlist: could not save banlist.\n");
}
END_COMMAND(savebanlist)

BEGIN_COMMAND(loadbanlist)
{
	std::string banfile;
	if (argc > 1)
		banfile = argv[1];
	else
		banfile = sv_banfile.cstring();

	Json::Value json_bans;
	if (!M_ReadJSON(json_bans, banfile))
	{
		PrintFmt(PRINT_HIGH, "loadbanlist: could not load banlist.\n");
		return;
	}
	if (!banlist.json_replace(json_bans))
	{
		PrintFmt(PRINT_HIGH, "loadbanlist: malformed banlist file, aborted.\n");
		return;
	}

	size_t bansize = banlist.size();
	size_t jsonsize = json_bans.size();

	if (bansize == jsonsize)
		PrintFmt(PRINT_HIGH, "loadbanlist: loaded {} bans from {}.\n", bansize, banfile);
	else
		PrintFmt(PRINT_HIGH, "loadbanlist: loaded {} bans and skipped {} invalid entries from {}.", bansize, jsonsize - bansize, banfile);
}
END_COMMAND(loadbanlist)

BEGIN_COMMAND(exceptionlist)
{
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	exceptionlist_results_t results;
	if (!banlist.query_exception(JoinStrings(arguments, " "), results))
	{
		PrintFmt(PRINT_HIGH, "exceptionlist: exceptionlist is empty.\n");
		return;
	}

	if (results.empty())
	{
		PrintFmt(PRINT_HIGH, "exceptionlist: no results found.\n");
		return;
	}

	for (const auto& [num, exception] : results)
	{
		std::ostringstream buffer;
		buffer << num + 1 << ". " << exception->range.string();

		if (!exception->name.empty())
		{
			buffer << " (" << exception->name << ")";
		}

		PrintFmt(PRINT_HIGH, "{}", buffer.str());
	}
}
END_COMMAND(exceptionlist)

BEGIN_COMMAND(clearexceptionlist)
{
	banlist.clear_exceptions();
	PrintFmt(PRINT_HIGH, "clearexceptionlist: exceptionlist cleared.\n");
}
END_COMMAND(clearexceptionlist)

// Load banlist
void SV_InitBanlist()
{
	const char* banfile = sv_banfile.cstring();

	if (!banfile)
	{
		PrintFmt(PRINT_HIGH, "SV_InitBanlist: No banlist loaded.\n");
		return;
	}

	Json::Value json_bans;
	if (!M_ReadJSON(json_bans, banfile))
	{
		if (!M_FileExists(banfile))
		{
			if (!M_WriteJSON(banfile, json_bans, true))
				PrintFmt(PRINT_HIGH, "SV_InitBanlist: Could not create new banlist.\n");
			else
				PrintFmt(PRINT_HIGH, "SV_InitBanlist: Initialized new banlist.\n");
		}
		else
		{
			PrintFmt(PRINT_HIGH, "SV_InitBanlist: Could not parse banlist.\n");
		}
		return;
	}
	if (!banlist.json_replace(json_bans))
	{
		PrintFmt(PRINT_HIGH, "SV_InitBanlist: Detected malformed banlist file, ignored.\n");
		return;
	}

	size_t bansize = banlist.size();
	size_t jsonsize = json_bans.size();

	if (bansize == jsonsize)
		PrintFmt(PRINT_HIGH, "SV_InitBanlist: Loaded {} bans from {}.\n", bansize, banfile);
	else
		PrintFmt(PRINT_HIGH, "SV_InitBanlist: Loaded {} bans and skipped {} invalid entries from {}.", bansize, jsonsize - bansize, banfile);
}

// Check to see if a client is on the banlist, and kick them out of the server
// if they are.  Returns true if the player was banned.
bool SV_BanCheck(client_t* cl)
{
	Ban ban;
	if (!banlist.check(cl->address, ban))
	{
		return false;
	}

	std::ostringstream buffer;
	if (ban.expire == 0)
	{
		buffer << "You are indefinitely banned from this server.\n";
	}
	else
	{
		buffer << "You are banned from this server until ";

		char tbuffer[32];
		if (strftime(tbuffer, 32, "%c %Z", localtime(&ban.expire)))
		{
			buffer << tbuffer << ".\n";
		}
		else
		{
			buffer << ban.expire << " seconds after midnight on January 1st, 1970.\n";
		}
	}

	int name = ban.name.compare("");
	int reason = ban.reason.compare("");
	if (name != 0 || reason != 0)
	{
		buffer << "The given reason was: \"";
		if (name != 0 && reason == 0)
		{
			buffer << ban.name;
		}
		else if (name == 0 && reason != 0)
		{
			buffer << ban.reason;
		}
		else
		{
			buffer << ban.name << ": " << ban.reason;
		}
		buffer << "\"\n";
	}

	if (*(sv_email.cstring()) != 0)
	{
		buffer << "The server host can be contacted at ";
		buffer << sv_email.cstring();
		buffer << " if you feel this ban is in error or wish to contest it.";
	}

	// Log the banned connection attempt to the server.
	PrintFmt(PRINT_HIGH, "{} is banned, dropping client.\n", NET_AdrToString(cl->address));

	// Send the message to the client.
	SV_ClientPrintFmt(cl, PRINT_HIGH, "{}", buffer.str());

	return true;
}

// Run every tic to see if we should load the banfile.
void SV_BanlistTics()
{
	const dtime_t min_delta_time = I_ConvertTimeFromMs(1000 * sv_banfile_reload);

	// 0 seconds means not enabled.
	if (min_delta_time == 0)
		return;

	const char* banfile = sv_banfile.cstring();

	// No banfile to automatically read.
	if (!banfile)
		return;

	const dtime_t current_time = I_GetTime();
	static dtime_t last_reload_time = current_time;

	if (current_time - last_reload_time >= min_delta_time)
	{
		last_reload_time = current_time;

		// Load the banlist.
		Json::Value json_bans;
		if (!M_ReadJSON(json_bans, banfile))
		{
			PrintFmt(PRINT_HIGH, "sv_banfile_reload: could not load banlist.\n");
			return;
		}
		if (!banlist.json_replace(json_bans))
		{
			PrintFmt(PRINT_HIGH, "sv_banfile_reload: malformed banlist file, ignored.\n");
			return;
		}
	}
}
