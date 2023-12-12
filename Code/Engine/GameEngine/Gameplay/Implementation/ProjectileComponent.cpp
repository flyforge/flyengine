#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/ProjectileComponent.h>
#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plProjectileReaction, 1)
  PLASMA_ENUM_CONSTANT(plProjectileReaction::Absorb),
  PLASMA_ENUM_CONSTANT(plProjectileReaction::Reflect),
  PLASMA_ENUM_CONSTANT(plProjectileReaction::Attach),
  PLASMA_ENUM_CONSTANT(plProjectileReaction::PassThrough)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plProjectileSurfaceInteraction, plNoBase, 3, plRTTIDefaultAllocator<plProjectileSurfaceInteraction>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface")),
    PLASMA_ENUM_MEMBER_PROPERTY("Reaction", plProjectileReaction, m_Reaction),
    PLASMA_MEMBER_PROPERTY("Interaction", m_sInteraction)->AddAttributes(new plDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    PLASMA_MEMBER_PROPERTY("Impulse", m_fImpulse),
    PLASMA_MEMBER_PROPERTY("Damage", m_fDamage),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_COMPONENT_TYPE(plProjectileComponent, 4, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Speed", m_fMetersPerSecond)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("GravityMultiplier", m_fGravityMultiplier),
    PLASMA_MEMBER_PROPERTY("MaxLifetime", m_MaxLifetime)->AddAttributes(new plClampValueAttribute(plTime(), plVariant())),
    PLASMA_ACCESSOR_PROPERTY("OnTimeoutSpawn", GetTimeoutPrefab, SetTimeoutPrefab)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
    PLASMA_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PLASMA_ACCESSOR_PROPERTY("FallbackSurface", GetFallbackSurfaceFile, SetFallbackSurfaceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface")),
    PLASMA_ARRAY_MEMBER_PROPERTY("Interactions", m_SurfaceInteractions),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnTriggered),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
    new plColorAttribute(plColorScheme::Gameplay),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 0.4f, plColor::OrangeRed),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plProjectileSurfaceInteraction::SetSurface(const char* szSurface)
{
  plSurfaceResourceHandle hSurface;

  if (!plStringUtils::IsNullOrEmpty(szSurface))
  {
    hSurface = plResourceManager::LoadResource<plSurfaceResource>(szSurface);
  }

  m_hSurface = hSurface;
}

const char* plProjectileSurfaceInteraction::GetSurface() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

plProjectileComponent::plProjectileComponent()
{
  m_fMetersPerSecond = 10.0f;
  m_uiCollisionLayer = 0;
  m_fGravityMultiplier = 0.0f;
  m_vVelocity.SetZero();
}

plProjectileComponent::~plProjectileComponent() = default;

void plProjectileComponent::Update()
{
  plPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetModule<plPhysicsWorldModuleInterface>();

  if (pPhysicsInterface)
  {
    plGameObject* pEntity = GetOwner();

    const float fTimeDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

    plVec3 vNewPosition;

    // gravity
    if (m_fGravityMultiplier != 0.0f && m_fMetersPerSecond > 0.0f) // mps == 0 for attached state
    {
      const plVec3 vGravity = pPhysicsInterface->GetGravity() * m_fGravityMultiplier;

      m_vVelocity += vGravity * fTimeDiff;
    }

    plVec3 vCurDirection = m_vVelocity * fTimeDiff;
    float fDistance = 0.0f;

    if (!vCurDirection.IsZero())
      fDistance = vCurDirection.GetLengthAndNormalize();

    plPhysicsQueryParameters queryParams(m_uiCollisionLayer);
    queryParams.m_bIgnoreInitialOverlap = true;
    queryParams.m_ShapeTypes.Remove(plPhysicsShapeType::Trigger);
    //queryParams.m_ShapeTypes.Remove(plPhysicsShapeType::Character); // TODO: expose this ??

    plPhysicsCastResult castResult;
    if (pPhysicsInterface->Raycast(castResult, pEntity->GetGlobalPosition(), vCurDirection, fDistance, queryParams))
    {
      const plSurfaceResourceHandle hSurface = castResult.m_hSurface.IsValid() ? castResult.m_hSurface : m_hFallbackSurface;

      const plInt32 iInteraction = FindSurfaceInteraction(hSurface);

      if (iInteraction == -1)
      {
        GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
        vNewPosition = castResult.m_vPosition;
      }
      else
      {
        const auto& interaction = m_SurfaceInteractions[iInteraction];

        if (!interaction.m_sInteraction.IsEmpty())
        {
          TriggerSurfaceInteraction(hSurface, castResult.m_hActorObject, castResult.m_vPosition, castResult.m_vNormal, vCurDirection, interaction.m_sInteraction);
        }

        // if we hit some valid object
        if (!castResult.m_hActorObject.IsInvalidated())
        {
          plGameObject* pObject = nullptr;

          // apply a physical impulse
          if (interaction.m_fImpulse > 0.0f)
          {
            if (GetWorld()->TryGetObject(castResult.m_hActorObject, pObject))
            {
              plMsgPhysicsAddImpulse msg;
              msg.m_vGlobalPosition = castResult.m_vPosition;
              msg.m_vImpulse = vCurDirection * interaction.m_fImpulse;
              msg.m_uiObjectFilterID = castResult.m_uiObjectFilterID;
              msg.m_pInternalPhysicsShape = castResult.m_pInternalPhysicsShape;
              msg.m_pInternalPhysicsActor = castResult.m_pInternalPhysicsActor;

              pObject->SendMessage(msg);
            }
          }

          // apply damage
          if (interaction.m_fDamage > 0.0f)
          {
            // skip the TryGetObject if we already did that above
            if (pObject != nullptr || GetWorld()->TryGetObject(castResult.m_hShapeObject, pObject))
            {
              plMsgDamage msg;
              msg.m_fDamage = interaction.m_fDamage;
              msg.m_vGlobalPosition = castResult.m_vPosition;
              msg.m_vImpactDirection = vCurDirection;

              plGameObject* pHitShape = nullptr;
              if (GetWorld()->TryGetObject(castResult.m_hShapeObject, pHitShape))
              {
                msg.m_sHitObjectName = pHitShape->GetName();
              }
              else
              {
                msg.m_sHitObjectName = pObject->GetName();
              }

              pObject->SendEventMessage(msg, this);
            }
          }
        }

        if (interaction.m_Reaction == plProjectileReaction::Absorb)
        {
          GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
          vNewPosition = castResult.m_vPosition;
        }
        else if (interaction.m_Reaction == plProjectileReaction::Reflect)
        {
          /// \todo Should reflect around the actual hit position
          /// \todo Should preserve travel distance while reflecting

          // const float fLength = (vPos - pEntity->GetGlobalPosition()).GetLength();

          vNewPosition = pEntity->GetGlobalPosition(); // vPos;

          const plVec3 vNewDirection = vCurDirection.GetReflectedVector(castResult.m_vNormal);

          plQuat qRot;
          qRot.SetShortestRotation(vCurDirection, vNewDirection);

          GetOwner()->SetGlobalRotation(qRot * GetOwner()->GetGlobalRotation());

          m_vVelocity = qRot * m_vVelocity;
        }
        else if (interaction.m_Reaction == plProjectileReaction::Attach)
        {
          m_fMetersPerSecond = 0.0f;
          vNewPosition = castResult.m_vPosition;

          plGameObject* pObject;
          if (GetWorld()->TryGetObject(castResult.m_hActorObject, pObject))
          {
            pObject->AddChild(GetOwner()->GetHandle(), plGameObject::TransformPreservation::PreserveGlobal);
          }
        }
        else if (interaction.m_Reaction == plProjectileReaction::PassThrough)
        {
          vNewPosition = pEntity->GetGlobalPosition() + fDistance * vCurDirection;
        }
      }
    }
    else
    {
      vNewPosition = pEntity->GetGlobalPosition() + fDistance * vCurDirection;
    }

    GetOwner()->SetGlobalPosition(vNewPosition);
  }
}

void plProjectileComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fMetersPerSecond;
  s << m_fGravityMultiplier;
  s << m_uiCollisionLayer;
  s << m_MaxLifetime;
  s << m_hTimeoutPrefab;

  // Version 3
  s << m_hFallbackSurface;

  s << m_SurfaceInteractions.GetCount();
  for (const auto& ia : m_SurfaceInteractions)
  {
    s << ia.m_hSurface;

    plProjectileReaction::StorageType storage = ia.m_Reaction;
    s << storage;

    s << ia.m_sInteraction;

    // Version 3
    s << ia.m_fImpulse;

    // Version 4
    s << ia.m_fDamage;
  }
}

void plProjectileComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fMetersPerSecond;
  s >> m_fGravityMultiplier;
  s >> m_uiCollisionLayer;
  s >> m_MaxLifetime;
  s >> m_hTimeoutPrefab;

  if (uiVersion >= 3)
  {
    s >> m_hFallbackSurface;
  }

  plUInt32 count;
  s >> count;
  m_SurfaceInteractions.SetCount(count);
  for (plUInt32 i = 0; i < count; ++i)
  {
    auto& ia = m_SurfaceInteractions[i];
    s >> ia.m_hSurface;

    plProjectileReaction::StorageType storage = 0;
    s >> storage;
    ia.m_Reaction = (plProjectileReaction::Enum)storage;

    s >> ia.m_sInteraction;

    if (uiVersion >= 3)
    {
      s >> ia.m_fImpulse;
    }

    if (uiVersion >= 4)
    {
      s >> ia.m_fDamage;
    }
  }
}


plInt32 plProjectileComponent::FindSurfaceInteraction(const plSurfaceResourceHandle& hSurface) const
{
  plSurfaceResourceHandle hCurSurf = hSurface;

  while (hCurSurf.IsValid())
  {
    for (plUInt32 i = 0; i < m_SurfaceInteractions.GetCount(); ++i)
    {
      if (hCurSurf == m_SurfaceInteractions[i].m_hSurface)
        return i;
    }

    // get parent surface
    {
      plResourceLock<plSurfaceResource> pSurf(hCurSurf, plResourceAcquireMode::BlockTillLoaded);
      hCurSurf = pSurf->GetDescriptor().m_hBaseSurface;
    }
  }

  return -1;
}


