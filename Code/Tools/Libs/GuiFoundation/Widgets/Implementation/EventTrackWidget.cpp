#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <GuiFoundation/Widgets/EventTrackWidget.moc.h>
#include <GuiFoundation/Widgets/GridBarWidget.moc.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <QPainter>
#include <QRubberBand>
#include <qevent.h>

plQtEventTrackWidget::plQtEventTrackWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setFocusPolicy(Qt::FocusPolicy::ClickFocus);
  setMouseTracking(true);

  m_fSceneTranslationX = -2;
  m_SceneToPixelScale = QPointF(1, -1);

  m_ControlPointBrush.setColor(QColor(200, 150, 0));
  m_ControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);

  m_ControlPointPen.setStyle(Qt::PenStyle::DashLine);
  m_ControlPointPen.setCosmetic(true);

  m_SelectedControlPointBrush.setColor(QColor(220, 200, 50));
  m_SelectedControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);
}


void plQtEventTrackWidget::SetData(const plEventTrackData* pData, double fMinCurveLength)
{
  m_pEditData = pData;
  m_fMaxCurveExtent = fMinCurveLength;

  RecreateSortedData();

  //  // make sure the selection does not contain points that got deleted
  //  for (plUInt32 i = 0; i < m_SelectedCPs.GetCount(); )
  //  {
  //    if (m_SelectedCPs[i].m_uiCurve >= m_Curves.GetCount() ||
  //      m_SelectedCPs[i].m_uiPoint >= m_Curves[m_SelectedCPs[i].m_uiCurve].GetNumControlPoints())
  //    {
  //      m_SelectedCPs.RemoveAtAndCopy(i);
  //    }
  //    else
  //    {
  //      ++i;
  //    }
  //  }

  ComputeSelectionRect();

  update();
}

void plQtEventTrackWidget::RecreateSortedData()
{
  for (auto& cat : m_Categories)
  {
    cat.m_SortedPoints.Clear();
  }

  for (plUInt32 idx = 0; idx < m_pEditData->m_ControlPoints.GetCount(); ++idx)
  {
    const auto& pt = m_pEditData->m_ControlPoints[idx];

    plUInt32 uiCategory = 0;
    if (!m_NameToCategory.TryGetValue(pt.m_sEvent, uiCategory))
    {
      uiCategory = m_Categories.GetCount();
      m_NameToCategory[pt.m_sEvent] = uiCategory;
      m_Categories.ExpandAndGetRef().m_sName = pt.m_sEvent;
    }

    auto& npt = m_Categories[uiCategory].m_SortedPoints.ExpandAndGetRef();

    npt.m_uiOrgIndex = idx;
    npt.m_fPosX = pt.GetTickAsTime().GetSeconds();
  }

  // actually this is not needed (or used)
  // and when we sort, the selection index does not match when moving points around

  // sort points by X position
  // for (auto& cat : m_Categories)
  //{
  //  cat.m_SortedPoints.Sort([](const plQtEventTrackWidget::Point& lhs, const plQtEventTrackWidget::Point& rhs) -> bool
  //  {
  //    return lhs.m_fPosX < rhs.m_fPosX;
  //  });
  //}
}

void plQtEventTrackWidget::SetScrubberPosition(double fPosition)
{
  m_bShowScrubber = true;
  m_fScrubberPosition = fPosition;

  update();
}

