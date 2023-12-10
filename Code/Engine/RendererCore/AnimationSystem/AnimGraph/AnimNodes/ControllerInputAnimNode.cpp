#include <RendererCore/RendererCorePCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/ControllerInputAnimNode.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plControllerInputAnimNode, 1, plRTTIDefaultAllocator<plControllerInputAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("LeftStickX", m_OutLeftStickX)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("LeftStickY", m_OutLeftStickY)->AddAttributes(new plHiddenAttribute()),

    PLASMA_MEMBER_PROPERTY("RightStickX", m_OutRightStickX)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("RightStickY", m_OutRightStickY)->AddAttributes(new plHiddenAttribute()),

    PLASMA_MEMBER_PROPERTY("LeftTrigger", m_OutLeftTrigger)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("RightTrigger", m_OutRightTrigger)->AddAttributes(new plHiddenAttribute()),

    PLASMA_MEMBER_PROPERTY("ButtonA", m_OutButtonA)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("ButtonB", m_OutButtonB)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("ButtonX", m_OutButtonX)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("ButtonY", m_OutButtonY)->AddAttributes(new plHiddenAttribute()),

    PLASMA_MEMBER_PROPERTY("LeftShoulder", m_OutLeftShoulder)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("RightShoulder", m_OutRightShoulder)->AddAttributes(new plHiddenAttribute()),

    PLASMA_MEMBER_PROPERTY("PadLeft", m_OutPadLeft)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("PadRight", m_OutPadRight)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("PadUp", m_OutPadUp)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("PadDown", m_OutPadDown)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Input"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Pink)),
    new plTitleAttribute("Controller"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plControllerInputAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(m_OutLeftStickX.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutLeftStickY.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutRightStickX.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutRightStickY.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutLeftTrigger.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutRightTrigger.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutButtonA.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutButtonB.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutButtonX.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutButtonY.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutLeftShoulder.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutRightShoulder.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPadLeft.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPadRight.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPadUp.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPadDown.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plControllerInputAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(m_OutLeftStickX.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutLeftStickY.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutRightStickX.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutRightStickY.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutLeftTrigger.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutRightTrigger.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutButtonA.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutButtonB.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutButtonX.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutButtonY.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutLeftShoulder.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutRightShoulder.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPadLeft.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPadRight.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPadUp.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutPadDown.Deserialize(stream));

  return PLASMA_SUCCESS;
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

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_ControllerInputAnimNode);
