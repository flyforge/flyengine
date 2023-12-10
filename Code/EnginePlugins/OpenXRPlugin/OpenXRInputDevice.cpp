#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Profiling/Profiling.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRInputDevice.h>
#include <OpenXRPlugin/OpenXRSingleton.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plOpenXRInputDevice, 1, plRTTINoAllocator);
// no properties or message handlers
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on


#define XR_Trigger "trigger_value"

#define XR_Select_Click "select_click"
#define XR_Menu_Click "menu_click"
#define XR_Squeple_Click "squeple_click"

#define XR_Primary_Analog_Stick_Axis "primary_analog_stick"
#define XR_Primary_Analog_Stick_Click "primary_analog_stick_click"
#define XR_Primary_Analog_Stick_Touch "primary_analog_stick_touch"

#define XR_Secondary_Analog_Stick_Axis "secondary_analog_stick"
#define XR_Secondary_Analog_Stick_Click "secondary_analog_stick_click"
#define XR_Secondary_Analog_Stick_Touch "secondary_analog_stick_touch"

#define XR_Grip_Pose "grip_pose"
#define XR_Aim_Pose "aim_pose"

void plOpenXRInputDevice::GetDeviceList(plHybridArray<plXRDeviceID, 64>& out_Devices) const
{
  PLASMA_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'Initialize' first.");
  out_Devices.PushBack(0);
  for (plXRDeviceID i = 0; i < 3; i++)
  {
    if (m_DeviceState[i].m_bDeviceIsConnected)
    {
      out_Devices.PushBack(i);
    }
  }
}

plXRDeviceID plOpenXRInputDevice::GetDeviceIDByType(plXRDeviceType::Enum type) const
{
  plXRDeviceID deviceID = -1;
  switch (type)
  {
    case plXRDeviceType::HMD:
      deviceID = 0;
      break;
    case plXRDeviceType::LeftController:
      deviceID = m_iLeftControllerDeviceID;
      break;
    case plXRDeviceType::RightController:
      deviceID = m_iRightControllerDeviceID;
      break;
    default:
      deviceID = -1;
      break;
  }

  if (deviceID != -1 && !m_DeviceState[deviceID].m_bDeviceIsConnected)
  {
    deviceID = -1;
  }
  return deviceID;
}

const plXRDeviceState& plOpenXRInputDevice::GetDeviceState(plXRDeviceID iDeviceID) const
{
  PLASMA_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'Initialize' first.");
  PLASMA_ASSERT_DEV(iDeviceID < 3 && iDeviceID >= 0, "Invalid device ID.");
  PLASMA_ASSERT_DEV(m_DeviceState[iDeviceID].m_bDeviceIsConnected, "Invalid device ID.");
  return m_DeviceState[iDeviceID];
}

plString plOpenXRInputDevice::GetDeviceName(plXRDeviceID iDeviceID) const
{
  PLASMA_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'Initialize' first.");
  PLASMA_ASSERT_DEV(iDeviceID < 3 && iDeviceID >= 0, "Invalid device ID.");
  PLASMA_ASSERT_DEV(m_DeviceState[iDeviceID].m_bDeviceIsConnected, "Invalid device ID.");
  return m_sActiveProfile[iDeviceID];
}

plBitflags<plXRDeviceFeatures> plOpenXRInputDevice::GetDeviceFeatures(plXRDeviceID iDeviceID) const
{
  PLASMA_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'Initialize' first.");
  PLASMA_ASSERT_DEV(iDeviceID < 3 && iDeviceID >= 0, "Invalid device ID.");
  PLASMA_ASSERT_DEV(m_DeviceState[iDeviceID].m_bDeviceIsConnected, "Invalid device ID.");
  return m_SupportedFeatures[iDeviceID];
}

plOpenXRInputDevice::plOpenXRInputDevice(plOpenXR* pOpenXR)
  : plXRInputDevice()
  , m_pOpenXR(pOpenXR)
{
  m_instance = m_pOpenXR->m_instance;
}

