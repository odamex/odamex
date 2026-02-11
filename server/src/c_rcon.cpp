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

std::optional<std::variant<Server::Print, Server::Command>> Server::getCommandQueue()
{
	if (!m_running)
		return std::nullopt;
	if (auto thingy = tomainthread.try_pop())
	{
		return thingy.value();
	}
	return std::nullopt;
}

// Callback for the websocket protocol
int Server::connection_callback(
	lws* wsi,
	lws_callback_reasons reason,
	void* user, void* in, size_t len)
{
	Server* self = static_cast<Server*>(lws_context_user(lws_get_context(wsi)));
	switch (reason)
	{
	case LWS_CALLBACK_RECEIVE:
		try
		{
			std::string_view payload((char*)in, len);

			const auto parsed_message = ClientMessage::deserialize(payload);
			if (!parsed_message)
			{
				self->log("JSON parse error: {}\n", parsed_message.error().what());
				break;
			}

			// // Echo JSON back
			// const ServerMessage reply
			// {
			// 	0,
			// 	rcon::messages::server::Print {
			// 		PRINT_HIGH,
			// 		fmt::format("Received command '{}' from RCON client", std::get<rcon::messages::client::Command>(parsed_message->content).command)
			// 	}
			// };

			self->tomainthread.emplace(Command{std::get<rcon::messages::client::Command>(parsed_message->content).command});

			// const std::string reply_str = reply.serialize();

			// // libwebsockets requires a preallocated buffer
			// const size_t buf_size = LWS_PRE + reply_str.size();
			// auto buf = std::make_unique<byte[]>(buf_size);
			// memcpy(buf.get() + LWS_PRE, reply_str.data(), reply_str.size());

			// lws_write(wsi, buf.get() + LWS_PRE, reply_str.size(), LWS_WRITE_TEXT);

		}
		catch (const std::exception& e)
		{
			self->log("Exception: {}\n", e.what());
		}
		break;

	case LWS_CALLBACK_ESTABLISHED:
		self->log("Client connected\n");
		{
			std::scoped_lock lock(self->clients_mutex);
			static_cast<ConnectionData*>(user)->wsi = wsi;
			self->clients[static_cast<ConnectionData*>(user)];
		}
    	break;

	case LWS_CALLBACK_CLOSED:
		self->log("Client disconnected\n");
		{
			std::scoped_lock lock(self->clients_mutex);
			self->clients.erase(static_cast<ConnectionData*>(user));
		}
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			Print print;
			{
				std::scoped_lock lock(self->clients_mutex);
				auto& queue = self->clients[static_cast<ConnectionData*>(user)];
				if (queue.empty())
					break;

				print = std::move(queue.front());
				queue.pop();
			}

			const ServerMessage reply
			{
				0,
				rcon::messages::server::Print {
					print.level,
					print.text
				}
			};

			const std::string reply_str = reply.serialize();

			// libwebsockets requires a preallocated buffer
			const size_t buf_size = LWS_PRE + reply_str.size();
			auto buf = std::make_unique<byte[]>(buf_size);
			memcpy(buf.get() + LWS_PRE, reply_str.data(), reply_str.size());

			lws_write(wsi, buf.get() + LWS_PRE, reply_str.size(), LWS_WRITE_TEXT);
		}
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
	info.user = this;

	// TODO: maybe keep errors and use custom logging func
	lws_set_log_level(0, nullptr);
	m_context = lws_create_context(&info);
	if (!m_context)
	{
		log("Failed to create WebSocket context\n");
		return;
	}

	log("RCON server running on TCP port {}\n", m_port);

	while (m_running)
	{
		if (auto idk = frommainthread.try_pop())
		{
			std::scoped_lock lock(clients_mutex);
			for (auto& [client, queue] : clients) {
				queue.push(std::get<Print>(idk.value()));
				lws_callback_on_writable(client->wsi);
			}
		}

		lws_service(m_context, 0);
	}
}

} // namespace rcon

VERSION_CONTROL (c_rcon_cpp, "$Id$")