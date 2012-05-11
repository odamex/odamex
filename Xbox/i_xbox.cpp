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
//	Xbox Support
//
//-----------------------------------------------------------------------------
#ifdef _XBOX

#include <xtl.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <list>
#include <errno.h>

#include "i_xbox.h"
#include "i_system.h"

using namespace std;

// Xbox drive letters
#define DriveC "\\??\\C:"
#define DriveD "\\??\\D:"
#define DriveE "\\??\\E:"
#define DriveF "\\??\\F:"
#define DriveG "\\??\\G:"
#define DriveT "\\??\\T:"
#define DriveU "\\??\\U:"
#define DriveZ "\\??\\Z:"

// Partition device mapping
#define DeviceC "\\Device\\Harddisk0\\Partition2"
#define CdRom "\\Device\\CdRom0"
#define DeviceE "\\Device\\Harddisk0\\Partition1"
#define DeviceF "\\Device\\Harddisk0\\Partition6"
#define DeviceG "\\Device\\Harddisk0\\Partition7"
#define DeviceT "\\Device\\Harddisk0\\Partition1\\TDATA\\4F444D58"
#define DeviceU "\\Device\\Harddisk0\\Partition1\\UDATA\\4F444D58"
#define DeviceZ "\\Device\\Harddisk0\\Partition5"

// Custom LAUNCH_DATA struct for external XBE execution from AG_Execute()
#define AG_LAUNCH_MAGIC 0x41474152

typedef struct {
	DWORD magic;               // Test this against AG_LAUNCH_MAGIC to know this special struct was used
	DWORD dwID;                // The Title ID of the launcher XBE
	CHAR  szLauncherXBE[256];  // The full path to the launcher XBE
	CHAR  szLaunchedXBE[256];  // The full path to the launched XBE
	CHAR  szCmdLine[MAX_LAUNCH_DATA_SIZE - 520]; // The command-line parameters
} AG_LAUNCH_DATA, *PAG_LAUNCH_DATA;

// Standard homebrew custom LAUNCH_DATA designed to launch emu's directly into a game.
// This is used to pass custom parameters from many popular dashboards such as XBMC.
#define CUSTOM_LAUNCH_MAGIC 0xEE456777

typedef struct {
	DWORD magic;
	CHAR szFilename[300];
	CHAR szLaunchXBEOnExit[100];
	CHAR szRemap_D_As[350];
	BYTE country;
	BYTE launchInsertedMedia;
	BYTE executionType;
	CHAR reserved[MAX_LAUNCH_DATA_SIZE-757];
} CUSTOM_LAUNCH_DATA, *PCUSTOM_LAUNCH_DATA;

#define INVALID_FILE_ATTRIBUTES -1

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
extern int    I_Main(int argc, char *argv[]);		// i_main.cpp
extern size_t I_BytesToMegabytes (size_t Bytes);	// i_system.cpp

//Globals
static std::list<void (*)(void)>  ExitFuncList;
static DWORD                      LauncherID;
static char                      *LauncherXBE = NULL;
static bool                       Xbox_RROD = false; // Novelty - Red Ring of DOOM!

//
// xbox_Getenv 
// Environment variables don't exist on Xbox. Return NULL.
//
char *xbox_Getenv(const char *)
{
	return NULL;
}

//
// xbox_Putenv 
//
// Environment variables don't exist on Xbox. Just return success.
//
int xbox_Putenv(const char *)
{
	return 0;
}

//
// xbox_GetCWD 
//
// Return working directory which is always D:\
//
char *xbox_GetCWD(char *buf, size_t size)
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
// xbox_InetNtoa
//
char *xbox_InetNtoa(struct in_addr in)
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
// xbox_GetHostByName
//
// Custom implementation for Xbox
//
struct hostent *xbox_GetHostByName(const char *name)
{
	static struct hostent *he = NULL;
	unsigned long          addr = INADDR_NONE;
	WSAEVENT               hEvent;
	XNDNS                 *pDns = NULL;
	INT                    err;
	
