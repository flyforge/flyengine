#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStream, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleStreamFactory::plParticleStreamFactory(const char* szStreamName, plProcessingStream::DataType dataType, const plRTTI* pStreamTypeToCreate)
{
  m_szStreamName = szStreamName;
  m_DataType = dataType;
  m_pStreamTypeToCreate = pStreamTypeToCreate;
}

const plRTTI* plParticleStreamFactory::GetParticleStreamType() const
{
  return m_pStreamTypeToCreate;
}

plProcessingStream::DataType plParticleStreamFactory::GetStreamDataType() const
{
  return m_DataType;
}

const char* plParticleStreamFactory::GetStreamName() const
{
  return m_szStreamName;
}

void plParticleStreamFactory::GetFullStreamName(const char* szName, plProcessingStream::DataType type, plStringBuilder& out_sResult)
{
  out_sResult = szName;
  out_sResult.AppendFormat("({0})", (int)type);
}

plParticleStream* plParticleStreamFactory::CreateParticleStream(plParticleSystemInstance* pOwner) const
{
  const plRTTI* pRtti = GetParticleStreamType();
  PL_ASSERT_DEBUG(pRtti->IsDerivedFrom<plParticleStream>(), "Particle stream factory does not create a valid stream type");

  plParticleStream* pStream = pRtti->GetAllocator()->Allocate<plParticleStream>();

  pOwner->CreateStream(GetStreamName(), GetStreamDataType(), &pStream->m_pStream, pStream->m_StreamBinding, true);
  pStream->Initialize(pOwner);

  return pStream;
}

//////////////////////////////////////////////////////////////////////////

plParticleStream::plParticleStream()
{
  // make sure default stream initializers are run very first
  m_fPriority = -1000.0f;
}

plResult plParticleStream::UpdateStreamBindings()
{
  m_StreamBinding.UpdateBindings(m_pStreamGroup);
  return PL_SUCCESS;
}

void plParticleStream::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  const plUInt64 uiElementSize = m_pStream->GetElementSize();
  const plUInt64 uiElementStride = m_pStream->GetElementStride();

  for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    plMemoryUtils::ZeroFill<plUInt8>(
      static_cast<plUInt8*>(plMemoryUtils::AddByteOffset(m_pStream->GetWritableData(), static_cast<ptrdiff_t>(i * uiElementStride))),
      static_cast<size_t>(uiElementSize));
  }
}



PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_ParticleStream);
