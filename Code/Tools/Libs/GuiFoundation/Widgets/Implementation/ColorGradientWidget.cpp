#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/Color8UNorm.h>
#include <GuiFoundation/Widgets/ColorGradientWidget.moc.h>
#include <QPainter>
#include <qevent.h>

static const plUInt32 CpAreaHeight = 20;
static const plUInt32 CpRadius = 5;
static const plUInt32 MaxCpPickDistance = 5;
static const plUInt32 CpRoundedCorner = 3;

plQtColorGradientWidget::plQtColorGradientWidget(QWidget* pParent)
  : QWidget(pParent)
{
  m_pColorGradientData = nullptr;

  m_fDisplayExtentMinX = 0;
  m_fDisplayExtentMaxX = 1;

  m_bShowColorCPs = false;
  m_bShowAlphaCPs = false;
  m_bShowIntensityCPs = false;
  m_bEditMode = false;
  m_bDraggingCP = false;
  m_bTempMode = false;
  m_bShowCoordsTop = false;
  m_bShowCoordsBottom = false;

  m_iSelectedColorCP = -1;
  m_iSelectedAlphaCP = -1;
  m_iSelectedIntensityCP = -1;

  // needed to get keyPress events
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);

  if (!m_bEditMode)
  {
    setCursor(Qt::PointingHandCursor);
  }

  {
    // Build grid pattern.
    const int iSize = 16;
    QImage img(iSize, iSize, QImage::Format::Format_RGBA8888);
    img.fill(Qt::white);
    QRgb halfGrayColor = qRgb(191, 191, 191);
    for (int i = 0; i < iSize / 2; ++i)
    {
      for (int j = 0; j < iSize / 2; ++j)
      {
        img.setPixel(i, j, halfGrayColor);
        img.setPixel(i + iSize / 2, j + iSize / 2, halfGrayColor);
      }
    }
    m_AlphaPattern = QPixmap::fromImage(img);
  }
}


plQtColorGradientWidget::~plQtColorGradientWidget() = default;

void plQtColorGradientWidget::SetScrubberPosition(double fPosition)
{
  m_bShowScrubber = true;
  m_fScrubberPosition = fPosition;

  update();
}

void plQtColorGradientWidget::setColorGradientData(const plColorGradient* pGradient)
{
  m_pColorGradientData = pGradient;
  FrameExtents();

  update();
}


void plQtColorGradientWidget::setEditMode(bool bEdit)
{
  m_bEditMode = bEdit;

  setMouseTracking(m_bEditMode);
  setCursor(m_bEditMode ? Qt::ArrowCursor : Qt::PointingHandCursor);
}

void plQtColorGradientWidget::setShowColorCPs(bool bShow)
{
  m_bShowColorCPs = bShow;
}


void plQtColorGradientWidget::setShowAlphaCPs(bool bShow)
{
  m_bShowAlphaCPs = bShow;
}


void plQtColorGradientWidget::setShowIntensityCPs(bool bShow)
{
  m_bShowIntensityCPs = bShow;
}


void plQtColorGradientWidget::setShowCoords(bool bTop, bool bBottom)
{
  m_bShowCoordsTop = bTop;
  m_bShowCoordsBottom = bBottom;
}

void plQtColorGradientWidget::ClearSelectedCP()
{
  SelectCP(-1, -1, -1);
}

void plQtColorGradientWidget::SelectCP(plInt32 iColorCP, plInt32 iAlphaCP, plInt32 iIntensityCP)
{
  m_bDraggingCP = false;

  if (m_bTempMode)
  {
    m_bTempMode = false;
    Q_EMIT endOperation(true);
  }

  bool changed = false;

  if (iColorCP != m_iSelectedColorCP)
  {
    m_iSelectedColorCP = iColorCP;
    changed = true;
  }

  if (iAlphaCP != m_iSelectedAlphaCP)
  {
    m_iSelectedAlphaCP = iAlphaCP;
    changed = true;
  }

  if (iIntensityCP != m_iSelectedIntensityCP)
  {
    m_iSelectedIntensityCP = iIntensityCP;
    changed = true;
  }

  if (changed)
  {
    Q_EMIT selectionChanged(m_iSelectedColorCP, m_iSelectedAlphaCP, m_iSelectedIntensityCP);
  }
}

