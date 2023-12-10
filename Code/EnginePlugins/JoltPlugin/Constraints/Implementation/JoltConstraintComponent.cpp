#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Constraints/JoltConstraintComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plJoltConstraintComponent, 1)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("PairCollision", GetPairCollision, SetPairCollision)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ACCESSOR_PROPERTY("ParentActor", DummyGetter, SetParentActorReference)->AddAttributes(new plGameObjectReferenceAttribute()),
    PLASMA_ACCESSOR_PROPERTY("ChildActor", DummyGetter, SetChildActorReference)->AddAttributes(new plGameObjectReferenceAttribute()),
    PLASMA_ACCESSOR_PROPERTY("ChildActorAnchor", DummyGetter, SetChildActorAnchorReference)->AddAttributes(new plGameObjectReferenceAttribute()),
    PLASMA_ACCESSOR_PROPERTY("BreakForce", GetBreakForce, SetBreakForce),
    PLASMA_ACCESSOR_PROPERTY("BreakTorque", GetBreakTorque, SetBreakTorque),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics/Jolt/Constraints"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plJoltMsgDisconnectConstraints, OnJoltMsgDisconnectConstraints),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plJoltConstraintLimitMode, 1)
  PLASMA_ENUM_CONSTANTS(plJoltConstraintLimitMode::NoLimit, plJoltConstraintLimitMode::HardLimit/*, plJoltConstraintLimitMode::SoftLimit*/)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plJoltConstraintDriveMode, 1)
  PLASMA_ENUM_CONSTANTS(plJoltConstraintDriveMode::NoDrive, plJoltConstraintDriveMode::DriveVelocity, plJoltConstraintDriveMode::DrivePosition)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

plJoltConstraintComponent::plJoltConstraintComponent() = default;
plJoltConstraintComponent::~plJoltConstraintComponent() = default;

void plJoltConstraintComponent::BreakConstraint()
{
  if (m_pConstraint == nullptr)
    return;

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  pModule->GetJoltSystem()->RemoveConstraint(m_pConstraint);

  pModule->m_BreakableConstraints.Remove(GetHandle());

  // wake up the joined bodies, so that removing a constraint doesn't let them hang in the air
  {
    JPH::BodyID bodies[2] = {JPH::BodyID(JPH::BodyID::cInvalidBodyID), JPH::BodyID(JPH::BodyID::cInvalidBodyID)};
    plInt32 iBodies = 0;

    if (!m_hActorA.IsInvalidated())
    {
      plGameObject* pObject = nullptr;
      plJoltDynamicActorComponent* pRbComp = nullptr;

      if (GetWorld()->TryGetObject(m_hActorA, pObject) && pObject->IsActive() && pObject->TryGetComponentOfBaseType(pRbComp))
      {
        bodies[iBodies] = JPH::BodyID(pRbComp->GetJoltBodyID());
        ++iBodies;

        pRbComp->RemoveConstraint(GetHandle());
      }
    }

    if (!m_hActorB.IsInvalidated())
    {
      plGameObject* pObject = nullptr;
      plJoltDynamicActorComponent* pRbComp = nullptr;

      if (GetWorld()->TryGetObject(m_hActorB, pObject) && pObject->IsActive() && pObject->TryGetComponentOfBaseType(pRbComp))
      {
        bodies[iBodies] = JPH::BodyID(pRbComp->GetJoltBodyID());
        ++iBodies;

        pRbComp->RemoveConstraint(GetHandle());
      }
    }

    if (iBodies > 0)
    {
      plLog::Info("Waking up {} bodies", iBodies);
      pModule->GetJoltSystem()->GetBodyInterface().ActivateBodies(bodies, iBodies);
    }
  }

  m_pConstraint->Release();
  m_pConstraint = nullptr;
}

void plJoltConstraintComponent::SetBreakForce(float value)
{
  m_fBreakForce = value;
  QueueApplySettings();
}

