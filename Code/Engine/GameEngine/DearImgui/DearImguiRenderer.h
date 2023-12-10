#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/ResourceManager/ResourceHandle.h>
#  include <Foundation/Math/Rect.h>
#  include <GameEngine/GameEngineDLL.h>
#  include <Imgui/imgui.h>
#  include <RendererCore/Meshes/MeshBufferResource.h>
#  include <RendererCore/Pipeline/Extractor.h>
#  include <RendererCore/Pipeline/RenderData.h>
#  include <RendererCore/Pipeline/Renderer.h>

class plRenderDataBatch;
using plShaderResourceHandle = plTypedResourceHandle<class plShaderResource>;

struct alignas(16) plImguiVertex
{
  PLASMA_DECLARE_POD_TYPE();

  plVec3 m_Position;
  plVec2 m_TexCoord;
  plColorLinearUB m_Color;
};

struct plImguiBatch
{
  PLASMA_DECLARE_POD_TYPE();

  plRectU32 m_ScissorRect;
  plUInt16 m_uiTextureID;
  plUInt16 m_uiVertexCount;
};

class PLASMA_GAMEENGINE_DLL plImguiRenderData : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plImguiRenderData, plRenderData);

public:
  plArrayPtr<plImguiVertex> m_Vertices;
  plArrayPtr<ImDrawIdx> m_Indices;
  plArrayPtr<plImguiBatch> m_Batches;
};

class PLASMA_GAMEENGINE_DLL plImguiExtractor : public plExtractor
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plImguiExtractor, plExtractor);

public:
  plImguiExtractor(const char* szName = "ImguiExtractor");

  virtual void Extract(
    const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& extractedRenderData) override;
};

class PLASMA_GAMEENGINE_DLL plImguiRenderer : public plRenderer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plImguiRenderer, plRenderer);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plImguiRenderer);

public:
  plImguiRenderer();
  ~plImguiRenderer();

  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& types) const override;
  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& categories) const override;
  virtual void RenderBatch(
    const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

protected:
  void SetupRenderer();

  static const plUInt32 s_uiVertexBufferSize = 10000;
  static const plUInt32 s_uiIndexBufferSize = s_uiVertexBufferSize * 2;

  plShaderResourceHandle m_hShader;
  plGALBufferHandle m_hVertexBuffer;
  plGALBufferHandle m_hIndexBuffer;
  plVertexDeclarationInfo m_VertexDeclarationInfo;
};

#endif
