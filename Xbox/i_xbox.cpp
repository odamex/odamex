// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	Xbox Support
//
//-----------------------------------------------------------------------------
#ifdef _XBOX

#include <xtl.h>
#include <errno.h>

#include "i_xbox.h"

// Partition device mapping
#define DriveC "\\??\\C:"
#define DeviceC "\\Device\\Harddisk0\\Partition2"
#define DriveD "\\??\\D:"
#define CdRom "\\Device\\CdRom0"
#define DriveE "\\??\\E:"
#define DeviceE "\\Device\\Harddisk0\\Partition1"
#define DriveF "\\??\\F:"
#define DeviceF "\\Device\\Harddisk0\\Partition6"
#define DriveG "\\??\\G:"
#define DeviceG "\\Device\\Harddisk0\\Partition7"

typedef struct _STRING 
{
	USHORT	Length;
	USHORT	MaximumLength;
	PSTR	Buffer;
} UNICODE_STRING, *PUNICODE_STRING, ANSI_STRING, *PANSI_STRING;

extern "C" XBOXAPI LONG WINAPI IoCreateSymbolicLink(IN PUNICODE_STRING SymbolicLinkName,IN PUNICODE_STRING DeviceName);

int i_main(int argc, char *argv[]); // i_main.cpp

// getenv - Environment variables don't exist on Xbox. Return NULL.

char *getenv(const char *)
{
	return NULL;
}

// putenv - Environment variables don't exist on Xbox. Just return success.

int putenv(const char *)
{
	return 0;
}

// getcwd - Return working directory which is always D:\

char *getcwd(char *buf, size_t size)
{
	if(size > 0 && buf)
	{
		if(size >= 3)
		{
			strcpy(buf, "D:\\");
			return buf;
		}
		else
			errno = ERANGE;
	}
	else
		errno = EINVAL;

	return NULL;
}

// Custom implementation of gethostbyname()

struct hostent *gethostbyname(const char *name)
{
	static struct hostent *he = NULL;
	WSAEVENT               hEvent;
	XNDNS                 *pDns = NULL;
	INT                    err;
	int                    i;
	
	if(!name)
		return NULL;

	hEvent = WSACreateEvent();
	err = XNetDnsLookup(name, hEvent, &pDns);

	WaitForSingleObject( (HANDLE)hEvent, INFINITE);

	if(pDns && pDns->iStatus == 0)
	{
		if(he)
		{
			for(i = 0; i < 4; i++)
			{
				if(he->h_addr_list[i])
					free(he->h_addr_list[i]);

				free(he);
			}
		}

		he = (struct hostent *)malloc(sizeof(struct hostent));
		if(!he)
		{
			// Failed to allocate!
			return NULL;
		}

		he->h_addr_list = (char **)malloc(sizeof(char*) * 4);
		for(i = 0; i < 4; i++)
			he->h_addr_list[i] = (char *)malloc(sizeof(unsigned long));

		memcpy(&he->h_addr_list, &pDns->aina[0], sizeof(struct in_addr));

		XNetDnsRelease(pDns);
		WSACloseEvent(hEvent);

		return he;
	}

	XNetDnsRelease(pDns);
	WSACloseEvent(hEvent);

	return NULL;
}

// Custom implementation of gethostname()

int gethostname(char *name, int namelen)
{
	XNADDR xna;
	DWORD  dwState;

	if(name)
	{
		if(namelen > 0)
		{
			dwState = XNetGetTitleXnAddr(&xna);
			XNetInAddrToString(xna.ina, name, namelen);

			return 0;
		}
		else
			errno = EINVAL;
	}
	else
		errno = EFAULT;
	
	return -1;
}

/* XBox device mounting */
LONG xbox_MountDevice(LPSTR sSymbolicLinkName, LPSTR sDeviceName)
{
	UNICODE_STRING deviceName;
	deviceName.Buffer  = sDeviceName;
	deviceName.Length = (USHORT)strlen(sDeviceName);
	deviceName.MaximumLength = (USHORT)strlen(sDeviceName) + 1;

	UNICODE_STRING symbolicLinkName;
	symbolicLinkName.Buffer  = sSymbolicLinkName;
	symbolicLinkName.Length = (USHORT)strlen(sSymbolicLinkName);
	symbolicLinkName.MaximumLength = (USHORT)strlen(sSymbolicLinkName) + 1;

	return IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
}

void xbox_MountPartitions()
{
	xbox_MountDevice(DriveE, DeviceE); // Standard save partition
	xbox_MountDevice(DriveF, DeviceF); // Non-stock partition - modded consoles only
	xbox_MountDevice(DriveD, CdRom); // DVD-ROM or start path
}

void  __cdecl main()
{
	DWORD            launchDataType;
	LAUNCH_DATA      launchData;
	char            *xargv[100];
	int              xargc = 1;

	xargv[0] = strdup("D:\\default.xbe"); // mimic argv[0]

	XGetLaunchInfo (&launchDataType, &launchData);

	if(launchDataType == LDT_FROM_DEBUGGER_CMDLINE) 
	{
		xargv[xargc] = strtok(((PLD_FROM_DEBUGGER_CMDLINE)&launchData)->szCmdLine, " ");
		while(xargv[xargc] != NULL)
		{
			xargc++;
			xargv[xargc] = strtok(NULL, " ");
		}
	}

	xbox_MountPartitions();

	i_main(xargc, xargv);
}

#endif // _XBOX