#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleTypePointFactory final : public plParticleTypeFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleTypePointFactory, plParticleTypeFactory);

public:
  virtual const plRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleTypePoint final : public plParticleType
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleTypePoint, plParticleType);

public:
  plParticleTypePoint() {}

  virtual void CreateRequiredStreams() override;

  virtual void ExtractTypeRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const override;

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return 0.0f; }

protected:
  virtual void Process(plUInt64 uiNumElements) override {}

  plProcessingStream* m_pStreamPosition;
  plProcessingStream* m_pStreamColor;

  mutable plArrayPtr<plBaseParticleShaderData> m_BaseParticleData;
  mutable plArrayPtr<plBillboardQuadParticleShaderData> m_BillboardParticleData;
};
