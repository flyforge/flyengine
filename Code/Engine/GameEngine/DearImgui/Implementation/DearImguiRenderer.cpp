#include <GameEngine/GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <GameEngine/DearImgui/DearImgui.h>
#  include <GameEngine/DearImgui/DearImguiRenderer.h>
#  include <Imgui/imgui_internal.h>
#  include <RendererCore/Pipeline/ExtractedRenderData.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderContext/RenderContext.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#  include <RendererCore/Shader/ShaderResource.h>
#  include <RendererFoundation/Device/Device.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plImguiRenderData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plImguiExtractor, 1, plRTTIDefaultAllocator<plImguiExtractor>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plImguiRenderer, 1, plRTTIDefaultAllocator<plImguiRenderer>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plImguiExtractor::plImguiExtractor(const char* szName)
  : plExtractor(szName)
{
  m_DependsOn.PushBack(plMakeHashedString("plVisibleObjectsExtractor"));
}

void plImguiExtractor::Extract(const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& extractedRenderData)
{
  plImgui* pImGui = plImgui::GetSingleton();
  if (pImGui == nullptr)
  {
    return;
  }

  {
    PLASMA_LOCK(pImGui->m_ViewToContextTableMutex);
    plImgui::Context context;
    if (!pImGui->m_ViewToContextTable.TryGetValue(view.GetHandle(), context))
    {
      // No context for this view
      return;
    }

    plUInt64 uiCurrentFrameCounter = plRenderWorld::GetFrameCounter();
    if (context.m_uiFrameBeginCounter != uiCurrentFrameCounter)
    {
      // Nothing was rendered with ImGui this frame
      return;
    }

    context.m_uiFrameRenderCounter = uiCurrentFrameCounter;

    ImGui::SetCurrentContext(context.m_pImGuiContext);
  }

  ImGui::Render();

  ImDrawData* pDrawData = ImGui::GetDrawData();

  if (pDrawData && pDrawData->Valid)
  {
    for (int draw = 0; draw < pDrawData->CmdListsCount; ++draw)
    {
      plImguiRenderData* pRenderData = plCreateRenderDataForThisFrame<plImguiRenderData>(nullptr);
      pRenderData->m_uiSortingKey = draw;
      pRenderData->m_GlobalTransform.SetIdentity();
      pRenderData->m_GlobalBounds.SetInvalid();

      // copy the vertex data
      // uses the frame allocator to prevent unnecessary deallocations
      {
        const ImDrawList* pCmdList = pDrawData->CmdLists[draw];

        pRenderData->m_Vertices = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plImguiVertex, pCmdList->VtxBuffer.size());
        for (plUInt32 vtx = 0; vtx < pRenderData->m_Vertices.GetCount(); ++vtx)
        {
          const auto& vert = pCmdList->VtxBuffer[vtx];

          pRenderData->m_Vertices[vtx].m_Position.Set(vert.pos.x, vert.pos.y, 0);
          pRenderData->m_Vertices[vtx].m_TexCoord.Set(vert.uv.x, vert.uv.y);
          pRenderData->m_Vertices[vtx].m_Color = *reinterpret_cast<const plColorGammaUB*>(&vert.col);
        }

        pRenderData->m_Indices = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), ImDrawIdx, pCmdList->IdxBuffer.size());
        for (plUInt32 i = 0; i < pRenderData->m_Indices.GetCount(); ++i)
        {
          pRenderData->m_Indices[i] = pCmdList->IdxBuffer[i];
        }
      }

      // pass along an plImguiBatch for every necessary drawcall
      {
        const ImDrawList* pCommands = pDrawData->CmdLists[draw];

        pRenderData->m_Batches = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plImguiBatch, pCommands->CmdBuffer.Size);

        for (int cmdIdx = 0; cmdIdx < pCommands->CmdBuffer.Size; cmdIdx++)
        {
          const ImDrawCmd* pCmd = &pCommands->CmdBuffer[cmdIdx];
          const size_t iTextureID = reinterpret_cast<size_t>(pCmd->TextureId);

          plImguiBatch& batch = pRenderData->m_Batches[cmdIdx];
          batch.m_uiVertexCount = static_cast<plUInt16>(pCmd->ElemCount);
          batch.m_uiTextureID = (plUInt16)iTextureID;
          batch.m_ScissorRect = plRectU32((plUInt32)pCmd->ClipRect.x, (plUInt32)pCmd->ClipRect.y, (plUInt32)(pCmd->ClipRect.z - pCmd->ClipRect.x), (plUInt32)(pCmd->ClipRect.w - pCmd->ClipRect.y));
        }
      }

      extractedRenderData.AddRenderData(pRenderData, plDefaultRenderDataCategories::GUI);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

