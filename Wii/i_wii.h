// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	Wii Support
//
//-----------------------------------------------------------------------------

#ifndef _I_WII_H
#define _I_WII_H

#include <network.h>

#ifdef DEBUG
#include "net_print.h"
#endif

#define socket net_socket
#define bind net_bind
#define close net_close
#define gethostname wii_gethostname
#define gethostbyname net_gethostbyname
#define getsockname wii_getsockname
#define recvfrom net_recvfrom
#define sendto net_sendto
#define select net_select
#define ioctl net_ioctl

#define scandir wii_scandir
#define alphasort wii_alphasort

int wii_getsockname(int socket, struct sockaddr *address, socklen_t *address_len);
int wii_gethostname(char *name, size_t namelen);

int wii_scandir(const char *dir, struct  dirent ***namelist,
							int (*select)(const struct dirent *),
							int (*compar)(const struct dirent **, const struct dirent **));
int wii_alphasort(const struct dirent **a, const struct dirent **b);


#endif
