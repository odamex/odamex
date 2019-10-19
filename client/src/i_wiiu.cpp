// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2019 by The Odamex Team.
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
//
// DESCRIPTION:
//	Main WiiU Support file.
// TODO WIIU - FIX NETWORK
//
//-----------------------------------------------------------------------------

#ifdef __WIIU__

#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/systeminfo.h>
#include <nn/ac.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/sdcard.h>

#include <dirent.h>
#include <unistd.h>

#include <thread>

#include "i_wiiu.h"

#include <SDL.h>

#include "doomtype.h"

extern int I_Main(int argc, char *argv[]); // i_main.cpp

// Those functions are stubs, so that we don't really care for now...
int wiiu_getsockname(int socket, struct sockaddr *address, socklen_t *address_len)
{
	return 0;
}

int wiiu_gethostname(char *name, size_t namelen)
{
	return 0;
}

hostent *wiiu_gethostbyname(const char *addrString)
{
	return NULL;
}

int32_t wiiu_ioctl(int32_t s, uint32_t cmd, void *argp) {
	return 0;
}

int main(int argc, char *argv[])
{

	/*  Init logging. We log both to Cafe's internal logger (shows up in Decaf, some
		crash logs) and over UDP to be received with udplogserver. */
	WHBLogCafeInit();
	WHBLogUdpInit();
	/*  WHBLogPrint and WHBLogPrintf add new line characters for you */
	WHBLogPrint("UDP Logging initialised for Odamex WiiU.");

	/*  Init WHB's ProcUI wrapper. This will manage all the little Cafe OS bits and
		pieces for us - home menu overlay, power saving features, etc. */
	WHBProcInit();

#ifdef GCONSOLE
	WHBLogPrint("GCONSOLE flag properly initialized. GOOD.");
#endif

	WHBLogPrint("Trying to mount the SD Card...");
	if (!WHBMountSdCard()) {
		WHBLogPrint("Cannot mount the SD Card, exiting.");
		return 1;
	}

	char *sdRootPath = NULL;
	sdRootPath = WHBGetSdCardMountPath();
	WHBLogPrint(sdRootPath);


	WHBLogPrint("Trying to access folder...");
	if (chdir(WII_DATAPATH))
	{
		WHBLogPrint("Cannot access directory WII_DATAPATH... Exiting.");
		return 1;
	}

	WHBLogPrint("Starting Odamex...");
	I_Main(argc, argv); // Does not return
	return 1;

}

#endif