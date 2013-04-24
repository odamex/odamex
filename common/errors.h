// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	Error Handling
//
//-----------------------------------------------------------------------------


#ifndef __ERRORS_H__
#define __ERRORS_H__

#include <string>

class CDoomError
{
public:
	CDoomError (std::string message) : m_Message(message) {}
	std::string GetMsg (void) const { return m_Message;	}

private:
	std::string m_Message;
};

class CRecoverableError : public CDoomError
{
public:
	CRecoverableError(std::string message) : CDoomError(message) {}
};

class CFatalError : public CDoomError
{
public:
	CFatalError(std::string message) : CDoomError(message) {}
};

#endif //__ERRORS_H__

