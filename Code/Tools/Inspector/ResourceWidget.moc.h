#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_ResourceWidget.h>
#include <ads/DockWidget.h>

class plQtResourceWidget : public ads::CDockWidget, public Ui_ResourceWidget
{
public:
  Q_OBJECT

public:
  plQtResourceWidget(QWidget* pParent = 0);

  static plQtResourceWidget* s_pWidget;

private Q_SLOTS:

  void on_LineFilterByName_textChanged();
  void on_ComboResourceTypes_currentIndexChanged(int state);
  void on_CheckShowDeleted_toggled(bool checked);
  void on_ButtonSave_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

  void UpdateTable();

private:
  void UpdateAll();

  struct ResourceData
  {
    ResourceData()
    {
      m_pMainItem = nullptr;
      m_bUpdate = true;
    }

    bool m_bUpdate;
    QTableWidgetItem* m_pMainItem;
    plString m_sResourceID;
    plString m_sResourceType;
    plResourcePriority m_Priority;
    plBitflags<plResourceFlags> m_Flags;
    plResourceLoadDesc m_LoadingState;
    plResource::MemoryUsage m_Memory;
    plString m_sResourceDescription;
  };

  bool m_bShowDeleted;
  plString m_sTypeFilter;
  plString m_sNameFilter;
  plTime m_LastTableUpdate;
  bool m_bUpdateTable;

  bool m_bUpdateTypeBox;
  plSet<plString> m_ResourceTypes;
  plHashTable<plUInt64, ResourceData> m_Resources;
};