void plProjectileComponent::TriggerSurfaceInteraction(const plSurfaceResourceHandle& hSurface, plGameObjectHandle hObject, const plVec3& vPos, const plVec3& vNormal, const plVec3& vDirection, const char* szInteraction)
{
  plResourceLock<plSurfaceResource> pSurface(hSurface, plResourceAcquireMode::BlockTillLoaded);
  pSurface->InteractWithSurface(GetWorld(), hObject, vPos, vNormal, vDirection, plTempHashedString(szInteraction), &GetOwner()->GetTeamID());
}

static plHashedString s_sSuicide = plMakeHashedString("Suicide");

void plProjectileComponent::OnSimulationStarted()
{
  if (m_MaxLifetime.GetSeconds() > 0.0)
  {
    plMsgComponentInternalTrigger msg;
    msg.m_sMessage = s_sSuicide;

    PostMessage(msg, m_MaxLifetime);

    // make sure the prefab is available when the projectile dies
    if (m_hTimeoutPrefab.IsValid())
    {
      plResourceManager::PreloadResource(m_hTimeoutPrefab);
    }
  }

  m_vVelocity = GetOwner()->GetGlobalDirForwards() * m_fMetersPerSecond;
}

void plProjectileComponent::OnTriggered(plMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != s_sSuicide)
    return;

  if (m_hTimeoutPrefab.IsValid())
  {
    plResourceLock<plPrefabResource> pPrefab(m_hTimeoutPrefab, plResourceAcquireMode::AllowLoadingFallback);

    plPrefabInstantiationOptions options;
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), options, nullptr);
  }

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}


void plProjectileComponent::SetTimeoutPrefab(const char* szPrefab)
{
  plPrefabResourceHandle hPrefab;

  if (!plStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = plResourceManager::LoadResource<plPrefabResource>(szPrefab);
  }

  m_hTimeoutPrefab = hPrefab;
}

const char* plProjectileComponent::GetTimeoutPrefab() const
{
  if (!m_hTimeoutPrefab.IsValid())
    return "";

  return m_hTimeoutPrefab.GetResourceID();
}

void plProjectileComponent::SetFallbackSurfaceFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hFallbackSurface = plResourceManager::LoadResource<plSurfaceResource>(szFile);
  }
  if (m_hFallbackSurface.IsValid())
    plResourceManager::PreloadResource(m_hFallbackSurface);
}

const char* plProjectileComponent::GetFallbackSurfaceFile() const
{
  if (!m_hFallbackSurface.IsValid())
    return "";

  return m_hFallbackSurface.GetResourceID();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plProjectileComponentPatch_1_2 : public plGraphPatch
{
public:
  plProjectileComponentPatch_1_2()
    : plGraphPatch("plProjectileComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Gravity Multiplier", "GravityMultiplier");
    pNode->RenameProperty("Max Lifetime", "MaxLifetime");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
    pNode->RenameProperty("Collision Layer", "CollisionLayer");
  }
};

plProjectileComponentPatch_1_2 g_plProjectileComponentPatch_1_2;



PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_ProjectileComponent);
