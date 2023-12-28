
#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/RaycastComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

plRaycastComponentManager::plRaycastComponentManager(plWorld* pWorld)
  : SUPER(pWorld)
{
}

void plRaycastComponentManager::Initialize()
{
  // we want to do the raycast as late as possible, ie. after animated objects and characters moved
  // such that we get the latest position that is in sync with those animated objects
  // therefore we move the update into the post async phase and set a low priority (low = updated late)
  // we DO NOT want to use post transform update, because when we move the target object
  // child objects of the target node should still get the full global transform update within this frame

  auto desc = plWorldModule::UpdateFunctionDesc(plWorldModule::UpdateFunction(&plRaycastComponentManager::Update, this), "plRaycastComponentManager::Update");
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = UpdateFunctionDesc::Phase::PostAsync;
  desc.m_fPriority = -1000;

  this->RegisterUpdateFunction(desc);
}

void plRaycastComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plRaycastComponent, 3, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new plDefaultValueAttribute(100.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("DisableTargetObjectOnNoHit", m_bDisableTargetObjectOnNoHit),
    PLASMA_ACCESSOR_PROPERTY("RaycastEndObject", DummyGetter, SetRaycastEndObject)->AddAttributes(new plGameObjectReferenceAttribute()),
    PLASMA_MEMBER_PROPERTY("ForceTargetParentless", m_bForceTargetParentless),
    PLASMA_BITFLAGS_MEMBER_PROPERTY("ShapeTypesToHit", plPhysicsShapeType, m_ShapeTypesToHit)->AddAttributes(new plDefaultValueAttribute(plPhysicsShapeType::Default & ~(plPhysicsShapeType::Trigger))),
    PLASMA_MEMBER_PROPERTY("CollisionLayerEndPoint", m_uiCollisionLayerEndPoint)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PLASMA_MEMBER_PROPERTY("CollisionLayerTrigger", m_uiCollisionLayerTrigger)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PLASMA_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay/Logic"),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 0.5f, plColor::YellowGreen),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRaycastComponent::plRaycastComponent() = default;
plRaycastComponent::~plRaycastComponent() = default;

void plRaycastComponent::Deinitialize()
{
  if (m_bForceTargetParentless)
  {
    // see end of plRaycastComponent::Update() for details
    GetWorld()->DeleteObjectDelayed(m_hRaycastEndObject);
  }

  SUPER::Deinitialize();
}

void plRaycastComponent::OnActivated()
{
  SUPER::OnActivated();
}

void plRaycastComponent::OnDeactivated()
{
  if (m_bDisableTargetObjectOnNoHit && m_bForceTargetParentless)
  {
    plGameObject* pEndObject = nullptr;
    if (GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
    {
      pEndObject->SetActiveFlag(false);
    }
  }

  SUPER::OnDeactivated();
}

void plRaycastComponent::OnSimulationStarted()
{
  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<plPhysicsWorldModuleInterface>();
  m_hLastTriggerObjectInRay.Invalidate();

  plGameObject* pEndObject = nullptr;
  if (GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
  {
    if (!pEndObject->IsDynamic())
    {
      pEndObject->MakeDynamic();
    }
  }
}

void plRaycastComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  stream.WriteGameObjectHandle(m_hRaycastEndObject);
  s << m_fMaxDistance;
  s << m_bDisableTargetObjectOnNoHit;
  s << m_uiCollisionLayerEndPoint;
  s << m_uiCollisionLayerTrigger;
  s << m_sTriggerMessage;
  s << m_bForceTargetParentless;
  s << m_ShapeTypesToHit;
}

void plRaycastComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  m_hRaycastEndObject = stream.ReadGameObjectHandle();
  s >> m_fMaxDistance;
  s >> m_bDisableTargetObjectOnNoHit;
  s >> m_uiCollisionLayerEndPoint;
  s >> m_uiCollisionLayerTrigger;
  s >> m_sTriggerMessage;

  if (uiVersion >= 2)
  {
    s >> m_bForceTargetParentless;
  }

  if (uiVersion >= 3)
  {
    s >> m_ShapeTypesToHit;
  }
}

void plRaycastComponent::SetTriggerMessage(const char* sz)
{
  m_sTriggerMessage.Assign(sz);
}

const char* plRaycastComponent::GetTriggerMessage() const
{
  return m_sTriggerMessage.GetData();
}

void plRaycastComponent::SetRaycastEndObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hRaycastEndObject = resolver(szReference, GetHandle(), "RaycastEndObject");
}

