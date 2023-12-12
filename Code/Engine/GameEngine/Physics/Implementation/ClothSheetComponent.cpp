#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/ClothSheetComponent.h>
#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

/* TODO:
 * cache render category
 */

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plClothSheetRenderData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plClothSheetRenderer, 1, plRTTIDefaultAllocator<plClothSheetRenderer>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_BITFLAGS(plClothSheetFlags, 1)
  PLASMA_ENUM_CONSTANT(plClothSheetFlags::FixedCornerTopLeft),
  PLASMA_ENUM_CONSTANT(plClothSheetFlags::FixedCornerTopRight),
  PLASMA_ENUM_CONSTANT(plClothSheetFlags::FixedCornerBottomRight),
  PLASMA_ENUM_CONSTANT(plClothSheetFlags::FixedCornerBottomLeft),
  PLASMA_ENUM_CONSTANT(plClothSheetFlags::FixedEdgeTop),
  PLASMA_ENUM_CONSTANT(plClothSheetFlags::FixedEdgeRight),
  PLASMA_ENUM_CONSTANT(plClothSheetFlags::FixedEdgeBottom),
  PLASMA_ENUM_CONSTANT(plClothSheetFlags::FixedEdgeLeft),
PLASMA_END_STATIC_REFLECTED_BITFLAGS;

PLASMA_BEGIN_COMPONENT_TYPE(plClothSheetComponent, 1, plComponentMode::Static)
  {
    PLASMA_BEGIN_PROPERTIES
    {
      PLASMA_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new plDefaultValueAttribute(plVec2(0.5f, 0.5f))),
      PLASMA_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new plDefaultValueAttribute(plVec2(0.0f, 0.0f))),
      PLASMA_ACCESSOR_PROPERTY("Segments", GetSegments, SetSegments)->AddAttributes(new plDefaultValueAttribute(plVec2U32(7, 7)), new plClampValueAttribute(plVec2U32(1, 1), plVec2U32(31, 31))),
      PLASMA_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
      PLASMA_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 10.0f)),
      PLASMA_BITFLAGS_ACCESSOR_PROPERTY("Flags", plClothSheetFlags, GetFlags, SetFlags),
      PLASMA_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
      PLASMA_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new plDefaultValueAttribute(plColor::White)),
    }
    PLASMA_END_PROPERTIES;
    PLASMA_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Effects/Cloth"),
      new plColorAttribute(plColorScheme::Effects),
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

plClothSheetComponent::plClothSheetComponent() = default;
plClothSheetComponent::~plClothSheetComponent() = default;

void plClothSheetComponent::SetSize(plVec2 val)
{
  m_vSize = val;
  SetupCloth();
}

void plClothSheetComponent::SetSlack(plVec2 val)
{
  m_vSlack = val;
  SetupCloth();
}

void plClothSheetComponent::SetSegments(plVec2U32 val)
{
  m_vSegments = val;
  SetupCloth();
}

void plClothSheetComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_vSize;
  s << m_vSegments;
  s << m_vSlack;
  s << m_fWindInfluence;
  s << m_fDamping;
  s << m_Flags;
  s << m_hMaterial;
  s << m_Color;
}

void plClothSheetComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_vSize;
  s >> m_vSegments;
  s >> m_vSlack;
  s >> m_fWindInfluence;
  s >> m_fDamping;
  s >> m_Flags;
  s >> m_hMaterial;
  s >> m_Color;
}

void plClothSheetComponent::OnActivated()
{
  SUPER::OnActivated();

  SetupCloth();
}

void plClothSheetComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  SetupCloth();
}

