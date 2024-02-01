#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/Implementation/PlacementTile.h>
#include <ProcGenPlugin/Components/ProcPlacementComponent.h>
#include <ProcGenPlugin/Tasks/FindPlacementTilesTask.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PlacementTask.h>
#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

using namespace plProcGenInternal;

plCVarInt cvar_ProcGenProcessingMaxTiles("ProcGen.Processing.MaxTiles", 8, plCVarFlags::Default, "Maximum number of tiles in process");
plCVarInt cvar_ProcGenProcessingMaxNewObjectsPerFrame("ProcGen.Processing.MaxNewObjectsPerFrame", 256, plCVarFlags::Default, "Maximum number of objects placed per frame");
plCVarBool cvar_ProcGenVisTiles("ProcGen.VisTiles", false, plCVarFlags::Default, "Enables debug visualization of procedural placement tiles");

plProcPlacementComponentManager::plProcPlacementComponentManager(plWorld* pWorld)
  : plComponentManager<plProcPlacementComponent, plBlockStorageType::Compact>(pWorld)
{
}

plProcPlacementComponentManager::~plProcPlacementComponentManager() = default;

void plProcPlacementComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plProcPlacementComponentManager::FindTiles, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plProcPlacementComponentManager::PreparePlace, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::Async;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plProcPlacementComponentManager::PlaceObjects, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostAsync;

    this->RegisterUpdateFunction(desc);
  }

  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plProcPlacementComponentManager::OnResourceEvent, this));
}

void plProcPlacementComponentManager::Deinitialize()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plProcPlacementComponentManager::OnResourceEvent, this));

  for (auto& activeTile : m_ActiveTiles)
  {
    activeTile.Deinitialize(*GetWorld());
  }
  m_ActiveTiles.Clear();

  SUPER::Deinitialize();
}

void plProcPlacementComponentManager::FindTiles(const plWorldModule::UpdateContext& context)
{
  // Update resource data
  bool bAnyObjectsRemoved = false;

  for (auto& hComponent : m_ComponentsToUpdate)
  {
    plProcPlacementComponent* pComponent = nullptr;
    if (!TryGetComponent(hComponent, pComponent))
    {
      continue;
    }

    RemoveTilesForComponent(pComponent, &bAnyObjectsRemoved);

    plResourceLock<plProcGenGraphResource> pResource(pComponent->m_hResource, plResourceAcquireMode::BlockTillLoaded);
    auto outputs = pResource->GetPlacementOutputs();

    pComponent->m_OutputContexts.Clear();
    for (plUInt32 uiIndex = 0; uiIndex < outputs.GetCount(); ++uiIndex)
    {
      const auto& pOutput = outputs[uiIndex];
      if (pOutput->IsValid())
      {
        auto& outputContext = pComponent->m_OutputContexts.ExpandAndGetRef();
        outputContext.m_pOutput = pOutput;
        outputContext.m_pUpdateTilesTask = PL_DEFAULT_NEW(FindPlacementTilesTask, pComponent, uiIndex);
      }
    }
  }
  m_ComponentsToUpdate.Clear();

  // If we removed any objects during resource update do nothing else this frame so objects are actually deleted before we place new ones.
  if (bAnyObjectsRemoved)
  {
    return;
  }

  // Schedule find tiles tasks
  m_UpdateTilesTaskGroupID = plTaskSystem::CreateTaskGroup(plTaskPriority::EarlyThisFrame);

  for (auto& visibleComponent : m_VisibleComponents)
  {
    plProcPlacementComponent* pComponent = nullptr;
    if (!TryGetComponent(visibleComponent.m_hComponent, pComponent))
    {
      continue;
    }

    auto& outputContexts = pComponent->m_OutputContexts;
    for (auto& outputContext : outputContexts)
    {
      outputContext.m_pUpdateTilesTask->AddCameraPosition(visibleComponent.m_vCameraPosition);

      if (outputContext.m_pUpdateTilesTask->IsTaskFinished())
      {
        plTaskSystem::AddTaskToGroup(m_UpdateTilesTaskGroupID, outputContext.m_pUpdateTilesTask);
      }
    }
  }

  plTaskSystem::StartTaskGroup(m_UpdateTilesTaskGroupID);
}