void plQtColorGradientWidget::paintEvent(QPaintEvent* event)
{
  QWidget::paintEvent(event);

  ClampDisplayExtents();

  if (m_fDisplayExtentMinX >= m_fDisplayExtentMaxX)
  {
    m_fDisplayExtentMinX = 0.0f;
    m_fDisplayExtentMaxX = 1.0f;
  }

  QPainter p(this);

  PaintColorGradient(p);
  PaintCoordinateLines(p);
  PaintCoordinateStrips(p);
  PaintColorCpArea(p);
  PaintAlphaCpArea(p);
  PaintIntensityCpArea(p);

  PaintColorCPs(p);
  PaintAlphaCPs(p);
  PaintIntensityCPs(p);

  PaintScrubber(p);
}

void plQtColorGradientWidget::PaintColorGradient(QPainter& p) const
{
  const QRect GradientArea = GetGradientArea();

  if (m_pColorGradientData == nullptr)
  {
    QBrush whiteBrush;
    whiteBrush.setStyle(Qt::BrushStyle::Dense4Pattern);
    whiteBrush.setColor(QColor(255, 255, 255, 255));
    p.fillRect(GradientArea, whiteBrush);
    return;
  }

  plColorGradient GradientFinal;
  GradientFinal = *m_pColorGradientData;
  GradientFinal.SortControlPoints();

  p.drawTiledPixmap(GradientArea, m_AlphaPattern);

  if (m_fDisplayExtentMinX <= m_fDisplayExtentMaxX)
  {
    const double range = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;

    const plInt32 width = GradientArea.width();

    const plInt32 yTop = GradientArea.top();
    const plInt32 yOnlyAlpha = yTop + GradientArea.height() / 4;
    const plInt32 yColorDark = yOnlyAlpha + GradientArea.height() / 4;
    const plInt32 yColorTransp = yColorDark + GradientArea.height() / 4;
    const plInt32 yOnlyColorHeight =
      GradientArea.bottom() - yColorTransp; // GradientArea.height() / 4 has rounding errors, so last segment has to fill the rest

    QImage qiOnlyAlpha(width, 1, QImage::Format::Format_RGB32);
    QImage qiColorDark(width, 1, QImage::Format::Format_RGB32);
    QImage qiColorTransp(width, 1, QImage::Format::Format_ARGB32);
    QImage qiOnlyColor(width, 1, QImage::Format::Format_RGB32);

    for (plInt32 posX = 0; posX < width; ++posX)
    {
      plColorGammaUB rgba;
      float intensity;

      const double lerp = (double)posX / (double)width;
      GradientFinal.Evaluate(m_fDisplayExtentMinX + lerp * range, rgba, intensity);

      const plColor linearCol = rgba;
      const plColor linearColDark = linearCol * linearCol.a;
      const plColorGammaUB rgbaColDar = linearColDark;
      const plColorLinearUB linearAlpha(rgba.a, rgba.a, rgba.a, 255);
      const plColorGammaUB srgbAlpha = plColor(linearAlpha);

      qiOnlyAlpha.setPixel(posX, 0, qRgb(srgbAlpha.r, srgbAlpha.g, srgbAlpha.b));
      qiColorDark.setPixel(posX, 0, qRgb(rgbaColDar.r, rgbaColDar.g, rgbaColDar.b));
      qiColorTransp.setPixel(posX, 0, qRgba(rgba.r, rgba.g, rgba.b, rgba.a));
      qiOnlyColor.setPixel(posX, 0, qRgb(rgba.r, rgba.g, rgba.b));
    }

    p.drawTiledPixmap(QRect(0, yTop, width, GradientArea.height() / 4), QPixmap::fromImage(qiOnlyAlpha));
    p.drawTiledPixmap(QRect(0, yOnlyAlpha, width, GradientArea.height() / 4), QPixmap::fromImage(qiColorDark));
    p.drawTiledPixmap(QRect(0, yColorDark, width, GradientArea.height() / 4), QPixmap::fromImage(qiColorTransp));
    p.drawTiledPixmap(QRect(0, yColorTransp, width, yOnlyColorHeight), QPixmap::fromImage(qiOnlyColor));

    // Paint Lines indicating the extremes
    {
      p.save();

      double fExtentMin, fExtentMax;
      GradientFinal.GetExtents(fExtentMin, fExtentMax);

      QPen endLines;
      endLines.setColor(Qt::white);
      endLines.setStyle(Qt::PenStyle::DashLine);

      p.setCompositionMode(QPainter::CompositionMode_Difference);

      const plInt32 minPos = GradientToWindowCoord(fExtentMin);
      const plInt32 maxPos = GradientToWindowCoord(fExtentMax);

      p.setPen(endLines);
      p.drawLine(QPoint(minPos, GradientArea.top()), QPoint(minPos, GradientArea.bottom()));
      p.drawLine(QPoint(maxPos, GradientArea.top()), QPoint(maxPos, GradientArea.bottom()));

      p.restore();
    }
  }
  else
  {
    p.setPen(QColor(255, 255, 255, 255));
    p.drawRect(GradientArea);
  }
}

