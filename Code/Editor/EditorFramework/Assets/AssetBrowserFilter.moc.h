#pragma once

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>

class PLASMA_EDITORFRAMEWORK_DLL plQtAssetBrowserFilter : public plQtAssetFilter
{
  Q_OBJECT
public:
  explicit plQtAssetBrowserFilter(QObject* pParent, plUInt8 uMaxDepth = 32);

  /// Resets all filters to their default state.
  void Reset();

  void SetShowItemsInSubFolders(bool bShow);
  bool GetShowItemsInSubFolders() const { return m_bShowItemsInSubFolders; }

  void SetShowItemsInHiddenFolders(bool bShow);
  bool GetShowItemsInHiddenFolders() const { return m_bShowItemsInHiddenFolders; }

  void SetSortByRecentUse(bool bSort);
  bool GetSortByRecentUse() const { return m_bSortByRecentUse; }

  void SetTextFilter(const char* szText);
  const char* GetTextFilter() const { return m_sTextFilter; }

  //register is true when we are not using the history to set the filter, otherwise false
  void SetPathFilter(const char* szPath, bool bRegister = true);
  const char* GetPathFilter() const { return m_sPathFilter; }

  void SetTypeFilter(const char* szTypes);
  const char* GetTypeFilter() const { return m_sTypeFilter; }

  void OnPrevious();
  void OnNext();

Q_SIGNALS:
  void TextFilterChanged();
  void TypeFilterChanged();
  void PathFilterChanged();
  void SortByRecentUseChanged();

public:
  virtual bool IsAssetFiltered(const plSubAsset* pInfo) const override;
  virtual bool Less(const plSubAsset* pInfoA, const plSubAsset* pInfoB) const override;

private:
  plString m_sTextFilter, m_sTypeFilter, m_sPathFilter;
  bool m_bShowItemsInSubFolders = false;
  bool m_bShowItemsInHiddenFolders = false;
  bool m_bSortByRecentUse = false;
  mutable plStringBuilder m_sTemp; // stored here to reduce unnecessary allocations

  // Cache for uses search
  bool m_bUsesSearchActive = false;
  bool m_bTransitive = false;
  plSet<plUuid> m_Uses;

  //emplacement history
  //lists used as stacks, push, peek, pop from the front and sometimes flush half at the back (the older data)
  plUInt8 m_uMaxDepth;
  plDynamicArray<plString> m_HistoryPrev; //plList not compiling ??? -> highly inefficient with dynarr.
  plDynamicArray<plString> m_HistoryNext; //plList not compiling ??? -> highly inefficient with dynarr.
};