XrResult plOpenXRInputDevice::CreateActions(XrSession session, XrSpace sceneSpace)
{
  m_session = session;

  // HMD is always connected or we wouldn't have been able to create a session.
  m_DeviceState[0] = plXRDeviceState();
  m_DeviceState[0].m_bDeviceIsConnected = true;
  m_sActiveProfile[0] = "HMD";
  m_SupportedFeatures[0] = plXRDeviceFeatures::AimPose | plXRDeviceFeatures::GripPose;

  // Controllers
  for (plUInt32 uiControllerId : {m_iLeftControllerDeviceID, m_iRightControllerDeviceID})
  {
    m_DeviceState[uiControllerId] = plXRDeviceState();
    m_sActiveProfile[uiControllerId].Clear();
  }

  XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
  plStringUtils::Copy(actionSetInfo.actionSetName, XR_MAX_ACTION_SET_NAME_SIZE, "gameplay");
  plStringUtils::Copy(actionSetInfo.localizedActionSetName, XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE, "Gameplay");
  actionSetInfo.priority = 0;
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateActionSet(m_instance, &actionSetInfo, &m_ActionSet), DestroyActions);

  m_subActionPrefix.SetCount(2);
  m_subActionPrefix[0] = "xr_hand_left_";
  m_subActionPrefix[1] = "xr_hand_right_";

  m_subActionPath.SetCount(2);
  m_subActionPath[0] = CreatePath("/user/hand/left");
  m_subActionPath[1] = CreatePath("/user/hand/right");

  XrAction Trigger = XR_NULL_HANDLE;
  XrAction SelectClick = XR_NULL_HANDLE;
  XrAction MenuClick = XR_NULL_HANDLE;
  XrAction SquepleClick = XR_NULL_HANDLE;

  XrAction PrimaryAnalogStickAxis = XR_NULL_HANDLE;
  XrAction PrimaryAnalogStickClick = XR_NULL_HANDLE;
  XrAction PrimaryAnalogStickTouch = XR_NULL_HANDLE;

  XrAction SecondaryAnalogStickAxis = XR_NULL_HANDLE;
  XrAction SecondaryAnalogStickClick = XR_NULL_HANDLE;
  XrAction SecondaryAnalogStickTouch = XR_NULL_HANDLE;

  XrAction GripPose = XR_NULL_HANDLE;
  XrAction AimPose = XR_NULL_HANDLE;

  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(plXRDeviceFeatures::Trigger, XR_Trigger, XR_ACTION_TYPE_FLOAT_INPUT, Trigger), DestroyActions);

  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(plXRDeviceFeatures::Select, XR_Select_Click, XR_ACTION_TYPE_BOOLEAN_INPUT, SelectClick), DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(plXRDeviceFeatures::Menu, XR_Menu_Click, XR_ACTION_TYPE_BOOLEAN_INPUT, MenuClick), DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(plXRDeviceFeatures::Squeple, XR_Squeple_Click, XR_ACTION_TYPE_BOOLEAN_INPUT, SquepleClick), DestroyActions);

  XR_SUCCEED_OR_CLEANUP_LOG(
    CreateAction(plXRDeviceFeatures::PrimaryAnalogStick, XR_Primary_Analog_Stick_Axis, XR_ACTION_TYPE_VECTOR2F_INPUT, PrimaryAnalogStickAxis),
    DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(
    CreateAction(plXRDeviceFeatures::PrimaryAnalogStickClick, XR_Primary_Analog_Stick_Click, XR_ACTION_TYPE_BOOLEAN_INPUT, PrimaryAnalogStickClick),
    DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(
    CreateAction(plXRDeviceFeatures::PrimaryAnalogStickTouch, XR_Primary_Analog_Stick_Touch, XR_ACTION_TYPE_BOOLEAN_INPUT, PrimaryAnalogStickTouch),
    DestroyActions);

  XR_SUCCEED_OR_CLEANUP_LOG(
    CreateAction(plXRDeviceFeatures::SecondaryAnalogStick, XR_Secondary_Analog_Stick_Axis, XR_ACTION_TYPE_VECTOR2F_INPUT, SecondaryAnalogStickAxis),
    DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(plXRDeviceFeatures::SecondaryAnalogStickClick, XR_Secondary_Analog_Stick_Click, XR_ACTION_TYPE_BOOLEAN_INPUT,
                              SecondaryAnalogStickClick),
    DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(plXRDeviceFeatures::SecondaryAnalogStickTouch, XR_Secondary_Analog_Stick_Touch, XR_ACTION_TYPE_BOOLEAN_INPUT,
                              SecondaryAnalogStickTouch),
    DestroyActions);

  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(plXRDeviceFeatures::GripPose, XR_Grip_Pose, XR_ACTION_TYPE_POSE_INPUT, GripPose), DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(plXRDeviceFeatures::AimPose, XR_Aim_Pose, XR_ACTION_TYPE_POSE_INPUT, AimPose), DestroyActions);

  Bind simpleController[] = {
    {SelectClick, "/user/hand/left/input/select"},
    {MenuClick, "/user/hand/left/input/menu"},
    {GripPose, "/user/hand/left/input/grip"},
    {AimPose, "/user/hand/left/input/aim"},

    {SelectClick, "/user/hand/right/input/select"},
    {MenuClick, "/user/hand/right/input/menu"},
    {GripPose, "/user/hand/right/input/grip"},
    {AimPose, "/user/hand/right/input/aim"},
  };
  SuggestInteractionProfileBindings("/interaction_profiles/khr/simple_controller", "Simple Controller", simpleController);

  Bind motionController[] = {
    {Trigger, "/user/hand/left/input/trigger"},
    {SelectClick, "/user/hand/left/input/trigger"},
    {MenuClick, "/user/hand/left/input/menu"},
    {SquepleClick, "/user/hand/left/input/squeple"},
    {PrimaryAnalogStickAxis, "/user/hand/left/input/thumbstick"},
    {PrimaryAnalogStickClick, "/user/hand/left/input/thumbstick"},
    {SecondaryAnalogStickAxis, "/user/hand/left/input/trackpad"},
    {SecondaryAnalogStickClick, "/user/hand/left/input/trackpad/click"},
    {SecondaryAnalogStickTouch, "/user/hand/left/input/trackpad/touch"},
    {GripPose, "/user/hand/left/input/grip"},
    {AimPose, "/user/hand/left/input/aim"},

    {Trigger, "/user/hand/right/input/trigger"},
    {SelectClick, "/user/hand/right/input/trigger"},
    {MenuClick, "/user/hand/right/input/menu"},
    {SquepleClick, "/user/hand/right/input/squeple"},
    {PrimaryAnalogStickAxis, "/user/hand/right/input/thumbstick"},
    {PrimaryAnalogStickClick, "/user/hand/right/input/thumbstick"},
    {SecondaryAnalogStickAxis, "/user/hand/right/input/trackpad"},
    {SecondaryAnalogStickClick, "/user/hand/right/input/trackpad/click"},
    {SecondaryAnalogStickTouch, "/user/hand/right/input/trackpad/touch"},
    {GripPose, "/user/hand/right/input/grip"},
    {AimPose, "/user/hand/right/input/aim"},
  };
  SuggestInteractionProfileBindings("/interaction_profiles/microsoft/motion_controller", "Mixed Reality Motion Controller", motionController);

  if (m_pOpenXR->m_extensions.m_bHandInteraction)
  {
    Bind handInteraction[] = {
      {SelectClick, "/user/hand/left/input/select"},
      {GripPose, "/user/hand/left/input/grip"},
      {AimPose, "/user/hand/left/input/aim"},

      {SelectClick, "/user/hand/right/input/select"},
      {GripPose, "/user/hand/right/input/grip"},
      {AimPose, "/user/hand/right/input/aim"},
    };
    SuggestInteractionProfileBindings("/interaction_profiles/microsoft/hand_interaction", "Hand Interaction", handInteraction);
  }


  XrActionSpaceCreateInfo spaceCreateInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
  spaceCreateInfo.poseInActionSpace = m_pOpenXR->ConvertTransform(plTransform::MakeIdentity());
  for (plUInt32 uiSide : {0, 1})
  {
    spaceCreateInfo.subactionPath = m_subActionPath[uiSide];
    spaceCreateInfo.action = GripPose;
    XR_SUCCEED_OR_CLEANUP_LOG(xrCreateActionSpace(m_session, &spaceCreateInfo, &m_gripSpace[uiSide]), DestroyActions);

    spaceCreateInfo.action = AimPose;
    XR_SUCCEED_OR_CLEANUP_LOG(xrCreateActionSpace(m_session, &spaceCreateInfo, &m_aimSpace[uiSide]), DestroyActions);
  }
  return XR_SUCCESS;
}