void plQtEventTrackWidget::FrameCurve()
{
  m_bFrameBeforePaint = false;

  double fWidth = m_fMaxCurveExtent;
  double fOffsetX = 0;

  if (m_pEditData->m_ControlPoints.GetCount() == 0)
  {
    fWidth = 10.0;
  }
  else if (m_SelectedPoints.GetCount() > 1)
  {
    double minX = m_Categories[m_SelectedPoints[0].m_uiCategory].m_SortedPoints[m_SelectedPoints[0].m_uiSortedIdx].m_fPosX;
    double maxX = minX;

    for (const auto& cpSel : m_SelectedPoints)
    {
      const double pos = m_Categories[cpSel.m_uiCategory].m_SortedPoints[cpSel.m_uiSortedIdx].m_fPosX;
      minX = plMath::Min(minX, pos);
      maxX = plMath::Max(maxX, pos);
    }

    // fWidth = m_selectionBRect.width();
    // fOffsetX = m_selectionBRect.left();
    fWidth = maxX - minX;
    fOffsetX = minX;
  }
  else if (m_SelectedPoints.GetCount() == 1)
  {
    fWidth = 0.1f;

    fOffsetX = m_Categories[m_SelectedPoints[0].m_uiCategory].m_SortedPoints[m_SelectedPoints[0].m_uiSortedIdx].m_fPosX - 0.05;
  }

  fWidth = plMath::Max(fWidth, 0.1);

  const double fFinalWidth = fWidth * 1.2;

  fOffsetX -= (fFinalWidth - fWidth) * 0.5;

  m_SceneToPixelScale.setX(rect().width() / fFinalWidth);
  m_fSceneTranslationX = fOffsetX;

  ClampZoomPan();

  update();
}

QPoint plQtEventTrackWidget::MapFromScene(const QPointF& pos) const
{
  double x = pos.x() - m_fSceneTranslationX;
  double y = pos.y();
  x *= m_SceneToPixelScale.x();
  y *= m_SceneToPixelScale.y();

  return QPoint((int)x, (int)y);
}

QPointF plQtEventTrackWidget::MapToScene(const QPoint& pos) const
{
  double x = pos.x();
  double y = pos.y();
  x /= m_SceneToPixelScale.x();
  y /= m_SceneToPixelScale.y();

  return QPointF(x + m_fSceneTranslationX, y);
}

void plQtEventTrackWidget::ClearSelection()
{
  m_SelectionBRect = QRectF();

  if (!m_SelectedPoints.IsEmpty())
  {
    for (const auto& cpSel : m_SelectedPoints)
    {
      m_Categories[cpSel.m_uiCategory].m_SortedPoints[cpSel.m_uiSortedIdx].m_bSelected = false;
    }

    m_SelectedPoints.Clear();
    update();
  }

  Q_EMIT SelectionChangedEvent();
}


void plQtEventTrackWidget::GetSelection(plHybridArray<plUInt32, 32>& out_selection) const
{
  out_selection.Clear();

  for (const auto& pt : m_SelectedPoints)
  {
    out_selection.PushBack(m_Categories[pt.m_uiCategory].m_SortedPoints[pt.m_uiSortedIdx].m_uiOrgIndex);
  }
}

bool plQtEventTrackWidget::IsSelected(SelectedPoint cp) const
{
  return m_Categories[cp.m_uiCategory].m_SortedPoints[cp.m_uiSortedIdx].m_bSelected;
}

void plQtEventTrackWidget::SetSelection(SelectedPoint cp)
{
  ClearSelection();

  m_SelectedPoints.PushBack(cp);

  m_Categories[cp.m_uiCategory].m_SortedPoints[cp.m_uiSortedIdx].m_bSelected = true;

  ComputeSelectionRect();

  Q_EMIT SelectionChangedEvent();
}

void plQtEventTrackWidget::SetSelection(const plHybridArray<SelectedPoint, 32>& selection)
{
  for (const auto& cpSel : m_SelectedPoints)
  {
    m_Categories[cpSel.m_uiCategory].m_SortedPoints[cpSel.m_uiSortedIdx].m_bSelected = false;
  }

  m_SelectedPoints = selection;

  for (const auto& cpSel : m_SelectedPoints)
  {
    m_Categories[cpSel.m_uiCategory].m_SortedPoints[cpSel.m_uiSortedIdx].m_bSelected = true;
  }

  Q_EMIT SelectionChangedEvent();
}

