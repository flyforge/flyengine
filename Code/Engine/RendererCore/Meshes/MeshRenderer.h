#pragma once

#include <RendererCore/Pipeline/Renderer.h>

class plMeshRenderData;
struct plPerInstanceData;

/// \brief Implements rendering of static meshes
class PLASMA_RENDERERCORE_DLL plMeshRenderer : public plRenderer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMeshRenderer, plRenderer);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plMeshRenderer);

public:
  plMeshRenderer();
  ~plMeshRenderer();

  // plRenderer implementation
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(
    const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

protected:
  virtual void SetAdditionalData(const plRenderViewContext& renderViewContext, const plMeshRenderData* pRenderData) const;
  virtual void FillPerInstanceData(
    plArrayPtr<plPerInstanceData> instanceData, const plRenderDataBatch& batch, plUInt32 uiStartIndex, plUInt32& out_uiFilteredCount) const;
};
