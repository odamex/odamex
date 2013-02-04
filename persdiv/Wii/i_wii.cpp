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
#ifdef GEKKO

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <gccore.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL.h>

#ifdef DEBUG
#include <debug.h>
#endif

#include "i_wii.h"

// External function declarations
extern "C" { extern void __exception_setreload(int t); }
extern int I_Main(int argc, char *argv[]); // i_main.cpp


int wii_getsockname(int socket, struct sockaddr *address, socklen_t *address_len)
{
	return 0;
}

int wii_gethostname(char *name, size_t namelen)
{
	return 0;
}

// scandir() and alphasort() originally obtained from the viewmol project. They have been slightly modified.
// The original source is located here: 
//		http://viewmol.cvs.sourceforge.net/viewvc/viewmol/source/scandir.c?revision=1.3&view=markup
// The license (GPL) is located here: http://viewmol.sourceforge.net/documentation/node2.html

//
// wii_scandir - Custom implementation of scandir()
//

int wii_scandir(const char *dir, struct  dirent ***namelist,
									int (*select)(const struct dirent *),
									int (*compar)(const struct dirent **, const struct dirent **))
{
	DIR *d;
	struct dirent *entry;
	register int i=0;
	size_t entrysize;

	if ((d=opendir(dir)) == NULL)
		return(-1);

	*namelist=NULL;
	while ((entry=readdir(d)) != NULL)
	{
		if (select == NULL || (select != NULL && (*select)(entry)))
		{
			*namelist=(struct dirent **)realloc((void *)(*namelist),
								(size_t)((i+1)*sizeof(struct dirent *)));
			if (*namelist == NULL) return(-1);
				entrysize=sizeof(struct dirent)-sizeof(entry->d_name)+strlen(entry->d_name)+1;
			(*namelist)[i]=(struct dirent *)malloc(entrysize);
			if ((*namelist)[i] == NULL) return(-1);
				memcpy((*namelist)[i], entry, entrysize);
			i++;
		}
	}
	if (closedir(d)) return(-1);
	if (i == 0) return(-1);
	if (compar != NULL)
		qsort((void *)(*namelist), (size_t)i, sizeof(struct dirent *), (int (*)(const void*,const void*))compar);
               
	return(i);
}

//
// wii_alphasort - Custom implementation of alphasort

int wii_alphasort(const struct dirent **a, const struct dirent **b)
{
	return(strcmp((*a)->d_name, (*b)->d_name));
}

void wii_InitNet()
{
	char localip[16] = {0};
	char gateway[16] = {0};
	char netmask[16] = {0};
	
	if(if_config(localip, gateway, netmask, TRUE) >= 0)
	{
#if DEBUG
		// Connect to the remote debug console
		if(net_print_init(NULL,0) >= 0)
			net_print_string( __FILE__, __LINE__, "net_print_init() successful\n");

		// Initialize the debug listener
		DEBUG_Init(100, 5656);
		
#if 0 // Enable this section to debug remotely over a network connection.
		// Wait for the debugger
		_break();
#endif
#endif
	}
}


int main(int argc, char *argv[])
{
	__exception_setreload(8);
	
	wii_InitNet();
	
	if(!fatInitDefault()) 
	{
#if DEBUG
		net_print_string( __FILE__, __LINE__, "Unable to initialise FAT subsystem, exiting.\n");
#endif
		exit(0);
	}
	if(chdir("sd:/"))
	{
#if DEBUG
		net_print_string( __FILE__, __LINE__, "Could not change to root directory, exiting.\n");
#endif
		exit(0);
	}

#if DEBUG
	net_print_string(__FILE__, __LINE__, "Calling I_Main\n");
#endif

	I_Main(argc, argv); // Does not return
	
	return 0;
}

#endif
