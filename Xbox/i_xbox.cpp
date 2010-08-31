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
#include <list>
#include <errno.h>

#include "i_xbox.h"
#include "i_system.h"

// Xbox drive letters
#define DriveC "\\??\\C:"
#define DriveD "\\??\\D:"
#define DriveE "\\??\\E:"
#define DriveF "\\??\\F:"
#define DriveG "\\??\\G:"
#define DriveT "\\??\\T:"
#define DriveU "\\??\\U:"
#define DriveX "\\??\\X:"

// Partition device mapping
#define DeviceC "\\Device\\Harddisk0\\Partition2"
#define CdRom "\\Device\\CdRom0"
#define DeviceE "\\Device\\Harddisk0\\Partition1"
#define DeviceF "\\Device\\Harddisk0\\Partition6"
#define DeviceG "\\Device\\Harddisk0\\Partition7"
#define DeviceT "\\Device\\Harddisk0\\Partition1\\TDATA\\4F444D58"
#define DeviceU "\\Device\\Harddisk0\\Partition1\\UDATA\\4F444D58"
#define DeviceX "\\Device\\Harddisk0\\Partition3"

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
extern "C" XBOXAPI INT WINAPI XWriteTitleInfoAndRebootA(LPVOID,LPVOID,DWORD,DWORD,LPVOID);
extern "C" XBOXAPI LONG WINAPI HalWriteSMBusValue(UCHAR devddress, UCHAR offset, UCHAR writedw, DWORD data);

// External function declarations
extern int    i_main(int argc, char *argv[]);		// i_main.cpp
extern size_t I_BytesToMegabytes (size_t Bytes);	// i_system.cpp

//Globals
std::list<void (*)(void)>	 ExitFuncList;
DWORD                        LauncherID;
char						*LauncherXBE = NULL;

//
// getenv 
// Environment variables don't exist on Xbox. Return NULL.
//
char *getenv(const char *)
{
	return NULL;
}

//
// putenv 
//
// Environment variables don't exist on Xbox. Just return success.
//
int putenv(const char *)
{
	return 0;
}

//
// getcwd 
//
// Return working directory which is always D:\
//
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

//
// gethostbyname
//
// Custom implementation for Xbox
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
// gethostname
//
// Custom implementation for Xbox
//
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

//
// xbox_PrintMemoryDebug
//
void xbox_PrintMemoryDebug()
{
	extern size_t got_heapsize;
	MEMORYSTATUS  stat;
	static DWORD  lastmem = 0;
	char          buf[100];

	// Get the memory status.
	GlobalMemoryStatus(&stat);

	if (stat.dwAvailPhys != lastmem)
	{
		sprintf(buf, "\nMemory Debug:\n");
		OutputDebugString( buf );

		sprintf(buf, "Heap Size: \t%4d MB\n", got_heapsize);
		OutputDebugString( buf );


		sprintf(buf, "Total Physical Memory: \t%4d bytes / %4d MB\n", stat.dwTotalPhys, I_BytesToMegabytes(stat.dwTotalPhys));
		OutputDebugString( buf );

		sprintf(buf, "Used Physical Memory : \t%4d bytes / %4d MB\n", stat.dwTotalPhys - stat.dwAvailPhys, 
																			I_BytesToMegabytes(stat.dwTotalPhys - stat.dwAvailPhys));
		OutputDebugString( buf );

		sprintf(buf, "Free Physical Memory : \t%4d bytes / %4d MB\n\n", stat.dwAvailPhys, I_BytesToMegabytes(stat.dwAvailPhys));
		OutputDebugString( buf );

		lastmem = stat.dwAvailPhys;
	}
}

//
// xbox_MountDevice
//
// XBox device mounting
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
	xbox_MountDevice(DriveD, CdRom);   // DVD-ROM or start path
	xbox_MountDevice(DriveE, DeviceE); // Standard save partition
	xbox_MountDevice(DriveF, DeviceF); // Non-stock partition - modded consoles only
	xbox_MountDevice(DriveG, DeviceG); // Non-stock partition - modded consoles only
	xbox_MountDevice(DriveT, DeviceT); // Odamex's unique TDATA - peristent save data (configs, etc.)
	xbox_MountDevice(DriveU, DeviceU); // Odamex's unique UDATA - user save data (save games)
	xbox_MountDevice(DriveX, DeviceX); // Cache partition - appropriate place for temporary files
}

