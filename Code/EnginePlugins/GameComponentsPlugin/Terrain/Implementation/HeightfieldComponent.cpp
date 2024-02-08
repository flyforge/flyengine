#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Terrain/HeightfieldComponent.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plHeightfieldComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("HeightfieldImage", GetHeightfieldFile, SetHeightfieldFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_2D")),
    PL_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PL_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new plDefaultValueAttribute(plVec2(50))),
    PL_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new plDefaultValueAttribute(50)),
    PL_ACCESSOR_PROPERTY("Tesselation", GetTesselation, SetTesselation)->AddAttributes(new plDefaultValueAttribute(plVec2U32(128))),
    PL_ACCESSOR_PROPERTY("TexCoordOffset", GetTexCoordOffset, SetTexCoordOffset)->AddAttributes(new plDefaultValueAttribute(plVec2(0))),
    PL_ACCESSOR_PROPERTY("TexCoordScale", GetTexCoordScale, SetTexCoordScale)->AddAttributes(new plDefaultValueAttribute(plVec2(1))),
    PL_ACCESSOR_PROPERTY("GenerateCollision", GetGenerateCollision, SetGenerateCollision)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_ACCESSOR_PROPERTY("ColMeshTesselation", GetColMeshTesselation, SetColMeshTesselation)->AddAttributes(new plDefaultValueAttribute(plVec2U32(64))),
    PL_ACCESSOR_PROPERTY("IncludeInNavmesh", GetIncludeInNavmesh, SetIncludeInNavmesh)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Terrain"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgBuildStaticMesh, OnBuildStaticMesh),
    PL_MESSAGE_HANDLER(plMsgExtractGeometry, OnMsgExtractGeometry),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_COMPONENT_TYPE;
// clang-format on

plHeightfieldComponent::plHeightfieldComponent() = default;
plHeightfieldComponent::~plHeightfieldComponent() = default;

void plHeightfieldComponent::SetHalfExtents(plVec2 value)
{
  m_vHalfExtents = value;
  InvalidateMesh();
}

void plHeightfieldComponent::SetHeight(float value)
{
  m_fHeight = value;
  InvalidateMesh();
}

void plHeightfieldComponent::SetTexCoordOffset(plVec2 value)
{
  m_vTexCoordOffset = value;
  InvalidateMesh();
}

void plHeightfieldComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  plStreamWriter& s = stream.GetStream();

  s << m_hHeightfield;
  s << m_hMaterial;
  s << m_vHalfExtents;
  s << m_fHeight;
  s << m_vTexCoordOffset;
  s << m_vTexCoordScale;
  s << m_vTesselation;
  s << m_vColMeshTesselation;

  // Version 2
  s << m_bGenerateCollision;
  s << m_bIncludeInNavmesh;
}

void plHeightfieldComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_hHeightfield;
  s >> m_hMaterial;
  s >> m_vHalfExtents;
  s >> m_fHeight;
  s >> m_vTexCoordOffset;
  s >> m_vTexCoordScale;
  s >> m_vTesselation;
  s >> m_vColMeshTesselation;

  if (uiVersion >= 2)
  {
    s >> m_bGenerateCollision;
    s >> m_bIncludeInNavmesh;
  }
}

void plHeightfieldComponent::OnActivated()
{
  if (!m_hMesh.IsValid())
  {
    m_hMesh = GenerateMesh<plMeshResource>();
  }

  // First generate the mesh and then call the base implementation which will update the bounds
  SUPER::OnActivated();
}

plResult plHeightfieldComponent::GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg)
{
  if (m_hMesh.IsValid())
  {
    plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::AllowLoadingFallback);
    bounds = pMesh->GetBounds();
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

void plHeightfieldComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
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
      pRenderData->m_LastGlobalTransform = GetOwner()->GetLastGlobalTransform();
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_Color = plColor::White;

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

void plHeightfieldComponent::SetTexCoordScale(plVec2 value) // [ property ]
{
  m_vTexCoordScale = value;
  InvalidateMesh();
}

void plHeightfieldComponent::SetMaterialFile(const char* szFile)
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

const char* plHeightfieldComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}


void plHeightfieldComponent::SetHeightfieldFile(const char* szFile)
{
  plImageDataResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plImageDataResource>(szFile);
  }

  SetHeightfield(hResource);
}

const char* plHeightfieldComponent::GetHeightfieldFile() const
{
  if (!m_hHeightfield.IsValid())
    return "";

  return m_hHeightfield.GetResourceID();
}