void plClothSheetComponent::SetupCloth()
{
  m_Bbox.SetInvalid();

  if (IsActiveAndSimulating())
  {
    m_uiSleepCounter = 0;
    m_uiVisibleCounter = 5;

    m_Simulator.m_uiWidth = static_cast<plUInt8>(m_vSegments.x + 1);
    m_Simulator.m_uiHeight = static_cast<plUInt8>(m_vSegments.y + 1);
    m_Simulator.m_vAcceleration.Set(0, 0, -10);
    m_Simulator.m_vSegmentLength = m_vSize.CompMul(plVec2(1.0f) + m_vSlack);
    m_Simulator.m_vSegmentLength.x /= (float)m_vSegments.x;
    m_Simulator.m_vSegmentLength.y /= (float)m_vSegments.y;
    m_Simulator.m_Nodes.Clear();
    m_Simulator.m_Nodes.SetCount(m_Simulator.m_uiWidth * m_Simulator.m_uiHeight);

    const plVec3 pos = plVec3(0);
    const plVec3 dirX = plVec3(1, 0, 0);
    const plVec3 dirY = plVec3(0, 1, 0);

    plVec2 dist = m_vSize;
    dist.x /= (float)m_vSegments.x;
    dist.y /= (float)m_vSegments.y;

    for (plUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
    {
      for (plUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const plUInt32 idx = (y * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_vPosition = plSimdConversion::ToVec3(pos + x * dist.x * dirX + y * dist.y * dirY);
        m_Simulator.m_Nodes[idx].m_vPreviousPosition = m_Simulator.m_Nodes[idx].m_vPosition;
      }
    }

    if (m_Flags.IsSet(plClothSheetFlags::FixedCornerTopLeft))
      m_Simulator.m_Nodes[0].m_bFixed = true;

    if (m_Flags.IsSet(plClothSheetFlags::FixedCornerTopRight))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth - 1].m_bFixed = true;

    if (m_Flags.IsSet(plClothSheetFlags::FixedCornerBottomRight))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth * m_Simulator.m_uiHeight - 1].m_bFixed = true;

    if (m_Flags.IsSet(plClothSheetFlags::FixedCornerBottomLeft))
      m_Simulator.m_Nodes[m_Simulator.m_uiWidth * (m_Simulator.m_uiHeight - 1)].m_bFixed = true;

    if (m_Flags.IsSet(plClothSheetFlags::FixedEdgeTop))
    {
      for (plUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const plUInt32 idx = (0 * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(plClothSheetFlags::FixedEdgeRight))
    {
      for (plUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
      {
        const plUInt32 idx = (y * m_Simulator.m_uiWidth) + (m_Simulator.m_uiWidth - 1);

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(plClothSheetFlags::FixedEdgeBottom))
    {
      for (plUInt32 x = 0; x < m_Simulator.m_uiWidth; ++x)
      {
        const plUInt32 idx = ((m_Simulator.m_uiHeight - 1) * m_Simulator.m_uiWidth) + x;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }

    if (m_Flags.IsSet(plClothSheetFlags::FixedEdgeLeft))
    {
      for (plUInt32 y = 0; y < m_Simulator.m_uiHeight; ++y)
      {
        const plUInt32 idx = (y * m_Simulator.m_uiWidth) + 0;

        m_Simulator.m_Nodes[idx].m_bFixed = true;
      }
    }
  }

  TriggerLocalBoundsUpdate();
}

void plClothSheetComponent::OnDeactivated()
{
  m_Simulator.m_Nodes.Clear();

  SUPER::OnDeactivated();
}

plResult plClothSheetComponent::GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg)
{
  if (m_Bbox.IsValid())
  {
    bounds.ExpandToInclude(m_Bbox);
  }
  else
  {
    plBoundingBox box;
    box.SetInvalid();
    box.ExpandToInclude(plVec3::ZeroVector());
    box.ExpandToInclude(plVec3(m_vSize.x, 0, -0.1f));
    box.ExpandToInclude(plVec3(0, m_vSize.y, +0.1f));
    box.ExpandToInclude(plVec3(m_vSize.x, m_vSize.y, 0));

    bounds.ExpandToInclude(box);
  }

  return PLASMA_SUCCESS;
}

void plClothSheetComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  auto pRenderData = plCreateRenderDataForThisFrame<plClothSheetRenderData>(GetOwner());
  pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
  pRenderData->m_Color = m_Color;
  pRenderData->m_LastGlobalTransform = GetOwner()->GetLastGlobalTransform();
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_uiBatchId = plHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash());
  pRenderData->m_uiSortingKey = pRenderData->m_uiBatchId;
  pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
  pRenderData->m_hMaterial = m_hMaterial;


  if (m_Simulator.m_Nodes.IsEmpty())
  {
    pRenderData->m_uiVerticesX = 2;
    pRenderData->m_uiVerticesY = 2;

    pRenderData->m_Positions = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plVec3, 4);
    pRenderData->m_Indices = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plUInt16, 6);

    pRenderData->m_Positions[0] = plVec3(0, 0, 0);
    pRenderData->m_Positions[1] = plVec3(m_vSize.x, 0, 0);
    pRenderData->m_Positions[2] = plVec3(0, m_vSize.y, 0);
    pRenderData->m_Positions[3] = plVec3(m_vSize.x, m_vSize.y, 0);

    pRenderData->m_Indices[0] = 0;
    pRenderData->m_Indices[1] = 1;
    pRenderData->m_Indices[2] = 2;

    pRenderData->m_Indices[3] = 1;
    pRenderData->m_Indices[4] = 3;
    pRenderData->m_Indices[5] = 2;
  }
  else
  {
    m_uiVisibleCounter = 3;

    pRenderData->m_uiVerticesX = m_Simulator.m_uiWidth;
    pRenderData->m_uiVerticesY = m_Simulator.m_uiHeight;

    pRenderData->m_Positions = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plVec3, pRenderData->m_uiVerticesX * pRenderData->m_uiVerticesY);
    pRenderData->m_Indices = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plUInt16, (pRenderData->m_uiVerticesX - 1) * (pRenderData->m_uiVerticesY - 1) * 2 * 3);

    {
      plUInt32 vidx = 0;
      for (plUInt32 y = 0; y < pRenderData->m_uiVerticesY; ++y)
      {
        for (plUInt32 x = 0; x < pRenderData->m_uiVerticesX; ++x, ++vidx)
        {
          pRenderData->m_Positions[vidx] = plSimdConversion::ToVec3(m_Simulator.m_Nodes[vidx].m_vPosition);
        }
      }
    }

    {
      plUInt32 tidx = 0;
      plUInt16 vidx = 0;
      for (plUInt16 y = 0; y < pRenderData->m_uiVerticesY - 1; ++y)
      {
        for (plUInt16 x = 0; x < pRenderData->m_uiVerticesX - 1; ++x, ++vidx)
        {
          pRenderData->m_Indices[tidx++] = vidx;
          pRenderData->m_Indices[tidx++] = vidx + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX;

          pRenderData->m_Indices[tidx++] = vidx + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX;
        }

        ++vidx;
      }
    }
  }

  // TODO: render pass category (plus cache this)
  plRenderData::Category category = plDefaultRenderDataCategories::LitOpaque;

  if (m_hMaterial.IsValid())
  {
    plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::AllowLoadingFallback);
    category = pMaterial->GetRenderDataCategory();
  }

  msg.AddRenderData(pRenderData, category, plRenderData::Caching::Never);
}