void plOpenXRInputDevice::DestroyActions()
{
  for (Action& action : m_booleanActions)
  {
    XR_LOG_ERROR(xrDestroyAction(action.m_Action));
  }
  m_booleanActions.Clear();

  for (Action& action : m_floatActions)
  {
    XR_LOG_ERROR(xrDestroyAction(action.m_Action));
  }
  m_floatActions.Clear();

  for (Vec2Action& action : m_vec2Actions)
  {
    XR_LOG_ERROR(xrDestroyAction(action.m_Action));
  }
  m_vec2Actions.Clear();

  for (Action& action : m_poseActions)
  {
    XR_LOG_ERROR(xrDestroyAction(action.m_Action));
  }
  m_poseActions.Clear();

  if (m_ActionSet)
  {
    XR_LOG_ERROR(xrDestroyActionSet(m_ActionSet));
    m_ActionSet = XR_NULL_HANDLE;
  }

  for (plUInt32 uiSide : {0, 1})
  {
    if (m_gripSpace[uiSide])
    {
      XR_LOG_ERROR(xrDestroySpace(m_gripSpace[uiSide]));
      m_gripSpace[uiSide] = XR_NULL_HANDLE;
    }
    if (m_aimSpace[uiSide])
    {
      XR_LOG_ERROR(xrDestroySpace(m_aimSpace[uiSide]));
      m_aimSpace[uiSide] = XR_NULL_HANDLE;
    }
  }

  for (plUInt32 i = 0; i < 3; i++)
  {
    m_sActiveProfile[i].Clear();
    m_SupportedFeatures[i].Clear();
    m_DeviceState[i].m_bDeviceIsConnected = false;
  }
}

