#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/BakedProbes/BakedProbesComponent.h>
#include <RendererCore/BakedProbes/BakedProbesWorldModule.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Resources/Texture.h>

struct plBakedProbesComponent::RenderDebugViewTask : public plTask
{
  RenderDebugViewTask()
  {
    ConfigureTask("BakingDebugView", plTaskNesting::Never);
  }

  virtual void Execute() override
  {
    PLASMA_ASSERT_DEV(m_PixelData.GetCount() == m_uiWidth * m_uiHeight, "Pixel data must be pre-allocated");

    plProgress progress;
    progress.m_Events.AddEventHandler([this](const plProgressEvent& e) {
      if (e.m_Type != plProgressEvent::Type::CancelClicked)
      {
        if (HasBeenCanceled())
        {
          e.m_pProgressbar->UserClickedCancel();
        }
        m_bHasNewData = true;
      }
    });

    if (m_pBakingInterface->RenderDebugView(*m_pWorld, m_InverseViewProjection, m_uiWidth, m_uiHeight, m_PixelData, progress).Succeeded())
    {
      m_bHasNewData = true;
    }
  }

  plBakingInterface* m_pBakingInterface = nullptr;

  const plWorld* m_pWorld = nullptr;
  plMat4 m_InverseViewProjection = plMat4::IdentityMatrix();
  plUInt32 m_uiWidth = 0;
  plUInt32 m_uiHeight = 0;
  plDynamicArray<plColorGammaUB> m_PixelData;

  bool m_bHasNewData = false;
};

//////////////////////////////////////////////////////////////////////////


plBakedProbesComponentManager::plBakedProbesComponentManager(plWorld* pWorld)
  : plSettingsComponentManager<plBakedProbesComponent>(pWorld)
{
}

plBakedProbesComponentManager::~plBakedProbesComponentManager() = default;

void plBakedProbesComponentManager::Initialize()
{
  {
    auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plBakedProbesComponentManager::RenderDebug, this);

    this->RegisterUpdateFunction(desc);
  }

  plRenderWorld::GetRenderEvent().AddEventHandler(plMakeDelegate(&plBakedProbesComponentManager::OnRenderEvent, this));

  CreateDebugResources();
}

void plBakedProbesComponentManager::Deinitialize()
{
  plRenderWorld::GetRenderEvent().RemoveEventHandler(plMakeDelegate(&plBakedProbesComponentManager::OnRenderEvent, this));
}

void plBakedProbesComponentManager::RenderDebug(const plWorldModule::UpdateContext& updateContext)
{
  if (plBakedProbesComponent* pComponent = GetSingletonComponent())
  {
    if (pComponent->GetShowDebugOverlay())
    {
      pComponent->RenderDebugOverlay();
    }
  }
}

