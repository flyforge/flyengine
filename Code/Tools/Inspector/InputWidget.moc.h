#pragma once

#include <Core/Input/InputManager.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_InputWidget.h>
#include <ads/DockWidget.h>

class plQtInputWidget : public ads::CDockWidget, public Ui_InputWidget
{
public:
  Q_OBJECT

public:
  plQtInputWidget(QWidget* pParent = 0);

  static plQtInputWidget* s_pWidget;

private Q_SLOTS:
  virtual void on_ButtonClearSlots_clicked();
  virtual void on_ButtonClearActions_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  void ClearSlots();
  void ClearActions();

  void UpdateSlotTable(bool bRecreate);
  void UpdateActionTable(bool bRecreate);

  struct SlotData
  {
    plInt32 m_iTableRow;
    plUInt16 m_uiSlotFlags;
    plKeyState::Enum m_KeyState;
    float m_fValue;
    float m_fDeadZone;

    SlotData()
    {
      m_iTableRow = -1;
      m_uiSlotFlags = 0;
      m_KeyState = plKeyState::Up;
      m_fValue = 0;
      m_fDeadZone = 0;
    }
  };

  plMap<plString, SlotData> m_InputSlots;

  struct ActionData
  {
    plInt32 m_iTableRow;
    plKeyState::Enum m_KeyState;
    float m_fValue;
    bool m_bUseTimeScaling;

    plString m_sTrigger[plInputActionConfig::MaxInputSlotAlternatives];
    float m_fTriggerScaling[plInputActionConfig::MaxInputSlotAlternatives];

    ActionData()
    {
      m_iTableRow = -1;
      m_KeyState = plKeyState::Up;
      m_fValue = 0;
      m_bUseTimeScaling = false;
    }
  };

  plMap<plString, ActionData> m_InputActions;
};

