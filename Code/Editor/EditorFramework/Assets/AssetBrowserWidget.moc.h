#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserWidget.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <EditorFramework/Assets/CuratorControl.moc.h>

class plQtToolBarActionMapView;
class plQtAssetBrowserFilter;
class plQtAssetBrowserModel;
struct plAssetCuratorEvent;

class plQtAssetBrowserWidget : public QWidget, public Ui_AssetBrowserWidget
{
  Q_OBJECT
public:
  plQtAssetBrowserWidget(QWidget* parent);
  ~plQtAssetBrowserWidget();

  void SetDialogMode();
  void SetSelectedAsset(plUuid preselectedAsset);
  void ShowOnlyTheseTypeFilters(const char* szFilters);

  void SaveState(const char* szSettingsName);
  void RestoreState(const char* szSettingsName);

  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
  void dragLeaveEvent(QDragLeaveEvent* event) override;
  void dropEvent(QDropEvent* event) override;

  plQtAssetBrowserModel* GetAssetBrowserModel() { return m_pModel; }
  const plQtAssetBrowserModel* GetAssetBrowserModel() const { return m_pModel; }
  plQtAssetBrowserFilter* GetAssetBrowserFilter() { return m_pFilter; }
  const plQtAssetBrowserFilter* GetAssetBrowserFilter() const { return m_pFilter; }

Q_SIGNALS:
  void ItemChosen(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void ItemSelected(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void ItemCleared();

private Q_SLOTS:
  void OnTextFilterChanged();
  void OnTypeFilterChanged();
  void OnPathFilterChanged();
  void on_ListAssets_doubleClicked(const QModelIndex& index);
  void on_ListAssets_activated(const QModelIndex& index);
  void on_ListAssets_clicked(const QModelIndex& index);
  void onPreviousFolder();
  void onNextFolder();
  void on_ButtonListMode_clicked();
  void on_ButtonIconMode_clicked();
  void on_IconSizeSlider_valueChanged(int iValue);
  void on_ListAssets_ViewZoomed(plInt32 iIconSizePercentage);
  void OnSearchWidgetTextChanged(const QString& text);
  void on_ListTypeFilter_itemChanged(QListWidgetItem* item);
  void on_TreeFolderFilter_itemSelectionChanged();
  void on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt);
  void on_TypeFilter_currentIndexChanged(int index);
  void OnScrollToItem(plUuid preselectedAsset);
  void OnTreeOpenExplorer();
  void OnShowSubFolderItemsToggled();
  void OnShowHiddenFolderItemsToggled();
  void on_ListAssets_customContextMenuRequested(const QPoint& pt);
  void OnListOpenExplorer();
  void OnListOpenAssetDocument();
  void OnTransform();
  void OnListToggleSortByRecentlyUsed();
  void OnListCopyAssetGuid();
  void OnFilterToThisPath();
  void OnListFindAllReferences(bool transitive);
  void OnSelectionTimer();
  void OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void OnAssetSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
  void OnModelReset();
  void OnNewAsset();
  void OnAddFolder();
  void OnDelete();
  void OnDuplicate();
  void OnRename();

private:
  void AssetCuratorEventHandler(const plAssetCuratorEvent& e);
  void UpdateDirectoryTree();
  void ClearDirectoryTree();
  void BuildDirectoryTree(const char* szCurPath, QTreeWidgetItem* pParent, const char* szCurPathToItem, bool bIsHidden, const char* szAbsPath);
  bool SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath);
  void UpdateAssetTypes();
  void ProjectEventHandler(const plToolsProjectEvent& e);
  void AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset);
  QString getSelectedPath(); //used to find the path for folder and asset creation

  bool m_bDialogMode;
  plUInt32 m_uiKnownAssetFolderCount;
  bool m_bTreeSelectionChangeInProgress = false;

  QStatusBar* m_pStatusBar;
  plQtCuratorControl* m_pCuratorControl;

  plQtToolBarActionMapView* m_pToolbar;
  plString m_sAllTypesFilter;
  plQtAssetBrowserModel* m_pModel;
  plQtAssetBrowserFilter* m_pFilter;
};