void plProcPlacementComponentManager::PreparePlace(const plWorldModule::UpdateContext& context)
{
  // Find new active tiles and remove old ones
  {
    PL_PROFILE_SCOPE("Add new/remove old tiles");

    plTaskSystem::WaitForGroup(m_UpdateTilesTaskGroupID);
    m_UpdateTilesTaskGroupID.Invalidate();

    for (auto& visibleComponent : m_VisibleComponents)
    {
      plProcPlacementComponent* pComponent = nullptr;
      if (!TryGetComponent(visibleComponent.m_hComponent, pComponent))
      {
        continue;
      }

      auto& outputContexts = pComponent->m_OutputContexts;
      for (auto& outputContext : outputContexts)
      {
        auto oldTiles = outputContext.m_pUpdateTilesTask->GetOldTiles();
        for (plUInt64 uiOldTileKey : oldTiles)
        {
          plProcPlacementComponent::OutputContext::TileIndexAndAge tileIndex;
          if (outputContext.m_TileIndices.Remove(uiOldTileKey, &tileIndex))
          {
            if (tileIndex.m_uiIndex != NewTileIndex)
            {
              DeallocateTile(tileIndex.m_uiIndex);
            }
          }

          // Also remove from new tiles list
          for (plUInt32 i = 0; i < m_NewTiles.GetCount(); ++i)
          {
            auto& newTile = m_NewTiles[i];
            plUInt64 uiTileKey = GetTileKey(newTile.m_iPosX, newTile.m_iPosY);
            if (uiTileKey == uiOldTileKey)
            {
              m_NewTiles.RemoveAtAndSwap(i);
              break;
            }
          }
        }

        m_NewTiles.PushBackRange(outputContext.m_pUpdateTilesTask->GetNewTiles());
      }
    }

    // Sort new tiles
    {
      PL_PROFILE_SCOPE("Sort new tiles");

      // Update distance to camera
      for (auto& newTile : m_NewTiles)
      {
        plVec2 tilePos = plVec2((float)newTile.m_iPosX, (float)newTile.m_iPosY);
        newTile.m_fDistanceToCamera = plMath::MaxValue<float>();

        for (auto& visibleComponent : m_VisibleComponents)
        {
          plVec2 cameraPos = visibleComponent.m_vCameraPosition.GetAsVec2() / newTile.m_fTileSize;

          float fDistance = (tilePos - cameraPos).GetLengthSquared();
          newTile.m_fDistanceToCamera = plMath::Min(newTile.m_fDistanceToCamera, fDistance);
        }
      }

      // Sort by distance, larger distances come first since new tiles are processed in reverse order.
      m_NewTiles.Sort([](auto& ref_tileA, auto& ref_tileB) { return ref_tileA.m_fDistanceToCamera > ref_tileB.m_fDistanceToCamera; });
    }

    ClearVisibleComponents();
  }

  // Debug draw tiles
  if (cvar_ProcGenVisTiles)
  {
    plStringBuilder sb;
    sb.SetFormat("Procedural Placement Stats:\nNum Tiles to process: {}", m_NewTiles.GetCount());

    plColor textColor = plColorScheme::LightUI(plColorScheme::Grape);
    plDebugRenderer::DrawInfoText(GetWorld(), plDebugTextPlacement::TopLeft, "ProcPlaceStats", sb, textColor);

    for (plUInt32 i = 0; i < m_NewTiles.GetCount(); ++i)
    {
      DebugDrawTile(m_NewTiles[i], textColor, m_NewTiles.GetCount() - i - 1);
    }

    for (auto& activeTile : m_ActiveTiles)
    {
      if (!activeTile.IsValid())
        continue;

      DebugDrawTile(activeTile.GetDesc(), activeTile.GetDebugColor());
    }
  }

  // Allocate new tiles and placement tasks
  {
    PL_PROFILE_SCOPE("Allocate new tiles");

    while (!m_NewTiles.IsEmpty() && GetNumAllocatedProcessingTasks() < (plUInt32)cvar_ProcGenProcessingMaxTiles)
    {
      const PlacementTileDesc& newTile = m_NewTiles.PeekBack();

      plProcPlacementComponent* pComponent = nullptr;
      if (TryGetComponent(newTile.m_hComponent, pComponent))
      {
        auto& pOutput = pComponent->m_OutputContexts[newTile.m_uiOutputIndex].m_pOutput;
        plUInt32 uiNewTileIndex = AllocateTile(newTile, pOutput);

        AllocateProcessingTask(uiNewTileIndex);
      }

      m_NewTiles.PopBack();
    }
  }

  const plWorld* pWorld = GetWorld();

  // Update processing tasks
  if (GetWorldSimulationEnabled())
  {
    {
      PL_PROFILE_SCOPE("Prepare processing tasks");

      plTaskGroupID prepareTaskGroupID = plTaskSystem::CreateTaskGroup(plTaskPriority::EarlyThisFrame);

      for (auto& processingTask : m_ProcessingTasks)
      {
        if (!processingTask.IsValid() || processingTask.IsScheduled())
          continue;

        auto& activeTile = m_ActiveTiles[processingTask.m_uiTileIndex];
        activeTile.PreparePlacementData(pWorld, pWorld->GetModuleReadOnly<plPhysicsWorldModuleInterface>(), *processingTask.m_pData);

        plTaskSystem::AddTaskToGroup(prepareTaskGroupID, processingTask.m_pPrepareTask);
      }

      plTaskSystem::StartTaskGroup(prepareTaskGroupID);
      plTaskSystem::WaitForGroup(prepareTaskGroupID);
    }

    {
      PL_PROFILE_SCOPE("Kickoff placement tasks");

      for (auto& processingTask : m_ProcessingTasks)
      {
        if (!processingTask.IsValid() || processingTask.IsScheduled())
          continue;

        processingTask.m_uiScheduledFrame = plRenderWorld::GetFrameCounter();
        processingTask.m_PlacementTaskGroupID = plTaskSystem::StartSingleTask(processingTask.m_pPlacementTask, plTaskPriority::LongRunningHighPriority);
      }
    }
  }
}

