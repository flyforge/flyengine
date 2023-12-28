#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcVertexColorRenderData, 1, plRTTIDefaultAllocator<plProcVertexColorRenderData>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plProcVertexColorRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_hVertexColorBuffer.GetInternalID().m_Data);
}

//////////////////////////////////////////////////////////////////////////

enum
{
  BUFFER_ACCESS_OFFSET_BITS = 28,
  BUFFER_ACCESS_OFFSET_MASK = (1 << BUFFER_ACCESS_OFFSET_BITS) - 1,

  VERTEX_COLOR_BUFFER_SIZE = 1024 * 1024
};

using namespace plProcGenInternal;

plProcVertexColorComponentManager::plProcVertexColorComponentManager(plWorld* pWorld)
  : plComponentManager<plProcVertexColorComponent, plBlockStorageType::Compact>(pWorld)
{
}

plProcVertexColorComponentManager::~plProcVertexColorComponentManager() = default;

void plProcVertexColorComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    plGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(plUInt32);
    desc.m_uiTotalSize = desc.m_uiStructSize * VERTEX_COLOR_BUFFER_SIZE;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hVertexColorBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc);

    m_VertexColorData.SetCountUninitialized(VERTEX_COLOR_BUFFER_SIZE);
  }

  {
    auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plProcVertexColorComponentManager::UpdateVertexColors, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  plRenderWorld::GetRenderEvent().AddEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnRenderEvent, this));
  plRenderWorld::GetExtractionEvent().AddEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnExtractionEvent, this));

  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnResourceEvent, this));

  // TODO: also do this in plProcPlacementComponentManager
  plProcVolumeComponent::GetAreaInvalidatedEvent().AddEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnAreaInvalidated, this));
}

void plProcVertexColorComponentManager::Deinitialize()
{
  plGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexColorBuffer);
  m_hVertexColorBuffer.Invalidate();

  plRenderWorld::GetRenderEvent().RemoveEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnRenderEvent, this));
  plRenderWorld::GetExtractionEvent().RemoveEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnExtractionEvent, this));

  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnResourceEvent, this));

  plProcVolumeComponent::GetAreaInvalidatedEvent().RemoveEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnAreaInvalidated, this));

  SUPER::Deinitialize();
}

void plProcVertexColorComponentManager::UpdateVertexColors(const plWorldModule::UpdateContext& context)
{
  m_UpdateTaskGroupID = plTaskSystem::CreateTaskGroup(plTaskPriority::EarlyThisFrame);
  m_ModifiedDataRange.Reset();

  for (const auto& componentToUpdate : m_ComponentsToUpdate)
  {
    plProcVertexColorComponent* pComponent = nullptr;
    if (!TryGetComponent(componentToUpdate, pComponent))
      continue;

    UpdateComponentVertexColors(pComponent);

    // Invalidate all cached render data so the new buffer handle and offset are propagated to the render data
    plRenderWorld::DeleteCachedRenderData(pComponent->GetOwner()->GetHandle(), pComponent->GetHandle());
  }

  m_ComponentsToUpdate.Clear();

  plTaskSystem::StartTaskGroup(m_UpdateTaskGroupID);
}

void plProcVertexColorComponentManager::UpdateComponentVertexColors(plProcVertexColorComponent* pComponent)
{
  pComponent->m_Outputs.Clear();
  plHybridArray<plProcVertexColorMapping, 2> outputMappings;

  {
    plResourceLock<plProcGenGraphResource> pResource(pComponent->m_hResource, plResourceAcquireMode::BlockTillLoaded);
    auto outputs = pResource->GetVertexColorOutputs();

    for (auto& outputDesc : pComponent->m_OutputDescs)
    {
      if (!outputDesc.m_sName.IsEmpty())
      {
        bool bOutputFound = false;
        for (auto& pOutput : outputs)
        {
          if (pOutput->m_sName == outputDesc.m_sName)
          {
            pComponent->m_Outputs.PushBack(pOutput);
            bOutputFound = true;
            break;
          }
        }

        if (!bOutputFound)
        {
          pComponent->m_Outputs.PushBack(nullptr);
          plLog::Error("Vertex Color Output with name '{}' not found in Proc Gen Graph '{}'", outputDesc.m_sName, pResource->GetResourceID());
        }
      }
      else
      {
        pComponent->m_Outputs.PushBack(nullptr);
      }

      outputMappings.PushBack(outputDesc.m_Mapping);
    }
  }

  if (!pComponent->HasValidOutputs())
    return;

  const char* szMesh = pComponent->GetMeshFile();
  if (plStringUtils::IsNullOrEmpty(szMesh))
    return;

  plCpuMeshResourceHandle hCpuMesh = plResourceManager::LoadResource<plCpuMeshResource>(szMesh);
  plResourceLock<plCpuMeshResource> pCpuMesh(hCpuMesh, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCpuMesh.GetAcquireResult() != plResourceAcquireResult::Final)
  {
    plLog::Warning("Failed to retrieve CPU mesh '{}'", szMesh);
    return;
  }

  const auto& mbDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();
  const plUInt32 uiNumOutputs = pComponent->m_Outputs.GetCount();
  const plUInt32 uiVertexColorCount = mbDesc.GetVertexCount() * uiNumOutputs;

  pComponent->m_hVertexColorBuffer = m_hVertexColorBuffer;

  if (pComponent->m_uiBufferAccessData == 0)
  {
    pComponent->m_uiBufferAccessData = (uiNumOutputs << BUFFER_ACCESS_OFFSET_BITS) | m_uiCurrentBufferOffset;
    m_uiCurrentBufferOffset += uiVertexColorCount;
  }

  const plUInt32 uiBufferOffset = pComponent->m_uiBufferAccessData & BUFFER_ACCESS_OFFSET_MASK;
  m_ModifiedDataRange.SetToIncludeRange(uiBufferOffset, uiBufferOffset + uiVertexColorCount - 1);

  if (m_uiNextTaskIndex >= m_UpdateTasks.GetCount())
  {
    m_UpdateTasks.PushBack(PLASMA_DEFAULT_NEW(plProcGenInternal::VertexColorTask));
  }

  auto& pUpdateTask = m_UpdateTasks[m_uiNextTaskIndex];

  plStringBuilder taskName = "VertexColor ";
  taskName.Append(pCpuMesh->GetResourceDescription().GetView());
  pUpdateTask->ConfigureTask(taskName, plTaskNesting::Never);

  pUpdateTask->Prepare(*GetWorld(), mbDesc, pComponent->GetOwner()->GetGlobalTransform(), pComponent->m_Outputs, outputMappings, m_VertexColorData.GetArrayPtr().GetSubArray(uiBufferOffset, uiVertexColorCount));

  plTaskSystem::AddTaskToGroup(m_UpdateTaskGroupID, pUpdateTask);

  ++m_uiNextTaskIndex;
}

