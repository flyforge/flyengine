#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Events/ParticleEventReaction_Prefab.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEventReactionFactory_Prefab, 1, plRTTIDefaultAllocator<plParticleEventReactionFactory_Prefab>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Prefab", m_sPrefab)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
    PLASMA_ENUM_MEMBER_PROPERTY("Alignment", plSurfaceInteractionAlignment, m_Alignment),
    //PLASMA_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("CompatibleAsset_Prefab"), new plExposeColorAlphaAttribute),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEventReaction_Prefab, 1, plRTTIDefaultAllocator<plParticleEventReaction_Prefab>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleEventReactionFactory_Prefab::plParticleEventReactionFactory_Prefab()
{
  // m_Parameters = PLASMA_DEFAULT_NEW(plParticlePrefabParameters);
}

enum class ReactionPrefabVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  // Version_3, // added Prefab parameters

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleEventReactionFactory_Prefab::Save(plStreamWriter& stream) const
{
  SUPER::Save(stream);

  const plUInt8 uiVersion = (int)ReactionPrefabVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_sPrefab;

  // Version 2
  stream << m_Alignment;

  // Version 3
  // stream << m_Parameters->m_FloatParams.GetCount();
  // for (plUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
  //{
  //  stream << m_Parameters->m_FloatParams[i].m_sName;
  //  stream << m_Parameters->m_FloatParams[i].m_Value;
  //}
  // stream << m_Parameters->m_ColorParams.GetCount();
  // for (plUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
  //{
  //  stream << m_Parameters->m_ColorParams[i].m_sName;
  //  stream << m_Parameters->m_ColorParams[i].m_Value;
  //}
}

void plParticleEventReactionFactory_Prefab::Load(plStreamReader& stream)
{
  SUPER::Load(stream);

  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)ReactionPrefabVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  stream >> m_sPrefab;

  if (uiVersion >= 2)
  {
    stream >> m_Alignment;
  }

  // if (uiVersion >= 3)
  //{
  //  plUInt32 numFloats, numColors;

  //  stream >> numFloats;
  //  m_Parameters->m_FloatParams.SetCountUninitialized(numFloats);

  //  for (plUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
  //  {
  //    stream >> m_Parameters->m_FloatParams[i].m_sName;
  //    stream >> m_Parameters->m_FloatParams[i].m_Value;
  //  }

  //  stream >> numColors;
  //  m_Parameters->m_ColorParams.SetCountUninitialized(numColors);

  //  for (plUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
  //  {
  //    stream >> m_Parameters->m_ColorParams[i].m_sName;
  //    stream >> m_Parameters->m_ColorParams[i].m_Value;
  //  }
  //}
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

  // pReaction->m_Parameters = m_Parameters;
}
//
// const plRangeView<const char*, plUInt32> plParticleEventReactionFactory_Prefab::GetParameters() const
//{
//  return plRangeView<const char*, plUInt32>(
//      [this]() -> plUInt32 { return 0; },
//      [this]() -> plUInt32 { return m_Parameters->m_FloatParams.GetCount() + m_Parameters->m_ColorParams.GetCount(); },
//      [this](plUInt32& it) { ++it; },
//      [this](const plUInt32& it) -> const char* {
//        if (it < m_Parameters->m_FloatParams.GetCount())
//          return m_Parameters->m_FloatParams[it].m_sName.GetData();
//        else
//          return m_Parameters->m_ColorParams[it - m_Parameters->m_FloatParams.GetCount()].m_sName.GetData();
//      });
//}
//
// void plParticleEventReactionFactory_Prefab::SetParameter(const char* szKey, const plVariant& var)
//{
//  const plTempHashedString th(szKey);
//  if (var.CanConvertTo<float>())
//  {
//    float value = var.ConvertTo<float>();
//
//    for (plUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
//    {
//      if (m_Parameters->m_FloatParams[i].m_sName == th)
//      {
//        if (m_Parameters->m_FloatParams[i].m_Value != value)
//        {
//          m_Parameters->m_FloatParams[i].m_Value = value;
//        }
//        return;
//      }
//    }
//
//    auto& e = m_Parameters->m_FloatParams.ExpandAndGetRef();
//    e.m_sName.Assign(szKey);
//    e.m_Value = value;
//
//    return;
//  }
//
//  if (var.CanConvertTo<plColor>())
//  {
//    plColor value = var.ConvertTo<plColor>();
//
//    for (plUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
//    {
//      if (m_Parameters->m_ColorParams[i].m_sName == th)
//      {
//        if (m_Parameters->m_ColorParams[i].m_Value != value)
//        {
//          m_Parameters->m_ColorParams[i].m_Value = value;
//        }
//        return;
//      }
//    }
//
//    auto& e = m_Parameters->m_ColorParams.ExpandAndGetRef();
//    e.m_sName.Assign(szKey);
//    e.m_Value = value;
//
//    return;
//  }
//}
//
// void plParticleEventReactionFactory_Prefab::RemoveParameter(const char* szKey)
//{
//  const plTempHashedString th(szKey);
//
//  for (plUInt32 i = 0; i < m_Parameters->m_FloatParams.GetCount(); ++i)
//  {
//    if (m_Parameters->m_FloatParams[i].m_sName == th)
//    {
//      m_Parameters->m_FloatParams.RemoveAtAndSwap(i);
//      return;
//    }
//  }
//
//  for (plUInt32 i = 0; i < m_Parameters->m_ColorParams.GetCount(); ++i)
//  {
//    if (m_Parameters->m_ColorParams[i].m_sName == th)
//    {
//      m_Parameters->m_ColorParams.RemoveAtAndSwap(i);
//      return;
//    }
//  }
//}
//
// bool plParticleEventReactionFactory_Prefab::GetParameter(const char* szKey, plVariant& out_value) const
//{
//  const plTempHashedString th(szKey);
//
//  for (const auto& e : m_Parameters->m_FloatParams)
//  {
//    if (e.m_sName == th)
//    {
//      out_value = e.m_Value;
//      return true;
//    }
//  }
//  for (const auto& e : m_Parameters->m_ColorParams)
//  {
//    if (e.m_sName == th)
//    {
//      out_value = e.m_Value;
//      return true;
//    }
//  }
//  return false;
//}

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
  plQuat qRot;
  qRot.SetFromAxisAndAngle(plVec3(1, 0, 0), plAngle::Radian((float)m_pOwnerEffect->GetRNG().DoubleZeroToOneInclusive() * plMath::Pi<float>() * 2.0f));

  vAlignDir.NormalizeIfNotZero(plVec3::UnitXAxis()).IgnoreResult();

  trans.m_qRotation.SetShortestRotation(plVec3(1, 0, 0), vAlignDir);
  trans.m_qRotation = trans.m_qRotation * qRot;

  plResourceLock<plPrefabResource> pPrefab(m_hPrefab, plResourceAcquireMode::BlockTillLoaded);

  plPrefabInstantiationOptions options;

  pPrefab->InstantiatePrefab(*m_pOwnerEffect->GetWorld(), trans, options);
}
