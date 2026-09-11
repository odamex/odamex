// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021, 2026 by Alex Mayfield and et al.
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
//   Message map.
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_net.h"

namespace google
{
namespace protobuf
{
class Descriptor;
}
} // namespace google

const google::protobuf::Descriptor* MSG_ResolveHeader(const msg_t header);
msg_t MSG_ResolveDescriptor(const google::protobuf::Descriptor* desc);