void plProcVertexColorComponentManager::OnExtractionEvent(const plRenderWorldExtractionEvent& e)
{
  if (e.m_Type != plRenderWorldExtractionEvent::Type::EndExtraction)
    return;

  plTaskSystem::WaitForGroup(m_UpdateTaskGroupID);
  m_UpdateTaskGroupID.Invalidate();
  m_uiNextTaskIndex = 0;

  if (m_ModifiedDataRange.IsValid())
  {
    auto& dataCopy = m_DataCopy[plRenderWorld::GetDataIndexForExtraction()];
    dataCopy.m_Data = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plUInt32, m_ModifiedDataRange.GetCount());
    dataCopy.m_Data.CopyFrom(m_VertexColorData.GetArrayPtr().GetSubArray(m_ModifiedDataRange.m_uiMin, m_ModifiedDataRange.GetCount()));
    dataCopy.m_uiStart = m_ModifiedDataRange.m_uiMin;
  }
}

void plProcVertexColorComponentManager::OnRenderEvent(const plRenderWorldRenderEvent& e)
{
  if (e.m_Type != plRenderWorldRenderEvent::Type::BeginRender)
    return;

  auto& dataCopy = m_DataCopy[plRenderWorld::GetDataIndexForRendering()];
  if (!dataCopy.m_Data.IsEmpty())
  {
    plGALDevice* pGALDevice = plGALDevice::GetDefaultDevice();
    plGALPass* pGALPass = pGALDevice->BeginPass("ProcVertexUpdate");
    plGALComputeCommandEncoder* pGALCommandEncoder = pGALPass->BeginCompute();

    plUInt32 uiByteOffset = dataCopy.m_uiStart * sizeof(plUInt32);
    pGALCommandEncoder->UpdateBuffer(m_hVertexColorBuffer, uiByteOffset, dataCopy.m_Data.ToByteArray(), plGALUpdateMode::CopyToTempStorage);

    dataCopy = DataCopy();

    pGALPass->EndCompute(pGALCommandEncoder);
    pGALDevice->EndPass(pGALPass);
  }
}

void plProcVertexColorComponentManager::EnqueueUpdate(plProcVertexColorComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  if (!m_ComponentsToUpdate.Contains(pComponent->GetHandle()))
  {
    m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
  }
}

void plProcVertexColorComponentManager::RemoveComponent(plProcVertexColorComponent* pComponent)
{
  m_ComponentsToUpdate.RemoveAndSwap(pComponent->GetHandle());

  if (pComponent->m_uiBufferAccessData != 0)
  {
    /// \todo compact buffer somehow?

    pComponent->m_uiBufferAccessData = 0;
  }
}

void plProcVertexColorComponentManager::OnResourceEvent(const plResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != plResourceEvent::Type::ResourceContentUnloading)
    return;

  if (auto pResource = plDynamicCast<const plProcGenGraphResource*>(resourceEvent.m_pResource))
  {
    plProcGenGraphResourceHandle hResource = pResource->GetResourceHandle();

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource)
      {
        EnqueueUpdate(it);
      }
    }
  }
}

