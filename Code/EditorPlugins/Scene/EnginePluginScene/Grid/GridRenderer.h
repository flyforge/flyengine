#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>

class plRenderDataBatch;
class plSceneContext;

using plShaderResourceHandle = plTypedResourceHandle<class plShaderResource>;

class plGridRenderData : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plGridRenderData, plRenderData);

public:
  float m_fDensity;
  plInt32 m_iFirstLine1;
  plInt32 m_iLastLine1;
  plInt32 m_iFirstLine2;
  plInt32 m_iLastLine2;
  bool m_bOrthoMode;
  bool m_bGlobal;
};

class plEditorGridExtractor : public plExtractor
{
  PL_ADD_DYNAMIC_REFLECTION(plEditorGridExtractor, plExtractor);

public:
  plEditorGridExtractor(const char* szName = "EditorGridExtractor");

  virtual void Extract(const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  void SetSceneContext(plSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  plSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  plSceneContext* m_pSceneContext;
};

struct alignas(16) GridVertex
{
  plVec3 m_position;
  plColorLinearUB m_color;
};

class plGridRenderer : public plRenderer
{
  PL_ADD_DYNAMIC_REFLECTION(plGridRenderer, plRenderer);
  PL_DISALLOW_COPY_AND_ASSIGN(plGridRenderer);

public:
  plGridRenderer();

  // plRenderer implementation
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

protected:
  void CreateVertexBuffer();

  static constexpr plUInt32 s_uiBufferSize = 1024 * 8;
  static constexpr plUInt32 s_uiLineVerticesPerBatch = s_uiBufferSize / sizeof(GridVertex);

  plShaderResourceHandle m_hShader;
  plGALBufferHandle m_hVertexBuffer;
  plVertexDeclarationInfo m_VertexDeclarationInfo;
  mutable plDynamicArray<GridVertex, plAlignedAllocatorWrapper> m_Vertices;

private:
  void CreateGrid(const plGridRenderData& rd) const;
};
