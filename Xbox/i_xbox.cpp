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
	
	if(!name)
		return NULL;

	hEvent = WSACreateEvent();
	err = XNetDnsLookup(name, hEvent, &pDns);

	WaitForSingleObject( (HANDLE)hEvent, INFINITE);

	if(pDns && pDns->iStatus == 0)
	{
		if(he)
		{
			if(he->h_addr_list)
			{
				if(he->h_addr_list[0])
					free(he->h_addr_list[0]);
				free(he->h_addr_list);
			}
			free(he);
		}

		he = (struct hostent *)malloc(sizeof(struct hostent));
		if(!he)
		{
			// Failed to allocate!
			return NULL;
		}

		he->h_addr_list = (char **)malloc(sizeof(char*));
		he->h_addr_list[0] = (char *)malloc(sizeof(struct in_addr));

		memcpy(he->h_addr_list[0], pDns->aina, sizeof(struct in_addr));

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

void xbox_TranslateSdlKbdEvent(SDL_Event &ev)
{
	bool  uppercase = false;

	if(ev.key.keysym.sym == SDLK_RETURN)
		ev.key.keysym.unicode = '\r';
	else if(isalpha(ev.key.keysym.scancode))
	{
		if(SDL_GetModState() & (KMOD_LSHIFT | KMOD_RSHIFT) ) // Shift is held down
		{
			if(SDL_GetModState() & KMOD_CAPS)
				uppercase = false; // If caps lock is enabled the modifier function reverses
			else
				uppercase = true;
		}
		else if(SDL_GetModState() & KMOD_CAPS) // Caps Lock is enabled
			uppercase = true;

		if(!uppercase)
			ev.key.keysym.unicode = ev.key.keysym.scancode + 32;
		else
			ev.key.keysym.unicode = ev.key.keysym.scancode;
	}
	else
	{
		if(SDL_GetModState() & (KMOD_LSHIFT | KMOD_RSHIFT) ) // Shift is held down
		{
			switch(ev.key.keysym.sym)
			{
				case SDLK_1:
					ev.key.keysym.unicode = SDLK_EXCLAIM;
					break;
				case SDLK_2:
					ev.key.keysym.unicode = SDLK_AT;
					break;
				case SDLK_3:
					ev.key.keysym.unicode = SDLK_HASH;
					break;
				case SDLK_4:
					ev.key.keysym.unicode = SDLK_DOLLAR;
					break;
				case SDLK_5:
					ev.key.keysym.unicode = '%';
					break;
				case SDLK_6:
					ev.key.keysym.unicode = SDLK_CARET;
					break;
				case SDLK_7:
					ev.key.keysym.unicode = SDLK_AMPERSAND;
					break;
				case SDLK_8:
					ev.key.keysym.unicode = SDLK_ASTERISK;
					break;
				case SDLK_9:
					ev.key.keysym.unicode = SDLK_LEFTPAREN;
					break;
				case SDLK_0:
					ev.key.keysym.unicode = SDLK_RIGHTPAREN;
					break;
				case SDLK_MINUS:
					ev.key.keysym.unicode = SDLK_UNDERSCORE;
					break;
				case SDLK_EQUALS:
					ev.key.keysym.unicode = SDLK_PLUS;
					break;
				case SDLK_LEFTBRACKET:
					ev.key.keysym.unicode = '{';
					break;
				case SDLK_RIGHTBRACKET:
					ev.key.keysym.unicode = '}';
					break;
				case SDLK_BACKSLASH:
					ev.key.keysym.unicode = '|';
					break;
				case SDLK_SEMICOLON:
					ev.key.keysym.unicode = SDLK_COLON;
					break;
				case '\'':
					ev.key.keysym.unicode = SDLK_QUOTEDBL;
					break;
				case SDLK_COMMA:
					ev.key.keysym.unicode = SDLK_LESS;
					break;
				case SDLK_PERIOD:
					ev.key.keysym.unicode = SDLK_GREATER;
					break;
				case SDLK_SLASH:
					ev.key.keysym.unicode = SDLK_QUESTION;
					break;
				case SDLK_BACKQUOTE:
					ev.key.keysym.unicode = '~';
					break;
				default:
					ev.key.keysym.unicode = ev.key.keysym.sym;
			}
		}
		else
			ev.key.keysym.unicode = ev.key.keysym.sym;
	}
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

void xbox_CloseNetwork()
{
	XNetCleanup();
}

int xbox_SetScreenPosition(float x, float y)
{
	float x2, y2;

	SDL_XBOX_SetScreenPosition(x, y);
	SDL_XBOX_GetScreenPosition(&x2, &y2);

	if(x != x2 || y != y2)
		return -1;

	return 0;
}

int xbox_SetScreenStretch(float xs, float ys)
{
	float xs2, ys2;

	SDL_XBOX_SetScreenStretch(xs, ys);
	SDL_XBOX_GetScreenStretch(&xs2, &ys2);

	if(xs != xs2 || ys != ys2)
		return -1;

	return 0;
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

	xbox_InitNet();

	i_main(xargc, xargv);

	xbox_CloseNetwork();
}

#endif // _XBOX