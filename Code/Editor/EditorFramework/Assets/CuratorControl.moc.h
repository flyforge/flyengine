#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Assets/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <Foundation/Basics.h>
#include <QWidget>
#include <QPropertyAnimation>

struct plAssetCuratorEvent;
struct plToolsProjectEvent;
struct plAssetProcessorEvent;
class QToolButton;

/// \brief
class PLASMA_EDITORFRAMEWORK_DLL plQtCuratorControl : public QAbstractButton
{
  Q_OBJECT
public:
  Q_PROPERTY(int curatorWidth READ getCuratorWidth WRITE setCuratorWidth)


public:
  explicit plQtCuratorControl(QWidget* pParent, plQtAssetCuratorPanel* pCuratorPanel, QSplitter* pCuratorSplitter);
  ~plQtCuratorControl();

  int getCuratorWidth() const { return m_iCuratorWidth; };
  void setCuratorWidth(int newWidth);


protected:
  virtual void paintEvent(QPaintEvent* e) override;

private Q_SLOTS:
  void SlotUpdateTransformStats();
  void UpdateBackgroundProcessState();
  void BackgroundProcessClicked(bool checked);
  void AnimateCurator();

private:
  void ScheduleUpdateTransformStats();
  void AssetCuratorEvents(const plAssetCuratorEvent& e);
  void AssetProcessorEvents(const plAssetProcessorEvent& e);
  void ProjectEvents(const plToolsProjectEvent& e);

  //curatorPanel stuff
  int m_iCuratorWidth;
  QSplitter* m_pCuratorSplitter;
  plQtAssetCuratorPanel* m_pCuratorPanel;
  QPropertyAnimation* m_pCuratorAnimation;

  bool m_bScheduled;
  QToolButton* m_pBackgroundProcess;
};

