#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QDoubleSpinBox>
class plVariant;

class PLASMA_GUIFOUNDATION_DLL plQtDoubleSpinBox : public QDoubleSpinBox
{
  Q_OBJECT
public:
  explicit plQtDoubleSpinBox(QWidget* pParent, bool bIntMode = false);

  void SetIntMode(bool enable);

  void setDisplaySuffix(const char* szSuffix);
  void setDefaultValue(double value);
  void setDefaultValue(const plVariant& val);
  using QDoubleSpinBox::setMaximum;
  using QDoubleSpinBox::setMinimum;
  void setMinimum(const plVariant& val);
  void setMaximum(const plVariant& val);

  virtual QString textFromValue(double val) const override;
  virtual double valueFromText(const QString& text) const override;

  void setValueInvalid();
  void setValue(double val);
  void setValue(const plVariant& val);
  double value() const;

protected:
  virtual void wheelEvent(QWheelEvent* event) override;
  virtual void focusInEvent(QFocusEvent* event) override;
  virtual void focusOutEvent(QFocusEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void keyPressEvent(QKeyEvent* event) override;
  virtual bool event(QEvent* event) override;

private Q_SLOTS:
  void onCustomContextMenuRequested();

private:
  QString m_sSuffix;
  double m_fDefaultValue;
  mutable double m_fDisplayedValue;
  mutable QString m_sDisplayedText;
  mutable bool m_bInvalid;
  bool m_bModified;
  bool m_bIntMode;
  bool m_bDragging;
  double m_fStartDragValue;
  QPoint m_LastDragPos;
  plInt32 m_iDragDelta;
};

