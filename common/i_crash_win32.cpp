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
//   Windows Crash handling.
//
//   This document is invaluable for documenting the ways programs compiled
//   with Visual Studio can die - just don't copy any code from it, the license
//   is not GPL-compatible:
//
//       http://www.codeproject.com/Articles/207464/Exception-Handling-in-Visual-Cplusplus
//
//-----------------------------------------------------------------------------

#if defined _WIN32 && !defined _XBOX && defined _MSC_VER && !defined _DEBUG

#define CRASH_DIR_LEN 1024

#include "i_crash.h"

#include <exception>
#include <new.h>
#include <signal.h>
#include <stdio.h>

#include "win32inc.h"
#include <DbgHelp.h>

// Must be loaded last or else we're missing functions.

#include "doomtype.h"
#include "i_system.h"
#include "m_fileio.h"

/**
 * @brief An array containing the directory where crashes are written to.
 */
static TCHAR gCrashDir[CRASH_DIR_LEN];

// Fucntion pointer for MiniDumpWriteDump.
typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(
    HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

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
	MINIDUMPWRITEDUMP pMiniDumpWriteDump =
	    (MINIDUMPWRITEDUMP)GetProcAddress(dbghelp, "MiniDumpWriteDump");
	if (pMiniDumpWriteDump == NULL)
	{
		// We can't access the dumping function - oh well.
		return;
	}

	// Open a file to write our dump into.
	SYSTEMTIME dt;
	GetSystemTime(&dt);
	char filename[CRASH_DIR_LEN];
	sprintf_s(filename, sizeof(filename), "%s\\%s_g%s_%u_%4d%02d%02dT%02d%02d%02d.dmp",
	          ::gCrashDir, GAMEEXE, GitShortHash(), GetCurrentProcessId(), dt.wYear,
	          dt.wMonth, dt.wDay, dt.wHour, dt.wMinute, dt.wSecond);
	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW,
	                          FILE_ATTRIBUTE_NORMAL, 0);
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
	pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
	                   MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory), &mei, 0, 0);
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
	signal(SIGABRT, SIG_DFL);

	// Die screaming.
	abort();
}

void purecallCallback()
{
	// Exception pointer is located at _pxcptinfoptrs.
	writeMinidump(static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs));

	// Set the standard abort handler.
	signal(SIGABRT, SIG_DFL);

	// Die screaming.
	abort();
}

void invalidparamCallback(const wchar_t* expression, const wchar_t* function,
                          const wchar_t* filename, unsigned int line, uintptr_t x)
{
	// Exception pointer is located at _pxcptinfoptrs.
	writeMinidump(static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs));

	// Set the standard abort handler.
	signal(SIGABRT, SIG_DFL);

	// Die screaming.
	abort();
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
	signal(SIGABRT, SIG_DFL);

	// Die screaming.
	abort();
}

void signalCallback(int sig)
{
	// Set our default signal handlers.
	signal(SIGILL, SIG_DFL);
	signal(SIGABRT, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);

	// Exception pointer is located at _pxcptinfoptrs.
	writeMinidump(static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs));

	// Once we're done, bail out.
	abort();
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
	signal(SIGILL, signalCallback);
	signal(SIGABRT, signalCallback);
	signal(SIGFPE, signalCallback);
	signal(SIGSEGV, signalCallback);
}

void I_SetCrashDir(const char* crashdir)
{
	std::string homedir;
	TCHAR testfile[MAX_PATH];

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
	UINT res = GetTempFileName(crashdir, "crash", 0, testfile);
	if (res == 0 || res == ERROR_BUFFER_OVERFLOW)
	{
		I_FatalError("Crash directory \"%s\" is not writable.  Please point -crashout to "
		             "a directory with write permissions.",
		             crashdir);
		abort();
	}

	// We don't need the temporary file anymore.
	DeleteFile(testfile);

	// Copy the crash directory.
	memcpy(::gCrashDir, crashdir, len);
}

#endif
