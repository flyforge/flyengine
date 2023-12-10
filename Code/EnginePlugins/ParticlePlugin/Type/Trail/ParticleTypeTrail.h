#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;
struct plTrailParticleData;

class PLASMA_PARTICLEPLUGIN_DLL plParticleTypeTrailFactory final : public plParticleTypeFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleTypeTrailFactory, plParticleTypeFactory);

public:
  virtual const plRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  plEnum<plParticleTypeRenderMode> m_RenderMode;
  plUInt16 m_uiMaxPoints;
  plTime m_UpdateDiff;
  plString m_sTexture;
  plEnum<plParticleTextureAtlasType> m_TextureAtlasType;
  plUInt8 m_uiNumSpritesX = 1;
  plUInt8 m_uiNumSpritesY = 1;
  plString m_sTintColorParameter;
  plString m_sDistortionTexture;
  float m_fDistortionStrength = 0;
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleTypeTrail final : public plParticleType
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleTypeTrail, plParticleType);

public:
  plParticleTypeTrail();
  ~plParticleTypeTrail();

  plEnum<plParticleTypeRenderMode> m_RenderMode;
  plUInt16 m_uiMaxPoints;
  plTime m_UpdateDiff;
  plTexture2DResourceHandle m_hTexture;
  plEnum<plParticleTextureAtlasType> m_TextureAtlasType;
  plUInt8 m_uiNumSpritesX = 1;
  plUInt8 m_uiNumSpritesY = 1;
  plTempHashedString m_sTintColorParameter;
  plTexture2DResourceHandle m_hDistortionTexture;
  float m_fDistortionStrength = 0;

  virtual void CreateRequiredStreams() override;
  virtual void ExtractTypeRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const override;
  /// \todo This is a hacky guess, one would actually need to inspect the trail positions
  virtual float GetMaxParticleRadius(float fParticleSize) const override { return fParticleSize + m_uiMaxPoints * 0.05f; }

  static plUInt16 ComputeTrailPointBucketSize(plUInt16 uiMaxTrailPoints);

protected:
  friend class plParticleTypeTrailFactory;

  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
  virtual void Process(plUInt64 uiNumElements) override;
  void OnParticleDeath(const plStreamGroupElementRemovedEvent& e);

  plProcessingStream* m_pStreamLifeTime = nullptr;
  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamSize = nullptr;
  plProcessingStream* m_pStreamColor = nullptr;
  plProcessingStream* m_pStreamTrailData = nullptr;
  plProcessingStream* m_pStreamVariation = nullptr;
  plTime m_LastSnapshot;
  plUInt8 m_uiCurFirstIndex = 0;
  float m_fSnapshotFraction;

  mutable plArrayPtr<plBaseParticleShaderData> m_BaseParticleData;
  mutable plArrayPtr<plTrailParticleShaderData> m_TrailParticleData;
  mutable plArrayPtr<plVec4> m_TrailPointsShared;

  struct TrailData
  {
    plUInt16 m_uiNumPoints;
    plUInt16 m_uiIndexForTrailPoints;
  };

  plUInt16 GetIndexForTrailPoints();
  const plVec4* GetTrailPointsPositions(plUInt32 index) const;
  plVec4* GetTrailPointsPositions(plUInt32 index);

  /// \todo Use a shared freelist across effects instead
  // plDynamicArray<plTrailParticlePointsData8> m_TrailPoints8;
  // plDynamicArray<plTrailParticlePointsData16> m_TrailPoints16;
  // plDynamicArray<plTrailParticlePointsData32> m_TrailPoints32;
  plDynamicArray<plTrailParticlePointsData64, plAlignedAllocatorWrapper> m_TrailPoints64;
  plDynamicArray<plUInt16> m_FreeTrailData;
};
