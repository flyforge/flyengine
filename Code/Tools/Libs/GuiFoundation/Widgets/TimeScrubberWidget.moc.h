#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Time/Time.h>

#include <QToolBar>
#include <QWidget>

class QMouseEvent;
class QPushButton;
class QLineEdit;

class PLASMA_GUIFOUNDATION_DLL plQtTimeScrubberWidget : public QWidget
{
  Q_OBJECT

public:
  explicit plQtTimeScrubberWidget(QWidget* pParent);
  ~plQtTimeScrubberWidget();

  /// \brief Sets the duration in 'ticks'. There are 4800 ticks per second.
  void SetDuration(plUInt64 uiNumTicks);

  /// \brief Sets the duration.
  void SetDuration(plTime time);

  /// \brief Sets the current position in 'ticks'. There are 4800 ticks per second.
  void SetScrubberPosition(plUInt64 uiTick);

  /// \brief Sets the current position.
  void SetScrubberPosition(plTime time);

Q_SIGNALS:
  void ScrubberPosChangedEvent(plUInt64 uiNewScrubberTickPos);

private:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  void SetScrubberPosFromPixelCoord(plInt32 x);

  plUInt64 m_uiDurationTicks = 0;
  plTime m_Duration;
  plUInt64 m_uiScrubberTickPos = 0;
  double m_fNormScrubberPosition = 0.0;
  bool m_bDragging = false;
};

class PLASMA_GUIFOUNDATION_DLL plQtTimeScrubberToolbar : public QToolBar
{
  Q_OBJECT

public:
  explicit plQtTimeScrubberToolbar(QWidget* pParent);

  /// \brief Sets the duration in 'ticks'. There are 4800 ticks per second.
  void SetDuration(plUInt64 uiNumTicks);

  /// \brief Sets the current position in 'ticks'. There are 4800 ticks per second.
  void SetScrubberPosition(plUInt64 uiTick);

  void SetButtonState(bool bPlaying, bool bRepeatEnabled);

Q_SIGNALS:
  void ScrubberPosChangedEvent(plUInt64 uiNewScrubberTickPos);
  void PlayPauseEvent();
  void RepeatEvent();
  void DurationChangedEvent(double fDuration);
  void AdjustDurationEvent();

private:
  plQtTimeScrubberWidget* m_pScrubber = nullptr;
  QPushButton* m_pPlayButton = nullptr;
  QPushButton* m_pRepeatButton = nullptr;
  QLineEdit* m_pDuration = nullptr;
  QPushButton* m_pAdjustDurationButton = nullptr;
};

