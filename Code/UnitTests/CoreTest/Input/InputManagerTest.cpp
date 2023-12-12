#include <CoreTest/CoreTestPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Memory/MemoryUtils.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Input);

static bool operator==(const plInputActionConfig& lhs, const plInputActionConfig& rhs)
{
  if (lhs.m_bApplyTimeScaling != rhs.m_bApplyTimeScaling)
    return false;
  if (lhs.m_fFilteredPriority != rhs.m_fFilteredPriority)
    return false;
  if (lhs.m_fFilterXMaxValue != rhs.m_fFilterXMaxValue)
    return false;
  if (lhs.m_fFilterXMinValue != rhs.m_fFilterXMinValue)
    return false;
  if (lhs.m_fFilterYMaxValue != rhs.m_fFilterYMaxValue)
    return false;
  if (lhs.m_fFilterYMinValue != rhs.m_fFilterYMinValue)
    return false;

  if (lhs.m_OnEnterArea != rhs.m_OnEnterArea)
    return false;
  if (lhs.m_OnLeaveArea != rhs.m_OnLeaveArea)
    return false;

  for (int i = 0; i < plInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    if (lhs.m_sInputSlotTrigger[i] != rhs.m_sInputSlotTrigger[i])
      return false;
    if (lhs.m_fInputSlotScale[i] != rhs.m_fInputSlotScale[i])
      return false;
    if (lhs.m_sFilterByInputSlotX[i] != rhs.m_sFilterByInputSlotX[i])
      return false;
    if (lhs.m_sFilterByInputSlotY[i] != rhs.m_sFilterByInputSlotY[i])
      return false;
  }

  return true;
}

class plTestInputDevide : public plInputDevice
{
public:
  void ActivateAll()
  {
    m_InputSlotValues["testdevice_button"] = 0.1f;
    m_InputSlotValues["testdevice_stick"] = 0.2f;
    m_InputSlotValues["testdevice_wheel"] = 0.3f;
    m_InputSlotValues["testdevice_touchpoint"] = 0.4f;
    m_uiLastCharacter = '\42';
  }

private:
  void InitializeDevice() override {}
  void UpdateInputSlotValues() override {}
  void RegisterInputSlots() override
  {
    RegisterInputSlot("testdevice_button", "", plInputSlotFlags::IsButton);
    RegisterInputSlot("testdevice_stick", "", plInputSlotFlags::IsAnalogStick);
    RegisterInputSlot("testdevice_wheel", "", plInputSlotFlags::IsMouseWheel);
    RegisterInputSlot("testdevice_touchpoint", "", plInputSlotFlags::IsTouchPoint);
  }

  void ResetInputSlotValues() override { m_InputSlotValues.Clear(); }
};

static bool operator!=(const plInputActionConfig& lhs, const plInputActionConfig& rhs)
{
  return !(lhs == rhs);
}

