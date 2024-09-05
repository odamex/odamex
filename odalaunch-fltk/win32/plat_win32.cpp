// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Platform-specific functions.
//
//-----------------------------------------------------------------------------

#include "plat.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string.h>

/******************************************************************************/

class PlatformWin32 : public Platform
{
public:
	virtual void DebugOut(const char* str) override;
	virtual void ExecuteOdamex() override;
};

/******************************************************************************/

void PlatformWin32::DebugOut(const char* str)
{
	OutputDebugStringA(str);
}

/******************************************************************************/

void PlatformWin32::ExecuteOdamex()
{
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	std::wstring path = L"";
	std::wstring args = L"";

	static wchar_t argsBuffer[32767];
	wcsncpy(argsBuffer, args.c_str(), ARRAY_LENGTH(argsBuffer) - 1);

	CreateProcess(path.c_str(), argsBuffer, nullptr, nullptr, FALSE, 0, nullptr, nullptr,
	              &si, &pi);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}

/******************************************************************************/

Platform& Platform::Get()
{
	static PlatformWin32 s_cPlatform;
	return s_cPlatform;
}
