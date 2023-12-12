#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>

#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshContext, 1, plRTTIDefaultAllocator<plMeshContext>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_CONSTANT_PROPERTY("DocumentType", (const char*) "Mesh"),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plMeshContext::plMeshContext()
  : PlasmaEngineProcessDocumentContext(PlasmaEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMeshObject = nullptr;
}

void plMeshContext::HandleMessage(const PlasmaEditorEngineDocumentMsg* pDocMsg)
{
  if (auto* pMsg = plDynamicCast<const PlasmaEditorEngineSetMaterialsMsg*>(pDocMsg))
  {
    plMeshComponent* pMesh;
    if (m_pMeshObject && m_pMeshObject->TryGetComponentOfBaseType(pMesh))
    {
      for (plUInt32 i = 0; i < pMsg->m_Materials.GetCount(); ++i)
      {
        plMaterialResourceHandle hMat;

        if (!pMsg->m_Materials[i].IsEmpty())
        {
          hMat = plResourceManager::LoadResource<plMaterialResource>(pMsg->m_Materials[i]);
        }

        pMesh->SetMaterial(i, hMat);
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

  PlasmaEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void plMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  PLASMA_LOCK(pWorld->GetWriteMarker());

  plGameObjectDesc obj;
  plMeshComponent* pMesh;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, m_pMeshObject);

    const plTag& tagCastShadows = plTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMeshObject->SetTag(tagCastShadows);

    plMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    plStringBuilder sMeshGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMesh = plResourceManager::LoadResource<plMeshResource>(sMeshGuid);
    pMesh->SetMesh(m_hMesh);

    {
      plResourceLock<plMeshResource> pMeshRes(m_hMesh, plResourceAcquireMode::PointerOnly);
      pMeshRes->m_ResourceEvents.AddEventHandler(plMakeDelegate(&plMeshContext::OnResourceEvent, this), m_MeshResourceEventSubscriber);
    }
  }
}

PlasmaEngineProcessViewContext* plMeshContext::CreateViewContext()
{
  return PLASMA_DEFAULT_NEW(plMeshViewContext, this);
}

void plMeshContext::DestroyViewContext(PlasmaEngineProcessViewContext* pContext)
{
  PLASMA_DEFAULT_DELETE(pContext);
}

bool plMeshContext::UpdateThumbnailViewContext(PlasmaEngineProcessViewContext* pThumbnailViewContext)
{
  if (m_bBoundsDirty)
  {
    PLASMA_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    m_bBoundsDirty = false;
  }
  plBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  plMeshViewContext* pMeshViewContext = static_cast<plMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void plMeshContext::QuerySelectionBBox(const PlasmaEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  plBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    PLASMA_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    m_bBoundsDirty = false;
    const auto& b = m_pMeshObject->GetGlobalBounds();

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

void plMeshContext::OnResourceEvent(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUpdated)
  {
    m_bBoundsDirty = true;
  }
}
