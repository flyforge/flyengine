#include <Core/CorePCH.h>

#include <Core/Collection/CollectionUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

void plCollectionUtils::AddFiles(plCollectionResourceDescriptor& collection, plStringView sAssetTypeNameView, plStringView sAbsPathToFolder, plStringView sFileExtension, plStringView sStripPrefix, plStringView sPrependPrefix)
{
#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS)

  const plUInt32 uiStripPrefixLength = plStringUtils::GetCharacterCount(sStripPrefix.GetStartPointer(), sStripPrefix.GetEndPointer());

  plFileSystemIterator fsIt;
  fsIt.StartSearch(sAbsPathToFolder, plFileSystemIteratorFlags::ReportFilesRecursive);

  if (!fsIt.IsValid())
    return;

  plStringBuilder sFullPath;
  plHashedString sAssetTypeName;
  sAssetTypeName.Assign(sAssetTypeNameView);

  for (; fsIt.IsValid(); fsIt.Next())
  {
    const auto& stats = fsIt.GetStats();

    if (plPathUtils::HasExtension(stats.m_sName, sFileExtension))
    {
      stats.GetFullPath(sFullPath);

      sFullPath.Shrink(uiStripPrefixLength, 0);
      sFullPath.Prepend(sPrependPrefix);
      sFullPath.MakeCleanPath();

      auto& entry = collection.m_Resources.ExpandAndGetRef();
      entry.m_sAssetTypeName = sAssetTypeName;
      entry.m_sResourceID = sFullPath;
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }

#else
  PLASMA_ASSERT_NOT_IMPLEMENTED;
#endif
}


PLASMA_CORE_DLL void plCollectionUtils::MergeCollections(plCollectionResourceDescriptor& result, plArrayPtr<const plCollectionResourceDescriptor*> inputCollections)
{
  plMap<plString, const plCollectionEntry*> firstEntryOfID;

  for (const plCollectionResourceDescriptor* inputDesc : inputCollections)
  {
    for (const plCollectionEntry& inputEntry : inputDesc->m_Resources)
    {
      if (!firstEntryOfID.Contains(inputEntry.m_sResourceID))
      {
        firstEntryOfID.Insert(inputEntry.m_sResourceID, &inputEntry);
        result.m_Resources.PushBack(inputEntry);
      }
    }
  }
}


PLASMA_CORE_DLL void plCollectionUtils::DeDuplicateEntries(plCollectionResourceDescriptor& result, const plCollectionResourceDescriptor& input)
{
  const plCollectionResourceDescriptor* firstInput = &input;
  MergeCollections(result, plArrayPtr<const plCollectionResourceDescriptor*>(&firstInput, 1));
}

void plCollectionUtils::AddResourceHandle(plCollectionResourceDescriptor& collection, plTypelessResourceHandle handle, plStringView sAssetTypeName, plStringView sAbsFolderpath)
{
  if (!handle.IsValid())
    return;

  const char* resID = handle.GetResourceID();

  auto& entry = collection.m_Resources.ExpandAndGetRef();

  entry.m_sAssetTypeName.Assign(sAssetTypeName);
  entry.m_sResourceID = resID;

  plStringBuilder absFilename;

  // if a folder path is specified, replace the root (for testing filesize below)
  if (!sAbsFolderpath.IsEmpty())
  {
    plStringView root, relFile;
    plPathUtils::GetRootedPathParts(resID, root, relFile);
    absFilename = sAbsFolderpath;
    absFilename.AppendPath(relFile.GetStartPointer());
    absFilename.MakeCleanPath();

    plFileStats stats;
    if (!absFilename.IsEmpty() && absFilename.IsAbsolutePath() && plFileSystem::GetFileStats(absFilename, stats).Succeeded())
    {
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }
}

PLASMA_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionUtils);
