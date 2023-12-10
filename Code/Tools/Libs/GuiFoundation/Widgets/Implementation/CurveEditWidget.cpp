#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <GuiFoundation/Widgets/CurveEditWidget.moc.h>
#include <GuiFoundation/Widgets/GridBarWidget.moc.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <QPainter>
#include <QRubberBand>
#include <qevent.h>

plQtCurveEditWidget::plQtCurveEditWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setFocusPolicy(Qt::FocusPolicy::ClickFocus);
  setMouseTracking(true);

  m_SceneTranslation = QPointF(-2, 0);
  m_SceneToPixelScale = QPointF(1, -1);

  m_ControlPointBrush.setColor(plToQtColor(plColorScheme::LightUI(plColorScheme::Gray)));
  m_ControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);

  m_SelectedControlPointBrush.setColor(plToQtColor(plColorScheme::LightUI(plColorScheme::Yellow)));
  m_SelectedControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);

  m_TangentLinePen.setCosmetic(true);
  m_TangentLinePen.setColor(plToQtColor(plColorScheme::LightUI(plColorScheme::Orange)));
  m_TangentLinePen.setStyle(Qt::PenStyle::DashLine);

  m_TangentHandleBrush.setColor(plToQtColor(plColorScheme::LightUI(plColorScheme::Orange)));
  m_TangentHandleBrush.setStyle(Qt::BrushStyle::SolidPattern);
}

void plQtCurveEditWidget::SetCurves(plCurveGroupData* pCurveEditData)
{
  m_pCurveEditData = pCurveEditData;

  m_Curves.Clear();
  m_Curves.Reserve(pCurveEditData->m_Curves.GetCount());

  for (plUInt32 i = 0; i < pCurveEditData->m_Curves.GetCount(); ++i)
  {
    auto& curve = m_Curves.ExpandAndGetRef();

    pCurveEditData->ConvertToRuntimeData(i, curve);
  }

  // make sure the selection does not contain points that got deleted
  for (plUInt32 i = 0; i < m_SelectedCPs.GetCount();)
  {
    if (m_SelectedCPs[i].m_uiCurve >= m_Curves.GetCount() || m_SelectedCPs[i].m_uiPoint >= m_Curves[m_SelectedCPs[i].m_uiCurve].GetNumControlPoints())
    {
      m_SelectedCPs.RemoveAtAndCopy(i);
    }
    else
    {
      ++i;
    }
  }

  m_CurvesSorted = m_Curves;
  m_CurveExtents.SetCount(m_Curves.GetCount());
  m_fMinExtentValue = m_fLowerExtent;
  m_fMaxExtentValue = m_fUpperExtent;
  m_fMinValue = plMath::HighValue<float>();
  m_fMaxValue = -plMath::HighValue<float>();

  for (plUInt32 i = 0; i < m_CurvesSorted.GetCount(); ++i)
  {
    plCurve1D& curve = m_CurvesSorted[i];

    curve.SortControlPoints();
    curve.CreateLinearApproximation();

    curve.QueryExtents(m_CurveExtents[i].x, m_CurveExtents[i].y);

    double fMin, fMax;
    curve.QueryExtremeValues(fMin, fMax);

    if (!m_bLowerExtentFixed)
      m_fMinExtentValue = plMath::Min(m_fMinExtentValue, m_CurveExtents[i].x);
    if (!m_bUpperExtentFixed)
      m_fMaxExtentValue = plMath::Max(m_fMaxExtentValue, m_CurveExtents[i].y);

    m_fMinValue = plMath::Min(m_fMinValue, fMin);
    m_fMaxValue = plMath::Max(m_fMaxValue, fMax);
  }

  ComputeSelectionRect();

  update();
}

void plQtCurveEditWidget::SetScrubberPosition(double fPosition)
{
  m_bShowScrubber = true;
  m_fScrubberPosition = fPosition;

  update();
}

void plQtCurveEditWidget::FrameCurve()
{
  double fLowPoint = m_fMinValue;
  double fUprPoint = m_fMaxValue;

  if (m_fLowerRange > -plMath::HighValue<double>() * 0.5)
    fLowPoint = plMath::Min(fLowPoint, m_fLowerRange);
  else
    fLowPoint = plMath::Min(fLowPoint, 0.0);

  if (m_fUpperRange < plMath::HighValue<double>() * 0.5)
    fUprPoint = plMath::Max(fUprPoint, m_fUpperRange);
  else
    fUprPoint = plMath::Max(fUprPoint, 1.0);

  double fLeftPoint = plMath::Min(0.0, m_fMinExtentValue);
  double fRightPoint = plMath::Max(1.0, m_fMaxExtentValue);

  double fWidth = plMath::Max(1.0, fRightPoint - fLeftPoint);
  double fHeight = fUprPoint - fLowPoint;
  double fOffsetX = fLeftPoint;
  double fOffsetY = fLowPoint;

  Frame(fOffsetX, fOffsetY, fWidth, fHeight);
}

void plQtCurveEditWidget::FrameSelection()
{
  double fWidth = m_fMaxExtentValue - m_fMinExtentValue;
  double fHeight = m_fMaxValue - m_fMinValue;
  double fOffsetX = m_fMinExtentValue;
  double fOffsetY = m_fMinValue;

  if (m_Curves.GetCount() == 0)
  {
    fWidth = 10.0;
    fHeight = 10.0;
    fOffsetY = -5.0;
  }
  else if (m_SelectedCPs.GetCount() > 1)
  {
    fWidth = m_SelectionBRect.width();
    fHeight = m_SelectionBRect.height();

    fOffsetX = m_SelectionBRect.left();
    fOffsetY = m_SelectionBRect.top();
  }
  else if (m_SelectedCPs.GetCount() == 1)
  {
    fWidth = 0.1f;
    fHeight = 0.1f;

    const auto& point = m_pCurveEditData->m_Curves[m_SelectedCPs[0].m_uiCurve]->m_ControlPoints[m_SelectedCPs[0].m_uiPoint];
    fOffsetX = point.GetTickAsTime().GetSeconds() - 0.05;
    fOffsetY = point.m_fValue - 0.05;
  }

  Frame(fOffsetX, fOffsetY, fWidth, fHeight);
}

void plQtCurveEditWidget::Frame(double fOffsetX, double fOffsetY, double fWidth, double fHeight)
{
  m_bFrameBeforePaint = false;

  fWidth = plMath::Max(fWidth, 0.1);
  fHeight = plMath::Max(fHeight, 0.1);

  const double fFinalWidth = fWidth * 1.2;
  const double fFinalHeight = fHeight * 1.2;

  fOffsetX -= (fFinalWidth - fWidth) * 0.5;
  fOffsetY -= (fFinalHeight - fHeight) * 0.5;

  m_SceneToPixelScale.setX(rect().width() / fFinalWidth);
  m_SceneToPixelScale.setY(-rect().height() / fFinalHeight);
  m_SceneTranslation.setX(fOffsetX);
  m_SceneTranslation.setY((-rect().height() / m_SceneToPixelScale.y()) + fOffsetY);

  ClampZoomPan();

  update();
}

