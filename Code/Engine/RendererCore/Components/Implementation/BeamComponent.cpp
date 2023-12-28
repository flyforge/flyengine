#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <RendererCore/Components/BeamComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererFoundation/Device/Device.h>


// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plBeamComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("TargetObject", DummyGetter, SetTargetObject)->AddAttributes(new plGameObjectReferenceAttribute()),
    PLASMA_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PLASMA_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new plDefaultValueAttribute(plColor::White)),
    PLASMA_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new plDefaultValueAttribute(0.1f), new plClampValueAttribute(0.001f, plVariant()), new plSuffixAttribute(" m")),
    PLASMA_ACCESSOR_PROPERTY("UVUnitsPerWorldUnit", GetUVUnitsPerWorldUnit, SetUVUnitsPerWorldUnit)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.01f, plVariant())),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBeamComponent::plBeamComponent() = default;

plBeamComponent::~plBeamComponent() = default;

void plBeamComponent::Update()
{
  plGameObject* pTargetObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pTargetObject))
  {
    plVec3 currentOwnerPosition = GetOwner()->GetGlobalPosition();
    plVec3 currentTargetPosition = pTargetObject->GetGlobalPosition();

    if (!pTargetObject->IsActive())
    {
      currentTargetPosition = currentOwnerPosition;
    }

    bool updateMesh = false;

    if ((currentOwnerPosition - m_vLastOwnerPosition).GetLengthSquared() > m_fDistanceUpdateEpsilon)
    {
      updateMesh = true;
      m_vLastOwnerPosition = currentOwnerPosition;
    }

    if ((currentTargetPosition - m_vLastTargetPosition).GetLengthSquared() > m_fDistanceUpdateEpsilon)
    {
      updateMesh = true;
      m_vLastTargetPosition = currentTargetPosition;
    }

    if (updateMesh)
    {
      ReinitMeshes();
    }
  }
  else
  {
    m_hMesh.Invalidate();
  }
}

void plBeamComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  inout_stream.WriteGameObjectHandle(m_hTargetObject);

  s << m_fWidth;
  s << m_fUVUnitsPerWorldUnit;
  s << m_hMaterial;
  s << m_Color;
}

void plBeamComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  m_hTargetObject = inout_stream.ReadGameObjectHandle();

  s >> m_fWidth;
  s >> m_fUVUnitsPerWorldUnit;
  s >> m_hMaterial;
  s >> m_Color;
}

plResult plBeamComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  plGameObject* pTargetObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pTargetObject))
  {
    const plVec3 currentTargetPosition = pTargetObject->GetGlobalPosition();
    const plVec3 targetPositionInOwnerSpace = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(currentTargetPosition);

    plVec3 pts[] = {plVec3::ZeroVector(), targetPositionInOwnerSpace};

    plBoundingBox box;
    box.SetFromPoints(pts, 2);
    const float fHalfWidth = m_fWidth * 0.5f;
    box.m_vMin -= plVec3(0, fHalfWidth, fHalfWidth);
    box.m_vMax += plVec3(0, fHalfWidth, fHalfWidth);
    ref_bounds = box;

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}


void plBeamComponent::OnActivated()
{
  SUPER::OnActivated();

  ReinitMeshes();
}

void plBeamComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  Cleanup();
}

void plBeamComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  const plUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const plUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  plMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_Color = m_Color;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  // Determine render data category.
  plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::AllowLoadingFallback);
  plRenderData::Category category = pMaterial->GetRenderDataCategory();


  msg.AddRenderData(pRenderData, category, plRenderData::Caching::Never);
}

void plBeamComponent::SetTargetObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hTargetObject = resolver(szReference, GetHandle(), "TargetObject");

  ReinitMeshes();
}

void plBeamComponent::SetWidth(float fWidth)
{
  if (fWidth <= 0.0f)
    return;

  m_fWidth = fWidth;

  ReinitMeshes();
}

float plBeamComponent::GetWidth() const
{
  return m_fWidth;
}

void plBeamComponent::SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit)
{
  if (fUVUnitsPerWorldUnit <= 0.0f)
    return;

  m_fUVUnitsPerWorldUnit = fUVUnitsPerWorldUnit;

  ReinitMeshes();
}

float plBeamComponent::GetUVUnitsPerWorldUnit() const
{
  return m_fUVUnitsPerWorldUnit;
}

void plBeamComponent::SetMaterialFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = plResourceManager::LoadResource<plMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* plBeamComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

plMaterialResourceHandle plBeamComponent::GetMaterial() const
{
  return m_hMaterial;
}

