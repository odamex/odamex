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

#include "odamex.h"

#include "c_rcon.h"
#include "rconprotocol.h"

namespace rcon
{

void Server::start()
{
	m_running = true;
	m_serverThread = std::thread([this]() { this->run(); });
}

void Server::stop()
{
	m_running = false;
	if (m_serverThread.joinable())
	{
		m_serverThread.join();
	}
	if (m_context)
	{
		lws_context_destroy(m_context);
		m_context = nullptr;
	}
}

	// Callback for the websocket protocol
int Server::callback_json_server(
	lws* wsi,
	lws_callback_reasons reason,
	void* user, void* in, size_t len)
{
	switch (reason)
	{
	case LWS_CALLBACK_RECEIVE:
		try
		{
			std::string_view payload((char*)in, len);

			const auto parsed_message = ClientMessage::deserialize(payload);
			if (!parsed_message)
			{
				PrintFmt("JSON parse error: {}\n", parsed_message.error().what());
				break;
			}

			// Echo JSON back
			const ServerMessage reply
			{
				0,
				rcon::messages::server::Print {
					PRINT_HIGH,
					fmt::format("Received command '{}' from RCON client", std::get<rcon::messages::client::Command>(parsed_message->content).command)
				}
			};

			const std::string reply_str = reply.serialize();

			// libwebsockets requires a preallocated buffer
			const size_t buf_size = LWS_PRE + reply_str.size();
			auto buf = std::make_unique<byte[]>(buf_size);
			memcpy(buf.get() + LWS_PRE, reply_str.data(), reply_str.size());

			lws_write(wsi, buf.get() + LWS_PRE, reply_str.size(), LWS_WRITE_TEXT);

		}
		catch (const std::exception& e)
		{
			PrintFmt("Exception: {}\n", e.what());
		}
		break;

	case LWS_CALLBACK_ESTABLISHED:
	    PrintFmt("Client connected\n");
    	break;

	case LWS_CALLBACK_CLOSED:
	    PrintFmt("Client disconnected\n");
	    break;


	default:
		break;
	}

	return 0;
}

void Server::run()
{
	lws_context_creation_info info;
	memset(&info, 0, sizeof(info));
	info.port = m_port;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	// TODO: maybe keep errors and use custom logging func/
	lws_set_log_level(0, nullptr);
	m_context = lws_create_context(&info);
	if (!m_context)
	{
		PrintFmt("Failed to create WebSocket context\n");
		return;
	}

	PrintFmt("RCON server running on TCP port {}\n", m_port);

	while (m_running)
	{
		lws_service(m_context, 0);
	}
}

} // namespace rcon

VERSION_CONTROL (c_rcon_cpp, "$Id$")