XrPath plOpenXRInputDevice::CreatePath(const char* szPath)
{
  XrInstance instance = m_pOpenXR->m_instance;

  XrPath path;
  if (xrStringToPath(instance, szPath, &path) != XR_SUCCESS)
  {
    plLog::Error("OpenXR path conversion failure: {0}", szPath);
  }
  return path;
}

XrResult plOpenXRInputDevice::CreateAction(plXRDeviceFeatures::Enum feature, const char* actionName, XrActionType actionType, XrAction& out_action)
{
  XrActionCreateInfo actionCreateInfo{XR_TYPE_ACTION_CREATE_INFO};
  actionCreateInfo.actionType = actionType;
  actionCreateInfo.countSubactionPaths = m_subActionPath.GetCount();
  actionCreateInfo.subactionPaths = m_subActionPath.GetData();
  plStringUtils::Copy(actionCreateInfo.actionName, XR_MAX_ACTION_NAME_SIZE, actionName);
  plStringUtils::Copy(actionCreateInfo.localizedActionName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE, actionName);

  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateAction(m_ActionSet, &actionCreateInfo, &out_action), voidFunction);

  plStringBuilder sLeft(m_subActionPrefix[0], actionName);
  plStringBuilder sRight(m_subActionPrefix[1], actionName);

  switch (actionType)
  {
    case XR_ACTION_TYPE_BOOLEAN_INPUT:
      m_booleanActions.PushBack({feature, out_action, sLeft, sRight});
      break;
    case XR_ACTION_TYPE_FLOAT_INPUT:
      m_floatActions.PushBack({feature, out_action, sLeft, sRight});
      break;
    case XR_ACTION_TYPE_VECTOR2F_INPUT:
      m_vec2Actions.PushBack(Vec2Action(feature, out_action, sLeft, sRight));
      break;
    case XR_ACTION_TYPE_POSE_INPUT:
      m_poseActions.PushBack({feature, out_action, sLeft, sRight});
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  return XR_SUCCESS;
}