void plProcPlacementComponentManager::PlaceObjects(const plWorldModule::UpdateContext& context)
{
  m_SortedProcessingTasks.Clear();
  for (plUInt32 i = 0; i < m_ProcessingTasks.GetCount(); ++i)
  {
    auto& sortedTask = m_SortedProcessingTasks.ExpandAndGetRef();
    sortedTask.m_uiScheduledFrame = m_ProcessingTasks[i].m_uiScheduledFrame;
    sortedTask.m_uiTaskIndex = i;
  }

  m_SortedProcessingTasks.Sort([](auto& ref_taskA, auto& ref_taskB) { return ref_taskA.m_uiScheduledFrame < ref_taskB.m_uiScheduledFrame; });

  plUInt32 uiTotalNumPlacedObjects = 0;

  for (auto& sortedTask : m_SortedProcessingTasks)
  {
    auto& task = m_ProcessingTasks[sortedTask.m_uiTaskIndex];
    if (!task.IsValid() || !task.IsScheduled())
      continue;

    if (task.m_pPlacementTask->IsTaskFinished())
    {
      plUInt32 uiPlacedObjects = 0;

      plUInt32 uiTileIndex = task.m_uiTileIndex;
      auto& activeTile = m_ActiveTiles[uiTileIndex];

      auto& tileDesc = activeTile.GetDesc();
      plProcPlacementComponent* pComponent = nullptr;
      if (TryGetComponent(tileDesc.m_hComponent, pComponent))
      {
        auto& outputContext = pComponent->m_OutputContexts[tileDesc.m_uiOutputIndex];

        plUInt64 uiTileKey = GetTileKey(tileDesc.m_iPosX, tileDesc.m_iPosY);
        if (auto pTile = outputContext.m_TileIndices.GetValue(uiTileKey))
        {
          uiPlacedObjects = activeTile.PlaceObjects(*GetWorld(), task.m_pPlacementTask->GetOutputTransforms());

          pTile->m_uiIndex = uiPlacedObjects > 0 ? uiTileIndex : EmptyTileIndex;
          pTile->m_uiLastSeenFrame = plRenderWorld::GetFrameCounter();
        }
      }

      if (uiPlacedObjects == 0)
      {
        // mark tile for re-use
        DeallocateTile(uiTileIndex);
      }

      // mark task for re-use
      DeallocateProcessingTask(sortedTask.m_uiTaskIndex);

      uiTotalNumPlacedObjects += uiPlacedObjects;
    }

    if (uiTotalNumPlacedObjects >= (plUInt32)cvar_ProcGenProcessingMaxNewObjectsPerFrame)
    {
      break;
    }
  }
}

