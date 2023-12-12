#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Components/VolumetricCloudsComponent.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/View.h>

#include <RendererCore/Lights/DirectionalLightComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plVolumetricCloudsComponent, 1, plComponentMode::Static)
{

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Alpha),
    new plCategoryAttribute("Effects/Sky"),
    new plColorAttribute(plColorScheme::Effects),
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

plVolumetricCloudsComponent::plVolumetricCloudsComponent() = default;
plVolumetricCloudsComponent::~plVolumetricCloudsComponent() = default;

void plVolumetricCloudsComponent::Initialize()
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

  const char* szMeshResourceName = "VolulmetricCloudsMesh";
  m_hMesh = plResourceManager::GetExistingResource<plMeshResource>(szMeshResourceName);
  if (!m_hMesh.IsValid())
  {
    plMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    desc.AddSubMesh(2, 0, 0);
    desc.ComputeBounds();

    m_hMesh = plResourceManager::GetOrCreateResource<plMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
  }

  plStringBuilder cubeMapMaterialName = "VolulmetricCloudsMaterial";
  cubeMapMaterialName.AppendFormat("_{0}", plArgP(GetWorld())); // make the resource unique for each world

  m_hMaterial = plResourceManager::GetExistingResource<plMaterialResource>(cubeMapMaterialName);
  if (!m_hMaterial.IsValid())
  {
    plMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = plResourceManager::LoadResource<plMaterialResource>("{ 0ff457ed-7091-4bce-879f-21da28318922 }"); // VolulmetricClouds.plMaterialAsset

    m_hMaterial = plResourceManager::CreateResource<plMaterialResource>(cubeMapMaterialName, std::move(desc), cubeMapMaterialName);
  }

  m_hNoiseLut = plResourceManager::LoadResource<plTexture3DResource>("{ 9b0145e8-4c39-4326-af73-f75cf78eb7fa }"); // Clouds.plLutAsset
  if(!m_hNoiseLut.IsValid())
  {
    plLog::Error("Failed to find resource Clouds.plLUTAsset (9b0145e8-4c39-4326-af73-f75cf78eb7fa)");
  }

  m_hDetailNoiseLut = plResourceManager::LoadResource<plTexture3DResource>("{ 10533e7d-67a7-4ece-8277-7f065beef252 }"); // CloudsDetails.plLutAsset
  if (!m_hDetailNoiseLut.IsValid())
  {
    plLog::Error("Failed to find resource CloudsDetails.plLUTAsset (10533e7d-67a7-4ece-8277-7f065beef252)");
  }

  UpdateMaterials();
}

plResult plVolumetricCloudsComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return PLASMA_SUCCESS;
}

void plVolumetricCloudsComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't extract sky render data for selection or in orthographic views.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory || msg.m_pView->GetCamera()->IsOrthographic())
    return;

  UpdateMaterials();

  plMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = plTransform::IdentityTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  plRenderData::Category category = plDefaultRenderDataCategories::PostSky;

  if (m_hMaterial.IsValid())
  {
    plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::AllowLoadingFallback);
    category = pMaterial->GetRenderDataCategory();
  }

  msg.AddRenderData(pRenderData, category, plRenderData::Caching::Never);
}

void plVolumetricCloudsComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();
}

void plVolumetricCloudsComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();
}

void plVolumetricCloudsComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMaterials();
}

void plVolumetricCloudsComponent::UpdateMaterials() const
{
  if (m_hMaterial.IsValid())
  {
    plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::AllowLoadingFallback);

    plVec3 sunDirection = GetOwner()->GetGlobalTransform().m_qRotation * plVec3(-1, 0, 0);
    pMaterial->SetParameter("SunDir", sunDirection);
    pMaterial->SetTexture3DBinding("NoiseMap", m_hNoiseLut);
    pMaterial->SetTexture3DBinding("DetailNoiseMap", m_hDetailNoiseLut);

    pMaterial->PreserveCurrentDesc();
  }
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_plVolulmetricCloudsComponent);