void plQtColorGradientWidget::PaintCpBackground(QPainter& p, const QRect& area) const
{
  QBrush bg;
  bg.setStyle(Qt::BrushStyle::SolidPattern);
  bg.setColor(QColor(150, 150, 150));

  p.fillRect(area, bg);
}

void plQtColorGradientWidget::PaintColorCpArea(QPainter& p)
{
  if (!m_bShowColorCPs)
    return;

  PaintCpBackground(p, GetColorCpArea());
}

void plQtColorGradientWidget::PaintAlphaCpArea(QPainter& p)
{
  if (!m_bShowAlphaCPs)
    return;

  PaintCpBackground(p, GetAlphaCpArea());
}

void plQtColorGradientWidget::PaintIntensityCpArea(QPainter& p)
{
  if (!m_bShowIntensityCPs)
    return;

  PaintCpBackground(p, GetIntensityCpArea());
}


void plQtColorGradientWidget::PaintCoordinateStrips(QPainter& p) const
{
  if (m_bShowCoordsTop)
    PaintCoordinateStrip(p, GetCoordAreaTop());

  if (m_bShowCoordsBottom)
    PaintCoordinateStrip(p, GetCoordAreaBottom());
}


void plQtColorGradientWidget::PaintCoordinateStrip(QPainter& p, const QRect& area) const
{
  QBrush bg;
  bg.setStyle(Qt::BrushStyle::SolidPattern);
  bg.setColor(QColor(180, 180, 180));

  p.fillRect(area, bg);

  const double fStep = ComputeCoordinateDisplayStep();

  const double fFirstStop = plMath::RoundToMultiple(m_fDisplayExtentMinX, fStep);

  QString text;
  p.setPen(QColor(0, 85, 127));

  for (double fCurStop = fFirstStop; fCurStop < m_fDisplayExtentMaxX; fCurStop += fStep)
  {
    const plInt32 xPos = GradientToWindowCoord(fCurStop);

    text.asprintf("%.2f", fCurStop);
    QRectF r(xPos - 50, area.top(), 100, area.height());
    p.drawText(r, text, QTextOption(Qt::AlignCenter));
  }
}


void plQtColorGradientWidget::PaintCoordinateLines(QPainter& p)
{
  if (!m_bEditMode)
    return;

  const double fStep = ComputeCoordinateDisplayStep();

  const QRect area = GetGradientArea();

  p.save();

  QPen endLines;
  endLines.setColor(Qt::white);
  endLines.setStyle(Qt::PenStyle::SolidLine);

  p.setCompositionMode(QPainter::CompositionMode_Difference);
  p.setPen(endLines);

  const double fFirstStop = plMath::RoundToMultiple(m_fDisplayExtentMinX, fStep);

  const plInt32 iLineHeight = area.height() / 8;

  QVarLengthArray<QLine, 100> lines;
  for (double fCurStop = fFirstStop; fCurStop < m_fDisplayExtentMaxX; fCurStop += fStep)
  {
    const plInt32 xPos = GradientToWindowCoord(fCurStop);

    lines.push_back(QLine(QPoint(xPos, area.top()), QPoint(xPos, area.top() + iLineHeight)));
    lines.push_back(QLine(QPoint(xPos, area.bottom()), QPoint(xPos, area.bottom() - iLineHeight)));
  }
  p.drawLines(lines.data(), lines.size());
  p.restore();
}

