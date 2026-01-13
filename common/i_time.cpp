// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//      This module contains the basic time functions.
//
//-----------------------------------------------------------------------------

#include "i_time.h"

#if defined OSX
	#include <mach/clock.h>
	#include <mach/mach.h>

#elif defined UNIX
	#include <time.h>

#elif defined WIN32
	#include "win32inc.h"

#else
	#include "i_sdl.h"
#endif

//
// I_GetTime
//
// [SL] Retrieve an arbitrarily-based time from a high-resolution timer with
// nanosecond accuracy.
//
dtime_t I_GetTime()
{
#if defined OSX
	clock_serv_t cclock;
	mach_timespec_t mts;

	host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	return mts.tv_sec * 1000LL * 1000LL * 1000LL + mts.tv_nsec;

#elif defined UNIX
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000LL * 1000LL * 1000LL + ts.tv_nsec;

#elif defined WIN32
	static bool initialized = false;
	static LARGE_INTEGER initial_count;
	static double nanoseconds_per_count;

	if (!initialized)
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		nanoseconds_per_count = 1000.0 * 1000.0 * 1000.0 / double(freq.QuadPart);

		QueryPerformanceCounter(&initial_count);

		initialized = true;
	}

	LARGE_INTEGER current_count;
	QueryPerformanceCounter(&current_count);
	return nanoseconds_per_count * (current_count.QuadPart - initial_count.QuadPart);

#else
	// [SL] use SDL_GetTicks, but account for the fact that after
	// 49 days, it wraps around since it returns a 32-bit int
	static constexpr uint64_t mask = 0xFFFFFFFFLL;
	static uint64_t last_time = 0LL;
	uint64_t current_time = SDL_GetTicks();

	if (current_time < (last_time & mask))      // just wrapped around
		last_time += mask + 1 - (last_time & mask) + current_time;
	else
		last_time = current_time;

	return last_time * 1000000LL;

#endif
}

dtime_t I_MSTime()
{
	return I_ConvertTimeToMs(I_GetTime());
}

dtime_t I_ConvertTimeToMs(dtime_t value)
{
	return value / 1000000LL;
}

dtime_t I_ConvertTimeFromMs(dtime_t value)
{
	return value * 1000000LL;
}

//
// I_Sleep
//
// Sleeps for the specified number of nanoseconds, yielding control to the
// operating system. In actuality, the highest resolution availible with
// the select() function is 1 microsecond, but the nanosecond parameter
// is used for consistency with I_GetTime().
//
void I_Sleep(dtime_t sleep_time)
{
#if defined UNIX
	usleep(sleep_time / 1000LL);

#elif defined(WIN32)
	Sleep(sleep_time / 1000000LL);

#else
	SDL_Delay(sleep_time / 1000000LL);

#endif
}

//
// I_Yield
//
// Sleeps for 1 millisecond
//
void I_Yield()
{
	I_Sleep(1000LL * 1000LL);		// sleep for 1ms
}

//
// I_WaitVBL
//
// I_WaitVBL is never used to actually synchronize to the
// vertical blank. Instead, it's used for delay purposes.
//
void I_WaitVBL(int count)
{
	I_Sleep(1000000LL * 1000LL * count / 70);
}
