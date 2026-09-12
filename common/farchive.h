// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
// DESCRIPTION:
//	FARCHIVE
//
//-----------------------------------------------------------------------------


#pragma once

#include <array>
#include <cassert>
#include <type_traits>

#include "dobject.h"
#include "doomtype.h"
#include "flags.h"

#define FA_RESET (1 << 0)

class DObject;

class FFile
{
public:
	enum EOpenMode
	{
		EReading,
		EWriting,
		ENotOpen
	};

	enum ESeekPos
	{
		ESeekSet,
		ESeekRelative,
		ESeekEnd
	};

	virtual	~FFile() {}

	virtual	bool Open(const char* name, EOpenMode mode) = 0;
	virtual	void Close() = 0;
	virtual	void Flush() = 0;
	virtual EOpenMode Mode() const = 0;
	virtual bool IsPersistent() const = 0;
	virtual bool IsOpen() const = 0;

	virtual	FFile& Write(const void*, unsigned int) = 0;
	virtual	FFile& Read(void*, unsigned int) = 0;

	virtual	unsigned int Tell() const = 0;
	virtual	FFile& Seek(int, ESeekPos) = 0;
	inline	FFile& Seek(unsigned int i, ESeekPos p) { return Seek(static_cast<int>(i), p); }
};

class FLZOFile : public FFile
{
public:
	FLZOFile();
	FLZOFile(const char* name, EOpenMode mode, bool dontcompress = false);
	FLZOFile(FILE* file, EOpenMode mode, bool dontcompress = false);
	~FLZOFile();

	bool Open(const char* name, EOpenMode mode) override;
	void Close() override;
	void Flush() override;
	EOpenMode Mode() const override;
	bool IsPersistent() const override { return true; }
	bool IsOpen() const override;

	FFile& Write(const void*, unsigned int) override;
	FFile& Read(void*, unsigned int) override;
	unsigned int Tell() const override;
	FFile& Seek(int, ESeekPos) override;

protected:
	unsigned int m_Pos;
	unsigned int m_BufferSize;
	unsigned int m_MaxBufferSize;
	unsigned char* m_Buffer;
	bool m_NoCompress;
	EOpenMode m_Mode;
	FILE* m_File;

	virtual void Implode();
	virtual void Explode();
	virtual bool FreeOnExplode() { return true; }

private:
	void clear();
	void PostOpen();
};

class FLZOMemFile : public FLZOFile
{
public:
	FLZOMemFile();

	~FLZOMemFile() override;

	bool Open(const char* name, EOpenMode mode) override;	// Works for reading only
	virtual bool Open(void* memblock);	// Open for reading only
	virtual bool Open();	// Open for writing only
	virtual bool Reopen();	// Re-opens imploded file for reading only
	void Close() override;
	bool IsOpen() const override;

	void Serialize(FArchive &arc);

	size_t Length() const;
	void WriteToBuffer(void* buf, size_t length) const;

protected:
	bool FreeOnExplode() override { return !m_SourceFromMem; }

private:
	bool m_SourceFromMem;
	unsigned char* m_ImplodedBuffer;
};

class FArchive
{
public:
	FArchive(FFile& file, uint32_t flags = 0);
	virtual ~FArchive();

	inline bool IsLoading() const { return m_Loading; }
	inline bool IsStoring() const { return m_Storing; }
	inline bool IsPeristent() const { return m_Persistent; }
	inline bool IsReset() const { return m_Reset; }

	void SetHubTravel() { m_HubTravel = true; }

	void Close();

	virtual	void Write(const void* mem, unsigned int len);
	virtual void Read(void* mem, unsigned int len);

	void WriteCount(uint32_t count);
	uint32_t ReadCount();

	// ------------ Streaming-in operations ---------------

	// Integer operators
	FArchive& operator<< (std::integral auto value)
	{
		value = BESWAP(value);
		Write(&value, sizeof(value));
		return *this;
	}

	// any enum or enum class without their own overload for operator<<
	// will use this version
	FArchive& operator<< (const OUtil::Enum auto value)
	{
		*this << OUtil::to_underlying(value);
		return *this;
	}

	template <typename E>
	FArchive& operator<< (const OFlags<E> value)
	{
		*this << value.to_int();
		return *this;
	}

