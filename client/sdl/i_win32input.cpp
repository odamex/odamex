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
//
//-----------------------------------------------------------------------------

#include <SDL.h>		// for SDL_GetWMInfo()
#include <SDL_syswm.h>
#include "win32inc.h"
#include "i_input.h"
#include "i_win32input.h"
#include "doomkeys.h"

#include <queue>
#include <cassert>

// ============================================================================
//
// IRawWin32MouseInputDevice
//
// ============================================================================

#ifdef USE_RAW_WIN32_MOUSE

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC  ((USHORT) 0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE  ((USHORT) 0x02)
#endif


//
// getMouseRawInputDevice
//
// Helper function that searches for a registered mouse raw input device. If
// found, the device parameter is filled with the information for the device
// and the function returns true.
//
bool getMouseRawInputDevice(RAWINPUTDEVICE& device)
{
	device.usUsagePage	= 0;
	device.usUsage		= 0;
	device.dwFlags		= 0;
	device.hwndTarget	= 0;

	// get the number of raw input devices
	UINT num_devices;
	GetRegisteredRawInputDevices(NULL, &num_devices, sizeof(RAWINPUTDEVICE));

	// create a buffer to hold the raw input device info
	RAWINPUTDEVICE* devices = new RAWINPUTDEVICE[num_devices];

	// look at existing registered raw input devices
	GetRegisteredRawInputDevices(devices, &num_devices, sizeof(RAWINPUTDEVICE));
	for (UINT i = 0; i < num_devices; i++)
	{
		// is there already a mouse device registered?
		if (devices[i].usUsagePage == HID_USAGE_PAGE_GENERIC &&
			devices[i].usUsage == HID_USAGE_GENERIC_MOUSE)
		{
			device.usUsagePage	= devices[i].usUsagePage;
			device.usUsage		= devices[i].usUsage;
			device.dwFlags		= devices[i].dwFlags;
			device.hwndTarget	= devices[i].hwndTarget;
			break;
		}
	}

    delete [] devices;

	return device.usUsagePage == HID_USAGE_PAGE_GENERIC &&
			device.usUsage == HID_USAGE_GENERIC_MOUSE;
}

// define the static member variables declared in the header
IRawWin32MouseInputDevice* IRawWin32MouseInputDevice::mInstance = NULL;

//
// IRawWin32MouseInputDevice::IRawWin32MouseInputDevice
//
IRawWin32MouseInputDevice::IRawWin32MouseInputDevice(int id) :
	mActive(false), mInitialized(false),
	mHasBackupDevice(false), mRegisteredMouseDevice(false),
	mWindow(NULL), mBaseWindowProc(NULL), mRegisteredWindowProc(false),
	mAggregateX(0), mAggregateY(0)
{
	// get a handle to the window

	// TODO: FIXME
	#if 0
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)
	SDL_GetWMInfo(&wminfo);
	mWindow = wminfo.window;
    #endif
	
	mInstance = this;

	mInitialized = true;
	registerMouseDevice();
	registerWindowProc();
}


//
// IRawWin32MouseInputDevice::~IRawWin32MouseInputDevice
//
// Remove the callback for retreiving input and unregister the RAWINPUTDEVICE
//
IRawWin32MouseInputDevice::~IRawWin32MouseInputDevice()
{
	pause();
	mInstance = NULL;
}


//
// IRawWin32MouseInputDevice::gatherEvents
//
void IRawWin32MouseInputDevice::gatherEvents()
{
	if (!mActive)
	{
		flushEvents();
		return;
	}

	// The button and scroll wheel events are gathered in the callback windowProc
	// and inserted directly into the queue. However, we still need to handle
	// the aggregated mouse motion by creating an event_t instance and inserting it
	// into the queue.
	event_t motion_event;
	motion_event.type = ev_mouse;
	motion_event.data1 = 0;
	motion_event.data2 = mAggregateX;
	motion_event.data3 = mAggregateY;
	mEvents.push(motion_event);

	// reset the motion values for the next frame
	mAggregateX = mAggregateY = 0;
}