void plQtColorGradientWidget::PaintControlPoint(
  QPainter& p, const QRect& area, double posX, const plColorGammaUB& outlineColor, const plColorGammaUB& fillColor, bool selected) const
{
  const plInt32 iPosX = GradientToWindowCoord(posX);

  if (iPosX < area.left() - (plInt32)CpRadius)
    return;
  if (iPosX > area.right() + (plInt32)CpRadius)
    return;

  QColor penColor;
  penColor.setRgb(outlineColor.r, outlineColor.g, outlineColor.b);

  QColor brushColor;
  brushColor.setRgb(fillColor.r, fillColor.g, fillColor.b);

  QBrush brush;
  brush.setStyle(Qt::BrushStyle::SolidPattern);
  brush.setColor(brushColor);

  const plInt32 iPosY = area.center().y();

  p.setPen(penColor);
  p.setBrush(brush);

  if (!selected)
  {
    p.drawEllipse(QPoint(iPosX, iPosY), CpRadius, CpRadius);
  }
  else
  {
    p.drawEllipse(QPoint(iPosX, iPosY), CpRadius, CpRadius);
    // p.drawRoundRect(QRect(iPosX - CpRadius, iPosY - CpRadius, 2 * CpRadius, 2 * CpRadius), CpRoundedCorner);
  }
}

void plQtColorGradientWidget::PaintColorCPs(QPainter& p) const
{
  if (!m_bShowColorCPs)
    return;

  const QRect area = GetColorCpArea();

  plUInt32 numRgb;
  plUInt32 numAlpha;
  plUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  for (plUInt32 i = 0; i < numRgb; ++i)
  {
    const auto& cp = m_pColorGradientData->GetColorControlPoint(i);

    const bool selected = (i == m_iSelectedColorCP);

    PaintControlPoint(
      p, area, cp.m_PosX, selected ? plColor::White : plColor::Black, plColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue), selected);
  }
}


void plQtColorGradientWidget::PaintAlphaCPs(QPainter& p) const
{
  if (!m_bShowAlphaCPs)
    return;

  const QRect area = GetAlphaCpArea();

  plUInt32 numRgb;
  plUInt32 numAlpha;
  plUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  for (plUInt32 i = 0; i < numAlpha; ++i)
  {
    const auto& cp = m_pColorGradientData->GetAlphaControlPoint(i);

    const bool selected = i == m_iSelectedAlphaCP;

    PaintControlPoint(p, area, cp.m_PosX, selected ? plColor::White : plColor::Black, plColorGammaUB(cp.m_Alpha, cp.m_Alpha, cp.m_Alpha), selected);
  }
}

void plQtColorGradientWidget::PaintIntensityCPs(QPainter& p) const
{
  if (!m_bShowIntensityCPs)
    return;

  const QRect area = GetIntensityCpArea();

  plUInt32 numRgb;
  plUInt32 numAlpha;
  plUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  float fMaxIntensity = 0.0f;
  for (plUInt32 i = 0; i < numIntensity; ++i)
  {
    const auto& cp = m_pColorGradientData->GetIntensityControlPoint(i);
    fMaxIntensity = plMath::Max(cp.m_Intensity, fMaxIntensity);
  }

  const float fInvMaxIntensity = 1.0f / fMaxIntensity;

  for (plUInt32 i = 0; i < numIntensity; ++i)
  {
    const auto& cp = m_pColorGradientData->GetIntensityControlPoint(i);

    const bool selected = i == m_iSelectedIntensityCP;

    float fIntensity = cp.m_Intensity * fInvMaxIntensity;
    PaintControlPoint(p, area, cp.m_PosX, selected ? plColor::White : plColor::Black, plColor(fIntensity, fIntensity, fIntensity), selected);
  }
}

