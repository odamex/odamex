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
//  Websocket server for RCON
//
//-----------------------------------------------------------------------------

#pragma once

#include <libwebsockets.h>
#include <json/json.h>

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
// #include "fmt/compile.h"

inline constexpr uint8_t PROTOCOL_VERSION_MAJOR = 1;
inline constexpr uint8_t PROTOCOL_VERSION_MINOR = 0;
inline constexpr uint8_t PROTOCOL_VERSION_PATCH = 0;
inline constexpr uint32_t PROTOCOL_VERSION = PROTOCOL_VERSION_MAJOR | PROTOCOL_VERSION_MINOR << 8 | PROTOCOL_VERSION_PATCH << 16;
// inline constexpr std::string_view PROTOCOL_VERSION_STR = FMT_STATIC_FORMAT("{}.{}.{}", PROTOCOL_VERSION_MAJOR, PROTOCOL_VERSION_MINOR, PROTOCOL_VERSION_PATCH).str();

namespace rcon
{

class Server {
public:
	explicit Server(uint16_t port, std::string password) noexcept :
		m_port(port), m_password(password) {}

	~Server()
	{
		stop();
	}

	void start();
	void stop();

	void changePassword(std::string new_password)
	{
		stop();
		m_password = new_password;
		start();
	};

	void getCommandQueue();
	void queueResponse(std::string_view response) {};

	static void Init(uint16_t port, std::string password)
	{
		singleton = std::make_unique<Server>(port, password);
		singleton->start();
	};

	static void Shutdown() { singleton.reset(); };
	static Server* GetInstance() noexcept { return singleton.get(); };

private:
	uint16_t m_port;
	lws_context* m_context = nullptr;
	std::thread m_serverThread;
	std::atomic<bool> m_running = false;
	std::string m_password;

	inline static std::unique_ptr<Server> singleton;

	// Callback for the websocket protocol
	static int callback_json_server(
		struct lws* wsi,
		enum lws_callback_reasons reason,
		void* user, void* in, size_t len);

	// Protocols recognized by this server
	inline static lws_protocols protocols[] = {
		{
			"odamex-rcon",
			Server::callback_json_server,
			0,
			65536,
			PROTOCOL_VERSION,
			nullptr,
			0
		},
		LWS_PROTOCOL_LIST_TERM
	};

	void run();
};

} // namespace rcon