void plHeightfieldComponent::SetHeightfield(const plImageDataResourceHandle& hResource)
{
  m_hHeightfield = hResource;
  InvalidateMesh();
}

void plHeightfieldComponent::SetTesselation(plVec2U32 value)
{
  m_vTesselation = value;
  InvalidateMesh();
}

void plHeightfieldComponent::SetGenerateCollision(bool b)
{
  m_bGenerateCollision = b;
}

void plHeightfieldComponent::SetColMeshTesselation(plVec2U32 value)
{
  m_vColMeshTesselation = value;
  // don't invalidate the render mesh
}

void plHeightfieldComponent::SetIncludeInNavmesh(bool b)
{
  m_bIncludeInNavmesh = b;
}

void plHeightfieldComponent::OnBuildStaticMesh(plMsgBuildStaticMesh& msg) const
{
  if (!m_bGenerateCollision)
    return;

  plGeometry geom;
  BuildGeometry(geom);

  auto* pDesc = msg.m_pStaticMeshDescription;
  auto& subMesh = pDesc->m_SubMeshes.ExpandAndGetRef();
  subMesh.m_uiFirstTriangle = pDesc->m_Triangles.GetCount();

  const plTransform trans = GetOwner()->GetGlobalTransform();

  const plUInt32 uiTriOffset = pDesc->m_Vertices.GetCount();

  for (const auto& verts : geom.GetVertices())
  {
    pDesc->m_Vertices.PushBack(trans * verts.m_vPosition);
  }

  for (const auto& polys : geom.GetPolygons())
  {
    for (plUInt32 t = 0; t < polys.m_Vertices.GetCount() - 2; ++t)
    {
      auto& tri = pDesc->m_Triangles.ExpandAndGetRef();
      tri.m_uiVertexIndices[0] = uiTriOffset + polys.m_Vertices[0];
      tri.m_uiVertexIndices[1] = uiTriOffset + polys.m_Vertices[t + 1];
      tri.m_uiVertexIndices[2] = uiTriOffset + polys.m_Vertices[t + 2];
    }
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

void plHeightfieldComponent::OnMsgExtractGeometry(plMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == plWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && (m_bGenerateCollision == false || GetOwner()->IsDynamic()))
    return;

  if (msg.m_Mode == plWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration && (m_bIncludeInNavmesh == false || GetOwner()->IsDynamic()))
    return;

  msg.AddMeshObject(GetOwner()->GetGlobalTransform(), GenerateMesh<plCpuMeshResource>());
}

void plHeightfieldComponent::InvalidateMesh()
{
  if (m_hMesh.IsValid())
  {
    m_hMesh.Invalidate();

    m_hMesh = GenerateMesh<plMeshResource>();

    TriggerLocalBoundsUpdate();
  }
}

void plHeightfieldComponent::BuildGeometry(plGeometry& geom) const
{
  if (!m_hHeightfield.IsValid())
    return;

  PL_PROFILE_SCOPE("Heightfield: BuildGeometry");

  plResourceLock<plImageDataResource> pImageData(m_hHeightfield, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pImageData.GetAcquireResult() != plResourceAcquireResult::Final)
  {
    plLog::Error("Failed to load heightmap image data '{}'", m_hHeightfield.GetResourceID());
    return;
  }

  const plImage& heightmap = pImageData->GetDescriptor().m_Image;

  const plUInt32 uiNumVerticesX = plMath::Clamp(m_vColMeshTesselation.x + 1u, 5u, 512u);
  const plUInt32 uiNumVerticesY = plMath::Clamp(m_vColMeshTesselation.y + 1u, 5u, 512u);

  const plVec3 vSize(m_vHalfExtents.x * 2, m_vHalfExtents.y * 2, m_fHeight);
  const plVec2 vToNDC = plVec2(1.0f / (uiNumVerticesX - 1), 1.0f / (uiNumVerticesY - 1));
  const plVec3 vPosOffset(-m_vHalfExtents.x, -m_vHalfExtents.y, 0);

  const plColor* pImgData = heightmap.GetPixelPointer<plColor>();
  const plUInt32 imgWidth = heightmap.GetWidth();
  const plUInt32 imgHeight = heightmap.GetHeight();

  for (plUInt32 y = 0; y < uiNumVerticesY; ++y)
  {
    for (plUInt32 x = 0; x < uiNumVerticesX; ++x)
    {
      const plVec2 ndc = plVec2((float)x, (float)y).CompMul(vToNDC);
      const plVec2 tc = m_vTexCoordOffset + ndc.CompMul(m_vTexCoordScale);
      const plVec2 heightTC = ndc;

      const float fHeightScale = 1.0f - plImageUtils::BilinearSample(pImgData, imgWidth, imgHeight, plImageAddressMode::Clamp, heightTC).r;

      const plVec3 vNewPos = vPosOffset + plVec3(ndc.x, ndc.y, -fHeightScale).CompMul(vSize);

      geom.AddVertex(vNewPos, plVec3(0, 0, 1), tc, plColor::White);
    }
  }

  plUInt32 uiVertexIdx = 0;

  for (plUInt32 y = 0; y < uiNumVerticesY - 1; ++y)
  {
    for (plUInt32 x = 0; x < uiNumVerticesX - 1; ++x)
    {
      plUInt32 indices[4];
      indices[0] = uiVertexIdx;
      indices[1] = uiVertexIdx + 1;
      indices[2] = uiVertexIdx + uiNumVerticesX + 1;
      indices[3] = uiVertexIdx + uiNumVerticesX;

      geom.AddPolygon(indices, false);

      ++uiVertexIdx;
    }

    ++uiVertexIdx;
  }
}

plResult plHeightfieldComponent::BuildMeshDescriptor(plMeshResourceDescriptor& desc) const
{
  PL_PROFILE_SCOPE("Heightfield: GenerateRenderMesh");

  plResourceLock<plImageDataResource> pImageData(m_hHeightfield, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pImageData.GetAcquireResult() != plResourceAcquireResult::Final)
  {
    plLog::Error("Failed to load heightmap image data '{}'", m_hHeightfield.GetResourceID());
    return PL_FAILURE;
  }

  const plImage& heightmap = pImageData->GetDescriptor().m_Image;

  // Data/Base/Materials/Common/Pattern.plMaterialAsset
  desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");

  desc.MeshBufferDesc().AddCommonStreams();
  // 0 = position
  // 1 = texcoord
  // 2 = normal
  // 3 = tangent

  {
    auto& mb = desc.MeshBufferDesc();

    const plUInt32 uiNumVerticesX = plMath::Clamp(m_vTesselation.x + 1u, 5u, 1024u);
    const plUInt32 uiNumVerticesY = plMath::Clamp(m_vTesselation.y + 1u, 5u, 1024u);
    const plUInt32 uiNumTriangles = (uiNumVerticesX - 1) * (uiNumVerticesY - 1) * 2;

    mb.AllocateStreams(uiNumVerticesX * uiNumVerticesY, plGALPrimitiveTopology::Triangles, uiNumTriangles);

    const plVec3 vSize(m_vHalfExtents.x * 2, m_vHalfExtents.y * 2, m_fHeight);
    const plVec2 vToNDC = plVec2(1.0f / (uiNumVerticesX - 1), 1.0f / (uiNumVerticesY - 1));
    const plVec3 vPosOffset(-m_vHalfExtents.x, -m_vHalfExtents.y, -m_fHeight);

    const auto texCoordFormat = plMeshTexCoordPrecision::ToResourceFormat(plMeshTexCoordPrecision::Default);
    const auto normalFormat = plMeshNormalPrecision::ToResourceFormatNormal(plMeshNormalPrecision::Default);
    const auto tangentFormat = plMeshNormalPrecision::ToResourceFormatTangent(plMeshNormalPrecision::Default);

    // access the vertex data directly
    // this is way more complicated than going through SetVertexData, but it is ~20% faster

    auto positionData = mb.GetVertexData(0, 0);
    auto texcoordData = mb.GetVertexData(1, 0);
    auto normalData = mb.GetVertexData(2, 0);
    auto tangentData = mb.GetVertexData(3, 0);

    const plUInt32 uiVertexDataSize = mb.GetVertexDataSize();

    plUInt32 uiVertexIdx = 0;

    const plColor* pImgData = heightmap.GetPixelPointer<plColor>();
    const plUInt32 imgWidth = heightmap.GetWidth();
    const plUInt32 imgHeight = heightmap.GetHeight();

    for (plUInt32 y = 0; y < uiNumVerticesY; ++y)
    {
      for (plUInt32 x = 0; x < uiNumVerticesX; ++x)
      {
        const plVec2 ndc = plVec2((float)x, (float)y).CompMul(vToNDC);
        const plVec2 tc = m_vTexCoordOffset + ndc.CompMul(m_vTexCoordScale);
        const plVec2 heightTC = ndc;

        const size_t uiByteOffset = (size_t)uiVertexIdx * (size_t)uiVertexDataSize;

        const float fHeightScale = plImageUtils::BilinearSample(pImgData, imgWidth, imgHeight, plImageAddressMode::Clamp, heightTC).r;

        // complicated but faster
        *reinterpret_cast<plVec3*>(positionData.GetPtr() + uiByteOffset) = vPosOffset + plVec3(ndc.x, ndc.y, fHeightScale).CompMul(vSize);
        plMeshBufferUtils::EncodeTexCoord(tc, plByteArrayPtr(texcoordData.GetPtr() + uiByteOffset, 32), texCoordFormat).IgnoreResult();

        // easier to understand, but slower
        // mb.SetVertexData(0, uiVertexIdx, vPosOffset + plVec3(ndc.x, ndc.y, -fHeightScale).CompMul(vSize));
        // plMeshBufferUtils::EncodeTexCoord(tc, mb.GetVertexData(1, uiVertexIdx), texCoordFormat).IgnoreResult();

        ++uiVertexIdx;
      }
    }

    uiVertexIdx = 0;

    for (plUInt32 y = 0; y < uiNumVerticesY; ++y)
    {
      for (plUInt32 x = 0; x < uiNumVerticesX; ++x)
      {
        const size_t uiByteOffset = (size_t)uiVertexIdx * (size_t)uiVertexDataSize;

        const plInt32 centerIDx = uiVertexIdx;
        plInt32 leftIDx = uiVertexIdx - 1;
        plInt32 rightIDx = uiVertexIdx + 1;
        plInt32 bottomIDx = uiVertexIdx - uiNumVerticesX;
        plInt32 topIDx = uiVertexIdx + uiNumVerticesX;

        // clamp the indices
        if (x == 0)
          leftIDx = centerIDx;
        if (x + 1 == uiNumVerticesX)
          rightIDx = centerIDx;
        if (y == 0)
          bottomIDx = centerIDx;
        if (y + 1 == uiNumVerticesY)
          topIDx = centerIDx;

        const plVec3 vPosCenter = *reinterpret_cast<plVec3*>(positionData.GetPtr() + (size_t)centerIDx * (size_t)uiVertexDataSize);
        const plVec3 vPosLeft = *reinterpret_cast<plVec3*>(positionData.GetPtr() + (size_t)leftIDx * (size_t)uiVertexDataSize);
        const plVec3 vPosRight = *reinterpret_cast<plVec3*>(positionData.GetPtr() + (size_t)rightIDx * (size_t)uiVertexDataSize);
        const plVec3 vPosBottom = *reinterpret_cast<plVec3*>(positionData.GetPtr() + (size_t)bottomIDx * (size_t)uiVertexDataSize);
        const plVec3 vPosTop = *reinterpret_cast<plVec3*>(positionData.GetPtr() + (size_t)topIDx * (size_t)uiVertexDataSize);

        plVec3 edgeL = vPosLeft - vPosCenter;
        plVec3 edgeR = vPosRight - vPosCenter;
        plVec3 edgeB = vPosBottom - vPosCenter;
        plVec3 edgeT = vPosTop - vPosCenter;

        // rotate edges by 90 degrees, so that they become normals
        plMath::Swap(edgeL.x, edgeL.z);
        plMath::Swap(edgeR.x, edgeR.z);
        plMath::Swap(edgeB.y, edgeB.z);
        plMath::Swap(edgeT.y, edgeT.z);

        edgeL.z = -edgeL.z;
        edgeR.x = -edgeR.x;
        edgeB.z = -edgeB.z;
        edgeT.y = -edgeT.y;

        // don't normalize the edges first, if they are longer, they shall have more influence
        plVec3 vNormal(0);
        vNormal += edgeL;
        vNormal += edgeR;
        vNormal += edgeB;
        vNormal += edgeT;
        vNormal.Normalize();

        plVec3 vTangent = plVec3(1, 0, 0).CrossRH(vNormal).GetNormalized();

        // complicated but faster
        plMeshBufferUtils::EncodeNormal(vNormal, plByteArrayPtr(normalData.GetPtr() + uiByteOffset, 32), normalFormat).IgnoreResult();
        plMeshBufferUtils::EncodeTangent(vTangent, 1.0f, plByteArrayPtr(tangentData.GetPtr() + uiByteOffset, 32), tangentFormat).IgnoreResult();

        // easier to understand, but slower
        // plMeshBufferUtils::EncodeNormal(plVec3(0, 0, 1), mb.GetVertexData(2, uiVertexIdx), normalFormat).IgnoreResult();
        // plMeshBufferUtils::EncodeTangent(plVec3(1, 0, 0), 1.0f, mb.GetVertexData(3, uiVertexIdx), tangentFormat).IgnoreResult();

        ++uiVertexIdx;
      }
    }

    desc.SetBounds(plBoundingBoxSphere::MakeFromBox(plBoundingBox::MakeFromMinMax(vPosOffset, vPosOffset + vSize)));

    plUInt32 uiTriangleIdx = 0;
    uiVertexIdx = 0;

    for (plUInt32 y = 0; y < uiNumVerticesY - 1; ++y)
    {
      for (plUInt32 x = 0; x < uiNumVerticesX - 1; ++x)
      {
        mb.SetTriangleIndices(uiTriangleIdx + 0, uiVertexIdx, uiVertexIdx + 1, uiVertexIdx + uiNumVerticesX);
        mb.SetTriangleIndices(uiTriangleIdx + 1, uiVertexIdx + 1, uiVertexIdx + uiNumVerticesX + 1, uiVertexIdx + uiNumVerticesX);
        uiTriangleIdx += 2;

        ++uiVertexIdx;
      }

      ++uiVertexIdx;
    }
  }

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);
  return PL_SUCCESS;
}

