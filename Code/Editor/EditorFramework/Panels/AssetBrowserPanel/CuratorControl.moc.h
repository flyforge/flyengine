#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>
#include <QWidget>

struct plAssetCuratorEvent;
struct plToolsProjectEvent;
struct plAssetProcessorEvent;
class QToolButton;

/// \brief
class PLASMA_EDITORFRAMEWORK_DLL plQtCuratorControl : public QWidget
{
  Q_OBJECT
public:
  explicit plQtCuratorControl(QWidget* pParent);
  ~plQtCuratorControl();

protected:
  virtual void paintEvent(QPaintEvent* e) override;

private Q_SLOTS:
  void SlotUpdateTransformStats();
  void UpdateBackgroundProcessState();
  void BackgroundProcessClicked(bool checked);

private:
  void ScheduleUpdateTransformStats();
  void AssetCuratorEvents(const plAssetCuratorEvent& e);
  void AssetProcessorEvents(const plAssetProcessorEvent& e);
  void ProjectEvents(const plToolsProjectEvent& e);

  bool m_bScheduled = false;
  QToolButton* m_pBackgroundProcess = nullptr;
};

