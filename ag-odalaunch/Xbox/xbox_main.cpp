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
// DESCRIPTION:
//	Xbox Support
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <xtl.h>
#include <xstring>

#include "xbox_main.h"

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

// These are undocumented Xbox functions that are not in the XDK includes.
// They can be found by looking through the symbols found in the Xbox libs (xapilib.lib mostly).
extern "C" XBOXAPI LONG WINAPI IoCreateSymbolicLink(IN PUNICODE_STRING SymbolicLinkName,IN PUNICODE_STRING DeviceName);
extern "C" XBOXAPI LONG WINAPI IoDeleteSymbolicLink(IN PUNICODE_STRING SymbolicLinkName);

extern int agol_main(int argc, char *argv[]);

//
// inet_ntoa
//
char *inet_ntoa(struct in_addr in)
{
	static char addr[32];

	sprintf(addr, "%d.%d.%d.%d",
				in.S_un.S_un_b.s_b1,
				in.S_un.S_un_b.s_b2,
				in.S_un.S_un_b.s_b3,
				in.S_un.S_un_b.s_b4);

	return addr;
}

//
// gethostbyname
//
struct hostent *gethostbyname(const char *name)
{
	static struct hostent *he = NULL;
	WSAEVENT               hEvent;
	XNDNS                 *pDns = NULL;
	INT                    err;
	
	if(!name)
		return NULL;

	if(he)
	{
		if(he->h_addr_list)
		{
			if(he->h_addr_list[0])
				free(he->h_addr_list[0]);
			free(he->h_addr_list);
		}
		free(he);
		he = NULL;
	}

	hEvent = WSACreateEvent();
	err = XNetDnsLookup(name, hEvent, &pDns);

	WaitForSingleObject( (HANDLE)hEvent, INFINITE);

	if(pDns && pDns->iStatus == 0)
	{
		he = (struct hostent *)malloc(sizeof(struct hostent));
		if(!he)
		{
			// Failed to allocate!
			return NULL;
		}

		he->h_addr_list = (char **)malloc(sizeof(char*));
		he->h_addr_list[0] = (char *)malloc(sizeof(struct in_addr));

		memcpy(he->h_addr_list[0], pDns->aina, sizeof(struct in_addr));
	}

	XNetDnsRelease(pDns);
	WSACloseEvent(hEvent);

	return he;
}

//
// xbox_MountDevice
//
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

//
// xbox_UnMountDevice
//
LONG xbox_UnMountDevice(LPSTR sSymbolicLinkName)
{
  UNICODE_STRING  symbolicLinkName;
  symbolicLinkName.Buffer  = sSymbolicLinkName;
  symbolicLinkName.Length = (USHORT)strlen(sSymbolicLinkName);
  symbolicLinkName.MaximumLength = (USHORT)strlen(sSymbolicLinkName) + 1;

  return IoDeleteSymbolicLink(&symbolicLinkName);
}

//
// xbox_MountPartitions
//
void xbox_MountPartitions()
{
	xbox_MountDevice(DriveE, DeviceE); // Standard save partition
	xbox_MountDevice(DriveF, DeviceF); // Non-stock partition - modded consoles only
	xbox_MountDevice(DriveD, CdRom); // DVD-ROM or start path
}

//
// xbox_UnMountPartitions
//
void xbox_UnMountPartitions()
{
	xbox_UnMountDevice(DriveE);
	xbox_UnMountDevice(DriveF);
	xbox_UnMountDevice(DriveD);
}

//
// xbox_InitNet
//
void xbox_InitNet()
{
	XNetStartupParams xnsp;

	ZeroMemory( &xnsp, sizeof(xnsp) );
	xnsp.cfgSizeOfStruct = sizeof(xnsp);

	xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
	xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
	xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
	xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
	xnsp.cfgSockMaxSockets = 64; // default = 64
	xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
	xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16

	// Bypass security so we can talk to the outside world as we please
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;

	XNetStartup( &xnsp );
}

//
// xbox_CloseNetwork
//
void xbox_CloseNetwork()
{
	XNetCleanup();
}

//
// xbox_main
//
void __cdecl main()
{
	DWORD            launchDataType;
	LAUNCH_DATA      launchData;
	char            *xargv[100];
	int              xargc = 1;

	xargv[0] = strdup("D:\\default.xbe"); // mimic argv[0]

	if(XGetLaunchInfo (&launchDataType, &launchData) == ERROR_SUCCESS)
	{
		if(launchDataType == LDT_FROM_DEBUGGER_CMDLINE)
		{
			xargv[xargc] = strtok(((PLD_FROM_DEBUGGER_CMDLINE)&launchData)->szCmdLine, " ");

			while(xargv[xargc] != NULL)
			{
				xargc++;
				xargv[xargc] = strtok(NULL, " ");
			}
		}
	}

	xbox_MountPartitions();

	xbox_InitNet();

	agol_main(xargc, xargv); // Normal entry point

	xbox_CloseNetwork();

	xbox_UnMountPartitions();

	XLaunchNewImage(NULL, NULL);
}