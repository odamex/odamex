// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//
// uPnP Network Functionality 
//
//-----------------------------------------------------------------------------

#ifndef __NET_UPNP_H__
#define __NET_UPNP_H__

#include "network/net_socketaddress.h"

#ifdef ODA_HAVE_MINIUPNP
    #define MINIUPNP_STATICLIB
    #include "miniwget.h"
    #include "miniupnpc.h"
    #include "upnpcommands.h"
#endif  // ODA_HAVE_MINIUPNP


class UPNPInterface
{
public:
    UPNPInterface(const SocketAddress& local_address);
    virtual ~UPNPInterface();

private:
    bool                mInitialized;
    SocketAddress       mLocalAddress;

#ifdef ODA_HAVE_MINIUPNP
    struct UPNPUrls     mUrls;
    struct IGDdatas     mData;
#endif  // ODA_HAVE_MINIUPNP
};

#endif  // __NET_UPNP_H__