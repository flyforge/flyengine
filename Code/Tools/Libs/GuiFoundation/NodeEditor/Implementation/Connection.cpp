#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QApplication>
#include <QPalette>

plQtConnection::plQtConnection(QGraphicsItem* parent)
  : QGraphicsPathItem(parent)
{
  auto palette = QApplication::palette();

  QPen pen(palette.text().color(), 3, Qt::SolidLine);
  setPen(pen);
  setBrush(Qt::NoBrush);

  m_InDir = QPointF(-1.0f, 0.0f);
  m_OutDir = QPointF(1.0f, 0.0f);
  setZValue(-1);
}

plQtConnection::~plQtConnection() = default;

void plQtConnection::InitConnection(const plDocumentObject* pObject, const plConnection* pConnection)
{
  m_pObject = pObject;
  m_pConnection = pConnection;
}

void plQtConnection::SetPosIn(const QPointF& point)
{
  m_InPoint = point;
  UpdateGeometry();
}

void plQtConnection::SetPosOut(const QPointF& point)
{
  m_OutPoint = point;
  UpdateGeometry();
}

void plQtConnection::SetDirIn(const QPointF& dir)
{
  m_InDir = dir;
  UpdateGeometry();
}

void plQtConnection::SetDirOut(const QPointF& dir)
{
  m_OutDir = dir;
  UpdateGeometry();
}

void plQtConnection::DrawSubwayPath(QPainterPath& path, const QPointF& startPoint, const QPointF& endPoint)
{
  const bool isStartAboveTarget = startPoint.y() <= endPoint.y();
  const bool isStartLeftOfTarget = startPoint.x() <= endPoint.x();

  const float diffX = fabs(endPoint.x() - startPoint.x());
  float diffY = fabs(endPoint.y() - startPoint.y());

  const qreal bezierOffset = 5;
  const qreal nodeCableOffset = 20;

  if (!isStartLeftOfTarget)
  {
    if (diffY < 0.000001f)
    {
      diffY += nodeCableOffset;
    }

    float step = diffY / 4 - bezierOffset;
    step = step < nodeCableOffset ? step : nodeCableOffset;

    const float yDistance = fabs(diffY - step * 4) / 2;

    const float x = startPoint.x() + step;
    const float x2 = x - step;
    const float x3 = x - step * 2 - diffX;

    if (!isStartAboveTarget)
    {
      const float y = startPoint.y() - step;
      const float y2 = startPoint.y() - step * 2;
      const float y3 = startPoint.y() - step * 3;

      path.lineTo(x, y);
      path.lineTo(x, y - yDistance);
      path.lineTo(x2, y2 - yDistance);
      path.lineTo(x2 - diffX, y2 - yDistance);
      path.lineTo(x3, y3 - yDistance);
      path.lineTo(x3, y3 - yDistance * 2);
    }
    else
    {
      const float y = startPoint.y() + step;
      const float y2 = startPoint.y() + step * 2;
      const float y3 = startPoint.y() + step * 3;

      path.lineTo(x, y);
      path.lineTo(x, y + yDistance);
      path.lineTo(x2, y2 + yDistance);
      path.lineTo(x2 - diffX, y2 + yDistance);
      path.lineTo(x3, y3 + yDistance);
      path.lineTo(x3, y3 + yDistance * 2);
    }
  }
  else
  {
    const bool isDistanceTooShort = diffX <= diffY;

    if (!isStartAboveTarget)
    {
      if (!isDistanceTooShort)
      {
        path.lineTo(startPoint.x() + diffY, startPoint.y() - diffY);
      }
      else
      {
        const float x = startPoint.x() + diffX / 2;
        path.lineTo(x, startPoint.y() - diffX / 2);
        path.lineTo(x, startPoint.y() - diffX / 2 - (diffY - diffX));
      }
    }
    else
    {
      if (!isDistanceTooShort)
      {
        path.lineTo(startPoint.x() + diffY, startPoint.y() + diffY);
      }
      else
      {
        const float x = startPoint.x() + diffX / 2;
        path.lineTo(x, startPoint.y() + diffX / 2);
        path.lineTo(x, startPoint.y() + diffX / 2 + (diffY - diffX));
      }
    }
  }
}