void plQtEventTrackWidget::ToggleSelected(SelectedPoint cp)
{
  SetSelected(cp, !IsSelected(cp));

  ComputeSelectionRect();

  Q_EMIT SelectionChangedEvent();
}

void plQtEventTrackWidget::SetSelected(SelectedPoint cp, bool set)
{
  if (m_Categories[cp.m_uiCategory].m_SortedPoints[cp.m_uiSortedIdx].m_bSelected == set)
    return;

  m_Categories[cp.m_uiCategory].m_SortedPoints[cp.m_uiSortedIdx].m_bSelected = set;

  if (!set)
  {
    m_SelectedPoints.RemoveAndCopy(cp);
  }
  else
  {
    m_SelectedPoints.PushBack(cp);
  }

  ComputeSelectionRect();
  Q_EMIT SelectionChangedEvent();
}

QRectF plQtEventTrackWidget::ComputeViewportSceneRect() const
{
  const QPointF topLeft = MapToScene(rect().topLeft());
  const QPointF bottomRight = MapToScene(rect().bottomRight());

  return QRectF(topLeft, bottomRight);
}

static plColorGammaUB g_EventColors[10 * 3] = {
  plColorGammaUB(255, 102, 0),
  plColorGammaUB(76, 255, 0),
  plColorGammaUB(0, 255, 255),
  plColorGammaUB(239, 35, 0),
  plColorGammaUB(127, 255, 0),
  plColorGammaUB(0, 0, 255),
  plColorGammaUB(205, 92, 92),
  plColorGammaUB(120, 158, 39),
  plColorGammaUB(81, 120, 188),
  plColorGammaUB(255, 105, 180),
  plColorGammaUB(0, 250, 154),
  plColorGammaUB(0, 191, 255),
  plColorGammaUB(220, 20, 60),
  plColorGammaUB(0, 255, 127),
  plColorGammaUB(30, 144, 255),
  plColorGammaUB(240, 128, 128),
  plColorGammaUB(60, 179, 113),
  plColorGammaUB(135, 206, 250),
  plColorGammaUB(178, 34, 34),
  plColorGammaUB(46, 139, 87),
  plColorGammaUB(65, 105, 225),
  plColorGammaUB(211, 122, 122),
  plColorGammaUB(144, 238, 144),
  plColorGammaUB(135, 206, 235),
  plColorGammaUB(219, 112, 147),
  plColorGammaUB(0, 128, 0),
  plColorGammaUB(70, 130, 180),
  plColorGammaUB(255, 182, 193),
  plColorGammaUB(102, 205, 170),
  plColorGammaUB(100, 149, 237),
};

void plQtEventTrackWidget::paintEvent(QPaintEvent* e)
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
    m_pGridBar->SetConfig(viewportSceneRect, fRoughGridDensity, fFineGridDensity, [this](const QPointF& pt) -> QPoint { return MapFromScene(pt); });
  }

  PaintOutsideAreaOverlay(&painter);
  PaintControlPoints(&painter);
  PaintMultiSelectionSquare(&painter);
  PaintScrubber(painter);
}

void plQtEventTrackWidget::ClampZoomPan()
{
  m_fSceneTranslationX = plMath::Clamp(m_fSceneTranslationX, -2.0, 50000.0);
  m_SceneToPixelScale.setX(plMath::Clamp(m_SceneToPixelScale.x(), 0.0005, 10000.0));
  m_SceneToPixelScale.setY(plMath::Clamp(m_SceneToPixelScale.y(), -10000.0, -0.0005));
}

