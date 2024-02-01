#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using plCurve1DResourceHandle = plTypedResourceHandle<class plCurve1DResource>;

class PL_PARTICLEPLUGIN_DLL plParticleInitializerFactory_RandomSize final : public plParticleInitializerFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleInitializerFactory_RandomSize, plParticleInitializerFactory);

public:
  virtual const plRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(plParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  void SetSizeCurveFile(const char* szFile);
  const char* GetSizeCurveFile() const;

  plVarianceTypeFloat m_Size;
  plCurve1DResourceHandle m_hCurve;
};


class PL_PARTICLEPLUGIN_DLL plParticleInitializer_RandomSize final : public plParticleInitializer
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleInitializer_RandomSize, plParticleInitializer);

public:
  plVarianceTypeFloat m_Size;
  plCurve1DResourceHandle m_hCurve;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamSize;
};
