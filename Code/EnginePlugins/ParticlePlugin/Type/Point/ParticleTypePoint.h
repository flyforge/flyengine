#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class PL_PARTICLEPLUGIN_DLL plParticleTypePointFactory final : public plParticleTypeFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleTypePointFactory, plParticleTypeFactory);

public:
  virtual const plRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;
};

class PL_PARTICLEPLUGIN_DLL plParticleTypePoint final : public plParticleType
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleTypePoint, plParticleType);

public:
  plParticleTypePoint() = default;

  virtual void CreateRequiredStreams() override;

  virtual void ExtractTypeRenderData(plMsgExtractRenderData& ref_msg, const plTransform& instanceTransform) const override;

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return 0.0f; }

protected:
  virtual void Process(plUInt64 uiNumElements) override {}

  plProcessingStream* m_pStreamPosition;
  plProcessingStream* m_pStreamColor;

  mutable plArrayPtr<plBaseParticleShaderData> m_BaseParticleData;
  mutable plArrayPtr<plBillboardQuadParticleShaderData> m_BillboardParticleData;
};
