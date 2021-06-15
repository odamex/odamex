// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// DESCRIPTION:
//  Subprocess class
//
//-----------------------------------------------------------------------------

#include "common.h"

#include "subproc.h"

void subprocWin_t::create(HANDLE process, HANDLE thread, HANDLE stdIn, HANDLE stdOut,
                          HANDLE stdErr)
{
	m_process = process;
	m_thread = thread;
	m_stdIn = stdIn;
	m_stdOut = stdOut;
	m_stdErr = stdErr;
}

void subprocWin_t::destroy()
{
	CloseHandle(m_process);
	CloseHandle(m_thread);
	CloseHandle(m_stdIn);
	CloseHandle(m_stdOut);
	CloseHandle(m_stdErr);
}

bool subprocWin_t::send(const std::string& in)
{
	DWORD written;
	return WriteFile(m_stdIn, in.data(), in.size(), &written, NULL);
}

bool subprocWin_t::readOut(std::string& out)
{
	const DWORD BLOCKSIZE = 4096;
	ReadFile(m_stdOut);
}

bool subprocWin_t::readErr(std::string& err)
{
	ReadFile(m_stdErr);
}

result_t<bool> WinStartServer(subprocWin_t& subproc)
{
	// Command to run.
	TCHAR appName[128];
	ZeroMemory(appName, sizeof(appName));
	CopyMemory(appName, "Debug\\odasrv.exe", sizeof("Debug\\odasrv.exe"));

	// Full command to run, with arguments.
	TCHAR cmdLine[128];
	ZeroMemory(cmdLine, sizeof(cmdLine));
	cmdLine[0] = '\0';

	// Set up pipes
	SECURITY_ATTRIBUTES secAttrs;
	ZeroMemory(&secAttrs, sizeof(secAttrs));
	secAttrs.nLength = sizeof(secAttrs);
	secAttrs.bInheritHandle = TRUE;
	secAttrs.lpSecurityDescriptor = NULL;

	HANDLE stdInR, stdInW;
	if (!CreatePipe(&stdInR, &stdInW, &secAttrs, 0))
	{
		return result_t<bool>(false, "Could not initialize midiproc stdin");
	}

	if (!SetHandleInformation(stdInW, HANDLE_FLAG_INHERIT, 0))
	{
		return result_t<bool>(false, "Could not disinherit midiproc stdin");
	}

	HANDLE stdOutR, stdOutW;
	if (!CreatePipe(&stdOutR, &stdOutW, &secAttrs, 0))
	{
		return result_t<bool>(false, "Could not initialize midiproc stdout");
	}

	if (!SetHandleInformation(stdOutR, HANDLE_FLAG_INHERIT, 0))
	{
		return result_t<bool>(false, "Could not disinherit midiproc stdout");
	}

	HANDLE stdErrR, stdErrW;
	if (!CreatePipe(&stdErrR, &stdErrW, &secAttrs, 0))
	{
		return result_t<bool>(false, "Could not initialize midiproc stderr");
	}

	if (!SetHandleInformation(stdErrR, HANDLE_FLAG_INHERIT, 0))
	{
		return result_t<bool>(false, "Could not disinherit midiproc stderr");
	}

	// Startup info.
	STARTUPINFOA startupInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.dwFlags = STARTF_USESTDHANDLES;
	startupInfo.hStdInput = stdInR;
	startupInfo.hStdOutput = stdOutW;
	startupInfo.hStdError = stdErrW;

	// Process info.
	PROCESS_INFORMATION procInfo;
	ZeroMemory(&procInfo, sizeof(procInfo));

	if (!CreateProcess(appName, cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL,
	                   &startupInfo, &procInfo))
	{
		DWORD error = GetLastError();
		return false;
	}

	// Since the server has these handles, we don't need them anymore.
	CloseHandle(stdInR);
	CloseHandle(stdOutW);
	CloseHandle(stdErrW);

	// The other three handles are our end, so assign them.
	subproc.create(procInfo.hProcess, procInfo.hThread, stdInW, stdOutR, stdErrR);

	return true;
}