void plBeamComponent::CreateMeshes()
{
  plVec3 targetPositionInOwnerSpace = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(m_vLastTargetPosition);

  if (targetPositionInOwnerSpace.IsZero(0.01f))
    return;

  // Create the beam mesh name, it expresses the beam in local space with it's width
  // this way multiple beams in a corridor can share the same mesh for example.
  plStringBuilder meshName;
  meshName.Format("plBeamComponent_{0}_{1}_{2}_{3}.createdAtRuntime.plMesh", m_fWidth, plArgF(targetPositionInOwnerSpace.x, 2), plArgF(targetPositionInOwnerSpace.y, 2), plArgF(targetPositionInOwnerSpace.z, 2));

  m_hMesh = plResourceManager::GetExistingResource<plMeshResource>(meshName);

  // We build a cross mesh, thus we need the following vectors, x is the origin and we need to construct
  // the star points.
  //
  //  3        1
  //
  //      x
  //
  //  4        2
  plVec3 crossVector1 = (0.5f * plVec3::UnitYAxis() + 0.5f * plVec3::UnitZAxis());
  crossVector1.SetLength(m_fWidth * 0.5f).IgnoreResult();

  plVec3 crossVector2 = (0.5f * plVec3::UnitYAxis() - 0.5f * plVec3::UnitZAxis());
  crossVector2.SetLength(m_fWidth * 0.5f).IgnoreResult();

  plVec3 crossVector3 = (-0.5f * plVec3::UnitYAxis() + 0.5f * plVec3::UnitZAxis());
  crossVector3.SetLength(m_fWidth * 0.5f).IgnoreResult();

  plVec3 crossVector4 = (-0.5f * plVec3::UnitYAxis() - 0.5f * plVec3::UnitZAxis());
  crossVector4.SetLength(m_fWidth * 0.5f).IgnoreResult();

  const float fDistance = (m_vLastOwnerPosition - m_vLastTargetPosition).GetLength();



  // Build mesh if no existing one is found
  if (!m_hMesh.IsValid())
  {
    plGeometry g;

    // Quad 1
    {
      plUInt32 index0 = g.AddVertex(plVec3::ZeroVector() + crossVector1, plVec3::UnitXAxis(), plVec2(0, 0), plColor::White);
      plUInt32 index1 = g.AddVertex(plVec3::ZeroVector() + crossVector4, plVec3::UnitXAxis(), plVec2(0, 1), plColor::White);
      plUInt32 index2 = g.AddVertex(targetPositionInOwnerSpace + crossVector1, plVec3::UnitXAxis(), plVec2(fDistance * m_fUVUnitsPerWorldUnit, 0), plColor::White);
      plUInt32 index3 = g.AddVertex(targetPositionInOwnerSpace + crossVector4, plVec3::UnitXAxis(), plVec2(fDistance * m_fUVUnitsPerWorldUnit, 1), plColor::White);

      plUInt32 indices[] = {index0, index2, index3, index1};
      g.AddPolygon(plArrayPtr(indices), false);
      g.AddPolygon(plArrayPtr(indices), true);
    }

    // Quad 2
    {
      plUInt32 index0 = g.AddVertex(plVec3::ZeroVector() + crossVector2, plVec3::UnitXAxis(), plVec2(0, 0), plColor::White);
      plUInt32 index1 = g.AddVertex(plVec3::ZeroVector() + crossVector3, plVec3::UnitXAxis(), plVec2(0, 1), plColor::White);
      plUInt32 index2 = g.AddVertex(targetPositionInOwnerSpace + crossVector2, plVec3::UnitXAxis(), plVec2(fDistance * m_fUVUnitsPerWorldUnit, 0), plColor::White);
      plUInt32 index3 = g.AddVertex(targetPositionInOwnerSpace + crossVector3, plVec3::UnitXAxis(), plVec2(fDistance * m_fUVUnitsPerWorldUnit, 1), plColor::White);

      plUInt32 indices[] = {index0, index2, index3, index1};
      g.AddPolygon(plArrayPtr(indices), false);
      g.AddPolygon(plArrayPtr(indices), true);
    }

    g.ComputeTangents();

    plMeshResourceDescriptor desc;
    BuildMeshResourceFromGeometry(g, desc);

    m_hMesh = plResourceManager::CreateResource<plMeshResource>(meshName, std::move(desc));
  }
}

void plBeamComponent::BuildMeshResourceFromGeometry(plGeometry& Geometry, plMeshResourceDescriptor& MeshDesc) const
{
  auto& MeshBufferDesc = MeshDesc.MeshBufferDesc();

  MeshBufferDesc.AddCommonStreams();
  MeshBufferDesc.AllocateStreamsFromGeometry(Geometry, plGALPrimitiveTopology::Triangles);

  MeshDesc.AddSubMesh(MeshBufferDesc.GetPrimitiveCount(), 0, 0);

  MeshDesc.ComputeBounds();
}

void plBeamComponent::ReinitMeshes()
{
  Cleanup();

  if (IsActiveAndInitialized())
  {
    CreateMeshes();
    GetOwner()->UpdateLocalBounds();
  }
}

void plBeamComponent::Cleanup()
{
  m_hMesh.Invalidate();
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_BeamComponent);
