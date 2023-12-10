#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using plMeshResourceHandle = plTypedResourceHandle<class plMeshResource>;
using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;

class PLASMA_PARTICLEPLUGIN_DLL plParticleTypeMeshFactory final : public plParticleTypeFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleTypeMeshFactory, plParticleTypeFactory);

public:
  virtual const plRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  plString m_sMesh;
  plString m_sMaterial;
  plString m_sTintColorParameter;
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleTypeMesh final : public plParticleType
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleTypeMesh, plParticleType);

public:
  plParticleTypeMesh();
  ~plParticleTypeMesh();

  virtual void CreateRequiredStreams() override;

  plMeshResourceHandle m_hMesh;
  mutable plMaterialResourceHandle m_hMaterial;
  plTempHashedString m_sTintColorParameter;

  virtual void ExtractTypeRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const override;

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
  virtual void Process(plUInt64 uiNumElements) override {}

  bool QueryMeshAndMaterialInfo() const;

  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamSize = nullptr;
  plProcessingStream* m_pStreamColor = nullptr;
  plProcessingStream* m_pStreamRotationSpeed = nullptr;
  plProcessingStream* m_pStreamRotationOffset = nullptr;
  plProcessingStream* m_pStreamAxis = nullptr;

  mutable bool m_bRenderDataCached = false;
  mutable plBoundingBoxSphere m_Bounds;
  mutable plRenderData::Category m_RenderCategory;
};
