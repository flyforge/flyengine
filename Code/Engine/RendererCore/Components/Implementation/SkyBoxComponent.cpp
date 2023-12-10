#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/TextureCubeResource.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plSkyBoxComponent, 4, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("CubeMap", GetCubeMapFile, SetCubeMapFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    PLASMA_ACCESSOR_PROPERTY("ExposureBias", GetExposureBias, SetExposureBias)->AddAttributes(new plClampValueAttribute(-32.0f, 32.0f)),
    PLASMA_ACCESSOR_PROPERTY("InverseTonemap", GetInverseTonemap, SetInverseTonemap),
    PLASMA_ACCESSOR_PROPERTY("UseFog", GetUseFog, SetUseFog)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ACCESSOR_PROPERTY("VirtualDistance", GetVirtualDistance, SetVirtualDistance)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1000.0f)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plSkyBoxComponent::plSkyBoxComponent() = default;
plSkyBoxComponent::~plSkyBoxComponent() = default;

void plSkyBoxComponent::Initialize()
{
  SUPER::Initialize();

  const char* szBufferResourceName = "SkyBoxBuffer";
  plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szBufferResourceName);
  if (!hMeshBuffer.IsValid())
  {
    plGeometry geom;
    geom.AddRectXY(plVec2(2.0f));

    plMeshBufferResourceDescriptor desc;
    desc.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

    hMeshBuffer = plResourceManager::GetOrCreateResource<plMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
  }

  const char* szMeshResourceName = "SkyBoxMesh";
  m_hMesh = plResourceManager::GetExistingResource<plMeshResource>(szMeshResourceName);
  if (!m_hMesh.IsValid())
  {
    plMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    desc.AddSubMesh(2, 0, 0);
    desc.ComputeBounds();

    m_hMesh = plResourceManager::GetOrCreateResource<plMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
  }

  plStringBuilder cubeMapMaterialName = "SkyBoxMaterial_CubeMap";
  cubeMapMaterialName.AppendFormat("_{0}", plArgP(GetWorld())); // make the resource unique for each world

  m_hCubeMapMaterial = plResourceManager::GetExistingResource<plMaterialResource>(cubeMapMaterialName);
  if (!m_hCubeMapMaterial.IsValid())
  {
    plMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = plResourceManager::LoadResource<plMaterialResource>("{ b4b75b1c-c2c8-4a0e-8076-780bdd46d18b }"); // Sky.plMaterialAsset

    m_hCubeMapMaterial = plResourceManager::CreateResource<plMaterialResource>(cubeMapMaterialName, std::move(desc), cubeMapMaterialName);
  }

  UpdateMaterials();
}

plResult plSkyBoxComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return PLASMA_SUCCESS;
}

void plSkyBoxComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't extract sky render data for selection or in orthographic views.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory || msg.m_pView->GetCamera()->IsOrthographic())
    return;

  plMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalTransform.m_vPosition.SetZero(); // skybox should always be at the origin
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = m_hCubeMapMaterial;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::Sky, plRenderData::Caching::Never);
}

void plSkyBoxComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_fExposureBias;
  s << m_bInverseTonemap;
  s << m_bUseFog;
  s << m_fVirtualDistance;
  s << m_hCubeMap;
}

void plSkyBoxComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  s >> m_fExposureBias;
  s >> m_bInverseTonemap;

  if (uiVersion >= 4)
  {
    s >> m_bUseFog;
    s >> m_fVirtualDistance;
  }

  if (uiVersion >= 3)
  {
    s >> m_hCubeMap;
  }
  else
  {
    plTexture2DResourceHandle dummyHandle;
    for (int i = 0; i < 6; i++)
    {
      s >> dummyHandle;
    }
  }
}

void plSkyBoxComponent::SetExposureBias(float fExposureBias)
{
  m_fExposureBias = fExposureBias;

  UpdateMaterials();
}

void plSkyBoxComponent::SetInverseTonemap(bool bInverseTonemap)
{
  m_bInverseTonemap = bInverseTonemap;

  UpdateMaterials();
}

void plSkyBoxComponent::SetUseFog(bool bUseFog)
{
  m_bUseFog = bUseFog;

  UpdateMaterials();
}

void plSkyBoxComponent::SetVirtualDistance(float fVirtualDistance)
{
  m_fVirtualDistance = fVirtualDistance;

  UpdateMaterials();
}

void plSkyBoxComponent::SetCubeMapFile(const char* szFile)
{
  plTextureCubeResourceHandle hCubeMap;
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hCubeMap = plResourceManager::LoadResource<plTextureCubeResource>(szFile);
  }

  SetCubeMap(hCubeMap);
}

const char* plSkyBoxComponent::GetCubeMapFile() const
{
  return m_hCubeMap.IsValid() ? m_hCubeMap.GetResourceID().GetData() : "";
}

void plSkyBoxComponent::SetCubeMap(const plTextureCubeResourceHandle& hCubeMap)
{
  m_hCubeMap = hCubeMap;
  UpdateMaterials();
}

const plTextureCubeResourceHandle& plSkyBoxComponent::GetCubeMap() const
{
  return m_hCubeMap;
}

void plSkyBoxComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMaterials();
}

void plSkyBoxComponent::UpdateMaterials()
{
  if (m_hCubeMapMaterial.IsValid())
  {
    plResourceLock<plMaterialResource> pMaterial(m_hCubeMapMaterial, plResourceAcquireMode::AllowLoadingFallback);

    pMaterial->SetParameter("ExposureBias", m_fExposureBias);
    pMaterial->SetParameter("InverseTonemap", m_bInverseTonemap);
    pMaterial->SetParameter("UseFog", m_bUseFog);
    pMaterial->SetParameter("VirtualDistance", m_fVirtualDistance);
    pMaterial->SetTextureCubeBinding("CubeMap", m_hCubeMap);

    pMaterial->PreserveCurrentDesc();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class plSkyBoxComponentPatch_1_2 : public plGraphPatch
{
public:
  plSkyBoxComponentPatch_1_2()
    : plGraphPatch("plSkyBoxComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Exposure Bias", "ExposureBias");
    pNode->RenameProperty("Inverse Tonemap", "InverseTonemap");
    pNode->RenameProperty("Left Texture", "LeftTexture");
    pNode->RenameProperty("Front Texture", "FrontTexture");
    pNode->RenameProperty("Right Texture", "RightTexture");
    pNode->RenameProperty("Back Texture", "BackTexture");
    pNode->RenameProperty("Up Texture", "UpTexture");
    pNode->RenameProperty("Down Texture", "DownTexture");
  }
};

plSkyBoxComponentPatch_1_2 g_plSkyBoxComponentPatch_1_2;



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SkyBoxComponent);
