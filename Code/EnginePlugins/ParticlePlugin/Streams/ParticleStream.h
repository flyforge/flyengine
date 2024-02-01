#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class plParticleStream;
class plParticleSystemInstance;

/// \brief Base class for all particle stream factories
class PL_PARTICLEPLUGIN_DLL plParticleStreamFactory : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory, plReflectedClass);

public:
  plParticleStreamFactory(const char* szStreamName, plProcessingStream::DataType dataType, const plRTTI* pStreamTypeToCreate);

  const plRTTI* GetParticleStreamType() const;
  plProcessingStream::DataType GetStreamDataType() const;
  const char* GetStreamName() const;

  static void GetFullStreamName(const char* szName, plProcessingStream::DataType type, plStringBuilder& out_sResult);

  plParticleStream* CreateParticleStream(plParticleSystemInstance* pOwner) const;

private:
  const char* m_szStreamName = nullptr;
  plProcessingStream::DataType m_DataType = plProcessingStream::DataType::Float;
  const plRTTI* m_pStreamTypeToCreate = nullptr;
};

/// \brief Base class for all particle streams
class PL_PARTICLEPLUGIN_DLL plParticleStream : public plProcessingStreamProcessor
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleStream, plProcessingStreamProcessor);

  friend class plParticleSystemInstance;
  friend class plParticleStreamFactory;

protected:
  plParticleStream();
  virtual void Initialize(plParticleSystemInstance* pOwner) {}
  virtual plResult UpdateStreamBindings() final override;
  virtual void Process(plUInt64 uiNumElements) final override {}

  /// \brief The default implementation initializes all data with zero.
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plProcessingStream* m_pStream;

private:
  plParticleStreamBinding m_StreamBinding;
};
