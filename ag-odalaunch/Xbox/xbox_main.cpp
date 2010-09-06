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

#include <iostream>
#include <xtl.h>
#include <xstring>

#include <agar/core.h>

#include <SDL/SDL.h>

#include "xbox_main.h"
#include "typedefs.h"

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
#define DeviceT "\\Device\\Harddisk0\\Partition1\\TDATA\\4F444C43"
#define DeviceU "\\Device\\Harddisk0\\Partition1\\UDATA\\4F444C43"
#define DeviceZ "\\Device\\Harddisk0\\Partition5"

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

enum
{
	JOY_BTTN_A = 0,
	JOY_BTTN_B,
	JOY_BTTN_X,
	JOY_BTTN_Y,
	JOY_BTTN_WHITE,
	JOY_BTTN_BLACK,
	JOY_BTTN_LTRIG,
	JOY_BTTN_RTRIG,
	JOY_BTTN_START,
	JOY_BTTN_BACK,
	JOY_BTTN_RSTICK,
	JOY_BTTN_LSTICK,
	JOY_BTTN_TOTAL
};

enum
{
	JOY_AXIS_LX = 0,
	JOY_AXIS_LY,
	JOY_AXIS_RX,
	JOY_AXIS_RY,
	JOY_AXIS_TOTAL
};

#define JOYPAD1 0
#define JOY_DEADZONE 3200

static SDL_Joystick *OpenedJoy = NULL;
static AG_Timeout    JoyUpdateTimeout;
static bool          BPressed[JOY_BTTN_TOTAL];

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
	xbox_MountDevice(DriveT, DeviceT); // AG-Odalaunch's unique TDATA - peristent save data (configs, etc.) - automounted
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
	xbox_UnMountDevice(DriveZ);
}

//
// xbox_UpdateJoystick
//
uint32_t xbox_UpdateJoystick(void *obj, uint32_t ival, void *arg)
{
	int16_t     axis_x = 0, axis_y = 0;
	int16_t     move_x = 0, move_y = 0;
	int         mouse_x, mouse_y;
	uint8_t     button;

	if(!SDL_JoystickOpened(JOYPAD1) || !OpenedJoy)
		return false;

	// Get the cursor position
	SDL_GetMouseState(&mouse_x, &mouse_y);

	// Get the current joystick state
	SDL_JoystickUpdate();

	// Left Stick X Axis
	axis_x = SDL_JoystickGetAxis(OpenedJoy, JOY_AXIS_LX);
	// Left Stick Y Axis
	axis_y = SDL_JoystickGetAxis(OpenedJoy, JOY_AXIS_LY);

	if(axis_x || axis_y)
	{
		if(axis_x > JOY_DEADZONE)
			axis_x -= JOY_DEADZONE;
		else if(axis_x < -JOY_DEADZONE)
			axis_x += JOY_DEADZONE;
		else
			axis_x = 0;

		if(axis_y > JOY_DEADZONE)
			axis_y -= JOY_DEADZONE;
		else if(axis_y < -JOY_DEADZONE)
			axis_y += JOY_DEADZONE;
		else
			axis_y = 0;

		move_x = mouse_x;
		move_y = mouse_y;

		if(axis_x)
			move_x += axis_x / 2000;
		if(axis_y)
			move_y += axis_y / 2000;

		// Move the cursor
		if(move_x || move_y)
			SDL_WarpMouse(move_x, move_y);
	}

	// Get all the button states
	for(int i = 0; i < JOY_BTTN_TOTAL; i++)
	{
		SDL_Event ev;

		button = SDL_JoystickGetButton(OpenedJoy, i);

		// A Button
		if(i == JOY_BTTN_A)
		{
			if(button && !BPressed[i])
			{
				ev.button.type = SDL_MOUSEBUTTONDOWN;
				ev.button.state = SDL_PRESSED;
			}
			else if(!button && BPressed[i])
			{
				ev.button.type = SDL_MOUSEBUTTONUP;
				ev.button.state = SDL_RELEASED;
			}
			else
				continue;

			ev.button.which = 0;
			ev.button.button = SDL_BUTTON_LEFT;
			ev.button.x = mouse_x;
			ev.button.y = mouse_y;
		}
		// Triggers
		else if(i == JOY_BTTN_LTRIG || i == JOY_BTTN_RTRIG)
		{
			if(button && !BPressed[i])
			{
				ev.key.type = SDL_KEYDOWN;
				ev.key.state = SDL_PRESSED;
			}
			else if(!button && BPressed[i])
			{
				ev.key.type = SDL_KEYUP;
				ev.key.state = SDL_RELEASED;
			}
			else
				continue;

			if(i == JOY_BTTN_LTRIG)
				ev.key.keysym.sym = SDLK_PAGEUP;
			else
				ev.key.keysym.sym = SDLK_PAGEDOWN;
		}
		else
			continue;

		BPressed[i] = !BPressed[i];

		// Push the translated event
		SDL_PushEvent(&ev);
	}

	// Repeat after interval
	return ival;
}

//
// xbox_EnableJoystickUpdates
//
void xbox_EnableJoystickUpdates(bool enable)
{
	AG_LockTimeouts(NULL);

	if(enable)
	{
		if(!AG_TimeoutIsScheduled(NULL, &JoyUpdateTimeout))
			AG_ScheduleTimeout(NULL, &JoyUpdateTimeout, 5);
	}
	else
	{
		if(AG_TimeoutIsScheduled(NULL, &JoyUpdateTimeout))
			AG_DelTimeout(NULL, &JoyUpdateTimeout);
	}

	AG_UnlockTimeouts(NULL);
}

//
// xbox_InitializeJoystick
//
int xbox_InitializeJoystick()
{
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	// We will manually retrieve the states
	SDL_JoystickEventState(SDL_IGNORE);

	OpenedJoy = SDL_JoystickOpen(JOYPAD1);

	if(!SDL_JoystickOpened(JOYPAD1))
		return -1;

	AG_SetTimeout(&JoyUpdateTimeout, xbox_UpdateJoystick, NULL, 0);

	xbox_EnableJoystickUpdates(true);

	fill_n(BPressed, (size_t)JOY_BTTN_TOTAL, false);

	return 0;
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