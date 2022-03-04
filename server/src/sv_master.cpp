// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2020 by The Odamex Team.
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
//	SV_MASTER
//
//-----------------------------------------------------------------------------


#include "odamex.h"


#include "d_player.h"
#include "sv_master.h"
#include "c_dispatch.h"
#include "i_system.h"
#include "md5.h"

#define MASTERPORT			15000

// [Russell] - default master list
// This is here for complete master redundancy, including domain name failure
static const char* def_masterlist[] = { "master1.odamex.net", "voxelsoft.com", NULL };

class masterserver
{
public:
	std::string	masterip;
	netadr_t	masteraddr; // address of the master server

	masterserver()
	{
	}
	
	masterserver(const masterserver &other)
		: masterip(other.masterip), masteraddr(other.masteraddr)
	{
	}

	masterserver& operator =(const masterserver &other)
	{
		masterip = other.masterip;
		masteraddr = other.masteraddr;
		return *this;
	}
};

static buf_t ml_message(MAX_UDP_PACKET);

static std::vector<masterserver> masters;

EXTERN_CVAR(sv_natport)
EXTERN_CVAR(port)

// Server appears in the server list when true.
CVAR_FUNC_IMPL(sv_usemasters)
{
	if (network_game)
		SV_InitMasters();
}


BEGIN_COMMAND(addmaster)
{
	if (argc > 1)
		SV_AddMaster(argv[1]);
}
END_COMMAND(addmaster)


BEGIN_COMMAND(delmaster)
{
	if (argc > 1)
		SV_RemoveMaster(argv[1]);
}
END_COMMAND(delmaster)


BEGIN_COMMAND(masters)
{
	SV_ListMasters();
}
END_COMMAND(masters)



//
// SV_InitMaster
//
void SV_InitMasters()
{
	static bool previous_sv_usemasters = (sv_usemasters == 0);

	if (previous_sv_usemasters != sv_usemasters)
	{
		if (sv_usemasters)
		{
			// [Russell] - Add some default masters
			// so we can dump them to the server cfg file if one does not exist
			if (masters.empty())
			{
				for (int i = 0; def_masterlist[i] != NULL; i++)
					SV_AddMaster(def_masterlist[i]);
			}
		}
		else
		{
			Printf(PRINT_HIGH, "Masters will not be contacted because sv_usemasters is 0\n");
		}
	}

	previous_sv_usemasters = (sv_usemasters != 0);
}


//
// SV_AddMaster
//
bool SV_AddMaster(const char *masterip)
{
	if(strlen(masterip) >= MAX_UDP_PACKET)
		return false;

	masterserver m;
	m.masterip = masterip;

	NET_StringToAdr (m.masterip.c_str(), &m.masteraddr);

	if(!m.masteraddr.port)
		I_SetPort(m.masteraddr, MASTERPORT);

	for(size_t i = 0; i < masters.size(); i++)
	{
		if(masters[i].masterip == m.masterip)
		{
			Printf("Master %s [%s] is already on the list", m.masterip.c_str(), NET_AdrToString(m.masteraddr));
			return false;
		}
	}
	
	if(m.masteraddr.ip[0] == 0 && m.masteraddr.ip[1] == 0 && m.masteraddr.ip[2] == 0 && m.masteraddr.ip[3] == 0)
	{
		Printf("Failed to resolve master server: %s, not added", m.masterip.c_str());
		return false;
	}
	else
	{
		Printf("Added master: %s [%s]", m.masterip.c_str(), NET_AdrToString(m.masteraddr));
		masters.push_back(m);
	}

	return true;
}

//
// SV_ArchiveMasters
//
void SV_ArchiveMasters(FILE *fp)
{
	for(size_t i = 0; i < masters.size(); i++)
		fprintf(fp, "addmaster %s\n", masters[i].masterip.c_str());
}

//
// SV_ListMasters
//
void SV_ListMasters()
{
	Printf("Use addmaster/delmaster commands to modify this list");

	for(size_t index = 0; index < masters.size(); index++)
		Printf("%s [%s]", masters[index].masterip.c_str(), NET_AdrToString(masters[index].masteraddr));
}

//
// SV_RemoveMaster
//
bool SV_RemoveMaster(const char *masterip)
{
	for(size_t index = 0; index < masters.size(); index++)
	{
		if(strnicmp(masters[index].masterip.c_str(), masterip, strlen(masterip)) == 0)
		{
			Printf("Removed master server: %s", masters[index].masterip.c_str());
			masters.erase(masters.begin() + index);
			return true;
		}
	}

	Printf("Failed to remove master: %s, not in list", masterip);
	return false;
}

//
// SV_UpdateMasterServer
//
void SV_UpdateMasterServer(masterserver &m)
{
		SZ_Clear(&ml_message);
		MSG_WriteLong(&ml_message, MSG_CHALLENGE);

		// send out actual port, because NAT may present an incorrect port to the master
		if(sv_natport)
			MSG_WriteShort(&ml_message, sv_natport.asInt());
		else
			MSG_WriteShort(&ml_message, port);

		NET_SendPacket(ml_message, m.masteraddr);
}

//
// SV_UpdateMasterServers
//
void SV_UpdateMasterServers()
{
	for (size_t index = 0; index < masters.size(); index++)
		SV_UpdateMasterServer(masters[index]);
}

//
// SV_UpdateMaster
//
void SV_UpdateMaster()
{
	if (!sv_usemasters)
		return;

	const dtime_t current_time = I_GetTime();

	static dtime_t last_address_resolution = 0;

	// update master addresses from names every 3 hours
	if (current_time - last_address_resolution >= I_ConvertTimeFromMs(1000 * 60 * 60 * 3))
	{
		for (size_t index = 0; index < masters.size(); index++)
		{
			NET_StringToAdr(masters[index].masterip.c_str(), &masters[index].masteraddr);
			I_SetPort(masters[index].masteraddr, MASTERPORT);
		}

		last_address_resolution = current_time;
	}

	static dtime_t last_keep_alive = 0;

	// Send to masters every 25 seconds
	if (current_time - last_keep_alive >= I_ConvertTimeFromMs(1000 * 25))
	{
		SV_UpdateMasterServers();

		last_keep_alive = current_time;
	}
}

VERSION_CONTROL (sv_master_cpp, "$Id$")
