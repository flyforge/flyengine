#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Events/ParticleEventReaction_Prefab.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEventReactionFactory_Prefab, 1, plRTTIDefaultAllocator<plParticleEventReactionFactory_Prefab>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Prefab", m_sPrefab)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
    PL_ENUM_MEMBER_PROPERTY("Alignment", plSurfaceInteractionAlignment, m_Alignment),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEventReaction_Prefab, 1, plRTTIDefaultAllocator<plParticleEventReaction_Prefab>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleEventReactionFactory_Prefab::plParticleEventReactionFactory_Prefab() = default;

enum class ReactionPrefabVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleEventReactionFactory_Prefab::Save(plStreamWriter& inout_stream) const
{
  SUPER::Save(inout_stream);

  const plUInt8 uiVersion = (int)ReactionPrefabVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sPrefab;

  // Version 2
  inout_stream << m_Alignment;
}

void plParticleEventReactionFactory_Prefab::Load(plStreamReader& inout_stream)
{
  SUPER::Load(inout_stream);

  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion <= (int)ReactionPrefabVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  inout_stream >> m_sPrefab;

  if (uiVersion >= 2)
  {
    inout_stream >> m_Alignment;
  }
}


const plRTTI* plParticleEventReactionFactory_Prefab::GetEventReactionType() const
{
  return plGetStaticRTTI<plParticleEventReaction_Prefab>();
}


void plParticleEventReactionFactory_Prefab::CopyReactionProperties(plParticleEventReaction* pObject, bool bFirstTime) const
{
  plParticleEventReaction_Prefab* pReaction = static_cast<plParticleEventReaction_Prefab*>(pObject);

  pReaction->m_hPrefab.Invalidate();
  pReaction->m_Alignment = m_Alignment;

  if (!m_sPrefab.IsEmpty())
    pReaction->m_hPrefab = plResourceManager::LoadResource<plPrefabResource>(m_sPrefab);
}

//////////////////////////////////////////////////////////////////////////

plParticleEventReaction_Prefab::plParticleEventReaction_Prefab() = default;
plParticleEventReaction_Prefab::~plParticleEventReaction_Prefab() = default;

void plParticleEventReaction_Prefab::ProcessEvent(const plParticleEvent& e)
{
  if (!m_hPrefab.IsValid())
    return;

  plTransform trans;
  trans.m_vScale.Set(1.0f);
  trans.m_vPosition = e.m_vPosition;

  plVec3 vAlignDir = e.m_vNormal;

  switch (m_Alignment)
  {
    case plSurfaceInteractionAlignment::IncidentDirection:
      vAlignDir = -e.m_vDirection;
      break;

    case plSurfaceInteractionAlignment::ReflectedDirection:
      vAlignDir = e.m_vDirection.GetReflectedVector(e.m_vNormal);
      break;

    case plSurfaceInteractionAlignment::ReverseSurfaceNormal:
      vAlignDir = -e.m_vNormal;
      break;

    case plSurfaceInteractionAlignment::ReverseIncidentDirection:
      vAlignDir = e.m_vDirection;
      ;
      break;

    case plSurfaceInteractionAlignment::ReverseReflectedDirection:
      vAlignDir = -e.m_vDirection.GetReflectedVector(e.m_vNormal);
      break;

    case plSurfaceInteractionAlignment::SurfaceNormal:
      break;
  }

  // rotate the prefab randomly along its main axis (the X axis)
  plQuat qRot = plQuat::MakeFromAxisAndAngle(plVec3(1, 0, 0), plAngle::MakeFromRadian((float)m_pOwnerEffect->GetRNG().DoubleZeroToOneInclusive() * plMath::Pi<float>() * 2.0f));

  vAlignDir.NormalizeIfNotZero(plVec3::MakeAxisX()).IgnoreResult();

  trans.m_qRotation = plQuat::MakeShortestRotation(plVec3(1, 0, 0), vAlignDir);
  trans.m_qRotation = trans.m_qRotation * qRot;

  plResourceLock<plPrefabResource> pPrefab(m_hPrefab, plResourceAcquireMode::BlockTillLoaded);

  plPrefabInstantiationOptions options;

  pPrefab->InstantiatePrefab(*m_pOwnerEffect->GetWorld(), trans, options);
}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Events_ParticleEventReaction_Prefab);

