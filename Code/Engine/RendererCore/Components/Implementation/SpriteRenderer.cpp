#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Components/SpriteRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <Shaders/Materials/SpriteData.h>
PL_CHECK_AT_COMPILETIME(sizeof(plPerSpriteData) == 48);

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSpriteRenderer, 1, plRTTIDefaultAllocator<plSpriteRenderer>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plSpriteRenderer::plSpriteRenderer()
{
  m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Materials/SpriteMaterial.plShader");
}

plSpriteRenderer::~plSpriteRenderer() = default;

void plSpriteRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plSpriteRenderData>());
}

void plSpriteRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(plDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(plDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(plDefaultRenderDataCategories::SimpleOpaque);
  ref_categories.PushBack(plDefaultRenderDataCategories::SimpleTransparent);
  ref_categories.PushBack(plDefaultRenderDataCategories::Selection);
}

void plSpriteRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plRenderContext* pContext = renderViewContext.m_pRenderContext;

  const plSpriteRenderData* pRenderData = batch.GetFirstData<plSpriteRenderData>();

  const plUInt32 uiBufferSize = plMath::RoundUp(batch.GetCount(), 128u);
  plGALBufferHandle hSpriteData = CreateSpriteDataBuffer(uiBufferSize);
  PL_SCOPE_EXIT(DeleteSpriteDataBuffer(hSpriteData));

  pContext->BindShader(m_hShader);
  pContext->BindBuffer("spriteData", pDevice->GetDefaultResourceView(hSpriteData));
  pContext->BindTexture2D("SpriteTexture", pRenderData->m_hTexture);

  pContext->SetShaderPermutationVariable("BLEND_MODE", plSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));
  pContext->SetShaderPermutationVariable("SHAPE_ICON", pRenderData->m_BlendMode == plSpriteBlendMode::ShapeIcon ? plMakeHashedString("TRUE") : plMakeHashedString("FALSE"));

  FillSpriteData(batch);

  if (m_SpriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    pContext->GetCommandEncoder()->UpdateBuffer(hSpriteData, 0, m_SpriteData.GetByteArrayPtr());

    pContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, m_SpriteData.GetCount() * 2);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

plGALBufferHandle plSpriteRenderer::CreateSpriteDataBuffer(plUInt32 uiBufferSize) const
{
  plGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(plPerSpriteData);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiBufferSize;
  desc.m_BufferType = plGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer = true;
  desc.m_bAllowShaderResourceView = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  return plGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void plSpriteRenderer::DeleteSpriteDataBuffer(plGALBufferHandle hBuffer) const
{
  plGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void plSpriteRenderer::FillSpriteData(const plRenderDataBatch& batch) const
{
  m_SpriteData.Clear();
  m_SpriteData.Reserve(batch.GetCount());

  for (auto it = batch.GetIterator<plSpriteRenderData>(); it.IsValid(); ++it)
  {
    const plSpriteRenderData* pRenderData = it;

    auto& spriteData = m_SpriteData.ExpandAndGetRef();

    spriteData.WorldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    spriteData.Size = pRenderData->m_fSize;
    spriteData.MaxScreenSize = pRenderData->m_fMaxScreenSize;
    spriteData.AspectRatio = pRenderData->m_fAspectRatio;
    spriteData.ColorRG = plShaderUtils::Float2ToRG16F(plVec2(pRenderData->m_color.r, pRenderData->m_color.g));
    spriteData.ColorBA = plShaderUtils::Float2ToRG16F(plVec2(pRenderData->m_color.b, pRenderData->m_color.a));
    spriteData.TexCoordScale = plShaderUtils::Float2ToRG16F(pRenderData->m_texCoordScale);
    spriteData.TexCoordOffset = plShaderUtils::Float2ToRG16F(pRenderData->m_texCoordOffset);
    spriteData.GameObjectID = pRenderData->m_uiUniqueID;
    spriteData.Reserved = 0;
  }
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteRenderer);
