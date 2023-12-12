#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>

struct SpriteData;
class plRenderDataBatch;
using plShaderResourceHandle = plTypedResourceHandle<class plShaderResource>;

/// \brief Implements rendering of sprites
class PLASMA_RENDERERCORE_DLL plSpriteRenderer : public plRenderer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSpriteRenderer, plRenderer);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plSpriteRenderer);

public:
  plSpriteRenderer();
  ~plSpriteRenderer();

  // plRenderer implementation
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(
    const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

protected:
  plGALBufferHandle CreateSpriteDataBuffer() const;
  void DeleteSpriteDataBuffer(plGALBufferHandle hBuffer) const;
  virtual void FillSpriteData(const plRenderDataBatch& batch, plUInt32 uiStartIndex, plUInt32 uiCount) const;

  plShaderResourceHandle m_hShader;
  mutable plDynamicArray<SpriteData, plAlignedAllocatorWrapper> m_SpriteData;
};
