#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <QCloseEvent>

plDynamicArray<plQtDocumentPanel*> plQtDocumentPanel::s_AllDocumentPanels;

plQtDocumentPanel::plQtDocumentPanel(QWidget* parent, plDocument* pDocument)
  : QDockWidget(parent)
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

bool plQtDocumentPanel::event(QEvent* event)
{
  if (event->type() == QEvent::ShortcutOverride)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (plQtProxy::TriggerDocumentAction(m_pDocument, keyEvent))
      return true;
  }
  return QDockWidget::event(event);
}