void plJoltConstraintComponent::SetBreakTorque(float value)
{
  m_fBreakTorque = value;
  QueueApplySettings();
}

void plJoltConstraintComponent::SetPairCollision(bool value)
{
  m_bPairCollision = value;
  QueueApplySettings();
}

void plJoltConstraintComponent::OnSimulationStarted()
{
  plUInt32 uiBodyIdA = plInvalidIndex;
  plUInt32 uiBodyIdB = plInvalidIndex;

  plJoltDynamicActorComponent* pRbParent = nullptr;
  plJoltDynamicActorComponent* pRbChild = nullptr;

  if (FindParentBody(uiBodyIdA, pRbParent).Failed())
    return;

  if (FindChildBody(uiBodyIdB, pRbChild).Failed())
    return;

  if (uiBodyIdB == plInvalidIndex)
    return;

  if (uiBodyIdA == uiBodyIdB)
  {
    plLog::Error("Constraint can't be linked to the same body twice");
    return;
  }

  m_LocalFrameA.m_qRotation.Normalize();
  m_LocalFrameB.m_qRotation.Normalize();

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  {
    JPH::BodyID bodies[2] = {JPH::BodyID(uiBodyIdA), JPH::BodyID(uiBodyIdB)};
    JPH::BodyLockMultiWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), bodies, 2);

    if (uiBodyIdB != plInvalidIndex && bodyLock.GetBody(1) != nullptr)
    {
      if (uiBodyIdA != plInvalidIndex && bodyLock.GetBody(0) != nullptr)
      {
        CreateContstraintType(bodyLock.GetBody(0), bodyLock.GetBody(1));

        pModule->EnableJoinedBodiesCollisions(bodyLock.GetBody(0)->GetCollisionGroup().GetGroupID(), bodyLock.GetBody(1)->GetCollisionGroup().GetGroupID(), m_bPairCollision);
      }
      else
      {
        CreateContstraintType(&JPH::Body::sFixedToWorld, bodyLock.GetBody(1));
      }
    }
  }

  if (m_pConstraint)
  {
    m_pConstraint->AddRef();
    pModule->GetJoltSystem()->AddConstraint(m_pConstraint);
    ApplySettings();

    if (pRbParent)
    {
      pRbParent->AddConstraint(GetHandle());
    }

    if (pRbChild)
    {
      pRbChild->AddConstraint(GetHandle());
    }
  }
}

void plJoltConstraintComponent::OnDeactivated()
{
  BreakConstraint();

  SUPER::OnDeactivated();
}

void plJoltConstraintComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  // s << m_fBreakForce;
  // s << m_fBreakTorque;
  s << m_bPairCollision;

  inout_stream.WriteGameObjectHandle(m_hActorA);
  inout_stream.WriteGameObjectHandle(m_hActorB);

  s << m_LocalFrameA;
  s << m_LocalFrameB;

  inout_stream.WriteGameObjectHandle(m_hActorBAnchor);
}

void plJoltConstraintComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = inout_stream.GetStream();

  // s >> m_fBreakForce;
  // s >> m_fBreakTorque;
  s >> m_bPairCollision;

  m_hActorA = inout_stream.ReadGameObjectHandle();
  m_hActorB = inout_stream.ReadGameObjectHandle();

  s >> m_LocalFrameA;
  s >> m_LocalFrameB;

  m_hActorBAnchor = inout_stream.ReadGameObjectHandle();
}

void plJoltConstraintComponent::SetParentActorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetParentActor(resolver(szReference, GetHandle(), "ParentActor"));
}

void plJoltConstraintComponent::SetChildActorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetChildActor(resolver(szReference, GetHandle(), "ChildActor"));
}

void plJoltConstraintComponent::SetChildActorAnchorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetUserFlag(1, false); // local frame B is not valid
  m_hActorBAnchor = resolver(szReference, GetHandle(), "ChildActorAnchor");
}

