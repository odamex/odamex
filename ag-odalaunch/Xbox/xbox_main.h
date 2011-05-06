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

#ifndef _XBOX_MAIN_H
#define _XBOX_MAIN_H

#ifdef _XBOX

#include <fstream>
#include <agar/core.h>
#include <SDL/SDL.h>

#include "typedefs.h"

#define inet_ntoa Xbox::InetNtoa
#define gethostbyname Xbox::GetHostByName

// hostent
struct hostent
{
	char  *h_name;       /* canonical name of host */
	char **h_aliases;    /* alias list */
	int    h_addrtype;   /* host address type */
	int    h_length;     /* length of address */
	char **h_addr_list;  /* list of addresses */
#define h_addr h_addr_list[0]
};

class Xbox
{
public:
	static char *InetNtoa(struct in_addr in);
	static struct hostent *GetHostByName(const char *name);

	static int  InitializeJoystick(void);
	static void EnableJoystickUpdates(bool);

	static void MountPartitions();
	static void UnMountPartitions();

	static void OutputDebugString(const char *str, ...);
	static int  InitLogFile();
	static void CloseLogFile();

	static void InitNet();
	static void CloseNetwork();

	static void EnableDebugConsole();

private:
	Xbox() {};
	Xbox(const Xbox&) {};
	Xbox& operator=(const Xbox&) {};

	static LONG MountDevice(LPSTR sSymbolicLinkName, LPSTR sDeviceName);
	static LONG UnMountDevice(LPSTR sSymbolicLinkName);

	static uint32_t UpdateJoystick(void *obj, uint32_t ival, void *arg);

	static const int JOYPAD1 = 0;
	static const int JOY_DEADZONE = 3200;

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

	static SDL_Joystick *OpenedJoy;
	static AG_Timeout    JoyUpdateTimeout;
	static bool          BPressed[JOY_BTTN_TOTAL];
	static uint8_t       HPressed;

	static std::ofstream XBLogFile;
	static AG_Mutex      XBLogMutex;
	static bool          DebugConsole;
};

#endif // _XBOX

#endif // _XBOX_MAIN_H