void plQtColorGradientWidget::PaintScrubber(QPainter& p) const
{
  if (!m_bShowScrubber)
    return;

  const QRect area = rect();

  const plInt32 xPos = GradientToWindowCoord(m_fScrubberPosition);
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

void plQtColorGradientWidget::mousePressEvent(QMouseEvent* event)
{
  if (!m_bEditMode)
  {
    // in non-edit mode, allow to react to this click (only)

    if (event->button() == Qt::MouseButton::LeftButton)
    {
      Q_EMIT GradientClicked();
    }
  }
  else
  {
    if (event->button() == Qt::MouseButton::RightButton)
    {
      m_LastMousePosition = event->globalPosition();
    }

    if (event->buttons() == Qt::MouseButton::LeftButton)
    {
      // left click and nothing else

      plInt32 iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp;
      if (HoversControlPoint(event->pos(), iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp))
      {
        SelectCP(iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp);

        m_bDraggingCP = true;

        update();
      }
    }
  }

  QWidget::mousePressEvent(event);
}


void plQtColorGradientWidget::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::MouseButton::LeftButton)
  {
    m_bDraggingCP = false;

    if (m_bTempMode)
    {
      m_bTempMode = false;
      Q_EMIT endOperation(true);
    }
  }

  QWidget::mouseReleaseEvent(event);
}

void plQtColorGradientWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
  if (m_bEditMode)
  {
    const Area area = HoversInteractiveArea(event->pos());

    // left click and nothing else
    if (event->buttons() == Qt::MouseButton::LeftButton)
    {
      const double posX = WindowToGradientCoord(event->pos().x());

      const bool hovers = HoversControlPoint(event->pos());

      // in an interactive area, not over any control point
      if (area > Area::Gradient)
      {
        if (!hovers)
        {
          plColorGammaUB rgba;
          float intensity;
          EvaluateAt(event->pos().x(), rgba, intensity);

          if (area == Area::ColorCPs)
          {
            Q_EMIT addColorCp(posX, rgba);
          }
          else if (area == Area::AlphaCPs)
          {
            Q_EMIT addAlphaCp(posX, rgba.a);
          }
          else if (area == Area::IntensityCPs)
          {
            Q_EMIT addIntensityCp(posX, intensity);
          }

          setCursor(Qt::SizeHorCursor);
        }
        else if (m_iSelectedColorCP != -1)
        {
          if (m_bTempMode)
          {
            m_bDraggingCP = false;
            m_bTempMode = false;
            Q_EMIT endOperation(true);
          }

          Q_EMIT triggerPickColor();
        }
      }
    }
  }

  QWidget::mouseDoubleClickEvent(event);
}

void plQtColorGradientWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (m_bEditMode)
  {
    // reset mouse dragging flag if necessary
    if (!event->buttons().testFlag(Qt::MouseButton::LeftButton))
    {
      m_bDraggingCP = false;

      if (m_bTempMode)
      {
        m_bTempMode = false;
        Q_EMIT endOperation(true);
      }
    }

    if (m_bDraggingCP)
    {
      if (!m_bTempMode)
      {
        m_bTempMode = true;
        Q_EMIT beginOperation();
      }

      const double newPosX = WindowToGradientCoord(event->pos().x());

      if (m_iSelectedColorCP != -1)
      {
        Q_EMIT moveColorCpToPos(m_iSelectedColorCP, newPosX);
      }
      else if (m_iSelectedAlphaCP != -1)
      {
        Q_EMIT moveAlphaCpToPos(m_iSelectedAlphaCP, newPosX);
      }
      else if (m_iSelectedIntensityCP != -1)
      {
        Q_EMIT moveIntensityCpToPos(m_iSelectedIntensityCP, newPosX);
      }
    }
    else
    {
      if (event->buttons() == Qt::MouseButton::RightButton)
      {
        // scroll displayed area
        if (m_fDisplayExtentMinX < m_fDisplayExtentMaxX)
        {
          const QPointF mouseMove = event->globalPosition() - m_LastMousePosition;
          m_LastMousePosition = event->globalPosition();

          const double range = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;

          const double scrolled = (double)mouseMove.x() / (double)GetGradientArea().width();

          /// \todo Why not += ?
          m_fDisplayExtentMinX -= scrolled * range;
          m_fDisplayExtentMaxX -= scrolled * range;

          update();
        }
      }
    }
  }

  UpdateMouseCursor(event);
  QWidget::mouseMoveEvent(event);
}