QPoint plQtCurveEditWidget::MapFromScene(const QPointF& pos) const
{
  double x = pos.x() - m_SceneTranslation.x();
  double y = pos.y() - m_SceneTranslation.y();
  x *= m_SceneToPixelScale.x();
  y *= m_SceneToPixelScale.y();

  return QPoint((int)x, (int)y);
}

QPointF plQtCurveEditWidget::MapToScene(const QPoint& pos) const
{
  double x = pos.x();
  double y = pos.y();
  x /= m_SceneToPixelScale.x();
  y /= m_SceneToPixelScale.y();

  return QPointF(x, y) + m_SceneTranslation;
}

plVec2 plQtCurveEditWidget::MapDirFromScene(const plVec2& vPos) const
{
  const float x = vPos.x * m_SceneToPixelScale.x();
  const float y = vPos.y * m_SceneToPixelScale.y();

  return plVec2(x, y);
}

void plQtCurveEditWidget::ClearSelection()
{
  m_SelectionBRect = QRectF();

  if (!m_SelectedCPs.IsEmpty())
  {
    m_SelectedCPs.Clear();
    update();
  }

  Q_EMIT SelectionChangedEvent();
}

void plQtCurveEditWidget::SelectAll()
{
  m_SelectedCPs.Clear();

  for (plUInt32 curveIdx = 0; curveIdx < m_Curves.GetCount(); ++curveIdx)
  {
    for (plUInt32 cpIdx = 0; cpIdx < m_Curves[curveIdx].GetNumControlPoints(); ++cpIdx)
    {
      auto& sel = m_SelectedCPs.ExpandAndGetRef();
      sel.m_uiCurve = curveIdx;
      sel.m_uiPoint = cpIdx;
    }
  }

  ComputeSelectionRect();
  update();

  Q_EMIT SelectionChangedEvent();
}

bool plQtCurveEditWidget::IsSelected(const plSelectedCurveCP& cp) const
{
  for (const auto& other : m_SelectedCPs)
  {
    if (other.m_uiCurve == cp.m_uiCurve && other.m_uiPoint == cp.m_uiPoint)
      return true;
  }

  return false;
}

void plQtCurveEditWidget::SetSelection(const plSelectedCurveCP& cp)
{
  m_SelectedCPs.Clear();
  m_SelectedCPs.PushBack(cp);

  ComputeSelectionRect();
  update();

  Q_EMIT SelectionChangedEvent();
}

void plQtCurveEditWidget::ToggleSelected(const plSelectedCurveCP& cp)
{
  SetSelected(cp, !IsSelected(cp));

  ComputeSelectionRect();
  update();

  Q_EMIT SelectionChangedEvent();
}

void plQtCurveEditWidget::SetSelected(const plSelectedCurveCP& cp, bool bSet)
{
  if (!bSet)
  {
    for (plUInt32 i = 0; i < m_SelectedCPs.GetCount(); ++i)
    {
      if (m_SelectedCPs[i].m_uiCurve == cp.m_uiCurve && m_SelectedCPs[i].m_uiPoint == cp.m_uiPoint)
      {
        m_SelectedCPs.RemoveAtAndCopy(i);
        break;
      }
    }
  }
  else
  {
    if (!IsSelected(cp))
    {
      m_SelectedCPs.PushBack(cp);
    }
  }

  ComputeSelectionRect();
  update();
  Q_EMIT SelectionChangedEvent();
}

bool plQtCurveEditWidget::GetSelectedTangent(plInt32& out_iCurve, plInt32& out_iPoint, bool& out_bLeftTangent) const
{
  out_iCurve = m_iSelectedTangentCurve;
  out_iPoint = m_iSelectedTangentPoint;
  out_bLeftTangent = m_bSelectedTangentLeft;
  return (out_iCurve >= 0);
}

QRectF plQtCurveEditWidget::ComputeViewportSceneRect() const
{
  const QPointF topLeft = MapToScene(rect().topLeft());
  const QPointF bottomRight = MapToScene(rect().bottomRight());

  return QRectF(topLeft, bottomRight);
}

void plQtCurveEditWidget::paintEvent(QPaintEvent* e)
{
  if (m_bFrameBeforePaint)
    FrameCurve();

  ClampZoomPan();

  QPainter painter(this);
  painter.fillRect(rect(), palette().base());
  painter.translate(0.5, 0.5);

  painter.setRenderHint(QPainter::Antialiasing, true);

  const QRectF viewportSceneRect = ComputeViewportSceneRect();

  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  plWidgetUtils::AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().width(), viewportSceneRect.width(), 20);

  RenderVerticalGrid(&painter, viewportSceneRect, fRoughGridDensity);

  if (m_pGridBar)
  {
    m_pGridBar->SetConfig(viewportSceneRect, fRoughGridDensity, fFineGridDensity, [this](const QPointF& pt) -> QPoint
      { return MapFromScene(pt); });
  }

  RenderSideLinesAndText(&painter, viewportSceneRect);

  const float fExtWidth = (float)(m_fMaxExtentValue - m_fMinExtentValue);

  RenderValueRanges(&painter);
  PaintCurveSegments(&painter, 0, 255);
  PaintCurveSegments(&painter, fExtWidth, 200);
  PaintCurveSegments(&painter, 2.0f * fExtWidth, 150);
  PaintOutsideAreaOverlay(&painter);
  PaintSelectedTangentLines(&painter);
  PaintControlPoints(&painter);
  PaintSelectedTangentHandles(&painter);
  PaintSelectedControlPoints(&painter);
  PaintMultiSelectionSquare(&painter);
  PaintScrubber(painter);
}

void plQtCurveEditWidget::ClampZoomPan()
{
  m_SceneTranslation.setX(plMath::Clamp(m_SceneTranslation.x(), -2.0, 50000.0));
  m_SceneTranslation.setY(plMath::Clamp(m_SceneTranslation.y(), -200000.0, 200000.0));
  m_SceneToPixelScale.setX(plMath::Clamp(m_SceneToPixelScale.x(), 0.0005, 10000.0));
  m_SceneToPixelScale.setY(plMath::Clamp(m_SceneToPixelScale.y(), -10000.0, -0.0005));
}

