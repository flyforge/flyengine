#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>

struct plXRPoseLocation
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    Grip,
    Aim,
    Default = Grip,
  };
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plXRPoseLocation);

//////////////////////////////////////////////////////////////////////////

typedef plComponentManagerSimple<class plDeviceTrackingComponent, plComponentUpdateType::WhenSimulating> plDeviceTrackingComponentManager;

/// \brief Tracks the position of a XR device and applies it to the owner.
class PLASMA_GAMEENGINE_DLL plDeviceTrackingComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plDeviceTrackingComponent, plComponent, plDeviceTrackingComponentManager);

public:
  plDeviceTrackingComponent();
  ~plDeviceTrackingComponent();

  /// \brief Sets the type of device this component is going to track.
  void SetDeviceType(plEnum<plXRDeviceType> type);
  plEnum<plXRDeviceType> GetDeviceType() const;

  void SetPoseLocation(plEnum<plXRPoseLocation> poseLocation);
  plEnum<plXRPoseLocation> GetPoseLocation() const;

  /// \brief Whether to set the owner's local or global transform, see plXRTransformSpace.
  void SetTransformSpace(plEnum<plXRTransformSpace> space);
  plEnum<plXRTransformSpace> GetTransformSpace() const;

  //
  // plComponent Interface
  //

protected:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  void Update();

  plEnum<plXRDeviceType> m_DeviceType;
  plEnum<plXRPoseLocation> m_PoseLocation;
  plEnum<plXRTransformSpace> m_Space;
  bool m_bRotation = true;
  bool m_bScale = true;
};