//
// IRawWin32MouseInputDevice::getEvent
//
void IRawWin32MouseInputDevice::getEvent(event_t* ev)
{
	assert(hasEvent());

	memcpy(ev, &mEvents.front(), sizeof(event_t));

	mEvents.pop();
}


//
// IRawWin32MouseInputDevice::flushEvents
//
void IRawWin32MouseInputDevice::flushEvents()
{
	// clear the queue of events
	while (!mEvents.empty())
		mEvents.pop();
	// clear the aggregated mouse motion
	mAggregateX = mAggregateY = 0;
}


//
// IRawWin32MouseInputDevice::center
//
void IRawWin32MouseInputDevice::center()
{
	RECT rect;
	GetWindowRect(mWindow, &rect);
	SetCursorPos((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
}


//
// IRawWin32MouseInputDevice::pause
//
void IRawWin32MouseInputDevice::pause()
{
	mActive = false;

	unregisterMouseDevice();
	unregisterWindowProc();
}


//
// IRawWin32MouseInputDevice::resume
//
void IRawWin32MouseInputDevice::resume()
{
	mActive = true;
	flushEvents();

	registerMouseDevice();
	registerWindowProc();
}


void IRawWin32MouseInputDevice::reset()
{
	flushEvents();
	center();
}

//
// IRawWin32MouseInputDevice::registerWindowProc
//
// Saves the existing WNDPROC for the app window and installs our own.
//
void IRawWin32MouseInputDevice::registerWindowProc()
{
	if (!mRegisteredWindowProc)
	{
		// install our own window message callback and save the previous
		// callback as mBaseWindowProc
		mBaseWindowProc = (WNDPROC)SetWindowLongPtr(
							mWindow, GWLP_WNDPROC,
							(LONG_PTR)IRawWin32MouseInputDevice::windowProcWrapper);
		mRegisteredWindowProc = true;
	}
}


//
// IRawWin32MouseInputDevice::unregisterWindowProc
//
// Restore the saved WNDPROC for the app window.
//
void IRawWin32MouseInputDevice::unregisterWindowProc()
{
	if (mRegisteredWindowProc)
	{
		SetWindowLongPtr(mWindow, GWLP_WNDPROC, (LONG_PTR)mBaseWindowProc);
		mBaseWindowProc = NULL;
		mRegisteredWindowProc = false;
	}
}


//
// IRawWin32MouseInputDevice::windowProc
//
// A callback function that reads WM_Input messages, converts them to event_t
// instances and queues them for later retrieval.
//
LRESULT CALLBACK IRawWin32MouseInputDevice::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_INPUT)
	{
		RAWINPUT raw;
		UINT size = sizeof(raw);

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));

		if (raw.header.dwType == RIM_TYPEMOUSE)
		{
			const RAWMOUSE& mouse = raw.data.mouse;

			// Handle mouse buttons and convert each button click/release
			// into a separate event_t instance and insert it into the queue.
			for (int button_num = 0; button_num < 5; button_num++)
			{
				static const UINT ri_down_lookup[5] = {
					RI_MOUSE_BUTTON_1_DOWN, RI_MOUSE_BUTTON_2_DOWN, RI_MOUSE_BUTTON_3_DOWN,
					RI_MOUSE_BUTTON_4_DOWN,	RI_MOUSE_BUTTON_5_DOWN };

				static const UINT ri_up_lookup[5] = {
					RI_MOUSE_BUTTON_1_UP, RI_MOUSE_BUTTON_2_UP, RI_MOUSE_BUTTON_3_UP,
					RI_MOUSE_BUTTON_4_UP, RI_MOUSE_BUTTON_5_UP };

				static const int oda_button_lookup[5] = {
					KEY_MOUSE1, KEY_MOUSE2, KEY_MOUSE3, KEY_MOUSE4, KEY_MOUSE5 };

				event_t button_event;
				button_event.data1 = button_event.data2 = button_event.data3 = 0;

				if (mouse.usButtonFlags & ri_down_lookup[button_num])
				{
					button_event.type = ev_keydown;
					button_event.data1 = oda_button_lookup[button_num];
					mEvents.push(button_event);
				}
				else if (mouse.usButtonFlags & ri_up_lookup[button_num])
				{
					button_event.type = ev_keyup;
					button_event.data1 = oda_button_lookup[button_num];
					mEvents.push(button_event);
				}
			}

			// Handle mouse wheel events and convert them into event_t instances
			// and insert them into the queue.
			event_t wheel_event;
			wheel_event.type = ev_keydown;
			wheel_event.data1 = wheel_event.data2 = wheel_event.data3 = 0;

			if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
			{
				if ((SHORT)mouse.usButtonData < 0)
				{
					wheel_event.data1 = KEY_MWHEELDOWN;
					mEvents.push(wheel_event);
				}
				else if ((SHORT)mouse.usButtonData > 0)
				{
					wheel_event.data1 = KEY_MWHEELUP;
					mEvents.push(wheel_event);
				}
			}

			// Handle mouse movement by aggregating the relative motion into
			// mAggregateX and mAggregateY. This will be converted into an
			// event_t instance later.
			static int prevx, prevy;
			static bool prev_valid = false;

			if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
			{
				// we're given absolute mouse coordinates and need to convert
				// them to relative coordinates based on the previous x & y
				if (prev_valid)
				{
					mAggregateX += mouse.lLastX - prevx;
					mAggregateY -= mouse.lLastY - prevy;
				}

				prevx = mouse.lLastX;
				prevy = mouse.lLastY;
				prev_valid = true;
			}
			else
			{
				// we're given relative mouse coordinates
				mAggregateX += mouse.lLastX;
				mAggregateY -= mouse.lLastY;
				prev_valid = false;
			}

			// indicate that the event was eaten by this callback
			return 0;
		}
	}

	// hand the message off to mDefaultWindowProc since it's not a WM_INPUT mouse message
	return CallWindowProc(mBaseWindowProc, hwnd, message, wParam, lParam);
}