void plBakedProbesComponentManager::OnRenderEvent(const plRenderWorldRenderEvent& e)
{
  if (e.m_Type != plRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (plBakedProbesComponent* pComponent = GetSingletonComponent())
  {
    auto& task = pComponent->m_pRenderDebugViewTask;
    if (task != nullptr && task->m_bHasNewData)
    {
      task->m_bHasNewData = false;

      plGALDevice* pGALDevice = plGALDevice::GetDefaultDevice();
      plGALPass* pGALPass = pGALDevice->BeginPass("BakingDebugViewUpdate");
      auto pCommandEncoder = pGALPass->BeginCompute();

      plBoundingBoxu32 destBox;
      destBox.m_vMin.SetZero();
      destBox.m_vMax = plVec3U32(task->m_uiWidth, task->m_uiHeight, 1);

      plGALSystemMemoryDescription sourceData;
      sourceData.m_pData = task->m_PixelData.GetData();
      sourceData.m_uiRowPitch = task->m_uiWidth * sizeof(plColorGammaUB);

      pCommandEncoder->UpdateTexture(pComponent->m_hDebugViewTexture, plGALTextureSubresource(), destBox, sourceData);

      pGALPass->EndCompute(pCommandEncoder);
      pGALDevice->EndPass(pGALPass);
    }
  }
}

void plBakedProbesComponentManager::CreateDebugResources()
{
  if (!m_hDebugSphere.IsValid())
  {
    plGeometry geom;
    geom.AddSphere(0.3f, 32, 16);

    const char* szBufferResourceName = "IrradianceProbeDebugSphereBuffer";
    plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szBufferResourceName);
    if (!hMeshBuffer.IsValid())
    {
      plMeshBufferResourceDescriptor desc;
      desc.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
      desc.AddStream(plGALVertexAttributeSemantic::Normal, plGALResourceFormat::XYZFloat);
      desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

      hMeshBuffer = plResourceManager::GetOrCreateResource<plMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
    }

    const char* szMeshResourceName = "IrradianceProbeDebugSphere";
    m_hDebugSphere = plResourceManager::GetExistingResource<plMeshResource>(szMeshResourceName);
    if (!m_hDebugSphere.IsValid())
    {
      plMeshResourceDescriptor desc;
      desc.UseExistingMeshBuffer(hMeshBuffer);
      desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
      desc.ComputeBounds();

      m_hDebugSphere = plResourceManager::GetOrCreateResource<plMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
    }
  }

  if (!m_hDebugMaterial.IsValid())
  {
    m_hDebugMaterial = plResourceManager::LoadResource<plMaterialResource>(
      "{ 4d15c716-a8e9-43d4-9424-43174403fb94 }"); // IrradianceProbeVisualization.plMaterialAsset
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plBakedProbesComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Settings", m_Settings),
    PLASMA_ACCESSOR_PROPERTY("ShowDebugOverlay", GetShowDebugOverlay, SetShowDebugOverlay)->AddAttributes(new plGroupAttribute("Debug")),
    PLASMA_ACCESSOR_PROPERTY("ShowDebugProbes", GetShowDebugProbes, SetShowDebugProbes),
    PLASMA_ACCESSOR_PROPERTY("UseTestPosition", GetUseTestPosition, SetUseTestPosition),
    PLASMA_ACCESSOR_PROPERTY("TestPosition", GetTestPosition, SetTestPosition)
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnExtractRenderData),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_FUNCTION_PROPERTY(OnObjectCreated),
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Lighting/Baking"),
    new plLongOpAttribute("plLongOpProxy_BakeScene"),
    new plTransformManipulatorAttribute("TestPosition"),
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Beta),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plBakedProbesComponent::plBakedProbesComponent() = default;
plBakedProbesComponent::~plBakedProbesComponent() = default;

void plBakedProbesComponent::OnActivated()
{
  auto pModule = GetWorld()->GetOrCreateModule<plBakedProbesWorldModule>();
  pModule->SetProbeTreeResourcePrefix(m_sProbeTreeResourcePrefix);

  GetOwner()->UpdateLocalBounds();

  SUPER::OnActivated();
}

