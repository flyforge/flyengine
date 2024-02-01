#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMaterialContext, 1, plRTTIDefaultAllocator<plMaterialContext>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_CONSTANT_PROPERTY("DocumentType", (const char*) "Material"),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plMaterialContext::plMaterialContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
}

void plMaterialContext::HandleMessage(const plEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plCreateThumbnailMsgToEngine>())
  {
    plResourceManager::RestoreResource(m_hMaterial);
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plDocumentConfigMsgToEngine>())
  {
    const plDocumentConfigMsgToEngine* pMsg2 = static_cast<const plDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "InvalidateCache")
    {
      // make sure all scenes etc rebuild their render cache
      plRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg2->m_sWhatToDo == "PreviewModel" && m_PreviewModel != (PreviewModel)pMsg2->m_iValue)
    {
      m_PreviewModel = (PreviewModel)pMsg2->m_iValue;

      auto pWorld = m_pWorld;
      PL_LOCK(pWorld->GetWriteMarker());

      plMeshComponent* pMesh;
      if (pWorld->TryGetComponent(m_hMeshComponent, pMesh))
      {
        switch (m_PreviewModel)
        {
          case PreviewModel::Ball:
            pMesh->SetMesh(m_hBallMesh);
            break;
          case PreviewModel::Sphere:
            pMesh->SetMesh(m_hSphereMesh);
            break;
          case PreviewModel::Box:
            pMesh->SetMesh(m_hBoxMesh);
            break;
          case PreviewModel::Plane:
            pMesh->SetMesh(m_hPlaneMesh);
            break;
        }
      }
    }
  }

  plEngineProcessDocumentContext::HandleMessage(pMsg);
}

void plMaterialContext::OnInitialize()
{
  {
    const char* szSphereMeshName = "SphereMaterialPreviewMesh";
    m_hSphereMesh = plResourceManager::GetExistingResource<plMeshResource>(szSphereMeshName);

    if (!m_hSphereMesh.IsValid())
    {
      const char* szMeshBufferName = "SphereMaterialPreviewMeshBuffer";

      plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        plGeometry geom;

        plGeometry::GeoOptions opt;
        opt.m_Color = plColor::Red;
        opt.m_Transform = plMat4::MakeRotationZ(plAngle::MakeFromDegree(90));
        geom.AddSphere(0.1f, 64, 64, opt);
        geom.ComputeTangents();

        plMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(plGALVertexAttributeSemantic::TexCoord1, plMeshTexCoordPrecision::ToResourceFormat(plMeshTexCoordPrecision::Default));
        desc.AddStream(plGALVertexAttributeSemantic::Color0, plGALResourceFormat::RGBAUByteNormalized);
        desc.AddStream(plGALVertexAttributeSemantic::Color1, plGALResourceFormat::RGBAUByteNormalized);
        desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

        hMeshBuffer = plResourceManager::GetOrCreateResource<plMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        plResourceLock<plMeshBufferResource> pMeshBuffer(hMeshBuffer, plResourceAcquireMode::AllowLoadingFallback);

        plMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hSphereMesh = plResourceManager::GetOrCreateResource<plMeshResource>(szSphereMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    const char* szBoxMeshName = "BoxMaterialPreviewMesh";
    m_hBoxMesh = plResourceManager::GetExistingResource<plMeshResource>(szBoxMeshName);

    if (!m_hBoxMesh.IsValid())
    {
      const char* szMeshBufferName = "BoxMaterialPreviewMeshBuffer";

      plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        plGeometry::GeoOptions opt;
        opt.m_Color = plColor::Red;

        // Build geometry
        plGeometry geom;

        geom.AddBox(plVec3(0.12f), true, opt);
        geom.ComputeTangents();

        plMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(plGALVertexAttributeSemantic::TexCoord1, plMeshTexCoordPrecision::ToResourceFormat(plMeshTexCoordPrecision::Default));
        desc.AddStream(plGALVertexAttributeSemantic::Color0, plGALResourceFormat::RGBAUByteNormalized);
        desc.AddStream(plGALVertexAttributeSemantic::Color1, plGALResourceFormat::RGBAUByteNormalized);
        desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

        hMeshBuffer = plResourceManager::GetOrCreateResource<plMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        plResourceLock<plMeshBufferResource> pMeshBuffer(hMeshBuffer, plResourceAcquireMode::AllowLoadingFallback);

        plMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hBoxMesh = plResourceManager::GetOrCreateResource<plMeshResource>(szBoxMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    const char* szPlaneMeshName = "PlaneMaterialPreviewMesh";
    m_hPlaneMesh = plResourceManager::GetExistingResource<plMeshResource>(szPlaneMeshName);

    if (!m_hPlaneMesh.IsValid())
    {
      const char* szMeshBufferName = "PlaneMaterialPreviewMeshBuffer";

      plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        plGeometry geom;

        plGeometry::GeoOptions opt;
        opt.m_Color = plColor::Red;
        opt.m_Transform = plMat4::MakeRotationZ(plAngle::MakeFromDegree(-90));
        geom.AddRectXY(plVec2(0.2f), 64, 64, opt);
        geom.ComputeTangents();

        plMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AddStream(plGALVertexAttributeSemantic::TexCoord1, plMeshTexCoordPrecision::ToResourceFormat(plMeshTexCoordPrecision::Default));
        desc.AddStream(plGALVertexAttributeSemantic::Color0, plGALResourceFormat::RGBAUByteNormalized);
        desc.AddStream(plGALVertexAttributeSemantic::Color1, plGALResourceFormat::RGBAUByteNormalized);
        desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

        hMeshBuffer = plResourceManager::GetOrCreateResource<plMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }

      {
        plResourceLock<plMeshBufferResource> pMeshBuffer(hMeshBuffer, plResourceAcquireMode::AllowLoadingFallback);

        plMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "");
        md.ComputeBounds();

        m_hPlaneMesh = plResourceManager::GetOrCreateResource<plMeshResource>(szPlaneMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }
  }

  {
    m_hBallMesh = plResourceManager::LoadResource<plMeshResource>("Editor/Meshes/MaterialBall.plMesh");
  }

  auto pWorld = m_pWorld;
  PL_LOCK(pWorld->GetWriteMarker());

  plGameObjectDesc obj;
  plGameObject* pObj;

  // Preview Mesh
  {
    obj.m_sName.Assign("MaterialPreview");
    pWorld->CreateObject(obj, pObj);

    plMeshComponent* pMesh;
    m_hMeshComponent = plMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hBallMesh);
    plStringBuilder sMaterialGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sMaterialGuid);
    m_hMaterial = plResourceManager::LoadResource<plMaterialResource>(sMaterialGuid);

    // 20 material overrides should be enough for any mesh.
    for (plUInt32 i = 0; i < 20; ++i)
    {
      pMesh->SetMaterial(i, m_hMaterial);
    }
  }
}

plEngineProcessViewContext* plMaterialContext::CreateViewContext()
{
  return PL_DEFAULT_NEW(plMaterialViewContext, this);
}

void plMaterialContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PL_DEFAULT_DELETE(pContext);
}

bool plMaterialContext::UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext)
{
  plMaterialViewContext* pMaterialViewContext = static_cast<plMaterialViewContext*>(pThumbnailViewContext);
  pMaterialViewContext->PositionThumbnailCamera();
  return true;
}