//
// IRawWin32MouseInputDevice::windowProcWrapper
//
// A static member function that wraps calls to windowProc to allow member
// functions to use Windows callbacks.
//
LRESULT CALLBACK IRawWin32MouseInputDevice::windowProcWrapper(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return mInstance->windowProc(hwnd, message, wParam, lParam);
}


//
// IRawWin32MouseInputDevice::backupMouseDevice
//
void IRawWin32MouseInputDevice::backupMouseDevice(const RAWINPUTDEVICE& device)
{
	mBackupDevice.usUsagePage	= device.usUsagePage;
	mBackupDevice.usUsage		= device.usUsage;
	mBackupDevice.dwFlags		= device.dwFlags;
	mBackupDevice.hwndTarget	= device.hwndTarget;
}


//
// IRawWin32MouseInputDevice::restoreMouseDevice
//
void IRawWin32MouseInputDevice::restoreMouseDevice(RAWINPUTDEVICE& device) const
{
	device.usUsagePage	= mBackupDevice.usUsagePage;
	device.usUsage		= mBackupDevice.usUsage;
	device.dwFlags		= mBackupDevice.dwFlags;
	device.hwndTarget	= mBackupDevice.hwndTarget;
}


//
// IRawWin32MouseInputDevice::registerRawInputDevice
//
// Registers the mouse as a raw input device, backing up the previous raw input
// device for later restoration.
//
bool IRawWin32MouseInputDevice::registerMouseDevice()
{
	if (mRegisteredMouseDevice)
		return false;

	RAWINPUTDEVICE device;

	if (getMouseRawInputDevice(device))
	{
		// save a backup copy of this device
		if (!mHasBackupDevice)
		{
			backupMouseDevice(device);
			mHasBackupDevice = true;
		}

		// remove the existing device
		device.dwFlags = RIDEV_REMOVE;
		device.hwndTarget = NULL;
		RegisterRawInputDevices(&device, 1, sizeof(device));
	}

	// register our raw input mouse device
	device.usUsagePage	= HID_USAGE_PAGE_GENERIC;
	device.usUsage		= HID_USAGE_GENERIC_MOUSE;
	device.dwFlags		= RIDEV_NOLEGACY;
	device.hwndTarget	= mWindow;

	mRegisteredMouseDevice = RegisterRawInputDevices(&device, 1, sizeof(device));
	return mRegisteredMouseDevice;
}