//
// xbox_UnMountPartitions
//
void xbox_UnMountPartitions()
{
	xbox_UnMountDevice(DriveD);
	xbox_UnMountDevice(DriveE);
	xbox_UnMountDevice(DriveF);
	xbox_UnMountDevice(DriveG);
	xbox_UnMountDevice(DriveT);
	xbox_UnMountDevice(DriveU);
	xbox_UnMountDevice(DriveX);
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
// xbox_SetScreenPosition
//
int xbox_SetScreenPosition(float x, float y)
{
	float x2, y2;

	SDL_XBOX_SetScreenPosition(x, y);
	SDL_XBOX_GetScreenPosition(&x2, &y2);

	if(x != x2 || y != y2)
		return -1;

	return 0;
}

//
// xbox_SetScreenStretch
//
int xbox_SetScreenStretch(float xs, float ys)
{
	float xs2, ys2;

	SDL_XBOX_SetScreenStretch(xs, ys);
	SDL_XBOX_GetScreenStretch(&xs2, &ys2);

	if(xs != xs2 || ys != ys2)
		return -1;

	return 0;
}

//
// xbox_EnableCustomLED
//
void xbox_EnableCustomLED()
{
	// 0xF0 = Solid Red
	HalWriteSMBusValue(0x20, 0x08, 0, 0xF0);
	Sleep(10);
	HalWriteSMBusValue(0x20, 0x07, 0, 1);
}

//
// xbox_DisableCustomLED
//
void xbox_DisableCustomLED()
{
	HalWriteSMBusValue(0x20, 0x07, 0, 0);
}

//
// xbox_RecordLauncherXBE
//
void xbox_RecordLauncherXBE(char *szLauncherXBE, DWORD dwID)
{
	if(szLauncherXBE  && !LauncherXBE)
	{
		LauncherXBE = strdup(szLauncherXBE);
		LauncherID = dwID;
	}
}

//
// xbox_reboot 
//
// Exit Odamex and perform a warm reboot (no startup logo) to a launcher or dashboard
//
void xbox_reboot()
{
	LD_LAUNCH_DASHBOARD launchData = { XLD_LAUNCH_DASHBOARD_MAIN_MENU };

	// If Odamex was started from a launcher we want to return to it.
	if(LauncherXBE)
	{
		size_t pathLen;
		char  *mntDev;
		char  *p;

		// Determine the necessary D: mapping for the launcher XBE
		p = strrchr(LauncherXBE, '\\');
		pathLen = p - LauncherXBE;

		mntDev = (char *)malloc(pathLen + 1);
		memcpy(mntDev, LauncherXBE, pathLen);
		mntDev[pathLen] = '\0'; // This is necessary

		p++; // Now conveniently Points to the start of our XBE name with path stripped off.

		// Return to the launcher XBE
		XWriteTitleInfoAndRebootA(p, mntDev, LDT_TITLE, LauncherID, &launchData);
	}

	// Return to the dashboard
	XLaunchNewImage( NULL, (LAUNCH_DATA*)&launchData );
}

//
// xbox_atexit 
//
// Custom atexit function for Xbox
//
void xbox_atexit(void (*function)(void))
{
	if(function)
		ExitFuncList.push_back(function);
}

//
// xbox_exit
//
// Custom exit function for Xbox
//
void xbox_exit(int status)
{
	std::list<void (*)(void)>::iterator funcIter;

	for(funcIter = ExitFuncList.begin(); funcIter != ExitFuncList.end(); ++funcIter)
		(*funcIter)();

	xbox_CloseNetwork();

	xbox_UnMountPartitions();

	xbox_DisableCustomLED();

	xbox_reboot();
}

//
// main
//
// Entry point on Xbox
//
void  __cdecl main()
{
	DWORD            launchDataType;
	LAUNCH_DATA      launchData;
	char            *xargv[100];
	int              xargc = 1;

	xargv[0] = strdup("D:\\default.xbe"); // mimic argv[0]

	if(XGetLaunchInfo (&launchDataType, &launchData) == ERROR_SUCCESS)
	{
		if(launchDataType == LDT_FROM_DEBUGGER_CMDLINE) 
			xargv[xargc] = strtok(((PLD_FROM_DEBUGGER_CMDLINE)&launchData)->szCmdLine, " ");
		else if(launchDataType == LDT_TITLE && ((PLD_DEMO)&launchData)->dwID == 0x4F444C43)
		{
			xbox_RecordLauncherXBE(((PLD_DEMO)&launchData)->szLauncherXBE, ((PLD_DEMO)&launchData)->dwID);
			xargv[xargc] = strtok((char*)((PLD_DEMO)&launchData)->Reserved, " ");
		}

		while(xargv[xargc] != NULL)
		{
			xargc++;
			xargv[xargc] = strtok(NULL, " ");
		}
	}

	xbox_MountPartitions();

	xbox_InitNet();

	xbox_EnableCustomLED();

	i_main(xargc, xargv); // Does not return
}

#endif // _XBOX
