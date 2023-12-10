#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Volume.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleFinalizerFactory_Volume, 1, plRTTIDefaultAllocator<plParticleFinalizerFactory_Volume>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleFinalizer_Volume, 1, plRTTIDefaultAllocator<plParticleFinalizer_Volume>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleFinalizerFactory_Volume::plParticleFinalizerFactory_Volume() = default;
plParticleFinalizerFactory_Volume::~plParticleFinalizerFactory_Volume() = default;

const plRTTI* plParticleFinalizerFactory_Volume::GetFinalizerType() const
{
  return plGetStaticRTTI<plParticleFinalizer_Volume>();
}

void plParticleFinalizerFactory_Volume::CopyFinalizerProperties(plParticleFinalizer* pObject, bool bFirstTime) const
{
  plParticleFinalizer_Volume* pFinalizer = static_cast<plParticleFinalizer_Volume*>(pObject);
}

plParticleFinalizer_Volume::plParticleFinalizer_Volume() = default;
plParticleFinalizer_Volume::~plParticleFinalizer_Volume() = default;

void plParticleFinalizer_Volume::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  m_pStreamSize = nullptr;
}

void plParticleFinalizer_Volume::QueryOptionalStreams()
{
  m_pStreamSize = GetOwnerSystem()->QueryStream("Size", plProcessingStream::DataType::Half);
}

void plParticleFinalizer_Volume::Process(plUInt64 uiNumElements)
{
  if (uiNumElements == 0)
    return;

  PLASMA_PROFILE_SCOPE("PFX: Volume");

  const plSimdVec4f* pPosition = m_pStreamPosition->GetData<plSimdVec4f>();

  plSimdBBoxSphere volume;
  volume.SetFromPoints(pPosition, static_cast<plUInt32>(uiNumElements));

  float fMaxSize = 0;

  if (m_pStreamSize != nullptr)
  {
    const plFloat16* pSize = m_pStreamSize->GetData<plFloat16>();

    plSimdVec4f vMax;
    vMax.SetZero();

    constexpr plUInt32 uiElementsPerLoop = 4;
    for (plUInt64 i = 0; i < uiNumElements; i += uiElementsPerLoop)
    {
      const float x = pSize[i + 0];
      const float y = pSize[i + 1];
      const float z = pSize[i + 2];
      const float w = pSize[i + 3];

      vMax = vMax.CompMax(plSimdVec4f(x, y, z, w));
    }

    for (plUInt64 i = (uiNumElements / uiElementsPerLoop) * uiElementsPerLoop; i < uiNumElements; ++i)
    {
      fMaxSize = plMath::Max(fMaxSize, (float)pSize[i]);
    }

    fMaxSize = plMath::Max(fMaxSize, (float)vMax.HorizontalMax<4>());
  }

  GetOwnerSystem()->SetBoundingVolume(plSimdConversion::ToBBoxSphere(volume), fMaxSize);
}
