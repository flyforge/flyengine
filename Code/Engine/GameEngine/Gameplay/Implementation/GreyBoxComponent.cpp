#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plGreyBoxShape, 1)
  PL_ENUM_CONSTANTS(plGreyBoxShape::Box, plGreyBoxShape::RampX, plGreyBoxShape::RampY, plGreyBoxShape::Column)
  PL_ENUM_CONSTANTS(plGreyBoxShape::StairsX, plGreyBoxShape::StairsY, plGreyBoxShape::ArchX, plGreyBoxShape::ArchY, plGreyBoxShape::SpiralStairs)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_COMPONENT_TYPE(plGreyBoxComponent, 5, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_ACCESSOR_PROPERTY("Shape", plGreyBoxShape, GetShape, SetShape),
    PL_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PL_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new plDefaultValueAttribute(plColor::White), new plExposeColorAlphaAttribute()),
    PL_ACCESSOR_PROPERTY("SizeNegX", GetSizeNegX, SetSizeNegX)->AddAttributes(new plGroupAttribute("Size", "Size")),//->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("SizePosX", GetSizePosX, SetSizePosX),//->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("SizeNegY", GetSizeNegY, SetSizeNegY),//->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("SizePosY", GetSizePosY, SetSizePosY),//->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("SizeNegZ", GetSizeNegZ, SetSizeNegZ),//->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("SizePosZ", GetSizePosZ, SetSizePosZ),//->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new plGroupAttribute("Misc"), new plDefaultValueAttribute(16), new plClampValueAttribute(3, 32)),
    PL_ACCESSOR_PROPERTY("Curvature", GetCurvature, SetCurvature)->AddAttributes(new plClampValueAttribute(plAngle::MakeFromDegree(-360), plAngle::MakeFromDegree(360))),
    PL_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("SlopedTop", GetSlopedTop, SetSlopedTop),
    PL_ACCESSOR_PROPERTY("SlopedBottom", GetSlopedBottom, SetSlopedBottom),
    PL_ACCESSOR_PROPERTY("GenerateCollision", GetGenerateCollision, SetGenerateCollision)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_ACCESSOR_PROPERTY("IncludeInNavmesh", GetIncludeInNavmesh, SetIncludeInNavmesh)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("UseAsOccluder", m_bUseAsOccluder)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Construction"),
    new plNonUniformBoxManipulatorAttribute("SizeNegX", "SizePosX", "SizeNegY", "SizePosY", "SizeNegZ", "SizePosZ"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgBuildStaticMesh, OnBuildStaticMesh),
    PL_MESSAGE_HANDLER(plMsgExtractGeometry, OnMsgExtractGeometry),
    PL_MESSAGE_HANDLER(plMsgExtractOccluderData, OnMsgExtractOccluderData),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_COMPONENT_TYPE;
// clang-format on

plGreyBoxComponent::plGreyBoxComponent() = default;
plGreyBoxComponent::~plGreyBoxComponent() = default;

void plGreyBoxComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_Shape;
  s << m_hMaterial;
  s << m_fSizeNegX;
  s << m_fSizePosX;
  s << m_fSizeNegY;
  s << m_fSizePosY;
  s << m_fSizeNegZ;
  s << m_fSizePosZ;
  s << m_uiDetail;

  // Version 2
  s << m_Curvature;
  s << m_fThickness;
  s << m_bSlopedTop;
  s << m_bSlopedBottom;

  // Version 3
  s << m_Color;

  // Version 4
  s << m_bGenerateCollision;
  s << m_bIncludeInNavmesh;

  // Version 5
  s << m_bUseAsOccluder;
}

void plGreyBoxComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_Shape;
  s >> m_hMaterial;
  s >> m_fSizeNegX;
  s >> m_fSizePosX;
  s >> m_fSizeNegY;
  s >> m_fSizePosY;
  s >> m_fSizeNegZ;
  s >> m_fSizePosZ;
  s >> m_uiDetail;

  if (uiVersion >= 2)
  {
    s >> m_Curvature;
    s >> m_fThickness;
    s >> m_bSlopedTop;
    s >> m_bSlopedBottom;
  }

  if (uiVersion >= 3)
  {
    s >> m_Color;
  }

  if (uiVersion >= 4)
  {
    s >> m_bGenerateCollision;
    s >> m_bIncludeInNavmesh;
  }

  if (uiVersion >= 5)
  {
    s >> m_bUseAsOccluder;
  }
}

void plGreyBoxComponent::OnActivated()
{
  if (!m_hMesh.IsValid())
  {
    m_hMesh = GenerateMesh<plMeshResource>();
  }

  // First generate the mesh and then call the base implementation which will update the bounds
  SUPER::OnActivated();
}