void plClothSheetComponent::SetFlags(plBitflags<plClothSheetFlags> flags)
{
  m_Flags = flags;
  SetupCloth();
}

void plClothSheetComponent::SetMaterialFile(const char* szFile)
{
  plMaterialResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plMaterialResource>(szFile);
  }

  m_hMaterial = hResource;
}

const char* plClothSheetComponent::GetMaterialFile() const
{
  if (m_hMaterial.IsValid())
    return m_hMaterial.GetResourceID();

  return "";
}

void plClothSheetComponent::Update()
{
  if (m_Simulator.m_Nodes.IsEmpty() || m_uiVisibleCounter == 0)
    return;

  --m_uiVisibleCounter;

  {
    plVec3 acc = -GetOwner()->GetLinearVelocity();

    if (const plPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<plPhysicsWorldModuleInterface>())
    {
      acc += pModule->GetGravity();
    }
    else
    {
      acc += plVec3(0, 0, -9.81f);
    }

    if (m_fWindInfluence > 0.0f)
    {
      if (const plWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<plWindWorldModuleInterface>())
      {
        plVec3 ropeDir(0, 0, 1);

        // take the position of the center cloth node to sample the wind
        const plVec3 vSampleWindPos = GetOwner()->GetGlobalTransform().TransformPosition(plSimdConversion::ToVec3(m_Simulator.m_Nodes[m_Simulator.m_uiWidth * (m_Simulator.m_uiHeight / 2) + m_Simulator.m_uiWidth / 2].m_vPosition));

        const plVec3 vWind = pWind->GetWindAt(vSampleWindPos) * m_fWindInfluence;

        acc += vWind;
        acc += pWind->ComputeWindFlutter(vWind, ropeDir, 0.5f, GetOwner()->GetStableRandomSeed());
      }
    }

    // rotate the acceleration vector into the local simulation space
    acc = -GetOwner()->GetGlobalRotation() * acc;

    if (m_Simulator.m_vAcceleration != acc)
    {
      m_Simulator.m_vAcceleration = acc;
      m_uiSleepCounter = 0;
    }
  }

  if (m_uiSleepCounter <= 10)
  {
    m_Simulator.m_fDampingFactor = plMath::Lerp(1.0f, 0.97f, m_fDamping);

    m_Simulator.SimulateCloth(GetWorld()->GetClock().GetTimeDiff());

    auto prevBbox = m_Bbox;
    m_Bbox.ExpandToInclude(plSimdConversion::ToVec3(m_Simulator.m_Nodes[0].m_vPosition));
    m_Bbox.ExpandToInclude(plSimdConversion::ToVec3(m_Simulator.m_Nodes[m_Simulator.m_uiWidth - 1].m_vPosition));
    m_Bbox.ExpandToInclude(plSimdConversion::ToVec3(m_Simulator.m_Nodes[((m_Simulator.m_uiHeight - 1) * m_Simulator.m_uiWidth)].m_vPosition));
    m_Bbox.ExpandToInclude(plSimdConversion::ToVec3(m_Simulator.m_Nodes.PeekBack().m_vPosition));

    if (prevBbox != m_Bbox)
    {
      SetUserFlag(0, true); // flag 0 => requires local bounds update

      // can't call this here in the async phase
      // TriggerLocalBoundsUpdate();
    }

    ++m_uiCheckEquilibriumCounter;
    if (m_uiCheckEquilibriumCounter > 64)
    {
      m_uiCheckEquilibriumCounter = 0;

      if (m_Simulator.HasEquilibrium(0.01f))
      {
        ++m_uiSleepCounter;
      }
      else
      {
        m_uiSleepCounter = 0;
      }
    }
  }
}

