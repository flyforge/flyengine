#pragma once

#include <Foundation/Tracks/ColorGradient.h>
#include <GuiFoundation/GuiFoundationDLL.h>

#include <QWidget>

class QMouseEvent;

class PLASMA_GUIFOUNDATION_DLL plQtColorGradientWidget : public QWidget
{
  Q_OBJECT

public:
  explicit plQtColorGradientWidget(QWidget* pParent);
  ~plQtColorGradientWidget();

  void SetScrubberPosition(double fPosition);

  void setColorGradientData(const plColorGradient* gradient);

  void setEditMode(bool edit);
  void setShowColorCPs(bool show);
  void setShowAlphaCPs(bool show);
  void setShowIntensityCPs(bool show);
  void setShowCoords(bool top, bool bottom);

  void FrameExtents();
  void ClearSelectedCP();
  void SelectCP(plInt32 colorCP, plInt32 alphaCP, plInt32 intensityCP);

Q_SIGNALS:
  void GradientClicked();
  void addColorCp(double posX, const plColorGammaUB& color);
  void addAlphaCp(double posX, plUInt8 value);
  void addIntensityCp(double posX, float intensity);
  void moveColorCpToPos(plInt32 index, double newPosX);
  void moveAlphaCpToPos(plInt32 index, double newPosX);
  void moveIntensityCpToPos(plInt32 index, double newPosX);
  void deleteColorCp(plInt32 index);
  void deleteAlphaCp(plInt32 index);
  void deleteIntensityCp(plInt32 index);
  void selectionChanged(plInt32 colorCP, plInt32 alphaCP, plInt32 intensityCP);
  void beginOperation();
  void endOperation(bool commit);
  void triggerPickColor();

private:
  enum class Area
  {
    None = 0,
    Gradient = 1,
    ColorCPs = 2,
    AlphaCPs = 3,
    IntensityCPs = 4,
  };


  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;

  void UpdateMouseCursor(QMouseEvent* event);

  virtual void wheelEvent(QWheelEvent* event) override;

  void ClampDisplayExtents(double zoomCenter = 0.5);

  virtual void keyPressEvent(QKeyEvent* event) override;

  void PaintColorGradient(QPainter& p) const;
  void PaintCpBackground(QPainter& p, const QRect& area) const;
  void PaintColorCpArea(QPainter& p);
  void PaintAlphaCpArea(QPainter& p);
  void PaintIntensityCpArea(QPainter& p);
  void PaintCoordinateStrips(QPainter& p) const;
  void PaintCoordinateStrip(QPainter& p, const QRect& area) const;
  void PaintCoordinateLines(QPainter& p);

  void PaintControlPoint(
    QPainter& p, const QRect& area, double posX, const plColorGammaUB& outlineColor, const plColorGammaUB& fillColor, bool selected) const;
  void PaintColorCPs(QPainter& p) const;
  void PaintAlphaCPs(QPainter& p) const;
  void PaintIntensityCPs(QPainter& p) const;
  void PaintScrubber(QPainter& p) const;

  QRect GetColorCpArea() const;
  QRect GetAlphaCpArea() const;
  QRect GetIntensityCpArea() const;
  QRect GetGradientArea() const;
  QRect GetCoordAreaTop() const;
  QRect GetCoordAreaBottom() const;

  double WindowToGradientCoord(plInt32 mouseWindowPosX) const;
  plInt32 GradientToWindowCoord(double gradientPosX) const;

  plInt32 FindClosestColorCp(plInt32 iWindowPosX) const;
  plInt32 FindClosestAlphaCp(plInt32 iWindowPosX) const;
  plInt32 FindClosestIntensityCp(plInt32 iWindowPosX) const;

  bool HoversControlPoint(const QPoint& windowPos) const;
  bool HoversControlPoint(const QPoint& windowPos, plInt32& iHoverColorCp, plInt32& iHoverAlphaCp, plInt32& iHoverIntensityCp) const;
  Area HoversInteractiveArea(const QPoint& windowPos) const;

  void EvaluateAt(plInt32 windowPos, plColorGammaUB& rgba, float& intensity) const;

  double ComputeCoordinateDisplayStep() const;

  const plColorGradient* m_pColorGradientData;

  bool m_bEditMode;
  bool m_bShowColorCPs;
  bool m_bShowAlphaCPs;
  bool m_bShowIntensityCPs;
  bool m_bDraggingCP;
  bool m_bTempMode;
  bool m_bShowCoordsTop;
  bool m_bShowCoordsBottom;

  double m_fDisplayExtentMinX;
  double m_fDisplayExtentMaxX;

  plInt32 m_iSelectedColorCP;
  plInt32 m_iSelectedAlphaCP;
  plInt32 m_iSelectedIntensityCP;

  QPoint m_LastMousePosition;
  QPixmap m_AlphaPattern;

  bool m_bShowScrubber = false;
  double m_fScrubberPosition = 0;
};

