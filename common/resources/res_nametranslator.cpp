#include <assert.h>
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
	mResourceIdLookup(default_initial_resource_count)
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
	ResourceIdLookupTable::const_iterator it = mResourceIdLookup.find(path);
	if (it != mResourceIdLookup.end())
	{
		const ResourceIdList& res_id_list = it->second;
		assert(!res_id_list.empty());
		const ResourceId res_id = res_id_list.back();
		// assert(validateResourceId(res_id));
		return res_id;
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
	ResourceIdLookupTable::const_iterator it = mResourceIdLookup.find(path);
	if (it != mResourceIdLookup.end())
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
	ResourceIdLookupTable::iterator it = mResourceIdLookup.find(path);
	if (it == mResourceIdLookup.end())
	{
		// No other resources with the same path exist yet. Create a new list
		// for ResourceIds with a matching path.
		std::pair<ResourceIdLookupTable::iterator, bool> result =
				mResourceIdLookup.insert(std::make_pair(path, ResourceIdList()));
		it = result.first;
	}

	ResourceIdList& res_id_list = it->second;
	res_id_list.push_back(res_id);

	assert(translate(path) == res_id);
}