void plProcVertexColorComponentManager::OnAreaInvalidated(const plProcGenInternal::InvalidatedArea& area)
{
  if (area.m_pWorld != GetWorld())
    return;

  plSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = plDefaultSpatialDataCategories::RenderStatic.GetBitmask() | plDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

  GetWorld()->GetSpatialSystem()->FindObjectsInBox(area.m_Box, queryParams, [this](plGameObject* pObject) {
    plHybridArray<plProcVertexColorComponent*, 8> components;
    pObject->TryGetComponentsOfBaseType(components);

    for (auto pComponent : components)
    {
      EnqueueUpdate(pComponent);
    }

    return plVisitorExecution::Continue;
  });
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plProcVertexColorOutputDesc, plNoBase, 1, plRTTIDefaultAllocator<plProcVertexColorOutputDesc>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Name", GetName, SetName),
    PLASMA_MEMBER_PROPERTY("Mapping", m_Mapping),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

void plProcVertexColorOutputDesc::SetName(const char* szName)
{
  m_sName.Assign(szName);
}

static plTypeVersion s_ProcVertexColorOutputDescVersion = 1;
plResult plProcVertexColorOutputDesc::Serialize(plStreamWriter& stream) const
{
  stream.WriteVersion(s_ProcVertexColorOutputDescVersion);
  stream << m_sName;
  PLASMA_SUCCEED_OR_RETURN(m_Mapping.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plProcVertexColorOutputDesc::Deserialize(plStreamReader& stream)
{
  /*plTypeVersion version =*/stream.ReadVersion(s_ProcVertexColorOutputDescVersion);
  stream >> m_sName;
  PLASMA_SUCCEED_OR_RETURN(m_Mapping.Deserialize(stream));

  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plProcVertexColorComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_ProcGen_Graph")),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("OutputDescs", OutputDescs_GetCount, GetOutputDesc, SetOutputDesc, OutputDescs_Insert, OutputDescs_Remove),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgTransformChanged, OnTransformChanged)
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Construction/PCG"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plProcVertexColorComponent::plProcVertexColorComponent() = default;
plProcVertexColorComponent::~plProcVertexColorComponent() = default;

void plProcVertexColorComponent::OnActivated()
{
  SUPER::OnActivated();

  auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(this);

  GetOwner()->EnableStaticTransformChangesNotifications();
}

void plProcVertexColorComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(this);

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();
}

void plProcVertexColorComponent::SetResourceFile(const char* szFile)
{
  plProcGenGraphResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plProcGenGraphResource>(szFile);
    plResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

const char* plProcVertexColorComponent::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void plProcVertexColorComponent::SetResource(const plProcGenGraphResourceHandle& hResource)
{
  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

const plProcVertexColorOutputDesc& plProcVertexColorComponent::GetOutputDesc(plUInt32 uiIndex) const
{
  return m_OutputDescs[uiIndex];
}

void plProcVertexColorComponent::SetOutputDesc(plUInt32 uiIndex, const plProcVertexColorOutputDesc& outputDesc)
{
  m_OutputDescs.EnsureCount(uiIndex + 1);
  m_OutputDescs[uiIndex] = outputDesc;

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

void plProcVertexColorComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  plStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_OutputDescs).IgnoreResult();
}

void plProcVertexColorComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_hResource;
  if (uiVersion >= 2)
  {
    s.ReadArray(m_OutputDescs).IgnoreResult();
  }
  else
  {
    plHybridArray<plHashedString, 2> outputNames;
    s.ReadArray(outputNames).IgnoreResult();

    for (auto& outputName : outputNames)
    {
      auto& outputDesc = m_OutputDescs.ExpandAndGetRef();
      outputDesc.m_sName = outputName;
    }
  }
}

void plProcVertexColorComponent::OnTransformChanged(plMsgTransformChanged& msg)
{
  auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(this);
}

plMeshRenderData* plProcVertexColorComponent::CreateRenderData() const
{
  auto pRenderData = plCreateRenderDataForThisFrame<plProcVertexColorRenderData>(GetOwner());

  if (HasValidOutputs() && m_uiBufferAccessData != 0)
  {
    pRenderData->m_hVertexColorBuffer = m_hVertexColorBuffer;
    pRenderData->m_uiBufferAccessData = m_uiBufferAccessData;
  }

  return pRenderData;
}

plUInt32 plProcVertexColorComponent::OutputDescs_GetCount() const
{
  return m_OutputDescs.GetCount();
}

void plProcVertexColorComponent::OutputDescs_Insert(plUInt32 uiIndex, const plProcVertexColorOutputDesc& outputDesc)
{
  m_OutputDescs.Insert(outputDesc, uiIndex);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

void plProcVertexColorComponent::OutputDescs_Remove(plUInt32 uiIndex)
{
  m_OutputDescs.RemoveAtAndCopy(uiIndex);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(this);
  }
}

bool plProcVertexColorComponent::HasValidOutputs() const
{
  for (auto& pOutput : m_Outputs)
  {
    if (pOutput != nullptr && pOutput->m_pByteCode != nullptr)
    {
      return true;
    }
  }

  return false;
}
