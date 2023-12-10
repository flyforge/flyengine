#pragma once

#include <Foundation/Math/Color.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidget>

class PLASMA_GUIFOUNDATION_DLL plQtColorAreaWidget : public QWidget
{
  Q_OBJECT
public:
  plQtColorAreaWidget(QWidget* pParent);

  float GetHue() const { return m_fHue; }
  void SetHue(float fHue);

  float GetSaturation() const { return m_fSaturation; }
  void SetSaturation(float fSat);

  float GetValue() const { return m_fValue; }
  void SetValue(float fVal);

Q_SIGNALS:
  void valueChanged(double x, double y);

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;

  void UpdateImage();

  QImage m_Image;
  float m_fHue;
  float m_fSaturation;
  float m_fValue;
};

class PLASMA_GUIFOUNDATION_DLL plQtColorRangeWidget : public QWidget
{
  Q_OBJECT
public:
  plQtColorRangeWidget(QWidget* pParent);

  float GetHue() const { return m_fHue; }
  void SetHue(float fHue);

Q_SIGNALS:
  void valueChanged(double x);

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;

  void UpdateImage();

  QImage m_Image;
  float m_fHue;
};

class PLASMA_GUIFOUNDATION_DLL plQtColorCompareWidget : public QWidget
{
  Q_OBJECT
public:
  plQtColorCompareWidget(QWidget* pParent);

  void SetNewColor(const plColor& color);
  void SetInitialColor(const plColor& color);

protected:
  virtual void paintEvent(QPaintEvent*) override;

  plColor m_InitialColor;
  plColor m_NewColor;
};

