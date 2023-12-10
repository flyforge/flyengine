#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EditorEngineProcessFramework/PickingRenderPass/PickingRenderPass.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererCore/../../../Data/Base/Shaders/Editor/GizmoConstants.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGizmoRenderer, 1, plRTTIDefaultAllocator<plGizmoRenderer>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

float plGizmoRenderer::s_fGizmoScale = 1.0f;

plGizmoRenderer::plGizmoRenderer() = default;
plGizmoRenderer::~plGizmoRenderer() = default;

void plGizmoRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& inout_types) const
{
  inout_types.PushBack(plGetStaticRTTI<plGizmoRenderData>());
}

void plGizmoRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& inout_categories) const
{
  inout_categories.PushBack(plDefaultRenderDataCategories::SimpleOpaque);
  inout_categories.PushBack(plDefaultRenderDataCategories::SimpleForeground);
}

void plGizmoRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  bool bOnlyPickable = false;

  if (auto pPickingRenderPass = plDynamicCast<const plPickingRenderPass*>(pPass))
  {
    // gizmos only exist for 'selected' objects, so ignore all gizmo rendering, if we don't want to pick selected objects
    if (!pPickingRenderPass->m_bPickSelected)
      return;

    bOnlyPickable = true;
  }

  const plGizmoRenderData* pRenderData = batch.GetFirstData<plGizmoRenderData>();

  const plMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  const plMaterialResourceHandle& hMaterial = pRenderData->m_hMaterial;
  plUInt32 uiSubMeshIndex = pRenderData->m_uiSubMeshIndex;

  plResourceLock<plMeshResource> pMesh(hMesh, plResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiSubMeshIndex)
  {
    return;
  }

  const plMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiSubMeshIndex];

  renderViewContext.m_pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());
  renderViewContext.m_pRenderContext->BindMaterial(hMaterial);

  plConstantBufferStorage<plGizmoConstants>* pGizmoConstantBuffer;
  plConstantBufferStorageHandle hGizmoConstantBuffer = plRenderContext::CreateConstantBufferStorage(pGizmoConstantBuffer);
  PLASMA_SCOPE_EXIT(plRenderContext::DeleteConstantBufferStorage(hGizmoConstantBuffer));

  renderViewContext.m_pRenderContext->BindConstantBuffer("plGizmoConstants", hGizmoConstantBuffer);

  // since typically the fov is tied to the height, we orient the gizmo size on that
  const float fGizmoScale = s_fGizmoScale * (128.0f / (float)renderViewContext.m_pViewData->m_ViewPortRect.height);

  for (auto it = batch.GetIterator<plGizmoRenderData>(); it.IsValid(); ++it)
  {
    pRenderData = it;

    if (bOnlyPickable && !pRenderData->m_bIsPickable)
      continue;

    PLASMA_ASSERT_DEV(pRenderData->m_hMesh == hMesh, "Invalid batching (mesh)");
    PLASMA_ASSERT_DEV(pRenderData->m_hMaterial == hMaterial, "Invalid batching (material)");
    PLASMA_ASSERT_DEV(pRenderData->m_uiSubMeshIndex == uiSubMeshIndex, "Invalid batching (part)");

    plGizmoConstants& cb = pGizmoConstantBuffer->GetDataForWriting();
    cb.ObjectToWorldMatrix = pRenderData->m_GlobalTransform.GetAsMat4();
    cb.WorldToObjectMatrix = cb.ObjectToWorldMatrix;
    cb.WorldToObjectMatrix.Invert(0.001f).IgnoreResult(); // this can fail, if scale is 0 (which happens), doesn't matter in those cases
    cb.GizmoColor = pRenderData->m_GizmoColor;
    cb.GizmoScale = fGizmoScale;
    cb.GameObjectID = pRenderData->m_uiUniqueID;

    if (renderViewContext.m_pRenderContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive).Failed())
    {
      // draw bounding box instead
      if (pRenderData->m_GlobalBounds.IsValid())
      {
        plDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pRenderData->m_GlobalBounds.GetBox(), plColor::Magenta);
      }
    }
  }
}
