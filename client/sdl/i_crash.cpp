// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2016 by The Odamex Team.
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
//   Crash handling.
//
//   This document is invaluable for documenting the ways programs compiled
//   with Visual Studio can die - just don't copy any code from it, the license
//   is not GPL-compatible:
//
//       http://www.codeproject.com/Articles/207464/Exception-Handling-in-Visual-Cplusplus
//
//-----------------------------------------------------------------------------

#if defined _WIN32 && !defined _XBOX && defined _MSC_VER

#include <csignal>
#include <cstdio>
#include <exception>
#include <new.h>

#include "win32inc.h"
#include <DbgHelp.h>

// Fucntion pointer for MiniDumpWriteDump.
typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

// Write the minidump to a file.
void writeMinidump(EXCEPTION_POINTERS* exceptionPtrs)
{
	// Grab the debugging library.
	HMODULE dbghelp = LoadLibrary("dbghelp.dll");
	if (dbghelp == NULL)
	{
		// We can't load the debugging library - oh well.
		return;
	}

	// Grab the dump function.
	MINIDUMPWRITEDUMP pMiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress(dbghelp, "MiniDumpWriteDump");
	if (pMiniDumpWriteDump == NULL)
	{
		// We can't access the dumping function - oh well.
		return;
	}

	// Open a file to write our dump into.
	SYSTEMTIME dt;
	GetSystemTime(&dt);
	char filename[256];
	sprintf_s(filename, sizeof(filename), "odamex_%4d%02d%02d_%02d%02d%02d.dmp",
	          dt.wYear, dt.wMonth, dt.wDay, dt.wHour, dt.wMinute, dt.wSecond);
	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		// We couldn't create a dump file - oh well.
		return;
	}

	// Prepare exception information.
	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = exceptionPtrs;
	mei.ClientPointers = FALSE;

	// Do the actual dump.
	pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory), &mei, 0, 0);
	CloseHandle(hFile);

	return;
}

LONG CALLBACK sehCallback(EXCEPTION_POINTERS* e)
{
	writeMinidump(e);
	return EXCEPTION_CONTINUE_SEARCH;
}

void terminateCallback()
{
	// Exception pointer is located at _pxcptinfoptrs.
	writeMinidump(static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs));

	// Set the standard abort handler.
	std::signal(SIGABRT, SIG_DFL);

	// Die screaming.
	std::abort();
}

void purecallCallback()
{
	// Exception pointer is located at _pxcptinfoptrs.
	writeMinidump(static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs));

	// Set the standard abort handler.
	std::signal(SIGABRT, SIG_DFL);

	// Die screaming.
	std::abort();
}

void invalidparamCallback(const wchar_t* expression, const wchar_t* function, const wchar_t* filename, unsigned int line, uintptr_t x)
{
	// Exception pointer is located at _pxcptinfoptrs.
	writeMinidump(static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs));

	// Set the standard abort handler.
	std::signal(SIGABRT, SIG_DFL);

	// Die screaming.
	std::abort();
}

// [AM] In my testing, memory allocation failures return a dump of size 0.
//      In theory, you could work around this by spawning another process
//      and doing them dumping there, but I could never figure out how to
//      pass the necessary data along with it.
int newCallback(size_t)
{
	// Exception pointer is located at _pxcptinfoptrs.
	writeMinidump(static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs));

	// Set the standard abort handler.
	std::signal(SIGABRT, SIG_DFL);

	// Die screaming.
	std::abort();
}

void signalCallback(int sig)
{
	// Set our default signal handlers.
	std::signal(SIGILL, SIG_DFL);
	std::signal(SIGABRT, SIG_DFL);
	std::signal(SIGFPE, SIG_DFL);
	std::signal(SIGSEGV, SIG_DFL);

	// Exception pointer is located at _pxcptinfoptrs.
	writeMinidump(static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs));

	// Once we're done, bail out.
	std::abort();
}

void I_SetCrashCallbacks()
{
	// Structured Exception Handling is how 99% of Windows crashes are handled.
	SetUnhandledExceptionFilter(sehCallback);

	// Intercept calls to std::terminate().
	set_terminate(terminateCallback);

	// Pure virtual function calls.
	_set_purecall_handler(purecallCallback);

	// CRT invalid parameters.
	_set_invalid_parameter_handler(invalidparamCallback);

	// Memory allocation failures.
	_set_new_handler(newCallback);

	// Old-school UNIX signals.
	std::signal(SIGILL, signalCallback);
	std::signal(SIGABRT, signalCallback);
	std::signal(SIGFPE, signalCallback);
	std::signal(SIGSEGV, signalCallback);
}

#elif defined(UNIX)

#include <cstdio>
#include <csignal>
#include <cstring>

#include <execinfo.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

// Write a backtrace to a file.
//
// This is not a "safe" signal handler, but this is used in a process
// that is already crashing, and is meant to provide as much
// information as reasonably possible in the potential absence of a
// core dump.
void writeBacktrace(int sig, siginfo_t* si)
{
	char buf[256];
	int len = 0;

	// Open a file to write our dump into.
	len = snprintf(buf, sizeof(buf), "odamex_%d_dump.txt", (long long)time(NULL));
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
	write(fd, buf, len);

	len = snprintf(buf, sizeof(buf), "Errno: %d\n", si->si_errno);
	if (len < 0)
	{
		return;
	}
	write(fd, buf, len);

	len = snprintf(buf, sizeof(buf), "Signal code: %d\n", si->si_code);
	if (len < 0)
	{
		return;
	}
	write(fd, buf, len);

	len = snprintf(buf, sizeof(buf), "Fault Address: %p\n", si->si_addr);
	if (len < 0)
	{
		return;
	}
	write(fd, buf, len);

	// Get backtrace data.
	void* bt[50];
	size_t size = backtrace(bt, sizeof(bt) / sizeof(void*));

	// Write stack frames to file.
	backtrace_symbols_fd(bt, size, fd);
	close(fd);
}

void sigactionCallback(int sig, siginfo_t* si, void* ctx)
{
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

#else

void I_SetCrashCallbacks()
{
	// Not implemented.
}

#endif