void plQtEventTrackWidget::mousePressEvent(QMouseEvent* e)
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
          case plQtEventTrackWidget::SelectArea::None:
            break;
          case plQtEventTrackWidget::SelectArea::Center:
            m_State = EditState::DraggingPoints;
            m_TotalPointDrag = QPointF();
            break;
          case plQtEventTrackWidget::SelectArea::Left:
            m_State = EditState::ScaleLeftRight;
            m_ScaleReferencePoint = m_SelectionBRect.topRight();
            break;
          case plQtEventTrackWidget::SelectArea::Right:
            m_State = EditState::ScaleLeftRight;
            m_ScaleReferencePoint = m_SelectionBRect.topLeft();
            break;
        }
      }

      if (m_State == EditState::None)
      {
        SelectedPoint selectedCP;
        if (PickCpAt(e->pos(), 8, selectedCP))
        {
          if (e->modifiers().testFlag(Qt::ControlModifier))
          {
            ToggleSelected(selectedCP);
          }
          else if (e->modifiers().testFlag(Qt::ShiftModifier))
          {
            SetSelected(selectedCP, true);
          }
          else if (e->modifiers().testFlag(Qt::AltModifier))
          {
            SetSelected(selectedCP, false);
          }
          else
          {
            if (clickedOn == ClickTarget::Nothing)
              SetSelection(selectedCP);

            m_State = EditState::DraggingPoints;
            m_TotalPointDrag = QPointF();
          }
        }
      }

      if (m_State == EditState::None)
      {
        m_State = EditState::MultiSelect;
      }

      PLASMA_ASSERT_DEBUG(!m_bBegunChanges, "Invalid State");

      if (m_State == EditState::DraggingPoints)
      {
        Q_EMIT BeginOperationEvent("Drag Points");
        m_bBegunChanges = true;
      }
      else if (m_State == EditState::ScaleLeftRight)
      {
        Q_EMIT BeginOperationEvent("Scale Points Left / Right");
        m_bBegunChanges = true;
      }

      update();
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

void plQtEventTrackWidget::mouseReleaseEvent(QMouseEvent* e)
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
      (m_State == EditState::DraggingPoints || m_State == EditState::ScaleLeftRight || m_State == EditState::MultiSelect))
  {
    m_State = EditState::None;
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
      plHybridArray<SelectedPoint, 32> newSelection = m_SelectedPoints;
      plHybridArray<SelectedPoint, 32> change;
      ExecMultiSelection(change);
      m_MultiSelectRect = QRect();

      if (e->modifiers().testFlag(Qt::AltModifier))
      {
        CombineSelection(newSelection, change, false);
      }
      else if (e->modifiers().testFlag(Qt::ShiftModifier) || e->modifiers().testFlag(Qt::ControlModifier))
      {
        CombineSelection(newSelection, change, true);
      }
      else
      {
        newSelection = change;
      }

      SetSelection(newSelection);
      ComputeSelectionRect();

      update();

      Q_EMIT SelectionChangedEvent();
    }
  }

  if (e->buttons() == Qt::NoButton)
  {
    unsetCursor();

    m_State = EditState::None;

    if (m_bBegunChanges)
    {
      m_bBegunChanges = false;
      Q_EMIT EndOperationEvent(true);
    }

    update();
  }
}

void plQtEventTrackWidget::mouseMoveEvent(QMouseEvent* e)
{
  QWidget::mouseMoveEvent(e);
  Qt::CursorShape cursor = Qt::ArrowCursor;

  const QPoint diff = e->pos() - m_LastMousePos;
  double moveX = (double)diff.x() / m_SceneToPixelScale.x();
  double moveY = 0; // (double)diff.y() / m_SceneToPixelScale.y();

  if (m_State == EditState::RightClick || m_State == EditState::Panning)
  {
    m_State = EditState::Panning;
    cursor = Qt::ClosedHandCursor;

    m_fSceneTranslationX = m_fSceneTranslationX - moveX;

    ClampZoomPan();

    update();
  }

  if (m_State == EditState::DraggingPoints)
  {
    MoveControlPointsEvent(moveX);
    m_TotalPointDrag += QPointF(moveX, moveY);
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
      case plQtEventTrackWidget::SelectArea::None:
        break;
      case plQtEventTrackWidget::SelectArea::Center:
        // cursor = Qt::SizeAllCursor;
        break;
      case plQtEventTrackWidget::SelectArea::Left:
      case plQtEventTrackWidget::SelectArea::Right:
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

    ScaleControlPointsEvent(m_ScaleReferencePoint, wsDiff.x() / norm.x());
  }

  setCursor(cursor);
  m_LastMousePos = e->pos();
}

void plQtEventTrackWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
  QWidget::mouseDoubleClickEvent(e);

  if (e->button() == Qt::LeftButton)
  {
    SelectedPoint selPt;
    if (PickCpAt(e->pos(), 15, selPt))
    {
      SetSelection(selPt);
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

      Q_EMIT DoubleClickEvent(scenePos.x(), epsilon.x());
    }
  }
}

void plQtEventTrackWidget::wheelEvent(QWheelEvent* e)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  const double ptAtX = MapToScene(mapFromGlobal(e->globalPosition().toPoint())).x();
#else
  const double ptAtX = MapToScene(mapFromGlobal(e->globalPos())).x();
#endif
  double posDiff = m_fSceneTranslationX - ptAtX;

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

  posDiff = posDiff * (1.0 / changeX);

  m_fSceneTranslationX = ptAtX + posDiff;

  ClampZoomPan();

  update();
}

void plQtEventTrackWidget::keyPressEvent(QKeyEvent* e)
{
  QWidget::keyPressEvent(e);

  if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_F)
  {
    FrameCurve();
  }

  if (e->modifiers() == Qt::NoModifier)
  {
    if (e->key() == Qt::Key_Escape)
    {
      ClearSelection();
    }

    if (e->key() == Qt::Key_Delete)
    {
      Q_EMIT DeleteControlPointsEvent();

      ClearSelection();
    }
  }
}

void plQtEventTrackWidget::PaintOutsideAreaOverlay(QPainter* painter) const
{
  const int iRightEdge = MapFromScene(QPointF(plMath::Max(0.0, m_fMaxCurveExtent), 0)).x();

  if (iRightEdge >= rect().width())
    return;

  QRect area = rect();
  area.setLeft(iRightEdge);

  QBrush b;
  b.setColor(palette().light().color());
  b.setStyle(Qt::BrushStyle::Dense6Pattern);

  painter->setPen(Qt::NoPen);
  painter->setBrush(b);
  painter->drawRect(area);
}

void plQtEventTrackWidget::PaintControlPoints(QPainter* painter) const
{
  if (m_Categories.IsEmpty())
    return;

  painter->save();

  plHybridArray<QLineF, 100> lines;
  plHybridArray<QRectF, 100> rects;
  plHybridArray<QLineF, 100> linesSelected;
  plHybridArray<QRectF, 100> rectsSelected;

  const double fRectLeft = rect().left() - 10;
  const double fRectRight = rect().right() + 10;
  const double fHeight = rect().bottom() - rect().top();
  const double fStepY = fHeight / m_Categories.GetCount();
  const double fLineStepY = fStepY / 3.5;
  double fOffsetY = rect().top() + fStepY * 0.5;

  for (plUInt32 catIdx = 0; catIdx < m_Categories.GetCount(); ++catIdx)
  {
    const PointCategory& cat = m_Categories[catIdx];

    const auto& allPoints = cat.m_SortedPoints;

    for (const auto& point : allPoints)
    {
      double ptPosX = MapFromScene(QPointF(point.m_fPosX, 0)).x();

      if (ptPosX < fRectLeft || ptPosX > fRectRight)
        continue;

      QLineF& l = point.m_bSelected ? linesSelected.ExpandAndGetRef() : lines.ExpandAndGetRef();
      l.setLine(ptPosX, fOffsetY - fLineStepY, ptPosX, fOffsetY + fLineStepY);

      QRectF& r = point.m_bSelected ? rectsSelected.ExpandAndGetRef() : rects.ExpandAndGetRef();
      r.setX(ptPosX - 5.0);
      r.setY(fOffsetY - 5.0);
      r.setWidth(10.0);
      r.setHeight(10.0);
    }

    QBrush brush = m_ControlPointBrush;
    QPen pen = m_ControlPointPen;

    if (!rects.IsEmpty())
    {
      const plColorGammaUB& col = g_EventColors[catIdx % PLASMA_ARRAY_SIZE(g_EventColors)];

      brush.setColor(qRgb(col.r, col.g, col.b));
      pen.setColor(qRgb(col.r, col.g, col.b));

      painter->setBrush(brush);
      painter->setPen(pen);

      painter->drawLines(lines.GetData(), lines.GetCount());

      painter->setPen(Qt::NoPen);
      painter->drawRects(rects.GetData(), rects.GetCount());

      lines.Clear();
      rects.Clear();
    }

    if (!rectsSelected.IsEmpty())
    {
      const QColor hlCol = palette().highlight().color();

      brush.setColor(hlCol);
      pen.setColor(hlCol);
      pen.setStyle(Qt::PenStyle::SolidLine);

      painter->setBrush(brush);
      painter->setPen(pen);

      painter->drawLines(linesSelected.GetData(), linesSelected.GetCount());
      painter->drawRects(rectsSelected.GetData(), rectsSelected.GetCount());

      linesSelected.Clear();
      rectsSelected.Clear();
    }

    fOffsetY += fStepY;
  }

  painter->restore();
}