	// Overload bool because its size is implementation-defined, and we want archived sizes to be exact.
	FArchive& operator<< (bool b) { return operator<< (uint8_t(b)); }

	template <typename ElementType, size_t N>
	FArchive& operator<< (const std::array<ElementType, N>& i_array)
	{
		*this << i_array.size();
		for (const auto& element : i_array)
		{
			*this << element;
		}
		return *this;
	}

	FArchive& operator<< (float f);
	FArchive& operator<< (double d);
	FArchive& operator<< (argb_t color);
	FArchive& operator<< (const std::string& str);
	FArchive& operator<< (const char* str);
	FArchive& operator<< (DObject* obj);

	FArchive& operator<< (const unsigned char* str) { return operator<< (reinterpret_cast<const char*>(str)); }
	FArchive& operator<< (const signed char* str) { return operator<< (reinterpret_cast<const char*>(str)); }

	// ------------ Streaming-out operations ---------------

	// Integer operators
	FArchive& operator>> (std::integral auto& value)
	{
		Read(&value, sizeof(value));
		value = BESWAP(value);
		return *this;
	}

	template <OUtil::Enum E>
	FArchive& operator>> (E& value)
	{
		std::underlying_type_t<E> temp;
		*this >> temp;
		value = static_cast<E>(temp);
		return *this;
	}

	template <typename E>
	FArchive& operator>> (OFlags<E>& value)
	{
		std::remove_cvref_t<decltype(value.to_int())> temp;
		*this >> temp;
		value = OFlags<E>::unsafe_from_int(temp);
		return *this;
	}

	// Overload bool because its size is implementation-defined, and we want archived sizes to be exact.
	FArchive& operator>> (bool& b)
	{
		uint8_t value;
		*this >> value;
		b = static_cast<bool>(value);
		return *this;
	}

	template <typename ElementType, size_t N>
	FArchive& operator>> (std::array<ElementType, N>& o_array)
	{
		size_t arraySize{0};

		*this >> arraySize;
		// TODO: should we probably just use I_Error here?
		assert(arraySize == o_array.size());

		for (auto& element : o_array)
		{
			*this >> element;
		}
		return *this;
	}

	FArchive& operator>> (float& f);
	FArchive& operator>> (double& d);
	FArchive& operator>> (argb_t& color);
	FArchive& operator>> (std::string& s);
	FArchive& ReadObject(DObject *&obj, TypeInfo* wanttype);

	//FArchive& operator>> (unsigned char *&str) { return operator>> ((char *&)str); }
	//FArchive& operator>> (signed char *&str) { return operator>> ((char *&)str); }
	FArchive& operator>> (DObject* &object) { return ReadObject (object, RUNTIME_CLASS(DObject)); }

protected:
	enum { EObjectHashSize = 137 };

	uint32_t FindObjectIndex(const DObject* obj) const;
	uint32_t MapObject(const DObject* obj);
	uint32_t WriteClass(const TypeInfo* info);
	const TypeInfo* ReadClass();
	const TypeInfo* ReadClass(const TypeInfo* wanttype);
	const TypeInfo* ReadStoredClass(const TypeInfo* wanttype);
	uint32_t HashObject(const DObject* obj) const;

	bool m_Persistent;		// meant for persistent storage (disk)?
	bool m_Loading;			// extracting objects?
	bool m_Storing;			// inserting objects?
	bool m_HubTravel;		// travelling inside a hub?
	bool m_Reset;			// reset state?
	FFile* m_File;			// unerlying file object
	uint32_t m_ObjectCount;	// # of objects currently serialized
	uint32_t m_MaxObjectCount;
	uint32_t m_ClassCount;		// # of unique classes currently serialized

	struct TypeMap
	{
		const TypeInfo* toCurrent;	// maps archive type index to execution type index
		uint32_t toArchive;		// maps execution type index to archive type index

//		enum { NO_INDEX = 0xffffffff };
	} *m_TypeMap;

	struct ObjectMap
	{
		const DObject* object;
		size_t hashNext;
	} *m_ObjectMap;
	size_t m_ObjectHash[EObjectHashSize];

private:
	FArchive(const FArchive &src) {}
	void operator= (const FArchive &src) {}
};

class player_t;

FArchive &operator<< (FArchive& arc, player_t* p);
FArchive &operator>> (FArchive& arc, player_t* &p);