void plQtColorGradientWidget::UpdateMouseCursor(QMouseEvent* event)
{
  setCursor(Qt::ArrowCursor);

  if (m_bDraggingCP)
  {
    setCursor(Qt::SizeHorCursor);
    return;
  }

  if (event->buttons() == Qt::MouseButton::RightButton)
  {
    setCursor(Qt::ClosedHandCursor);
    return;
  }

  plInt32 iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp;

  if (HoversInteractiveArea(event->pos()) > Area::Gradient)
  {
    if (HoversControlPoint(event->pos(), iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp))
      setCursor(Qt::SizeHorCursor);
    else
      setCursor(Qt::PointingHandCursor);
  }
}

void plQtColorGradientWidget::wheelEvent(QWheelEvent* event)
{
  if (m_bEditMode)
  {
    // zoom displayed area
    if (m_fDisplayExtentMinX < m_fDisplayExtentMaxX)
    {
      const double range = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      const double zoomCenter = WindowToGradientCoord(event->position().x());
#else
      const double zoomCenter = WindowToGradientCoord(event->pos().x());
#endif
      const double zoomNorm = (zoomCenter - m_fDisplayExtentMinX) / range;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      const double changePerc = (event->angleDelta().y() > 0) ? 0.1 : -0.1;
#else
      const double changePerc = (event->delta() > 0) ? 0.1 : -0.1;
#endif
      const double change = changePerc * range;

      m_fDisplayExtentMinX += change * zoomNorm;
      m_fDisplayExtentMaxX -= change * (1.0 - zoomNorm);

      ClampDisplayExtents(zoomNorm);

      update();
    }
  }

  QWidget::wheelEvent(event);
}

void plQtColorGradientWidget::ClampDisplayExtents(double zoomCenter)
{
  const double newRange = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;
  const double clampedRange = plMath::Clamp(newRange, 0.05, 100.0);
  const double center = plMath::Lerp(m_fDisplayExtentMinX, m_fDisplayExtentMaxX, zoomCenter);

  m_fDisplayExtentMinX = center - clampedRange * zoomCenter;
  m_fDisplayExtentMaxX = center + clampedRange * (1.0 - zoomCenter);
}

void plQtColorGradientWidget::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Delete)
  {
    if (m_iSelectedColorCP != -1)
    {
      Q_EMIT deleteColorCp(m_iSelectedColorCP);
      ClearSelectedCP();
    }
    else if (m_iSelectedAlphaCP != -1)
    {
      Q_EMIT deleteAlphaCp(m_iSelectedAlphaCP);
      ClearSelectedCP();
    }
    else if (m_iSelectedIntensityCP != -1)
    {
      Q_EMIT deleteIntensityCp(m_iSelectedIntensityCP);
      ClearSelectedCP();
    }
  }

  QWidget::keyPressEvent(event);
}

QRect plQtColorGradientWidget::GetColorCpArea() const
{
  QRect r = rect();
  r.setTop(r.bottom() - CpAreaHeight);

  if (m_bShowCoordsBottom)
    r.setTop(r.top() - CpAreaHeight);

  r.setHeight(CpAreaHeight);
  return r;
}

QRect plQtColorGradientWidget::GetAlphaCpArea() const
{
  QRect r = rect();

  if (m_bShowIntensityCPs)
  {
    // below intensity curve
    r.setTop(r.top() + CpAreaHeight);
  }

  if (m_bShowCoordsTop)
    r.setTop(r.top() + CpAreaHeight);

  r.setHeight(CpAreaHeight);
  return r;
}

QRect plQtColorGradientWidget::GetIntensityCpArea() const
{
  QRect r = rect();

  if (m_bShowCoordsTop)
    r.setTop(r.top() + CpAreaHeight);

  r.setHeight(CpAreaHeight);
  return r;
}

