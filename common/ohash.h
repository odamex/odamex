// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
// DESCRIPTION:
//  A collection of strongly typed hash types.
//
//-----------------------------------------------------------------------------

#pragma once

#include "hashtable.h"

class OHash
{
  private:
	virtual void concrete() = 0; // [AM] Hack to make this class abstract.

  protected:
	std::string m_hash;

  public:
	virtual ~OHash() { }
	bool operator==(const OHash& other) const { return m_hash == other.m_hash; }
	bool operator!=(const OHash& other) const { return !(operator==(other)); }
	const std::string& getHexStr() const { return m_hash; }
	const char* getHexCStr() const { return m_hash.c_str(); }
	bool empty() const { return m_hash.empty(); }
};

class OMD5Hash : public OHash
{
  protected:
	void concrete() { }

  public:
	static bool makeFromHexStr(OMD5Hash& out, const std::string& hash);
};

class OCRC32Sum : public OHash
{
  protected:
	void concrete() { }

  public:
	static bool makeFromHexStr(OCRC32Sum& out, const std::string& hash);
};

template <>
struct hashfunc<OCRC32Sum>
{
	unsigned int operator()(const OCRC32Sum& str) const
	{
		return __hash_cstring(str.getHexCStr());
	}
};

template <>
struct hashfunc<OMD5Hash>
{
	unsigned int operator()(const OMD5Hash& str) const
	{
		return __hash_cstring(str.getHexCStr());
	}
};