XrResult plOpenXRInputDevice::SuggestInteractionProfileBindings(const char* szInteractionProfile, const char* szNiceName, plArrayPtr<Bind> bindings)
{
  XrInstance instance = m_pOpenXR->m_instance;

  XrPath InteractionProfile = CreatePath(szInteractionProfile);

  plDynamicArray<XrActionSuggestedBinding> xrBindings;
  xrBindings.Reserve(bindings.GetCount());
  for (const Bind& binding : bindings)
  {
    xrBindings.PushBack({binding.action, CreatePath(binding.szPath)});
  }


  XrInteractionProfileSuggestedBinding profileBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
  profileBindings.interactionProfile = InteractionProfile;
  profileBindings.suggestedBindings = xrBindings.GetData();
  profileBindings.countSuggestedBindings = xrBindings.GetCount();
  XR_SUCCEED_OR_RETURN_LOG(xrSuggestInteractionProfileBindings(instance, &profileBindings));

  m_InteractionProfileToNiceName[InteractionProfile] = szNiceName;

  return XR_SUCCESS;
}

XrResult plOpenXRInputDevice::AttachSessionActionSets(XrSession session)
{
  m_session = session;
  XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
  plHybridArray<XrActionSet, 1> actionSets;
  actionSets.PushBack(m_ActionSet);

  attachInfo.countActionSets = actionSets.GetCount();
  attachInfo.actionSets = actionSets.GetData();
  XR_SUCCEED_OR_RETURN_LOG(xrAttachSessionActionSets(session, &attachInfo));

  return XR_SUCCESS;
}

XrResult plOpenXRInputDevice::UpdateCurrentInteractionProfile()
{
  // This function is triggered by the XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED event.
  // Unfortunately it does not seem to provide any info in regards to what top level path is affected
  // so we check both controllers again.
  auto GetActiveControllerProfile = [this](plUInt32 uiSide) -> XrPath {
    XrInteractionProfileState state{XR_TYPE_INTERACTION_PROFILE_STATE};
    XrResult res = xrGetCurrentInteractionProfile(m_session, m_subActionPath[uiSide], &state);
    if (res == XR_SUCCESS)
    {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
      if (state.interactionProfile != XR_NULL_PATH && !m_InteractionProfileToNiceName.Contains(state.interactionProfile))
      {
        char buffer[256];
        plUInt32 temp;
        xrPathToString(m_instance, state.interactionProfile, 256, &temp, buffer);
        PLASMA_REPORT_FAILURE("Unknown interaction profile was selected by the OpenXR runtime: '{}'", buffer);
      }
#endif
      return state.interactionProfile;
    }
    return XR_NULL_PATH;
  };

  for (plUInt32 uiSide : {0, 1})
  {
    const plUInt32 uiControllerId = uiSide == 0 ? m_iLeftControllerDeviceID : m_iRightControllerDeviceID;
    XrPath path = GetActiveControllerProfile(uiSide);
    m_sActiveProfile[uiControllerId] = m_InteractionProfileToNiceName[path];
  }

  UpdateActions();

  return XR_SUCCESS;
}

void plOpenXRInputDevice::InitializeDevice() {}

