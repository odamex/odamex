// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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


#if defined UNIX && defined HAVE_BACKTRACE && !defined GCONSOLE

#include "odamex.h"

#define CRASH_DIR_LEN 1024

#include "i_crash.h"

#include <signal.h>

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
static void WriteBacktrace(int sig, siginfo_t* si)
{
	// Generate a timestamp.
	time_t now = time(NULL);
	struct tm* local = localtime(&now);

	char timebuf[1024];
	strftime(timebuf, ARRAY_LENGTH(timebuf), "%Y%m%dT%H%M%S", local);

	// Find the spot to write our backtrace.
	int len = 0;
	char filename[CRASH_DIR_LEN];
	len = snprintf(filename, ARRAY_LENGTH(filename), "%s/%s_g%s_%d_%s_dump.txt",
	               ::gCrashDir, GAMEEXE, GitShortHash(), getpid(), timebuf);
	if (len < 0)
	{
		fprintf(stderr, "%s: File path too long.\n", __FUNCTION__);
		return;
	}

	// Create a file.
	int fd = creat(filename, 0644);
	if (fd == -1)
	{
		fprintf(stderr, "%s: File could not be created.\n", __FUNCTION__);
		return;
	}

	// Stamp out the header.
	char buf[1024];
	len = snprintf(buf, sizeof(buf),
	               "Signal number: %d\nErrno: %d\nSignal code: %d\nFault Address: %p\n",
	               si->si_signo, si->si_errno, si->si_code, si->si_addr);
	if (len < 0)
	{
		fprintf(stderr, "%s: Header too long for buffer.\n", __FUNCTION__);
		return;
	}

	// Write the header.
	ssize_t writelen = write(fd, buf, len);
	if (writelen < 1)
	{
		fprintf(stderr, "%s: Failed to write to fd.\n", __FUNCTION__);
		return;
	}

	// Get backtrace data.
	void* bt[50];
	size_t size = backtrace(bt, ARRAY_LENGTH(bt));

	// Write stack frames to file.
	backtrace_symbols_fd(bt, size, fd);
	close(fd);

	// Tell the user about it.
	fprintf(stderr, "Wrote \"%s\".\n", filename);
}

static void SigActionCallback(int sig, siginfo_t* si, void* ctx)
{
	fprintf(stderr, "Caught Signal %d (%s), dumping crash info...\n", si->si_signo,
	        strsignal(si->si_signo));

	// Change our signal handler back to default.
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_DFL;

	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGILL, &act, NULL);
	sigaction(SIGABRT, &act, NULL);
	sigaction(SIGFPE, &act, NULL);
	sigaction(SIGSEGV, &act, NULL);
	sigaction(SIGBUS, &act, NULL);

	// Write out the backtrace
	WriteBacktrace(sig, si);

	// Once we're done, re-raise the signal.
	kill(getpid(), sig);
}

void I_SetCrashCallbacks()
{
	struct sigaction act;
	memset(&act, 0, sizeof(act));

	act.sa_sigaction = &SigActionCallback;
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
		    "Crash directory \"%s\" is too long.  Please pass a correct -crashout param.",
		    crashdir);
		abort();
	}

	// Check to see if we can write to our crash directory.
	snprintf(testfile, ARRAY_LENGTH(testfile), "%s/crashXXXXXX", crashdir);
	int res = mkstemp(testfile);
	if (res == -1)
	{
		I_FatalError("Crash directory \"%s\" is not writable.  Please point -crashout to "
		             "a directory with write permissions.",
		             crashdir);
		abort();
	}

	// We don't need the temporary file anymore.
	remove(testfile);

	// Copy the crash directory.
	memcpy(::gCrashDir, crashdir, len);
}

#endif

#if defined(__SWITCH__)

#include <switch.h>
#include <stdlib.h>

extern "C"
{

alignas(16) u8 __nx_exception_stack[0x1000];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);

void __libnx_exception_handler(ThreadExceptionDump *ctx)
{
	fprintf(stderr, "FATAL CRASH! See odamex_crash.log for details\n");

	FILE *f = fopen("./odamex_crash.log", "w");
	if (f == NULL) return;

	fprintf(f, "error_desc: 0x%x\n", ctx->error_desc);
	for(int i = 0; i < 29; i++)
		fprintf(f, "[X%d]: 0x%lx\n", i, ctx->cpu_gprs[i].x);

	fprintf(f, "fp: 0x%lx\n", ctx->fp.x);
	fprintf(f, "lr: 0x%lx\n", ctx->lr.x);
	fprintf(f, "sp: 0x%lx\n", ctx->sp.x);
	fprintf(f, "pc: 0x%lx\n", ctx->pc.x);

	fprintf(f, "pstate: 0x%x\n", ctx->pstate);
	fprintf(f, "afsr0: 0x%x\n", ctx->afsr0);
	fprintf(f, "afsr1: 0x%x\n", ctx->afsr1);
	fprintf(f, "esr: 0x%x\n", ctx->esr);

	fprintf(f, "far: 0x%lx\n", ctx->far.x);

	fclose(f);
}
}

void I_SetCrashCallbacks()
{
}

void I_SetCrashDir(const char* crashdir)
{
}

#endif
