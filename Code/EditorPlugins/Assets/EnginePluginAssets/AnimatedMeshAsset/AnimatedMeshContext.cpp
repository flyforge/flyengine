#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshContext.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshView.h>

#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimatedMeshContext, 1, plRTTIDefaultAllocator<plAnimatedMeshContext>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_CONSTANT_PROPERTY("DocumentType", (const char*) "Animated Mesh"),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAnimatedMeshContext::plAnimatedMeshContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pAnimatedMeshObject = nullptr;
}

void plAnimatedMeshContext::HandleMessage(const plEditorEngineDocumentMsg* pDocMsg)
{
  if (auto* pMsg = plDynamicCast<const plEditorEngineSetMaterialsMsg*>(pDocMsg))
  {
    plAnimatedMeshComponent* pAnimatedMesh;
    if (m_pAnimatedMeshObject && m_pAnimatedMeshObject->TryGetComponentOfBaseType(pAnimatedMesh))
    {
      for (plUInt32 i = 0; i < pMsg->m_Materials.GetCount(); ++i)
      {
        plMaterialResourceHandle hMat;

        if (!pMsg->m_Materials[i].IsEmpty())
        {
          hMat = plResourceManager::LoadResource<plMaterialResource>(pMsg->m_Materials[i]);
        }

        pAnimatedMesh->SetMaterial(i, hMat);
      }
    }

    return;
  }

  if (auto* pMsg = plDynamicCast<const plQuerySelectionBBoxMsgToEngine*>(pDocMsg))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = plDynamicCast<const plSimpleDocumentConfigMsgToEngine*>(pDocMsg))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_fPayload > 0;
        return;
      }
    }
  }

  plEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void plAnimatedMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  PL_LOCK(pWorld->GetWriteMarker());

  plAnimatedMeshComponent* pAnimatedMesh;

  // Preview AnimatedMesh
  {
    plGameObjectDesc obj;
    obj.m_bDynamic = true;
    obj.m_sName.Assign("AnimatedMeshPreview");
    pWorld->CreateObject(obj, m_pAnimatedMeshObject);

    const plTag& tagCastShadows = plTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pAnimatedMeshObject->SetTag(tagCastShadows);

    plAnimatedMeshComponent::CreateComponent(m_pAnimatedMeshObject, pAnimatedMesh);
    plStringBuilder sAnimatedMeshGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sAnimatedMeshGuid);
    m_hAnimatedMesh = plResourceManager::LoadResource<plMeshResource>(sAnimatedMeshGuid);
    pAnimatedMesh->SetMesh(m_hAnimatedMesh);
  }
}

plEngineProcessViewContext* plAnimatedMeshContext::CreateViewContext()
{
  return PL_DEFAULT_NEW(plAnimatedMeshViewContext, this);
}

void plAnimatedMeshContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PL_DEFAULT_DELETE(pContext);
}

bool plAnimatedMeshContext::UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext)
{
  plBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  plAnimatedMeshViewContext* pAnimatedMeshViewContext = static_cast<plAnimatedMeshViewContext*>(pThumbnailViewContext);
  return pAnimatedMeshViewContext->UpdateThumbnailCamera(bounds);
}


void plAnimatedMeshContext::QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg)
{
  if (m_pAnimatedMeshObject == nullptr)
    return;

  plBoundingBoxSphere bounds = plBoundingBoxSphere::MakeInvalid();

  {
    PL_LOCK(m_pWorld->GetWriteMarker());

    m_pAnimatedMeshObject->UpdateLocalBounds();
    m_pAnimatedMeshObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pAnimatedMeshObject->GetGlobalBounds();

    if (b.IsValid())
      bounds.ExpandToInclude(b);
  }

  const plQuerySelectionBBoxMsgToEngine* msg = static_cast<const plQuerySelectionBBoxMsgToEngine*>(pMsg);

  plQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID = msg->m_uiViewID;
  res.m_iPurpose = msg->m_iPurpose;
  res.m_vCenter = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}
