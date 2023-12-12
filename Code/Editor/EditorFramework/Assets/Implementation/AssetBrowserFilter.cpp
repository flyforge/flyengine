#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>


plQtAssetBrowserFilter::plQtAssetBrowserFilter(QObject* pParent, plUInt8 uMaxDepth)
  : plQtAssetFilter(pParent)
  , m_uMaxDepth(uMaxDepth)
{
  m_HistoryPrev.Reserve(m_uMaxDepth);
  m_HistoryNext.Reserve(m_uMaxDepth);
}


void plQtAssetBrowserFilter::Reset()
{
  SetShowItemsInSubFolders(true);
  SetShowItemsInHiddenFolders(false);
  SetSortByRecentUse(false);
  SetTextFilter("");
  SetTypeFilter("");
  SetPathFilter("");
  m_HistoryPrev.Clear();
}

void plQtAssetBrowserFilter::SetShowItemsInSubFolders(bool bShow)
{
  if (m_bShowItemsInSubFolders == bShow)
    return;

  m_bShowItemsInSubFolders = bShow;

  Q_EMIT FilterChanged();
}

void plQtAssetBrowserFilter::SetShowItemsInHiddenFolders(bool bShow)
{
  if (m_bShowItemsInHiddenFolders == bShow)
    return;

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

  if (m_sTextFilter == sCleanText)
    return;

  m_sTextFilter = sCleanText;
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

void plQtAssetBrowserFilter::SetPathFilter(const char* szPath, bool bRegister)
{
  plStringBuilder sCleanText = szPath;
  sCleanText.MakeCleanPath();

  if (m_sPathFilter == sCleanText)
    return;

  

  if (bRegister)
  {
    m_HistoryPrev.PushBack(m_sPathFilter);

    //flush the oldest data if too big
    //flushes a chunk to avoid moving the data around every time
    if (m_HistoryPrev.GetCount() == m_uMaxDepth)
    {
      m_HistoryPrev.RemoveAtAndCopy(0, m_uMaxDepth/3);  
    }
  }

  m_sPathFilter = sCleanText;

  Q_EMIT FilterChanged();
  Q_EMIT PathFilterChanged();
}

void plQtAssetBrowserFilter::SetTypeFilter(const char* szTypes)
{
  if (m_sTypeFilter == szTypes)
    return;

  m_sTypeFilter = szTypes;

  Q_EMIT FilterChanged();
  Q_EMIT TypeFilterChanged();
}

void plQtAssetBrowserFilter::OnPrevious()
{
  if (m_HistoryPrev.IsEmpty())
    return;
  m_HistoryNext.PushBack(m_sPathFilter);
  SetPathFilter(m_HistoryPrev.PeekBack(), false);
  m_HistoryPrev.PopBack();
}

void plQtAssetBrowserFilter::OnNext()
{
  if (m_HistoryNext.IsEmpty())
    return;
  SetPathFilter(m_HistoryNext.PeekBack());
  m_HistoryNext.PopBack();
}


bool plQtAssetBrowserFilter::IsAssetFiltered(const plSubAsset* pInfo) const
{
  if (!m_sPathFilter.IsEmpty())
  {
    // if the string is not found in the path, ignore this asset
    if (!pInfo->m_pAssetInfo->m_sDataDirParentRelativePath.StartsWith_NoCase(m_sPathFilter))
      return true;
  }

  if (!m_bShowItemsInSubFolders)
  {
    // do we find another path separator after the prefix path?
    // if so, there is a sub-folder, and thus we ignore it
    if (plStringUtils::FindSubString(pInfo->m_pAssetInfo->m_sDataDirParentRelativePath + m_sPathFilter.GetElementCount() + 1, "/") != nullptr)
      return true;
  }

  if (!m_bShowItemsInHiddenFolders)
  {
    if (plStringUtils::FindSubString_NoCase(pInfo->m_pAssetInfo->m_sDataDirParentRelativePath + m_sPathFilter.GetElementCount() + 1, "_data/") !=
        nullptr)
      return true;
  }

  if (!m_sTextFilter.IsEmpty())
  {
    if (m_bUsesSearchActive)
    {
      if (!m_Uses.Contains(pInfo->m_Data.m_Guid))
        return true;
    }
    else
    {
      // if the string is not found in the path, ignore this asset
      if (pInfo->m_pAssetInfo->m_sDataDirRelativePath.FindSubString_NoCase(m_sTextFilter) == nullptr)
      {
        if (pInfo->GetName().FindSubString_NoCase(m_sTextFilter) == nullptr)
        {
          plConversionUtils::ToString(pInfo->m_Data.m_Guid, m_sTemp);
          if (m_sTemp.FindSubString_NoCase(m_sTextFilter) == nullptr)
            return true;

          // we could actually (partially) match the GUID
        }
      }
    }
  }

  if (!m_sTypeFilter.IsEmpty())
  {
    

    if (pInfo->m_bIsDir)  //Show only dir containing the selected type
    {
      bool bHasType = false;
      plStringBuilder sDataDir = pInfo->m_pAssetInfo->m_sAbsolutePath;
      plFileSystemIterator iterator;
      iterator.StartSearch(sDataDir, plFileSystemIteratorFlags::ReportFilesRecursive);

      if (!iterator.IsValid())
        return true;

      plStringBuilder sPath;
      plFileStats Stats;
      for (; !bHasType && iterator.IsValid(); iterator.Next())
      {
        Stats = iterator.GetStats();
        sPath = iterator.GetCurrentPath();
        sPath.AppendPath(Stats.m_sName);
        sPath.MakeCleanPath();

        plAssetCurator::plLockedSubAsset Info = plAssetCurator::GetSingleton()->FindSubAsset(sPath);
        if (Info)
        {
          m_sTemp.Set(";", Info->m_Data.m_sSubAssetsDocumentTypeName, ";");
          if (m_sTypeFilter.FindSubString(m_sTemp))
            bHasType = true;
        }
      }
      if (!bHasType)
      {
        return true;
      }
    }
    else
    {
      m_sTemp.Set(";", pInfo->m_Data.m_sSubAssetsDocumentTypeName, ";");
      if (!m_sTypeFilter.FindSubString(m_sTemp))
        return true;
    }
  }
  return false;
}

bool plQtAssetBrowserFilter::Less(const plSubAsset* pInfoA, const plSubAsset* pInfoB) const
{
  if (m_bSortByRecentUse && pInfoA->m_LastAccess.GetSeconds() != pInfoB->m_LastAccess.GetSeconds())
  {
    return pInfoA->m_LastAccess > pInfoB->m_LastAccess;
  }

  if (pInfoA->m_bIsDir != pInfoB->m_bIsDir)
  {
    return pInfoA->m_bIsDir;
  }

  plStringView sSortA = pInfoA->GetName();
  plStringView sSortB = pInfoB->GetName();

  plInt32 iValue = plStringUtils::Compare_NoCase(sSortA.GetStartPointer(), sSortB.GetStartPointer(), sSortA.GetEndPointer(), sSortB.GetEndPointer());
  if (iValue == 0)
  {
    return pInfoA->m_Data.m_Guid < pInfoB->m_Data.m_Guid;
  }
  return iValue < 0;
}
