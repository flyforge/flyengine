#include <EnginePluginKraut/EnginePluginKrautPCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>

#include <GameEngine/Effects/Wind/SimpleWindComponent.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Components/SkyBoxComponent.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeContext, 1, plRTTIDefaultAllocator<plKrautTreeContext>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_CONSTANT_PROPERTY("DocumentType", (const char*) "Kraut Tree"),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plKrautTreeContext::plKrautTreeContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMainObject = nullptr;
}

void plKrautTreeContext::HandleMessage(const plEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = plDynamicCast<const plQuerySelectionBBoxMsgToEngine*>(pMsg0))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = plDynamicCast<const plSimpleDocumentConfigMsgToEngine*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "UpdateTree" && !m_hKrautComponent.IsInvalidated())
    {
      PLASMA_LOCK(m_pWorld->GetWriteMarker());

      plKrautTreeComponent* pTree = nullptr;
      if (!m_pWorld->TryGetComponent(m_hKrautComponent, pTree))
        return;

      if (pMsg->m_sPayload == "DisplayRandomSeed")
      {
        m_uiDisplayRandomSeed = static_cast<plUInt32>(pMsg->m_fPayload);

        pTree->SetCustomRandomSeed(m_uiDisplayRandomSeed);
      }
    }

    return;
  }

  plEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void plKrautTreeContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  PLASMA_LOCK(pWorld->GetWriteMarker());


  plKrautTreeComponent* pTree;

  // Preview Mesh
  {
    plGameObjectDesc obj;
    obj.m_sName.Assign("KrautTreePreview");
    // TODO: making the object dynamic is a workaround!
    // without it, shadows keep disappearing when switching between tree documents
    // triggering resource reload will also trigger plKrautTreeComponent::OnMsgExtractRenderData,
    // which fixes the shadows for a while, but not caching the render-data (plRenderData::Caching::IfStatic)
    // 'solves' the issue in the preview
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pMainObject);

    const plTag& tagCastShadows = plTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMainObject->SetTag(tagCastShadows);

    m_hKrautComponent = plKrautTreeComponent::CreateComponent(m_pMainObject, pTree);
    plStringBuilder sMeshGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMainResource = plResourceManager::LoadResource<plKrautGeneratorResource>(sMeshGuid);
    pTree->SetVariationIndex(0xFFFF); // takes the 'display seed'
    pTree->SetKrautGeneratorResource(m_hMainResource);
  }


  // Wind
  {
    plGameObjectDesc obj;
    obj.m_sName.Assign("Wind");

    plGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    plSimpleWindComponent* pWind = nullptr;
    plSimpleWindComponent::CreateComponent(pObj, pWind);

    pWind->m_Deviation = plAngle::MakeFromDegree(180);
    pWind->m_MinWindStrength = plWindStrength::Calm;
    pWind->m_MaxWindStrength = plWindStrength::ModerateBreple;
  }

  // ground
  {
    const char* szMeshName = "KrautPreviewGroundMesh";
    m_hPreviewMeshResource = plResourceManager::GetExistingResource<plMeshResource>(szMeshName);

    if (!m_hPreviewMeshResource.IsValid())
    {
      const char* szMeshBufferName = "KrautPreviewGroundMeshBuffer";

      plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        plGeometry::GeoOptions opt;
        opt.m_Transform = plMat4::MakeTranslation(plVec3(0, 0, -0.05f));

        plGeometry geom;
        geom.AddCylinder(8.0f, 7.9f, 0.05f, 0.05f, true, true, 32, opt);
        geom.TriangulatePolygons();
        geom.ComputeTangents();

        plMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

        hMeshBuffer = plResourceManager::GetOrCreateResource<plMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }
      {
        plResourceLock<plMeshBufferResource> pMeshBuffer(hMeshBuffer, plResourceAcquireMode::AllowLoadingFallback);

        plMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Pattern.plMaterialAsset
        md.ComputeBounds();

        m_hPreviewMeshResource = plResourceManager::GetOrCreateResource<plMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }

    // Ground Mesh Component
    {
      plGameObjectDesc obj;
      obj.m_sName.Assign("KrautGround");

      plGameObject* pObj;
      pWorld->CreateObject(obj, pObj);

      plMeshComponent* pMesh;
      plMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hPreviewMeshResource);
    }
  }
}

plEngineProcessViewContext* plKrautTreeContext::CreateViewContext()
{
  return PLASMA_DEFAULT_NEW(plKrautTreeViewContext, this);
}

void plKrautTreeContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PLASMA_DEFAULT_DELETE(pContext);
}

bool plKrautTreeContext::UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext)
{
  {
    PLASMA_LOCK(m_pWorld->GetWriteMarker());

    m_pMainObject->UpdateLocalBounds();
    m_pMainObject->UpdateGlobalTransformAndBounds();
  }

  plBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  // undo the artificial bounds scale to get a tight bbox for better thumbnails
  const float fAdditionalZoom = 1.5f;
  bounds.m_fSphereRadius /= plKrautTreeComponent::s_iLocalBoundsScale * fAdditionalZoom;
  bounds.m_vBoxHalfExtends /= plKrautTreeComponent::s_iLocalBoundsScale * fAdditionalZoom;

  plKrautTreeViewContext* pMeshViewContext = static_cast<plKrautTreeViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void plKrautTreeContext::QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg)
{
  if (m_pMainObject == nullptr)
    return;

  plBoundingBoxSphere bounds = plBoundingBoxSphere::MakeInvalid();

  {
    PLASMA_LOCK(m_pWorld->GetWriteMarker());

    m_pMainObject->UpdateLocalBounds();
    m_pMainObject->UpdateGlobalTransformAndBounds();
    auto b = m_pMainObject->GetGlobalBounds();

    if (b.IsValid())
    {
      b.m_fSphereRadius /= plKrautTreeComponent::s_iLocalBoundsScale;
      b.m_vBoxHalfExtends /= (float)plKrautTreeComponent::s_iLocalBoundsScale;

      bounds.ExpandToInclude(b);
    }
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