plClothSheetRenderer::plClothSheetRenderer()
{
  CreateVertexBuffer();
}

plClothSheetRenderer::~plClothSheetRenderer() = default;

void plClothSheetRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& categories) const
{
  categories.PushBack(plDefaultRenderDataCategories::LitOpaque);
  categories.PushBack(plDefaultRenderDataCategories::LitMasked);
  categories.PushBack(plDefaultRenderDataCategories::LitTransparent);
  categories.PushBack(plDefaultRenderDataCategories::Selection);
}

void plClothSheetRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& types) const
{
  types.PushBack(plGetStaticRTTI<plClothSheetRenderData>());
}

void plClothSheetRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  const bool bNeedsNormals = (renderViewContext.m_pViewData->m_CameraUsageHint != plCameraUsageHint::Shadow);


  plRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  plInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<plInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  plResourceLock<plDynamicMeshBufferResource> pBuffer(m_hDynamicMeshBuffer, plResourceAcquireMode::BlockTillLoaded);

  for (auto it = batch.GetIterator<plClothSheetRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const plClothSheetRenderData* pRenderData = it;

    PLASMA_ASSERT_DEV(pRenderData->m_uiVerticesX > 1 && pRenderData->m_uiVerticesY > 1, "Invalid cloth render data");

    pRenderContext->BindMaterial(pRenderData->m_hMaterial);

    plUInt32 uiInstanceDataOffset = 0;
    plArrayPtr<plPerInstanceData> instanceData = pInstanceData->GetInstanceData(1, uiInstanceDataOffset);

    #if PLASMA_ENABLED(PLASMA_GAMEOBJECT_VELOCITY)
        instanceData[0].LastObjectToWorld = pRenderData->m_LastGlobalTransform;
        instanceData[0].LastObjectToWorldNormal = instanceData[0].LastObjectToWorld;
    #endif

    instanceData[0].ObjectToWorld = pRenderData->m_GlobalTransform;
    instanceData[0].ObjectToWorldNormal = instanceData[0].ObjectToWorld;
    instanceData[0].GameObjectID = pRenderData->m_uiUniqueID;
    instanceData[0].Color = pRenderData->m_Color;

    pInstanceData->UpdateInstanceData(pRenderContext, 1);

    {
      auto pVertexData = pBuffer->AccessVertexData();
      auto pIndexData = pBuffer->AccessIndex16Data();

      const float fDivU = 1.0f / (pRenderData->m_uiVerticesX - 1);
      const float fDivY = 1.0f / (pRenderData->m_uiVerticesY - 1);

      const plUInt16 width = pRenderData->m_uiVerticesX;

      if (bNeedsNormals)
      {
        const plUInt16 widthM1 = width - 1;
        const plUInt16 heightM1 = pRenderData->m_uiVerticesY - 1;

        plUInt16 topIdx = 0;

        plUInt32 vidx = 0;
        for (plUInt16 y = 0; y < pRenderData->m_uiVerticesY; ++y)
        {
          plUInt16 leftIdx = 0;
          const plUInt16 bottomIdx = plMath::Min<plUInt16>(y + 1, heightM1);

          const plUInt32 yOff = y * width;
          const plUInt32 yOffTop = topIdx * width;
          const plUInt32 yOffBottom = bottomIdx * width;

          for (plUInt16 x = 0; x < width; ++x, ++vidx)
          {
            const plUInt16 rightIdx = plMath::Min<plUInt16>(x + 1, widthM1);

            const plVec3 leftPos = pRenderData->m_Positions[yOff + leftIdx];
            const plVec3 rightPos = pRenderData->m_Positions[yOff + rightIdx];
            const plVec3 topPos = pRenderData->m_Positions[yOffTop + x];
            const plVec3 bottomPos = pRenderData->m_Positions[yOffBottom + x];

            const plVec3 leftToRight = rightPos - leftPos;
            const plVec3 bottomToTop = topPos - bottomPos;
            plVec3 normal = -leftToRight.CrossRH(bottomToTop);
            normal.NormalizeIfNotZero(plVec3(0, 0, 1)).IgnoreResult();

            plVec3 tangent = leftToRight;
            tangent.NormalizeIfNotZero(plVec3(1, 0, 0)).IgnoreResult();

            pVertexData[vidx].m_vPosition = pRenderData->m_Positions[vidx];
            pVertexData[vidx].m_vTexCoord = plVec2(x * fDivU, y * fDivY);
            pVertexData[vidx].EncodeNormal(normal);
            pVertexData[vidx].EncodeTangent(tangent, 1.0f);

            leftIdx = x;
          }

          topIdx = y;
        }
      }
      else
      {
        plUInt32 vidx = 0;
        for (plUInt16 y = 0; y < pRenderData->m_uiVerticesY; ++y)
        {
          for (plUInt16 x = 0; x < width; ++x, ++vidx)
          {
            pVertexData[vidx].m_vPosition = pRenderData->m_Positions[vidx];
            pVertexData[vidx].m_vTexCoord = plVec2(x * fDivU, y * fDivY);
            pVertexData[vidx].EncodeNormal(plVec3::UnitZAxis());
            pVertexData[vidx].EncodeTangent(plVec3::UnitXAxis(), 1.0f);
          }
        }
      }

      plMemoryUtils::Copy<plUInt16>(pIndexData.GetPtr(), pRenderData->m_Indices.GetPtr(), pRenderData->m_Indices.GetCount());
    }

    const plUInt32 uiNumPrimitives = (pRenderData->m_uiVerticesX - 1) * (pRenderData->m_uiVerticesY - 1) * 2;

    pBuffer->UpdateGpuBuffer(pGALCommandEncoder, 0, pRenderData->m_uiVerticesX * pRenderData->m_uiVerticesY);

    // redo this after the primitive count has changed
    pRenderContext->BindMeshBuffer(m_hDynamicMeshBuffer);

    renderViewContext.m_pRenderContext->DrawMeshBuffer(uiNumPrimitives).IgnoreResult();
  }
}

