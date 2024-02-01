#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>

#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureContext, 1, plRTTIDefaultAllocator<plTextureContext>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture 2D;Render Target"),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static void CreatePreviewRect(plGeometry& ref_geom)
{
  const plMat4 mTransform = plMat4::MakeIdentity();
  const plVec2 size(1.0f);
  const plColor color = plColor::White;

  const plVec2 halfSize = size * 0.5f;

  plUInt32 idx[4];

  idx[0] = ref_geom.AddVertex(plVec3(-halfSize.x, 0, -halfSize.y), plVec3(-1, 0, 0), plVec2(-1, 2), color, 0, mTransform);
  idx[1] = ref_geom.AddVertex(plVec3(halfSize.x, 0, -halfSize.y), plVec3(-1, 0, 0), plVec2(2, 2), color, 0, mTransform);
  idx[2] = ref_geom.AddVertex(plVec3(halfSize.x, 0, halfSize.y), plVec3(-1, 0, 0), plVec2(2, -1), color, 0, mTransform);
  idx[3] = ref_geom.AddVertex(plVec3(-halfSize.x, 0, halfSize.y), plVec3(-1, 0, 0), plVec2(-1, -1), color, 0, mTransform);

  ref_geom.AddPolygon(idx, false);
}

plTextureContext::plTextureContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
}

void plTextureContext::HandleMessage(const plEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plDocumentConfigMsgToEngine>())
  {
    const plDocumentConfigMsgToEngine* pMsg2 = static_cast<const plDocumentConfigMsgToEngine*>(pMsg);

    if (pMsg2->m_sWhatToDo == "ChannelMode" && m_hMaterial.IsValid())
    {
      plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::AllowLoadingFallback);
      pMaterial->SetParameter("ShowChannelMode", pMsg2->m_iValue);
      pMaterial->SetParameter("LodLevel", pMsg2->m_fValue);
    }
  }

  plEngineProcessDocumentContext::HandleMessage(pMsg);
}

void plTextureContext::OnInitialize()
{
  plStringBuilder sTextureGuid;
  plConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);
  const plStringBuilder sMaterialResource(sTextureGuid.GetData(), " - Texture Preview");

  m_hMaterial = plResourceManager::GetExistingResource<plMaterialResource>(sMaterialResource);

  m_hTexture = plResourceManager::LoadResource<plTexture2DResource>(sTextureGuid);
  plGALResourceFormat::Enum textureFormat = plGALResourceFormat::Invalid;
  {
    plResourceLock<plTexture2DResource> pTexture(m_hTexture, plResourceAcquireMode::PointerOnly);

    textureFormat = pTexture->GetFormat();
    pTexture->m_ResourceEvents.AddEventHandler(plMakeDelegate(&plTextureContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);
  }

  // Preview Mesh
  const char* szMeshName = "DefaultTexturePreviewMesh";
  m_hPreviewMeshResource = plResourceManager::GetExistingResource<plMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultTexturePreviewMeshBuffer";

    plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      plGeometry geom;
      CreatePreviewRect(geom);
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
      md.SetMaterial(0, "");
      md.ComputeBounds();

      m_hPreviewMeshResource = plResourceManager::GetOrCreateResource<plMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  // Preview Material
  if (!m_hMaterial.IsValid())
  {
    plMaterialResourceDescriptor md;
    md.m_hBaseMaterial = plResourceManager::LoadResource<plMaterialResource>("Editor/Materials/TexturePreview.plMaterial");

    auto& tb = md.m_Texture2DBindings.ExpandAndGetRef();
    tb.m_Name.Assign("BaseTexture");
    tb.m_Value = m_hTexture;

    auto& param = md.m_Parameters.ExpandAndGetRef();
    param.m_Name.Assign("IsLinear");
    param.m_Value = textureFormat != plGALResourceFormat::Invalid ? !plGALResourceFormat::IsSrgb(textureFormat) : false;

    m_hMaterial = plResourceManager::GetOrCreateResource<plMaterialResource>(sMaterialResource, std::move(md));
  }

  // Preview Object
  {
    PL_LOCK(m_pWorld->GetWriteMarker());

    plGameObjectDesc obj;
    plGameObject* pObj;

    obj.m_sName.Assign("TexturePreview");
    obj.m_LocalRotation = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(90));
    m_hPreviewObject = m_pWorld->CreateObject(obj, pObj);

    plMeshComponent* pMesh;
    m_hPreviewMesh2D = plMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
    pMesh->SetMaterial(0, m_hMaterial);
  }
}

plEngineProcessViewContext* plTextureContext::CreateViewContext()
{
  return PL_DEFAULT_NEW(plTextureViewContext, this);
}

void plTextureContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PL_DEFAULT_DELETE(pContext);
}

void plTextureContext::OnResourceEvent(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUpdated)
  {
    const plTexture2DResource* pTexture = static_cast<const plTexture2DResource*>(e.m_pResource);
    if (pTexture->GetFormat() != plGALResourceFormat::Invalid)
    {
      plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::BlockTillLoaded);
      pMaterial->SetParameter("IsLinear", !plGALResourceFormat::IsSrgb(pTexture->GetFormat()));
    }
  }
}
