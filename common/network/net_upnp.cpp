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

#include "doomdef.h"
#include "m_ostring.h"
#include "m_random.h"
#include "c_cvars.h"
#include "cmdlib.h"

#include "network/net_log.h"
#include "network/net_socketaddress.h"
#include "network/net_upnp.h"


EXTERN_CVAR(sv_upnp)
EXTERN_CVAR(sv_upnp_discovertimeout)
EXTERN_CVAR(sv_upnp_description)
EXTERN_CVAR(sv_upnp_internalip)
EXTERN_CVAR(sv_upnp_externalip)


//
// UPNPInterface::UPNPInterface
//
UPNPInterface::UPNPInterface(const SocketAddress& local_address) :
    mInitialized(false),
    mLocalAddress(local_address)
{
#ifdef ODA_HAVE_MINIUPNP
    if (!sv_upnp)
        return;
	int res = 0;

	memset(&mUrls, 0, sizeof(struct UPNPUrls));
	memset(&mData, 0, sizeof(struct IGDdatas));

	Net_Printf("UPnP: Discovering router (max 1 unit supported).");

	struct UPNPDev* devlist = upnpDiscover(sv_upnp_discovertimeout.asInt(), NULL, NULL, 0, 0, &res);

	if (!devlist || res != UPNPDISCOVER_SUCCESS)
	{
		Net_Printf("UPnP: Router not found or timed out, error %d.", res);
		return;
	}

	struct UPNPDev* dev = devlist;

	while (dev)
	{
		if (strstr(dev->st, "InternetGatewayDevice"))
			break;
		dev = dev->pNext;
	}

	if (!dev)
		dev = devlist;	// defaulting to first device

	int descXMLsize = 0;
	char* descXML = (char *)miniwget(dev->descURL, &descXMLsize, 0);

	if (descXML)
	{
		parserootdesc(descXML, descXMLsize, &mData);
		free(descXML);
		descXML = NULL;
		GetUPNPUrls(&mUrls, &mData, dev->descURL, 0);
	}

	freeUPNPDevlist(devlist);

	char IPAddress[40];
	if (UPNP_GetExternalIPAddress(mUrls.controlURL, mData.first.servicetype, IPAddress) == 0)
	{
		Net_Printf("UPnP: Router found, external IP address is: %s.", IPAddress);

		// Store ip address just in case admin wants it
		sv_upnp_externalip.ForceSet(IPAddress);
		return;
	}
	else
	{
		Net_Printf("UPnP: Router found but unable to get external IP address.");
		return;
	}

	if (mUrls.controlURL == NULL)
		return;

	char port_str[6], ip_str[16];
	sprintf(port_str, "%d", mLocalAddress.getPort());
	sprintf(ip_str, "%d.%d.%d.%d",
			(mLocalAddress.getIPAddress() >> 24) & 0xFF,
			(mLocalAddress.getIPAddress() >> 16) & 0xFF,
			(mLocalAddress.getIPAddress() >> 8 ) & 0xFF,
			(mLocalAddress.getIPAddress()      ) & 0xFF);

	sv_upnp_internalip.Set(ip_str);

	// Set a description if none exists
	if (!sv_upnp_description.cstring()[0])
	{
		char desc[40];
		sprintf(desc, "Odasrv (%s:%s)", ip_str, port_str);
		sv_upnp_description.Set(desc);
	}

	res = UPNP_AddPortMapping(mUrls.controlURL, mData.first.servicetype,
            port_str, port_str, ip_str, sv_upnp_description.cstring(), "UDP", NULL, 0);

	if (res == 0)
	{
		Net_Printf("UPnP: Port mapping added to router: %s", sv_upnp_description.cstring());
		Net_Printf("NetInterface: Initialized uPnP.");
		return;
	}
	else
	{
		Net_Printf("UPnP: AddPortMapping failed: %d", res);
		return;
	}
#endif	// ODA_HAVE_MINIUPNP
}


//
// UPNPInterface::~UPNPInterface
//
UPNPInterface::~UPNPInterface()
{
#ifdef ODA_HAVE_MINIUPNP
	if (mUrls.controlURL == NULL)
		return;

	char port_str[16];
	sprintf(port_str, "%d", mLocalAddress.getPort());

	int res = UPNP_DeletePortMapping(mUrls.controlURL, mData.first.servicetype, port_str, "UDP", 0);
	if (res == 0)
	{
		return;
	}
	else
	{
		Net_Printf("UPnP: DeletePortMapping failed: %d", res);
		return;
    }
#endif	// ODA_HAVE_MINIUPNP
}