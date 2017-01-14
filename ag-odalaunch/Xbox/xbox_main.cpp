// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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

// The following header/license applies to gettimeofday()
/*
 * gettimeofday.c
 *    Win32 gettimeofday() replacement
 *
 * src/port/gettimeofday.c
 *
 * Copyright (c) 2003 SRA, Inc.
 * Copyright (c) 2003 SKC, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose, without fee, and without a
 * written agreement is hereby granted, provided that the above
 * copyright notice and this paragraph and the following two
 * paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS
 * IS" BASIS, AND THE AUTHOR HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <xstring>

#include "xbox_main.h"
#include "xbox_api.h"

using namespace std;

namespace agOdalaunch {

// Static members
SDL_Joystick *Xbox::OpenedJoy = NULL;
AG_Timeout    Xbox::JoyUpdateTimeout;
bool          Xbox::BPressed[JOY_BTTN_TOTAL];
uint8_t       Xbox::HPressed = 0;

std::ofstream Xbox::XBLogFile;
AG_Mutex      Xbox::XBLogMutex;
bool          Xbox::DebugConsole = false;

/* FILETIME of Jan 1 1970 00:00:00. */
static const unsigned __int64 epoch = ((unsigned __int64) 116444736000000000ULL);

int gettimeofday(struct timeval * tp, ...)
{
	FILETIME    file_time;
	SYSTEMTIME  system_time;
	ULARGE_INTEGER ularge;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	tp->tv_sec = (long) ((ularge.QuadPart - epoch) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);

	return 0;
}

//
// InetNtoa
//
char *Xbox::InetNtoa(struct in_addr in)
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
// GetHostByName
//
struct hostent *Xbox::GetHostByName(const char *name)
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
// InitLogFile
//
int Xbox::InitLogFile()
{
	AG_MutexInit(&XBLogMutex);

	AG_MutexLock(&XBLogMutex);

	XBLogFile.open("T:\\ag-odalaunch.log");
	if(XBLogFile.fail())
	{
		AG_MutexUnlock(&XBLogMutex);
		return -1;
	}

	AG_MutexUnlock(&XBLogMutex);

	return 0;
}

//
// CloseLogFile
//
void Xbox::CloseLogFile()
{
	AG_MutexLock(&XBLogMutex);

	XBLogFile.close();

	AG_MutexUnlock(&XBLogMutex);
}


//
// OutputDebugString
//
void Xbox::OutputDebugString(const char *str, ...)
{
	va_list ap;
	char    res[1024];

	if(!str)
		return;

	va_start(ap, str);
	_vsnprintf(res, sizeof(res), str, ap);

	AG_MutexLock(&XBLogMutex);

	if(DebugConsole)
		::OutputDebugString(res);

	XBLogFile << res;
	XBLogFile.flush();

	AG_MutexUnlock(&XBLogMutex);
}

//
// MountDevice
//
LONG Xbox::MountDevice(LPSTR sSymbolicLinkName, LPSTR sDeviceName)
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
// UnMountDevice
//
LONG Xbox::UnMountDevice(LPSTR sSymbolicLinkName)
{
  UNICODE_STRING  symbolicLinkName;
  symbolicLinkName.Buffer  = sSymbolicLinkName;
  symbolicLinkName.Length = (USHORT)strlen(sSymbolicLinkName);
  symbolicLinkName.MaximumLength = (USHORT)strlen(sSymbolicLinkName) + 1;

  return IoDeleteSymbolicLink(&symbolicLinkName);
}

//
// MountPartitions
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
void Xbox::MountPartitions()
{
	MountDevice(DriveD, CdRom);   // DVD-ROM or start path - automounted
	MountDevice(DriveE, DeviceE); // Standard save partition
	MountDevice(DriveF, DeviceF); // Non-stock partition - modded consoles only
	MountDevice(DriveG, DeviceG); // Non-stock partition - modded consoles only
	MountDevice(DriveT, DeviceT); // AG-Odalaunch's unique TDATA - peristent save data (configs, etc.) - automounted
	MountDevice(DriveZ, DeviceZ); // Cache partition - appropriate place for temporary files - automounted
}

//
// UnMountPartitions
//
void Xbox::UnMountPartitions()
{
	UnMountDevice(DriveD);
	UnMountDevice(DriveE);
	UnMountDevice(DriveF);
	UnMountDevice(DriveG);
	UnMountDevice(DriveT);
	UnMountDevice(DriveZ);
}

