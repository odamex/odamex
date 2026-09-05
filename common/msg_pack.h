// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//  General utility for packing protocol messages to string or net buffer
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>

#include "i_net.h"

namespace google
{
	namespace protobuf
	{
		class Message;
	}
}

std::optional<msg_t> MSG_Pack(std::string& serializationBuffer, const google::protobuf::Message& msg);
void                 MSG_Pack(buf_t&       serializationBuffer, const google::protobuf::Message& msg);
