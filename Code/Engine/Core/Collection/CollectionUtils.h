#pragma once

#include <Core/Collection/CollectionResource.h>

class plHashedString;

namespace plCollectionUtils
{
  /// \brief Adds all files from \a szAbsPathToFolder and \a szFileExtension to \a collection
  ///
  /// The files are added as new entries using szAssetTypeName as the resource type identifier (see plResourceManager::RegisterResourceForAssetType).
  /// \a szStripPrefix is stripped from the file system paths and \a szPrependPrefix is prepended.
  PL_CORE_DLL void AddFiles(plCollectionResourceDescriptor& ref_collection, plStringView sAssetTypeName, plStringView sAbsPathToFolder,
    plStringView sFileExtension, plStringView sStripPrefix, plStringView sPrependPrefix);

  /// \brief Merges all collections from the input array into the target result collection. Resource entries will be de-duplicated by resource ID
  /// string.
  PL_CORE_DLL void MergeCollections(plCollectionResourceDescriptor& ref_result, plArrayPtr<const plCollectionResourceDescriptor*> inputCollections);

  /// \brief Special case of plCollectionUtils::MergeCollections which outputs unique entries from input collection into the result collection
  PL_CORE_DLL void DeDuplicateEntries(plCollectionResourceDescriptor& ref_result, const plCollectionResourceDescriptor& input);

  /// \brief Extracts info (i.e. resource ID as file path) from the passed handle and adds it as a new resource entry. Does not add an entry if the
  /// resource handle is not valid.
  ///
  /// The resource type identifier must be passed explicity as szAssetTypeName (see plResourceManager::RegisterResourceForAssetType). To determine the
  /// file size, the resource ID is used as a filename passed to plFileSystem::GetFileStats. In case the resource's path root is not mounted, the path
  /// root can be replaced by passing non-NULL string to szAbsFolderpath, which will replace the root, e.g. with an absolute file path. This is just
  /// for the file size check within the scope of the function, it will not modify the resource Id.
  PL_CORE_DLL void AddResourceHandle(plCollectionResourceDescriptor& ref_collection, plTypelessResourceHandle hHandle, plStringView sAssetTypeName, plStringView sAbsFolderpath);

}; // namespace plCollectionUtils