void plQtCurveEditWidget::mousePressEvent(QMouseEvent* e)
{
  QWidget::mousePressEvent(e);
  m_LastMousePos = e->pos();

  if (m_State != EditState::None)
    return;

  if (e->button() == Qt::RightButton)
  {
    m_State = EditState::RightClick;
    return;
  }

  if (e->buttons() == Qt::LeftButton) // nothing else pressed
  {
    const ClickTarget clickedOn = DetectClickTarget(e->pos());

    if (clickedOn == ClickTarget::Nothing || clickedOn == ClickTarget::SelectedPoint)
    {
      if (e->modifiers() == Qt::NoModifier)
      {
        m_ScaleStartPoint = MapToScene(e->pos());

        switch (WhereIsPoint(e->pos()))
        {
          case plQtCurveEditWidget::SelectArea::None:
            break;
          case plQtCurveEditWidget::SelectArea::Center:
            m_State = EditState::DraggingPoints;
            m_TotalPointDrag = QPointF();
            break;
          case plQtCurveEditWidget::SelectArea::Top:
            m_ScaleReferencePoint = m_SelectionBRect.topLeft();
            m_State = EditState::ScaleUpDown;
            break;
          case plQtCurveEditWidget::SelectArea::Bottom:
            m_ScaleReferencePoint = m_SelectionBRect.bottomRight();
            m_State = EditState::ScaleUpDown;
            break;
          case plQtCurveEditWidget::SelectArea::Left:
            m_State = EditState::ScaleLeftRight;
            m_ScaleReferencePoint = m_SelectionBRect.topRight();
            break;
          case plQtCurveEditWidget::SelectArea::Right:
            m_State = EditState::ScaleLeftRight;
            m_ScaleReferencePoint = m_SelectionBRect.topLeft();
            break;
        }
      }

      if (m_State == EditState::None)
      {
        plSelectedCurveCP cp;
        if (PickCpAt(e->pos(), 8, cp))
        {
          if (e->modifiers().testFlag(Qt::ControlModifier))
          {
            ToggleSelected(cp);
          }
          else if (e->modifiers().testFlag(Qt::ShiftModifier))
          {
            SetSelected(cp, true);
          }
          else if (e->modifiers().testFlag(Qt::AltModifier))
          {
            SetSelected(cp, false);
          }
          else
          {
            if (clickedOn == ClickTarget::Nothing)
              SetSelection(cp);

            m_State = EditState::DraggingPoints;
            m_TotalPointDrag = QPointF();
          }
        }
      }

      if (m_State == EditState::None && e->modifiers() == Qt::AltModifier)
      {
        m_iDraggedCurve = PickCurveAt(e->pos());

        if (m_iDraggedCurve >= 0)
          m_State = EditState::DraggingCurve;
      }

      if (m_State == EditState::None)
      {
        m_State = EditState::MultiSelect;
      }

      PLASMA_ASSERT_DEBUG(!m_bBegunChanges, "Invalid State");

      if (m_State == EditState::DraggingCurve)
      {
        Q_EMIT BeginOperationEvent("Drag Curve");
        m_bBegunChanges = true;
      }
      else if (m_State == EditState::DraggingPoints)
      {
        Q_EMIT BeginOperationEvent("Drag Points");
        m_bBegunChanges = true;
      }
      else if (m_State == EditState::ScaleLeftRight)
      {
        Q_EMIT BeginOperationEvent("Scale Points Left / Right");
        m_bBegunChanges = true;
      }
      else if (m_State == EditState::ScaleUpDown)
      {
        Q_EMIT BeginOperationEvent("Scale Points Up / Down");
        m_bBegunChanges = true;
      }

      update();
    }
    else if (clickedOn == ClickTarget::TangentHandle)
    {
      m_State = EditState::DraggingTangents;
      Q_EMIT BeginOperationEvent("Drag Tangents");
      PLASMA_ASSERT_DEBUG(!m_bBegunChanges, "Invalid State");
      m_bBegunChanges = true;
    }
  }

  if (m_State == EditState::MultiSelect && m_pRubberband == nullptr)
  {
    m_MultiSelectionStart = e->pos();
    m_MultiSelectRect = QRect();
    m_pRubberband = new QRubberBand(QRubberBand::Shape::Rectangle, this);
    m_pRubberband->setGeometry(QRect(m_MultiSelectionStart, QSize()));
    m_pRubberband->hide();
  }
}

void plQtCurveEditWidget::mouseReleaseEvent(QMouseEvent* e)
{
  QWidget::mouseReleaseEvent(e);

  if (e->button() == Qt::RightButton)
  {
    if (m_State == EditState::Panning)
      m_State = EditState::None;

    if (m_State == EditState::RightClick)
    {
      m_State = EditState::None;

      ContextMenuEvent(mapToGlobal(e->pos()), MapToScene(e->pos()));
    }
  }

  if (e->button() == Qt::LeftButton &&
      (m_State == EditState::DraggingPoints || m_State == EditState::DraggingPointsHorz || m_State == EditState::DraggingPointsVert ||
        m_State == EditState::DraggingTangents || m_State == EditState::DraggingCurve || m_State == EditState::ScaleLeftRight ||
        m_State == EditState::ScaleUpDown || m_State == EditState::MultiSelect))
  {
    m_State = EditState::None;
    m_iSelectedTangentCurve = -1;
    m_iSelectedTangentPoint = -1;
    m_TotalPointDrag = QPointF();

    if (m_bBegunChanges)
    {
      m_bBegunChanges = false;
      Q_EMIT EndOperationEvent(true);
    }

    update();
  }

  if (m_State != EditState::MultiSelect && m_pRubberband)
  {
    delete m_pRubberband;
    m_pRubberband = nullptr;

    if (!m_MultiSelectRect.isEmpty())
    {
      plDynamicArray<plSelectedCurveCP> change;
      ExecMultiSelection(change);
      m_MultiSelectRect = QRect();

      if (e->modifiers().testFlag(Qt::AltModifier))
      {
        CombineSelectionRemove(m_SelectedCPs, change);
      }
      else if (e->modifiers().testFlag(Qt::ShiftModifier))
      {
        CombineSelectionAdd(m_SelectedCPs, change);
      }
      else if (e->modifiers().testFlag(Qt::ControlModifier))
      {
        CombineSelectionToggle(m_SelectedCPs, change);
      }
      else
      {
        m_SelectedCPs = change;
      }

      ComputeSelectionRect();
      update();

      Q_EMIT SelectionChangedEvent();
    }
  }

  if (e->buttons() == Qt::NoButton)
  {
    unsetCursor();

    m_State = EditState::None;
    m_iSelectedTangentCurve = -1;
    m_iSelectedTangentPoint = -1;

    if (m_bBegunChanges)
    {
      m_bBegunChanges = false;
      Q_EMIT EndOperationEvent(true);
    }

    update();
  }
}

