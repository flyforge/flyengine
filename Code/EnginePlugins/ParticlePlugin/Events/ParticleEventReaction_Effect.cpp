#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Events/ParticleEventReaction_Effect.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEventReactionFactory_Effect, 1, plRTTIDefaultAllocator<plParticleEventReactionFactory_Effect>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Effect", m_sEffect)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Particle_Effect")),
    PLASMA_ENUM_MEMBER_PROPERTY("Alignment", plSurfaceInteractionAlignment, m_Alignment),
    PLASMA_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("Effect"), new plExposeColorAlphaAttribute),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEventReaction_Effect, 1, plRTTIDefaultAllocator<plParticleEventReaction_Effect>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleEventReactionFactory_Effect::plParticleEventReactionFactory_Effect()
{
  m_pParameters = PLASMA_DEFAULT_NEW(plParticleEffectParameters);
}

enum class ReactionEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added effect parameters
  Version_3, // added alignment

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleEventReactionFactory_Effect::Save(plStreamWriter& stream) const
{
  SUPER::Save(stream);

  const plUInt8 uiVersion = (int)ReactionEffectVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_sEffect;

  // Version 2
  stream << m_pParameters->m_FloatParams.GetCount();
  for (plUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
  {
    stream << m_pParameters->m_FloatParams[i].m_sName;
    stream << m_pParameters->m_FloatParams[i].m_Value;
  }
  stream << m_pParameters->m_ColorParams.GetCount();
  for (plUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
  {
    stream << m_pParameters->m_ColorParams[i].m_sName;
    stream << m_pParameters->m_ColorParams[i].m_Value;
  }

  // Version 3
  stream << m_Alignment;
}

void plParticleEventReactionFactory_Effect::Load(plStreamReader& stream)
{
  SUPER::Load(stream);

  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)ReactionEffectVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  stream >> m_sEffect;

  if (uiVersion >= 2)
  {
    plUInt32 numFloats, numColors;

    stream >> numFloats;
    m_pParameters->m_FloatParams.SetCountUninitialized(numFloats);

    for (plUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
    {
      stream >> m_pParameters->m_FloatParams[i].m_sName;
      stream >> m_pParameters->m_FloatParams[i].m_Value;
    }

    stream >> numColors;
    m_pParameters->m_ColorParams.SetCountUninitialized(numColors);

    for (plUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
    {
      stream >> m_pParameters->m_ColorParams[i].m_sName;
      stream >> m_pParameters->m_ColorParams[i].m_Value;
    }
  }

  if (uiVersion >= 3)
  {
    stream >> m_Alignment;
  }
}


const plRTTI* plParticleEventReactionFactory_Effect::GetEventReactionType() const
{
  return plGetStaticRTTI<plParticleEventReaction_Effect>();
}


void plParticleEventReactionFactory_Effect::CopyReactionProperties(plParticleEventReaction* pObject, bool bFirstTime) const
{
  plParticleEventReaction_Effect* pReaction = static_cast<plParticleEventReaction_Effect*>(pObject);

  pReaction->m_hEffect.Invalidate();
  pReaction->m_Alignment = m_Alignment;

  if (!m_sEffect.IsEmpty())
    pReaction->m_hEffect = plResourceManager::LoadResource<plParticleEffectResource>(m_sEffect);

  pReaction->m_Parameters = m_pParameters;
}

const plRangeView<const char*, plUInt32> plParticleEventReactionFactory_Effect::GetParameters() const
{
  return plRangeView<const char*, plUInt32>([this]() -> plUInt32 { return 0; },
    [this]() -> plUInt32 { return m_pParameters->m_FloatParams.GetCount() + m_pParameters->m_ColorParams.GetCount(); }, [this](plUInt32& it) { ++it; },
    [this](const plUInt32& it) -> const char* {
      if (it < m_pParameters->m_FloatParams.GetCount())
        return m_pParameters->m_FloatParams[it].m_sName.GetData();
      else
        return m_pParameters->m_ColorParams[it - m_pParameters->m_FloatParams.GetCount()].m_sName.GetData();
    });
}

void plParticleEventReactionFactory_Effect::SetParameter(const char* szKey, const plVariant& var)
{
  const plTempHashedString th(szKey);
  if (var.CanConvertTo<float>())
  {
    float value = var.ConvertTo<float>();

    for (plUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
    {
      if (m_pParameters->m_FloatParams[i].m_sName == th)
      {
        if (m_pParameters->m_FloatParams[i].m_Value != value)
        {
          m_pParameters->m_FloatParams[i].m_Value = value;
        }
        return;
      }
    }

    auto& e = m_pParameters->m_FloatParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }

  if (var.CanConvertTo<plColor>())
  {
    plColor value = var.ConvertTo<plColor>();

    for (plUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
    {
      if (m_pParameters->m_ColorParams[i].m_sName == th)
      {
        if (m_pParameters->m_ColorParams[i].m_Value != value)
        {
          m_pParameters->m_ColorParams[i].m_Value = value;
        }
        return;
      }
    }

    auto& e = m_pParameters->m_ColorParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }
}

void plParticleEventReactionFactory_Effect::RemoveParameter(const char* szKey)
{
  const plTempHashedString th(szKey);

  for (plUInt32 i = 0; i < m_pParameters->m_FloatParams.GetCount(); ++i)
  {
    if (m_pParameters->m_FloatParams[i].m_sName == th)
    {
      m_pParameters->m_FloatParams.RemoveAtAndSwap(i);
      return;
    }
  }

  for (plUInt32 i = 0; i < m_pParameters->m_ColorParams.GetCount(); ++i)
  {
    if (m_pParameters->m_ColorParams[i].m_sName == th)
    {
      m_pParameters->m_ColorParams.RemoveAtAndSwap(i);
      return;
    }
  }
}

bool plParticleEventReactionFactory_Effect::GetParameter(const char* szKey, plVariant& out_value) const
{
  const plTempHashedString th(szKey);

  for (const auto& e : m_pParameters->m_FloatParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  for (const auto& e : m_pParameters->m_ColorParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////

plParticleEventReaction_Effect::plParticleEventReaction_Effect() = default;
plParticleEventReaction_Effect::~plParticleEventReaction_Effect() = default;

void plParticleEventReaction_Effect::ProcessEvent(const plParticleEvent& e)
{
  if (!m_hEffect.IsValid())
    return;

  plGameObjectDesc god;
  god.m_bDynamic = true;
  god.m_LocalPosition = e.m_vPosition;

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

  if (!vAlignDir.IsZero())
  {
    god.m_LocalRotation.SetShortestRotation(plVec3(0, 0, 1), vAlignDir);
  }

  plGameObject* pObject = nullptr;
  m_pOwnerEffect->GetWorld()->CreateObject(god, pObject);

  plParticleComponent* pComponent = nullptr;
  plParticleComponent::CreateComponent(pObject, pComponent);

  pComponent->m_uiRandomSeed = m_pOwnerEffect->GetRandomSeed();

  pComponent->m_bIfContinuousStopRightAway = true;
  pComponent->m_OnFinishedAction = plOnComponentFinishedAction2::DeleteGameObject;
  pComponent->SetParticleEffect(m_hEffect);

  if (!m_Parameters->m_FloatParams.IsEmpty())
  {
    pComponent->m_bFloatParamsChanged = true;
    pComponent->m_FloatParams = m_Parameters->m_FloatParams;
  }

  if (!m_Parameters->m_ColorParams.IsEmpty())
  {
    pComponent->m_bColorParamsChanged = true;
    pComponent->m_ColorParams = m_Parameters->m_ColorParams;
  }
}
