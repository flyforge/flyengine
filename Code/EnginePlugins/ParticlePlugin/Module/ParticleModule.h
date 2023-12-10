#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class plProcessingStream;

class PLASMA_PARTICLEPLUGIN_DLL plParticleModule : public plProcessingStreamProcessor
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleModule, plProcessingStreamProcessor);

  friend class plParticleSystemInstance;

public:
  virtual void CreateRequiredStreams() = 0;
  virtual void QueryOptionalStreams() {}

  void Reset(plParticleSystemInstance* pOwner)
  {
    m_pOwnerSystem = pOwner;
    m_StreamBinding.Clear();

    OnReset();
  }

  /// \brief Called after everything is set up.
  virtual void OnFinalize() {}

  plParticleSystemInstance* GetOwnerSystem() { return m_pOwnerSystem; }

  const plParticleSystemInstance* GetOwnerSystem() const { return m_pOwnerSystem; }

  plParticleEffectInstance* GetOwnerEffect() const { return m_pOwnerSystem->GetOwnerEffect(); }

  /// \brief Override this to cache world module pointers for later (through plParticleWorldModule::GetCachedWorldModule()).
  virtual void RequestRequiredWorldModulesForCache(plParticleWorldModule* pParticleModule) {}

protected:
  /// \brief Called by Reset()
  virtual void OnReset() {}

  void CreateStream(const char* szName, plProcessingStream::DataType Type, plProcessingStream** ppStream, bool bWillInitializeStream)
  {
    m_pOwnerSystem->CreateStream(szName, Type, ppStream, m_StreamBinding, bWillInitializeStream);
  }

  virtual plResult UpdateStreamBindings() final override
  {
    m_StreamBinding.UpdateBindings(m_pStreamGroup);
    return PLASMA_SUCCESS;
  }

  plRandom& GetRNG() const { return GetOwnerEffect()->GetRNG(); }

private:
  plParticleSystemInstance* m_pOwnerSystem;
  plParticleStreamBinding m_StreamBinding;
};
