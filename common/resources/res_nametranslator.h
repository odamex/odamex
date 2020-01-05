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

