#ifndef __RES_NAMETRANSLATOR_H__
#define __RES_NAMETRANSLATOR_H__

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

	virtual void addTranslation(const ResourcePath& path, const ResourceId res_id);

	virtual const ResourceId translate(const ResourcePath& path) const;

	virtual const ResourceIdList getAllTranslations(const ResourcePath& path) const;

	virtual bool checkNameVisibility(const ResourcePath& path, const ResourceId res_id) const;

private:
	typedef OHashTable<ResourcePath, ResourceIdList> ResourceIdLookupTable;
	ResourceIdLookupTable			mResourceIdLookup;
};


// ============================================================================
//
// TextureResourceNameTranslator
//
// Provides translation from ResourcePaths to ResourceIds for texture resources.
//
// ============================================================================

class TextureResourceNameTranslator : public ResourceNameTranslator
{
public:
	TextureResourceNameTranslator();
	virtual ~TextureResourceNameTranslator() { }

	virtual const ResourceId translate(const ResourcePath& path) const;

private:
	typedef std::vector<ResourcePath> ResourcePathList;
	static ResourcePathList mAlternativeDirectories;
};

#endif	// __RES_NAMETRANSLATOR_H__