PLASMA_CREATE_SIMPLE_TEST(Input, InputManager)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetInputSlotDisplayName / GetInputSlotDisplayName")
  {
    plInputManager::SetInputSlotDisplayName("test_slot_1", "Test Slot 1 Name");
    plInputManager::SetInputSlotDisplayName("test_slot_2", "Test Slot 2 Name");
    plInputManager::SetInputSlotDisplayName("test_slot_3", "Test Slot 3 Name");
    plInputManager::SetInputSlotDisplayName("test_slot_4", "Test Slot 4 Name");

    PLASMA_TEST_STRING(plInputManager::GetInputSlotDisplayName("test_slot_1"), "Test Slot 1 Name");
    PLASMA_TEST_STRING(plInputManager::GetInputSlotDisplayName("test_slot_2"), "Test Slot 2 Name");
    PLASMA_TEST_STRING(plInputManager::GetInputSlotDisplayName("test_slot_3"), "Test Slot 3 Name");
    PLASMA_TEST_STRING(plInputManager::GetInputSlotDisplayName("test_slot_4"), "Test Slot 4 Name");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetInputSlotDeadZone / GetInputSlotDisplayName")
  {
    plInputManager::SetInputSlotDeadZone("test_slot_1", 0.1f);
    plInputManager::SetInputSlotDeadZone("test_slot_2", 0.2f);
    plInputManager::SetInputSlotDeadZone("test_slot_3", 0.3f);
    plInputManager::SetInputSlotDeadZone("test_slot_4", 0.4f);

    PLASMA_TEST_FLOAT(plInputManager::GetInputSlotDeadZone("test_slot_1"), 0.1f, 0.0f);
    PLASMA_TEST_FLOAT(plInputManager::GetInputSlotDeadZone("test_slot_2"), 0.2f, 0.0f);
    PLASMA_TEST_FLOAT(plInputManager::GetInputSlotDeadZone("test_slot_3"), 0.3f, 0.0f);
    PLASMA_TEST_FLOAT(plInputManager::GetInputSlotDeadZone("test_slot_4"), 0.4f, 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetInputActionConfig / GetInputActionConfig")
  {
    plInputActionConfig iac1, iac2;
    iac1.m_bApplyTimeScaling = true;
    iac1.m_fFilteredPriority = 23.0f;
    iac1.m_fInputSlotScale[0] = 2.0f;
    iac1.m_fInputSlotScale[1] = 3.0f;
    iac1.m_fInputSlotScale[2] = 4.0f;
    iac1.m_sInputSlotTrigger[0] = plInputSlot_Key0;
    iac1.m_sInputSlotTrigger[1] = plInputSlot_Key1;
    iac1.m_sInputSlotTrigger[2] = plInputSlot_Key2;

    iac2.m_bApplyTimeScaling = false;
    iac2.m_fFilteredPriority = 42.0f;
    iac2.m_fInputSlotScale[0] = 4.0f;
    iac2.m_fInputSlotScale[1] = 5.0f;
    iac2.m_fInputSlotScale[2] = 6.0f;
    iac2.m_sInputSlotTrigger[0] = plInputSlot_Key3;
    iac2.m_sInputSlotTrigger[1] = plInputSlot_Key4;
    iac2.m_sInputSlotTrigger[2] = plInputSlot_Key5;

    plInputManager::SetInputActionConfig("test_inputset", "test_action_1", iac1, true);
    plInputManager::SetInputActionConfig("test_inputset", "test_action_2", iac2, true);

    PLASMA_TEST_BOOL(plInputManager::GetInputActionConfig("test_inputset", "test_action_1") == iac1);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionConfig("test_inputset", "test_action_2") == iac2);

    plInputManager::SetInputActionConfig("test_inputset", "test_action_3", iac1, false);
    plInputManager::SetInputActionConfig("test_inputset", "test_action_4", iac2, false);

    PLASMA_TEST_BOOL(plInputManager::GetInputActionConfig("test_inputset", "test_action_1") == iac1);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionConfig("test_inputset", "test_action_2") == iac2);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionConfig("test_inputset", "test_action_3") == iac1);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionConfig("test_inputset", "test_action_4") == iac2);

    plInputManager::SetInputActionConfig("test_inputset", "test_action_3", iac1, true);
    plInputManager::SetInputActionConfig("test_inputset", "test_action_4", iac2, true);

    PLASMA_TEST_BOOL(plInputManager::GetInputActionConfig("test_inputset", "test_action_1") != iac1);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionConfig("test_inputset", "test_action_2") != iac2);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionConfig("test_inputset", "test_action_3") == iac1);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionConfig("test_inputset", "test_action_4") == iac2);


    plInputManager::RemoveInputAction("test_inputset", "test_action_1");
    plInputManager::RemoveInputAction("test_inputset", "test_action_2");
    plInputManager::RemoveInputAction("test_inputset", "test_action_3");
    plInputManager::RemoveInputAction("test_inputset", "test_action_4");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Input Slot State Changes / Dead Zones")
  {
    float f = 0;
    plInputManager::InjectInputSlotValue("test_slot_1", 0.0f);
    plInputManager::SetInputSlotDeadZone("test_slot_1", 0.25f);

    // just check the first state
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotState("test_slot_1", &f) == plKeyState::Up);
    PLASMA_TEST_FLOAT(f, 0.0f, 0);

    // value is not yet propagated
    plInputManager::InjectInputSlotValue("test_slot_1", 1.0f);
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotState("test_slot_1", &f) == plKeyState::Up);
    PLASMA_TEST_FLOAT(f, 0.0f, 0);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotState("test_slot_1", &f) == plKeyState::Pressed);
    PLASMA_TEST_FLOAT(f, 1.0f, 0);

    plInputManager::InjectInputSlotValue("test_slot_1", 0.5f);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotState("test_slot_1", &f) == plKeyState::Down);
    PLASMA_TEST_FLOAT(f, 0.5f, 0);

    plInputManager::InjectInputSlotValue("test_slot_1", 0.3f);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotState("test_slot_1", &f) == plKeyState::Down);
    PLASMA_TEST_FLOAT(f, 0.3f, 0);

    // below dead zone value
    plInputManager::InjectInputSlotValue("test_slot_1", 0.2f);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotState("test_slot_1", &f) == plKeyState::Released);
    PLASMA_TEST_FLOAT(f, 0.0f, 0);

    plInputManager::InjectInputSlotValue("test_slot_1", 0.5f);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotState("test_slot_1", &f) == plKeyState::Pressed);
    PLASMA_TEST_FLOAT(f, 0.5f, 0);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotState("test_slot_1", &f) == plKeyState::Released);
    PLASMA_TEST_FLOAT(f, 0.0f, 0);

    plInputManager::InjectInputSlotValue("test_slot_1", 0.2f);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotState("test_slot_1", &f) == plKeyState::Up);
    PLASMA_TEST_FLOAT(f, 0.0f, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetActionDisplayName / GetActionDisplayName")
  {
    plInputManager::SetActionDisplayName("test_action_1", "Test Action 1 Name");
    plInputManager::SetActionDisplayName("test_action_2", "Test Action 2 Name");
    plInputManager::SetActionDisplayName("test_action_3", "Test Action 3 Name");
    plInputManager::SetActionDisplayName("test_action_4", "Test Action 4 Name");

    PLASMA_TEST_STRING(plInputManager::GetActionDisplayName("test_action_0"), "test_action_0");
    PLASMA_TEST_STRING(plInputManager::GetActionDisplayName("test_action_1"), "Test Action 1 Name");
    PLASMA_TEST_STRING(plInputManager::GetActionDisplayName("test_action_2"), "Test Action 2 Name");
    PLASMA_TEST_STRING(plInputManager::GetActionDisplayName("test_action_3"), "Test Action 3 Name");
    PLASMA_TEST_STRING(plInputManager::GetActionDisplayName("test_action_4"), "Test Action 4 Name");
    PLASMA_TEST_STRING(plInputManager::GetActionDisplayName("test_action_5"), "test_action_5");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Input Sets")
  {
    plInputActionConfig iac;
    plInputManager::SetInputActionConfig("test_inputset", "test_action_1", iac, true);
    plInputManager::SetInputActionConfig("test_inputset2", "test_action_2", iac, true);

    plDynamicArray<plString> InputSetNames;
    plInputManager::GetAllInputSets(InputSetNames);

    PLASMA_TEST_INT(InputSetNames.GetCount(), 2);

    PLASMA_TEST_STRING(InputSetNames[0].GetData(), "test_inputset");
    PLASMA_TEST_STRING(InputSetNames[1].GetData(), "test_inputset2");

    plInputManager::RemoveInputAction("test_inputset", "test_action_1");
    plInputManager::RemoveInputAction("test_inputset2", "test_action_2");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAllInputActions / RemoveInputAction")
  {
    plHybridArray<plString, 24> InputActions;

    plInputManager::GetAllInputActions("test_inputset_3", InputActions);

    PLASMA_TEST_BOOL(InputActions.IsEmpty());

    plInputActionConfig iac;
    plInputManager::SetInputActionConfig("test_inputset_3", "test_action_1", iac, true);
    plInputManager::SetInputActionConfig("test_inputset_3", "test_action_2", iac, true);
    plInputManager::SetInputActionConfig("test_inputset_3", "test_action_3", iac, true);

    plInputManager::GetAllInputActions("test_inputset_3", InputActions);

    PLASMA_TEST_INT(InputActions.GetCount(), 3);

    PLASMA_TEST_STRING(InputActions[0].GetData(), "test_action_1");
    PLASMA_TEST_STRING(InputActions[1].GetData(), "test_action_2");
    PLASMA_TEST_STRING(InputActions[2].GetData(), "test_action_3");


    plInputManager::RemoveInputAction("test_inputset_3", "test_action_2");

    plInputManager::GetAllInputActions("test_inputset_3", InputActions);

    PLASMA_TEST_INT(InputActions.GetCount(), 2);

    PLASMA_TEST_STRING(InputActions[0].GetData(), "test_action_1");
    PLASMA_TEST_STRING(InputActions[1].GetData(), "test_action_3");

    plInputManager::RemoveInputAction("test_inputset_3", "test_action_1");
    plInputManager::RemoveInputAction("test_inputset_3", "test_action_3");

    plInputManager::GetAllInputActions("test_inputset_3", InputActions);

    PLASMA_TEST_BOOL(InputActions.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Input Action State Changes")
  {
    plInputActionConfig iac;
    iac.m_bApplyTimeScaling = false;
    iac.m_sInputSlotTrigger[0] = "test_input_slot_1";
    iac.m_sInputSlotTrigger[1] = "test_input_slot_2";
    iac.m_sInputSlotTrigger[2] = "test_input_slot_3";

    // bind the three slots to this action
    plInputManager::SetInputActionConfig("test_inputset", "test_action", iac, true);

    // bind the same three slots to another action
    plInputManager::SetInputActionConfig("test_inputset", "test_action_2", iac, false);

    // the first slot to trigger the action is bound to it, the other slots can now trigger other actions
    // but not this one anymore
    plInputManager::InjectInputSlotValue("test_input_slot_2", 1.0f);

    float f = 0;
    plInt8 iSlot = 0;
    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == plKeyState::Up);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == plKeyState::Up);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == plKeyState::Pressed);
    PLASMA_TEST_INT(iSlot, 1);
    PLASMA_TEST_FLOAT(f, 1.0f, 0.0f);

    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == plKeyState::Pressed);
    PLASMA_TEST_INT(iSlot, 1);
    PLASMA_TEST_FLOAT(f, 1.0f, 0.0f);

    // inject all three input slots
    plInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    plInputManager::InjectInputSlotValue("test_input_slot_2", 1.0f);
    plInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == plKeyState::Down);
    PLASMA_TEST_INT(iSlot, 1); // still the same slot that 'triggered' the action
    PLASMA_TEST_FLOAT(f, 1.0f, 0.0f);

    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == plKeyState::Down);
    PLASMA_TEST_INT(iSlot, 1); // still the same slot that 'triggered' the action
    PLASMA_TEST_FLOAT(f, 1.0f, 0.0f);

    plInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    plInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == plKeyState::Released);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == plKeyState::Released);

    plInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    plInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == plKeyState::Up);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == plKeyState::Up);

    plInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == plKeyState::Pressed);
    PLASMA_TEST_INT(iSlot, 2);
    PLASMA_TEST_FLOAT(f, 1.0f, 0.0f);

    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == plKeyState::Pressed);
    PLASMA_TEST_INT(iSlot, 2);
    PLASMA_TEST_FLOAT(f, 1.0f, 0.0f);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == plKeyState::Released);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == plKeyState::Released);

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == plKeyState::Up);
    PLASMA_TEST_BOOL(plInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == plKeyState::Up);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetPressedInputSlot")
  {
    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    plStringView sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::None, plInputSlotFlags::None);
    PLASMA_TEST_BOOL(sSlot.IsEmpty());

    plInputManager::InjectInputSlotValue("test_slot", 1.0f);

    sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::None, plInputSlotFlags::None);
    PLASMA_TEST_STRING(sSlot, "");

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::None, plInputSlotFlags::None);
    PLASMA_TEST_STRING(sSlot, "test_slot");


    {
      plTestInputDevide dev;
      dev.ActivateAll();

      plInputManager::InjectInputSlotValue("test_slot", 1.0f);

      plInputManager::Update(plTime::Seconds(1.0 / 60.0));

      sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::IsButton, plInputSlotFlags::None);
      PLASMA_TEST_STRING(sSlot, "testdevice_button");

      sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::IsAnalogStick, plInputSlotFlags::None);
      PLASMA_TEST_STRING(sSlot, "testdevice_stick");

      sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::IsMouseWheel, plInputSlotFlags::None);
      PLASMA_TEST_STRING(sSlot, "testdevice_wheel");

      sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::IsTouchPoint, plInputSlotFlags::None);
      PLASMA_TEST_STRING(sSlot, "testdevice_touchpoint");

      plInputManager::InjectInputSlotValue("test_slot", 1.0f);

      plInputManager::Update(plTime::Seconds(1.0 / 60.0));

      sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::IsButton, plInputSlotFlags::None);
      PLASMA_TEST_STRING(sSlot, "");

      sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::IsAnalogStick, plInputSlotFlags::None);
      PLASMA_TEST_STRING(sSlot, "");

      sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::IsMouseWheel, plInputSlotFlags::None);
      PLASMA_TEST_STRING(sSlot, "");

      sSlot = plInputManager::GetPressedInputSlot(plInputSlotFlags::IsTouchPoint, plInputSlotFlags::None);
      PLASMA_TEST_STRING(sSlot, "");
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "LastCharacter")
  {
    plTestInputDevide dev;
    dev.ActivateAll();

    PLASMA_TEST_BOOL(plInputManager::RetrieveLastCharacter(true) == '\0');

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    PLASMA_TEST_BOOL(plInputManager::RetrieveLastCharacter(false) == '\42');
    PLASMA_TEST_BOOL(plInputManager::RetrieveLastCharacter(true) == '\42');
    PLASMA_TEST_BOOL(plInputManager::RetrieveLastCharacter(true) == '\0');
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Time Scaling")
  {
    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    plInputActionConfig iac;
    iac.m_bApplyTimeScaling = true;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    plInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    plTestInputDevide dev;
    dev.ActivateAll();

    float fVal;

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));
    plInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    PLASMA_TEST_FLOAT(fVal, 0.1f * (1.0 / 60.0), 0.0001f); // testdevice_button has a value of 0.1f

    dev.ActivateAll();

    plInputManager::Update(plTime::Seconds(1.0 / 30.0));
    plInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    PLASMA_TEST_FLOAT(fVal, 0.1f * (1.0 / 30.0), 0.0001f);


    iac.m_bApplyTimeScaling = false;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    plInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    dev.ActivateAll();

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));
    plInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    PLASMA_TEST_FLOAT(fVal, 0.1f, 0.0001f); // testdevice_button has a value of 0.1f

    dev.ActivateAll();

    plInputManager::Update(plTime::Seconds(1.0 / 30.0));
    plInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    PLASMA_TEST_FLOAT(fVal, 0.1f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetInputSlotFlags")
  {
    plTestInputDevide dev;
    plInputManager::Update(plTime::Seconds(1.0 / 30.0));

    PLASMA_TEST_BOOL(plInputManager::GetInputSlotFlags("testdevice_button") == plInputSlotFlags::IsButton);
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotFlags("testdevice_stick") == plInputSlotFlags::IsAnalogStick);
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotFlags("testdevice_wheel") == plInputSlotFlags::IsMouseWheel);
    PLASMA_TEST_BOOL(plInputManager::GetInputSlotFlags("testdevice_touchpoint") == plInputSlotFlags::IsTouchPoint);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ClearInputMapping")
  {
    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    plInputActionConfig iac;
    iac.m_bApplyTimeScaling = true;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    plInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    plTestInputDevide dev;
    dev.ActivateAll();

    float fVal;

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));
    plInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    PLASMA_TEST_FLOAT(fVal, 0.1f * (1.0 / 60.0), 0.0001f); // testdevice_button has a value of 0.1f

    // clear the button from the action
    plInputManager::ClearInputMapping("test_inputset", "testdevice_button");

    dev.ActivateAll();

    plInputManager::Update(plTime::Seconds(1.0 / 60.0));

    // should not receive input anymore
    plInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    PLASMA_TEST_FLOAT(fVal, 0.0f, 0.0001f);
  }
}
