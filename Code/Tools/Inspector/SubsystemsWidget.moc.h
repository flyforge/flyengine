#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_SubsystemsWidget.h>
#include <ads/DockWidget.h>

class plQtSubsystemsWidget : public ads::CDockWidget, public Ui_SubsystemsWidget
{
public:
  Q_OBJECT

public:
  plQtSubsystemsWidget(QWidget* pParent = 0);

  static plQtSubsystemsWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  void UpdateSubSystems();

  struct SubsystemData
  {
    plString m_sPlugin;
    bool m_bStartupDone[plStartupStage::ENUM_COUNT];
    plString m_sDependencies;
  };

  bool m_bUpdateSubsystems;
  plMap<plString, SubsystemData> m_Subsystems;
};