plResult plGreyBoxComponent::GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg)
{
  if (m_hMesh.IsValid())
  {
    plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::AllowLoadingFallback);
    bounds = pMesh->GetBounds();

    if (m_bUseAsOccluder)
    {
      msg.AddBounds(bounds, GetOwner()->IsStatic() ? plDefaultSpatialDataCategories::OcclusionStatic : plDefaultSpatialDataCategories::OcclusionDynamic);
    }

    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

void plGreyBoxComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const plUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const plUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::AllowLoadingFallback);
  plArrayPtr<const plMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (plUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const plUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    plMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[uiMaterialIndex];

    plMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plMeshRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_Color = m_Color;

      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiFlipWinding = uiFlipWinding;
      pRenderData->m_uiUniformScale = uiUniformScale;

      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    bool bDontCacheYet = false;

    // Determine render data category.
    plRenderData::Category category = plDefaultRenderDataCategories::LitOpaque;

    if (hMaterial.IsValid())
    {
      plResourceLock<plMaterialResource> pMaterial(hMaterial, plResourceAcquireMode::AllowLoadingFallback);

      if (pMaterial.GetAcquireResult() == plResourceAcquireResult::LoadingFallback)
        bDontCacheYet = true;

      category = pMaterial->GetRenderDataCategory();
    }

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? plRenderData::Caching::Never : plRenderData::Caching::IfStatic);
  }
}

void plGreyBoxComponent::SetShape(plEnum<plGreyBoxShape> shape)
{
  m_Shape = shape;
  InvalidateMesh();
}

void plGreyBoxComponent::SetMaterialFile(const char* szFile)
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

const char* plGreyBoxComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void plGreyBoxComponent::SetSizeNegX(float f)
{
  m_fSizeNegX = f;
  InvalidateMesh();
}

void plGreyBoxComponent::SetSizePosX(float f)
{
  m_fSizePosX = f;
  InvalidateMesh();
}

void plGreyBoxComponent::SetSizeNegY(float f)
{
  m_fSizeNegY = f;
  InvalidateMesh();
}

void plGreyBoxComponent::SetSizePosY(float f)
{
  m_fSizePosY = f;
  InvalidateMesh();
}

void plGreyBoxComponent::SetSizeNegZ(float f)
{
  m_fSizeNegZ = f;
  InvalidateMesh();
}

void plGreyBoxComponent::SetSizePosZ(float f)
{
  m_fSizePosZ = f;
  InvalidateMesh();
}

void plGreyBoxComponent::SetDetail(plUInt32 uiDetail)
{
  m_uiDetail = uiDetail;
  InvalidateMesh();
}

void plGreyBoxComponent::SetCurvature(plAngle curvature)
{
  m_Curvature = plAngle::MakeFromDegree(plMath::RoundToMultiple(curvature.GetDegree(), 5.0f));
  InvalidateMesh();
}

void plGreyBoxComponent::SetSlopedTop(bool b)
{
  m_bSlopedTop = b;
  InvalidateMesh();
}

void plGreyBoxComponent::SetSlopedBottom(bool b)
{
  m_bSlopedBottom = b;
  InvalidateMesh();
}

void plGreyBoxComponent::SetThickness(float f)
{
  m_fThickness = f;
  InvalidateMesh();
}

void plGreyBoxComponent::SetGenerateCollision(bool b)
{
  m_bGenerateCollision = b;
}

void plGreyBoxComponent::SetIncludeInNavmesh(bool b)
{
  m_bIncludeInNavmesh = b;
}

void plGreyBoxComponent::OnBuildStaticMesh(plMsgBuildStaticMesh& msg) const
{
  if (!m_bGenerateCollision)
    return;

  plGeometry geom;
  BuildGeometry(geom, m_Shape, false);
  geom.TriangulatePolygons();

  auto* pDesc = msg.m_pStaticMeshDescription;
  auto& subMesh = pDesc->m_SubMeshes.ExpandAndGetRef();
  subMesh.m_uiFirstTriangle = pDesc->m_Triangles.GetCount();

  const plTransform t = GetOwner()->GetGlobalTransform();

  const plUInt32 uiTriOffset = pDesc->m_Vertices.GetCount();

  for (const auto& verts : geom.GetVertices())
  {
    pDesc->m_Vertices.PushBack(t * verts.m_vPosition);
  }

  for (const auto& polys : geom.GetPolygons())
  {
    auto& tri = pDesc->m_Triangles.ExpandAndGetRef();
    tri.m_uiVertexIndices[0] = uiTriOffset + polys.m_Vertices[0];
    tri.m_uiVertexIndices[1] = uiTriOffset + polys.m_Vertices[1];
    tri.m_uiVertexIndices[2] = uiTriOffset + polys.m_Vertices[2];
  }

  subMesh.m_uiNumTriangles = pDesc->m_Triangles.GetCount() - subMesh.m_uiFirstTriangle;

  plMaterialResourceHandle hMaterial = m_hMaterial;
  if (!hMaterial.IsValid())
  {
    // Data/Base/Materials/Common/Pattern.plMaterialAsset
    hMaterial = plResourceManager::LoadResource<plMaterialResource>("{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");
  }

  if (hMaterial.IsValid())
  {
    plResourceLock<plMaterialResource> pMaterial(hMaterial, plResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pMaterial.GetAcquireResult() == plResourceAcquireResult::Final)
    {
      const plString surface = pMaterial->GetSurface().GetString();

      if (!surface.IsEmpty())
      {
        plUInt32 idx = pDesc->m_Surfaces.IndexOf(surface);
        if (idx == plInvalidIndex)
        {
          idx = pDesc->m_Surfaces.GetCount();
          pDesc->m_Surfaces.PushBack(surface);
        }

        subMesh.m_uiSurfaceIndex = static_cast<plUInt16>(idx);
      }
    }
  }
}