void plQtEventTrackWidget::PaintMultiSelectionSquare(QPainter* painter) const
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
  // painter->drawLine(tl.x(), br.y() - 10, br.x(), br.y() - 10);
  // painter->drawLine(tl.x(), tl.y() + 10, br.x(), tl.y() + 10);

  painter->restore();
}

void plQtEventTrackWidget::PaintScrubber(QPainter& p) const
{
  if (!m_bShowScrubber)
    return;

  const QRect area = rect();

  const plInt32 xPos = MapFromScene(QPointF(m_fScrubberPosition, 0)).x();
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

void plQtEventTrackWidget::RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity)
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

    painter->drawLines(lines.GetData(), lines.GetCount());
  }
}

bool plQtEventTrackWidget::PickCpAt(const QPoint& pos, float fMaxPixelDistance, SelectedPoint& out_Result) const
{
  const plVec2 at((float)pos.x(), (float)pos.y());
  float fMaxDistSqr = plMath::Square(fMaxPixelDistance);

  out_Result.m_uiCategory = 0xFFFFFFFF;
  out_Result.m_uiSortedIdx = 0xFFFFFFFF;

  const double fHeight = rect().bottom() - rect().top();
  const double fStepY = fHeight / m_Categories.GetCount();
  double fOffsetY = rect().top() + fStepY * 0.5;

  for (plUInt32 catIdx = 0; catIdx < m_Categories.GetCount(); ++catIdx)
  {
    const auto& cat = m_Categories[catIdx];

    const double diffY = fOffsetY - pos.y();

    if (plMath::Abs(diffY) < 20)
    {
      const auto& allPoints = cat.m_SortedPoints;

      for (plUInt32 ptIdx = 0; ptIdx < allPoints.GetCount(); ++ptIdx)
      {
        const auto& point = allPoints[ptIdx];

        double ptPosX = MapFromScene(QPointF(point.m_fPosX, 0)).x();
        const plVec2 fDiff(ptPosX - pos.x(), diffY);

        const float fDistSqr = fDiff.GetLengthSquared();
        if (fDistSqr <= fMaxDistSqr)
        {
          fMaxDistSqr = fDistSqr;
          out_Result.m_uiCategory = catIdx;
          out_Result.m_uiSortedIdx = ptIdx;
        }
      }
    }

    fOffsetY += fStepY;
  }

  return out_Result.m_uiSortedIdx != 0xFFFFFFFF;
}

plQtEventTrackWidget::ClickTarget plQtEventTrackWidget::DetectClickTarget(const QPoint& pos)
{
  SelectedPoint ptSel;
  if (PickCpAt(pos, 15.0f, ptSel))
  {
    if (IsSelected(ptSel))
      return ClickTarget::SelectedPoint;
  }

  return ClickTarget::Nothing;
}

