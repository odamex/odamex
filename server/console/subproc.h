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

#pragma once

#include "common.h"

class subproc_t
{
  public:
	virtual bool send(const std::string& in) = 0;
	virtual bool readOut(std::string& out) = 0;
	virtual bool readErr(std::string& err) = 0;
	virtual void destroy() = 0;
};

class subprocWin_t final : public subproc_t
{
	HANDLE m_process;
	HANDLE m_thread;
	HANDLE m_stdIn;
	HANDLE m_stdOut;
	HANDLE m_stdErr;

  public:
	void create(HANDLE process, HANDLE thread, HANDLE stdIn, HANDLE stdOut,
	            HANDLE stdErr);
	void destroy();
	bool send(const std::string& in);
	bool readOut(std::string& out);
	bool readErr(std::string& err);
};

result_t<bool> WinStartServer(subproc_t& subproc);
