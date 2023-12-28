#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Components/RopeRenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/Types.h>
#include <RendererFoundation/Device/Device.h>

plCVarBool cvar_FeatureRopesVisBones("Feature.Ropes.VisBones", false, plCVarFlags::Default, "Enables debug visualization of rope bones");

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plRopeRenderComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PLASMA_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new plDefaultValueAttribute(plColor::White), new plExposeColorAlphaAttribute()),
    PLASMA_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new plDefaultValueAttribute(0.05f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new plDefaultValueAttribute(6), new plClampValueAttribute(3, 16)),
    PLASMA_ACCESSOR_PROPERTY("Subdivide", GetSubdivide, SetSubdivide),
    PLASMA_ACCESSOR_PROPERTY("UScale", GetUScale, SetUScale)->AddAttributes(new plDefaultValueAttribute(1.0f)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PLASMA_MESSAGE_HANDLER(plMsgRopePoseUpdated, OnRopePoseUpdated),
    PLASMA_MESSAGE_HANDLER(plMsgSetColor, OnMsgSetColor),
    PLASMA_MESSAGE_HANDLER(plMsgSetMeshMaterial, OnMsgSetMeshMaterial),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects/Ropes"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRopeRenderComponent::plRopeRenderComponent() = default;
plRopeRenderComponent::~plRopeRenderComponent() = default;

void plRopeRenderComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Color;
  s << m_hMaterial;
  s << m_fThickness;
  s << m_uiDetail;
  s << m_bSubdivide;
  s << m_fUScale;
}

void plRopeRenderComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;
  s >> m_fThickness;
  s >> m_uiDetail;
  s >> m_bSubdivide;
  s >> m_fUScale;
}

void plRopeRenderComponent::OnActivated()
{
  SUPER::OnActivated();

  m_LocalBounds.SetInvalid();
}

void plRopeRenderComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  SUPER::OnDeactivated();
}

plResult plRopeRenderComponent::GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg)
{
  bounds = m_LocalBounds;
  return PLASMA_SUCCESS;
}

void plRopeRenderComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const plUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const plUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::AllowLoadingFallback);
  plMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[0];

  plSkinnedMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plSkinnedMeshRenderData>(GetOwner());
  {
    pRenderData->m_LastGlobalTransform = GetOwner()->GetLastGlobalTransform();
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = hMaterial;
    pRenderData->m_Color = m_Color;

    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiFlipWinding = uiFlipWinding;
    pRenderData->m_uiUniformScale = uiUniformScale;

    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    m_SkinningState.FillSkinnedMeshRenderData(*pRenderData);

    pRenderData->FillBatchIdAndSortingKey();
  }

  // Determine render data category.
  plRenderData::Category category = plDefaultRenderDataCategories::LitOpaque;

  if (hMaterial.IsValid())
  {
    plResourceLock<plMaterialResource> pMaterial(hMaterial, plResourceAcquireMode::AllowLoadingFallback);
    category = pMaterial->GetRenderDataCategory();
  }

  msg.AddRenderData(pRenderData, category, plRenderData::Caching::Never);

  if (cvar_FeatureRopesVisBones)
  {
    plHybridArray<plDebugRenderer::Line, 128> lines(plFrameAllocator::GetCurrentAllocator());
    lines.Reserve(m_SkinningState.m_Transforms.GetCount() * 3);

    plMat4 offsetMat;
    offsetMat.SetIdentity();

    for (plUInt32 i = 0; i < m_SkinningState.m_Transforms.GetCount(); ++i)
    {
      offsetMat.SetTranslationVector(plVec3(static_cast<float>(i), 0, 0));
      plMat4 skinningMat = m_SkinningState.m_Transforms[i].GetAsMat4() * offsetMat;

      plVec3 pos = skinningMat.GetTranslationVector();

      auto& x = lines.ExpandAndGetRef();
      x.m_start = pos;
      x.m_end = x.m_start + skinningMat.TransformDirection(plVec3::UnitXAxis());
      x.m_startColor = plColor::Red;
      x.m_endColor = plColor::Red;

      auto& y = lines.ExpandAndGetRef();
      y.m_start = pos;
      y.m_end = y.m_start + skinningMat.TransformDirection(plVec3::UnitYAxis() * 2.0f);
      y.m_startColor = plColor::Green;
      y.m_endColor = plColor::Green;

      auto& z = lines.ExpandAndGetRef();
      z.m_start = pos;
      z.m_end = z.m_start + skinningMat.TransformDirection(plVec3::UnitZAxis() * 2.0f);
      z.m_startColor = plColor::Blue;
      z.m_endColor = plColor::Blue;
    }

    plDebugRenderer::DrawLines(msg.m_pView->GetHandle(), lines, plColor::White, GetOwner()->GetGlobalTransform());
  }
}

void plRopeRenderComponent::SetMaterialFile(const char* szFile)
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