void plProcPlacementComponentManager::DebugDrawTile(const plProcGenInternal::PlacementTileDesc& desc, const plColor& color, plUInt32 uiQueueIndex)
{
  const plProcPlacementComponent* pComponent = nullptr;
  if (!TryGetComponent(desc.m_hComponent, pComponent))
    return;

  plBoundingBox bbox = desc.GetBoundingBox();
  plDebugRenderer::DrawLineBox(GetWorld(), bbox, color);

  plUInt64 uiAge = -1;
  auto& outputContext = pComponent->m_OutputContexts[desc.m_uiOutputIndex];
  if (auto pTile = outputContext.m_TileIndices.GetValue(GetTileKey(desc.m_iPosX, desc.m_iPosY)))
  {
    uiAge = plRenderWorld::GetFrameCounter() - pTile->m_uiLastSeenFrame;
  }

  plStringBuilder sb;
  if (uiQueueIndex != plInvalidIndex)
  {
    sb.SetFormat("Queue Index: {}\n", uiQueueIndex);
  }
  sb.AppendFormat("Age: {}\nDistance: {}", uiAge, desc.m_fDistanceToCamera);
  plDebugRenderer::Draw3DText(GetWorld(), sb, bbox.GetCenter(), color);
}

void plProcPlacementComponentManager::AddComponent(plProcPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
}

void plProcPlacementComponentManager::RemoveComponent(plProcPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  RemoveTilesForComponent(pComponent);
}

plUInt32 plProcPlacementComponentManager::AllocateTile(const PlacementTileDesc& desc, plSharedPtr<const PlacementOutput>& pOutput)
{
  plUInt32 uiNewTileIndex = plInvalidIndex;
  if (!m_FreeTiles.IsEmpty())
  {
    uiNewTileIndex = m_FreeTiles.PeekBack();
    m_FreeTiles.PopBack();
  }
  else
  {
    uiNewTileIndex = m_ActiveTiles.GetCount();
    m_ActiveTiles.ExpandAndGetRef();
  }

  m_ActiveTiles[uiNewTileIndex].Initialize(desc, pOutput);
  return uiNewTileIndex;
}

void plProcPlacementComponentManager::DeallocateTile(plUInt32 uiTileIndex)
{
  m_ActiveTiles[uiTileIndex].Deinitialize(*GetWorld());
  m_FreeTiles.PushBack(uiTileIndex);
}

