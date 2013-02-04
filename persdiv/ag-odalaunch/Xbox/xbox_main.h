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

/**
 * agOdalaunch namespace.
 *
 * All code for the ag-odalaunch launcher is contained within
 * the agOdalaunch namespace.
 */
namespace agOdalaunch {

#define inet_ntoa Xbox::InetNtoa
#define gethostbyname Xbox::GetHostByName

/**
 * Standard hostent structure.
 *
 * The hostent structure is typically used with the gethostbyname() function.
 */
struct hostent
{
	char  *h_name;       /** canonical name of host */
	char **h_aliases;    /** alias list */
	int    h_addrtype;   /** host address type */
	int    h_length;     /** length of address */
	char **h_addr_list;  /** list of addresses */
#define h_addr h_addr_list[0] /** The first address in h_addr_list */
};

/**
 * Xbox Class.
 *
 * The Xbox class provides utilities and function overrides
 * that are needed for the Xbox platform.
 */
class Xbox
{
public:
	/**
	 * Network Byte Order address to ascii string.
	 *
	 * This is a custom implementation of inet_ntoa() for Xbox. inet_ntoa()
	 * converts an internet address in network byte order to an ascii string.
	 *
	 * @param in Network address.
	 * @returns Address ascii string.
	 */
	static char *InetNtoa(struct in_addr in);

	/**
	 * Get Host by Name
	 *
	 * This is a custom implementation of gethostbyname() for Xbox.
	 * gethostbyname() returns a hostent structure for the provided hostname
	 * or IP address.
	 *
	 * @param name A hostname or IP address.
	 * @returns A filled in hostent structure or NULL on error.
	 */
	static struct hostent *GetHostByName(const char *name);

	/**
	 * Initialize the Xbox joystick.
	 *
	 * This function initializes the Xbox joystick which will move the cursor
	 * within the launcher as well as activate widgets.
	 *
	 * @returns 0 on success, -1 on error.
	 */
	static int InitializeJoystick(void);

	/**
	 * Enable or disable Xbox joystick updates.
	 *
	 * The Xbox joystick can be enabled or disabled by passing true or false
	 * to this function.
	 *
	 * @param enable true or false.
	 */
	static void EnableJoystickUpdates(bool enable);

	/**
	 * Mount Xbox partitions.
	 *
	 * This function mounts the Xbox partitions which makes them available for
	 * reading and writing.
	 */
	static void MountPartitions();

	/**
	 * Unmount Xbox partitions.
	 *
	 * This function unmounts the Xbox partitions.
	 */
	static void UnMountPartitions();

	/**
	 * Output a string for debugging.
	 *
	 * Provided a string, with optional standard variable arguments, this function
	 * will write the contents to a debug file and, if running in the debugger, print
	 * the string to the debug console.
	 *
	 * @param str The formatted string.
	 * @param ... Standard variable arguments.
	 */
	static void OutputDebugString(const char *str, ...);

	/**
	 * Initialize the log file.
	 *
	 * This function open the Xbox log file for writing.
	 *
	 * @returns 0 on success, -1 on error.
	 */
	static int  InitLogFile();

	/**
	 * Close the log file.
	 *
	 * This function closes the Xbox log file.
	 */
	static void CloseLogFile();

	/**
	 * Initialize the network adapter.
	 *
	 * Initialize the Xbox network adapter and attempt to obtain an IP address.
	 */
	static void InitNet();

	/**
	 * Close the network adapter.
	 *
	 * Close the Xbox network adapter and release any leased IP address.
	 */
	static void CloseNetwork();

	/**
	 * Enable the debug console.
	 *
	 * This function enables the debug console which causes debug output to be
	 * sent to the debug console.
	 */
	static void EnableDebugConsole();

private:
	Xbox();
	Xbox(const Xbox&);
	Xbox& operator=(const Xbox&);

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

} // namespace

#endif // _XBOX

#endif // _XBOX_MAIN_H
