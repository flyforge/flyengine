#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

plJoltQueryShapeActorComponentManager::plJoltQueryShapeActorComponentManager(plWorld* pWorld)
  : plComponentManager<plJoltQueryShapeActorComponent, plBlockStorageType::FreeList>(pWorld)
{
}

plJoltQueryShapeActorComponentManager::~plJoltQueryShapeActorComponentManager() = default;

void plJoltQueryShapeActorComponentManager::UpdateMovingQueryShapes()
{
  PLASMA_PROFILE_SCOPE("UpdateMovingQueryShapes");

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  for (auto pComponent : m_MovingQueryShapes)
  {
    JPH::BodyID bodyId(pComponent->m_uiJoltBodyID);

    if (bodyId.IsInvalid())
      continue;

    plGameObject* pObject = pComponent->GetOwner();

    pObject->UpdateGlobalTransform();

    const plSimdVec4f pos = pObject->GetGlobalPositionSimd();
    const plSimdQuat rot = pObject->GetGlobalRotationSimd();

    pBodies->SetPositionAndRotation(bodyId, plJoltConversionUtils::ToVec3(pos), plJoltConversionUtils::ToQuat(rot), JPH::EActivation::DontActivate);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltQueryShapeActorComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plJoltQueryShapeActorComponent::plJoltQueryShapeActorComponent() = default;
plJoltQueryShapeActorComponent::~plJoltQueryShapeActorComponent() = default;

void plJoltQueryShapeActorComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hSurface;
}

void plJoltQueryShapeActorComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSurface;
}

void plJoltQueryShapeActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  const plSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  JPH::BodyCreationSettings bodyCfg;

  if (CreateShape(&bodyCfg, 1.0f, GetJoltMaterial()).Failed())
  {
    plLog::Error("Jolt query-shape actor component '{}' has no valid shape.", GetOwner()->GetName());
    return;
  }

  plJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  bodyCfg.mPosition = plJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = plJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType = JPH::EMotionType::Static;
  bodyCfg.mObjectLayer = plJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, plJoltBroadphaseLayer::Query);
  bodyCfg.mMotionQuality = JPH::EMotionQuality::Discrete;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  // bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints
  bodyCfg.mUserData = reinterpret_cast<plUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, true);

  if (GetOwner()->IsDynamic())
  {
    GetWorld()->GetOrCreateComponentManager<plJoltQueryShapeActorComponentManager>()->m_MovingQueryShapes.PushBack(this);
  }
}

void plJoltQueryShapeActorComponent::OnDeactivated()
{
  if (GetOwner()->IsDynamic())
  {
    GetWorld()->GetOrCreateComponentManager<plJoltQueryShapeActorComponentManager>()->m_MovingQueryShapes.RemoveAndSwap(this);
  }

  SUPER::OnDeactivated();
}

void plJoltQueryShapeActorComponent::SetSurfaceFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = plResourceManager::LoadResource<plSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    plResourceManager::PreloadResource(m_hSurface);
}

const char* plJoltQueryShapeActorComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

const plJoltMaterial* plJoltQueryShapeActorComponent::GetJoltMaterial() const
{
  if (m_hSurface.IsValid())
  {
    plResourceLock<plSurfaceResource> pSurface(m_hSurface, plResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<plJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return nullptr;
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltQueryShapeActorComponent);