void plQtCurveEditWidget::mouseMoveEvent(QMouseEvent* e)
{
  QWidget::mouseMoveEvent(e);
  Qt::CursorShape cursor = Qt::ArrowCursor;

  const QPoint diff = e->pos() - m_LastMousePos;
  double moveX = (double)diff.x() / m_SceneToPixelScale.x();
  double moveY = (double)diff.y() / m_SceneToPixelScale.y();

  if (m_State == EditState::RightClick || m_State == EditState::Panning)
  {
    m_State = EditState::Panning;
    cursor = Qt::ClosedHandCursor;

    m_SceneTranslation.setX(m_SceneTranslation.x() - moveX);
    m_SceneTranslation.setY(m_SceneTranslation.y() - moveY);

    ClampZoomPan();

    update();
  }

  if (m_State == EditState::DraggingPoints)
  {
    if (e->modifiers() == Qt::ShiftModifier)
    {
      if (plMath::Abs(m_TotalPointDrag.x()) > plMath::Abs(m_TotalPointDrag.y()))
      {
        moveY = -m_TotalPointDrag.y();
        m_State = EditState::DraggingPointsHorz;
      }
      else
      {
        moveX = -m_TotalPointDrag.x();
        m_State = EditState::DraggingPointsVert;
      }
    }

    MoveControlPointsEvent(moveX, moveY);
    m_TotalPointDrag += QPointF(moveX, moveY);
  }
  else
  {
    if (m_State == EditState::DraggingPointsHorz)
    {
      MoveControlPointsEvent(moveX, 0);
    }

    if (m_State == EditState::DraggingPointsVert)
    {
      MoveControlPointsEvent(0, moveY);
    }
  }

  if (m_State == EditState::DraggingTangents)
  {
    MoveTangentsEvent(moveX, moveY);
  }

  if (m_State == EditState::MultiSelect && m_pRubberband)
  {
    m_MultiSelectRect = QRect(m_MultiSelectionStart, e->pos()).normalized();
    m_pRubberband->setGeometry(m_MultiSelectRect);
    m_pRubberband->show();
  }

  if (m_State == EditState::None && !m_SelectionBRect.isEmpty())
  {
    switch (WhereIsPoint(e->pos()))
    {
      case plQtCurveEditWidget::SelectArea::None:
        break;
      case plQtCurveEditWidget::SelectArea::Center:
        // cursor = Qt::SizeAllCursor;
        break;
      case plQtCurveEditWidget::SelectArea::Top:
      case plQtCurveEditWidget::SelectArea::Bottom:
        cursor = Qt::SizeVerCursor;
        break;
      case plQtCurveEditWidget::SelectArea::Left:
      case plQtCurveEditWidget::SelectArea::Right:
        cursor = Qt::SizeHorCursor;
        break;
    }
  }

  if (m_State == EditState::ScaleLeftRight)
  {
    cursor = Qt::SizeHorCursor;

    const QPointF wsPos = MapToScene(e->pos());
    const QPointF norm = m_ScaleReferencePoint - m_ScaleStartPoint;
    const QPointF wsDiff = m_ScaleReferencePoint - wsPos;

    ScaleControlPointsEvent(m_ScaleReferencePoint, wsDiff.x() / norm.x(), 1);
  }

  if (m_State == EditState::ScaleUpDown)
  {
    cursor = Qt::SizeVerCursor;

    const QPointF wsPos = MapToScene(e->pos());
    const QPointF norm = m_ScaleReferencePoint - m_ScaleStartPoint;
    const QPointF wsDiff = m_ScaleReferencePoint - wsPos;

    ScaleControlPointsEvent(m_ScaleReferencePoint, 1, wsDiff.y() / norm.y());
  }

  if (m_State == EditState::DraggingCurve)
  {
    cursor = Qt::SizeVerCursor;
    Q_EMIT MoveCurveEvent(m_iDraggedCurve, moveY);
  }

  setCursor(cursor);
  m_LastMousePos = e->pos();
}

void plQtCurveEditWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
  QWidget::mouseDoubleClickEvent(e);

  if (e->button() == Qt::LeftButton)
  {
    plSelectedCurveCP cp;
    if (PickCpAt(e->pos(), 15, cp))
    {
      SetSelection(cp);
    }
    else
    {
      const QPointF epsilon = MapToScene(QPoint(15, 15)) - MapToScene(QPoint(0, 0));
      const QPointF scenePos = MapToScene(e->pos());

      if (m_bBegunChanges)
      {
        m_bBegunChanges = false;
        Q_EMIT EndOperationEvent(true);
      }

      Q_EMIT DoubleClickEvent(scenePos, epsilon);
    }
  }
}

void plQtCurveEditWidget::wheelEvent(QWheelEvent* e)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  const QPointF ptAt = MapToScene(mapFromGlobal(e->globalPosition().toPoint()));
#else
  const QPointF ptAt = MapToScene(mapFromGlobal(e->globalPos()));
#endif
  QPointF posDiff = m_SceneTranslation - ptAt;

  double changeX = 1.2;
  double changeY = 1.2;

  if (e->modifiers().testFlag(Qt::ShiftModifier))
    changeX = 1;
  if (e->modifiers().testFlag(Qt::ControlModifier))
    changeY = 1;

  const double oldScaleX = m_SceneToPixelScale.x();
  const double oldScaleY = m_SceneToPixelScale.y();

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  if (e->angleDelta().y() > 0)
#else
  if (e->delta() > 0)
#endif
  {

    m_SceneToPixelScale.setX(m_SceneToPixelScale.x() * changeX);
    m_SceneToPixelScale.setY(m_SceneToPixelScale.y() * changeY);
  }
  else
  {
    m_SceneToPixelScale.setX(m_SceneToPixelScale.x() * (1.0 / changeX));
    m_SceneToPixelScale.setY(m_SceneToPixelScale.y() * (1.0 / changeY));
  }

  ClampZoomPan();

  changeX = m_SceneToPixelScale.x() / oldScaleX;
  changeY = m_SceneToPixelScale.y() / oldScaleY;

  posDiff.setX(posDiff.x() * (1.0 / changeX));
  posDiff.setY(posDiff.y() * (1.0 / changeY));

  m_SceneTranslation = ptAt + posDiff;

  ClampZoomPan();

  update();
}

