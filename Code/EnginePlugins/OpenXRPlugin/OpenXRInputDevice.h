#pragma once

#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>

class plOpenXR;

PL_DEFINE_AS_POD_TYPE(XrActionSuggestedBinding);
PL_DEFINE_AS_POD_TYPE(XrActiveActionSet);

class PL_OPENXRPLUGIN_DLL plOpenXRInputDevice : public plXRInputDevice
{
  PL_ADD_DYNAMIC_REFLECTION(plOpenXRInputDevice, plXRInputDevice);

public:
  void GetDeviceList(plHybridArray<plXRDeviceID, 64>& out_devices) const override;
  plXRDeviceID GetDeviceIDByType(plXRDeviceType::Enum type) const override;
  const plXRDeviceState& GetDeviceState(plXRDeviceID deviceID) const override;
  plString GetDeviceName(plXRDeviceID deviceID) const override;
  plBitflags<plXRDeviceFeatures> GetDeviceFeatures(plXRDeviceID deviceID) const override;

private:
  friend class plOpenXR;
  struct Bind
  {
    XrAction action;
    const char* szPath;
  };

  struct Action
  {
    plXRDeviceFeatures::Enum m_Feature;
    XrAction m_Action;
    plString m_sKey[2];
  };

  struct Vec2Action
  {
    Vec2Action(plXRDeviceFeatures::Enum feature, XrAction pAction, plStringView sLeft, plStringView sRight);
    plXRDeviceFeatures::Enum m_Feature;
    XrAction m_Action;
    plString m_sKey_negx[2];
    plString m_sKey_posx[2];
    plString m_sKey_negy[2];
    plString m_sKey_posy[2];
  };

  plOpenXRInputDevice(plOpenXR* pOpenXR);
  XrResult CreateActions(XrSession session, XrSpace m_sceneSpace);
  void DestroyActions();

  XrPath CreatePath(const char* szPath);
  XrResult CreateAction(plXRDeviceFeatures::Enum feature, const char* actionName, XrActionType actionType, XrAction& out_action);
  XrResult SuggestInteractionProfileBindings(const char* szInteractionProfile, const char* szNiceName, plArrayPtr<Bind> bindings);
  XrResult AttachSessionActionSets(XrSession session);
  XrResult UpdateCurrentInteractionProfile();

  void InitializeDevice() override;
  void RegisterInputSlots() override;
  void UpdateInputSlotValues() override {}

  XrResult UpdateActions();
  void UpdateControllerState();

private:
  plOpenXR* m_pOpenXR = nullptr;
  XrInstance m_pInstance = XR_NULL_HANDLE;
  XrSession m_pSession = XR_NULL_HANDLE;

  plXRDeviceState m_DeviceState[3]; // Hard-coded for now
  plString m_sActiveProfile[3];
  plBitflags<plXRDeviceFeatures> m_SupportedFeatures[3];
  const plInt8 m_iLeftControllerDeviceID = 1;
  const plInt8 m_iRightControllerDeviceID = 2;

  XrActionSet m_pActionSet = XR_NULL_HANDLE;
  plHashTable<XrPath, plString> m_InteractionProfileToNiceName;

  plStaticArray<const char*, 2> m_SubActionPrefix;
  plStaticArray<XrPath, 2> m_SubActionPath;

  plHybridArray<Action, 4> m_BooleanActions;
  plHybridArray<Action, 4> m_FloatActions;
  plHybridArray<Vec2Action, 4> m_Vec2Actions;
  plHybridArray<Action, 4> m_PoseActions;

  XrSpace m_gripSpace[2];
  XrSpace m_aimSpace[2];
};