plImguiRenderer::plImguiRenderer()
{
  SetupRenderer();
}

plImguiRenderer::~plImguiRenderer()
{
  m_hShader.Invalidate();

  if (!m_hVertexBuffer.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexBuffer);
    m_hVertexBuffer.Invalidate();
  }

  if (!m_hIndexBuffer.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);
    m_hIndexBuffer.Invalidate();
  }
}

void plImguiRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& types) const
{
  types.PushBack(plGetStaticRTTI<plImguiRenderData>());
}

void plImguiRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& categories) const
{
  categories.PushBack(plDefaultRenderDataCategories::GUI);
}

void plImguiRenderer::RenderBatch(const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  if (plImgui::GetSingleton() == nullptr)
    return;

  plRenderContext* pRenderContext = renderContext.m_pRenderContext;
  plGALRenderCommandEncoder* pCommandEncoder = pRenderContext->GetRenderCommandEncoder();

  pRenderContext->BindShader(m_hShader);
  const auto& textures = plImgui::GetSingleton()->m_Textures;
  const plUInt32 numTextures = textures.GetCount();

  for (auto it = batch.GetIterator<plImguiRenderData>(); it.IsValid(); ++it)
  {
    const plImguiRenderData* pRenderData = it;

    PLASMA_ASSERT_DEV(pRenderData->m_Vertices.GetCount() < s_uiVertexBufferSize, "GUI has too many elements to render in one drawcall");
    PLASMA_ASSERT_DEV(pRenderData->m_Indices.GetCount() < s_uiIndexBufferSize, "GUI has too many elements to render in one drawcall");

    pCommandEncoder->UpdateBuffer(m_hVertexBuffer, 0, plMakeArrayPtr(pRenderData->m_Vertices.GetPtr(), pRenderData->m_Vertices.GetCount()).ToByteArray());
    pCommandEncoder->UpdateBuffer(m_hIndexBuffer, 0, plMakeArrayPtr(pRenderData->m_Indices.GetPtr(), pRenderData->m_Indices.GetCount()).ToByteArray());

    pRenderContext->BindMeshBuffer(m_hVertexBuffer, m_hIndexBuffer, &m_VertexDeclarationInfo, plGALPrimitiveTopology::Triangles, pRenderData->m_Indices.GetCount() / 3);

    plUInt32 uiFirstIndex = 0;
    const plUInt32 numBatches = pRenderData->m_Batches.GetCount();
    for (plUInt32 batchIdx = 0; batchIdx < numBatches; ++batchIdx)
    {
      const plImguiBatch& imGuiBatch = pRenderData->m_Batches[batchIdx];

      if (imGuiBatch.m_uiVertexCount > 0 && imGuiBatch.m_uiTextureID < numTextures)
      {
        pCommandEncoder->SetScissorRect(imGuiBatch.m_ScissorRect);
        pRenderContext->BindTexture2D("BaseTexture", textures[imGuiBatch.m_uiTextureID]);
        pRenderContext->DrawMeshBuffer(imGuiBatch.m_uiVertexCount / 3, uiFirstIndex / 3).IgnoreResult();
      }

      uiFirstIndex += imGuiBatch.m_uiVertexCount;
    }
  }
}

void plImguiRenderer::SetupRenderer()
{
  if (!m_hVertexBuffer.IsInvalidated())
    return;

  // load the shader
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/GUI/DearImguiPrimitives.plShader");
  }

  // Create the vertex buffer
  {
    plGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(plImguiVertex);
    desc.m_uiTotalSize = s_uiVertexBufferSize * desc.m_uiStructSize;
    desc.m_BufferType = plGALBufferType::VertexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hVertexBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  // Create the index buffer
  {
    plGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ImDrawIdx);
    desc.m_uiTotalSize = s_uiIndexBufferSize * desc.m_uiStructSize;
    desc.m_BufferType = plGALBufferType::IndexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hIndexBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  // Setup the vertex declaration
  {
    {
      plVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::Position;
      si.m_Format = plGALResourceFormat::XYZFloat;
      si.m_uiOffset = 0;
      si.m_uiElementSize = 12;
    }

    {
      plVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::TexCoord0;
      si.m_Format = plGALResourceFormat::UVFloat;
      si.m_uiOffset = 12;
      si.m_uiElementSize = 8;
    }

    {
      plVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::Color0;
      si.m_Format = plGALResourceFormat::RGBAUByteNormalized;
      si.m_uiOffset = 20;
      si.m_uiElementSize = 4;
    }
  }
}

#endif

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_DearImgui_Implementation_DearImguiRenderer);
