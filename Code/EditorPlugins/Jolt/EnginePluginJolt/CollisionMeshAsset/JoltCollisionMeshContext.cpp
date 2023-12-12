#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshContext.h>
#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshView.h>

#include <JoltPlugin/Components/JoltVisColMeshComponent.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltCollisionMeshContext, 1, plRTTIDefaultAllocator<plJoltCollisionMeshContext>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_CONSTANT_PROPERTY("DocumentType", (const char*) "Jolt_Colmesh_Triangle;Jolt_Colmesh_Convex"),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltCollisionMeshContext::plJoltCollisionMeshContext()
  : PlasmaEngineProcessDocumentContext(PlasmaEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMeshObject = nullptr;
}

void plJoltCollisionMeshContext::HandleMessage(const PlasmaEditorEngineDocumentMsg* pDocMsg)
{
  if (auto pMsg = plDynamicCast<const plQuerySelectionBBoxMsgToEngine*>(pDocMsg))
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

void plJoltCollisionMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  PLASMA_LOCK(pWorld->GetWriteMarker());

  plGameObjectDesc obj;
  plJoltVisColMeshComponent* pMesh = nullptr;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, m_pMeshObject);

    const plTag& tagCastShadows = plTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMeshObject->SetTag(tagCastShadows);

    plJoltVisColMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    plStringBuilder sMeshGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMesh = plResourceManager::LoadResource<plJoltMeshResource>(sMeshGuid);
    pMesh->SetMesh(m_hMesh);
  }
}

PlasmaEngineProcessViewContext* plJoltCollisionMeshContext::CreateViewContext()
{
  return PLASMA_DEFAULT_NEW(plJoltCollisionMeshViewContext, this);
}

void plJoltCollisionMeshContext::DestroyViewContext(PlasmaEngineProcessViewContext* pContext)
{
  PLASMA_DEFAULT_DELETE(pContext);
}

bool plJoltCollisionMeshContext::UpdateThumbnailViewContext(PlasmaEngineProcessViewContext* pThumbnailViewContext)
{
  plBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  plJoltCollisionMeshViewContext* pMeshViewContext = static_cast<plJoltCollisionMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void plJoltCollisionMeshContext::QuerySelectionBBox(const PlasmaEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  plBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    PLASMA_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
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
