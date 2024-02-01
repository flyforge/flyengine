#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_CylinderPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializerFactory_CylinderPosition, 2, plRTTIDefaultAllocator<plParticleInitializerFactory_CylinderPosition>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    PL_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(0.25f), new plClampValueAttribute(0.01f, 100.0f)),
    PL_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 100.0f)),
    PL_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    PL_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    PL_MEMBER_PROPERTY("Speed", m_Speed),
    PL_MEMBER_PROPERTY("ScaleRadiusParam", m_sScaleRadiusParameter),
    PL_MEMBER_PROPERTY("ScaleHeightParam", m_sScaleHeightParameter),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCylinderVisualizerAttribute(plBasisAxis::PositiveZ, "Height", "Radius", plColor::MediumVioletRed, nullptr, plVisualizerAnchor::Center, plVec3(1.0f), "PositionOffset")
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializer_CylinderPosition, 1, plRTTIDefaultAllocator<plParticleInitializer_CylinderPosition>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleInitializerFactory_CylinderPosition::plParticleInitializerFactory_CylinderPosition()
{
  m_vPositionOffset.SetZero();
  m_fRadius = 0.25f;
  m_fHeight = 1.0f;
  m_bSpawnOnSurface = false;
  m_bSetVelocity = false;
}

const plRTTI* plParticleInitializerFactory_CylinderPosition::GetInitializerType() const
{
  return plGetStaticRTTI<plParticleInitializer_CylinderPosition>();
}

void plParticleInitializerFactory_CylinderPosition::CopyInitializerProperties(plParticleInitializer* pInitializer0, bool bFirstTime) const
{
  plParticleInitializer_CylinderPosition* pInitializer = static_cast<plParticleInitializer_CylinderPosition*>(pInitializer0);

  const float fScaleRadius = pInitializer->GetOwnerEffect()->GetFloatParameter(plTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);
  const float fScaleHeight = pInitializer->GetOwnerEffect()->GetFloatParameter(plTempHashedString(m_sScaleHeightParameter.GetData()), 1.0f);

  pInitializer->m_vPositionOffset = m_vPositionOffset;
  pInitializer->m_fRadius = plMath::Max(m_fRadius * fScaleRadius, 0.01f); // prevent 0 radius
  pInitializer->m_fHeight = plMath::Max(m_fHeight * fScaleHeight, 0.0f);
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_Speed = m_Speed;
}

float plParticleInitializerFactory_CylinderPosition::GetSpawnCountMultiplier(const plParticleEffectInstance* pEffect) const
{
  const float fScaleRadius = pEffect->GetFloatParameter(plTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);
  const float fScaleHeight = pEffect->GetFloatParameter(plTempHashedString(m_sScaleHeightParameter.GetData()), 1.0f);

  if (m_bSpawnOnSurface)
  {
    const float s0 = /* 2.0f * plMath::Pi<float>() * m_fRadius **/ m_fRadius + /* 2.0f * plMath::Pi<float>() * m_fRadius **/ m_fHeight;
    const float s1 = /* 2.0f * plMath::Pi<float>() * m_fRadius **/ m_fRadius * fScaleRadius * fScaleRadius +
                     /*2.0f * plMath::Pi<float>() * m_fRadius **/ fScaleRadius * m_fHeight * fScaleHeight;

    return s1 / s0;
  }
  else
  {
    const float v0 = 1.0f /* plMath::Pi<float>() * m_fRadius * m_fRadius*/;
    const float v1 = 1.0f /* plMath::Pi<float>() * m_fRadius * m_fRadius*/ * fScaleRadius * fScaleRadius;

    return v1 / v0;
  }
}

void plParticleInitializerFactory_CylinderPosition::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 3;
  inout_stream << uiVersion;

  inout_stream << m_fRadius;
  inout_stream << m_fHeight;
  inout_stream << m_bSpawnOnSurface;
  inout_stream << m_bSetVelocity;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleRadiusParameter;
  inout_stream << m_sScaleHeightParameter;
}

void plParticleInitializerFactory_CylinderPosition::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fRadius;
  inout_stream >> m_fHeight;
  inout_stream >> m_bSpawnOnSurface;
  inout_stream >> m_bSetVelocity;
  inout_stream >> m_Speed.m_Value;
  inout_stream >> m_Speed.m_fVariance;

  if (uiVersion >= 2)
  {
    inout_stream >> m_vPositionOffset;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_sScaleRadiusParameter;
    inout_stream >> m_sScaleHeightParameter;
  }
}

void plParticleInitializerFactory_CylinderPosition::QueryFinalizerDependencies(plSet<const plRTTI*>& inout_finalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_finalizerDeps.Insert(plGetStaticRTTI<plParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void plParticleInitializer_CylinderPosition::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", plProcessingStream::DataType::Float3, &m_pStreamVelocity, true);
  }
}

void plParticleInitializer_CylinderPosition::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  PL_PROFILE_SCOPE("PFX: Cylinder Position");

  const plVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  plVec4* pPosition = m_pStreamPosition->GetWritableData<plVec4>();
  plVec3* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<plVec3>() : nullptr;

  plRandom& rng = GetRNG();

  const float fRadiusSqr = m_fRadius * m_fRadius;
  const float fHalfHeight = m_fHeight * 0.5f;

  const plTransform trans = GetOwnerSystem()->GetTransform();

  for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    plVec3 pos;
    float len = 0.0f;
    pos.z = 0.0f;

    do
    {
      pos.x = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);
      pos.y = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);

      len = pos.GetLengthSquared();
    } while (len > fRadiusSqr ||
             len <= 0.000001f); // prevent spawning at the exact center (note: this has to be smaller than the minimum allowed radius sqr)

    plVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * m_fRadius;

    if (m_fHeight > 0)
    {
      pos.z = (float)rng.DoubleMinMax(-fHalfHeight, fHalfHeight);
    }

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

class plParticleInitializerFactory_CylinderPosition_1_2 : public plGraphPatch
{
public:
  plParticleInitializerFactory_CylinderPosition_1_2()
    : plGraphPatch("plParticleInitializerFactory_CylinderPosition", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

plParticleInitializerFactory_CylinderPosition_1_2 g_plParticleInitializerFactory_CylinderPosition_1_2;

PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