void plQtCurveEditWidget::keyPressEvent(QKeyEvent* e)
{
  QWidget::keyPressEvent(e);

  if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_F)
  {
    e->accept();
    FrameCurve();
  }
  else if (e->modifiers() == Qt::ShiftModifier && e->key() == Qt::Key_F)
  {
    e->accept();
    FrameSelection();
  }
  else if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_A)
  {
    e->accept();
    SelectAll();
  }

  if (e->modifiers() == Qt::NoModifier)
  {
    if (e->key() == Qt::Key_Escape)
    {
      e->accept();
      ClearSelection();
    }

    if (e->key() == Qt::Key_Delete)
    {
      e->accept();
      Q_EMIT DeleteControlPointsEvent();
    }
  }
}

void plQtCurveEditWidget::PaintCurveSegments(QPainter* painter, float fOffsetX, plUInt8 alpha) const
{
  if (MapFromScene(QPointF(fOffsetX, 0)).x() >= rect().width())
    return;

  painter->save();
  painter->setBrush(Qt::NoBrush);

  QPen pen;
  pen.setCosmetic(true);
  pen.setStyle(Qt::PenStyle::SolidLine);

  const bool bRenderRealCurve = true;
  const bool bRenderLinearCurve = false;

  for (plUInt32 curveIdx = 0; curveIdx < m_CurvesSorted.GetCount(); ++curveIdx)
  {
    const plCurve1D& curve = m_CurvesSorted[curveIdx];
    const plUInt32 numCps = curve.GetNumControlPoints();

    if (numCps == 0)
      continue;

    const plColorGammaUB curveColor = m_pCurveEditData->m_Curves[curveIdx]->m_CurveColor;

    pen.setColor(QColor(curveColor.r, curveColor.g, curveColor.b, alpha));

    if (bRenderRealCurve)
    {
      QPainterPath path;

      pen.setStyle(Qt::PenStyle::SolidLine);
      painter->setPen(pen);

      // line from zero to first cp
      {
        const plCurve1D::ControlPoint& cp = curve.GetControlPoint(0);
        path.moveTo(MapFromScene(QPointF(fOffsetX + m_fMinExtentValue, cp.m_Position.y)));

        if (cp.m_Position.x > m_fMinExtentValue)
        {
          path.lineTo(MapFromScene(QPointF(fOffsetX + cp.m_Position.x, cp.m_Position.y)));
        }
      }

      for (plUInt32 cpIdx = 1; cpIdx < numCps; ++cpIdx)
      {
        const plCurve1D::ControlPoint& cpPrev = curve.GetControlPoint(cpIdx - 1);
        const plCurve1D::ControlPoint& cpThis = curve.GetControlPoint(cpIdx);

        const QPointF startPt = QPointF(fOffsetX + cpPrev.m_Position.x, cpPrev.m_Position.y);
        const QPointF endPt = QPointF(fOffsetX + cpThis.m_Position.x, cpThis.m_Position.y);
        const QPointF tangent1 = QPointF(cpPrev.m_RightTangent.x, cpPrev.m_RightTangent.y);
        const QPointF tangent2 = QPointF(cpThis.m_LeftTangent.x, cpThis.m_LeftTangent.y);
        const QPointF ctrlPt1 = startPt + tangent1;
        const QPointF ctrlPt2 = endPt + tangent2;

        path.moveTo(MapFromScene(startPt));
        path.cubicTo(MapFromScene(ctrlPt1), MapFromScene(ctrlPt2), MapFromScene(endPt));
      }

      // line from last cp to end
      {
        const plCurve1D::ControlPoint& cp = curve.GetControlPoint(numCps - 1);

        if (cp.m_Position.x < m_fMaxExtentValue)
        {
          path.lineTo(MapFromScene(QPointF(fOffsetX + m_fMaxExtentValue, cp.m_Position.y)));
        }
      }

      painter->drawPath(path);
    }

    if (bRenderLinearCurve)
    {
      QPainterPath path;

      pen.setStyle(Qt::PenStyle::DashLine);
      painter->setPen(pen);

      plCurve1D linearCurve;
      m_pCurveEditData->m_Curves[curveIdx]->ConvertToRuntimeData(linearCurve);
      linearCurve.SortControlPoints();
      linearCurve.ApplyTangentModes();
      linearCurve.CreateLinearApproximation();
      const auto& linear = linearCurve.GetLinearApproximation();

      path.moveTo(MapFromScene(QPointF(fOffsetX + linear[0].x, linear[0].y)));

      for (plUInt32 i = 1; i < linear.GetCount(); ++i)
      {
        path.lineTo(MapFromScene(QPointF(fOffsetX + linear[i].x, linear[i].y)));
      }

      painter->drawPath(path);
    }
  }

  painter->restore();
}

void plQtCurveEditWidget::PaintOutsideAreaOverlay(QPainter* painter) const
{
  const int iLeftEdge = MapFromScene(QPointF(m_fMinExtentValue, 0)).x();
  const int iRightEdge = MapFromScene(QPointF(m_fMaxExtentValue, 0)).x();

  if (iLeftEdge > 0)
  {
    QRect area = rect();
    area.setRight(iLeftEdge);

    QBrush b;
    b.setColor(palette().light().color());
    b.setStyle(Qt::BrushStyle::Dense6Pattern);

    painter->setPen(Qt::NoPen);
    painter->setBrush(b);
    painter->drawRect(area);
  }

  if (iRightEdge < rect().width())
  {
    QRect area = rect();
    area.setLeft(iRightEdge);

    QBrush b;
    b.setColor(palette().light().color());
    b.setStyle(Qt::BrushStyle::Dense6Pattern);

    painter->setPen(Qt::NoPen);
    painter->setBrush(b);
    painter->drawRect(area);
  }
}

void plQtCurveEditWidget::PaintControlPoints(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_ControlPointBrush);
  painter->setPen(Qt::NoPen);

  for (plUInt32 curveIdx = 0; curveIdx < m_Curves.GetCount(); ++curveIdx)
  {
    const plCurve1D& curve = m_Curves[curveIdx];

    const plUInt32 numCps = curve.GetNumControlPoints();
    for (plUInt32 cpIdx = 0; cpIdx < numCps; ++cpIdx)
    {
      const plCurve1D::ControlPoint& cp = curve.GetControlPoint(cpIdx);

      const QPointF ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));

      painter->drawEllipse(ptPos, 3.5, 3.5);
    }
  }

  painter->restore();
}

void plQtCurveEditWidget::PaintSelectedControlPoints(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_SelectedControlPointBrush);
  painter->setPen(Qt::NoPen);

  for (const auto& cpSel : m_SelectedCPs)
  {
    const plCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const plCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    const QPointF ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));

    painter->drawEllipse(ptPos, 4.5, 4.5);
  }

  painter->restore();
}