template <typename ResourceType>
plTypedResourceHandle<ResourceType> plHeightfieldComponent::GenerateMesh() const
{
  if (!m_hHeightfield.IsValid())
    return plTypedResourceHandle<ResourceType>();

  plStringBuilder sResourceName;

  {
    plUInt64 uiSettingsHash = m_hHeightfield.GetResourceIDHash() + m_uiHeightfieldChangeCounter;
    uiSettingsHash = plHashingUtils::xxHash64(&m_vHalfExtents, sizeof(m_vHalfExtents), uiSettingsHash);
    uiSettingsHash = plHashingUtils::xxHash64(&m_fHeight, sizeof(m_fHeight), uiSettingsHash);
    uiSettingsHash = plHashingUtils::xxHash64(&m_vTexCoordOffset, sizeof(m_vTexCoordOffset), uiSettingsHash);
    uiSettingsHash = plHashingUtils::xxHash64(&m_vTexCoordScale, sizeof(m_vTexCoordScale), uiSettingsHash);
    uiSettingsHash = plHashingUtils::xxHash64(&m_vTesselation, sizeof(m_vTesselation), uiSettingsHash);

    sResourceName.SetFormat("Heightfield:{}", uiSettingsHash);

    plTypedResourceHandle<ResourceType> hResource = plResourceManager::GetExistingResource<ResourceType>(sResourceName);
    if (hResource.IsValid())
      return hResource;
  }

  plMeshResourceDescriptor desc;
  if (BuildMeshDescriptor(desc).Succeeded())
  {
    return plResourceManager::CreateResource<ResourceType>(sResourceName, std::move(desc), sResourceName);
  }

  return plTypedResourceHandle<ResourceType>();
}

