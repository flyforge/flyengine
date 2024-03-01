#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Components/AtmosphericScatteringComponent.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/View.h>

#include <RendererCore/Lights/DirectionalLightComponent.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plAtmosphericScatteringComponent, 1, plComponentMode::Static)
{

  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("PlanetRadius", GetPlanetRadius, SetPlanetRadius)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(6380000)),
    PL_ACCESSOR_PROPERTY("AtmosphereRadius", GetAtmosphereRadius, SetAtmosphereRadius)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(7580000)),
    PL_ACCESSOR_PROPERTY("RayleighScattering", GetRayleighScattering, SetRayleighScattering)->AddAttributes(new plDefaultValueAttribute(plColor(0.04, 0.22, 0.75))),
    PL_ACCESSOR_PROPERTY("MieScattering", GetMieScattering, SetMieScattering)->AddAttributes(new plDefaultValueAttribute(plColor(0.64, 0.64, 0.64))),
    PL_ACCESSOR_PROPERTY("Absorption", GetAbsorption, SetAbsorption)->AddAttributes(new plDefaultValueAttribute(plColor(0.01f, 0.01f, 0.01f))),
    PL_ACCESSOR_PROPERTY("AmbientScattering", GetAmbientScattering, SetAmbientScattering)->AddAttributes(new plDefaultValueAttribute(plColor(0.01f, 0.02f, 0.02f))),
    PL_ACCESSOR_PROPERTY("MieScatterDirection", GetMieScatterDirection, SetMieScatterDirection)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(0.7f)),
    PL_ACCESSOR_PROPERTY("RayleighHeight", GetRayleighHeight, SetRayleighHeight)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(8e3)),
    PL_ACCESSOR_PROPERTY("MieHeight", GetMieHeight, SetMieHeight)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.2e3)),
    PL_ACCESSOR_PROPERTY("AbsorptionHeight", GetAbsorptionHeight, SetAbsorptionHeight)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(30e3)),
    PL_ACCESSOR_PROPERTY("AbsorptionFalloff", GetAbsorptionFalloff, SetAbsorptionFalloff)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(4e3)),
    PL_ACCESSOR_PROPERTY("RaySteps", GetRaySteps, SetRaySteps)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(32)),
    PL_ACCESSOR_PROPERTY("LightSteps", GetLightSteps, SetLightSteps)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(8)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects/Sky"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_COMPONENT_TYPE;
// clang-format on

plAtmosphericScatteringComponent::plAtmosphericScatteringComponent() = default;
plAtmosphericScatteringComponent::~plAtmosphericScatteringComponent() = default;

void plAtmosphericScatteringComponent::Initialize()
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

  const char* szMeshResourceName = "AtmosphericScatteringMesh";
  m_hMesh = plResourceManager::GetExistingResource<plMeshResource>(szMeshResourceName);
  if (!m_hMesh.IsValid())
  {
    plMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    desc.AddSubMesh(2, 0, 0);
    desc.ComputeBounds();

    m_hMesh = plResourceManager::GetOrCreateResource<plMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
  }

  plStringBuilder cubeMapMaterialName = "AtmosphericScatteringMaterial";
  cubeMapMaterialName.AppendFormat("_{0}", plArgP(GetWorld())); // make the resource unique for each world

  m_hMaterial = plResourceManager::GetExistingResource<plMaterialResource>(cubeMapMaterialName);
  if (!m_hMaterial.IsValid())
  {
    plMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = plResourceManager::LoadResource<plMaterialResource>("{ 152503c6-4699-46e2-86f3-0661d35bf000 }"); // AtmosphericScattering.plMaterialAsset

    m_hMaterial = plResourceManager::CreateResource<plMaterialResource>(cubeMapMaterialName, std::move(desc), cubeMapMaterialName);
  }

  UpdateMaterials();
}

plResult plAtmosphericScatteringComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return PL_SUCCESS;
}

void plAtmosphericScatteringComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't extract sky render data for selection or in orthographic views.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory || msg.m_pView->GetCamera()->IsOrthographic())
    return;

  UpdateMaterials();

  plMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = plTransform::MakeIdentity();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  plRenderData::Category category = plDefaultRenderDataCategories::Sky;

  if (m_hMaterial.IsValid())
  {
    plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::AllowLoadingFallback);
    category = pMaterial->GetRenderDataCategory();
  }

  msg.AddRenderData(pRenderData, category, plRenderData::Caching::Never);
}

void plAtmosphericScatteringComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();
}

void plAtmosphericScatteringComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();
}

void plAtmosphericScatteringComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMaterials();
}

