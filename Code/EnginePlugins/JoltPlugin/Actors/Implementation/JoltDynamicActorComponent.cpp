#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <Physics/Collision/Shape/OffsetCenterOfMassShape.h>

plJoltDynamicActorComponentManager::plJoltDynamicActorComponentManager(plWorld* pWorld)
  : plComponentManager<plJoltDynamicActorComponent, plBlockStorageType::FreeList>(pWorld)
{
}

plJoltDynamicActorComponentManager::~plJoltDynamicActorComponentManager() = default;

void plJoltDynamicActorComponentManager::UpdateDynamicActors()
{
  PL_PROFILE_SCOPE("UpdateDynamicActors");

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();

  for (auto itActor : pModule->GetActiveActors())
  {
    plJoltDynamicActorComponent* pActor = itActor;

    JPH::BodyID bodyId(pActor->GetJoltBodyID());

    JPH::BodyLockRead bodyLock(pSystem->GetBodyLockInterface(), bodyId);
    if (!bodyLock.Succeeded())
      continue;

    const JPH::Body& body = bodyLock.GetBody();

    if (!body.IsDynamic())
      continue;

    plSimdTransform trans = pActor->GetOwner()->GetGlobalTransformSimd();

    trans.m_Position = plJoltConversionUtils::ToSimdVec3(body.GetPosition());
    trans.m_Rotation = plJoltConversionUtils::ToSimdQuat(body.GetRotation());

    pActor->GetOwner()->SetGlobalTransform(trans);
  }
}

void plJoltDynamicActorComponentManager::UpdateKinematicActors(plTime deltaTime)
{
  PL_PROFILE_SCOPE("UpdateKinematicActors");

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  const float tDiff = deltaTime.AsFloatInSeconds();

  for (auto pKinematicActorComponent : m_KinematicActorComponents)
  {
    JPH::BodyID bodyId(pKinematicActorComponent->m_uiJoltBodyID);

    if (bodyId.IsInvalid())
      continue;

    plGameObject* pObject = pKinematicActorComponent->GetOwner();

    pObject->UpdateGlobalTransform();

    const plSimdVec4f pos = pObject->GetGlobalPositionSimd();
    const plSimdQuat rot = pObject->GetGlobalRotationSimd();

    pBodies->MoveKinematic(bodyId, plJoltConversionUtils::ToVec3(pos), plJoltConversionUtils::ToQuat(rot).Normalized(), tDiff);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plJoltDynamicActorComponent, 3, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
      PL_ACCESSOR_PROPERTY("Kinematic", GetKinematic, SetKinematic),
      PL_MEMBER_PROPERTY("StartAsleep", m_bStartAsleep),
      PL_MEMBER_PROPERTY("Mass", m_fInitialMass)->AddAttributes(new plSuffixAttribute(" kg"), new plClampValueAttribute(0.0f, plVariant())),
      PL_MEMBER_PROPERTY("Density", m_fDensity)->AddAttributes(new plDefaultValueAttribute(100.0f), new plSuffixAttribute(" kg/m^3")),
      PL_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
      PL_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new plDefaultValueAttribute(1.0f)),
      PL_MEMBER_PROPERTY("LinearDamping", m_fLinearDamping)->AddAttributes(new plDefaultValueAttribute(0.2f)),
      PL_MEMBER_PROPERTY("AngularDamping", m_fAngularDamping)->AddAttributes(new plDefaultValueAttribute(0.2f)),
      PL_MEMBER_PROPERTY("ContinuousCollisionDetection", m_bCCD),
      PL_BITFLAGS_MEMBER_PROPERTY("OnContact", plOnJoltContact, m_OnContact),
      PL_ACCESSOR_PROPERTY("CustomCenterOfMass", GetUseCustomCoM, SetUseCustomCoM),
      PL_MEMBER_PROPERTY("CenterOfMass", m_vCenterOfMass),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
      PL_MESSAGE_HANDLER(plMsgPhysicsAddForce, AddForceAtPos),
      PL_MESSAGE_HANDLER(plMsgPhysicsAddImpulse, AddImpulseAtPos),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(AddLinearForce, In, "vForce"),
    PL_SCRIPT_FUNCTION_PROPERTY(AddLinearImpulse, In, "vImpulse"),
    PL_SCRIPT_FUNCTION_PROPERTY(AddAngularForce, In, "vForce"),
    PL_SCRIPT_FUNCTION_PROPERTY(AddAngularImpulse, In, "vImpulse"),
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_ATTRIBUTES
  {
    new plTransformManipulatorAttribute("CenterOfMass")
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plJoltDynamicActorComponent::plJoltDynamicActorComponent()
{
  m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
}

plJoltDynamicActorComponent::~plJoltDynamicActorComponent() = default;

void plJoltDynamicActorComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_bKinematic;
  s << m_bCCD;
  s << m_fLinearDamping;
  s << m_fAngularDamping;
  s << m_fDensity;
  s << m_fInitialMass;
  s << m_fGravityFactor;
  s << m_hSurface;
  s << m_OnContact;
  s << GetUseCustomCoM();
  s << m_vCenterOfMass;
  s << m_bStartAsleep;
}

void plJoltDynamicActorComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_bKinematic;
  s >> m_bCCD;
  s >> m_fLinearDamping;
  s >> m_fAngularDamping;
  s >> m_fDensity;
  s >> m_fInitialMass;
  s >> m_fGravityFactor;
  s >> m_hSurface;
  s >> m_OnContact;

  if (uiVersion >= 2)
  {
    bool com;
    s >> com;
    SetUseCustomCoM(com);
    s >> m_vCenterOfMass;
  }

  if (uiVersion >= 3)
  {
    s >> m_bStartAsleep;
  }
}