void plJoltConstraintComponent::SetParentActor(plGameObjectHandle hActor)
{
  SetUserFlag(0, false); // local frame A is not valid
  m_hActorA = hActor;
}

void plJoltConstraintComponent::SetChildActor(plGameObjectHandle hActor)
{
  SetUserFlag(1, false); // local frame B is not valid
  m_hActorB = hActor;
}

void plJoltConstraintComponent::SetChildActorAnchor(plGameObjectHandle hActor)
{
  SetUserFlag(1, false); // local frame B is not valid
  m_hActorBAnchor = hActor;
}

void plJoltConstraintComponent::SetActors(plGameObjectHandle hActorA, const plTransform& localFrameA, plGameObjectHandle hActorB, const plTransform& localFrameB)
{
  m_hActorA = hActorA;
  m_hActorB = hActorB;

  // prevent FindParentBody() and FindChildBody() from overwriting the local frames
  // local frame A and B are already valid
  SetUserFlag(0, true);
  SetUserFlag(1, true);

  m_LocalFrameA = localFrameA;
  m_LocalFrameB = localFrameB;
}

void plJoltConstraintComponent::ApplySettings()
{
  SetUserFlag(2, false);

  if (m_fBreakForce > 0.0f || m_fBreakTorque > 0.0f)
  {
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    pModule->m_BreakableConstraints.Insert(GetHandle());
  }
  else
  {
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    pModule->m_BreakableConstraints.Remove(GetHandle());
  }
}

void plJoltConstraintComponent::OnJoltMsgDisconnectConstraints(plJoltMsgDisconnectConstraints& ref_msg)
{
  BreakConstraint();
}

