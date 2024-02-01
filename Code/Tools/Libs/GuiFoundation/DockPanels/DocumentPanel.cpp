#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <QCloseEvent>

plDynamicArray<plQtDocumentPanel*> plQtDocumentPanel::s_AllDocumentPanels;

plQtDocumentPanel::plQtDocumentPanel(QWidget* pParent, plDocument* pDocument)
  : QDockWidget(pParent)
{
  m_pDocument = pDocument;
  s_AllDocumentPanels.PushBack(this);

  setBackgroundRole(QPalette::ColorRole::Highlight);

  setFeatures(DockWidgetFeature::DockWidgetFloatable | DockWidgetFeature::DockWidgetMovable);
}

plQtDocumentPanel::~plQtDocumentPanel()
{
  s_AllDocumentPanels.RemoveAndSwap(this);
}

void plQtDocumentPanel::closeEvent(QCloseEvent* e)
{
  e->ignore();
}

bool plQtDocumentPanel::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (plQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;
  }
  return QDockWidget::event(pEvent);
}
