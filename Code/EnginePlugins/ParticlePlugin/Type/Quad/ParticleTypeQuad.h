#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Quad/QuadParticleRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;

struct PL_PARTICLEPLUGIN_DLL plQuadParticleOrientation
{
  using StorageType = plUInt8;

  enum Enum
  {
    Billboard,

    Rotating_OrthoEmitterDir,
    Rotating_EmitterDir,

    Fixed_EmitterDir,
    Fixed_WorldUp,
    Fixed_RandomDir,

    FixedAxis_EmitterDir,
    FixedAxis_ParticleDir,

    Default = Billboard
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_PARTICLEPLUGIN_DLL, plQuadParticleOrientation);

class PL_PARTICLEPLUGIN_DLL plParticleTypeQuadFactory final : public plParticleTypeFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleTypeQuadFactory, plParticleTypeFactory);

public:
  virtual const plRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_finalizerDeps) const override;

  plEnum<plQuadParticleOrientation> m_Orientation;
  plAngle m_MaxDeviation;
  plEnum<plParticleTypeRenderMode> m_RenderMode;
  plString m_sTexture;
  plEnum<plParticleTextureAtlasType> m_TextureAtlasType;
  plUInt8 m_uiNumSpritesX = 1;
  plUInt8 m_uiNumSpritesY = 1;
  plString m_sTintColorParameter;
  plString m_sDistortionTexture;
  float m_fDistortionStrength = 0;
  float m_fStretch = 1;
};

class PL_PARTICLEPLUGIN_DLL plParticleTypeQuad final : public plParticleType
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleTypeQuad, plParticleType);

public:
  plParticleTypeQuad();
  ~plParticleTypeQuad();

  virtual void CreateRequiredStreams() override;

  plEnum<plQuadParticleOrientation> m_Orientation;
  plAngle m_MaxDeviation;
  plEnum<plParticleTypeRenderMode> m_RenderMode;
  plTexture2DResourceHandle m_hTexture;
  plEnum<plParticleTextureAtlasType> m_TextureAtlasType;
  plUInt8 m_uiNumSpritesX = 1;
  plUInt8 m_uiNumSpritesY = 1;
  plTempHashedString m_sTintColorParameter;
  plTexture2DResourceHandle m_hDistortionTexture;
  float m_fDistortionStrength = 0;
  float m_fStretch = 1;

  virtual void ExtractTypeRenderData(plMsgExtractRenderData& ref_msg, const plTransform& instanceTransform) const override;

  struct sod
  {
    PL_DECLARE_POD_TYPE();

    float dist;
    plUInt32 index;
  };


protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
  virtual void Process(plUInt64 uiNumElements) override {}
  void AllocateParticleData(const plUInt32 numParticles, const bool bNeedsBillboardData, const bool bNeedsTangentData) const;
  void AddParticleRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const;
  void CreateExtractedData(const plHybridArray<sod, 64>* pSorted) const;

  plProcessingStream* m_pStreamLifeTime = nullptr;
  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamSize = nullptr;
  plProcessingStream* m_pStreamColor = nullptr;
  plProcessingStream* m_pStreamRotationSpeed = nullptr;
  plProcessingStream* m_pStreamRotationOffset = nullptr;
  plProcessingStream* m_pStreamAxis = nullptr;
  plProcessingStream* m_pStreamVariation = nullptr;
  plProcessingStream* m_pStreamLastPosition = nullptr;

  mutable plArrayPtr<plBaseParticleShaderData> m_BaseParticleData;
  mutable plArrayPtr<plBillboardQuadParticleShaderData> m_BillboardParticleData;
  mutable plArrayPtr<plTangentQuadParticleShaderData> m_TangentParticleData;
};
