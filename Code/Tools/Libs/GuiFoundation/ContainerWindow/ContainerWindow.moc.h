#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMainWindow>
#include <QSet>
#include <ToolsFoundation/Project/ToolsProject.h>

class plDocumentManager;
class plDocument;
class plQtApplicationPanel;
struct plDocumentTypeDescriptor;
class QLabel;

namespace ads
{
  class CDockManager;
  class CFloatingDockContainer;
  class CDockWidget;
} // namespace ads

/// \brief Container window that hosts documents and applications panels.
class PLASMA_GUIFOUNDATION_DLL plQtContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  /// \brief Constructor.
  plQtContainerWindow();
  ~plQtContainerWindow();

  static plQtContainerWindow* GetContainerWindow() { return s_pContainerWindow; }

  void AddDocumentWindow(plQtDocumentWindow* pDocWindow);
  void AddApplicationPanel(plQtApplicationPanel* pPanel);

  ads::CDockManager* GetDockManager() { return m_pDockManager; }

  static plResult EnsureVisibleAnyContainer(plDocument* pDocument);

  void GetDocumentWindows(plHybridArray<plQtDocumentWindow*, 16>& windows);

  void SaveWindowLayout();
  void SaveDocumentLayouts();
  void RestoreWindowLayout();

  void ScheduleRestoreWindowLayout();

protected:
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

private:
  friend class plQtDocumentWindow;
  friend class plQtApplicationPanel;

  plResult EnsureVisible(plQtDocumentWindow* pDocWindow);
  plResult EnsureVisible(plDocument* pDocument);
  plResult EnsureVisible(plQtApplicationPanel* pPanel);

  bool m_bWindowLayoutRestored;
  plInt32 m_iWindowLayoutRestoreScheduled;

private Q_SLOTS:
  void SlotDocumentTabCloseRequested();
  void SlotRestoreLayout();
  void SlotTabsContextMenuRequested(const QPoint& pos);
  void SlotUpdateWindowDecoration(void* pDocWindow);
  void SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget);
  void SlotDockWidgetFloatingChanged(bool bFloating);

private:
  void UpdateWindowTitle();

  void RemoveDocumentWindow(plQtDocumentWindow* pDocWindow);
  void RemoveApplicationPanel(plQtApplicationPanel* pPanel);

  void UpdateWindowDecoration(plQtDocumentWindow* pDocWindow);

  void DocumentWindowEventHandler(const plQtDocumentWindowEvent& e);
  void ProjectEventHandler(const plToolsProjectEvent& e);
  void UIServicesEventHandler(const plQtUiServices::Event& e);

  virtual void closeEvent(QCloseEvent* e) override;

private:
  ads::CDockManager* m_pDockManager = nullptr;
  QLabel* m_pStatusBarLabel;
  plDynamicArray<plQtDocumentWindow*> m_DocumentWindows;
  plDynamicArray<ads::CDockWidget*> m_DocumentDocks;

  plDynamicArray<plQtApplicationPanel*> m_ApplicationPanels;
  QSet<QString> m_DockNames;

  static plQtContainerWindow* s_pContainerWindow;
  static bool s_bForceClose;
};

