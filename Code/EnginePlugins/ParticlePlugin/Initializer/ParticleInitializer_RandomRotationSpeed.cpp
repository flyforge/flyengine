#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomRotationSpeed.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializerFactory_RandomRotationSpeed, 2, plRTTIDefaultAllocator<plParticleInitializerFactory_RandomRotationSpeed>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("RandomStartAngle", m_bRandomStartAngle),
    PLASMA_MEMBER_PROPERTY("DegreesPerSecond", m_RotationSpeed)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(90)), new plClampValueAttribute(plAngle::MakeFromDegree(0), plVariant())),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializer_RandomRotationSpeed, 1, plRTTIDefaultAllocator<plParticleInitializer_RandomRotationSpeed>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plRTTI* plParticleInitializerFactory_RandomRotationSpeed::GetInitializerType() const
{
  return plGetStaticRTTI<plParticleInitializer_RandomRotationSpeed>();
}

void plParticleInitializerFactory_RandomRotationSpeed::CopyInitializerProperties(plParticleInitializer* pInitializer0, bool bFirstTime) const
{
  plParticleInitializer_RandomRotationSpeed* pInitializer = static_cast<plParticleInitializer_RandomRotationSpeed*>(pInitializer0);

  pInitializer->m_RotationSpeed = m_RotationSpeed;
  pInitializer->m_bRandomStartAngle = m_bRandomStartAngle;
}

enum class InitializerRandomRotationVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added start offset

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleInitializerFactory_RandomRotationSpeed::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)InitializerRandomRotationVersion::Version_Current;
  stream << uiVersion;

  stream << m_RotationSpeed.m_Value;
  stream << m_RotationSpeed.m_fVariance;

  // Version 2
  stream << m_bRandomStartAngle;
}

void plParticleInitializerFactory_RandomRotationSpeed::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_RotationSpeed.m_Value;
  stream >> m_RotationSpeed.m_fVariance;

  if (uiVersion >= 2)
  {
    stream >> m_bRandomStartAngle;
  }
}


void plParticleInitializer_RandomRotationSpeed::CreateRequiredStreams()
{
  CreateStream("RotationSpeed", plProcessingStream::DataType::Half, &m_pStreamRotationSpeed, true);
  CreateStream("RotationOffset", plProcessingStream::DataType::Half, &m_pStreamRotationOffset, true);
}

void plParticleInitializer_RandomRotationSpeed::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: Random Rotation");

  plFloat16* pSpeed = m_pStreamRotationSpeed->GetWritableData<plFloat16>();

  // speed
  if (m_RotationSpeed.m_Value != plAngle::MakeFromRadian(0))
  {
    plRandom& rng = GetRNG();

    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float value = (float)rng.DoubleVariance(m_RotationSpeed.m_Value.GetRadian(), m_RotationSpeed.m_fVariance);

      pSpeed[i] = m_bPositiveSign ? value : -value;
      m_bPositiveSign = !m_bPositiveSign;
    }
  }
  else
  {
    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pSpeed[i] = 0;
    }
  }

  // offset
  if (m_bRandomStartAngle)
  {
    plFloat16* pOffset = m_pStreamRotationOffset->GetWritableData<plFloat16>();

    plRandom& rng = GetRNG();

    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pOffset[i] = (float)rng.DoubleInRange(-plMath::Pi<double>(), +plMath::Pi<double>());
    }
  }
  else
  {
    plFloat16* pOffset = m_pStreamRotationOffset->GetWritableData<plFloat16>();

    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pOffset[i] = 0;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class plParticleInitializerFactory_RandomRotationSpeed_1_2 : public plGraphPatch
{
public:
  plParticleInitializerFactory_RandomRotationSpeed_1_2()
    : plGraphPatch("plParticleInitializerFactory_RandomRotationSpeed", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("DegreesPerSecond").IgnoreResult();
  }
};

plParticleInitializerFactory_RandomRotationSpeed_1_2 g_plParticleInitializerFactory_RandomRotationSpeed_1_2;

PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);
