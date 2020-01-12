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

#ifndef __RES_NAMETRANSLATOR_H__
#define __RES_NAMETRANSLATOR_H__

#include <vector>

#include "resources/res_resourceid.h"
#include "resources/res_resourcepath.h"

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
	}

	virtual void addTranslation(const ResourcePath& path, const ResourceId res_id);

	virtual const ResourceId translate(const ResourcePath& path) const;

	virtual const ResourceIdList getAllTranslations(const ResourcePath& path) const;

	virtual bool checkNameVisibility(const ResourcePath& path, const ResourceId res_id) const;

private:
	typedef OHashTable<ResourcePath, ResourceIdList> ResourceIdLookupTable;
	ResourceIdLookupTable			mResourceIdLookup;
};

#endif	// __RES_NAMETRANSLATOR_H__