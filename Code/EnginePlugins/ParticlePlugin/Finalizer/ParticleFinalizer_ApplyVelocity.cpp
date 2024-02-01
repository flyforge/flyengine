#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleFinalizerFactory_ApplyVelocity, 1, plRTTIDefaultAllocator<plParticleFinalizerFactory_ApplyVelocity>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleFinalizer_ApplyVelocity, 1, plRTTIDefaultAllocator<plParticleFinalizer_ApplyVelocity>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleFinalizerFactory_ApplyVelocity::plParticleFinalizerFactory_ApplyVelocity() = default;

const plRTTI* plParticleFinalizerFactory_ApplyVelocity::GetFinalizerType() const
{
  return plGetStaticRTTI<plParticleFinalizer_ApplyVelocity>();
}

void plParticleFinalizerFactory_ApplyVelocity::CopyFinalizerProperties(plParticleFinalizer* pObject, bool bFirstTime) const
{
  plParticleFinalizer_ApplyVelocity* pFinalizer = static_cast<plParticleFinalizer_ApplyVelocity*>(pObject);
}

plParticleFinalizer_ApplyVelocity::plParticleFinalizer_ApplyVelocity()
{
  // a bit later than the other finalizers
  m_fPriority = 525.0f;
}

plParticleFinalizer_ApplyVelocity::~plParticleFinalizer_ApplyVelocity() = default;

void plParticleFinalizer_ApplyVelocity::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", plProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void plParticleFinalizer_ApplyVelocity::Process(plUInt64 uiNumElements)
{
  PL_PROFILE_SCOPE("PFX: ApplyVelocity");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  plProcessingStreamIterator<plVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  plProcessingStreamIterator<plVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    plVec3& pos = reinterpret_cast<plVec3&>(itPosition.Current());

    pos += itVelocity.Current() * tDiff;

    itPosition.Advance();
    itVelocity.Advance();
  }
}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_ApplyVelocity);

