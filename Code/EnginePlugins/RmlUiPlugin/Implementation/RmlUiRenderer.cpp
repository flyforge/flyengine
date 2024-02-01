#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderer.h>

#include <RendererCore/../../../Data/Plugins/Shaders/RmlUiConstants.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiRenderData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiRenderer, 1, plRTTIDefaultAllocator<plRmlUiRenderer>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRmlUiRenderer::plRmlUiRenderer()
{
  // load the shader
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/RmlUi.plShader");
  }

  // constant buffer storage
  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plRmlUiConstants>();
  }

  // quad index buffer
  {
    plUInt32 indices[] = {0, 1, 2, 0, 2, 3};

    plGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(plUInt32);
    desc.m_uiTotalSize = PL_ARRAY_SIZE(indices) * desc.m_uiStructSize;
    desc.m_BufferType = plGALBufferType::IndexBuffer;

    m_hQuadIndexBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc, plMakeArrayPtr(indices).ToByteArray());
  }

  // Setup the vertex declaration
  {
    plVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic = plGALVertexAttributeSemantic::Position;
    si.m_Format = plGALResourceFormat::XYZFloat;
    si.m_uiOffset = offsetof(plRmlUiInternal::Vertex, m_Position);
    si.m_uiElementSize = sizeof(plRmlUiInternal::Vertex::m_Position);
  }

  {
    plVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic = plGALVertexAttributeSemantic::TexCoord0;
    si.m_Format = plGALResourceFormat::UVFloat;
    si.m_uiOffset = offsetof(plRmlUiInternal::Vertex, m_TexCoord);
    si.m_uiElementSize = sizeof(plRmlUiInternal::Vertex::m_TexCoord);
  }

  {
    plVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic = plGALVertexAttributeSemantic::Color0;
    si.m_Format = plGALResourceFormat::RGBAUByteNormalized;
    si.m_uiOffset = offsetof(plRmlUiInternal::Vertex, m_Color);
    si.m_uiElementSize = sizeof(plRmlUiInternal::Vertex::m_Color);
  }
}

plRmlUiRenderer::~plRmlUiRenderer()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();

  plGALDevice::GetDefaultDevice()->DestroyBuffer(m_hQuadIndexBuffer);
  m_hQuadIndexBuffer.Invalidate();
}

void plRmlUiRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plRmlUiRenderData>());
}

void plRmlUiRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(plDefaultRenderDataCategories::GUI);
}

void plRmlUiRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  plRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

  pRenderContext->BindShader(m_hShader);
  pRenderContext->BindConstantBuffer("plRmlUiConstants", m_hConstantBuffer);

  // reset cached state
  m_mLastTransform = plMat4::MakeIdentity();
  m_LastRect = plRectFloat(0, 0);

  for (auto it = batch.GetIterator<plRmlUiRenderData>(); it.IsValid(); ++it)
  {
    const plRmlUiRenderData* pRenderData = it;

    const plUInt32 numBatches = pRenderData->m_Batches.GetCount();
    for (plUInt32 batchIdx = 0; batchIdx < numBatches; ++batchIdx)
    {
      const plRmlUiInternal::Batch& rmlUiBatch = pRenderData->m_Batches[batchIdx];

      plRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<plRmlUiConstants>(m_hConstantBuffer);
      pConstants->UiTransform = rmlUiBatch.m_Transform;
      pConstants->UiTranslation = rmlUiBatch.m_Translation.GetAsVec4(0, 1);

      SetScissorRect(renderViewContext, rmlUiBatch.m_ScissorRect, rmlUiBatch.m_bEnableScissorRect, rmlUiBatch.m_bTransformScissorRect);

      if (rmlUiBatch.m_bTransformScissorRect)
      {
        if (m_mLastTransform != rmlUiBatch.m_Transform || m_LastRect != rmlUiBatch.m_ScissorRect)
        {
          m_mLastTransform = rmlUiBatch.m_Transform;
          m_LastRect = rmlUiBatch.m_ScissorRect;

          PrepareStencil(renderViewContext, rmlUiBatch.m_ScissorRect);
        }

        pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_STENCIL_TEST");
      }
      else
      {
        pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_NORMAL");
      }

      pRenderContext->BindMeshBuffer(rmlUiBatch.m_CompiledGeometry.m_hVertexBuffer, rmlUiBatch.m_CompiledGeometry.m_hIndexBuffer, &m_VertexDeclarationInfo, plGALPrimitiveTopology::Triangles, rmlUiBatch.m_CompiledGeometry.m_uiTriangleCount);

      pRenderContext->BindTexture2D("BaseTexture", rmlUiBatch.m_CompiledGeometry.m_hTexture);

      pRenderContext->DrawMeshBuffer().IgnoreResult();
    }
  }
}

void plRmlUiRenderer::SetScissorRect(const plRenderViewContext& renderViewContext, const plRectFloat& rect, bool bEnable, bool bTransformRect) const
{
  plRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  plGALRenderCommandEncoder* pGALCommandEncoder = pRenderContext->GetRenderCommandEncoder();

  plRectFloat scissorRect = rect;
  if (!bEnable || bTransformRect)
  {
    scissorRect = renderViewContext.m_pViewData->m_ViewPortRect;
  }

  plUInt32 x = static_cast<plUInt32>(plMath::Max(scissorRect.x, 0.0f));
  plUInt32 y = static_cast<plUInt32>(plMath::Max(scissorRect.y, 0.0f));
  plUInt32 width = static_cast<plUInt32>(plMath::Max(scissorRect.width, 0.0f));
  plUInt32 height = static_cast<plUInt32>(plMath::Max(scissorRect.height, 0.0f));

  pGALCommandEncoder->SetScissorRect(plRectU32(x, y, width, height));
}

void plRmlUiRenderer::PrepareStencil(const plRenderViewContext& renderViewContext, const plRectFloat& rect) const
{
  plRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  plGALRenderCommandEncoder* pGALCommandEncoder = pRenderContext->GetRenderCommandEncoder();

  // Clear stencil
  pGALCommandEncoder->Clear(plColor::Black, 0, false, true, 1.0f, 0);

  // Draw quad to set stencil pixels
  pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_STENCIL_SET");

  plRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<plRmlUiConstants>(m_hConstantBuffer);
  pConstants->QuadVertexPos[0] = plVec4(rect.x, rect.y, 0, 1);
  pConstants->QuadVertexPos[1] = plVec4(rect.x + rect.width, rect.y, 0, 1);
  pConstants->QuadVertexPos[2] = plVec4(rect.x + rect.width, rect.y + rect.height, 0, 1);
  pConstants->QuadVertexPos[3] = plVec4(rect.x, rect.y + rect.height, 0, 1);

  pRenderContext->BindMeshBuffer(plGALBufferHandle(), m_hQuadIndexBuffer, nullptr, plGALPrimitiveTopology::Triangles, 2);
  pRenderContext->DrawMeshBuffer().IgnoreResult();
}