plUInt32 plProcPlacementComponentManager::AllocateProcessingTask(plUInt32 uiTileIndex)
{
  plUInt32 uiNewTaskIndex = plInvalidIndex;
  if (!m_FreeProcessingTasks.IsEmpty())
  {
    uiNewTaskIndex = m_FreeProcessingTasks.PeekBack();
    m_FreeProcessingTasks.PopBack();
  }
  else
  {
    uiNewTaskIndex = m_ProcessingTasks.GetCount();
    auto& newTask = m_ProcessingTasks.ExpandAndGetRef();

    newTask.m_pData = PL_DEFAULT_NEW(PlacementData);

    plStringBuilder sName;
    sName.SetFormat("Prepare Task {}", uiNewTaskIndex);
    newTask.m_pPrepareTask = PL_DEFAULT_NEW(PreparePlacementTask, newTask.m_pData.Borrow(), sName);

    sName.SetFormat("Placement Task {}", uiNewTaskIndex);
    newTask.m_pPlacementTask = PL_DEFAULT_NEW(PlacementTask, newTask.m_pData.Borrow(), sName);
  }

  m_ProcessingTasks[uiNewTaskIndex].m_uiTileIndex = uiTileIndex;
  return uiNewTaskIndex;
}

void plProcPlacementComponentManager::DeallocateProcessingTask(plUInt32 uiTaskIndex)
{
  auto& task = m_ProcessingTasks[uiTaskIndex];
  if (task.IsScheduled())
  {
    plTaskSystem::WaitForGroup(task.m_PlacementTaskGroupID);
  }

  task.m_pData->Clear();
  task.m_pPrepareTask->Clear();
  task.m_pPlacementTask->Clear();
  task.Invalidate();

  m_FreeProcessingTasks.PushBack(uiTaskIndex);
}

plUInt32 plProcPlacementComponentManager::GetNumAllocatedProcessingTasks() const
{
  return m_ProcessingTasks.GetCount() - m_FreeProcessingTasks.GetCount();
}

void plProcPlacementComponentManager::RemoveTilesForComponent(plProcPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved /*= nullptr*/)
{
  plComponentHandle hComponent = pComponent->GetHandle();

  for (plUInt32 uiNewTileIndex = 0; uiNewTileIndex < m_NewTiles.GetCount(); ++uiNewTileIndex)
  {
    if (m_NewTiles[uiNewTileIndex].m_hComponent == hComponent)
    {
      m_NewTiles.RemoveAtAndSwap(uiNewTileIndex);
      --uiNewTileIndex;
    }
  }

  for (plUInt32 uiTileIndex = 0; uiTileIndex < m_ActiveTiles.GetCount(); ++uiTileIndex)
  {
    auto& activeTile = m_ActiveTiles[uiTileIndex];
    if (!activeTile.IsValid())
      continue;

    auto& tileDesc = activeTile.GetDesc();
    if (tileDesc.m_hComponent == hComponent)
    {
      if (out_bAnyObjectsRemoved != nullptr && !m_ActiveTiles[uiTileIndex].GetPlacedObjects().IsEmpty())
      {
        *out_bAnyObjectsRemoved = true;
      }

      DeallocateTile(uiTileIndex);

      for (plUInt32 i = 0; i < m_ProcessingTasks.GetCount(); ++i)
      {
        auto& taskInfo = m_ProcessingTasks[i];
        if (taskInfo.m_uiTileIndex == uiTileIndex)
        {
          DeallocateProcessingTask(i);
        }
      }
    }
  }
}

void plProcPlacementComponentManager::OnResourceEvent(const plResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != plResourceEvent::Type::ResourceContentUnloading || resourceEvent.m_pResource->GetReferenceCount() == 0)
    return;

  if (auto pResource = plDynamicCast<const plProcGenGraphResource*>(resourceEvent.m_pResource))
  {
    plProcGenGraphResourceHandle hResource = pResource->GetResourceHandle();

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource && !m_ComponentsToUpdate.Contains(it->GetHandle()))
      {
        m_ComponentsToUpdate.PushBack(it->GetHandle());
      }
    }
  }
}