//
// UpdateJoystick
//
uint32_t Xbox::UpdateJoystick(void *obj, uint32_t ival, void *arg)
{
	int16_t     axis_x = 0, axis_y = 0;
	int16_t     move_x = 0, move_y = 0;
	int         mouse_x, mouse_y;
	uint8_t     button;
	uint8_t     hat;

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
		if(move_x != mouse_x || move_y != mouse_y)
			SDL_WarpMouse(move_x, move_y);
	}

	// Get all the button states
	for(int i = 0; i < JOY_BTTN_TOTAL; i++)
	{
		SDL_Event ev;

		button = SDL_JoystickGetButton(OpenedJoy, i);

		// Mouse Button Translations
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
		// Keyboard Translations
		else if(i == JOY_BTTN_B || i == JOY_BTTN_LTRIG || i == JOY_BTTN_RTRIG)
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

			if(i == JOY_BTTN_B)
				ev.key.keysym.sym = SDLK_RETURN;
			else if(i == JOY_BTTN_LTRIG)
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

	// Hat (D-Pad)
	hat = SDL_JoystickGetHat(OpenedJoy, 0);
	if(HPressed) // A hat position is recorded
	{
		if(!(hat & HPressed))
		{
			SDL_Event ev;

			ev.key.type = SDL_KEYUP;
			ev.key.state = SDL_RELEASED;

			if(HPressed == SDL_HAT_UP)
				ev.key.keysym.sym = SDLK_UP;
			else if(HPressed == SDL_HAT_DOWN)
				ev.key.keysym.sym = SDLK_DOWN;
			else
				ev.key.keysym.sym = SDLK_TAB;

			SDL_PushEvent(&ev);

			// In the Agar tabcycle code the action is taken when the tab keyup
			// event is received and it is then that the shift modifier is tested
			// so shift needs to be released after the tab event has been pushed or
			// the modifier will not be active when the test occurs.
			if(HPressed == SDL_HAT_LEFT)
			{
				// Push the mod keyup event
				ev.key.keysym.sym = SDLK_LSHIFT;
				SDL_PushEvent(&ev);
			}

			HPressed = 0;
		}
	}
	if(!HPressed) // Either nothing was pushed before or the state has changed
	{
		SDL_Event ev;

		ev.key.type = SDL_KEYDOWN;
		ev.key.state = SDL_PRESSED;

		if(hat & SDL_HAT_UP)
		{
			ev.key.keysym.sym = SDLK_UP;
			HPressed = SDL_HAT_UP;
		}
		else if(hat & SDL_HAT_DOWN)
		{
			ev.key.keysym.sym = SDLK_DOWN;
			HPressed = SDL_HAT_DOWN;
		}
		else if(hat & SDL_HAT_RIGHT)
		{
			ev.key.keysym.sym = SDLK_TAB;
			HPressed = SDL_HAT_RIGHT;
		}
		else if(hat & SDL_HAT_LEFT)
		{
			// Push the mod keydown event
			ev.key.keysym.sym = SDLK_LSHIFT;
			SDL_PushEvent(&ev);

			ev.key.keysym.sym = SDLK_TAB;
			HPressed = SDL_HAT_LEFT;
		}

		if(HPressed)
			SDL_PushEvent(&ev);
	}

	// Repeat after interval
	return ival;
}

//
// EnableJoystickUpdates
//
void Xbox::EnableJoystickUpdates(bool enable)
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
// InitializeJoystick
//
int Xbox::InitializeJoystick()
{
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	// We will manually retrieve the states
	SDL_JoystickEventState(SDL_IGNORE);

	OpenedJoy = SDL_JoystickOpen(JOYPAD1);

	if(!SDL_JoystickOpened(JOYPAD1))
		return -1;

	AG_SetTimeout(&JoyUpdateTimeout, UpdateJoystick, NULL, 0);

	EnableJoystickUpdates(true);

	fill_n(BPressed, (size_t)JOY_BTTN_TOTAL, false);

	return 0;
}

//
// InitNet
//
void Xbox::InitNet()
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
// CloseNetwork
//
void Xbox::CloseNetwork()
{
	XNetCleanup();
}

//
// EnableDebugConsole
//
void Xbox::EnableDebugConsole()
{
	DebugConsole = true;
}

} // namespace

using namespace agOdalaunch;

extern int agol_main(int argc, char *argv[]);

//
// main
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
			Xbox::EnableDebugConsole();

			xargv[xargc] = strtok(((PLD_FROM_DEBUGGER_CMDLINE)&launchData)->szCmdLine, " ");

			while(xargv[xargc] != NULL)
			{
				xargc++;
				xargv[xargc] = strtok(NULL, " ");
			}
		}
	}

	// Mount the partitions
	Xbox::MountPartitions();

	// Initialize the log file
	Xbox::InitLogFile();

	// Initialize the network
	Xbox::InitNet();

	agol_main(xargc, xargv); // Normal entry point

	// Shutdown the network
	Xbox::CloseNetwork();

	// Close the log file
	Xbox::CloseLogFile();

	// Unmount the partitions
	Xbox::UnMountPartitions();

	// Return to the dashboard
	XLaunchNewImage(NULL, NULL);
}
