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

#include "dobject.h"
#include "doomtype.h"

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

	FArchive& operator<< (uint8_t c);
	FArchive& operator<< (uint16_t s);
	FArchive& operator<< (uint32_t i);
	FArchive& operator<< (uint64_t i);
	FArchive& operator<< (float f);
	FArchive& operator<< (double d);
	FArchive& operator<< (argb_t color);
	FArchive& operator<< (const char* str);
	FArchive& operator<< (DObject* obj);

	inline	FArchive& operator<< (char c) { return operator<< (static_cast<uint8_t>(c)); }
	inline	FArchive& operator<< (int8_t c) { return operator<< (static_cast<uint8_t>(c)); }
	inline	FArchive& operator<< (int16_t s) { return operator<< (static_cast<uint16_t>(s)); }
	inline	FArchive& operator<< (int32_t i) { return operator<< (static_cast<uint32_t>(i)); }
	inline	FArchive& operator<< (int64_t i) { return operator<< (static_cast<uint64_t>(i)); }
	inline	FArchive& operator<< (const unsigned char* str) { return operator<< (reinterpret_cast<const char*>(str)); }
	inline	FArchive& operator<< (const signed char* str) { return operator<< (reinterpret_cast<const char*>(str)); }
	inline	FArchive& operator<< (bool b) { return operator<< (static_cast<uint8_t>(b)); }

	FArchive& operator>> (uint8_t& c);
	FArchive& operator>> (uint16_t& s);
	FArchive& operator>> (uint32_t& i);
	FArchive& operator>> (uint64_t& i);
	FArchive& operator>> (float& f);
	FArchive& operator>> (double& d);
	FArchive& operator>> (argb_t& color);
	FArchive& operator>> (std::string& s);
	FArchive& ReadObject(DObject *&obj, TypeInfo* wanttype);

	inline	FArchive& operator>> (char& c) { uint8_t in; operator>> (in); c = static_cast<char>(in); return *this; }
	inline	FArchive& operator>> (int8_t& c) { uint8_t in; operator>> (in); c = static_cast<int8_t>(in); return *this; }
	inline	FArchive& operator>> (int16_t& s) { uint16_t in; operator>> (in); s = static_cast<int16_t>(in); return *this; }
	inline	FArchive& operator>> (int32_t& i) { uint32_t in; operator>> (in); i = static_cast<int32_t>(in); return *this; }
	inline	FArchive& operator>> (int64_t& i) { uint64_t in; operator>> (in); i = static_cast<int64_t>(in); return *this; }
	//inline	FArchive& operator>> (unsigned char *&str) { return operator>> ((char *&)str); }
	//inline	FArchive& operator>> (signed char *&str) { return operator>> ((char *&)str); }
	inline	FArchive& operator>> (bool& b) { uint8_t in; operator>> (in); b = (in != 0); return *this; }
	inline  FArchive& operator>> (DObject* &object) { return ReadObject (object, RUNTIME_CLASS(DObject)); }

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