void plProcPlacementComponentManager::AddVisibleComponent(const plComponentHandle& hComponent, const plVec3& cameraPosition, const plVec3& cameraDirection) const
{
  PL_LOCK(m_VisibleComponentsMutex);

  for (auto& visibleComponent : m_VisibleComponents)
  {
    if (visibleComponent.m_hComponent == hComponent && visibleComponent.m_vCameraPosition == cameraPosition && visibleComponent.m_vCameraDirection == cameraDirection)
    {
      return;
    }
  }

  auto& visibleComponent = m_VisibleComponents.ExpandAndGetRef();
  visibleComponent.m_hComponent = hComponent;
  visibleComponent.m_vCameraPosition = cameraPosition;
  visibleComponent.m_vCameraDirection = cameraDirection;
}

void plProcPlacementComponentManager::ClearVisibleComponents()
{
  m_VisibleComponents.Clear();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plProcGenBoxExtents, plNoBase, 1, plRTTIDefaultAllocator<plProcGenBoxExtents>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Offset", m_vOffset),
    PL_MEMBER_PROPERTY("Rotation", m_Rotation),
    PL_MEMBER_PROPERTY("Extents", m_vExtents)->AddAttributes(new plDefaultValueAttribute(plVec3(10.0f)), new plClampValueAttribute(plVec3(0), plVariant())),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plBoxManipulatorAttribute("Extents", 1.0f, false, "Offset", "Rotation"),
    new plBoxVisualizerAttribute("Extents", 1.0f, plColorScheme::LightUI(plColorScheme::Blue), nullptr, plVisualizerAnchor::Center, plVec3(1.0f), "Offset", "Rotation"),
    new plTransformManipulatorAttribute("Offset", "Rotation"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_COMPONENT_TYPE(plProcPlacementComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_ProcGen_Graph")),
    PL_ARRAY_ACCESSOR_PROPERTY("BoxExtents", BoxExtents_GetCount, BoxExtents_GetValue, BoxExtents_SetValue, BoxExtents_Insert, BoxExtents_Remove),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Construction/Procedural Generation"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plProcPlacementComponent::plProcPlacementComponent() = default;
plProcPlacementComponent::~plProcPlacementComponent() = default;
plProcPlacementComponent& plProcPlacementComponent::operator=(plProcPlacementComponent&& other) = default;

void plProcPlacementComponent::OnActivated()
{
  UpdateBoundsAndTiles();
}

void plProcPlacementComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  m_Bounds.Clear();
  m_OutputContexts.Clear();

  auto pManager = static_cast<plProcPlacementComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(this);
}

void plProcPlacementComponent::SetResourceFile(const char* szFile)
{
  plProcGenGraphResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plProcGenGraphResource>(szFile);
    plResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

const char* plProcPlacementComponent::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void plProcPlacementComponent::SetResource(const plProcGenGraphResourceHandle& hResource)
{
  auto pManager = static_cast<plProcPlacementComponentManager*>(GetOwningManager());

  if (IsActiveAndInitialized())
  {
    pManager->RemoveComponent(this);
  }

  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    pManager->AddComponent(this);
  }
}

void plProcPlacementComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg)
{
  if (m_BoxExtents.IsEmpty())
    return;

  plBoundingBoxSphere bounds = plBoundingBoxSphere::MakeInvalid();

  for (auto& boxExtent : m_BoxExtents)
  {
    plBoundingBoxSphere localBox = plBoundingBoxSphere::MakeFromBox(plBoundingBox::MakeFromMinMax(-boxExtent.m_vExtents * 0.5f, boxExtent.m_vExtents * 0.5f));
    localBox.Transform(plTransform(boxExtent.m_vOffset, boxExtent.m_Rotation).GetAsMat4());

    bounds.ExpandToInclude(localBox);
  }

  ref_msg.AddBounds(bounds, GetOwner()->IsDynamic() ? plDefaultSpatialDataCategories::RenderDynamic : plDefaultSpatialDataCategories::RenderStatic);
}

void plProcPlacementComponent::OnMsgExtractRenderData(plMsgExtractRenderData& ref_msg) const
{
  // Don't extract render data for selection or in shadow views.
  if (ref_msg.m_OverrideCategory != plInvalidRenderDataCategory)
    return;

  if (ref_msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::MainView || ref_msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::EditorView)
  {
    const plCamera* pCamera = ref_msg.m_pView->GetCullingCamera();

    plVec3 cameraPosition = pCamera->GetCenterPosition();
    plVec3 cameraDirection = pCamera->GetCenterDirForwards();

    if (m_hResource.IsValid())
    {
      auto pManager = static_cast<const plProcPlacementComponentManager*>(GetOwningManager());
      pManager->AddVisibleComponent(GetHandle(), cameraPosition, cameraDirection);
    }
  }
}

void plProcPlacementComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_BoxExtents).IgnoreResult();
}

void plProcPlacementComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  s.ReadArray(m_BoxExtents).IgnoreResult();
}

plUInt32 plProcPlacementComponent::BoxExtents_GetCount() const
{
  return m_BoxExtents.GetCount();
}

const plProcGenBoxExtents& plProcPlacementComponent::BoxExtents_GetValue(plUInt32 uiIndex) const
{
  return m_BoxExtents[uiIndex];
}

void plProcPlacementComponent::BoxExtents_SetValue(plUInt32 uiIndex, const plProcGenBoxExtents& value)
{
  m_BoxExtents.EnsureCount(uiIndex + 1);
  m_BoxExtents[uiIndex] = value;

  UpdateBoundsAndTiles();
}

void plProcPlacementComponent::BoxExtents_Insert(plUInt32 uiIndex, const plProcGenBoxExtents& value)
{
  m_BoxExtents.Insert(value, uiIndex);

  UpdateBoundsAndTiles();
}

void plProcPlacementComponent::BoxExtents_Remove(plUInt32 uiIndex)
{
  m_BoxExtents.RemoveAtAndCopy(uiIndex);

  UpdateBoundsAndTiles();
}

void plProcPlacementComponent::UpdateBoundsAndTiles()
{
  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<plProcPlacementComponentManager*>(GetOwningManager());

    pManager->RemoveComponent(this);

    GetOwner()->UpdateLocalBounds();

    m_Bounds.Clear();
    m_OutputContexts.Clear();

    plSimdTransform ownerTransform = GetOwner()->GetGlobalTransformSimd();
    for (auto& boxExtent : m_BoxExtents)
    {
      plSimdTransform localBoxTransform;
      localBoxTransform.m_Position = plSimdConversion::ToVec3(boxExtent.m_vOffset);
      localBoxTransform.m_Rotation = plSimdConversion::ToQuat(boxExtent.m_Rotation);
      localBoxTransform.m_Scale = plSimdConversion::ToVec3(boxExtent.m_vExtents * 0.5f);

      plSimdTransform finalBoxTransform;
      finalBoxTransform = plSimdTransform::MakeGlobalTransform(ownerTransform, localBoxTransform);

      plSimdMat4f finalBoxMat = finalBoxTransform.GetAsMat4();

      plSimdBBox globalBox(plSimdVec4f(-1.0f), plSimdVec4f(1.0f));
      globalBox.Transform(finalBoxMat);

      auto& bounds = m_Bounds.ExpandAndGetRef();
      bounds.m_GlobalBoundingBox = globalBox;
      bounds.m_GlobalToLocalBoxTransform = finalBoxMat.GetInverse();
    }

    pManager->AddComponent(this);
  }
}

//////////////////////////////////////////////////////////////////////////

plResult plProcGenBoxExtents::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_vOffset;
  inout_stream << m_Rotation;
  inout_stream << m_vExtents;

  return PL_SUCCESS;
}

plResult plProcGenBoxExtents::Deserialize(plStreamReader& inout_stream)
{
  inout_stream >> m_vOffset;
  inout_stream >> m_Rotation;
  inout_stream >> m_vExtents;

  return PL_SUCCESS;
}
