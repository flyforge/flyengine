#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Strings/StringBuilder.h>
#include <GuiFoundation/Widgets/GridBarWidget.moc.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <QPainter>
#include <QTextOption>
#include <qevent.h>

plQGridBarWidget::plQGridBarWidget(QWidget* pParent)
  : QWidget(pParent)
{
  m_ViewportSceneRect.setRect(0, 1, 1, 1);
  m_fFineGridStops = 10;
  m_fTextGridStops = 100;
}

void plQGridBarWidget::SetConfig(
  const QRectF& viewportSceneRect, double fTextGridStops, double fFineGridStops, plDelegate<QPointF(const QPointF&)> mapFromSceneFunc)
{
  m_MapFromSceneFunc = mapFromSceneFunc;

  bool bUpdate = false;
  if (m_ViewportSceneRect != viewportSceneRect)
  {
    m_ViewportSceneRect = viewportSceneRect;
    bUpdate = true;
  }

  if (m_fTextGridStops != fTextGridStops)
  {
    m_fTextGridStops = fTextGridStops;
    bUpdate = true;
  }

  if (m_fFineGridStops != fFineGridStops)
  {
    m_fFineGridStops = fFineGridStops;
    bUpdate = true;
  }

  if (bUpdate)
  {
    update();
  }
}

void plQGridBarWidget::paintEvent(QPaintEvent* e)
{
  if (!m_MapFromSceneFunc.IsValid())
  {
    QWidget::paintEvent(e);
    return;
  }

  QPainter Painter(this);
  QPainter* painter = &Painter;
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setRenderHint(QPainter::TextAntialiasing);

  QRect areaRect = rect();

  // background
  painter->fillRect(areaRect, palette().button());
  painter->translate(0.5, 0.5);

  // render fine grid stop lines
  {
    double fSceneMinX, fSceneMaxX;
    plWidgetUtils::ComputeGridExtentsX(m_ViewportSceneRect, m_fFineGridStops, fSceneMinX, fSceneMaxX);
    fSceneMinX = plMath::Max(fSceneMinX, 0.0);

    painter->setPen(palette().buttonText().color());

    plHybridArray<QLine, 100> lines;

    // some overcompensation for the case that the GraphicsView displays a scrollbar at the side
    for (double x = fSceneMinX; x <= fSceneMaxX + m_fTextGridStops; x += m_fFineGridStops)
    {
      const QPointF pos = m_MapFromSceneFunc(QPointF(x, 0));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(pos.x(), areaRect.bottom() - 3, pos.x(), areaRect.bottom());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // Grid Stop Value Text
  {
    double fSceneMinX, fSceneMaxX;
    plWidgetUtils::ComputeGridExtentsX(m_ViewportSceneRect, m_fTextGridStops, fSceneMinX, fSceneMaxX);
    fSceneMinX = plMath::Max(fSceneMinX, 0.0);

    QTextOption textOpt(Qt::AlignCenter);
    QRectF textRect;

    painter->setPen(palette().buttonText().color());

    plStringBuilder tmp;

    for (double x = fSceneMinX; x <= fSceneMaxX; x += m_fTextGridStops)
    {
      const QPointF pos = m_MapFromSceneFunc(QPointF(x, 0));

      textRect.setRect(pos.x() - 50, areaRect.top(), 99, areaRect.height());
      tmp.Format("{0}", plArgF(x));

      painter->drawText(textRect, tmp.GetData(), textOpt);
    }
  }
}
