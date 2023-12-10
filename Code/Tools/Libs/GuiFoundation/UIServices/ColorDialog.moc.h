#pragma once

#include <Foundation/Math/Color.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/ColorDlgWidgets.moc.h>
#include <GuiFoundation/ui_ColorDialog.h>
#include <QDialog>

class QLineEdit;
class plQtDoubleSpinBox;
class QPushButton;
class QSlider;


class PLASMA_GUIFOUNDATION_DLL plQtColorDialog : public QDialog, Ui_ColorDialog
{
  Q_OBJECT
public:
  plQtColorDialog(const plColor& initial, QWidget* pParent);
  ~plQtColorDialog();

  void ShowAlpha(bool bEnable);
  void ShowHDR(bool bEnable);

  static QByteArray GetLastDialogGeometry() { return s_LastDialogGeometry; }

Q_SIGNALS:
  void CurrentColorChanged(const plColor& color);
  void ColorSelected(const plColor& color);

private Q_SLOTS:
  void ChangedRGB();
  void ChangedAlpha();
  void ChangedExposure();
  void ChangedHSV();
  void ChangedArea(double x, double y);
  void ChangedRange(double x);
  void ChangedHEX();

private:
  bool m_bAlpha;
  bool m_bHDR;

  float m_fHue;
  float m_fSaturation;
  float m_fValue;

  plUInt16 m_uiHue;
  plUInt8 m_uiSaturation;

  plUInt8 m_uiGammaRed;
  plUInt8 m_uiGammaGreen;
  plUInt8 m_uiGammaBlue;

  plUInt8 m_uiAlpha;
  float m_fExposureValue;

  plColor m_CurrentColor;

  static QByteArray s_LastDialogGeometry;

private:
  void ApplyColor();

  void RecomputeHDR();

  void ExtractColorRGB();
  void ExtractColorHSV();

  void ComputeRgbAndHsv(const plColor& color);
  void RecomputeRGB();
  void RecomputeHSV();
};