void plOpenXRInputDevice::RegisterInputSlots()
{
  for (const Action& action : m_booleanActions)
  {
    for (plUInt32 uiSide : {0, 1})
    {
      RegisterInputSlot(action.m_sKey[uiSide], action.m_sKey[uiSide], plInputSlotFlags::IsButton);
    }
  }
  for (const Action& action : m_floatActions)
  {
    for (plUInt32 uiSide : {0, 1})
    {
      RegisterInputSlot(action.m_sKey[uiSide], action.m_sKey[uiSide], plInputSlotFlags::IsAnalogTrigger);
    }
  }
  for (const Vec2Action& action : m_vec2Actions)
  {
    for (plUInt32 uiSide : {0, 1})
    {
      RegisterInputSlot(action.m_sKey_negx[uiSide], action.m_sKey_negx[uiSide], plInputSlotFlags::IsAnalogStick);
      RegisterInputSlot(action.m_sKey_posx[uiSide], action.m_sKey_posx[uiSide], plInputSlotFlags::IsAnalogStick);
      RegisterInputSlot(action.m_sKey_negy[uiSide], action.m_sKey_negy[uiSide], plInputSlotFlags::IsAnalogStick);
      RegisterInputSlot(action.m_sKey_posy[uiSide], action.m_sKey_posy[uiSide], plInputSlotFlags::IsAnalogStick);
    }
  }
}