void plRaycastComponent::Update()
{
  if (m_hRaycastEndObject.IsInvalidated())
    return;

  if (!m_pPhysicsWorldModule)
  {
    // Happens in Prefab viewports
    return;
  }

  plGameObject* pEndObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
  {
    // early out in the future
    m_hRaycastEndObject.Invalidate();
    return;
  }

  // if the owner object moved this frame, we want the latest global position as the ray starting position
  // this is especially important when the raycast component is attached to something that animates
  GetOwner()->UpdateGlobalTransform();

  const plVec3 rayStartPosition = GetOwner()->GetGlobalPosition();
  const plVec3 rayDir = GetOwner()->GetGlobalDirForwards().GetNormalized(); // PhysX is very picky about normalized vectors

  float fHitDistance = m_fMaxDistance;
  plPhysicsCastResult hit;

  {
    plPhysicsQueryParameters queryParams(m_uiCollisionLayerEndPoint);
    queryParams.m_bIgnoreInitialOverlap = true;
    queryParams.m_ShapeTypes = m_ShapeTypesToHit;

    if (m_pPhysicsWorldModule->Raycast(hit, rayStartPosition, rayDir, m_fMaxDistance, queryParams))
    {
      fHitDistance = hit.m_fDistance;

      if (!pEndObject->GetActiveFlag() && m_bDisableTargetObjectOnNoHit)
      {
        pEndObject->SetActiveFlag(true);
      }
    }
    else
    {
      if (m_bDisableTargetObjectOnNoHit)
      {
        pEndObject->SetActiveFlag(false);
      }
      else
      {
        if (!pEndObject->GetActiveFlag())
        {
          pEndObject->SetActiveFlag(true);
        }
      }
    }
  }

  if (false)
  {
    plDebugRenderer::Line lines[] = {{rayStartPosition, rayStartPosition + rayDir * fHitDistance}};
    plDebugRenderer::DrawLines(GetWorld(), lines, plColor::GreenYellow);
  }

  if (!m_sTriggerMessage.IsEmpty() && m_uiCollisionLayerEndPoint != m_uiCollisionLayerTrigger)
  {
    plPhysicsCastResult triggerHit;
    plPhysicsQueryParameters queryParams2(m_uiCollisionLayerTrigger);
    queryParams2.m_bIgnoreInitialOverlap = true;
    queryParams2.m_ShapeTypes = m_ShapeTypesToHit;

    if (m_pPhysicsWorldModule->Raycast(triggerHit, rayStartPosition, rayDir, fHitDistance, queryParams2) && triggerHit.m_fDistance < fHitDistance)
    {
      // We have a hit, check the objects
      if (m_hLastTriggerObjectInRay != triggerHit.m_hActorObject)
      {
        // If we had another object, we now have one closer - send
        // deactivated for the old object and activate the new one
        if (!m_hLastTriggerObjectInRay.IsInvalidated())
        {
          PostTriggerMessage(plTriggerState::Deactivated, m_hLastTriggerObjectInRay);
        }

        // Activate the new hit
        m_hLastTriggerObjectInRay = triggerHit.m_hActorObject;
        PostTriggerMessage(plTriggerState::Activated, m_hLastTriggerObjectInRay);
      }
      // If it is still the same object as before we send a continuing message
      else
      {
        PostTriggerMessage(plTriggerState::Continuing, m_hLastTriggerObjectInRay);
      }
    }
    else
    {
      // No hit anymore?
      if (!m_hLastTriggerObjectInRay.IsInvalidated())
      {
        PostTriggerMessage(plTriggerState::Deactivated, m_hLastTriggerObjectInRay);
      }

      m_hLastTriggerObjectInRay.Invalidate();
    }
  }

  if (m_bForceTargetParentless)
  {
    // this is necessary to ensure perfect positioning when the target is originally attached to a moving object
    // that happens, for instance, when the target is part of a prefab, which includes the raycast component, of course
    // and the prefab is then attached to e.g. a character
    // without detaching the target object from all parents, it is not possible to ensure that it will never deviate from the
    // position set by the raycast component
    // since we now change ownership (target is not deleted with its former parent anymore)
    // this flag also means that the raycast component will delete the target object, when it dies
    pEndObject->SetParent(plGameObjectHandle());
  }

  pEndObject->SetGlobalPosition(rayStartPosition + fHitDistance * rayDir);
}

void plRaycastComponent::PostTriggerMessage(plTriggerState::Enum state, plGameObjectHandle hObject)
{
  plMsgTriggerTriggered msg;

  msg.m_TriggerState = state;
  msg.m_sMessage = m_sTriggerMessage;
  msg.m_hTriggeringObject = hObject;

  m_TriggerEventSender.PostEventMessage(msg, this, GetOwner(), plTime::Zero(), plObjectMsgQueueType::PostTransform);
}


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_RaycastComponent);
