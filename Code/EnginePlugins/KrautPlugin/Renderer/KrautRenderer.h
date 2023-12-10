#pragma once

#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

#include <RendererCore/../../../Data/Plugins/Kraut/TreeShaderData.h>

struct plPerInstanceData;

/// \brief Implements rendering of static meshes
class PLASMA_KRAUTPLUGIN_DLL plKrautRenderer : public plRenderer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plKrautRenderer, plRenderer);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plKrautRenderer);

public:
  plKrautRenderer();
  ~plKrautRenderer();

  // plRenderer implementation
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

protected:
  virtual void FillPerInstanceData(const plVec3& vLodCamPos, plArrayPtr<plPerInstanceData> instanceData, const plRenderDataBatch& batch, bool bIsShadowView, plUInt32 uiStartIndex, plUInt32& out_uiFilteredCount) const;

  struct TempTreeCB
  {
    TempTreeCB(plRenderContext* pRenderContext);
    ~TempTreeCB();

    void SetTreeData(const plVec3& vTreeCenter, float fLeafShadowOffset);

    plConstantBufferStorage<plKrautTreeConstants>* m_pConstants;
    plConstantBufferStorageHandle m_hConstantBuffer;
  };
};
