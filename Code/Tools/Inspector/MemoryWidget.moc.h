#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_MemoryWidget.h>
#include <QAction>
#include <QGraphicsView>
#include <QPointer>
#include <ads/DockWidget.h>

class QTreeWidgetItem;

class plQtMemoryWidget : public ads::CDockWidget, public Ui_MemoryWidget
{
public:
  Q_OBJECT

public:
  static const plUInt8 s_uiMaxColors = 9;

  plQtMemoryWidget(QWidget* pParent = 0);

  static plQtMemoryWidget* s_pWidget;

private Q_SLOTS:

  void on_ListAllocators_itemChanged(QTreeWidgetItem* item);
  void on_ComboTimeframe_currentIndexChanged(int index);
  void on_actionEnableOnlyThis_triggered(bool);
  void on_actionEnableAll_triggered(bool);
  void on_actionDisableAll_triggered(bool);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  void CustomContextMenuRequested(const QPoint& pos);

  QGraphicsPathItem* m_pPath[s_uiMaxColors];
  QGraphicsPathItem* m_pPathMax;
  QGraphicsScene m_Scene;

  plTime m_LastUsedMemoryStored;
  plTime m_LastUpdatedAllocatorList;

  plUInt32 m_uiMaxSamples;
  plUInt32 m_uiDisplaySamples;

  plUInt8 m_uiColorsUsed;
  bool m_bAllocatorsChanged;

  struct AllocatorData
  {
    plDeque<plUInt64> m_UsedMemory;
    plString m_sName;

    bool m_bStillInUse = true;
    bool m_bReceivedData = false;
    bool m_bDisplay = true;
    plUInt8 m_uiColor = 0xFF;
    plUInt32 m_uiParentId = plInvalidIndex;
    plUInt64 m_uiAllocs = 0;
    plUInt64 m_uiDeallocs = 0;
    plUInt64 m_uiLiveAllocs = 0;
    plUInt64 m_uiMaxUsedMemoryRecently = 0;
    plUInt64 m_uiMaxUsedMemory = 0;
    QTreeWidgetItem* m_pTreeItem = nullptr;
  };

  AllocatorData m_Accu;

  plMap<plUInt32, AllocatorData> m_AllocatorData;
};

