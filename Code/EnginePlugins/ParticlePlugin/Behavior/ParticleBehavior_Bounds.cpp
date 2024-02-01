#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Bounds.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehaviorFactory_Bounds, 1, plRTTIDefaultAllocator<plParticleBehaviorFactory_Bounds>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    PL_MEMBER_PROPERTY("BoxExtents", m_vBoxExtents)->AddAttributes(new plDefaultValueAttribute(plVec3(2, 2, 2))),
    PL_ENUM_MEMBER_PROPERTY("OutOfBoundsMode", plParticleOutOfBoundsMode, m_OutOfBoundsMode),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plBoxVisualizerAttribute("BoxExtents", 1.0f, plColor::LightGreen, nullptr, plVisualizerAnchor::Center, plVec3(1.0f), "PositionOffset")
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehavior_Bounds, 1, plRTTIDefaultAllocator<plParticleBehavior_Bounds>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleBehaviorFactory_Bounds::plParticleBehaviorFactory_Bounds() = default;

const plRTTI* plParticleBehaviorFactory_Bounds::GetBehaviorType() const
{
  return plGetStaticRTTI<plParticleBehavior_Bounds>();
}

void plParticleBehaviorFactory_Bounds::CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const
{
  plParticleBehavior_Bounds* pBehavior = static_cast<plParticleBehavior_Bounds*>(pObject);

  pBehavior->m_vPositionOffset = m_vPositionOffset;
  pBehavior->m_vBoxExtents = m_vBoxExtents;
  pBehavior->m_OutOfBoundsMode = m_OutOfBoundsMode;
}

enum class BehaviorBoundsVersion
{
  Version_0 = 0,
  Version_1, // added out of bounds mode

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleBehaviorFactory_Bounds::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = (int)BehaviorBoundsVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_vPositionOffset;
  inout_stream << m_vBoxExtents;

  // version 1
  inout_stream << m_OutOfBoundsMode;
}

void plParticleBehaviorFactory_Bounds::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion <= (int)BehaviorBoundsVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_vPositionOffset;
  inout_stream >> m_vBoxExtents;

  if (uiVersion >= 1)
  {
    inout_stream >> m_OutOfBoundsMode;
  }
}

void plParticleBehavior_Bounds::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void plParticleBehavior_Bounds::QueryOptionalStreams()
{
  m_pStreamLastPosition = GetOwnerSystem()->QueryStream("LastPosition", plProcessingStream::DataType::Float3);
}

void plParticleBehavior_Bounds::Process(plUInt64 uiNumElements)
{
  PL_PROFILE_SCOPE("PFX: Bounds");

  const plSimdTransform trans = plSimdConversion::ToTransform(GetOwnerSystem()->GetTransform());
  const plSimdTransform invTrans = trans.GetInverse();

  const plSimdVec4f boxCenter = plSimdConversion::ToVec3(m_vPositionOffset);
  const plSimdVec4f boxExt = plSimdConversion::ToVec3(m_vBoxExtents);
  const plSimdVec4f halfExtPos = plSimdConversion::ToVec3(m_vBoxExtents) * 0.5f;
  const plSimdVec4f halfExtNeg = -halfExtPos;

  plProcessingStreamIterator<plSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  if (m_OutOfBoundsMode == plParticleOutOfBoundsMode::Teleport)
  {
    plVec3* pLastPosition = nullptr;

    if (m_pStreamLastPosition)
    {
      pLastPosition = m_pStreamLastPosition->GetWritableData<plVec3>();
    }

    while (!itPosition.HasReachedEnd())
    {
      const plSimdVec4f globalPosCur = itPosition.Current();
      const plSimdVec4f localPosCur = invTrans.TransformPosition(globalPosCur) - boxCenter;

      const plSimdVec4f localPosAdd = localPosCur + boxExt;
      const plSimdVec4f localPosSub = localPosCur - boxExt;

      plSimdVec4f localPosNew;
      localPosNew = plSimdVec4f::Select(localPosCur > halfExtPos, localPosSub, localPosCur);
      localPosNew = plSimdVec4f::Select(localPosCur < halfExtNeg, localPosAdd, localPosNew);

      localPosNew += boxCenter;
      const plSimdVec4f globalPosNew = trans.TransformPosition(localPosNew);

      if (m_pStreamLastPosition)
      {
        const plSimdVec4f posDiff = globalPosNew - globalPosCur;
        *pLastPosition += plSimdConversion::ToVec3(posDiff);
        ++pLastPosition;
      }

      itPosition.Current() = globalPosNew;
      itPosition.Advance();
    }
  }
  else
  {
    plUInt32 idx = 0;

    while (!itPosition.HasReachedEnd())
    {
      const plSimdVec4f globalPosCur = itPosition.Current();
      const plSimdVec4f localPosCur = invTrans.TransformPosition(globalPosCur) - boxCenter;

      if ((localPosCur > halfExtPos).AnySet() || (localPosCur < halfExtNeg).AnySet())
      {
        m_pStreamGroup->RemoveElement(idx);
      }

      ++idx;
      itPosition.Advance();
    }
  }
}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Bounds);

