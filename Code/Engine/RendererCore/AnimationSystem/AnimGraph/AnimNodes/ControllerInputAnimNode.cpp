#include <RendererCore/RendererCorePCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/ControllerInputAnimNode.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plControllerInputAnimNode, 1, plRTTIDefaultAllocator<plControllerInputAnimNode>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("LeftStickX", m_OutLeftStickX)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("LeftStickY", m_OutLeftStickY)->AddAttributes(new plHiddenAttribute()),

    PL_MEMBER_PROPERTY("RightStickX", m_OutRightStickX)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("RightStickY", m_OutRightStickY)->AddAttributes(new plHiddenAttribute()),

    PL_MEMBER_PROPERTY("LeftTrigger", m_OutLeftTrigger)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("RightTrigger", m_OutRightTrigger)->AddAttributes(new plHiddenAttribute()),

    PL_MEMBER_PROPERTY("ButtonA", m_OutButtonA)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("ButtonB", m_OutButtonB)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("ButtonX", m_OutButtonX)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("ButtonY", m_OutButtonY)->AddAttributes(new plHiddenAttribute()),

    PL_MEMBER_PROPERTY("LeftShoulder", m_OutLeftShoulder)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("RightShoulder", m_OutRightShoulder)->AddAttributes(new plHiddenAttribute()),

    PL_MEMBER_PROPERTY("PadLeft", m_OutPadLeft)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("PadRight", m_OutPadRight)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("PadUp", m_OutPadUp)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("PadDown", m_OutPadDown)->AddAttributes(new plHiddenAttribute()),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Input"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Pink)),
    new plTitleAttribute("Controller"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plControllerInputAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  PL_SUCCEED_OR_RETURN(m_OutLeftStickX.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutLeftStickY.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutRightStickX.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutRightStickY.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutLeftTrigger.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutRightTrigger.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutButtonA.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutButtonB.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutButtonX.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutButtonY.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutLeftShoulder.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutRightShoulder.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPadLeft.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPadRight.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPadUp.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPadDown.Serialize(stream));

  return PL_SUCCESS;
}

plResult plControllerInputAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  PL_SUCCEED_OR_RETURN(m_OutLeftStickX.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutLeftStickY.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutRightStickX.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutRightStickY.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutLeftTrigger.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutRightTrigger.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutButtonA.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutButtonB.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutButtonX.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutButtonY.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutLeftShoulder.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutRightShoulder.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPadLeft.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPadRight.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPadUp.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutPadDown.Deserialize(stream));

  return PL_SUCCESS;
}

void plControllerInputAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  {
    float fValue1 = 0.0f;
    float fValue2 = 0.0f;

    plInputManager::GetInputSlotState(plInputSlot_Controller0_LeftStick_NegX, &fValue1);
    plInputManager::GetInputSlotState(plInputSlot_Controller0_LeftStick_PosX, &fValue2);
    m_OutLeftStickX.SetNumber(ref_graph, -fValue1 + fValue2);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_LeftStick_NegY, &fValue1);
    plInputManager::GetInputSlotState(plInputSlot_Controller0_LeftStick_PosY, &fValue2);
    m_OutLeftStickY.SetNumber(ref_graph, -fValue1 + fValue2);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_RightStick_NegX, &fValue1);
    plInputManager::GetInputSlotState(plInputSlot_Controller0_RightStick_PosX, &fValue2);
    m_OutRightStickX.SetNumber(ref_graph, -fValue1 + fValue2);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_RightStick_NegY, &fValue1);
    plInputManager::GetInputSlotState(plInputSlot_Controller0_RightStick_PosY, &fValue2);
    m_OutRightStickY.SetNumber(ref_graph, -fValue1 + fValue2);
  }

  {
    float fValue = 0.0f;
    plInputManager::GetInputSlotState(plInputSlot_Controller0_ButtonA, &fValue);
    m_OutButtonA.SetBool(ref_graph, fValue > 0);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_ButtonB, &fValue);
    m_OutButtonB.SetBool(ref_graph, fValue > 0);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_ButtonX, &fValue);
    m_OutButtonX.SetBool(ref_graph, fValue > 0);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_ButtonY, &fValue);
    m_OutButtonY.SetBool(ref_graph, fValue > 0);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_LeftShoulder, &fValue);
    m_OutLeftShoulder.SetBool(ref_graph, fValue > 0);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_LeftTrigger, &fValue);
    m_OutLeftTrigger.SetNumber(ref_graph, fValue);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_RightShoulder, &fValue);
    m_OutRightShoulder.SetBool(ref_graph, fValue > 0);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_RightTrigger, &fValue);
    m_OutRightTrigger.SetNumber(ref_graph, fValue);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_PadLeft, &fValue);
    m_OutPadLeft.SetBool(ref_graph, fValue > 0);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_PadRight, &fValue);
    m_OutPadRight.SetBool(ref_graph, fValue > 0);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_PadUp, &fValue);
    m_OutPadUp.SetBool(ref_graph, fValue > 0);

    plInputManager::GetInputSlotState(plInputSlot_Controller0_PadDown, &fValue);
    m_OutPadDown.SetBool(ref_graph, fValue > 0);
  }
}

PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_ControllerInputAnimNode);
