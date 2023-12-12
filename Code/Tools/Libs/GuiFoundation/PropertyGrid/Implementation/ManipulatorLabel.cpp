#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/Implementation/ManipulatorLabel.moc.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QFont>
#include <qevent.h>

plQtManipulatorLabel::plQtManipulatorLabel(QWidget* parent, Qt::WindowFlags f)
  : QLabel(parent, f)
  , m_pItems(nullptr)
  , m_pManipulator(nullptr)
  , m_bActive(false)
{
  setCursor(Qt::WhatsThisCursor);
}

plQtManipulatorLabel::plQtManipulatorLabel(const QString& text, QWidget* parent, Qt::WindowFlags f)
  : QLabel(text, parent, f)
  , m_pItems(nullptr)
  , m_pManipulator(nullptr)
  , m_bActive(false)
  , m_bIsDefault(true)
{
}

const plManipulatorAttribute* plQtManipulatorLabel::GetManipulator() const
{
  return m_pManipulator;
}

void plQtManipulatorLabel::SetManipulator(const plManipulatorAttribute* pManipulator)
{
  m_pManipulator = pManipulator;

  if (m_pManipulator)
  {
    setCursor(Qt::PointingHandCursor);
    setForegroundRole(QPalette::ColorRole::Link);
  }
}

bool plQtManipulatorLabel::GetManipulatorActive() const
{
  return m_bActive;
}

void plQtManipulatorLabel::SetManipulatorActive(bool bActive)
{
  m_bActive = bActive;

  if (m_pManipulator)
  {
    setForegroundRole(m_bActive ? QPalette::ColorRole::LinkVisited : QPalette::ColorRole::Link);
  }
}

void plQtManipulatorLabel::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  m_pItems = &items;
}


void plQtManipulatorLabel::SetIsDefault(bool bIsDefault)
{
  if (m_bIsDefault != bIsDefault)
  {
    m_bIsDefault = bIsDefault;
    m_Font.setBold(!m_bIsDefault);
    setFont(m_Font);
  }
}


void plQtManipulatorLabel::contextMenuEvent(QContextMenuEvent* ev)
{
  Q_EMIT customContextMenuRequested(ev->globalPos());
}

void plQtManipulatorLabel::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set font.
  setFont(m_Font);
  QLabel::showEvent(event);
}

void plQtManipulatorLabel::mousePressEvent(QMouseEvent* ev)
{
  if (ev->button() != Qt::LeftButton)
    return;

  if (m_pManipulator == nullptr)
    return;

  const plDocument* pDoc = (*m_pItems)[0].m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  if (m_bActive)
    plManipulatorManager::GetSingleton()->ClearActiveManipulator(pDoc);
  else
    plManipulatorManager::GetSingleton()->SetActiveManipulator(pDoc, m_pManipulator, *m_pItems);
}

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
void plQtManipulatorLabel::enterEvent(QEnterEvent* ev)
#else
void plQtManipulatorLabel::enterEvent(QEvent* ev)
#endif
{
  if (m_pManipulator)
  {
    m_Font.setUnderline(true);
    setFont(m_Font);
  }

  QLabel::enterEvent(ev);
}

void plQtManipulatorLabel::leaveEvent(QEvent* ev)
{
  if (m_pManipulator)
  {
    m_Font.setUnderline(false);
    setFont(m_Font);
  }

  QLabel::leaveEvent(ev);
}
