// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// Mapping of resource paths to resource IDs
//
//-----------------------------------------------------------------------------

#pragma once

#include <vector>

#include "resources/res_resourceid.h"
#include "resources/res_resourcepath.h"


enum ResourceNamespace
{
	NS_HIDDEN = -1,
	NS_GLOBAL = 0,
	NS_SPRITES,
	NS_FLATS,
	NS_COLORMAPS,
	NS_ACSLIBRARY,
	NS_NEWTEXTURES,
	NS_HIRES,
	NS_VOXELS,

	// These namespaces are only used to mark lumps in special subdirectories
	// so that their contents doesn't interfere with the global namespace.
	// searching for data in these namespaces works differently for lumps coming
	// from Zips or other files.
	NS_SPECIALZIPDIRECTORY,
	NS_SOUNDS,
	NS_PATCHES,
	NS_GRAPHICS,
	NS_MUSIC,
};


struct NamespacedResourceName
{
	OString			name;
	ResourceNamespace	ns;

	bool operator!=(const NamespacedResourceName& other) const
	{	return name != other.name || ns != other.ns;	}
};


// ----------------------------------------------------------------------------
// hash function for OHashTable class
// ----------------------------------------------------------------------------

template <> struct hashfunc<NamespacedResourceName>
{
	unsigned int operator()(const NamespacedResourceName& namespaced_resource_name) const
	{
		hashfunc<int> ns_hashfunc;
		hashfunc<OString> name_hashfunc;
		return (ns_hashfunc(1 + namespaced_resource_name.ns) << 26) ^ name_hashfunc(namespaced_resource_name.name);
	}
};


// ============================================================================
//
// ResourceNameTranslator
//
// Provides translation from ResourcePaths to ResourceIds
//
// ============================================================================

class ResourceNameTranslator
{
public:
	ResourceNameTranslator();
	virtual ~ResourceNameTranslator() { }

	virtual void clear()
	{
		mResourceIdLookup.clear();
		mNamespacedResourceIdLookup.clear();
		mResourceIdByPathLookup.clear();
	}

	virtual void addTranslation(const ResourcePath& path, const ResourceId res_id);

	virtual const ResourceId translate(const ResourcePath& path) const;

	virtual const ResourceId translate(const OString& resource_name, ResourceNamespace ns, bool exact_ns_match = false) const;

	virtual const ResourceIdList getAllTranslations(const ResourcePath& path) const;

	virtual bool checkNameVisibility(const ResourcePath& path, const ResourceId res_id) const;

private:
	typedef OHashTable<OString, ResourceId> ResourceIdLookupTable;
	ResourceIdLookupTable	mResourceIdLookup;

	typedef OHashTable<NamespacedResourceName, ResourceId> NamespacedResourceIdLookupTable;
	NamespacedResourceIdLookupTable	mNamespacedResourceIdLookup;

	typedef OHashTable<ResourcePath, ResourceIdList> ResourceIdByPathLookupTable;
	ResourceIdByPathLookupTable		mResourceIdByPathLookup;
};
