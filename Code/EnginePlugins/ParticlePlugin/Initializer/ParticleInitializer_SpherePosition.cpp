#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_SpherePosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializerFactory_SpherePosition, 2, plRTTIDefaultAllocator<plParticleInitializerFactory_SpherePosition>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    PLASMA_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(0.25f), new plClampValueAttribute(0.01f, 100.0f)),
    PLASMA_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    PLASMA_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    PLASMA_MEMBER_PROPERTY("Speed", m_Speed),
    PLASMA_MEMBER_PROPERTY("ScaleRadiusParam", m_sScaleRadiusParameter),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plSphereVisualizerAttribute("Radius", plColor::MediumVioletRed, nullptr, plVisualizerAnchor::Center, plVec3::OneVector(), "PositionOffset"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializer_SpherePosition, 1, plRTTIDefaultAllocator<plParticleInitializer_SpherePosition>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleInitializerFactory_SpherePosition::plParticleInitializerFactory_SpherePosition()
{
  m_fRadius = 0.25f;
  m_vPositionOffset.SetZero();
  m_bSpawnOnSurface = false;
  m_bSetVelocity = false;
}

const plRTTI* plParticleInitializerFactory_SpherePosition::GetInitializerType() const
{
  return plGetStaticRTTI<plParticleInitializer_SpherePosition>();
}

void plParticleInitializerFactory_SpherePosition::CopyInitializerProperties(plParticleInitializer* pInitializer0, bool bFirstTime) const
{
  plParticleInitializer_SpherePosition* pInitializer = static_cast<plParticleInitializer_SpherePosition*>(pInitializer0);

  const float fScale = pInitializer->GetOwnerEffect()->GetFloatParameter(plTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);

  pInitializer->m_fRadius = plMath::Max(m_fRadius * fScale, 0.01f); // prevent 0 radius
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_Speed = m_Speed;
  pInitializer->m_vPositionOffset = m_vPositionOffset;
}

float plParticleInitializerFactory_SpherePosition::GetSpawnCountMultiplier(const plParticleEffectInstance* pEffect) const
{
  const float fScale = pEffect->GetFloatParameter(plTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);

  if (m_fRadius != 0.0f && fScale != 1.0f)
  {
    if (m_bSpawnOnSurface)
    {
      // original surface area
      const float s0 = 1.0f; /*4.0f * plMath::Pi<float>() * m_fRadius * m_fRadius; */
      // new surface area
      const float s1 = 1.0f /*4.0f * plMath::Pi<float>() * m_fRadius * m_fRadius */ * fScale * fScale;

      return s1 / s0;
    }
    else
    {
      // original volume
      const float v0 = 1.0f;
      /* 4.0f / 3.0f * plMath::Pi<float>() * m_fRadius* m_fRadius* m_fRadius; */
      // new volume
      const float v1 = 1.0f /* 4.0f / 3.0f * plMath::Pi<float>() * m_fRadius * m_fRadius * m_fRadius*/ * fScale * fScale * fScale;

      return v1 / v0;
    }
  }

  return 1.0f;
}

void plParticleInitializerFactory_SpherePosition::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = 3;
  stream << uiVersion;

  stream << m_fRadius;
  stream << m_bSpawnOnSurface;
  stream << m_bSetVelocity;
  stream << m_Speed.m_Value;
  stream << m_Speed.m_fVariance;

  // version 2
  stream << m_vPositionOffset;

  // version 3
  stream << m_sScaleRadiusParameter;
}

void plParticleInitializerFactory_SpherePosition::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_fRadius;
  stream >> m_bSpawnOnSurface;
  stream >> m_bSetVelocity;
  stream >> m_Speed.m_Value;
  stream >> m_Speed.m_fVariance;

  if (uiVersion >= 2)
  {
    stream >> m_vPositionOffset;
  }

  if (uiVersion >= 3)
  {
    stream >> m_sScaleRadiusParameter;
  }
}

void plParticleInitializerFactory_SpherePosition::QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_FinalizerDeps.Insert(plGetStaticRTTI<plParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void plParticleInitializer_SpherePosition::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", plProcessingStream::DataType::Float3, &m_pStreamVelocity, true);
  }
}

void plParticleInitializer_SpherePosition::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: Sphere Position");

  const plVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  plVec4* pPosition = m_pStreamPosition->GetWritableData<plVec4>();
  plVec3* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<plVec3>() : nullptr;

  plRandom& rng = GetRNG();

  const plTransform trans = GetOwnerSystem()->GetTransform();

  for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    plVec3 pos = plVec3::CreateRandomPointInSphere(rng) * m_fRadius;
    plVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * m_fRadius;

    pos += m_vPositionOffset;

    if (m_bSetVelocity)
    {
      const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance);

      pVelocity[i] = startVel + trans.m_qRotation * normalPos * fSpeed;
    }

    pPosition[i] = (trans * pos).GetAsVec4(0);
  }
}

//////////////////////////////////////////////////////////////////////////

class plParticleInitializerFactory_SpherePosition_1_2 : public plGraphPatch
{
public:
  plParticleInitializerFactory_SpherePosition_1_2()
    : plGraphPatch("plParticleInitializerFactory_SpherePosition", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

plParticleInitializerFactory_SpherePosition_1_2 g_plParticleInitializerFactory_SpherePosition_1_2;

PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
