#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/Color8UNorm.h>
#include <GuiFoundation/UIServices/ColorDlgWidgets.moc.h>
#include <QPainter>
#include <qevent.h>

plQtColorAreaWidget::plQtColorAreaWidget(QWidget* parent)
  : QWidget(parent)
{
  setAutoFillBackground(false);

  m_fHue = -1.0f;
}

void plQtColorAreaWidget::SetHue(float hue)
{
  if (m_fHue == hue)
    return;

  m_fHue = hue;
  UpdateImage();
  update();
}

void plQtColorAreaWidget::SetSaturation(float sat)
{
  if (m_fSaturation == sat)
    return;

  m_fSaturation = sat;
  update();
}

void plQtColorAreaWidget::SetValue(float val)
{
  if (m_fValue == val)
    return;

  m_fValue = val;
  update();
}

void plQtColorAreaWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::RenderHint::Antialiasing);

  const QRect area = rect();

  painter.drawTiledPixmap(area, QPixmap::fromImage(m_Image));

  QPointF center;
  center.setX((int)(area.width() * m_fSaturation) + 0.5f);
  center.setY((int)(area.height() * (1.0f - m_fValue)) + 0.5f);

  const QColor col = qRgb(40, 40, 40);

  painter.setPen(col);
  painter.drawEllipse(center, 5.5f, 5.5f);
}

void plQtColorAreaWidget::UpdateImage()
{
  const int width = rect().width();
  const int height = rect().height();

  m_Image = QImage(width, height, QImage::Format::Format_RGB32);


  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      plColor c;
      c.SetHSV(m_fHue, (double)x / (width - 1), (double)y / (height - 1));

      plColorGammaUB cg = c;
      m_Image.setPixel(x, (height - 1) - y, qRgb(cg.r, cg.g, cg.b));
    }
  }
}

void plQtColorAreaWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons().testFlag(Qt::LeftButton))
  {
    const int width = rect().width();
    const int height = rect().height();

    QPoint coord = event->pos();
    const int sat = plMath::Clamp(coord.x(), 0, width - 1);
    const int val = plMath::Clamp((height - 1) - coord.y(), 0, height - 1);

    const double fsat = (double)sat / (width - 1);
    const double fval = (double)val / (height - 1);

    valueChanged(fsat, fval);
  }
}

void plQtColorAreaWidget::mousePressEvent(QMouseEvent* event)
{
  mouseMoveEvent(event);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plQtColorRangeWidget::plQtColorRangeWidget(QWidget* parent)
  : QWidget(parent)
{
  setAutoFillBackground(false);
}

void plQtColorRangeWidget::SetHue(float hue)
{
  if (m_fHue == hue)
    return;

  m_fHue = hue;
  update();
}

void plQtColorRangeWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::RenderHint::Antialiasing);

  const QRect area = rect();

  if (area.width() != m_Image.width())
    UpdateImage();

  painter.drawTiledPixmap(area, QPixmap::fromImage(m_Image));

  const float pos = (int)(m_fHue / 359.0f * area.width()) + 0.5f;

  const float top = area.top() + 0.5f;
  const float bot = area.bottom() + 0.5f;
  const float len = 5.0f;
  const float wid = 2.0f;

  const QColor col = qRgb(80, 80, 80);

  painter.setPen(col);
  painter.setBrush(col);

  {
    QPainterPath path;
    path.moveTo(QPointF(pos - wid, top));
    path.lineTo(QPointF(pos, top + len));
    path.lineTo(QPointF(pos + wid, top));
    path.closeSubpath();

    painter.drawPath(path);
  }

  {
    QPainterPath path;
    path.moveTo(QPointF(pos - wid, bot));
    path.lineTo(QPointF(pos, bot - len));
    path.lineTo(QPointF(pos + wid, bot));
    path.closeSubpath();

    painter.drawPath(path);
  }
}

void plQtColorRangeWidget::UpdateImage()
{
  const int width = rect().width();
  const int height = rect().height();

  m_Image = QImage(width, 1, QImage::Format::Format_RGB32);

  for (int x = 0; x < width; ++x)
  {
    plColor c;
    c.SetHSV(((double)x / (width - 1.0)) * 360.0, 1, 1);

    plColorGammaUB cg = c;
    m_Image.setPixel(x, 0, qRgb(cg.r, cg.g, cg.b));
  }
}

void plQtColorRangeWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons().testFlag(Qt::LeftButton))
  {
    const int width = rect().width();
    const int height = rect().height();

    QPoint coord = event->pos();
    const int x = plMath::Clamp(coord.x(), 0, width - 1);

    const double fx = (double)x / (width - 1);

    valueChanged(fx);
  }
}

void plQtColorRangeWidget::mousePressEvent(QMouseEvent* event)
{
  mouseMoveEvent(event);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plQtColorCompareWidget::plQtColorCompareWidget(QWidget* parent)
{
  setAutoFillBackground(false);
}

void plQtColorCompareWidget::SetNewColor(const plColor& color)
{
  if (m_NewColor == color)
    return;

  m_NewColor = color;
  update();
}

void plQtColorCompareWidget::SetInitialColor(const plColor& color)
{
  m_InitialColor = color;
  m_NewColor = color;
}

void plQtColorCompareWidget::paintEvent(QPaintEvent*)
{
  const QRect area = rect();
  const QRect areaTop(area.left(), area.top(), area.width(), area.height() / 2);
  const QRect areaBot(area.left(), areaTop.bottom(), area.width(), area.height() - areaTop.height()); // rounding ...

  QPainter p(this);

  plColor inLDR = m_InitialColor;
  float fMultiplier = m_InitialColor.ComputeHdrMultiplier();

  if (fMultiplier > 1.0f)
  {
    inLDR.ScaleRGB(1.0f / fMultiplier);
  }

  QColor qInCol = plToQtColor(inLDR);

  plColor newLDR = m_NewColor;
  fMultiplier = m_NewColor.ComputeHdrMultiplier();

  if (fMultiplier > 1.0f)
  {
    newLDR.ScaleRGB(1.0f / fMultiplier);
  }

  QColor qNewCol = plToQtColor(newLDR);

  p.fillRect(areaTop, qInCol);
  p.fillRect(areaBot, qNewCol);
}
