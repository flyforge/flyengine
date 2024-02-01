#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameComponentsPlugin/Gameplay/ProjectileComponent.h>
#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plProjectileReaction, 2)
  PL_ENUM_CONSTANT(plProjectileReaction::Absorb),
  PL_ENUM_CONSTANT(plProjectileReaction::Reflect),
  PL_ENUM_CONSTANT(plProjectileReaction::Bounce),
  PL_ENUM_CONSTANT(plProjectileReaction::Attach),
  PL_ENUM_CONSTANT(plProjectileReaction::PassThrough)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_TYPE(plProjectileSurfaceInteraction, plNoBase, 3, plRTTIDefaultAllocator<plProjectileSurfaceInteraction>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
    PL_ENUM_MEMBER_PROPERTY("Reaction", plProjectileReaction, m_Reaction),
    PL_MEMBER_PROPERTY("Interaction", m_sInteraction)->AddAttributes(new plDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    PL_MEMBER_PROPERTY("Impulse", m_fImpulse),
    PL_MEMBER_PROPERTY("Damage", m_fDamage),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_COMPONENT_TYPE(plProjectileComponent, 6, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Speed", m_fMetersPerSecond)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("GravityMultiplier", m_fGravityMultiplier),
    PL_MEMBER_PROPERTY("MaxLifetime", m_MaxLifetime)->AddAttributes(new plClampValueAttribute(plTime(), plVariant())),
    PL_MEMBER_PROPERTY("SpawnPrefabOnStatic", m_bSpawnPrefabOnStatic),
    PL_ACCESSOR_PROPERTY("OnDeathPrefab", GetDeathPrefab, SetDeathPrefab)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab", plDependencyFlags::Package)),
    PL_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PL_BITFLAGS_MEMBER_PROPERTY("ShapeTypesToHit", plPhysicsShapeType, m_ShapeTypesToHit)->AddAttributes(new plDefaultValueAttribute(plVariant(plPhysicsShapeType::Default & ~(plPhysicsShapeType::Trigger)))),
    PL_ACCESSOR_PROPERTY("FallbackSurface", GetFallbackSurfaceFile, SetFallbackSurfaceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
    PL_ARRAY_MEMBER_PROPERTY("Interactions", m_SurfaceInteractions),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnTriggered),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 0.4f, plColor::OrangeRed),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
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
  m_bSpawnPrefabOnStatic = false;
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
    queryParams.m_ShapeTypes = m_ShapeTypesToHit;

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
          SpawnDeathPrefab();
          

          GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
          vNewPosition = castResult.m_vPosition;
        }
        else if (interaction.m_Reaction == plProjectileReaction::Reflect || interaction.m_Reaction == plProjectileReaction::Bounce)
        {
          /// \todo Should reflect around the actual hit position
          /// \todo Should preserve travel distance while reflecting

          // const float fLength = (vPos - pEntity->GetGlobalPosition()).GetLength();

          vNewPosition = pEntity->GetGlobalPosition(); // vPos;

          const plVec3 vNewDirection = vCurDirection.GetReflectedVector(castResult.m_vNormal);

          plQuat qRot = plQuat::MakeShortestRotation(vCurDirection, vNewDirection);

          GetOwner()->SetGlobalRotation(qRot * GetOwner()->GetGlobalRotation());

          m_vVelocity = qRot * m_vVelocity;

          if (interaction.m_Reaction == plProjectileReaction::Bounce)
          {
            plResourceLock<plSurfaceResource> pSurface(hSurface, plResourceAcquireMode::BlockTillLoaded);

            if (pSurface)
            {
              m_vVelocity *= pSurface->GetDescriptor().m_fPhysicsRestitution;
            }

            if (m_vVelocity.GetLength() < 1.0f)
            {
              m_vVelocity = plVec3::MakeZero();
              m_fGravityMultiplier = 0.0f;

              if (m_bSpawnPrefabOnStatic)
              {
                SpawnDeathPrefab();
                GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
              }
            }
          }
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

void plProjectileComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fMetersPerSecond;
  s << m_fGravityMultiplier;
  s << m_uiCollisionLayer;
  s << m_MaxLifetime;
  s << m_hDeathPrefab;

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

  // Version 5
  s << m_ShapeTypesToHit;

  // Version 6
  s << m_bSpawnPrefabOnStatic;
}

void plProjectileComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fMetersPerSecond;
  s >> m_fGravityMultiplier;
  s >> m_uiCollisionLayer;
  s >> m_MaxLifetime;
  s >> m_hDeathPrefab;

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

  if (uiVersion >= 5)
  {
    s >> m_ShapeTypesToHit;
  }

  if (uiVersion >= 6)
  {
    s >> m_bSpawnPrefabOnStatic;
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
    if (m_hDeathPrefab.IsValid())
    {
      plResourceManager::PreloadResource(m_hDeathPrefab);
    }
  }

  m_vVelocity = GetOwner()->GetGlobalDirForwards() * m_fMetersPerSecond;
}

void plProjectileComponent::SpawnDeathPrefab()
{
  if (!m_bSpawnPrefabOnStatic)
  {
    return;
  }

  if (m_hDeathPrefab.IsValid())
  {
    plResourceLock<plPrefabResource> pPrefab(m_hDeathPrefab, plResourceAcquireMode::AllowLoadingFallback);

    plPrefabInstantiationOptions options;
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), options, nullptr);
  }
}

void plProjectileComponent::OnTriggered(plMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != s_sSuicide)
    return;

  SpawnDeathPrefab();

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}


void plProjectileComponent::SetDeathPrefab(const char* szPrefab)
{
  plPrefabResourceHandle hPrefab;

  if (!plStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = plResourceManager::LoadResource<plPrefabResource>(szPrefab);
  }

  m_hDeathPrefab = hPrefab;
}

const char* plProjectileComponent::GetDeathPrefab() const
{
  if (!m_hDeathPrefab.IsValid())
    return "";

  return m_hDeathPrefab.GetResourceID();
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

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Gravity Multiplier", "GravityMultiplier");
    pNode->RenameProperty("Max Lifetime", "MaxLifetime");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
    pNode->RenameProperty("Collision Layer", "CollisionLayer");
  }
};

class plProjectileComponentPatch_5_6 : public plGraphPatch
{
public:
  plProjectileComponentPatch_5_6()
    : plGraphPatch("plProjectileComponent", 6)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("TimeoutPrefab", "DeathPrefab");
  }
};

plProjectileComponentPatch_1_2 g_plProjectileComponentPatch_1_2;
