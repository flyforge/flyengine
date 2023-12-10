#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>


plQtAssetBrowserFilter::plQtAssetBrowserFilter(QObject* pParent)
  : plQtAssetFilter(pParent)
{
}


void plQtAssetBrowserFilter::Reset()
{
  SetShowItemsInSubFolders(true);
  SetShowFiles(true);
  SetShowNonImportableFiles(true);
  SetShowItemsInHiddenFolders(false);
  SetSortByRecentUse(false);
  SetTextFilter("");
  SetTypeFilter("");
  SetPathFilter("");
}


void plQtAssetBrowserFilter::UpdateImportExtensions(const plSet<plString>& extensions)
{
  m_ImportExtensions = extensions;
  if (!m_bShowNonImportableFiles)
    Q_EMIT FilterChanged();
}

void plQtAssetBrowserFilter::SetShowItemsInSubFolders(bool bShow)
{
  if (m_bShowItemsInSubFolders == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowItemsInSubFolders = bShow;

  Q_EMIT FilterChanged();
}


void plQtAssetBrowserFilter::SetShowFiles(bool bShow)
{
  if (m_bShowFiles == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowFiles = bShow;

  Q_EMIT FilterChanged();
}

void plQtAssetBrowserFilter::SetShowNonImportableFiles(bool bShow)
{
  if (m_bShowNonImportableFiles == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowNonImportableFiles = bShow;

  Q_EMIT FilterChanged();
}

void plQtAssetBrowserFilter::SetShowItemsInHiddenFolders(bool bShow)
{
  if (m_bShowItemsInHiddenFolders == bShow)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_bShowItemsInHiddenFolders = bShow;

  Q_EMIT FilterChanged();
}

void plQtAssetBrowserFilter::SetSortByRecentUse(bool bSort)
{
  if (m_bSortByRecentUse == bSort)
    return;

  m_bSortByRecentUse = bSort;

  Q_EMIT FilterChanged();
  Q_EMIT SortByRecentUseChanged();
}


void plQtAssetBrowserFilter::SetTextFilter(const char* szText)
{
  plStringBuilder sCleanText = szText;
  sCleanText.MakeCleanPath();

  if (m_SearchFilter.GetSearchText() == sCleanText)
    return;

  m_SearchFilter.SetSearchText(sCleanText);
  // Clear uses search cache
  m_bUsesSearchActive = false;
  m_bTransitive = false;
  m_Uses.Clear();

  const char* szRefGuid = plStringUtils::FindSubString_NoCase(szText, "ref:");
  const char* szRefAllGuid = plStringUtils::FindSubString_NoCase(szText, "ref-all:");
  if (szRefGuid || szRefAllGuid)
  {
    bool bTransitive = szRefAllGuid != nullptr;
    const char* szGuid = szRefAllGuid ? szRefAllGuid + strlen("ref-all:") : szRefGuid + strlen("ref:");
    if (plConversionUtils::IsStringUuid(szGuid))
    {
      m_bUsesSearchActive = true;
      m_bTransitive = bTransitive;
      plUuid guid = plConversionUtils::ConvertStringToUuid(szGuid);
      plAssetCurator::GetSingleton()->FindAllUses(guid, m_Uses, m_bTransitive);
    }
  }

  Q_EMIT FilterChanged();
  Q_EMIT TextFilterChanged();
}

void plQtAssetBrowserFilter::SetPathFilter(const char* szPath)
{
  plStringBuilder sCleanText = szPath;
  sCleanText.MakeCleanPath();
  // The assumption is that only full directory names are set as path filters. Thus, we can ensure they end with a / to make it easier to filter items inside the path.
  if (!sCleanText.IsEmpty() && !sCleanText.EndsWith_NoCase("/"))
  {
    sCleanText.Append("/");
  }

  if (m_sPathFilter == sCleanText)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_sPathFilter = sCleanText;

  Q_EMIT FilterChanged();
  Q_EMIT PathFilterChanged();
}


plStringView plQtAssetBrowserFilter::GetPathFilter() const
{
  if (m_sPathFilter.EndsWith_NoCase("/"))
  {
    return m_sPathFilter.GetSubString(0, m_sPathFilter.GetCharacterCount() - 1);
  }
  return m_sPathFilter;
}

void plQtAssetBrowserFilter::SetTypeFilter(const char* szTypes)
{
  if (m_sTypeFilter == szTypes)
    return;

  m_sTemporaryPinnedItem.Clear();
  m_sTypeFilter = szTypes;

  Q_EMIT FilterChanged();
  Q_EMIT TypeFilterChanged();
}

void plQtAssetBrowserFilter::SetFileExtensionFilters(plStringView sExtensions)
{
  m_FileExtensions.Clear();

  plHybridArray<plStringView, 8> filters;
  sExtensions.Split(false, filters, ";", "*", ".");

  plStringBuilder tmp;
  for (plStringView filter : filters)
  {
    tmp = filter;
    tmp.ToLower();
    m_FileExtensions.Insert(tmp);
  }
}

void plQtAssetBrowserFilter::SetTemporaryPinnedItem(plStringView sDataDirParentRelativePath)
{
  if (m_sTemporaryPinnedItem == sDataDirParentRelativePath)
    return;

  m_sTemporaryPinnedItem = sDataDirParentRelativePath;
  Q_EMIT FilterChanged();
}

bool plQtAssetBrowserFilter::IsAssetFiltered(plStringView sDataDirParentRelativePath, bool bIsFolder, const plSubAsset* pInfo) const
{
  // ignore all paths leading into the AssetCache
  if (sDataDirParentRelativePath.FindSubString("/AssetCache/"))
    return true;

  // also ignore the AssetCache folder directly
  if (bIsFolder && sDataDirParentRelativePath.GetFileNameAndExtension() == "AssetCache")
    return true;

  if (sDataDirParentRelativePath == m_sTemporaryPinnedItem)
    return false;

  if (!m_bShowFiles && !pInfo && !bIsFolder)
    return true;

  plStringBuilder sExt;
  if (m_bShowFiles && !m_bShowNonImportableFiles && !pInfo && !bIsFolder)
  {
    sExt = sDataDirParentRelativePath.GetFileExtension();
    sExt.ToLower();
    if (!m_ImportExtensions.Contains(sExt))
      return true;
  }

  if (!m_sPathFilter.IsEmpty() || bIsFolder)
  {
    // if the string is not found in the path, ignore this asset
    if (!sDataDirParentRelativePath.StartsWith(m_sPathFilter))
      return true;

    if (!m_bShowItemsInSubFolders || bIsFolder)
    {
      // do we find another path separator after the prefix path?
      // if so, there is a sub-folder, and thus we ignore it
      if (plStringUtils::FindSubString(sDataDirParentRelativePath.GetStartPointer() + m_sPathFilter.GetElementCount(), "/", sDataDirParentRelativePath.GetEndPointer()) != nullptr)
        return true;
    }
  }

  if (!m_bShowItemsInHiddenFolders && !(m_bUsesSearchActive && !m_SearchFilter.IsEmpty()))
  {
    if (plStringUtils::FindSubString_NoCase(sDataDirParentRelativePath.GetStartPointer() + m_sPathFilter.GetElementCount() + 1, "_data/", sDataDirParentRelativePath.GetEndPointer()) !=
        nullptr)
      return true;
  }

  if (!m_SearchFilter.IsEmpty())
  {
    if (m_bUsesSearchActive)
    {
      if (pInfo == nullptr)
        return true;

      if (!m_Uses.Contains(pInfo->m_Data.m_Guid))
        return true;
    }
    else
    {
      // if the string is not found in the path, ignore this asset
      if (m_SearchFilter.PassesFilters(sDataDirParentRelativePath) == false)
      {
        if (pInfo && m_SearchFilter.PassesFilters(pInfo->GetName()) == false)
        {
          plConversionUtils::ToString(pInfo->m_Data.m_Guid, m_sTemp);
          if (m_SearchFilter.PassesFilters(m_sTemp) == false)
            return true;

          // we could actually (partially) match the GUID
        }
        else
          return true;
      }
    }
  }

  // Always show folders
  if (bIsFolder)
    return false;

  if (!m_FileExtensions.IsEmpty())
  {
    sExt = sDataDirParentRelativePath.GetFileExtension();
    sExt.ToLower();

    return !m_FileExtensions.Contains(sExt);
  }

  if (!m_sTypeFilter.IsEmpty())
  {
    if (pInfo == nullptr) // if this is no asset, but we have an asset type filter active, always hide this thing
      return true;

    m_sTemp.Set(";", pInfo->m_Data.m_sSubAssetsDocumentTypeName, ";");

    if (!m_sTypeFilter.FindSubString(m_sTemp))
      return true;
  }

  return false;
}
