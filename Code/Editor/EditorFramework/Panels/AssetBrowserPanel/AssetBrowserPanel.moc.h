#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserPanel.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

class QStatusBar;
class QLabel;
struct plToolsProjectEvent;
class plQtCuratorControl;

/// \brief The application wide panel that shows and asset browser.
class PLASMA_EDITORFRAMEWORK_DLL plQtAssetBrowserPanel : public plQtApplicationPanel, public Ui_AssetBrowserPanel
{
  Q_OBJECT

  PLASMA_DECLARE_SINGLETON(plQtAssetBrowserPanel);

public:
  plQtAssetBrowserPanel();
  ~plQtAssetBrowserPanel();

  const plUuid& GetLastSelectedAsset() const { return m_LastSelected; }

private Q_SLOTS:
  void SlotAssetChosen(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, plUInt8 uiAssetBrowserItemFlags);
  void SlotAssetSelected(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, plUInt8 uiAssetBrowserItemFlags);
  void SlotAssetCleared();

private:
  void AssetCuratorEvents(const plAssetCuratorEvent& e);
  void ProjectEvents(const plToolsProjectEvent& e);

  plUuid m_LastSelected;
  QStatusBar* m_pStatusBar;
  plQtCuratorControl* m_pCuratorControl;
};