void plClothSheetRenderer::CreateVertexBuffer()
{
  if (m_hDynamicMeshBuffer.IsValid())
    return;

  m_hDynamicMeshBuffer = plResourceManager::GetExistingResource<plDynamicMeshBufferResource>("ClothSheet");

  if (!m_hDynamicMeshBuffer.IsValid())
  {
    const plUInt32 uiMaxVerts = 32;

    plDynamicMeshBufferResourceDescriptor desc;
    desc.m_uiMaxVertices = uiMaxVerts * uiMaxVerts;
    desc.m_IndexType = plGALIndexType::UShort;
    desc.m_uiMaxPrimitives = plMath::Square(uiMaxVerts - 1) * 2;

    m_hDynamicMeshBuffer = plResourceManager::GetOrCreateResource<plDynamicMeshBufferResource>("ClothSheet", std::move(desc), "Cloth Sheet Buffer");
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plClothSheetComponentManager::plClothSheetComponentManager(plWorld* pWorld)
  : plComponentManager(pWorld)
{
}

plClothSheetComponentManager::~plClothSheetComponentManager() = default;

void plClothSheetComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plClothSheetComponentManager::Update, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plClothSheetComponentManager::UpdateBounds, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void plClothSheetComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update();
    }
  }
}

void plClothSheetComponentManager::UpdateBounds(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUserFlag(0))
    {
      it->TriggerLocalBoundsUpdate();

      // reset update bounds flag
      it->SetUserFlag(0, false);
    }
  }
}
