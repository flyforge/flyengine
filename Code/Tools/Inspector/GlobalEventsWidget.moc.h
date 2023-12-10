#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_GlobalEventsWidget.h>
#include <ads/DockWidget.h>

class plQtGlobalEventsWidget : public ads::CDockWidget, public Ui_GlobalEventsWidget
{
public:
  Q_OBJECT

public:
  plQtGlobalEventsWidget(QWidget* pParent = 0);

  static plQtGlobalEventsWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  void UpdateTable(bool bRecreate);

  struct GlobalEventsData
  {
    plInt32 m_iTableRow;
    plUInt32 m_uiTimesFired;
    plUInt16 m_uiNumHandlers;
    plUInt16 m_uiNumHandlersOnce;

    GlobalEventsData()
    {
      m_iTableRow = -1;

      m_uiTimesFired = 0;
      m_uiNumHandlers = 0;
      m_uiNumHandlersOnce = 0;
    }
  };

  plMap<plString, GlobalEventsData> m_Events;
};

