#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using plCurve1DResourceHandle = plTypedResourceHandle<class plCurve1DResource>;

class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializerFactory_RandomSize final : public plParticleInitializerFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializerFactory_RandomSize, plParticleInitializerFactory);

public:
  virtual const plRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(plParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  void SetSizeCurveFile(const char* szFile);
  const char* GetSizeCurveFile() const;

  plVarianceTypeFloat m_Size;
  plCurve1DResourceHandle m_hCurve;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializer_RandomSize final : public plParticleInitializer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializer_RandomSize, plParticleInitializer);

public:
  plVarianceTypeFloat m_Size;
  plCurve1DResourceHandle m_hCurve;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamSize;
};
