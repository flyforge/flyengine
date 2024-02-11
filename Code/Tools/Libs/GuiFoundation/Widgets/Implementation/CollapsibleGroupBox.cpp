#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>

plQtCollapsibleGroupBox::plQtCollapsibleGroupBox(QWidget* pParent)
  : plQtGroupBoxBase(pParent, true)

{
  setupUi(this);

  Header->installEventFilter(this);
}

void plQtCollapsibleGroupBox::SetTitle(plStringView sTitle)
{
  plQtGroupBoxBase::SetTitle(sTitle);
  update();
}

void plQtCollapsibleGroupBox::SetIcon(const QIcon& icon)
{
  plQtGroupBoxBase::SetIcon(icon);
  update();
}

void plQtCollapsibleGroupBox::SetFillColor(const QColor& color)
{
  plQtGroupBoxBase::SetFillColor(color);
  update();
}

void plQtCollapsibleGroupBox::SetCollapseState(bool bCollapsed)
{
  if (bCollapsed == m_bCollapsed)
    return;

  plQtScopedUpdatesDisabled sud(this);

  m_bCollapsed = bCollapsed;
  Content->setVisible(!bCollapsed);

  // Force re-layout of parent hierarchy to prevent flicker.
  QWidget* pCur = this;
  while (pCur != nullptr && qobject_cast<QScrollArea*>(pCur) == nullptr)
  {
    pCur->updateGeometry();
    pCur = pCur->parentWidget();
  }

  Q_EMIT CollapseStateChanged(bCollapsed);
}

bool plQtCollapsibleGroupBox::GetCollapseState() const
{
  return m_bCollapsed;
}

QWidget* plQtCollapsibleGroupBox::GetContent()
{
  return Content;
}

QWidget* plQtCollapsibleGroupBox::GetHeader()
{
  return Header;
}

bool plQtCollapsibleGroupBox::eventFilter(QObject* object, QEvent* event)
{
  switch (event->type())
  {
    case QEvent::Type::MouseButtonPress:
      HeaderMousePress(static_cast<QMouseEvent*>(event));
      return true;
    case QEvent::Type::MouseMove:
      HeaderMouseMove(static_cast<QMouseEvent*>(event));
      return true;
    case QEvent::Type::MouseButtonRelease:
      HeaderMouseRelease(static_cast<QMouseEvent*>(event));
      return true;
    default:
      break;
  }
  return false;
}

void plQtCollapsibleGroupBox::paintEvent(QPaintEvent* event)
{
  const QPalette& pal = palette();
  QWidget::paintEvent(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  QRect wr = contentsRect();
  QRect hr = Header->contentsRect();
  hr.moveTopLeft(Header->pos());

  QRect cr = wr;
  cr.setTop(hr.height());
  cr.adjust(Rounding / 2, 0, 0, -Rounding / 2);

  if (m_FillColor.isValid())
  {
    QLinearGradient colorGradient(wr.topLeft(), wr.bottomLeft());
    colorGradient.setColorAt(0.5f, m_FillColor);

    if (!m_bCollapsed)
      colorGradient.setColorAt(0.9f, pal.mid().color());

    QRectF wrAdjusted = wr;
    wrAdjusted.adjust(0.5, 0.5, Rounding, -0.5);
    QPainterPath oPath;
    oPath.addRoundedRect(wrAdjusted, Rounding, Rounding);
    p.fillPath(oPath, colorGradient);

    QRectF crAdjusted = cr;
    crAdjusted.adjust(0.5, 0.5, Rounding, -0.5);
    QPainterPath path;
    path.addRoundedRect(crAdjusted, Rounding, Rounding);
    p.fillPath(path, colorGradient);
  }
  else
  {
    QRectF wrAdjusted = wr;
    wrAdjusted.adjust(0.8, 0.8, Rounding, -0.8);
    QPainterPath oPath;
    oPath.addRoundedRect(wrAdjusted, Rounding, Rounding);
    p.fillPath(oPath, pal.alternateBase());
  }

  if (!Header->isHidden())
  {
    DrawHeader(p, hr);
  }
}