void plQtCurveEditWidget::PaintSelectedTangentLines(QPainter* painter) const
{
  painter->save();
  painter->setBrush(Qt::NoBrush);
  painter->setPen(m_TangentLinePen);

  plHybridArray<QLine, 50> lines;

  for (const auto& cpSel : m_SelectedCPs)
  {
    const plCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const plCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    plVec2d leftHandlePos = cp.m_Position + plVec2d(cp.m_LeftTangent.x, cp.m_LeftTangent.y);
    plVec2d rightHandlePos = cp.m_Position + plVec2d(cp.m_RightTangent.x, cp.m_RightTangent.y);

    const QPoint ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));
    const QPoint ptPosLeft = MapFromScene(QPointF(leftHandlePos.x, leftHandlePos.y));
    const QPoint ptPosRight = MapFromScene(QPointF(rightHandlePos.x, rightHandlePos.y));

    bool bDrawLeft = m_CurveExtents[cpSel.m_uiCurve].x != cp.m_Position.x;
    bool bDrawRight = m_CurveExtents[cpSel.m_uiCurve].y != cp.m_Position.x;

    const plCurveTangentMode::Enum tmLeft = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_LeftTangentMode;
    const plCurveTangentMode::Enum tmRight = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_RightTangentMode;

    if (bDrawLeft && tmLeft != plCurveTangentMode::Linear && tmLeft != plCurveTangentMode::Auto)
    {
      QLine& l1 = lines.ExpandAndGetRef();
      l1.setLine(ptPos.x(), ptPos.y(), ptPosLeft.x(), ptPosLeft.y());
    }

    if (bDrawRight && tmRight != plCurveTangentMode::Linear && tmRight != plCurveTangentMode::Auto)
    {
      QLine& l2 = lines.ExpandAndGetRef();
      l2.setLine(ptPos.x(), ptPos.y(), ptPosRight.x(), ptPosRight.y());
    }
  }

  painter->drawLines(lines.GetData(), lines.GetCount());

  painter->restore();
}

void plQtCurveEditWidget::PaintSelectedTangentHandles(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_TangentHandleBrush);
  painter->setPen(Qt::NoPen);

  for (const auto& cpSel : m_SelectedCPs)
  {
    const plCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const plCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    const plCurveTangentMode::Enum tmLeft = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_LeftTangentMode;
    const plCurveTangentMode::Enum tmRight = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint].m_RightTangentMode;

    const bool bDrawLeft = m_CurveExtents[cpSel.m_uiCurve].x != cp.m_Position.x;
    const bool bDrawRight = m_CurveExtents[cpSel.m_uiCurve].y != cp.m_Position.x;

    if (bDrawLeft && tmLeft != plCurveTangentMode::Linear && tmLeft != plCurveTangentMode::Auto)
    {
      if (tmLeft == plCurveTangentMode::Bezier)
      {
        const plVec2d leftHandlePos = cp.m_Position + plVec2d(cp.m_LeftTangent.x, cp.m_LeftTangent.y);
        const QPointF ptPosLeft = MapFromScene(QPointF(leftHandlePos.x, leftHandlePos.y));
        painter->drawRect(QRectF(ptPosLeft.x() - 4.5, ptPosLeft.y() - 4.5, 9, 9));
      }
      else
      {
        const plVec2d leftHandlePos = cp.m_Position + plVec2d(cp.m_LeftTangent.x, cp.m_LeftTangent.y);
        const QPointF ptPosLeft = MapFromScene(QPointF(leftHandlePos.x, leftHandlePos.y));
        // const plVec2 dir = MapDirFromScene(cp.m_LeftTangent).GetNormalized() * 50.0f;
        // const QPointF ptPosLeft = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y)) + QPointF(dir.x, dir.y);
        painter->drawEllipse(QPointF(ptPosLeft.x(), ptPosLeft.y()), 3.5, 3.5);
      }
    }

    if (bDrawRight && tmRight != plCurveTangentMode::Linear && tmRight != plCurveTangentMode::Auto)
    {
      if (tmRight == plCurveTangentMode::Bezier)
      {
        const plVec2d rightHandlePos = cp.m_Position + plVec2d(cp.m_RightTangent.x, cp.m_RightTangent.y);
        const QPointF ptPosRight = MapFromScene(QPointF(rightHandlePos.x, rightHandlePos.y));
        painter->drawRect(QRectF(ptPosRight.x() - 4.5, ptPosRight.y() - 4.5, 9, 9));
      }
      else
      {
        const plVec2d rightHandlePos = cp.m_Position + plVec2d(cp.m_RightTangent.x, cp.m_RightTangent.y);
        const QPointF ptPosRight = MapFromScene(QPointF(rightHandlePos.x, rightHandlePos.y));
        // const plVec2 dir = MapDirFromScene(cp.m_RightTangent).GetNormalized() * 50.0f;
        // ptPosRight = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y)) + QPointF(dir.x, dir.y);
        painter->drawEllipse(QPointF(ptPosRight.x(), ptPosRight.y()), 3.5, 3.5);
      }
    }
  }

  painter->restore();
}

void plQtCurveEditWidget::PaintMultiSelectionSquare(QPainter* painter) const
{
  if (m_SelectionBRect.isEmpty())
    return;

  painter->save();
  painter->setPen(Qt::NoPen);

  QColor col = palette().highlight().color();
  col.setAlpha(100);
  painter->setBrush(col);

  const QPoint tl = MapFromScene(m_SelectionBRect.topLeft());
  const QPoint br = MapFromScene(m_SelectionBRect.bottomRight());
  QRectF r = QRect(tl, br);
  r.adjust(-4.5, +4.5, +3.5, -5.5);

  painter->drawRect(r);

  col.setAlpha(255);
  QPen pen(col);
  pen.setStyle(Qt::PenStyle::SolidLine);
  pen.setCosmetic(true);
  pen.setWidth(1);
  pen.setCapStyle(Qt::PenCapStyle::SquareCap);
  painter->setPen(pen);

  painter->drawLine(tl.x() - 10, tl.y(), tl.x() - 10, br.y());
  painter->drawLine(br.x() + 10, tl.y(), br.x() + 10, br.y());
  painter->drawLine(tl.x(), br.y() - 10, br.x(), br.y() - 10);
  painter->drawLine(tl.x(), tl.y() + 10, br.x(), tl.y() + 10);

  painter->restore();
}

void plQtCurveEditWidget::PaintScrubber(QPainter& p) const
{
  if (!m_bShowScrubber)
    return;

  const QRect area = rect();

  const plInt32 xPos = MapFromScene(plVec2d(m_fScrubberPosition, 0)).x();
  if (xPos < 0 || xPos > area.width())
    return;

  p.save();

  QPen pen;
  pen.setCosmetic(true);
  pen.setColor(palette().highlight().color());
  pen.setWidth(1);

  p.setPen(pen);
  p.drawLine(QLine(xPos, area.top(), xPos, area.bottom()));

  p.restore();
}

