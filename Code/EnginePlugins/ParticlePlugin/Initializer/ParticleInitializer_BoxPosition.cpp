#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_BoxPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializerFactory_BoxPosition, 1, plRTTIDefaultAllocator<plParticleInitializerFactory_BoxPosition>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    PLASMA_MEMBER_PROPERTY("Size", m_vSize)->AddAttributes(new plDefaultValueAttribute(plVec3(0, 0, 0))),
    PLASMA_MEMBER_PROPERTY("ScaleXParam", m_sScaleXParameter),
    PLASMA_MEMBER_PROPERTY("ScaleYParam", m_sScaleYParameter),
    PLASMA_MEMBER_PROPERTY("ScaleZParam", m_sScaleZParameter),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plBoxVisualizerAttribute("Size", 1.0f, plColor::MediumVioletRed, nullptr, plVisualizerAnchor::Center, plVec3(1,1,1), "PositionOffset")
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializer_BoxPosition, 1, plRTTIDefaultAllocator<plParticleInitializer_BoxPosition>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleInitializerFactory_BoxPosition::plParticleInitializerFactory_BoxPosition()
{
  m_vPositionOffset.SetZero();
  m_vSize.Set(0, 0, 0);
}

const plRTTI* plParticleInitializerFactory_BoxPosition::GetInitializerType() const
{
  return plGetStaticRTTI<plParticleInitializer_BoxPosition>();
}

void plParticleInitializerFactory_BoxPosition::CopyInitializerProperties(plParticleInitializer* pInitializer0, bool bFirstTime) const
{
  plParticleInitializer_BoxPosition* pInitializer = static_cast<plParticleInitializer_BoxPosition*>(pInitializer0);

  const float fScaleX = pInitializer->GetOwnerEffect()->GetFloatParameter(plTempHashedString(m_sScaleXParameter.GetData()), 1.0f);
  const float fScaleY = pInitializer->GetOwnerEffect()->GetFloatParameter(plTempHashedString(m_sScaleYParameter.GetData()), 1.0f);
  const float fScaleZ = pInitializer->GetOwnerEffect()->GetFloatParameter(plTempHashedString(m_sScaleZParameter.GetData()), 1.0f);

  plVec3 vSize = m_vSize;
  vSize.x *= fScaleX;
  vSize.y *= fScaleY;
  vSize.z *= fScaleZ;

  pInitializer->m_vPositionOffset = m_vPositionOffset;
  pInitializer->m_vSize = vSize;
}

float plParticleInitializerFactory_BoxPosition::GetSpawnCountMultiplier(const plParticleEffectInstance* pEffect) const
{
  const float fScaleX = pEffect->GetFloatParameter(plTempHashedString(m_sScaleXParameter.GetData()), 1.0f);
  const float fScaleY = pEffect->GetFloatParameter(plTempHashedString(m_sScaleYParameter.GetData()), 1.0f);
  const float fScaleZ = pEffect->GetFloatParameter(plTempHashedString(m_sScaleZParameter.GetData()), 1.0f);

  float fSpawnMultiplier = 1.0f;

  if (m_vSize.x != 0.0f)
    fSpawnMultiplier *= fScaleX;

  if (m_vSize.y != 0.0f)
    fSpawnMultiplier *= fScaleY;

  if (m_vSize.z != 0.0f)
    fSpawnMultiplier *= fScaleZ;

  return fSpawnMultiplier;
}

void plParticleInitializerFactory_BoxPosition::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = 3;
  stream << uiVersion;

  stream << m_vSize;

  // version 2
  stream << m_vPositionOffset;

  // version 3
  stream << m_sScaleXParameter;
  stream << m_sScaleYParameter;
  stream << m_sScaleZParameter;
}

void plParticleInitializerFactory_BoxPosition::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_vSize;

  if (uiVersion >= 2)
  {
    stream >> m_vPositionOffset;
  }

  if (uiVersion >= 3)
  {
    stream >> m_sScaleXParameter;
    stream >> m_sScaleYParameter;
    stream >> m_sScaleZParameter;
  }
}

void plParticleInitializer_BoxPosition::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, true);
}

void plParticleInitializer_BoxPosition::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: Box Position");

  plSimdVec4f* pPosition = m_pStreamPosition->GetWritableData<plSimdVec4f>();

  plRandom& rng = GetRNG();

  if (m_vSize.IsZero())
  {
    plVec4 pos0 = (GetOwnerSystem()->GetTransform() * m_vPositionOffset).GetAsVec4(0);

    plSimdVec4f pos;
    pos.Load<4>(&pos0.x);

    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pPosition[i] = pos;
    }
  }
  else
  {
    plTransform ownerTransform = GetOwnerSystem()->GetTransform();

    plSimdVec4f pos;
    plSimdTransform transform;
    transform.m_Position.Load<3>(&ownerTransform.m_vPosition.x);
    transform.m_Rotation.m_v.Load<4>(&ownerTransform.m_qRotation.x);
    transform.m_Scale.Load<3>(&ownerTransform.m_vScale.x);

    float p0[4];
    p0[3] = 0;

    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      p0[0] = (float)(rng.DoubleMinMax(-m_vSize.x, m_vSize.x) * 0.5) + m_vPositionOffset.x;
      p0[1] = (float)(rng.DoubleMinMax(-m_vSize.y, m_vSize.y) * 0.5) + m_vPositionOffset.y;
      p0[2] = (float)(rng.DoubleMinMax(-m_vSize.z, m_vSize.z) * 0.5) + m_vPositionOffset.z;

      pos.Load<4>(p0);

      pPosition[i] = transform.TransformPosition(pos);
    }
  }
}



PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
