// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
//   POSIX Crash handling.
//
//-----------------------------------------------------------------------------

#if defined UNIX && !defined GEKKO

#define CRASH_DIR_LEN 1024

#include "i_crash.h"

#include <csignal>
#include <cstdio>
#include <cstring>

#include <execinfo.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "i_system.h"

/**
 * @brief An array containing the directory where crashes are written to.
 */
static char gCrashDir[CRASH_DIR_LEN];

// Write a backtrace to a file.
//
// This is not a "safe" signal handler, but this is used in a process
// that is already crashing, and is meant to provide as much
// information as reasonably possible in the potential absence of a
// core dump.
void writeBacktrace(int sig, siginfo_t* si)
{
	char buf[CRASH_DIR_LEN];
	int len = 0;

	// Open a file to write our dump into.
	len = snprintf(buf, sizeof(buf), "%s/odamex_%lld_dump.txt", ::gCrashDir,
	               (long long)time(NULL));
	if (len < 0)
	{
		return;
	}
	int fd = creat(buf, 0644);
	if (fd == -1)
	{
		// We couldn't create a dump file - oh well.
		return;
	}

	len = snprintf(buf, sizeof(buf), "Signal number: %d\n", si->si_signo);
	if (len < 0)
	{
		return;
	}
	ssize_t writelen = write(fd, buf, len);
	if (writelen < 1)
	{
		printf("writeBacktrace(): failed to write to fd\n");
	}

	len = snprintf(buf, sizeof(buf), "Errno: %d\n", si->si_errno);
	if (len < 0)
	{
		return;
	}
	writelen = write(fd, buf, len);
	if (writelen < 1)
	{
		printf("writeBacktrace(): failed to write to fd\n");
	}

	len = snprintf(buf, sizeof(buf), "Signal code: %d\n", si->si_code);
	if (len < 0)
	{
		return;
	}
	writelen = write(fd, buf, len);
	if (writelen < 1)
	{
		printf("writeBacktrace(): failed to write to fd\n");
	}

	len = snprintf(buf, sizeof(buf), "Fault Address: %p\n", si->si_addr);
	if (len < 0)
	{
		return;
	}
	writelen = write(fd, buf, len);
	if (writelen < 1)
	{
		printf("writeBacktrace(): failed to write to fd\n");
	}

	// Get backtrace data.
	void* bt[50];
	size_t size = backtrace(bt, sizeof(bt) / sizeof(void*));

	// Write stack frames to file.
	backtrace_symbols_fd(bt, size, fd);
	close(fd);
}

void sigactionCallback(int sig, siginfo_t* si, void* ctx)
{
	fprintf(stderr, "sigactionCallback\n");

	// Change our signal handler back to default.
	struct sigaction act;
	std::memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_DFL;

	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGILL, &act, NULL);
	sigaction(SIGABRT, &act, NULL);
	sigaction(SIGFPE, &act, NULL);
	sigaction(SIGSEGV, &act, NULL);
	sigaction(SIGBUS, &act, NULL);

	// Write out the backtrace
	writeBacktrace(sig, si);

	// Once we're done, re-raise the signal.
	kill(getpid(), sig);
}

void I_SetCrashCallbacks()
{
	fprintf(stderr, "I_SetCrashCallbacks\n");

	struct sigaction act;
	std::memset(&act, 0, sizeof(act));

	act.sa_sigaction = &sigactionCallback;
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGILL, &act, NULL);
	sigaction(SIGABRT, &act, NULL);
	sigaction(SIGFPE, &act, NULL);
	sigaction(SIGSEGV, &act, NULL);
	sigaction(SIGBUS, &act, NULL);
}

void I_SetCrashDir(const char* crashdir)
{
	std::string homedir;
	char testfile[CRASH_DIR_LEN];

	// Check to see if our crash dir is too big.
	size_t len = strlen(crashdir);
	if (len > CRASH_DIR_LEN)
	{
		I_FatalError(
		    "Crash directory is too long.\nPlease pass a correct -crashout param.");
		std::abort();
	}

	// Check to see if we can write to our crash directory.
	snprintf(testfile, ARRAY_LENGTH(testfile), "%s/crashXXXXXX", crashdir);
	int res = mkstemp(testfile);
	if (res == -1)
	{
		I_FatalError("Crash directory is not writable.\nPlease point -crashout to "
		             "a directory with write permissions.");
		std::abort();
	}

	// We don't need the temporary file anymore.
	remove(testfile);

	// Copy the crash directory.
	memcpy(::gCrashDir, crashdir, len);
}

#endif
