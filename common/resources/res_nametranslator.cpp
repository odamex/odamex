#include "resources/res_nametranslator.h"

// Default directory names for ZDoom zipped resource files.
// See: http://zdoom.org/wiki/Using_ZIPs_as_WAD_replacement
//
static const ResourcePath global_directory_name("/GLOBAL/");
static const ResourcePath patches_directory_name("/PATCHES/");
static const ResourcePath graphics_directory_name("/GRAPHICS/");
static const ResourcePath sounds_directory_name("/SOUNDS/");
static const ResourcePath music_directory_name("/MUSIC/");
static const ResourcePath maps_directory_name("/MAPS/");
static const ResourcePath flats_directory_name("/FLATS/");
static const ResourcePath sprites_directory_name("/SPRITES/");
static const ResourcePath textures_directory_name("/TEXTURES/");
static const ResourcePath hires_directory_name("/HIRES/");
static const ResourcePath colormaps_directory_name("/COLORMAPS/");
static const ResourcePath acs_directory_name("/ACS/");
static const ResourcePath voices_directory_name("/VOICES/");
static const ResourcePath voxels_directory_name("/VOXELS/");

static uint32_t default_initial_resource_count = 2048;

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


// ============================================================================
//
// TextureResourceNameTranslator class implementation
//
// ============================================================================

typedef std::vector<ResourcePath> ResourcePathList;
ResourcePathList TextureResourceNameTranslator::mAlternativeDirectories;

//
// TextureResourceNameTranslator::TextureResourceNameTranslator
//
TextureResourceNameTranslator::TextureResourceNameTranslator() :
	ResourceNameTranslator()
{
	if (mAlternativeDirectories.empty())
	{
		mAlternativeDirectories.push_back(textures_directory_name);
		mAlternativeDirectories.push_back(flats_directory_name);
		mAlternativeDirectories.push_back(patches_directory_name);
		mAlternativeDirectories.push_back(sprites_directory_name);
		mAlternativeDirectories.push_back(graphics_directory_name);
	}
}


//
// TextureResourceNameTranslator::translate
//
// Retrieves the ResourceId for a given resource path name using a set of
// rules for retrieving ResourceIds when there is not an exact path match.
// Alternative directories are searched for resources with a matching name
// if an exact path match is not found.
//
const ResourceId TextureResourceNameTranslator::translate(const ResourcePath& path) const
{
	const ResourceId res_id = ResourceNameTranslator::translate(path);
	if (res_id != ResourceId::INVALID_ID)
		return res_id;

	// TODO: get the path portion after the initial directory name
	const ResourcePath base_path(path.last());

	for (ResourcePathList::const_iterator it = mAlternativeDirectories.begin(); it != mAlternativeDirectories.end(); ++it)
	{
		const ResourcePath alternative_path = *it + base_path;
		const ResourceId res_id = ResourceNameTranslator::translate(alternative_path);
		if (res_id != ResourceId::INVALID_ID)
			return res_id;
	}

	return ResourceId::INVALID_ID;
}


