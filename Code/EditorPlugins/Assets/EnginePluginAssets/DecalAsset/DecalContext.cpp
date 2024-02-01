#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalContext, 1, plRTTIDefaultAllocator<plDecalContext>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_CONSTANT_PROPERTY("DocumentType", (const char*) "Decal"),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDecalContext::plDecalContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
}

void plDecalContext::OnInitialize()
{
  const char* szMeshName = "DefaultDecalPreviewMesh";
  m_hPreviewMeshResource = plResourceManager::GetExistingResource<plMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultDecalPreviewMeshBuffer";

    plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      plGeometry geom;
      plGeometry::GeoOptions opt;

      geom.AddBox(plVec3(0.5f, 1.0f, 1.0f), true);

      plMat4 t, r;
      t = plMat4::MakeTranslation(plVec3(0, 1.5f, 0));
      r = plMat4::MakeRotationZ(plAngle::MakeFromDegree(90));
      opt.m_Transform = t * r;
      geom.AddSphere(0.5f, 64, 64, opt);

      t.SetTranslationVector(plVec3(0, -1.5f, 0));
      r = plMat4::MakeRotationY(plAngle::MakeFromDegree(90));
      opt.m_Transform = t * r;
      geom.AddTorus(0.1f, 0.5f, 32, 64, true, opt);

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
      md.SetMaterial(0, "Materials/Common/TestBricks.plMaterial");
      md.ComputeBounds();

      m_hPreviewMeshResource = plResourceManager::GetOrCreateResource<plMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  auto pWorld = m_pWorld;
  PL_LOCK(pWorld->GetWriteMarker());

  plGameObjectDesc obj;
  plGameObject* pObj;

  // Preview Mesh that the decals get projected onto
  {
    obj.m_sName.Assign("DecalPreview");
    pWorld->CreateObject(obj, pObj);

    plMeshComponent* pMesh;
    plMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
  }

  // decals
  {
    plStringBuilder sDecalGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sDecalGuid);

    // box
    {
      obj.m_sName.Assign("Decal1");
      obj.m_LocalPosition.Set(-0.25f, 0, 0);
      pWorld->CreateObject(obj, pObj);

      plDecalComponent* pDecal;
      plDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // torus
    {
      obj.m_sName.Assign("Decal2");
      obj.m_LocalPosition.Set(-0.2f, -1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      plDecalComponent* pDecal;
      plDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // sphere
    {
      obj.m_sName.Assign("Decal3");
      obj.m_LocalPosition.Set(-0.5f, 1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      plDecalComponent* pDecal;
      plDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }


    // box
    {
      obj.m_sName.Assign("Decal4");
      obj.m_LocalRotation = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.25f, 0, 0);
      pWorld->CreateObject(obj, pObj);

      plDecalComponent* pDecal;
      plDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(plVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // torus
    {
      obj.m_sName.Assign("Decal5");
      obj.m_LocalRotation = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.2f, -1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      plDecalComponent* pDecal;
      plDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(plVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }

    // sphere
    {
      obj.m_sName.Assign("Decal6");
      obj.m_LocalRotation = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(180));
      obj.m_LocalPosition.Set(0.5f, 1.5f, 0);
      pWorld->CreateObject(obj, pObj);

      plDecalComponent* pDecal;
      plDecalComponent::CreateComponent(pObj, pDecal);
      pDecal->SetExtents(plVec3(2));
      pDecal->DecalFile_Insert(0, sDecalGuid);
    }
  }
}

plEngineProcessViewContext* plDecalContext::CreateViewContext()
{
  return PL_DEFAULT_NEW(plDecalViewContext, this);
}

void plDecalContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PL_DEFAULT_DELETE(pContext);
}
