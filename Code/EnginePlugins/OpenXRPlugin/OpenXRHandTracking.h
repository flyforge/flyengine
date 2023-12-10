#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/XRHandTrackingInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

class plOpenXR;

class PLASMA_OPENXRPLUGIN_DLL plOpenXRHandTracking : public plXRHandTrackingInterface
{
  PLASMA_DECLARE_SINGLETON_OF_INTERFACE(plOpenXRHandTracking, plXRHandTrackingInterface);

public:
  static bool IsHandTrackingSupported(plOpenXR* pOpenXR);

public:
  plOpenXRHandTracking(plOpenXR* pOpenXR);
  ~plOpenXRHandTracking();

  HandPartTrackingState TryGetBoneTransforms(
    plEnum<plXRHand> hand, plEnum<plXRHandPart> handPart, plEnum<plXRTransformSpace> space, plDynamicArray<plXRHandBone>& out_bones) override;

  void UpdateJointTransforms();

private:
  friend class plOpenXR;

  struct JointData
  {
    PLASMA_DECLARE_POD_TYPE();
    plXRHandBone m_Bone;
    bool m_bValid;
  };

  plOpenXR* m_pOpenXR = nullptr;
  XrHandTrackerEXT m_HandTracker[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
  XrHandJointLocationEXT m_JointLocations[2][XR_HAND_JOINT_COUNT_EXT];
  XrHandJointVelocityEXT m_JointVelocities[2][XR_HAND_JOINT_COUNT_EXT];
  XrHandJointLocationsEXT m_Locations[2]{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
  XrHandJointVelocitiesEXT m_Velocities[2]{XR_TYPE_HAND_JOINT_VELOCITIES_EXT};

  plStaticArray<JointData, XR_HAND_JOINT_LITTLE_TIP_EXT + 1> m_JointData[2];
  plStaticArray<plUInt32, 6> m_HandParts[plXRHandPart::Little + 1];
};
