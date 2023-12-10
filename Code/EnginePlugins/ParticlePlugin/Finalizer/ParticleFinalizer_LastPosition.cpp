#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleFinalizerFactory_LastPosition, 1, plRTTIDefaultAllocator<plParticleFinalizerFactory_LastPosition>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleFinalizer_LastPosition, 1, plRTTIDefaultAllocator<plParticleFinalizer_LastPosition>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleFinalizerFactory_LastPosition::plParticleFinalizerFactory_LastPosition() {}

const plRTTI* plParticleFinalizerFactory_LastPosition::GetFinalizerType() const
{
  return plGetStaticRTTI<plParticleFinalizer_LastPosition>();
}

void plParticleFinalizerFactory_LastPosition::CopyFinalizerProperties(plParticleFinalizer* pObject, bool bFirstTime) const
{
  plParticleFinalizer_LastPosition* pFinalizer = static_cast<plParticleFinalizer_LastPosition*>(pObject);
}

//////////////////////////////////////////////////////////////////////////

plParticleFinalizer_LastPosition::plParticleFinalizer_LastPosition()
{
  // do this at the start of the frame, but after the initializers
  m_fPriority = -499.0f;
}

plParticleFinalizer_LastPosition::~plParticleFinalizer_LastPosition() = default;

void plParticleFinalizer_LastPosition::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("LastPosition", plProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
}

void plParticleFinalizer_LastPosition::Process(plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: LastPosition");

  plProcessingStreamIterator<plVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  plProcessingStreamIterator<plVec3> itLastPosition(m_pStreamLastPosition, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    plVec3 curPos = itPosition.Current().GetAsVec3();
    plVec3& lastPos = itLastPosition.Current();

    lastPos = curPos;

    itPosition.Advance();
    itLastPosition.Advance();
  }
}