void plQtEventTrackWidget::ExecMultiSelection(plHybridArray<SelectedPoint, 32>& out_Selection)
{
  out_Selection.Clear();

  const double fHeight = rect().bottom() - rect().top();
  const double fStepY = fHeight / m_Categories.GetCount();
  double fOffsetY = rect().top() + fStepY * 0.5;

  for (plUInt32 catIdx = 0; catIdx < m_Categories.GetCount(); ++catIdx)
  {
    const PointCategory& cat = m_Categories[catIdx];

    const auto& allPoints = cat.m_SortedPoints;

    const int iY = (int)fOffsetY;

    for (plUInt32 ptIdx = 0; ptIdx < allPoints.GetCount(); ++ptIdx)
    {
      const auto& point = allPoints[ptIdx];

      QPoint cpPos = MapFromScene(QPointF(point.m_fPosX, 0));
      cpPos.setY(iY);

      if (m_MultiSelectRect.contains(cpPos))
      {
        auto& sel = out_Selection.ExpandAndGetRef();
        sel.m_uiCategory = catIdx;
        sel.m_uiSortedIdx = ptIdx;
      }
    }

    fOffsetY += fStepY;
  }
}

bool plQtEventTrackWidget::CombineSelection(
  plHybridArray<SelectedPoint, 32>& inout_Selection, const plHybridArray<SelectedPoint, 32>& change, bool add)
{
  bool bChange = false;

  for (plUInt32 i = 0; i < change.GetCount(); ++i)
  {
    const auto& cp = change[i];

    if (!add)
    {
      bChange |= inout_Selection.RemoveAndCopy(cp);
    }
    else if (!inout_Selection.Contains(cp))
    {
      inout_Selection.PushBack(cp);
      bChange = true;
    }
  }

  return bChange;
}

void plQtEventTrackWidget::ComputeSelectionRect()
{
  m_SelectionBRect = QRectF();

  if (m_SelectedPoints.GetCount() < 2)
    return;

  plBoundingBox bbox;
  bbox = plBoundingBox::MakeInvalid();

  // TODO: properly implement the Y value
  // for (const auto& cpSel : m_SelectedPoints)
  //{
  //  const double pos = m_Categories[cpSel.m_uiCategory].m_SortedPoints[cpSel.m_uiSortedIdx].m_fPosX;

  //  bbox.ExpandToInclude(plVec3(pos, cpSel.m_uiCategory - 1, 0));
  //  bbox.ExpandToInclude(plVec3(pos, cpSel.m_uiCategory + 1, 0));
  //}

  if (bbox.IsValid())
  {
    m_SelectionBRect.setCoords(bbox.m_vMin.x, bbox.m_vMin.y, bbox.m_vMax.x, bbox.m_vMax.y);
    m_SelectionBRect = m_SelectionBRect.normalized();
  }
}

plQtEventTrackWidget::SelectArea plQtEventTrackWidget::WhereIsPoint(QPoint pos) const
{
  if (m_SelectionBRect.isEmpty())
    return SelectArea::None;

  const QPoint tl = MapFromScene(m_SelectionBRect.topLeft());
  const QPoint br = MapFromScene(m_SelectionBRect.bottomRight());
  QRect selectionRectSS = QRect(tl, br);
  selectionRectSS.adjust(-4, +4, +3, -5);

  const QRect barLeft(selectionRectSS.left() - 10, selectionRectSS.top(), 10, selectionRectSS.height());
  const QRect barRight(selectionRectSS.right(), selectionRectSS.top(), 10, selectionRectSS.height());

  if (barLeft.contains(pos))
    return SelectArea::Left;

  if (barRight.contains(pos))
    return SelectArea::Right;

  if (selectionRectSS.contains(pos))
    return SelectArea::Center;

  return SelectArea::None;
}
