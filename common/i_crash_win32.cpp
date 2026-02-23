// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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


#if defined _WIN32 && defined _MSC_VER && !defined _DEBUG

#include "odamex.h"

#define CRASH_DIR_LEN 1024

#include "i_crash.h"

#include <exception>
#include <new.h>
#include <signal.h>

#include "win32inc.h"
#include <DbgHelp.h>

// Must be loaded last or else we're missing functions.

#include "i_system.h"
#include "m_fileio.h"

/**
 * @brief An array containing the directory where crashes are written to.
 */
static TCHAR gCrashDir[CRASH_DIR_LEN];

// Fucntion pointer for MiniDumpWriteDump.
typedef bool(WINAPI* MiniDumpWriteDumpPtrType)(
    HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

class MiniDumper
{
	public:

		MiniDumper() :
			m_dbghelp(nullptr),
			m_MiniDumpWriteDumpPtr(nullptr)
		{
			m_dbghelp = LoadLibrary("dbghelp.dll");
			if (m_dbghelp)
			{
				m_MiniDumpWriteDumpPtr = reinterpret_cast<MiniDumpWriteDumpPtrType>(GetProcAddress(m_dbghelp, "MiniDumpWriteDump"));
			}
		}

		~MiniDumper()
		{
			if (m_dbghelp)
			{
				FreeLibrary(m_dbghelp);
			}
		}

		MiniDumper(const MiniDumper&)              = delete;
		MiniDumper& operator=(const MiniDumper&)   = delete;
		MiniDumper(const MiniDumper&&)             = delete;
		MiniDumper& operator==(const MiniDumper&&) = delete;

		void Write(EXCEPTION_POINTERS* exceptionPtrs,
		           const char*         filepath,
		           MINIDUMP_TYPE       dumpTypeFlags)
		{
			if (m_MiniDumpWriteDumpPtr)
			{
				HANDLE hFile = CreateFile(filepath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS,
				                          FILE_ATTRIBUTE_NORMAL, 0);

				if (hFile != INVALID_HANDLE_VALUE)
				{
					// Prepare exception information.
					MINIDUMP_EXCEPTION_INFORMATION mei;
					mei.ThreadId = GetCurrentThreadId();
					mei.ExceptionPointers = exceptionPtrs;
					mei.ClientPointers = false;

					// Do the actual dump.
					m_MiniDumpWriteDumpPtr(GetCurrentProcess(),
					                       GetCurrentProcessId(),
					                       hFile,
					                       dumpTypeFlags,
					                       & mei,
					                       0,
					                       0);
					CloseHandle(hFile);
				}
			}
		}

	protected:

		HMODULE                  m_dbghelp;
		MiniDumpWriteDumpPtrType m_MiniDumpWriteDumpPtr;
};

// Write the minidumps.
void writeMinidump(EXCEPTION_POINTERS* exceptionPtrs)
{
	// Force the variables to be static because we want to ensure that we're as
	// gentle as possible with the stack in what could be a precarious situation.

	static SYSTEMTIME dt;
	GetSystemTime(&dt);

	static char filepath[CRASH_DIR_LEN];
	sprintf_s(filepath, sizeof(filepath), "%s\\%s_g%s_%4d%02d%02dT%02d%02d%02d_%u.dmp",
	          ::gCrashDir,
	          GAMEEXE,
	          GitShortHash(),
	          dt.wYear,
	          dt.wMonth,
	          dt.wDay,
	          dt.wHour,
	          dt.wMinute,
	          dt.wSecond,
	          GetCurrentProcessId());

    static MiniDumper dumper;
	dumper.Write(exceptionPtrs,
	             filepath,
	             static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory));

	// Now make a single massive .dmp that has everything in it for thorough port-mortem
	// debugging.  Unlike the slim minidump above, this one is intended to be overwriten
	// with successive crashes, thus we don't include the timestamp, hash, or PID in the
	// filename.  This is so that we don't have monster dmp files accumulating unbounded
	// in the CrashDir.

	sprintf_s(filepath, sizeof(filepath), "%s\\%s_FULL.dmp",
              ::gCrashDir,
              GAMEEXE);

	dumper.Write(exceptionPtrs,
	             filepath,
	             static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory |
	                                        MiniDumpIgnoreInaccessibleMemory));
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
		    "Crash directory \"{}\" is too long.  Please pass a correct -crashdir param.",
		    crashdir);
		abort();
	}

	// Check to see if we can write to our crash directory.
	UINT res = GetTempFileName(crashdir, "crash", 0, testfile);
	if (res == 0 || res == ERROR_BUFFER_OVERFLOW)
	{
		I_FatalError("Crash directory \"{}\" is not writable.  Please point -crashdir to "
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