//
// IRawWin32MouseInputDevice::unregisterRawInputDevice
//
// Removes the mouse as a raw input device, restoring a previously backedup
// mouse device if applicable.
//
bool IRawWin32MouseInputDevice::unregisterMouseDevice()
{
	if (!mRegisteredMouseDevice)
		return false;

	RAWINPUTDEVICE device;

	if (getMouseRawInputDevice(device))
	{
		// remove the device
		device.dwFlags = RIDEV_REMOVE;
		device.hwndTarget = NULL;
		RegisterRawInputDevices(&device, 1, sizeof(device));
		mRegisteredMouseDevice = false;
	}

	if (mHasBackupDevice)
	{
		restoreMouseDevice(device);
		mHasBackupDevice = false;
		RegisterRawInputDevices(&device, 1, sizeof(device));
	}

	return mRegisteredMouseDevice == false;
}


//
// IRawWin32MouseInputDevice::debug
//
void IRawWin32MouseInputDevice::debug() const
{
	// TODO: FIXME
	#if 0
	// get a handle to the window
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)
	SDL_GetWMInfo(&wminfo);
	HWND cur_window = wminfo.window;

	// determine the hwndTarget parameter of the registered rawinput device
	HWND hwndTarget = NULL;

	RAWINPUTDEVICE device;
	if (getMouseRawInputDevice(device))
	{
		hwndTarget = device.hwndTarget;
	}

	Printf(PRINT_HIGH, "IRawWin32MouseInputDevice: Current Window Address: 0x%x, mWindow: 0x%x, RAWINPUTDEVICE Window: 0x%x\n",
			cur_window, mWindow, hwndTarget);

	WNDPROC wndproc = (WNDPROC)GetWindowLongPtr(cur_window, GWLP_WNDPROC);
	Printf(PRINT_HIGH, "IRawWin32MouseInputDevice: windowProcWrapper Address: 0x%x, Current Window WNDPROC Address: 0x%x\n",
			IRawWin32MouseInputDevice::windowProcWrapper, wndproc);
			
    #endif
}

#endif	// USE_RAW_WIN32_MOUSE


//
// I_CheckForProc
//
// Checks if a function with the given name is in the given DLL file.
// This is used to determine if the user's version of Windows has the necessary
// functions availible.
//
#if defined(_WIN32) && !defined(_XBOX)
static bool I_CheckForProc(const char* dllname, const char* procname)
{
	bool avail = false;
	HMODULE dll = LoadLibrary(TEXT(dllname));
	if (dll)
	{
		avail = (GetProcAddress(dll, procname) != NULL);
		FreeLibrary(dll);
	}
	return avail;
}
#endif  // _WIN32


//
// I_RawWin32MouseAvailible
//
// Checks if the raw input mouse functions that the RawWin32Mouse
// class calls are availible on the current system. They require
// Windows XP or higher.
//
bool I_RawWin32MouseAvailible()
{
#if defined(_WIN32) && !defined(_XBOX) && defined(USE_RAW_WIN32_MOUSE)
	return	I_CheckForProc("user32.dll", "RegisterRawInputDevices") &&
			I_CheckForProc("user32.dll", "GetRegisteredRawInputDevices") &&
			I_CheckForProc("user32.dll", "GetRawInputData");
#else
	return false;
#endif
}


VERSION_CONTROL (i_win32input_cpp, "$Id$")