XrResult plOpenXRInputDevice::UpdateActions()
{
  if (m_session == XR_NULL_HANDLE)
    return XR_SUCCESS;

  PLASMA_PROFILE_SCOPE("UpdateActions");
  const XrFrameState& frameState = m_pOpenXR->m_frameState;

  plHybridArray<XrActiveActionSet, 1> activeActionSets;
  activeActionSets.PushBack({m_ActionSet, XR_NULL_PATH});

  XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
  syncInfo.countActiveActionSets = activeActionSets.GetCount();
  syncInfo.activeActionSets = activeActionSets.GetData();
  XrResult res = xrSyncActions(m_session, &syncInfo);
  if (res == XR_SESSION_NOT_FOCUSED)
    return XR_SUCCESS;

  XR_SUCCEED_OR_RETURN_LOG(res);

  for (plUInt32 uiSide : {0, 1})
  {
    const plUInt32 uiControllerId = uiSide == 0 ? m_iLeftControllerDeviceID : m_iRightControllerDeviceID;

    for (const Action& action : m_poseActions)
    {
      XrActionStatePose state{XR_TYPE_ACTION_STATE_POSE};
      XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
      getInfo.action = action.m_Action;
      getInfo.subactionPath = m_subActionPath[uiSide];
      XR_SUCCEED_OR_RETURN_LOG(xrGetActionStatePose(m_session, &getInfo, &state));
      m_SupportedFeatures[uiControllerId].AddOrRemove(action.m_Feature, state.isActive);
    }

    for (const Action& action : m_booleanActions)
    {
      XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
      XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
      getInfo.action = action.m_Action;
      getInfo.subactionPath = m_subActionPath[uiSide];
      XR_SUCCEED_OR_RETURN_LOG(xrGetActionStateBoolean(m_session, &getInfo, &state));
      m_InputSlotValues[action.m_sKey[uiSide]] = state.currentState ? 1.0f : 0.0f;
      m_SupportedFeatures[uiControllerId].AddOrRemove(action.m_Feature, state.isActive);
    }

    for (const Action& action : m_floatActions)
    {
      XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};
      XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
      getInfo.action = action.m_Action;
      getInfo.subactionPath = m_subActionPath[uiSide];
      XR_SUCCEED_OR_RETURN_LOG(xrGetActionStateFloat(m_session, &getInfo, &state));
      m_InputSlotValues[action.m_sKey[uiSide]] = state.currentState;
      m_SupportedFeatures[uiControllerId].AddOrRemove(action.m_Feature, state.isActive);
    }

    for (const Vec2Action& action : m_vec2Actions)
    {
      XrActionStateVector2f state{XR_TYPE_ACTION_STATE_VECTOR2F};
      XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
      getInfo.action = action.m_Action;
      getInfo.subactionPath = m_subActionPath[uiSide];
      XR_SUCCEED_OR_RETURN_LOG(xrGetActionStateVector2f(m_session, &getInfo, &state));

      m_InputSlotValues[action.m_sKey_negx[uiSide]] = state.currentState.x < 0 ? -state.currentState.x : 0.0f;
      m_InputSlotValues[action.m_sKey_posx[uiSide]] = state.currentState.x > 0 ? state.currentState.x : 0.0f;
      m_InputSlotValues[action.m_sKey_negy[uiSide]] = state.currentState.y < 0 ? -state.currentState.y : 0.0f;
      m_InputSlotValues[action.m_sKey_posy[uiSide]] = state.currentState.y > 0 ? state.currentState.y : 0.0f;
      m_SupportedFeatures[uiControllerId].AddOrRemove(action.m_Feature, state.isActive);
    }

    auto UpdatePose = [](plVec3& vPosition, plQuat& qRotation, bool& m_bIsValid, const XrSpaceLocation& viewInScene) {
      if ((viewInScene.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) ==
          (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
      {
        vPosition = plOpenXR::ConvertPosition(viewInScene.pose.position);
        qRotation = plOpenXR::ConvertOrientation(viewInScene.pose.orientation);
        m_bIsValid = true;
      }
      else
      {
        m_bIsValid = false;
      }
    };

    XrInteractionProfileState state2{XR_TYPE_INTERACTION_PROFILE_STATE};
    XrResult res2 = xrGetCurrentInteractionProfile(m_session, m_subActionPath[uiSide], &state2);

    plXRDeviceState& state = m_DeviceState[uiControllerId];
    const XrTime time = frameState.predictedDisplayTime;
    XrSpaceLocation viewInScene = {XR_TYPE_SPACE_LOCATION};
    XR_SUCCEED_OR_RETURN_LOG(xrLocateSpace(m_gripSpace[uiSide], m_pOpenXR->GetBaseSpace(), time, &viewInScene));
    UpdatePose(state.m_vGripPosition, state.m_qGripRotation, state.m_bGripPoseIsValid, viewInScene);

    XR_SUCCEED_OR_RETURN_LOG(xrLocateSpace(m_aimSpace[uiSide], m_pOpenXR->GetBaseSpace(), time, &viewInScene));
    UpdatePose(state.m_vAimPosition, state.m_qAimRotation, state.m_bAimPoseIsValid, viewInScene);
  }

  UpdateControllerState();
  return XR_SUCCESS;
}

void plOpenXRInputDevice::UpdateControllerState()
{
  for (plUInt32 uiSide : {0, 1})
  {
    const plUInt32 uiControllerId = uiSide == 0 ? m_iLeftControllerDeviceID : m_iRightControllerDeviceID;
    const bool bDeviceConnected = m_SupportedFeatures[uiControllerId].IsSet(plXRDeviceFeatures::AimPose);

    if (!m_DeviceState[uiControllerId].m_bDeviceIsConnected && bDeviceConnected)
    {
      // Connected
      m_DeviceState[uiControllerId].m_bDeviceIsConnected = true;

      plXRDeviceEventData data;
      data.m_Type = plXRDeviceEventData::Type::DeviceAdded;
      data.uiDeviceID = uiControllerId;
      m_InputEvents.Broadcast(data);
    }
    else if (m_DeviceState[uiControllerId].m_bDeviceIsConnected && !bDeviceConnected)
    {
      // Disconnected
      m_DeviceState[uiControllerId] = plXRDeviceState();
      m_SupportedFeatures[uiControllerId] = plXRDeviceFeatures::None;

      plXRDeviceEventData data;
      data.m_Type = plXRDeviceEventData::Type::DeviceRemoved;
      data.uiDeviceID = uiControllerId;
      m_InputEvents.Broadcast(data);
    }
  }
}

plOpenXRInputDevice::Vec2Action::Vec2Action(plXRDeviceFeatures::Enum feature, XrAction action, plStringView sLeft, plStringView sRight)
{
  m_Feature = feature;
  m_Action = action;

  plStringView sides[2] = {sLeft, sRight};
  for (plUInt32 uiSide : {0, 1})
  {
    plStringBuilder temp = sides[uiSide];
    temp.Append("_negx");
    m_sKey_negx[uiSide] = temp;

    temp.Shrink(0, 5);
    temp.Append("_posx");
    m_sKey_posx[uiSide] = temp;

    temp.Shrink(0, 5);
    temp.Append("_negy");
    m_sKey_negy[uiSide] = temp;

    temp.Shrink(0, 5);
    temp.Append("_posy");
    m_sKey_posy[uiSide] = temp;
  }
}