void plGreyBoxComponent::OnMsgExtractGeometry(plMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == plWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && (m_bGenerateCollision == false || GetOwner()->IsDynamic()))
    return;

  if (msg.m_Mode == plWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration && (m_bIncludeInNavmesh == false || GetOwner()->IsDynamic()))
    return;

  msg.AddMeshObject(GetOwner()->GetGlobalTransform(), GenerateMesh<plCpuMeshResource>());
}

void plGreyBoxComponent::OnMsgExtractOccluderData(plMsgExtractOccluderData& msg) const
{
  if (!IsActiveAndInitialized() || !m_bUseAsOccluder)
    return;

  if (m_pOccluderObject == nullptr)
  {
    plEnum<plGreyBoxShape> shape = m_Shape;
    if (shape == plGreyBoxShape::StairsX && m_Curvature == plAngle())
      shape = plGreyBoxShape::RampX;
    if (shape == plGreyBoxShape::StairsY && m_Curvature == plAngle())
      shape = plGreyBoxShape::RampY;

    plStringBuilder sResourceName;
    GenerateMeshName(sResourceName);

    m_pOccluderObject = plRasterizerObject::GetObject(sResourceName);

    if (m_pOccluderObject == nullptr)
    {
      plGeometry geom;
      BuildGeometry(geom, shape, true);

      m_pOccluderObject = plRasterizerObject::CreateMesh(sResourceName, geom);
    }
  }

  msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
}

void plGreyBoxComponent::InvalidateMesh()
{
  m_pOccluderObject = nullptr;

  if (m_hMesh.IsValid())
  {
    m_hMesh.Invalidate();

    m_hMesh = GenerateMesh<plMeshResource>();

    TriggerLocalBoundsUpdate();
  }
}

void plGreyBoxComponent::BuildGeometry(plGeometry& geom, plEnum<plGreyBoxShape> shape, bool bOnlyRoughDetails) const
{
  plGeometry::GeoOptions opt;
  opt.m_Color = m_Color;

  plVec3 size;
  size.x = m_fSizeNegX + m_fSizePosX;
  size.y = m_fSizeNegY + m_fSizePosY;
  size.z = m_fSizeNegZ + m_fSizePosZ;

  if (size.x == 0 || size.y == 0 || size.z == 0)
  {
    // create a tiny dummy box, so that we have valid geometry
    geom.AddBox(plVec3(0.01f), true, opt);
    return;
  }

  plVec3 offset(0);
  offset.x = (m_fSizePosX - m_fSizeNegX) * 0.5f;
  offset.y = (m_fSizePosY - m_fSizeNegY) * 0.5f;
  offset.z = (m_fSizePosZ - m_fSizeNegZ) * 0.5f;

  plMat4 t2, t3;

  opt.m_Transform = plMat4::MakeTranslation(offset);

  switch (shape)
  {
    case plGreyBoxShape::Box:
      geom.AddBox(size, true, opt);
      break;

    case plGreyBoxShape::RampX:
      geom.AddTexturedRamp(size, opt);
      break;

    case plGreyBoxShape::RampY:
      plMath::Swap(size.x, size.y);
      opt.m_Transform = plMat4::MakeRotationZ(plAngle::MakeFromDegree(-90.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddTexturedRamp(size, opt);
      break;

    case plGreyBoxShape::Column:
      opt.m_Transform.SetScalingFactors(size).IgnoreResult();
      geom.AddCylinder(0.5f, 0.5f, 0.5f, 0.5f, true, true, plMath::Min<plUInt16>(bOnlyRoughDetails ? 14 : 32, static_cast<plUInt16>(m_uiDetail)), opt);
      break;

    case plGreyBoxShape::StairsX:
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, opt);
      break;

    case plGreyBoxShape::StairsY:
      plMath::Swap(size.x, size.y);
      opt.m_Transform = plMat4::MakeRotationZ(plAngle::MakeFromDegree(-90.0f));
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddStairs(size, m_uiDetail, m_Curvature, m_bSlopedTop, opt);
      break;

    case plGreyBoxShape::ArchX:
    {
      const float tmp = size.z;
      size.z = size.x;
      size.x = size.y;
      size.y = tmp;
      opt.m_Transform = plMat4::MakeRotationY(plAngle::MakeFromDegree(-90));
      t2 = plMat4::MakeRotationX(plAngle::MakeFromDegree(90));
      opt.m_Transform = t2 * opt.m_Transform;
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, false, false, false, !bOnlyRoughDetails, opt);
    }
    break;

    case plGreyBoxShape::ArchY:
    {
      opt.m_Transform = plMat4::MakeRotationY(plAngle::MakeFromDegree(-90));
      t2 = plMat4::MakeRotationX(plAngle::MakeFromDegree(90));
      t3 = plMat4::MakeRotationZ(plAngle::MakeFromDegree(90));
      plMath::Swap(size.y, size.z);
      opt.m_Transform = t3 * t2 * opt.m_Transform;
      opt.m_Transform.SetTranslationVector(offset);
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, false, false, false, !bOnlyRoughDetails, opt);
    }
    break;

    case plGreyBoxShape::SpiralStairs:
      geom.AddArch(size, m_uiDetail, m_fThickness, m_Curvature, true, m_bSlopedBottom, m_bSlopedTop, true, opt);
      break;

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }
}