void plAtmosphericScatteringComponent::UpdateMaterials() const
{
  if (m_hMaterial.IsValid())
  {
    plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::AllowLoadingFallback);

    plVec3 sunDirection = GetOwner()->GetGlobalTransform().m_qRotation * plVec3(-1, 0, 0);
    pMaterial->SetParameter("SunDir", sunDirection);
    pMaterial->SetParameter("PlanetRadius", m_fPlanetRadius);
    pMaterial->SetParameter("AtmosphereRadius", m_fAtmosphereRadius);
    pMaterial->SetParameter("RayleighScattering", plVec3(m_RayleighScattering.r, m_RayleighScattering.g, m_RayleighScattering.b));
    pMaterial->SetParameter("MieScattering", plVec3(m_MieScattering.r, m_MieScattering.g, m_MieScattering.b));
    pMaterial->SetParameter("Absorption", plVec3(m_Absorption.r, m_Absorption.g, m_Absorption.b));
    pMaterial->SetParameter("AmbientScattering", plVec3(m_AmbientScattering.r, m_AmbientScattering.g, m_AmbientScattering.b));
    pMaterial->SetParameter("MieScatterDirection", m_fMieScatterDirection);
    pMaterial->SetParameter("RayleighHeight", m_fRayleighHeight);
    pMaterial->SetParameter("MieHeight", m_fMieHeight);
    pMaterial->SetParameter("AbsorptionHeight", m_fAbsorptionHeight);
    pMaterial->SetParameter("AbsorptionFalloff", m_fAbsorptionFalloff);
    pMaterial->SetParameter("RaySteps", m_fRaySteps);
    pMaterial->SetParameter("LightSteps", m_fLightSteps);

    pMaterial->PreserveCurrentDesc();
  }
}

void plAtmosphericScatteringComponent::SetPlanetRadius(float fPlanetRadius)
{
  m_fPlanetRadius = fPlanetRadius;
  UpdateMaterials();
}
float plAtmosphericScatteringComponent::GetPlanetRadius() const
{
  return m_fPlanetRadius;
}

void plAtmosphericScatteringComponent::SetAtmosphereRadius(float fAtmosphereRadius)
{
  m_fAtmosphereRadius = fAtmosphereRadius;
  UpdateMaterials();
}
float plAtmosphericScatteringComponent::GetAtmosphereRadius() const
{
  return m_fAtmosphereRadius;
}

void plAtmosphericScatteringComponent::SetRayleighScattering(plColor RayleighScattering)
{
  m_RayleighScattering = RayleighScattering;
  UpdateMaterials();
}
plColor plAtmosphericScatteringComponent::GetRayleighScattering() const
{
  return m_RayleighScattering;
}

void plAtmosphericScatteringComponent::SetMieScattering(plColor MieScattering)
{
  m_MieScattering = MieScattering;
  UpdateMaterials();
}
plColor plAtmosphericScatteringComponent::GetMieScattering() const
{
  return m_MieScattering;
}

void plAtmosphericScatteringComponent::SetAbsorption(plColor Absorption)
{
  m_Absorption = Absorption;
  UpdateMaterials();
}
plColor plAtmosphericScatteringComponent::GetAbsorption() const
{
  return m_Absorption;
}

void plAtmosphericScatteringComponent::SetAmbientScattering(plColor AmbientScattering)
{
  m_AmbientScattering = AmbientScattering;
  UpdateMaterials();
}
plColor plAtmosphericScatteringComponent::GetAmbientScattering() const
{
  return m_AmbientScattering;
}

void plAtmosphericScatteringComponent::SetMieScatterDirection(float MieScatterDirection)
{
  m_fMieScatterDirection = MieScatterDirection;
  UpdateMaterials();
}
float plAtmosphericScatteringComponent::GetMieScatterDirection() const
{
  return m_fMieScatterDirection;
}

void plAtmosphericScatteringComponent::SetRayleighHeight(float fRayleighHeight)
{
  m_fRayleighHeight = fRayleighHeight;
  UpdateMaterials();
}
float plAtmosphericScatteringComponent::GetRayleighHeight() const
{
  return m_fRayleighHeight;
}

void plAtmosphericScatteringComponent::SetMieHeight(float fMieHeight)
{
  m_fMieHeight = fMieHeight;
  UpdateMaterials();
}
float plAtmosphericScatteringComponent::GetMieHeight() const
{
  return m_fMieHeight;
}

void plAtmosphericScatteringComponent::SetAbsorptionHeight(float fAbsorptionHeight)
{
  m_fAbsorptionHeight = fAbsorptionHeight;
  UpdateMaterials();
}
float plAtmosphericScatteringComponent::GetAbsorptionHeight() const
{
  return m_fAbsorptionHeight;
}

void plAtmosphericScatteringComponent::SetAbsorptionFalloff(float fAbsorptionFalloff)
{
  m_fAbsorptionFalloff = fAbsorptionFalloff;
  UpdateMaterials();
}
float plAtmosphericScatteringComponent::GetAbsorptionFalloff() const
{
  return m_fAbsorptionFalloff;
}

void plAtmosphericScatteringComponent::SetRaySteps(float fRaySteps)
{
  m_fRaySteps = fRaySteps;
  UpdateMaterials();
}
float plAtmosphericScatteringComponent::GetRaySteps() const
{
  return m_fRaySteps;
}

void plAtmosphericScatteringComponent::SetLightSteps(float fLightSteps)
{
  m_fLightSteps = fLightSteps;
  UpdateMaterials();
}
float plAtmosphericScatteringComponent::GetLightSteps() const
{
  return m_fLightSteps;
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_plAtmosphericScatteringComponent);