const char* plRopeRenderComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void plRopeRenderComponent::SetThickness(float fThickness)
{
  if (m_fThickness != fThickness)
  {
    m_fThickness = fThickness;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      plHybridArray<plTransform, 128> transforms;
      transforms.SetCountUninitialized(m_SkinningState.m_Transforms.GetCount());

      plMat4 offsetMat;
      offsetMat.SetIdentity();

      for (plUInt32 i = 0; i < m_SkinningState.m_Transforms.GetCount(); ++i)
      {
        offsetMat.SetTranslationVector(plVec3(static_cast<float>(i), 0, 0));
        plMat4 skinningMat = m_SkinningState.m_Transforms[i].GetAsMat4() * offsetMat;

        transforms[i].SetFromMat4(skinningMat);
      }

      UpdateSkinningTransformBuffer(transforms);
    }
  }
}

void plRopeRenderComponent::SetDetail(plUInt32 uiDetail)
{
  if (m_uiDetail != uiDetail)
  {
    m_uiDetail = uiDetail;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void plRopeRenderComponent::SetSubdivide(bool bSubdivide)
{
  if (m_bSubdivide != bSubdivide)
  {
    m_bSubdivide = bSubdivide;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void plRopeRenderComponent::SetUScale(float fUScale)
{
  if (m_fUScale != fUScale)
  {
    m_fUScale = fUScale;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void plRopeRenderComponent::OnMsgSetColor(plMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

void plRopeRenderComponent::OnMsgSetMeshMaterial(plMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_hMaterial);
}

void plRopeRenderComponent::OnRopePoseUpdated(plMsgRopePoseUpdated& msg)
{
  if (msg.m_LinkTransforms.IsEmpty())
    return;

  if (m_SkinningState.m_Transforms.GetCount() != msg.m_LinkTransforms.GetCount())
  {
    m_SkinningState.Clear();

    GenerateRenderMesh(msg.m_LinkTransforms.GetCount());
  }

  UpdateSkinningTransformBuffer(msg.m_LinkTransforms);

  plBoundingBox newBounds;
  newBounds.SetFromPoints(&msg.m_LinkTransforms[0].m_vPosition, msg.m_LinkTransforms.GetCount(), sizeof(plTransform));

  // if the existing bounds are big enough, don't update them
  if (!m_LocalBounds.IsValid() || !m_LocalBounds.GetBox().Contains(newBounds))
  {
    m_LocalBounds.ExpandToInclude(newBounds);

    TriggerLocalBoundsUpdate();
  }
}

void plRopeRenderComponent::GenerateRenderMesh(plUInt32 uiNumRopePieces)
{
  plStringBuilder sResourceName;
  sResourceName.Format("Rope-Mesh:{}{}-d{}-u{}", uiNumRopePieces, m_bSubdivide ? "Sub" : "", m_uiDetail, m_fUScale);

  m_hMesh = plResourceManager::GetExistingResource<plMeshResource>(sResourceName);
  if (m_hMesh.IsValid())
    return;

  plGeometry geom;

  const plAngle fDegStep = plAngle::Degree(360.0f / m_uiDetail);
  const float fVStep = 1.0f / m_uiDetail;

  auto addCap = [&](float x, const plVec3& vNormal, plUInt16 uiBoneIndex, bool bFlipWinding) {
    plVec4U16 boneIndices(uiBoneIndex, 0, 0, 0);

    plUInt32 centerIndex = geom.AddVertex(plVec3(x, 0, 0), vNormal, plVec2(0.5f, 0.5f), plColor::White, boneIndices);

    plAngle deg = plAngle::Radian(0);
    for (plUInt32 s = 0; s < m_uiDetail; ++s)
    {
      const float fY = plMath::Cos(deg);
      const float fZ = plMath::Sin(deg);

      geom.AddVertex(plVec3(x, fY, fZ), vNormal, plVec2(fY, fZ), plColor::White, boneIndices);

      deg += fDegStep;
    }

    plUInt32 triangle[3];
    triangle[0] = centerIndex;
    for (plUInt32 s = 0; s < m_uiDetail; ++s)
    {
      triangle[1] = s + triangle[0] + 1;
      triangle[2] = ((s + 1) % m_uiDetail) + triangle[0] + 1;

      geom.AddPolygon(triangle, bFlipWinding);
    }
  };

  auto addPiece = [&](float x, const plVec4U16& vBoneIndices, const plColorLinearUB& boneWeights, bool bCreatePolygons) {
    plAngle deg = plAngle::Radian(0);
    float fU = x * m_fUScale;
    float fV = 0;

    for (plUInt32 s = 0; s <= m_uiDetail; ++s)
    {
      const float fY = plMath::Cos(deg);
      const float fZ = plMath::Sin(deg);

      const plVec3 pos(x, fY, fZ);
      const plVec3 normal(0, fY, fZ);

      geom.AddVertex(pos, normal, plVec2(fU, fV), plColor::White, vBoneIndices, boneWeights);

      deg += fDegStep;
      fV += fVStep;
    }

    if (bCreatePolygons)
    {
      plUInt32 endIndex = geom.GetVertices().GetCount() - (m_uiDetail + 1);
      plUInt32 startIndex = endIndex - (m_uiDetail + 1);

      plUInt32 triangle[3];
      for (plUInt32 s = 0; s < m_uiDetail; ++s)
      {
        triangle[0] = startIndex + s;
        triangle[1] = startIndex + s + 1;
        triangle[2] = endIndex + s + 1;
        geom.AddPolygon(triangle, false);

        triangle[0] = startIndex + s;
        triangle[1] = endIndex + s + 1;
        triangle[2] = endIndex + s;
        geom.AddPolygon(triangle, false);
      }
    }
  };

  // cap
  {
    const plVec3 normal = plVec3(-1, 0, 0);
    addCap(0.0f, normal, 0, true);
  }

  // pieces
  {
    // first ring full weight to first bone
    addPiece(0.0f, plVec4U16(0, 0, 0, 0), plColorLinearUB(255, 0, 0, 0), false);

    plUInt16 p = 1;

    if (m_bSubdivide)
    {
      addPiece(0.75f, plVec4U16(0, 0, 0, 0), plColorLinearUB(255, 0, 0, 0), true);

      for (; p < uiNumRopePieces - 2; ++p)
      {
        addPiece(static_cast<float>(p) + 0.25f, plVec4U16(p, 0, 0, 0), plColorLinearUB(255, 0, 0, 0), true);
        addPiece(static_cast<float>(p) + 0.75f, plVec4U16(p, 0, 0, 0), plColorLinearUB(255, 0, 0, 0), true);
      }

      addPiece(static_cast<float>(p) + 0.25f, plVec4U16(p, 0, 0, 0), plColorLinearUB(255, 0, 0, 0), true);
      ++p;
    }
    else
    {
      for (; p < uiNumRopePieces - 1; ++p)
      {
        // Middle rings half weight between bones. To ensure that weights sum up to 1 we weight one bone with 128 and the other with 127,
        // since "ubyte normalized" can't represent 0.5 perfectly.
        addPiece(static_cast<float>(p), plVec4U16(p - 1, p, 0, 0), plColorLinearUB(128, 127, 0, 0), true);
      }
    }

    // last ring full weight to last bone
    addPiece(static_cast<float>(p), plVec4U16(p, 0, 0, 0), plColorLinearUB(255, 0, 0, 0), true);
  }

  // cap
  {
    const plVec3 normal = plVec3(1, 0, 0);
    addCap(static_cast<float>(uiNumRopePieces - 1), normal, static_cast<plUInt16>(uiNumRopePieces - 1), false);
  }

  geom.ComputeTangents();

  plMeshResourceDescriptor desc;

  // Data/Base/Materials/Prototyping/PrototypeBlack.plMaterialAsset
  desc.SetMaterial(0, "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }");

  auto& meshBufferDesc = desc.MeshBufferDesc();
  meshBufferDesc.AddCommonStreams();
  meshBufferDesc.AddStream(plGALVertexAttributeSemantic::BoneIndices0, plGALResourceFormat::RGBAUByte);
  meshBufferDesc.AddStream(plGALVertexAttributeSemantic::BoneWeights0, plGALResourceFormat::RGBAUByteNormalized);
  meshBufferDesc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(meshBufferDesc.GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();

  m_hMesh = plResourceManager::CreateResource<plMeshResource>(sResourceName, std::move(desc), sResourceName);
}

void plRopeRenderComponent::UpdateSkinningTransformBuffer(plArrayPtr<const plTransform> skinningTransforms)
{
  plMat4 bindPoseMat;
  bindPoseMat.SetIdentity();
  m_SkinningState.m_Transforms.SetCountUninitialized(skinningTransforms.GetCount());

  const plVec3 newScale = plVec3(1.0f, m_fThickness * 0.5f, m_fThickness * 0.5f);
  for (plUInt32 i = 0; i < skinningTransforms.GetCount(); ++i)
  {
    plTransform t = skinningTransforms[i];
    t.m_vScale = newScale;

    // scale x axis to match the distance between this bone and the next bone
    if (i < skinningTransforms.GetCount() - 1)
    {
      t.m_vScale.x = (skinningTransforms[i + 1].m_vPosition - skinningTransforms[i].m_vPosition).GetLength();
    }

    bindPoseMat.SetTranslationVector(plVec3(-static_cast<float>(i), 0, 0));

    m_SkinningState.m_Transforms[i] = t.GetAsMat4() * bindPoseMat;
  }

  m_SkinningState.TransformsChanged();
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RopeRenderComponent);
