#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

#include <ads/DockAreaWidget.h>
#include <ads/DockContainerWidget.h>
#include <ads/DockWidgetTab.h>

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plQtApplicationPanel, plNoBase, 1, plRTTINoAllocator)
PLASMA_END_STATIC_REFLECTED_TYPE;

plDynamicArray<plQtApplicationPanel*> plQtApplicationPanel::s_AllApplicationPanels;

plQtApplicationPanel::plQtApplicationPanel(const char* szPanelName)
  : ads::CDockWidget(szPanelName, plQtContainerWindow::GetContainerWindow())
{
  plStringBuilder sPanel("AppPanel_", szPanelName);

  setObjectName(plMakeQString(sPanel));
  setWindowTitle(plMakeQString(plTranslate(szPanelName)));

  s_AllApplicationPanels.PushBack(this);

  m_pContainerWindow = nullptr;

  plQtContainerWindow::GetContainerWindow()->AddApplicationPanel(this);

  plToolsProject::s_Events.AddEventHandler(plMakeDelegate(&plQtApplicationPanel::ToolsProjectEventHandler, this));
}

plQtApplicationPanel::~plQtApplicationPanel()
{
  plToolsProject::s_Events.RemoveEventHandler(plMakeDelegate(&plQtApplicationPanel::ToolsProjectEventHandler, this));

  s_AllApplicationPanels.RemoveAndSwap(this);
}

void plQtApplicationPanel::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this).IgnoreResult();

  QWidget* pThis = this;

  if (dockAreaWidget())
  {
    dockAreaWidget()->setCurrentDockWidget(this);
  }

  while (pThis)
  {
    pThis->raise();
    pThis = qobject_cast<QWidget*>(pThis->parent());
  }
}


void plQtApplicationPanel::ToolsProjectEventHandler(const plToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case plToolsProjectEvent::Type::ProjectClosing:
      setEnabled(false);
      break;
    case plToolsProjectEvent::Type::ProjectOpened:
      setEnabled(true);
      break;

    default:
      break;
  }
}

bool plQtApplicationPanel::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (plQtProxy::TriggerDocumentAction(nullptr, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;
  }
  return ads::CDockWidget::event(pEvent);
}
