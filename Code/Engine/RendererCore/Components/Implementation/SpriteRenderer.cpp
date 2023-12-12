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

struct alignas(16) SpriteData
{
  plVec3 m_worldSpacePosition;
  float m_size;
  float m_maxScreenSize;
  float m_aspectRatio;
  plUInt32 m_colorRG;
  plUInt32 m_colorBA;
  plUInt32 m_texCoordScale;
  plUInt32 m_texCoordOffset;
  plUInt32 m_gameObjectID;
  plUInt32 m_reserved;
};

PLASMA_CHECK_AT_COMPILETIME(sizeof(SpriteData) == 48);

namespace
{
  enum
  {
    MAX_SPRITE_DATA_PER_BATCH = 1024
  };
}

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSpriteRenderer, 1, plRTTIDefaultAllocator<plSpriteRenderer>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plSpriteRenderer::plSpriteRenderer()
{
  m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Materials/SpriteMaterial.plShader");
}

plSpriteRenderer::~plSpriteRenderer() {}

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

  plGALBufferHandle hSpriteData = CreateSpriteDataBuffer();
  PLASMA_SCOPE_EXIT(DeleteSpriteDataBuffer(hSpriteData));

  pContext->BindShader(m_hShader);
  pContext->BindBuffer("spriteData", pDevice->GetDefaultResourceView(hSpriteData));
  pContext->BindTexture2D("SpriteTexture", pRenderData->m_hTexture);

  pContext->SetShaderPermutationVariable("BLEND_MODE", plSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));
  pContext->SetShaderPermutationVariable("SHAPE_ICON", pRenderData->m_BlendMode == plSpriteBlendMode::ShapeIcon ? plMakeHashedString("TRUE") : plMakeHashedString("FALSE"));

  plUInt32 uiStartIndex = 0;
  while (uiStartIndex < batch.GetCount())
  {
    const plUInt32 uiCount = plMath::Min(batch.GetCount() - uiStartIndex, (plUInt32)MAX_SPRITE_DATA_PER_BATCH);

    FillSpriteData(batch, uiStartIndex, uiCount);
    if (m_SpriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
    {
      pContext->GetCommandEncoder()->UpdateBuffer(hSpriteData, 0, m_SpriteData.GetByteArrayPtr());

      pContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, uiCount * 2);
      pContext->DrawMeshBuffer().IgnoreResult();
    }

    uiStartIndex += uiCount;
  }
}

plGALBufferHandle plSpriteRenderer::CreateSpriteDataBuffer() const
{
  plGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(SpriteData);
  desc.m_uiTotalSize = desc.m_uiStructSize * MAX_SPRITE_DATA_PER_BATCH;
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

void plSpriteRenderer::FillSpriteData(const plRenderDataBatch& batch, plUInt32 uiStartIndex, plUInt32 uiCount) const
{
  m_SpriteData.Clear();
  m_SpriteData.Reserve(uiCount);

  for (auto it = batch.GetIterator<plSpriteRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const plSpriteRenderData* pRenderData = it;

    auto& spriteData = m_SpriteData.ExpandAndGetRef();

    spriteData.m_worldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    spriteData.m_size = pRenderData->m_fSize;
    spriteData.m_maxScreenSize = pRenderData->m_fMaxScreenSize;
    spriteData.m_aspectRatio = pRenderData->m_fAspectRatio;
    spriteData.m_colorRG = plShaderUtils::Float2ToRG16F(plVec2(pRenderData->m_color.r, pRenderData->m_color.g));
    spriteData.m_colorBA = plShaderUtils::Float2ToRG16F(plVec2(pRenderData->m_color.b, pRenderData->m_color.a));
    spriteData.m_texCoordScale = plShaderUtils::Float2ToRG16F(pRenderData->m_texCoordScale);
    spriteData.m_texCoordOffset = plShaderUtils::Float2ToRG16F(pRenderData->m_texCoordOffset);
    spriteData.m_gameObjectID = pRenderData->m_uiUniqueID;
    spriteData.m_reserved = 0;
  }
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteRenderer);
