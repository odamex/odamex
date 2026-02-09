// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//  Odamex RCON protocol
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomtype.h"
#include "doomfunc.h"
#include "json/json.h"
#include <nonstd/expected.hpp>
#include <variant>

// This is separate from the RCON protocol used by the client
// It's here in common for unit testing purposes
#if defined(SERVER_APP) || defined(TEST_APP)

namespace rcon
{

namespace messages
{

class ParseError {
	std::string message;
public:
	explicit ParseError(std::string m): message(m) {}
	std::string_view what() const noexcept { return message; }
};

struct ProtocolVersion
{
	uint8_t major = 0, minor = 0, patch = 0;
	Json::Value serialize() const;
	static nonstd::expected<ProtocolVersion, ParseError> deserialize(const Json::Value&);
	// TODO: replace with = default in C++20
	bool operator==(const ProtocolVersion& other) const
	{
		return major == other.major && minor == other.minor && patch == other.patch;
	};
};

#define MESSAGE_TYPE_NAME(name) \
	static constexpr const char* MESSAGE_TYPE = name; \
	static constexpr uint32_t MESSAGE_TYPE_HASH = OUtil::CONST_HASH(MESSAGE_TYPE);

namespace client
{

struct LoginRequest
{
	MESSAGE_TYPE_NAME("login_request")
	ProtocolVersion version;
	Json::Value serialize() const;
	static nonstd::expected<LoginRequest, ParseError> deserialize(const Json::Value&);
	// TODO: replace with = default in C++20
	bool operator==(const LoginRequest& other) const
	{
		return version == other.version;
	};
};

struct LoginPassword
{
	MESSAGE_TYPE_NAME("login_password")
	std::string password;
	Json::Value serialize() const;
	static nonstd::expected<LoginPassword, ParseError> deserialize(const Json::Value&);
	// TODO: replace with = default in C++20
	bool operator==(const LoginPassword& other) const
	{
		return password == other.password;
	};
};

struct Command
{
	MESSAGE_TYPE_NAME("command")
	std::string command;
	Json::Value serialize() const;
	static nonstd::expected<Command, ParseError> deserialize(const Json::Value&);
	// TODO: replace with = default in C++20
	bool operator==(const Command& other) const
	{
		return command == other.command;
	};
};

struct Maplist {
	MESSAGE_TYPE_NAME("maplist")
	Json::Value serialize() const;
	static nonstd::expected<Maplist, ParseError> deserialize(const Json::Value&);
	// TODO: replace with = default in C++20
	bool operator==(const Maplist&) const { return true; }
};

using Message = std::variant<LoginRequest, LoginPassword, Command, Maplist>;

} // namespace client

namespace server
{

struct LoginResponse
{
	MESSAGE_TYPE_NAME("login_response")
	uint64_t nonce;
	Json::Value serialize() const;
	static nonstd::expected<LoginResponse, ParseError> deserialize(const Json::Value&);
	// TODO: replace with = default in C++20
	bool operator==(const LoginResponse& other) const
	{
		return nonce == other.nonce;
	};
};

struct LoginFailure
{
	MESSAGE_TYPE_NAME("login_failure")
	std::string message;
	Json::Value serialize() const;
	static nonstd::expected<LoginFailure, ParseError> deserialize(const Json::Value&);
	// TODO: replace with = default in C++20
	bool operator==(const LoginFailure& other) const
	{
		return message == other.message;
	};
};

// TODO: should this include something like sv_hostname? for printing "Successfully connect to <hostname>"
struct LoginSuccess {
	MESSAGE_TYPE_NAME("login_success")
	Json::Value serialize() const;
	static nonstd::expected<LoginSuccess, ParseError> deserialize(const Json::Value&);
	// TODO: replace with = default in C++20
	bool operator==(const LoginSuccess&) const { return true; }
};

struct Print
{
	MESSAGE_TYPE_NAME("print")
	printlevel_t printlevel;
	std::string text;

	Json::Value serialize() const;
	static nonstd::expected<Print, ParseError> deserialize(const Json::Value&);
	// TODO: replace with = default in C++20
	bool operator==(const Print& other) const
	{
		return printlevel == other.printlevel && text == other.text;
	};
};

struct Maplist {
	MESSAGE_TYPE_NAME("maplist")
	Json::Value serialize() const;
	static nonstd::expected<Maplist, ParseError> deserialize(const Json::Value&);
	// TODO: replace with = default in C++20
	bool operator==(const Maplist&) const { return true; };
};

using Message = std::variant<LoginResponse, LoginFailure, LoginSuccess, Print, Maplist>;

} // namespace server

#undef MESSAGE_TYPE_NAME

template <typename T, typename = std::enable_if_t<
	std::is_same_v<T, server::Message> ||
	std::is_same_v<T, client::Message>
>>
struct Message
{
	uint64_t id;
	T content;

	// TODO: replace with = default in C++20
	bool operator==(const Message& other) const
	{
		return id == other.id && content == other.content;
	}

	std::string serialize(bool pretty = false) const;
	static nonstd::expected<Message, ParseError> deserialize(std::string_view);
};

} // namespace messages

using ServerMessage = messages::Message<messages::server::Message>;
using ClientMessage = messages::Message<messages::client::Message>;

} // namespace rcon

#endif