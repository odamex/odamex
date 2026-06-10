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

#include "svc_parse.h"

#include <google/protobuf/message.h>

#include "svc_map.h"

parseError_e SVC_ParseMessage(google::protobuf::Message*& out, const svc_t cmd)
{
	// A message factory + Descriptor gives us the proper message.
	google::protobuf::MessageFactory* factory =
	    google::protobuf::MessageFactory::generated_factory();
	const google::protobuf::Descriptor* desc = MSG_ResolveHeader(cmd);
	if (desc == NULL)
	{
		return PERR_UNKNOWN_HEADER;
	}

	// Can we get the mssage prototype from the descriptor?
	const google::protobuf::Message* defmsg = factory->GetPrototype(desc);
	if (defmsg == NULL)
	{
		return PERR_UNKNOWN_MESSAGE;
	}

	const size_t size   = MSG_ReadUnVarint();
	const void*  buffer = MSG_ReadChunk(size);

	// Allocated with "new" - can't be null, and we own it.
	google::protobuf::Message* msg = defmsg->New();
	if (!msg->ParseFromArray(buffer, size))
	{
		return PERR_BAD_DECODE;
	}

	out = msg;
	return PERR_OK;
}
