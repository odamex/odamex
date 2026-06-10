// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jim Thoenen.
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
//  Utility for parsing messages out of the network buffer.
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_net.h"

enum parseError_e
{
	PERR_OK,
	PERR_UNKNOWN_HEADER,
	PERR_UNKNOWN_MESSAGE,
	PERR_BAD_DECODE
};

namespace google
{
	namespace protobuf
	{
		class Message;
	}
}

/**
 * @brief Given a message type, read a message from the MSG_ socket and
 *        return a decoded message in "out".
 *
 * @param out Output message - will not be modified unless successful.
 * @param cmd Command to parse out.
 * @return Error condition, or OK (0) if successful.
 */
parseError_e MSG_ParseMessage(google::protobuf::Message*& out, const svc_t cmd);
