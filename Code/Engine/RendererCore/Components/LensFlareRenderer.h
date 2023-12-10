#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>

struct plPerLensFlareData;
class plRenderDataBatch;
using plShaderResourceHandle = plTypedResourceHandle<class plShaderResource>;

/// \brief Implements rendering of lens flares
class PLASMA_RENDERERCORE_DLL plLensFlareRenderer : public plRenderer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLensFlareRenderer, plRenderer);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plLensFlareRenderer);

public:
  plLensFlareRenderer();
  ~plLensFlareRenderer();

  // plRenderer implementation
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(
    const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

protected:
  plGALBufferHandle CreateLensFlareDataBuffer(plUInt32 uiBufferSize) const;
  void DeleteLensFlareDataBuffer(plGALBufferHandle hBuffer) const;
  virtual void FillLensFlareData(const plRenderDataBatch& batch) const;

  plShaderResourceHandle m_hShader;
  mutable plDynamicArray<plPerLensFlareData, plAlignedAllocatorWrapper> m_LensFlareData;
};
