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

#include "odamex.h"

#include <assert.h>
#include "resources/res_main.h"
#include "resources/res_nametranslator.h"

static const size_t default_initial_resource_count = 2048;

// ============================================================================
//
// ResourceNameTranslator class implementation
//
// ============================================================================

//
// ResourceNameTranslator::ResourceNameTranslator
//
ResourceNameTranslator::ResourceNameTranslator() :
	mNamespacedResourceIdLookup(default_initial_resource_count),
	mResourceIdLookup(default_initial_resource_count),
	mResourceIdByPathLookup(default_initial_resource_count)
{ }


//
// ResourceNameTranslator::translate
//
// Retrieves the ResourceId for a given resource path name. If more than
// one ResourceId match the given path name, the ResouceId from the most
// recently loaded resource file will be returned.
//
const ResourceId ResourceNameTranslator::translate(const ResourcePath& path) const
{
	ResourceIdByPathLookupTable::const_iterator it = mResourceIdByPathLookup.find(path);
	if (it != mResourceIdByPathLookup.end())
	{
		const ResourceIdList& res_id_list = it->second;
		assert(!res_id_list.empty());
		const ResourceId res_id = res_id_list.back();
		assert(res_id != ResourceId::INVALID_ID);
		return res_id;
	}
	
	return ResourceId::INVALID_ID;
}


//
// ResourceNameTranslator::translate
//
// Retrieves the ResourceId for a given resource name and namespace. If more than
// one ResourceId match the given path name, the ResouceId from the most
// recently loaded resource file will be returned.
//
const ResourceId ResourceNameTranslator::translate(const OString& resource_name, ResourceNamespace ns, bool exact_ns_match) const
{
	// Check for an exact match for the resource name and namespace
	const NamespacedResourceName namespaced_resource_name = {resource_name, ns};
	NamespacedResourceIdLookupTable::const_iterator it_ns = mNamespacedResourceIdLookup.find(namespaced_resource_name);
	if (it_ns != mNamespacedResourceIdLookup.end())
	{
		const ResourceId res_id = it_ns->second;
		assert(res_id != ResourceId::INVALID_ID);
		return res_id;
	}

	if (!exact_ns_match)
	{
		// Fallback to the most recently added match for the resource name
		ResourceIdLookupTable::const_iterator it = mResourceIdLookup.find(resource_name);
		if (it != mResourceIdLookup.end())
		{
			const ResourceId res_id = it->second;
			assert(res_id != ResourceId::INVALID_ID);
			return res_id;
		}
	}
	
	return ResourceId::INVALID_ID;
}


//
// ResourceNameTranslator::getAllTranslations
//
// Returns a std::vector of ResourceIds that match a given resource path name.
// If there are no matches, an empty std::vector will be returned.
//
const ResourceIdList ResourceNameTranslator::getAllTranslations(const ResourcePath& path) const
{
	ResourceIdByPathLookupTable::const_iterator it = mResourceIdByPathLookup.find(path);
	if (it != mResourceIdByPathLookup.end())
	{
		const ResourceIdList& res_id_list = it->second;
		assert(!res_id_list.empty());
		return res_id_list;
	}
	return ResourceIdList();
}


//
// ResourceNameTranslator::checkNameVisibility
//
// Determine if no other resources have overridden this resource by having
// the same resource path.
//
bool ResourceNameTranslator::checkNameVisibility(const ResourcePath& path, const ResourceId res_id) const
{
	return translate(path) == res_id;
}


//
// ResourceNameTranslator::addTranslation
//
// Add the ResourceId to the ResourceIdLookupTable
// ResourceIdLookupTable uses a ResourcePath key and value that is a std::vector
// of ResourceIds.
//
void ResourceNameTranslator::addTranslation(const ResourcePath& path, const ResourceId res_id)
{
	ResourceIdByPathLookupTable::iterator it = mResourceIdByPathLookup.find(path);
	if (it == mResourceIdByPathLookup.end())
	{
		// No other resources with the same path exist yet. Create a new list
		// for ResourceIds with a matching path.
		std::pair<ResourceIdByPathLookupTable::iterator, bool> result = mResourceIdByPathLookup.insert(std::make_pair(path, ResourceIdList()));
		it = result.first;
	}

	ResourceIdList& res_id_list = it->second;
	res_id_list.push_back(res_id);

	assert(translate(path) == res_id);

	if (path.size() > 1)
	{
		OString resource_name = path.last();
		OString base_directory = path.first();

		ResourceNamespace ns = NS_GLOBAL;
		if (base_directory == OString("PATCHES"))
			ns = NS_PATCHES;
		else if (base_directory == OString("GRAPHICS"))
			ns = NS_GRAPHICS;
		else if (base_directory == OString("SOUNDS"))
			ns = NS_SOUNDS;
		else if (base_directory == OString("MUSIC"))
			ns = NS_MUSIC;
		else if (base_directory == OString("MAPS"))
			ns = NS_GLOBAL;			// TODO: Cross-check this behavior with ZDoom
		else if (base_directory == OString("FLATS"))
			ns = NS_FLATS;
		else if (base_directory == OString("SPRITES"))
			ns = NS_SPRITES;
		else if (base_directory == OString("TEXTURES"))
			ns = NS_NEWTEXTURES;
		else if (base_directory == OString("HIRES"))
			ns = NS_HIRES;
		else if (base_directory == OString("COLORMAPS"))
			ns = NS_COLORMAPS;
		else if (base_directory == OString("ACS"))
			ns = NS_ACSLIBRARY;
		else if (base_directory == OString("VOXELS"))
			ns = NS_VOXELS;
		// TODO: add scripts directory

		NamespacedResourceName key = {resource_name, ns};
		mNamespacedResourceIdLookup.insert(std::make_pair(key, res_id));

		mResourceIdLookup.insert(std::make_pair(resource_name, res_id));
	}
}