plResult plJoltConstraintComponent::FindParentBody(plUInt32& out_uiJoltBodyID, plJoltDynamicActorComponent*& pRbComp)
{
  plGameObject* pObject = nullptr;
  pRbComp = nullptr;

  if (!m_hActorA.IsInvalidated())
  {
    if (!GetWorld()->TryGetObject(m_hActorA, pObject) || !pObject->IsActive())
    {
      plLog::Error("{0} '{1}' parent reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
      return PLASMA_FAILURE;
    }

    if (!pObject->TryGetComponentOfBaseType(pRbComp))
    {
      plLog::Error("{0} '{1}' parent reference is an object without a plJoltDynamicActorComponent. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(),
        GetOwner()->GetName());
      return PLASMA_FAILURE;
    }
  }
  else
  {
    pObject = GetOwner();

    while (pObject != nullptr)
    {
      if (pObject->TryGetComponentOfBaseType(pRbComp))
        break;

      pObject = pObject->GetParent();
    }

    if (pRbComp == nullptr)
    {
      out_uiJoltBodyID = plInvalidIndex;

      if (GetUserFlag(0) == false)
      {
        // m_localFrameA is now valid
        SetUserFlag(0, true);
        m_LocalFrameA = GetOwner()->GetGlobalTransform();
      }
      return PLASMA_SUCCESS;
    }
    else
    {
      PLASMA_ASSERT_DEBUG(pObject != nullptr, "pRbComp and pObject should always be valid together");
      if (GetUserFlag(0) == true)
      {
        plTransform globalFrame = m_LocalFrameA;

        // m_localFrameA is already valid
        // assume it was in global space and move it into local space of the found parent
        m_LocalFrameA = plTransform::MakeLocalTransform(pRbComp->GetOwner()->GetGlobalTransform(), globalFrame);
        m_LocalFrameA.m_vPosition = m_LocalFrameA.m_vPosition.CompMul(pObject->GetGlobalScaling());
      }
    }
  }

  pRbComp->EnsureSimulationStarted();
  out_uiJoltBodyID = pRbComp->GetJoltBodyID();

  if (out_uiJoltBodyID == plInvalidIndex)
  {
    plLog::Error("{0} '{1}' parent reference is an object with an invalid plJoltDynamicActorComponent. Constraint is ignored.",
      GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return PLASMA_FAILURE;
  }

  m_hActorA = pObject->GetHandle();

  if (GetUserFlag(0) == false)
  {
    // m_localFrameA is now valid
    SetUserFlag(0, true);
    m_LocalFrameA = plTransform::MakeLocalTransform(pObject->GetGlobalTransform(), GetOwner()->GetGlobalTransform());
    m_LocalFrameA.m_vPosition = m_LocalFrameA.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return PLASMA_SUCCESS;
}

plResult plJoltConstraintComponent::FindChildBody(plUInt32& out_uiJoltBodyID, plJoltDynamicActorComponent*& pRbComp)
{
  plGameObject* pObject = nullptr;
  pRbComp = nullptr;

  if (m_hActorB.IsInvalidated())
  {
    plLog::Error("{0} '{1}' has no child reference. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return PLASMA_FAILURE;
  }

  if (!GetWorld()->TryGetObject(m_hActorB, pObject) || !pObject->IsActive())
  {
    plLog::Error("{0} '{1}' child reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return PLASMA_FAILURE;
  }

  if (!pObject->TryGetComponentOfBaseType(pRbComp))
  {
    // this makes it possible to link the Constraint to a prefab, because it may skip the top level hierarchy of the prefab
    pObject = pObject->SearchForChildByNameSequence("/", plGetStaticRTTI<plJoltDynamicActorComponent>());

    if (pObject == nullptr)
    {
      plLog::Error("{0} '{1}' child reference is an object without a plJoltDynamicActorComponent. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(),
        GetOwner()->GetName());
      return PLASMA_FAILURE;
    }

    pObject->TryGetComponentOfBaseType(pRbComp);
  }

  pRbComp->EnsureSimulationStarted();
  out_uiJoltBodyID = pRbComp->GetJoltBodyID();

  if (out_uiJoltBodyID == plInvalidIndex)
  {
    plLog::Error("{0} '{1}' child reference is an object with an invalid plJoltDynamicActorComponent. Constraint is ignored.",
      GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return PLASMA_FAILURE;
  }

  m_hActorB = pObject->GetHandle();

  if (GetUserFlag(1) == false)
  {
    plGameObject* pAnchorObject = GetOwner();

    if (!m_hActorBAnchor.IsInvalidated())
    {
      if (!GetWorld()->TryGetObject(m_hActorBAnchor, pAnchorObject))
      {
        plLog::Error("{0} '{1}' anchor reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
        return PLASMA_FAILURE;
      }
    }

    // m_localFrameB is now valid
    SetUserFlag(1, true);
    m_LocalFrameB = plTransform::MakeLocalTransform(pObject->GetGlobalTransform(), pAnchorObject->GetGlobalTransform());
    m_LocalFrameB.m_vPosition = m_LocalFrameB.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return PLASMA_SUCCESS;
}

plTransform plJoltConstraintComponent::ComputeParentBodyGlobalFrame() const
{
  if (!m_hActorA.IsInvalidated())
  {
    const plGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hActorA, pObject))
    {
      plTransform res;
      res = plTransform::MakeGlobalTransform(pObject->GetGlobalTransform(), m_LocalFrameA);
      return res;
    }
  }

  return m_LocalFrameA;
}

plTransform plJoltConstraintComponent::ComputeChildBodyGlobalFrame() const
{
  if (!m_hActorB.IsInvalidated())
  {
    const plGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hActorB, pObject))
    {
      plTransform res;
      res = plTransform::MakeGlobalTransform(pObject->GetGlobalTransform(), m_LocalFrameB);
      return res;
    }
  }

  return m_LocalFrameB;
}

void plJoltConstraintComponent::QueueApplySettings()
{
  if (m_pConstraint == nullptr)
    return;

  // already in queue ?
  if (GetUserFlag(2))
    return;

  SetUserFlag(2, true);

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  pModule->m_RequireUpdate.PushBack(GetHandle());
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltConstraintComponent);
