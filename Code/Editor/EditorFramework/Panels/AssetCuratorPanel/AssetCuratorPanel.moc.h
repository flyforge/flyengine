#pragma once

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetCuratorPanel.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

class plQtCuratorControl;
struct plLoggingEventData;

class PL_EDITORFRAMEWORK_DLL plQtAssetCuratorFilter : public plQtAssetFilter
{
  Q_OBJECT
public:
  explicit plQtAssetCuratorFilter(QObject* pParent);

  void SetFilterTransitive(bool bFilterTransitive);

public:
  virtual bool IsAssetFiltered(plStringView sDataDirParentRelativePath, bool bIsFolder, const plSubAsset* pInfo) const override;

  bool m_bFilterTransitive = true;
};

class PL_EDITORFRAMEWORK_DLL plQtAssetCuratorPanel : public plQtApplicationPanel, public Ui_AssetCuratorPanel
{
  Q_OBJECT

  PL_DECLARE_SINGLETON(plQtAssetCuratorPanel);

public:
  plQtAssetCuratorPanel();
  ~plQtAssetCuratorPanel();

public Q_SLOTS:
  void OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private Q_SLOTS:
  // note, because of the way we set up the widget, auto-connect doesn't work
  void onListAssetsDoubleClicked(const QModelIndex& index);
  void onCheckIndirectToggled(bool checked);

private:
  void LogWriter(const plLoggingEventData& e);
  void UpdateIssueInfo();

  plQtAssetBrowserModel* m_pModel;
  plQtAssetCuratorFilter* m_pFilter;
  QPersistentModelIndex m_SelectedIndex;
};

