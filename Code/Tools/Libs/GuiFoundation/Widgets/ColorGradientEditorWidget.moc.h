#pragma once

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ColorGradientEditorWidget.h>

#include <QWidget>

class QMouseEvent;

class PLASMA_GUIFOUNDATION_DLL plQtColorGradientEditorWidget : public QWidget, public Ui_ColorGradientEditorWidget
{
  Q_OBJECT

public:
  explicit plQtColorGradientEditorWidget(QWidget* pParent);
  ~plQtColorGradientEditorWidget();

  void SetColorGradient(const plColorGradient& gradient);
  const plColorGradient& GetColorGradient() const { return m_Gradient; }

  void ShowColorPicker() { on_ButtonColor_clicked(); }
  void SetScrubberPosition(plUInt64 uiTick);

  void FrameGradient();

Q_SIGNALS:
  void ColorCpAdded(double posX, const plColorGammaUB& color);
  void ColorCpMoved(plInt32 index, float newPosX);
  void ColorCpDeleted(plInt32 index);
  void ColorCpChanged(plInt32 index, const plColorGammaUB& color);

  void AlphaCpAdded(double posX, plUInt8 alpha);
  void AlphaCpMoved(plInt32 index, double newPosX);
  void AlphaCpDeleted(plInt32 index);
  void AlphaCpChanged(plInt32 index, plUInt8 alpha);

  void IntensityCpAdded(double posX, float intensity);
  void IntensityCpMoved(plInt32 index, double newPosX);
  void IntensityCpDeleted(plInt32 index);
  void IntensityCpChanged(plInt32 index, float intensity);

  void NormalizeRange();

  void BeginOperation();
  void EndOperation(bool commit);

private Q_SLOTS:
  void on_ButtonFrame_clicked();
  void on_GradientWidget_selectionChanged(plInt32 colorCP, plInt32 alphaCP, plInt32 intensityCP);
  void on_SpinPosition_valueChanged(double value);
  void on_SpinAlpha_valueChanged(int value);
  void on_SliderAlpha_valueChanged(int value);
  void on_SliderAlpha_sliderPressed();
  void on_SliderAlpha_sliderReleased();
  void on_SpinIntensity_valueChanged(double value);
  void on_ButtonColor_clicked();
  void onCurrentColorChanged(const plColor& col);
  void onColorAccepted();
  void onColorReset();
  void on_ButtonNormalize_clicked();

protected:
  virtual void showEvent(QShowEvent* event) override;

private:
  void UpdateCpUi();

  QPalette m_Pal;
  plInt32 m_iSelectedColorCP;
  plInt32 m_iSelectedAlphaCP;
  plInt32 m_iSelectedIntensityCP;
  plColorGradient m_Gradient;

  plColorGammaUB m_PickColorStart;
  plColorGammaUB m_PickColorCurrent;
};

