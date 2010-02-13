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

struct hostent *gethostbyname(const char *name)
{
	return NULL; // placeholder -- Hyper_Eye
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

	XGetLaunchInfo (&launchDataType, &launchData);

	if(launchDataType == LDT_FROM_DEBUGGER_CMDLINE) {
		xargv[xargc] = strtok(((PLD_FROM_DEBUGGER_CMDLINE)&launchDataType)->szCmdLine, " ");
		while(xargv[xargc] != NULL) {
			xargc++;
			xargv[xargc] = strtok(NULL, " ");
		}
	}

	xbox_MountPartitions();

	i_main(xargc, xargv);
}

#endif // _XBOX