void plJoltDynamicActorComponent::SetKinematic(bool b)
{
  if (m_bKinematic == b)
    return;

  m_bKinematic = b;

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (m_bKinematic && !bodyId.IsInvalid())
  {
    // do not insert this, until we actually have an actor pointer
    GetWorld()->GetOrCreateComponentManager<plJoltDynamicActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }
  else
  {
    GetWorld()->GetOrCreateComponentManager<plJoltDynamicActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  if (bodyId.IsInvalid())
    return;

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();

  {
    JPH::BodyLockWrite bodyLock(pSystem->GetBodyLockInterface(), bodyId);

    if (bodyLock.Succeeded())
    {
      JPH::Body& body = bodyLock.GetBody();
      body.SetMotionType(m_bKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic);
    }
  }

  if (!m_bKinematic && pSystem->GetBodyInterface().IsAdded(bodyId))
  {
    pSystem->GetBodyInterface().ActivateBody(bodyId);
  }
}

void plJoltDynamicActorComponent::SetGravityFactor(float fFactor)
{
  if (m_fGravityFactor == fFactor)
    return;

  m_fGravityFactor = fFactor;

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (bodyId.IsInvalid())
    return;

  auto* pSystem = GetWorld()->GetOrCreateModule<plJoltWorldModule>()->GetJoltSystem();

  JPH::BodyLockWrite bodyLock(pSystem->GetBodyLockInterface(), bodyId);

  if (bodyLock.Succeeded())
  {
    bodyLock.GetBody().GetMotionProperties()->SetGravityFactor(m_fGravityFactor);

    if (pSystem->GetBodyInterfaceNoLock().IsAdded(bodyId))
    {
      pSystem->GetBodyInterfaceNoLock().ActivateBody(bodyId);
    }
  }
}

void plJoltDynamicActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  const plSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();
  auto* pMaterial = GetJoltMaterial();

  JPH::BodyCreationSettings bodyCfg;

  if (CreateShape(&bodyCfg, m_fDensity, pMaterial).Failed())
  {
    plLog::Error("Jolt dynamic actor component '{}' has no valid shape.", GetOwner()->GetName());
    return;
  }

  if (pMaterial == nullptr)
    pMaterial = plJoltCore::GetDefaultMaterial();

  plJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  bodyCfg.mPosition = plJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = plJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType = m_bKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic;
  bodyCfg.mObjectLayer = plJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, plJoltBroadphaseLayer::Dynamic);
  bodyCfg.mMotionQuality = m_bCCD ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
  bodyCfg.mLinearDamping = m_fLinearDamping;
  bodyCfg.mAngularDamping = m_fAngularDamping;
  bodyCfg.mMassPropertiesOverride.mMass = m_fInitialMass;
  bodyCfg.mOverrideMassProperties = m_fInitialMass > 0.0f ? JPH::EOverrideMassProperties::CalculateInertia : JPH::EOverrideMassProperties::CalculateMassAndInertia;
  bodyCfg.mGravityFactor = m_fGravityFactor;
  bodyCfg.mRestitution = pMaterial->m_fRestitution;
  bodyCfg.mFriction = pMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<plUInt64>(pUserData);

  if (GetUseCustomCoM())
  {
    const plVec3 vGlobalScale = GetOwner()->GetGlobalScaling();
    const float scale = plMath::Min(vGlobalScale.x, vGlobalScale.y, vGlobalScale.z);
    auto vLocalCenterOfMass = plSimdVec4f(scale * m_vCenterOfMass.x, scale * m_vCenterOfMass.y, scale * m_vCenterOfMass.z);
    auto vPrevCoM = plJoltConversionUtils::ToSimdVec3(bodyCfg.GetShape()->GetCenterOfMass());

    auto vComShift = vLocalCenterOfMass - vPrevCoM;

    JPH::OffsetCenterOfMassShapeSettings com;
    com.mOffset = plJoltConversionUtils::ToVec3(vComShift);
    com.mInnerShapePtr = bodyCfg.GetShape();

    bodyCfg.SetShape(com.Create().Get());
  }

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, !m_bStartAsleep);

  if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<plJoltDynamicActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }
}

