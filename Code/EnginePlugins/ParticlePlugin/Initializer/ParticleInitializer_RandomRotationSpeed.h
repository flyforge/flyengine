#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using plCurve1DResourceHandle = plTypedResourceHandle<class plCurve1DResource>;

class PL_PARTICLEPLUGIN_DLL plParticleInitializerFactory_RandomRotationSpeed final : public plParticleInitializerFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleInitializerFactory_RandomRotationSpeed, plParticleInitializerFactory);

public:
  virtual const plRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(plParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  bool m_bRandomStartAngle = false;
  plVarianceTypeAngle m_RotationSpeed;
};


class PL_PARTICLEPLUGIN_DLL plParticleInitializer_RandomRotationSpeed final : public plParticleInitializer
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleInitializer_RandomRotationSpeed, plParticleInitializer);

public:
  bool m_bRandomStartAngle = false;
  plVarianceTypeAngle m_RotationSpeed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  bool m_bPositiveSign = false;
  plProcessingStream* m_pStreamRotationSpeed = nullptr;
  plProcessingStream* m_pStreamRotationOffset = nullptr;
};
