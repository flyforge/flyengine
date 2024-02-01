#pragma once

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <ToolsFoundation/Utilities/SearchPatternFilter.h>

class PL_EDITORFRAMEWORK_DLL plQtAssetBrowserFilter : public plQtAssetFilter
{
  Q_OBJECT
public:
  explicit plQtAssetBrowserFilter(QObject* pParent);

  /// Resets all filters to their default state.
  void Reset();
  void UpdateImportExtensions(const plSet<plString>& extensions);

  void SetShowItemsInSubFolders(bool bShow);
  bool GetShowItemsInSubFolders() const { return m_bShowItemsInSubFolders; }

  void SetShowFiles(bool bShow);
  bool GetShowFiles() const { return m_bShowFiles; }

  void SetShowNonImportableFiles(bool bShow);
  bool GetShowNonImportableFiles() const { return m_bShowNonImportableFiles; }

  void SetShowItemsInHiddenFolders(bool bShow);
  bool GetShowItemsInHiddenFolders() const { return m_bShowItemsInHiddenFolders; }

  void SetSortByRecentUse(bool bSort);
  virtual bool GetSortByRecentUse() const override { return m_bSortByRecentUse; }

  void SetTextFilter(const char* szText);
  const char* GetTextFilter() const { return m_SearchFilter.GetSearchText(); }

  void SetPathFilter(const char* szPath);
  plStringView GetPathFilter() const;

  void SetTypeFilter(const char* szTypes);
  const char* GetTypeFilter() const { return m_sTypeFilter; }

  void SetFileExtensionFilters(plStringView sExtensions);

  /// \brief If set, the given item will be visible no matter what until any other filter is changed.
  /// This is used to ensure that newly created assets are always visible, even if they are excluded from the current filter.
  void SetTemporaryPinnedItem(plStringView sDataDirParentRelativePath);
  plStringView GetTemporaryPinnedItem() const { return m_sTemporaryPinnedItem; }

Q_SIGNALS:
  void TextFilterChanged();
  void TypeFilterChanged();
  void PathFilterChanged();
  void SortByRecentUseChanged();

public:
  virtual bool IsAssetFiltered(plStringView sDataDirParentRelativePath, bool bIsFolder, const plSubAsset* pInfo) const override;

private:
  plString m_sTypeFilter;
  plString m_sPathFilter;
  plString m_sTemporaryPinnedItem;
  plSearchPatternFilter m_SearchFilter;
  bool m_bShowItemsInSubFolders = false;
  bool m_bShowFiles = true;
  bool m_bShowNonImportableFiles = true;
  bool m_bShowItemsInHiddenFolders = false;
  bool m_bSortByRecentUse = false;
  mutable plStringBuilder m_sTemp; // stored here to reduce unnecessary allocations

  // Cache for uses search
  bool m_bUsesSearchActive = false;
  bool m_bTransitive = false;
  plSet<plUuid> m_Uses;
  plSet<plString> m_ImportExtensions;
  plSet<plString> m_FileExtensions;
};
