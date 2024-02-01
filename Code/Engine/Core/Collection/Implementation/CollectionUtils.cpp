#include <Core/CorePCH.h>

#include <Core/Collection/CollectionUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

void plCollectionUtils::AddFiles(plCollectionResourceDescriptor& ref_collection, plStringView sAssetTypeNameView, plStringView sAbsPathToFolder, plStringView sFileExtension, plStringView sStripPrefix, plStringView sPrependPrefix)
{
#if PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS)

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

      auto& entry = ref_collection.m_Resources.ExpandAndGetRef();
      entry.m_sAssetTypeName = sAssetTypeName;
      entry.m_sResourceID = sFullPath;
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }

#else
  PL_ASSERT_NOT_IMPLEMENTED;
#endif
}


PL_CORE_DLL void plCollectionUtils::MergeCollections(plCollectionResourceDescriptor& ref_result, plArrayPtr<const plCollectionResourceDescriptor*> inputCollections)
{
  plMap<plString, const plCollectionEntry*> firstEntryOfID;

  for (const plCollectionResourceDescriptor* inputDesc : inputCollections)
  {
    for (const plCollectionEntry& inputEntry : inputDesc->m_Resources)
    {
      if (!firstEntryOfID.Contains(inputEntry.m_sResourceID))
      {
        firstEntryOfID.Insert(inputEntry.m_sResourceID, &inputEntry);
        ref_result.m_Resources.PushBack(inputEntry);
      }
    }
  }
}


PL_CORE_DLL void plCollectionUtils::DeDuplicateEntries(plCollectionResourceDescriptor& ref_result, const plCollectionResourceDescriptor& input)
{
  const plCollectionResourceDescriptor* firstInput = &input;
  MergeCollections(ref_result, plArrayPtr<const plCollectionResourceDescriptor*>(&firstInput, 1));
}

void plCollectionUtils::AddResourceHandle(plCollectionResourceDescriptor& ref_collection, plTypelessResourceHandle hHandle, plStringView sAssetTypeName, plStringView sAbsFolderpath)
{
  if (!hHandle.IsValid())
    return;

  const char* resID = hHandle.GetResourceID();

  auto& entry = ref_collection.m_Resources.ExpandAndGetRef();

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


