#include <RendererCore/RendererCorePCH.h>

#include <../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plCustomMeshComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new plExposeColorAlphaAttribute()),
    PL_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    PL_MESSAGE_HANDLER(plMsgSetColor, OnMsgSetColor),
  } PL_END_MESSAGEHANDLERS;
}
PL_END_COMPONENT_TYPE
// clang-format on

plAtomicInteger32 s_iCustomMeshResources;

plCustomMeshComponent::plCustomMeshComponent()
{
  m_Bounds = plBoundingBoxSphere::MakeInvalid();
}

plCustomMeshComponent::~plCustomMeshComponent() = default;

void plCustomMeshComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_Color;
  s << m_hMaterial;
}

void plCustomMeshComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;

  if (uiVersion < 2)
  {
    plUInt32 uiCategory = 0;
    s >> uiCategory;
  }
}

plResult plCustomMeshComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  if (m_Bounds.IsValid())
  {
    ref_bounds = m_Bounds;
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

plDynamicMeshBufferResourceHandle plCustomMeshComponent::CreateMeshResource(plGALPrimitiveTopology::Enum topology, plUInt32 uiMaxVertices, plUInt32 uiMaxPrimitives, plGALIndexType::Enum indexType)
{
  plDynamicMeshBufferResourceDescriptor desc;
  desc.m_Topology = topology;
  desc.m_uiMaxVertices = uiMaxVertices;
  desc.m_uiMaxPrimitives = uiMaxPrimitives;
  desc.m_IndexType = indexType;
  desc.m_bColorStream = true;

  plStringBuilder sGuid;
  sGuid.SetFormat("CustomMesh_{}", s_iCustomMeshResources.Increment());

  m_hDynamicMesh = plResourceManager::CreateResource<plDynamicMeshBufferResource>(sGuid, std::move(desc));

  InvalidateCachedRenderData();

  return m_hDynamicMesh;
}

void plCustomMeshComponent::SetMeshResource(const plDynamicMeshBufferResourceHandle& hMesh)
{
  m_hDynamicMesh = hMesh;
  InvalidateCachedRenderData();
}

void plCustomMeshComponent::SetBounds(const plBoundingBoxSphere& bounds)
{
  m_Bounds = bounds;
  TriggerLocalBoundsUpdate();
}

void plCustomMeshComponent::SetMaterial(const plMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
  InvalidateCachedRenderData();
}

plMaterialResourceHandle plCustomMeshComponent::GetMaterial() const
{
  return m_hMaterial;
}

void plCustomMeshComponent::SetMaterialFile(const char* szMaterial)
{
  plMaterialResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szMaterial))
  {
    hResource = plResourceManager::LoadResource<plMaterialResource>(szMaterial);
  }

  m_hMaterial = hResource;
}

const char* plCustomMeshComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void plCustomMeshComponent::SetColor(const plColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const plColor& plCustomMeshComponent::GetColor() const
{
  return m_Color;
}

void plCustomMeshComponent::OnMsgSetMeshMaterial(plMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_hMaterial);
}

void plCustomMeshComponent::OnMsgSetColor(plMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void plCustomMeshComponent::SetUsePrimitiveRange(plUInt32 uiFirstPrimitive /*= 0*/, plUInt32 uiNumPrimitives /*= plMath::MaxValue<plUInt32>()*/)
{
  m_uiFirstPrimitive = uiFirstPrimitive;
  m_uiNumPrimitives = uiNumPrimitives;
}

void plCustomMeshComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!m_hDynamicMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  plResourceLock<plDynamicMeshBufferResource> pMesh(m_hDynamicMesh, plResourceAcquireMode::BlockTillLoaded);

  plCustomMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plCustomMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hDynamicMesh;
    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_Color = m_Color;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
    pRenderData->m_uiFirstPrimitive = plMath::Min(m_uiFirstPrimitive, pMesh->GetDescriptor().m_uiMaxPrimitives);
    pRenderData->m_uiNumPrimitives = plMath::Min(m_uiNumPrimitives, pMesh->GetDescriptor().m_uiMaxPrimitives - pRenderData->m_uiFirstPrimitive);

    pRenderData->FillBatchIdAndSortingKey();
  }

  plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::AllowLoadingFallback);
  plRenderData::Category category = pMaterial->GetRenderDataCategory();
  bool bDontCacheYet = pMaterial.GetAcquireResult() == plResourceAcquireResult::LoadingFallback;

  msg.AddRenderData(pRenderData, category, bDontCacheYet ? plRenderData::Caching::Never : plRenderData::Caching::IfStatic);
}

void plCustomMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  if (false)
  {
    plGeometry geo;
    geo.AddTorus(1.0f, 1.5f, 32, 16, false);
    geo.TriangulatePolygons();
    geo.ComputeTangents();

    auto hMesh = CreateMeshResource(plGALPrimitiveTopology::Triangles, geo.GetVertices().GetCount(), geo.GetPolygons().GetCount(), plGALIndexType::UInt);

    plResourceLock<plDynamicMeshBufferResource> pMesh(hMesh, plResourceAcquireMode::BlockTillLoaded);

    auto verts = pMesh->AccessVertexData();
    auto cols = pMesh->AccessColorData();

    for (plUInt32 v = 0; v < verts.GetCount(); ++v)
    {
      verts[v].m_vPosition = geo.GetVertices()[v].m_vPosition;
      verts[v].m_vTexCoord.SetZero();
      verts[v].EncodeNormal(geo.GetVertices()[v].m_vNormal);
      verts[v].EncodeTangent(geo.GetVertices()[v].m_vTangent, 1.0f);

      cols[v] = plColor::CornflowerBlue;
    }

    auto ind = pMesh->AccessIndex32Data();

    for (plUInt32 i = 0; i < geo.GetPolygons().GetCount(); ++i)
    {
      ind[i * 3 + 0] = geo.GetPolygons()[i].m_Vertices[0];
      ind[i * 3 + 1] = geo.GetPolygons()[i].m_Vertices[1];
      ind[i * 3 + 2] = geo.GetPolygons()[i].m_Vertices[2];
    }

    SetBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), 1.5f));
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCustomMeshRenderData, 1, plRTTIDefaultAllocator<plCustomMeshRenderData>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


void plCustomMeshRenderData::FillBatchIdAndSortingKey()
{
  const plUInt32 uiAdditionalBatchData = 0;

  m_uiFlipWinding = m_GlobalTransform.ContainsNegativeScale() ? 1 : 0;
  m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

  const plUInt32 uiMeshIDHash = plHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
  const plUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? plHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;

  // Generate batch id from mesh, material and part index.
  plUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, 0 /*m_uiSubMeshIndex*/, m_uiFlipWinding, uiAdditionalBatchData};
  m_uiBatchId = plHashingUtils::xxHash32(data, sizeof(data));

  // Sort by material and then by mesh
  m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + 0 /*m_uiSubMeshIndex*/) & 0xFFFE) | m_uiFlipWinding;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCustomMeshRenderer, 1, plRTTIDefaultAllocator<plCustomMeshRenderer>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCustomMeshRenderer::plCustomMeshRenderer() = default;
plCustomMeshRenderer::~plCustomMeshRenderer() = default;

void plCustomMeshRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(plDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(plDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(plDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(plDefaultRenderDataCategories::Selection);
}

void plCustomMeshRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plCustomMeshRenderData>());
}

void plCustomMeshRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  plRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  plGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  plInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<plInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  const plCustomMeshRenderData* pRenderData1st = batch.GetFirstData<plCustomMeshRenderData>();

  if (pRenderData1st->m_uiFlipWinding)
  {
    pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  for (auto it = batch.GetIterator<plCustomMeshRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const plCustomMeshRenderData* pRenderData = it;

    plResourceLock<plDynamicMeshBufferResource> pBuffer(pRenderData->m_hMesh, plResourceAcquireMode::BlockTillLoaded);

    pRenderContext->BindMaterial(pRenderData->m_hMaterial);

    plUInt32 uiInstanceDataOffset = 0;
    plArrayPtr<plPerInstanceData> instanceData = pInstanceData->GetInstanceData(1, uiInstanceDataOffset);

    instanceData[0].GameObjectID = pRenderData->m_uiUniqueID;
    instanceData[0].Color = pRenderData->m_Color;
    instanceData[0].ObjectToWorld = pRenderData->m_GlobalTransform;

    if (pRenderData->m_uiUniformScale)
    {
      instanceData[0].ObjectToWorldNormal = instanceData[0].ObjectToWorld;
    }
    else
    {
      plMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

      plMat3 mInverse = objectToWorld.GetRotationalPart();
      mInverse.Invert(0.0f).IgnoreResult();
      // we explicitly ignore the return value here (success / failure)
      // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying
      instanceData[0].ObjectToWorldNormal = mInverse.GetTranspose();
    }

    pInstanceData->UpdateInstanceData(pRenderContext, 1);

    const auto& desc = pBuffer->GetDescriptor();
    pBuffer->UpdateGpuBuffer(pGALCommandEncoder);

    // redo this after the primitive count has changed
    pRenderContext->BindMeshBuffer(pRenderData->m_hMesh);

    renderViewContext.m_pRenderContext->DrawMeshBuffer(pRenderData->m_uiNumPrimitives, pRenderData->m_uiFirstPrimitive).IgnoreResult();
  }
}


PL_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_CustomMeshComponent);