//////////////////////////////////////////////////////////////////////////

plHeightfieldComponentManager::plHeightfieldComponentManager(plWorld* pWorld)
  : plComponentManager<ComponentType, plBlockStorageType::Compact>(pWorld)
{
  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plHeightfieldComponentManager::ResourceEventHandler, this));
}

plHeightfieldComponentManager::~plHeightfieldComponentManager()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plHeightfieldComponentManager::ResourceEventHandler, this));
}

void plHeightfieldComponentManager::Initialize()
{
  auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plHeightfieldComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void plHeightfieldComponentManager::ResourceEventHandler(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<plImageDataResource>())
  {
    plImageDataResource* pResource = (plImageDataResource*)(e.m_pResource);
    const plUInt32 uiChangeCounter = pResource->GetCurrentResourceChangeCounter();
    plImageDataResourceHandle hResource(pResource);

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hHeightfield == hResource)
      {
        it->m_uiHeightfieldChangeCounter = uiChangeCounter;
        AddToUpdateList(it);
      }
    }
  }
}

void plHeightfieldComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    plHeightfieldComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InvalidateMesh();
  }

  m_ComponentsToUpdate.Clear();
}

void plHeightfieldComponentManager::AddToUpdateList(plHeightfieldComponent* pComponent)
{
  plComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == plInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}
