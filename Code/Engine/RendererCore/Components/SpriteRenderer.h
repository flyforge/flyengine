#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>

struct plPerSpriteData;
class plRenderDataBatch;
using plShaderResourceHandle = plTypedResourceHandle<class plShaderResource>;

/// \brief Implements rendering of sprites
class PL_RENDERERCORE_DLL plSpriteRenderer : public plRenderer
{
  PL_ADD_DYNAMIC_REFLECTION(plSpriteRenderer, plRenderer);
  PL_DISALLOW_COPY_AND_ASSIGN(plSpriteRenderer);

public:
  plSpriteRenderer();
  ~plSpriteRenderer();

  // plRenderer implementation
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(
    const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

protected:
  plGALBufferHandle CreateSpriteDataBuffer(plUInt32 uiBufferSize) const;
  void DeleteSpriteDataBuffer(plGALBufferHandle hBuffer) const;
  virtual void FillSpriteData(const plRenderDataBatch& batch) const;

  plShaderResourceHandle m_hShader;
  mutable plDynamicArray<plPerSpriteData, plAlignedAllocatorWrapper> m_SpriteData;
};
