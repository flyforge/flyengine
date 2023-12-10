#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_TimeWidget.h>
#include <QGraphicsView>
#include <QListWidgetItem>
#include <ads/DockWidget.h>

class plQtTimeWidget : public ads::CDockWidget, public Ui_TimeWidget
{
public:
  Q_OBJECT

public:
  static const plUInt8 s_uiMaxColors = 9;

  plQtTimeWidget(QWidget* pParent = 0);

  static plQtTimeWidget* s_pWidget;

private Q_SLOTS:

  void on_ListClocks_itemChanged(QListWidgetItem* item);
  void on_ComboTimeframe_currentIndexChanged(int index);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  QGraphicsPathItem* m_pPath[s_uiMaxColors];
  QGraphicsPathItem* m_pPathMax;
  QGraphicsScene m_Scene;

  plUInt32 m_uiMaxSamples;

  plUInt8 m_uiColorsUsed;
  bool m_bClocksChanged;

  plTime m_MaxGlobalTime;
  plTime m_DisplayInterval;
  plTime m_LastUpdatedClockList;

  struct TimeSample
  {
    plTime m_AtGlobalTime;
    plTime m_Timestep;
  };

  struct ClockData
  {
    plDeque<TimeSample> m_TimeSamples;

    bool m_bDisplay = true;
    plUInt8 m_uiColor = 0xFF;
    plTime m_MinTimestep = plTime::Seconds(60.0);
    plTime m_MaxTimestep;
    QListWidgetItem* m_pListItem = nullptr;
  };

  plMap<plString, ClockData> m_ClockData;
};

