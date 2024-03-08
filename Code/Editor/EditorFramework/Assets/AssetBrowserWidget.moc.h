#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserWidget.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Project/ToolsProject.h>



class plQtToolBarActionMapView;
class plQtAssetBrowserFilter;
class plQtAssetBrowserModel;
struct plAssetCuratorEvent;
class plQtAssetBrowserModel;

class plQtAssetBrowserWidget : public QWidget, public Ui_AssetBrowserWidget
{
  Q_OBJECT
public:
  plQtAssetBrowserWidget(QWidget* pParent);
  ~plQtAssetBrowserWidget();

  enum class Mode
  {
    Browser,
    AssetPicker,
    FilePicker,
  };

  void SetMode(Mode mode);
  void SetSelectedAsset(plUuid preselectedAsset);
  void SetSelectedFile(plStringView sAbsPath);
  void ShowOnlyTheseTypeFilters(plStringView sFilters);
  void UseFileExtensionFilters(plStringView sFileExtensions);

  void SaveState(const char* szSettingsName);
  void RestoreState(const char* szSettingsName);

  //void dragEnterEvent(QDragEnterEvent* event) override;
  //void dragMoveEvent(QDragMoveEvent* event) override;
  //void dragLeaveEvent(QDragLeaveEvent* event) override;
  //void dropEvent(QDropEvent* event) override;


  plQtAssetBrowserModel* GetAssetBrowserModel() { return m_pModel; }
  const plQtAssetBrowserModel* GetAssetBrowserModel() const { return m_pModel; }
  plQtAssetBrowserFilter* GetAssetBrowserFilter() { return m_pFilter; }
  const plQtAssetBrowserFilter* GetAssetBrowserFilter() const { return m_pFilter; }

Q_SIGNALS:
  void ItemChosen(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, plUInt8 uiAssetBrowserItemFlags);
  void ItemSelected(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, plUInt8 uiAssetBrowserItemFlags);
  void ItemCleared();

private Q_SLOTS:
  void OnTextFilterChanged();
  void OnTypeFilterChanged();
  void OnPathFilterChanged();
  void on_ListAssets_doubleClicked(const QModelIndex& index);
  void on_ListAssets_activated(const QModelIndex& index);
  void on_ListAssets_clicked(const QModelIndex& index);
  void on_ButtonListMode_clicked();
  void on_ButtonIconMode_clicked();
  void on_IconSizeSlider_valueChanged(int iValue);
  void on_ListAssets_ViewZoomed(plInt32 iIconSizePercentage);
  void OnSearchWidgetTextChanged(const QString& text);
  void on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt);
  void on_TypeFilter_currentIndexChanged(int index);
  void OnScrollToItem(plUuid preselectedAsset);
  void OnScrollToFile(QString sPreselectedFile);
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
  void NewAsset();
  void OnFileEditingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset);
  void ImportSelection();
  void OnOpenImportReferenceAsset();
  void DeleteSelection();
  void OnImportAsAboutToShow();
  void OnImportAsClicked();


private:
  virtual void keyPressEvent(QKeyEvent* e) override;

private:
  void AssetCuratorEventHandler(const plAssetCuratorEvent& e);
  void UpdateAssetTypes();
  void ProjectEventHandler(const plToolsProjectEvent& e);
  void AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset);
  void AddImportedViaMenu(QMenu* pMenu);
  void GetSelectedImportableFiles(plDynamicArray<plString>& out_Files) const;

  Mode m_Mode = Mode::Browser;
  plQtToolBarActionMapView* m_pToolbar = nullptr;
  plString m_sAllTypesFilter;
  plQtAssetBrowserModel* m_pModel = nullptr;
  plQtAssetBrowserFilter* m_pFilter = nullptr;

  /// \brief After creating a new asset and renaming it, we want to open it as well.
  bool m_bOpenAfterRename = false;
};
