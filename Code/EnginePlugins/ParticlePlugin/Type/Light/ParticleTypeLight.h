#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class plView;
class plExtractedRenderData;

class PLASMA_PARTICLEPLUGIN_DLL plParticleTypeLightFactory final : public plParticleTypeFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleTypeLightFactory, plParticleTypeFactory);

public:
  plParticleTypeLightFactory();

  virtual const plRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  float m_fSizeFactor;
  float m_fIntensity;
  plUInt32 m_uiPercentage;
  plString m_sTintColorParameter;
  plString m_sIntensityParameter;
  plString m_sSizeScaleParameter;
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleTypeLight final : public plParticleType
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleTypeLight, plParticleType);

public:
  virtual void CreateRequiredStreams() override;

  float m_fSizeFactor;
  float m_fIntensity;
  plUInt32 m_uiPercentage;
  plTempHashedString m_sTintColorParameter;
  plTempHashedString m_sIntensityParameter;
  plTempHashedString m_sSizeScaleParameter;

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return 0.5f * fParticleSize * m_fSizeFactor; }

  virtual void ExtractTypeRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const override;

protected:
  virtual void Process(plUInt64 uiNumElements) override {}

  plProcessingStream* m_pStreamPosition;
  plProcessingStream* m_pStreamSize;
  plProcessingStream* m_pStreamColor;
  plProcessingStream* m_pStreamOnOff;
};
