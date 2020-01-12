// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
//
// Loads a resource from a ResourceContainer, performing any necessary data
// conversion and caching the resulting instance in the Zone memory system.
//
//-----------------------------------------------------------------------------

#ifndef __RES_RESOURCELOADER_H__
#define __RES_RESOURCELOADER_H__

#include "doomtype.h"
#include "resources/res_resourceid.h"
#include "resources/res_texture.h"

class RawResourceAccessor;


// ============================================================================
//
// ResourceLoader class interface
//
// ============================================================================
//

// ----------------------------------------------------------------------------
// ResourceLoader abstract base class interface
// ----------------------------------------------------------------------------

class ResourceLoader
{
public:
	ResourceLoader(const RawResourceAccessor* accessor) :
		mRawResourceAccessor(accessor)
	{}

	virtual ~ResourceLoader() {}

	virtual bool validate() const
	{
		return true;
	}

	virtual uint32_t size() const = 0;
	virtual void load(void* data) const = 0;

protected:
	const RawResourceAccessor*		mRawResourceAccessor;
};


// ---------------------------------------------------------------------------
// GenericResourceLoader class interface
//
// Generic resource loading functionality. Simply reads raw data and returns
// a pointer to the cached data.
// ---------------------------------------------------------------------------

class GenericResourceLoader : public ResourceLoader
{
public:
	GenericResourceLoader(const RawResourceAccessor* accessor, const ResourceId res_id) :
		ResourceLoader(accessor),
		mResId(res_id)
	{}

	virtual ~GenericResourceLoader() {} 
	virtual uint32_t size() const;
	virtual void load(void* data) const;

protected:
	const ResourceId			mResId;
};



class BaseTextureLoader : public ResourceLoader
{
public:
	BaseTextureLoader(const RawResourceAccessor* accessor) :
		ResourceLoader(accessor)
	{}

	virtual ~BaseTextureLoader() {}
	
protected:
	uint32_t calculateTextureSize(uint16_t width, uint16_t height) const;
	Texture* createTexture(void* data, uint16_t width, uint16_t height) const;
};


//
// RowMajorTextureLoader
//
// Abstract base class for loading an image from a headerless row-major
// image resource and converting it to a Texture.
//
class RowMajorTextureLoader : public BaseTextureLoader
{
public:
	RowMajorTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id) :
		BaseTextureLoader(accessor),
		mResId(res_id)
	{}

	virtual ~RowMajorTextureLoader() {}
	virtual uint32_t size() const;
	virtual void load(void* data) const;

protected:
	virtual uint16_t getWidth() const = 0;
	virtual uint16_t getHeight() const = 0;

	const ResourceId	mResId;
};


//
// FlatTextureLoader
//
// Loads a Flat image resource and converts it to a Texture.
// Flat image resources are expected to be square in dimensions and each
// dimension should be a power-of-two. Certain exceptions to this are
// also handled.
//
class FlatTextureLoader : public RowMajorTextureLoader
{
public:
	FlatTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id) :
		RowMajorTextureLoader(accessor, res_id)
	{}

	virtual ~FlatTextureLoader() {}

protected:
	virtual uint16_t getWidth() const;
	virtual uint16_t getHeight() const;
};


//
// RawTextureLoader
//
// Loads a raw full-screen (320x200) image resource and converts it to a Texture.
//
class RawTextureLoader : public RowMajorTextureLoader
{
public:
	RawTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id) :
		RowMajorTextureLoader(accessor, res_id)
	{}

	virtual ~RawTextureLoader() {}

protected:
	virtual uint16_t getWidth() const;
	virtual uint16_t getHeight() const;
};


//
// PatchTextureLoader
//
class PatchTextureLoader : public BaseTextureLoader
{
public:
	PatchTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id) :
		BaseTextureLoader(accessor),
		mResId(res_id)
	{}

	virtual ~PatchTextureLoader() {}
	virtual uint32_t size() const;
	virtual void load(void* data) const;

protected:
	const ResourceId	mResId;
};


//
// CompositeTextureLoader
//
//
class CompositeTextureLoader : public BaseTextureLoader
{
public:
	CompositeTextureLoader(const RawResourceAccessor* accessor, const CompositeTextureDefinition& texdef) :
		BaseTextureLoader(accessor),
		mTexDef(texdef)
	{}

	virtual ~CompositeTextureLoader() {}

	virtual uint32_t size() const;
	virtual void load(void* data) const;

protected:
	const CompositeTextureDefinition	mTexDef;
};


#endif	// __RES_RESOURCELOADER_H__