QRect plQtColorGradientWidget::GetGradientArea() const
{
  QRect r = rect();

  if (m_bShowIntensityCPs)
    r.setTop(r.top() + CpAreaHeight);

  if (m_bShowAlphaCPs)
    r.setTop(r.top() + CpAreaHeight);

  if (m_bShowCoordsTop)
    r.setTop(r.top() + CpAreaHeight);


  if (m_bShowColorCPs)
    r.setBottom(r.bottom() - CpAreaHeight);

  if (m_bShowCoordsBottom)
    r.setBottom(r.bottom() - CpAreaHeight);

  return r;
}


QRect plQtColorGradientWidget::GetCoordAreaTop() const
{
  QRect r = rect();

  r.setHeight(CpAreaHeight);
  return r;
}


QRect plQtColorGradientWidget::GetCoordAreaBottom() const
{
  QRect r = rect();
  r.setTop(r.bottom() - CpAreaHeight);

  r.setHeight(CpAreaHeight);
  return r;
}

double plQtColorGradientWidget::WindowToGradientCoord(plInt32 mouseWindowPosX) const
{
  QRect area = GetGradientArea();
  const double norm = (double)(mouseWindowPosX - area.left()) / (double)area.width();
  return m_fDisplayExtentMinX + norm * (m_fDisplayExtentMaxX - m_fDisplayExtentMinX);
}

plInt32 plQtColorGradientWidget::GradientToWindowCoord(double gradientPosX) const
{
  QRect area = GetGradientArea();
  const double norm = (gradientPosX - m_fDisplayExtentMinX) / (m_fDisplayExtentMaxX - m_fDisplayExtentMinX);
  return area.left() + norm * (area.right() - area.left());
}

plInt32 plQtColorGradientWidget::FindClosestColorCp(plInt32 iWindowPosX) const
{
  plUInt32 numRgb;
  plUInt32 numAlpha;
  plUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  plInt32 iClosest = -1;
  plInt32 iBestDistance = MaxCpPickDistance + 1;

  for (plUInt32 i = 0; i < numRgb; ++i)
  {
    const auto& cp = m_pColorGradientData->GetColorControlPoint(i);

    const plInt32 iCpPos = GradientToWindowCoord(cp.m_PosX);
    const plInt32 iDist = plMath::Abs(iCpPos - iWindowPosX);

    if (iDist < iBestDistance)
    {
      iClosest = i;
      iBestDistance = iDist;
    }
  }

  return iClosest;
}

plInt32 plQtColorGradientWidget::FindClosestAlphaCp(plInt32 iWindowPosX) const
{
  plUInt32 numRgb;
  plUInt32 numAlpha;
  plUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  plInt32 iClosest = -1;
  plInt32 iBestDistance = MaxCpPickDistance + 1;

  for (plUInt32 i = 0; i < numAlpha; ++i)
  {
    const auto& cp = m_pColorGradientData->GetAlphaControlPoint(i);

    const plInt32 iCpPos = GradientToWindowCoord(cp.m_PosX);
    const plInt32 iDist = plMath::Abs(iCpPos - iWindowPosX);

    if (iDist < iBestDistance)
    {
      iClosest = i;
      iBestDistance = iDist;
    }
  }

  return iClosest;
}

plInt32 plQtColorGradientWidget::FindClosestIntensityCp(plInt32 iWindowPosX) const
{
  plUInt32 numRgb;
  plUInt32 numAlpha;
  plUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  plInt32 iClosest = -1;
  plInt32 iBestDistance = MaxCpPickDistance + 1;

  for (plUInt32 i = 0; i < numIntensity; ++i)
  {
    const auto& cp = m_pColorGradientData->GetIntensityControlPoint(i);

    const plInt32 iCpPos = GradientToWindowCoord(cp.m_PosX);
    const plInt32 iDist = plMath::Abs(iCpPos - iWindowPosX);

    if (iDist < iBestDistance)
    {
      iClosest = i;
      iBestDistance = iDist;
    }
  }

  return iClosest;
}

