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
	I_Main(argc, argv); // Does not return
	return 0;
}

#endif