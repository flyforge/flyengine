#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>

#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureCubeContext, 1, plRTTIDefaultAllocator<plTextureCubeContext>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture Cube"),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTextureCubeContext::plTextureCubeContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
}

void plTextureCubeContext::HandleMessage(const plEditorEngineDocumentMsg* pMsg)
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

void plTextureCubeContext::OnInitialize()
{
  const char* szMeshName = "DefaultTextureCubePreviewMesh";
  plStringBuilder sTextureGuid;
  plConversionUtils::ToString(GetDocumentGuid(), sTextureGuid);
  const plStringBuilder sMaterialResource(sTextureGuid.GetData(), " - TextureCube Preview");

  m_hPreviewMeshResource = plResourceManager::GetExistingResource<plMeshResource>(szMeshName);
  m_hMaterial = plResourceManager::GetExistingResource<plMaterialResource>(sMaterialResource);

  m_hTexture = plResourceManager::LoadResource<plTextureCubeResource>(sTextureGuid);
  plGALResourceFormat::Enum textureFormat = plGALResourceFormat::Invalid;
  {
    plResourceLock<plTextureCubeResource> pTexture(m_hTexture, plResourceAcquireMode::PointerOnly);

    textureFormat = pTexture->GetFormat();
    pTexture->m_ResourceEvents.AddEventHandler(plMakeDelegate(&plTextureCubeContext::OnResourceEvent, this), m_TextureResourceEventSubscriber);
  }

  // Preview Mesh
  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "DefaultTextureCubePreviewMeshBuffer";

    plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      plGeometry geom;
      geom.AddSphere(0.5f, 64, 64);
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
    md.m_hBaseMaterial = plResourceManager::LoadResource<plMaterialResource>("Editor/Materials/TextureCubePreview.plMaterial");

    auto& tb = md.m_TextureCubeBindings.ExpandAndGetRef();
    tb.m_Name.Assign("BaseTexture");
    tb.m_Value = m_hTexture;

    auto& param = md.m_Parameters.ExpandAndGetRef();
    param.m_Name.Assign("IsLinear");
    param.m_Value = textureFormat != plGALResourceFormat::Invalid ? !plGALResourceFormat::IsSrgb(textureFormat) : false;

    m_hMaterial = plResourceManager::GetOrCreateResource<plMaterialResource>(sMaterialResource, std::move(md));
  }

  // Preview Object
  {
    PLASMA_LOCK(m_pWorld->GetWriteMarker());

    plGameObjectDesc obj;
    plGameObject* pObj;

    obj.m_sName.Assign("TextureCubePreview");
    obj.m_LocalRotation = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(90));
    m_hPreviewObject = m_pWorld->CreateObject(obj, pObj);

    plMeshComponent* pMesh;
    m_hPreviewMesh2D = plMeshComponent::CreateComponent(pObj, pMesh);
    pMesh->SetMesh(m_hPreviewMeshResource);
    pMesh->SetMaterial(0, m_hMaterial);
  }
}

plEngineProcessViewContext* plTextureCubeContext::CreateViewContext()
{
  return PLASMA_DEFAULT_NEW(plTextureCubeViewContext, this);
}

void plTextureCubeContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PLASMA_DEFAULT_DELETE(pContext);
}

void plTextureCubeContext::OnResourceEvent(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUpdated)
  {
    const plTextureCubeResource* pTexture = static_cast<const plTextureCubeResource*>(e.m_pResource);
    if (pTexture->GetFormat() != plGALResourceFormat::Invalid)
    {
      plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::BlockTillLoaded);
      pMaterial->SetParameter("IsLinear", !plGALResourceFormat::IsSrgb(pTexture->GetFormat()));
    }
  }
}