	if(!name)
		return NULL;

	// This data is static and it should not be freed.
	if(!he)
	{
		he = (struct hostent *)malloc(sizeof(struct hostent));
		if(!he)
		{
			// Failed to allocate!
			return NULL;
		}

		he->h_addr_list = (char **)malloc(sizeof(char*));
		he->h_addr_list[0] = (char *)malloc(sizeof(struct in_addr));
	}

	if(isdigit(name[0]))
		addr = inet_addr(name);

	if(addr != INADDR_NONE)
		*(int *)he->h_addr_list[0] = addr;
	else
	{
		hEvent = WSACreateEvent();
		err = XNetDnsLookup(name, hEvent, &pDns);

		WaitForSingleObject( (HANDLE)hEvent, INFINITE);

		if(!pDns || pDns->iStatus != 0)
			return NULL;

		memcpy(he->h_addr_list[0], pDns->aina, sizeof(struct in_addr));

		XNetDnsRelease(pDns);
		WSACloseEvent(hEvent);
	}

	return he;
}

//
// xbox_GetHostname
//
// Custom implementation for Xbox
//
int xbox_GetHostname(char *name, int namelen)
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

		sprintf(buf, "Free Physical Memory : \t%4d bytes / %4d MB\n", stat.dwAvailPhys, I_BytesToMegabytes(stat.dwAvailPhys));
		OutputDebugString( buf );

		OutputDebugString("\n");

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
// Some of these partitions are automatically mounted but just 
// to be on the safe side the mount will be attempted anyway
//
// Some of these partitions are only found on some modded consoles.
// These partitions will only successfully mount on consoles where
// the modder has created these partitions.
//
// F is likely to be found on systems that shipped with stock hdd's
// that were 10GB in size. Since the Xbox hdd was advertised as 8GB,
// 2GB were left unpartitioned on these and that extra space will
// typically be partitioned into F: during modding and games or apps
// will be installed there (like this one.) F: will also be found
// on non-stock hdd's of any size where any space above 8GB and below
// 137GB will be partitioned into F:
//
// G is likely to be found on non-stock hdd's that are larger than 137GB
// due to the partition size limitation. Anything above 137GB is partitioned
// into G: and it is used, typically, for the same purposes as F.
//
void xbox_MountPartitions()
{
	xbox_MountDevice(DriveD, CdRom);   // DVD-ROM or start path - automounted
	xbox_MountDevice(DriveE, DeviceE); // Standard save partition
	xbox_MountDevice(DriveF, DeviceF); // Non-stock partition - modded consoles only
	xbox_MountDevice(DriveG, DeviceG); // Non-stock partition - modded consoles only
	xbox_MountDevice(DriveT, DeviceT); // Odamex's unique TDATA - peristent save data (configs, etc.) - automounted
	xbox_MountDevice(DriveU, DeviceU); // Odamex's unique UDATA - user save data (save games) - automounted
	xbox_MountDevice(DriveZ, DeviceZ); // Cache partition - appropriate place for temporary files - automounted
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
	xbox_UnMountDevice(DriveZ);
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
// xbox_WriteSaveMeta
//
void xbox_WriteSaveMeta(string path, string text)
{
	if(!path.size() || !text.size())
		return;

	string   filename = path + PATHSEP + "SaveMeta.xbx";
	ofstream metafile(filename.c_str());

	if(metafile.fail())
	{
		I_Error ("Failed to create %s save meta:\n%s",
			   filename.c_str(), strerror (errno));
		return;
	}

	metafile << "Name=" << text;

	metafile.close();
}

//
// xbox_GetSavePath
//
string xbox_GetSavePath(string file, int slot)
{
	ostringstream path;
	DWORD         attrs;

	path << setiosflags(ios::right);
	path << setfill('0');

	path << "U:" << PATHSEP << setw(12) << slot;

	if((attrs = GetFileAttributes(path.str().c_str())) == INVALID_FILE_ATTRIBUTES)
	{
		if(!SUCCEEDED(CreateDirectory(path.str().c_str(), NULL)))
		{
			I_FatalError ("Failed to create %s directory:\n%s",
						   path.str().c_str(), strerror (errno));
		}

		xbox_WriteSaveMeta(path.str(), "Empty Slot");
	}
	else
	{
		if (!(attrs & FILE_ATTRIBUTE_DIRECTORY))
			I_FatalError ("%s must be a directory", path.str().c_str());
	}

	path << PATHSEP << file;

	return path.str();
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
void xbox_Reboot()
{
	LD_LAUNCH_DASHBOARD launchData = { XLD_LAUNCH_DASHBOARD_MAIN_MENU };

	// If Odamex was started from a launcher we want to return to it.
	if(LauncherXBE)
	{
		size_t pathLen;
		char  *mntDev;
		char  *p;

		// Determine the necessary D: mapping for the launcher XBE
		p = strrchr(LauncherXBE, PATHSEPCHAR);
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
// xbox_AtExit 
//
// Custom atexit function for Xbox
//
void xbox_AtExit(void (*function)(void))
{
	if(function)
		ExitFuncList.push_back(function);
}

//
// xbox_Exit
//
// Custom exit function for Xbox
//
void xbox_Exit(int status)
{
	std::list<void (*)(void)>::iterator funcIter;

	for(funcIter = ExitFuncList.begin(); funcIter != ExitFuncList.end(); ++funcIter)
		(*funcIter)();

	xbox_CloseNetwork();

	xbox_UnMountPartitions();

	if(Xbox_RROD)
		xbox_DisableCustomLED();

	xbox_Reboot();
}

//
// xbox_PrepareArgs
//
// Convert to standard C arguments
void xbox_PrepareArgs(string cmdline, char *argv[], int &argc)
{
	if(cmdline.size())
	{
		size_t pos, oldpos = 0;

		do
		{
			if(cmdline[oldpos] == '"')
				pos = cmdline.find('"', ++oldpos);
			else
				pos = cmdline.find(' ', oldpos);

			if(pos != oldpos)
			{
				argv[argc] = strdup(cmdline.substr(oldpos, pos - oldpos).c_str());

				if(!stricmp(argv[argc], "-rrod"))
					Xbox_RROD = true;

				argc++;
			}

			oldpos = pos + 1;
		} while(pos != string::npos);
	}

	argv[argc] = NULL;
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
		// Command line from debugger
		if(launchDataType == LDT_FROM_DEBUGGER_CMDLINE) 
			xbox_PrepareArgs((char*)((PLD_FROM_DEBUGGER_CMDLINE)&launchData)->szCmdLine, xargv, xargc);
		// Command line from homebrew dashboards (XBMC, etc.)
		else if(launchDataType == LDT_TITLE && ((PCUSTOM_LAUNCH_DATA)&launchData)->magic == CUSTOM_LAUNCH_MAGIC)
			xbox_PrepareArgs((char*)((PCUSTOM_LAUNCH_DATA)&launchData)->szFilename, xargv, xargc);
		// Command line from Agar application (AG_Odalaunch)
		else if(launchDataType == LDT_TITLE && ((PAG_LAUNCH_DATA)&launchData)->magic == AG_LAUNCH_MAGIC)
		{
			xbox_RecordLauncherXBE(((PAG_LAUNCH_DATA)&launchData)->szLauncherXBE, ((PAG_LAUNCH_DATA)&launchData)->dwID);
			xbox_PrepareArgs((char*)((PAG_LAUNCH_DATA)&launchData)->szCmdLine, xargv, xargc);
		}
	}

	xbox_MountPartitions();

	xbox_InitNet();

	if(Xbox_RROD)
		xbox_EnableCustomLED();

	I_Main(xargc, xargv); // Does not return
}

#endif // _XBOX
