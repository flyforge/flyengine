#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/XR/DeviceTrackingComponent.h>
#include <GameEngine/XR/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plXRPoseLocation, 1)
PLASMA_BITFLAGS_CONSTANTS(plXRPoseLocation::Grip, plXRPoseLocation::Aim)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_COMPONENT_TYPE(plDeviceTrackingComponent, 3, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_ACCESSOR_PROPERTY("DeviceType", plXRDeviceType, GetDeviceType, SetDeviceType),
    PLASMA_ENUM_ACCESSOR_PROPERTY("PoseLocation", plXRPoseLocation, GetPoseLocation, SetPoseLocation),
    PLASMA_ENUM_ACCESSOR_PROPERTY("TransformSpace", plXRTransformSpace, GetTransformSpace, SetTransformSpace),
    PLASMA_MEMBER_PROPERTY("Rotation", m_bRotation)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("Scale", m_bScale)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("XR"),
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Alpha),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plDeviceTrackingComponent::plDeviceTrackingComponent() = default;
plDeviceTrackingComponent::~plDeviceTrackingComponent() = default;

void plDeviceTrackingComponent::SetDeviceType(plEnum<plXRDeviceType> type)
{
  m_DeviceType = type;
}

plEnum<plXRDeviceType> plDeviceTrackingComponent::GetDeviceType() const
{
  return m_DeviceType;
}

void plDeviceTrackingComponent::SetPoseLocation(plEnum<plXRPoseLocation> poseLocation)
{
  m_PoseLocation = poseLocation;
}

plEnum<plXRPoseLocation> plDeviceTrackingComponent::GetPoseLocation() const
{
  return m_PoseLocation;
}

void plDeviceTrackingComponent::SetTransformSpace(plEnum<plXRTransformSpace> space)
{
  m_Space = space;
}

plEnum<plXRTransformSpace> plDeviceTrackingComponent::GetTransformSpace() const
{
  return m_Space;
}

void plDeviceTrackingComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  plStreamWriter& s = stream.GetStream();

  s << m_DeviceType;
  s << m_PoseLocation;
  s << m_Space;
  s << m_bRotation;
  s << m_bScale;
}

void plDeviceTrackingComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_DeviceType;
  if (uiVersion >= 2)
  {
    s >> m_PoseLocation;
  }
  s >> m_Space;
  if (uiVersion >= 3)
  {
    s >> m_bRotation;
    s >> m_bScale;
  }
}

void plDeviceTrackingComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  if (plXRInterface* pXRInterface = plSingletonRegistry::GetSingletonInstance<plXRInterface>())
  {
    if (!pXRInterface->IsInitialized())
      return;

    plXRDeviceID deviceID = pXRInterface->GetXRInput().GetDeviceIDByType(m_DeviceType);
    if (deviceID != -1)
    {
      const plXRDeviceState& state = pXRInterface->GetXRInput().GetDeviceState(deviceID);
      plVec3 vPosition;
      plQuat qRotation;
      if (m_PoseLocation == plXRPoseLocation::Grip && state.m_bGripPoseIsValid)
      {
        vPosition = state.m_vGripPosition;
        qRotation = state.m_qGripRotation;
      }
      else if (m_PoseLocation == plXRPoseLocation::Aim && state.m_bAimPoseIsValid)
      {
        vPosition = state.m_vAimPosition;
        qRotation = state.m_qAimRotation;
      }
      else
      {
        return;
      }
      if (m_Space == plXRTransformSpace::Local)
      {
        GetOwner()->SetLocalPosition(vPosition);
        if (m_bRotation)
          GetOwner()->SetLocalRotation(qRotation);
      }
      else
      {
        plTransform add;
        add.SetIdentity();
        if (const plStageSpaceComponentManager* pStageMan = GetWorld()->GetComponentManager<plStageSpaceComponentManager>())
        {
          if (const plStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
          {
            add = pStage->GetOwner()->GetGlobalTransform();
          }
        }

        const plTransform global(add * plTransform(vPosition, qRotation));
        plTransform local;
        if (GetOwner()->GetParent() != nullptr)
        {
          local.SetLocalTransform(GetOwner()->GetParent()->GetGlobalTransform(), global);
        }
        else
        {
          local = global;
        }
        GetOwner()->SetLocalPosition(local.m_vPosition);
        if (m_bRotation)
          GetOwner()->SetLocalRotation(local.m_qRotation);
        if (m_bScale)
          GetOwner()->SetLocalScaling(local.m_vScale);
      }
    }
  }
}

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_DeviceTrackingComponent);