void plGreyBoxComponent::GenerateMeshName(plStringBuilder& out_sName) const
{
  switch (m_Shape)
  {
    case plGreyBoxShape::Box:
      out_sName.SetFormat("Grey-Box:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case plGreyBoxShape::RampX:
      out_sName.SetFormat("Grey-RampX:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case plGreyBoxShape::RampY:
      out_sName.SetFormat("Grey-RampY:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
      break;

    case plGreyBoxShape::Column:
      out_sName.SetFormat("Grey-Column:{0}-{1},{2}-{3},{4}-{5}-d{6}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail);
      break;

    case plGreyBoxShape::StairsX:
      out_sName.SetFormat("Grey-StairsX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case plGreyBoxShape::StairsY:
      out_sName.SetFormat("Grey-StairsY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-st{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_bSlopedTop);
      break;

    case plGreyBoxShape::ArchX:
      out_sName.SetFormat("Grey-ArchX:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
      break;

    case plGreyBoxShape::ArchY:
      out_sName.SetFormat("Grey-ArchY:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness);
      break;

    case plGreyBoxShape::SpiralStairs:
      out_sName.SetFormat("Grey-Spiral:{0}-{1},{2}-{3},{4}-{5}-d{6}-c{7}-t{8}-st{9}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ, m_uiDetail, m_Curvature.GetDegree(), m_fThickness, m_bSlopedTop);
      out_sName.AppendFormat("-sb{0}", m_bSlopedBottom);
      break;


    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }
}

void plGreyBoxComponent::GenerateMeshResourceDescriptor(plMeshResourceDescriptor& desc) const
{
  plGeometry geom;
  BuildGeometry(geom, m_Shape, false);

  bool bInvertedGeo = false;

  if (-m_fSizeNegX > m_fSizePosX)
    bInvertedGeo = !bInvertedGeo;
  if (-m_fSizeNegY > m_fSizePosY)
    bInvertedGeo = !bInvertedGeo;
  if (-m_fSizeNegZ > m_fSizePosZ)
    bInvertedGeo = !bInvertedGeo;

  if (bInvertedGeo)
  {
    for (auto vert : geom.GetVertices())
    {
      vert.m_vNormal = -vert.m_vNormal;
    }
  }

  geom.TriangulatePolygons();
  geom.ComputeTangents();

  // Data/Base/Materials/Common/Pattern.plMaterialAsset
  desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");

  desc.MeshBufferDesc().AddCommonStreams();
  desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();
}

template <typename ResourceType>
plTypedResourceHandle<ResourceType> plGreyBoxComponent::GenerateMesh() const
{
  plStringBuilder sResourceName;
  GenerateMeshName(sResourceName);

  plTypedResourceHandle<ResourceType> hResource = plResourceManager::GetExistingResource<ResourceType>(sResourceName);
  if (hResource.IsValid())
    return hResource;

  plMeshResourceDescriptor desc;
  GenerateMeshResourceDescriptor(desc);

  return plResourceManager::GetOrCreateResource<ResourceType>(sResourceName, std::move(desc), sResourceName);
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GreyBoxComponent);