bool plQtColorGradientWidget::HoversControlPoint(const QPoint& windowPos) const
{
  plInt32 iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp;
  return HoversControlPoint(windowPos, iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp);
}

bool plQtColorGradientWidget::HoversControlPoint(
  const QPoint& windowPos, plInt32& iHoverColorCp, plInt32& iHoverAlphaCp, plInt32& iHoverIntensityCp) const
{
  iHoverColorCp = -1;
  iHoverAlphaCp = -1;
  iHoverIntensityCp = -1;

  if (m_bShowColorCPs)
  {
    if (GetColorCpArea().contains(windowPos))
    {
      iHoverColorCp = FindClosestColorCp(windowPos.x());
    }
  }

  if (m_bShowAlphaCPs)
  {
    if (GetAlphaCpArea().contains(windowPos))
    {
      iHoverAlphaCp = FindClosestAlphaCp(windowPos.x());
    }
  }

  if (m_bShowIntensityCPs)
  {
    if (GetIntensityCpArea().contains(windowPos))
    {
      iHoverIntensityCp = FindClosestIntensityCp(windowPos.x());
    }
  }

  return (iHoverColorCp != -1) || (iHoverAlphaCp != -1) || (iHoverIntensityCp != -1);
}


plQtColorGradientWidget::Area plQtColorGradientWidget::HoversInteractiveArea(const QPoint& windowPos) const
{
  if (m_bShowColorCPs)
  {
    if (GetColorCpArea().contains(windowPos))
      return Area::ColorCPs;
  }

  if (m_bShowAlphaCPs)
  {
    if (GetAlphaCpArea().contains(windowPos))
      return Area::AlphaCPs;
  }

  if (m_bShowIntensityCPs)
  {
    if (GetIntensityCpArea().contains(windowPos))
      return Area::IntensityCPs;
  }

  if (GetGradientArea().contains(windowPos))
    return Area::Gradient;

  return Area::None;
}


void plQtColorGradientWidget::EvaluateAt(plInt32 windowPos, plColorGammaUB& rgba, float& intensity) const
{
  plColorGradient GradientFinal;
  GradientFinal = *m_pColorGradientData;
  GradientFinal.SortControlPoints();

  const double range = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;

  const double lerp = (double)windowPos / (double)rect().width();
  GradientFinal.Evaluate(m_fDisplayExtentMinX + lerp * range, rgba, intensity);
}


double plQtColorGradientWidget::ComputeCoordinateDisplayStep() const
{
  const plInt32 iPixelsNeeded = 50;
  const double fFitInWindow = plMath::Max<double>(2, rect().width() / (double)iPixelsNeeded);

  const double fGradientRange = (m_fDisplayExtentMaxX - m_fDisplayExtentMinX);
  const double fSubRange = fGradientRange / fFitInWindow;

  const double fExp = plMath::Log10(fSubRange);
  const plInt32 iExp = plMath::Ceil(fExp);

  const double step = plMath::Pow(10.0, (double)iExp);

  return step;
}

void plQtColorGradientWidget::FrameExtents()
{
  if (m_pColorGradientData)
  {
    m_pColorGradientData->GetExtents(m_fDisplayExtentMinX, m_fDisplayExtentMaxX);
  }

  if (!m_pColorGradientData || m_fDisplayExtentMinX > m_fDisplayExtentMaxX)
  {
    m_fDisplayExtentMinX = 0;
    m_fDisplayExtentMaxX = 1;
  }

  if (m_fDisplayExtentMinX == m_fDisplayExtentMaxX)
  {
    m_fDisplayExtentMinX = plMath::Floor(m_fDisplayExtentMinX - 0.1);
    m_fDisplayExtentMaxX = plMath::Ceil(m_fDisplayExtentMaxX + 0.1);
  }

  if (m_bEditMode)
  {
    // round up/down to next multiple of 1
    m_fDisplayExtentMinX = plMath::Floor(m_fDisplayExtentMinX);
    m_fDisplayExtentMaxX = plMath::Ceil(m_fDisplayExtentMaxX);

    const double range = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;
    const double border = range * 0.05;

    m_fDisplayExtentMinX -= border;
    m_fDisplayExtentMaxX += border;
  }
}