void plBakedProbesComponent::OnDeactivated()
{
  if (m_pRenderDebugViewTask != nullptr)
  {
    plTaskSystem::CancelTask(m_pRenderDebugViewTask).IgnoreResult();
  }

  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void plBakedProbesComponent::SetShowDebugOverlay(bool bShow)
{
  m_bShowDebugOverlay = bShow;

  if (bShow && m_pRenderDebugViewTask == nullptr)
  {
    m_pRenderDebugViewTask = PLASMA_DEFAULT_NEW(RenderDebugViewTask);
  }
}

void plBakedProbesComponent::SetShowDebugProbes(bool bShow)
{
  if (m_bShowDebugProbes != bShow)
  {
    m_bShowDebugProbes = bShow;

    if (IsActiveAndInitialized())
    {
      plRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
    }
  }
}

void plBakedProbesComponent::SetUseTestPosition(bool bUse)
{
  if (m_bUseTestPosition != bUse)
  {
    m_bUseTestPosition = bUse;

    if (IsActiveAndInitialized())
    {
      plRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
    }
  }
}

void plBakedProbesComponent::SetTestPosition(const plVec3& vPos)
{
  m_vTestPosition = vPos;

  if (IsActiveAndInitialized())
  {
    plRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void plBakedProbesComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg)
{
  ref_msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? plDefaultSpatialDataCategories::RenderDynamic : plDefaultSpatialDataCategories::RenderStatic);
}

void plBakedProbesComponent::OnExtractRenderData(plMsgExtractRenderData& ref_msg) const
{
  if (!m_bShowDebugProbes)
    return;

  // Don't trigger probe rendering in shadow or reflection views.
  if (ref_msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow ||
      ref_msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Reflection)
    return;

  auto pModule = GetWorld()->GetModule<plBakedProbesWorldModule>();
  if (!pModule->HasProbeData())
    return;

  const plGameObject* pOwner = GetOwner();
  auto pManager = static_cast<const plBakedProbesComponentManager*>(GetOwningManager());

  auto addProbeRenderData = [&](const plVec3& vPosition, plCompressedSkyVisibility skyVisibility, plRenderData::Caching::Enum caching) {
    plTransform transform = plTransform::IdentityTransform();
    transform.m_vPosition = vPosition;

    plColor encodedSkyVisibility = plColor::Black;
    encodedSkyVisibility.r = *reinterpret_cast<const float*>(&skyVisibility);

    plMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plMeshRenderData>(pOwner);
    {
      pRenderData->m_GlobalTransform = transform;
      pRenderData->m_GlobalBounds.SetInvalid();
      pRenderData->m_hMesh = pManager->m_hDebugSphere;
      pRenderData->m_hMaterial = pManager->m_hDebugMaterial;
      pRenderData->m_Color = encodedSkyVisibility;
      pRenderData->m_uiSubMeshIndex = 0;
      pRenderData->m_uiUniqueID = plRenderComponent::GetUniqueIdForRendering(this, 0);

      pRenderData->FillBatchIdAndSortingKey();
    }

    ref_msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::SimpleOpaque, caching);
  };

  if (m_bUseTestPosition)
  {
    plBakedProbesWorldModule::ProbeIndexData indexData;
    if (pModule->GetProbeIndexData(m_vTestPosition, plVec3::UnitZAxis(), indexData).Failed())
      return;

    if (true)
    {
      plResourceLock<plProbeTreeSectorResource> pProbeTree(pModule->m_hProbeTree, plResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pProbeTree.GetAcquireResult() != plResourceAcquireResult::Final)
        return;

      for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(indexData.m_probeIndices); ++i)
      {
        plVec3 pos = pProbeTree->GetProbePositions()[indexData.m_probeIndices[i]];
        plDebugRenderer::DrawCross(ref_msg.m_pView->GetHandle(), pos, 0.5f, plColor::Yellow);

        pos.z += 0.5f;
        plDebugRenderer::Draw3DText(ref_msg.m_pView->GetHandle(), plFmt("Weight: {}", indexData.m_probeWeights[i]), pos, plColor::Yellow);
      }
    }

    plCompressedSkyVisibility skyVisibility = plBakingUtils::CompressSkyVisibility(pModule->GetSkyVisibility(indexData));

    addProbeRenderData(m_vTestPosition, skyVisibility, plRenderData::Caching::Never);
  }
  else
  {
    plResourceLock<plProbeTreeSectorResource> pProbeTree(pModule->m_hProbeTree, plResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pProbeTree.GetAcquireResult() != plResourceAcquireResult::Final)
      return;

    auto probePositions = pProbeTree->GetProbePositions();
    auto skyVisibility = pProbeTree->GetSkyVisibility();

    for (plUInt32 uiProbeIndex = 0; uiProbeIndex < probePositions.GetCount(); ++uiProbeIndex)
    {
      addProbeRenderData(probePositions[uiProbeIndex], skyVisibility[uiProbeIndex], plRenderData::Caching::IfStatic);
    }
  }
}

void plBakedProbesComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  if (m_Settings.Serialize(s).Failed())
    return;

  s << m_sProbeTreeResourcePrefix;
  s << m_bShowDebugOverlay;
  s << m_bShowDebugProbes;
  s << m_bUseTestPosition;
  s << m_vTestPosition;
}

void plBakedProbesComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  if (m_Settings.Deserialize(s).Failed())
    return;

  s >> m_sProbeTreeResourcePrefix;
  s >> m_bShowDebugOverlay;
  s >> m_bShowDebugProbes;
  s >> m_bUseTestPosition;
  s >> m_vTestPosition;
}

void plBakedProbesComponent::RenderDebugOverlay()
{
  plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView);
  if (pView == nullptr)
    return;

  plBakingInterface* pBakingInterface = plSingletonRegistry::GetSingletonInstance<plBakingInterface>();
  if (pBakingInterface == nullptr)
  {
    plDebugRenderer::Draw2DText(pView->GetHandle(), "Baking Plugin not loaded", plVec2I32(10, 10), plColor::OrangeRed);
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plRectFloat viewport = pView->GetViewport();
  plUInt32 uiWidth = static_cast<plUInt32>(plMath::Ceil(viewport.width / 3.0f));
  plUInt32 uiHeight = static_cast<plUInt32>(plMath::Ceil(viewport.height / 3.0f));

  plMat4 inverseViewProjection = pView->GetInverseViewProjectionMatrix(plCameraEye::Left);

  if (m_pRenderDebugViewTask->m_InverseViewProjection != inverseViewProjection ||
      m_pRenderDebugViewTask->m_uiWidth != uiWidth || m_pRenderDebugViewTask->m_uiHeight != uiHeight)
  {
    plTaskSystem::CancelTask(m_pRenderDebugViewTask).IgnoreResult();

    m_pRenderDebugViewTask->m_pBakingInterface = pBakingInterface;
    m_pRenderDebugViewTask->m_pWorld = GetWorld();
    m_pRenderDebugViewTask->m_InverseViewProjection = inverseViewProjection;
    m_pRenderDebugViewTask->m_uiWidth = uiWidth;
    m_pRenderDebugViewTask->m_uiHeight = uiHeight;
    m_pRenderDebugViewTask->m_PixelData.SetCount(uiWidth * uiHeight, plColor::Red);
    m_pRenderDebugViewTask->m_bHasNewData = false;

    plTaskSystem::StartSingleTask(m_pRenderDebugViewTask, plTaskPriority::LongRunning);
  }

  plUInt32 uiTextureWidth = 0;
  plUInt32 uiTextureHeight = 0;
  if (const plGALTexture* pTexture = pDevice->GetTexture(m_hDebugViewTexture))
  {
    uiTextureWidth = pTexture->GetDescription().m_uiWidth;
    uiTextureHeight = pTexture->GetDescription().m_uiHeight;
  }

  if (uiTextureWidth != uiWidth || uiTextureHeight != uiHeight)
  {
    if (!m_hDebugViewTexture.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hDebugViewTexture);
    }

    plGALTextureCreationDescription desc;
    desc.m_uiWidth = uiWidth;
    desc.m_uiHeight = uiHeight;
    desc.m_Format = plGALResourceFormat::RGBAUByteNormalizedsRGB;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hDebugViewTexture = pDevice->CreateTexture(desc);
  }

  plRectFloat rectInPixel = plRectFloat(10.0f, 10.0f, static_cast<float>(uiWidth), static_cast<float>(uiHeight));

  plDebugRenderer::Draw2DRectangle(pView->GetHandle(), rectInPixel, 0.0f, plColor::White, pDevice->GetDefaultResourceView(m_hDebugViewTexture));
}

void plBakedProbesComponent::OnObjectCreated(const plAbstractObjectNode& node)
{
  plStringBuilder sPrefix;
  sPrefix.Format(":project/AssetCache/Generated/{0}", node.GetGuid());

  m_sProbeTreeResourcePrefix.Assign(sPrefix);
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesComponent);
