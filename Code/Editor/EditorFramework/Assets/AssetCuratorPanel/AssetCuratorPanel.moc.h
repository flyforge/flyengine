#pragma once

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetCuratorPanel.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

class plQtCuratorControl;
struct plLoggingEventData;

class plQtAssetCuratorFilter : public plQtAssetFilter
{
  Q_OBJECT
public:
  explicit plQtAssetCuratorFilter(QObject* pParent);

  void SetFilterTransitive(bool bFilterTransitive);

public:
  virtual bool IsAssetFiltered(const plSubAsset* pInfo) const override;
  virtual bool Less(const plSubAsset* pInfoA, const plSubAsset* pInfoB) const override;

  bool m_bFilterTransitive = true;
};

class plQtAssetCuratorPanel : public QWidget, public Ui_AssetCuratorPanel
{
  Q_OBJECT

public:
  plQtAssetCuratorPanel(QWidget* parent);
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