void plQtCurveEditWidget::RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity)
{
  double lowX, highX;
  plWidgetUtils::ComputeGridExtentsX(viewportSceneRect, fRoughGridDensity, lowX, highX);
  lowX = plMath::Max(lowX, 0.0);

  const int iy = rect().bottom();

  // render grid lines
  {
    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    plHybridArray<QLine, 100> lines;

    for (double x = lowX; x <= highX; x += fRoughGridDensity)
    {
      const int ix = MapFromScene(QPointF(x, x)).x();

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(ix, 0, ix, iy);
    }

    // zero line
    {
      const QPoint x0 = MapFromScene(QPointF(lowX, 0));
      const QPoint x1 = MapFromScene(QPointF(highX, 0));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, x0.y(), x1.x(), x1.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }
}

void plQtCurveEditWidget::RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect)
{
  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  plWidgetUtils::AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().height(), plMath::Abs(viewportSceneRect.height()), 20);

  painter->save();

  const plInt32 iFineLineLength = 10;
  const plInt32 iRoughLineLength = 20;

  QRect areaRect = rect();
  areaRect.setRight(areaRect.left() + 30);

  // render fine grid stop lines
  {
    double lowY, highY;
    plWidgetUtils::ComputeGridExtentsY(viewportSceneRect, fFineGridDensity, lowY, highY);

    if (lowY > highY)
      plMath::Swap(lowY, highY);

    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    plHybridArray<QLine, 100> lines;

    for (double y = lowY; y <= highY; y += fFineGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, pos.y(), iFineLineLength, pos.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // render rough grid stop lines
  {
    double lowY, highY;
    plWidgetUtils::ComputeGridExtentsY(viewportSceneRect, fRoughGridDensity, lowY, highY);

    if (lowY > highY)
      plMath::Swap(lowY, highY);

    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    plHybridArray<QLine, 100> lines;

    for (double y = lowY; y <= highY; y += fRoughGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, pos.y(), iRoughLineLength, pos.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // Grid Stop Value Text
  {
    double lowY, highY;
    plWidgetUtils::ComputeGridExtentsY(viewportSceneRect, fRoughGridDensity, lowY, highY);

    if (lowY > highY)
      plMath::Swap(lowY, highY);

    QTextOption textOpt(Qt::AlignCenter);
    QRectF textRect;

    painter->setPen(palette().buttonText().color());

    plStringBuilder tmp;

    for (double y = lowY; y <= highY; y += fRoughGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      textRect.setRect(0, pos.y() - 15, areaRect.width(), 15);
      tmp.Format("{0}", plArgF(y));

      painter->drawText(textRect, tmp.GetData(), textOpt);
    }
  }

  painter->restore();
}

void plQtCurveEditWidget::RenderValueRanges(QPainter* painter)
{
  const int iUpperEdge = MapFromScene(QPointF(0, m_fUpperRange)).y();
  const int iLowerEdge = MapFromScene(QPointF(0, m_fLowerRange)).y();

  QPen p(QColor(255, 200, 0, 200));
  p.setStyle(Qt::PenStyle::DashLine);
  p.setWidth(1);

  painter->setPen(p);
  painter->drawLine(rect().left(), iUpperEdge, rect().right(), iUpperEdge);
  painter->drawLine(rect().left(), iLowerEdge, rect().right(), iLowerEdge);
}

bool plQtCurveEditWidget::PickCpAt(const QPoint& pos, float fMaxPixelDistance, plSelectedCurveCP& out_Result) const
{
  const plVec2 at((float)pos.x(), (float)pos.y());
  float fMaxDistSqr = plMath::Square(fMaxPixelDistance);

  out_Result.m_uiCurve = 0xFFFF;

  for (plUInt32 uiCurve = 0; uiCurve < m_Curves.GetCount(); ++uiCurve)
  {
    const plCurve1D& curve = m_Curves[uiCurve];

    for (plUInt32 uiCP = 0; uiCP < curve.GetNumControlPoints(); ++uiCP)
    {
      const auto& cp = curve.GetControlPoint(uiCP);

      const QPoint diff = MapFromScene(cp.m_Position) - pos;
      const plVec2 fDiff(diff.x(), diff.y());

      const float fDistSqr = fDiff.GetLengthSquared();
      if (fDistSqr <= fMaxDistSqr)
      {
        fMaxDistSqr = fDistSqr;
        out_Result.m_uiCurve = uiCurve;
        out_Result.m_uiPoint = uiCP;
      }
    }
  }

  return out_Result.m_uiCurve != 0xFFFF;
}

static inline plVec2d ToVec(const QPoint& pt)
{
  return plVec2d(pt.x(), pt.y());
}

plQtCurveEditWidget::ClickTarget plQtCurveEditWidget::DetectClickTarget(const QPoint& pos)
{
  const plVec2d vScreenPos(pos.x(), pos.y());
  float fMinDistSQR = plMath::Square(15);
  plInt32 iBestCurve = -1;
  plInt32 iBestCP = -1;
  plInt32 iBestComp = -1;

  for (plUInt32 i = 0; i < m_SelectedCPs.GetCount(); ++i)
  {
    const auto& cpSel = m_SelectedCPs[i];
    const auto& cp = m_pCurveEditData->m_Curves[cpSel.m_uiCurve]->m_ControlPoints[cpSel.m_uiPoint];

    const plVec2d point(cp.GetTickAsTime().GetSeconds(), cp.m_fValue);

    const plVec2d ptPos = ToVec(MapFromScene(point));
    plVec2d ptLeft;
    plVec2d ptRight;

    // if (cp.m_LeftTangentMode == plCurveTangentMode::Bplier)
    ptLeft = ToVec(MapFromScene(point + plVec2d(cp.m_LeftTangent.x, cp.m_LeftTangent.y)));
    // else
    // ptLeft = ToVec(MapFromScene(cp.m_Point)) + MapDirFromScene(cp.m_LeftTangent).GetNormalized() * 50.0f;

    // if (cp.m_RightTangentMode == plCurveTangentMode::Bplier)
    ptRight = ToVec(MapFromScene(point + plVec2d(cp.m_RightTangent.x, cp.m_RightTangent.y)));
    // else
    // ptRight = ToVec(MapFromScene(cp.m_Point)) + MapDirFromScene(cp.m_RightTangent).GetNormalized() * 50.0f;

    {
      const float distSQR = (ptPos - vScreenPos).GetLengthSquared();
      if (distSQR < fMinDistSQR)
      {
        fMinDistSQR = distSQR;
        iBestCurve = cpSel.m_uiCurve;
        iBestCP = cpSel.m_uiPoint;
        iBestComp = 0;
      }
    }
    {
      const float distSQR = (ptLeft - vScreenPos).GetLengthSquared();
      if (distSQR < fMinDistSQR)
      {
        fMinDistSQR = distSQR;
        iBestCurve = cpSel.m_uiCurve;
        iBestCP = cpSel.m_uiPoint;
        iBestComp = 1;
      }
    }
    {
      const float distSQR = (ptRight - vScreenPos).GetLengthSquared();
      if (distSQR < fMinDistSQR)
      {
        fMinDistSQR = distSQR;
        iBestCurve = cpSel.m_uiCurve;
        iBestCP = cpSel.m_uiPoint;
        iBestComp = 2;
      }
    }
  }

  m_iSelectedTangentCurve = -1;
  m_iSelectedTangentPoint = -1;
  m_bSelectedTangentLeft = false;

  if (iBestComp > 0)
  {
    m_iSelectedTangentCurve = iBestCurve;
    m_iSelectedTangentPoint = iBestCP;
    m_bSelectedTangentLeft = (iBestComp == 1);

    return ClickTarget::TangentHandle;
  }

  if (iBestComp == 0)
    return ClickTarget::SelectedPoint;

  return ClickTarget::Nothing;
}

void plQtCurveEditWidget::ExecMultiSelection(plDynamicArray<plSelectedCurveCP>& out_Selection)
{
  out_Selection.Clear();

  for (plUInt32 uiCurve = 0; uiCurve < m_Curves.GetCount(); ++uiCurve)
  {
    const plCurve1D& curve = m_Curves[uiCurve];

    for (plUInt32 uiCP = 0; uiCP < curve.GetNumControlPoints(); ++uiCP)
    {
      const auto& cp = curve.GetControlPoint(uiCP);

      const QPoint cpPos = MapFromScene(cp.m_Position);

      if (m_MultiSelectRect.contains(cpPos))
      {
        auto& sel = out_Selection.ExpandAndGetRef();
        sel.m_uiCurve = uiCurve;
        sel.m_uiPoint = uiCP;
      }
    }
  }
}

bool plQtCurveEditWidget::CombineSelectionAdd(plDynamicArray<plSelectedCurveCP>& inout_Selection, const plDynamicArray<plSelectedCurveCP>& change)
{
  bool bChange = false;

  for (plUInt32 i = 0; i < change.GetCount(); ++i)
  {
    const auto& cp = change[i];

    if (!inout_Selection.Contains(cp))
    {
      inout_Selection.PushBack(cp);
      bChange = true;
    }
  }

  return bChange;
}

bool plQtCurveEditWidget::CombineSelectionRemove(plDynamicArray<plSelectedCurveCP>& inout_Selection, const plDynamicArray<plSelectedCurveCP>& change)
{
  bool bChange = false;

  for (plUInt32 i = 0; i < change.GetCount(); ++i)
  {
    const auto& cp = change[i];

    bChange |= inout_Selection.RemoveAndCopy(cp);
  }

  return bChange;
}

bool plQtCurveEditWidget::CombineSelectionToggle(plDynamicArray<plSelectedCurveCP>& inout_Selection, const plDynamicArray<plSelectedCurveCP>& change)
{
  bool bChange = false;

  for (plUInt32 i = 0; i < change.GetCount(); ++i)
  {
    const auto& cp = change[i];

    if (!inout_Selection.Contains(cp))
    {
      inout_Selection.PushBack(cp);
      bChange = true;
    }
    else
    {
      bChange |= inout_Selection.RemoveAndCopy(cp);
    }
  }

  return bChange;
}

void plQtCurveEditWidget::ComputeSelectionRect()
{
  m_SelectionBRect = QRectF();

  if (m_SelectedCPs.GetCount() < 2)
    return;

  plBoundingBox bbox;
  bbox = plBoundingBox::MakeInvalid();

  for (const auto& cpSel : m_SelectedCPs)
  {
    const plCurve1D& curve = m_Curves[cpSel.m_uiCurve];
    const plCurve1D::ControlPoint& cp = curve.GetControlPoint(cpSel.m_uiPoint);

    bbox.ExpandToInclude(plVec3(cp.m_Position.x, cp.m_Position.y, cp.m_Position.x));
  }

  if (bbox.IsValid())
  {
    m_SelectionBRect.setCoords(bbox.m_vMin.x, bbox.m_vMin.y, bbox.m_vMax.x, bbox.m_vMax.y);
    m_SelectionBRect = m_SelectionBRect.normalized();
  }
}

plQtCurveEditWidget::SelectArea plQtCurveEditWidget::WhereIsPoint(QPoint pos) const
{
  if (m_SelectionBRect.isEmpty())
    return SelectArea::None;

  const QPoint tl = MapFromScene(m_SelectionBRect.topLeft());
  const QPoint br = MapFromScene(m_SelectionBRect.bottomRight());
  QRect selectionRectSS = QRect(tl, br);
  selectionRectSS.adjust(-4, +4, +3, -5);

  const QRect barTop(selectionRectSS.left(), selectionRectSS.bottom() - 10, selectionRectSS.width(), 10);
  const QRect barBottom(selectionRectSS.left(), selectionRectSS.top(), selectionRectSS.width(), 10);
  const QRect barLeft(selectionRectSS.left() - 10, selectionRectSS.top(), 10, selectionRectSS.height());
  const QRect barRight(selectionRectSS.right(), selectionRectSS.top(), 10, selectionRectSS.height());

  if (barTop.contains(pos))
    return SelectArea::Top;

  if (barBottom.contains(pos))
    return SelectArea::Bottom;

  if (barLeft.contains(pos))
    return SelectArea::Left;

  if (barRight.contains(pos))
    return SelectArea::Right;

  if (selectionRectSS.contains(pos))
    return SelectArea::Center;

  return SelectArea::None;
}

plInt32 plQtCurveEditWidget::PickCurveAt(QPoint pos) const
{
  const QPointF scenePos = MapToScene(pos);
  const float x = scenePos.x();

  plInt32 iCurveIdx = -1;
  plInt32 iMinDistance = 15;

  for (plUInt32 i = 0; i < m_CurvesSorted.GetCount(); ++i)
  {
    double minVal, maxVal;
    m_CurvesSorted[i].QueryExtents(minVal, maxVal);

    if (x < minVal || x > maxVal)
      continue;

    const float val = m_CurvesSorted[i].Evaluate(x);
    const QPoint pixelPos = MapFromScene(QPointF(x, val));

    const plInt32 dist = plMath::Abs(pixelPos.y() - pos.y());
    if (dist < iMinDistance)
    {
      iMinDistance = dist;
      iCurveIdx = i;
    }
  }

  return iCurveIdx;
}