void plQtConnection::UpdateGeometry()
{
  constexpr float arrowHalfSize = 8.0f;

  prepareGeometryChange();

  QPainterPath p;
  QPointF dir = m_InPoint - m_OutPoint;

  auto pScene = static_cast<plQtNodeScene*>(scene());
  if (pScene->GetConnectionStyle() == plQtNodeScene::ConnectionStyle::StraightLine)
  {
    QPointF startPoint = m_OutPoint;
    QPointF endPoint = m_InPoint;

    if (pScene->GetConnectionDecorationFlags().IsSet(plQtNodeScene::ConnectionDecorationFlags::DirectionArrows))
    {
      const float length = plMath::Sqrt(dir.x() * dir.x() + dir.y() * dir.y());
      const float invLength = length != 0.0f ? 1.0f / length : 1.0f;
      const QPointF dirNorm = dir * invLength;
      const QPointF normal = QPointF(dirNorm.y(), -dirNorm.x());

      // offset start and endpoint
      startPoint -= normal * (arrowHalfSize * 1.3f);
      endPoint -= normal * (arrowHalfSize * 1.3f);

      const QPointF midPoint = startPoint + dir * 0.5f;
      const QPointF tipPoint = midPoint + dirNorm * arrowHalfSize;
      const QPointF backPoint = midPoint - dirNorm * arrowHalfSize;

      QPolygonF arrow;
      arrow.append(tipPoint);
      arrow.append(backPoint + normal * arrowHalfSize);
      arrow.append(backPoint - normal * arrowHalfSize);
      arrow.append(tipPoint);

      p.addPolygon(arrow);
    }

    p.moveTo(startPoint);
    p.lineTo(endPoint);
  }
  else if (pScene->GetConnectionStyle() == plQtNodeScene::ConnectionStyle::Subway)
  {
    QPointF startPoint = m_OutPoint;

    QPointF offsetStartPoint = QPointF(startPoint.x() + 20.0f, startPoint.y());

    QPointF endPoint = m_InPoint;
    QPointF offsetEndPoint = QPointF(endPoint.x() - 20.0f, endPoint.y());

    p.moveTo(startPoint);
    p.lineTo(offsetStartPoint);
    DrawSubwayPath(p, offsetStartPoint, offsetEndPoint);
    p.lineTo(offsetEndPoint);
    p.lineTo(endPoint);
  }
  else
  {
    p.moveTo(m_OutPoint);
    float fDotOut = QPointF::dotProduct(m_OutDir, dir);
    float fDotIn = QPointF::dotProduct(m_InDir, -dir);

    fDotOut = plMath::Max(100.0f, plMath::Abs(fDotOut));
    fDotIn = plMath::Max(100.0f, plMath::Abs(fDotIn));

    QPointF ctr1 = m_OutPoint + m_OutDir * (fDotOut * 0.5f);
    QPointF ctr2 = m_InPoint + m_InDir * (fDotIn * 0.5f);

    p.cubicTo(ctr1, ctr2, m_InPoint);
  }

  setPath(p);
}

QPen plQtConnection::DeterminePen() const
{
  if (m_pConnection == nullptr)
  {
    return pen();
  }

  plColor sourceColor = m_pConnection->GetSourcePin().GetColor();
  plColor targetColor = m_pConnection->GetTargetPin().GetColor();


  if (m_bAdjacentNodeSelected)
  {
    sourceColor = plMath::Lerp(sourceColor, plColor::White, 0.1f);
    targetColor = plMath::Lerp(targetColor, plColor::White, 0.1f);
  }

  QPointF startPoint = m_OutPoint;
  QPointF endPoint = m_InPoint;
  QPointF dir = endPoint - startPoint;
  const QPointF midPoint = startPoint + dir * 0.5f;


  QLinearGradient gradient(startPoint, endPoint);
  gradient.setColorAt(0.0, plToQtColor(sourceColor));
  gradient.setColorAt(1.0, plToQtColor(targetColor));

//  if (m_bAdjacentNodeSelected)
//  {
//    color = plMath::Lerp(color, plColorGammaUB(255, 255, 255), 0.1f);
//    return QPen(QBrush(plToQtColor(color)), 3, Qt::DashLine);
//  }
//  else
//  {
    return QPen(gradient, 2, Qt::SolidLine);
  //}
}

void plQtConnection::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  auto palette = QApplication::palette();

  QPen p = DeterminePen();
  if (isSelected())
  {
    p.setColor(palette.highlight().color());
  }
  painter->setPen(p);

  painter->setRenderHints(QPainter::Antialiasing, true);

  auto decorationFlags = static_cast<plQtNodeScene*>(scene())->GetConnectionDecorationFlags();
  if (decorationFlags.IsSet(plQtNodeScene::ConnectionDecorationFlags::DirectionArrows))
  {
    painter->setBrush(p.brush());
  }

  painter->drawPath(path());

  if (decorationFlags.IsSet(plQtNodeScene::ConnectionDecorationFlags::Debugging))
  {
    const float offset = fmod(plTime::Now().GetSeconds(), 1.0f);
    const qreal segments = path().length() / 16;

    for (qreal length = 0; length < segments + 0.0005f; ++length)
    {
      painter->drawEllipse(path().pointAtPercent(path().percentAtLength((length + offset) * 16)), 2, 2);
    }
  }
}