void plJoltDynamicActorComponent::OnDeactivated()
{
  if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<plJoltDynamicActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  auto allConstraints = m_Constraints;
  m_Constraints.Clear();
  m_Constraints.Compact();

  plJoltMsgDisconnectConstraints msg;
  msg.m_pActor = this;
  msg.m_uiJoltBodyID = GetJoltBodyID();

  plWorld* pWorld = GetWorld();

  for (plComponentHandle hConstraint : allConstraints)
  {
    pWorld->SendMessage(hConstraint, msg);
  }

  SUPER::OnDeactivated();
}

void plJoltDynamicActorComponent::AddLinearForce(const plVec3& vForce)
{
  if (m_bKinematic || m_uiJoltBodyID == plInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<plJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddForce(JPH::BodyID(m_uiJoltBodyID), plJoltConversionUtils::ToVec3(vForce));
}

void plJoltDynamicActorComponent::AddLinearImpulse(const plVec3& vImpulse)
{
  if (m_bKinematic || m_uiJoltBodyID == plInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<plJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddImpulse(JPH::BodyID(m_uiJoltBodyID), plJoltConversionUtils::ToVec3(vImpulse));
}

void plJoltDynamicActorComponent::AddAngularForce(const plVec3& vForce)
{
  if (m_bKinematic || m_uiJoltBodyID == plInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<plJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddTorque(JPH::BodyID(m_uiJoltBodyID), plJoltConversionUtils::ToVec3(vForce));
}

void plJoltDynamicActorComponent::AddAngularImpulse(const plVec3& vImpulse)
{
  if (m_bKinematic || m_uiJoltBodyID == plInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<plJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddAngularImpulse(JPH::BodyID(m_uiJoltBodyID), plJoltConversionUtils::ToVec3(vImpulse));
}

void plJoltDynamicActorComponent::AddConstraint(plComponentHandle hComponent)
{
  m_Constraints.PushBack(hComponent);
}

void plJoltDynamicActorComponent::RemoveConstraint(plComponentHandle hComponent)
{
  m_Constraints.RemoveAndSwap(hComponent);
}

void plJoltDynamicActorComponent::AddForceAtPos(plMsgPhysicsAddForce& ref_msg)
{
  if (m_bKinematic || m_uiJoltBodyID == plInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<plJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddForce(JPH::BodyID(m_uiJoltBodyID), plJoltConversionUtils::ToVec3(ref_msg.m_vForce), plJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
}

void plJoltDynamicActorComponent::AddImpulseAtPos(plMsgPhysicsAddImpulse& ref_msg)
{
  if (m_bKinematic || m_uiJoltBodyID == plInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<plJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddImpulse(JPH::BodyID(m_uiJoltBodyID), plJoltConversionUtils::ToVec3(ref_msg.m_vImpulse), plJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
}

float plJoltDynamicActorComponent::GetMass() const
{
  if (m_bKinematic || m_uiJoltBodyID == plInvalidIndex)
    return 0.0f;

  auto& bodyLockInterface = GetWorld()->GetModule<plJoltWorldModule>()->GetJoltSystem()->GetBodyLockInterface();
  JPH::BodyLockRead bodyLock(bodyLockInterface, JPH::BodyID(m_uiJoltBodyID));

  const float fInverseMass = bodyLock.GetBody().GetMotionProperties()->GetInverseMass();
  return fInverseMass > 0.0f ? 1.0f / fInverseMass : 0.0f;
}

const plJoltMaterial* plJoltDynamicActorComponent::GetJoltMaterial() const
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

void plJoltDynamicActorComponent::SetSurfaceFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = plResourceManager::LoadResource<plSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    plResourceManager::PreloadResource(m_hSurface);
}

const char* plJoltDynamicActorComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltDynamicActorComponent);
