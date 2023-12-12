#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/InstancedMeshComponent.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plMeshInstanceData, plNoBase, 1, plRTTIDefaultAllocator<plMeshInstanceData>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new plSuffixAttribute(" m")),
    PLASMA_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    PLASMA_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new plDefaultValueAttribute(plVec3(1.0f, 1.0f, 1.0f))),

    PLASMA_MEMBER_PROPERTY("Color", m_color)
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plColorAttribute(plColorScheme::Rendering),
    new plTransformManipulatorAttribute("LocalPosition", "LocalRotation", "LocalScaling"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_STATIC_REFLECTED_TYPE
// clang-format on

void plMeshInstanceData::SetLocalPosition(plVec3 vPosition)
{
  m_transform.m_vPosition = vPosition;
}
plVec3 plMeshInstanceData::GetLocalPosition() const
{
  return m_transform.m_vPosition;
}

void plMeshInstanceData::SetLocalRotation(plQuat qRotation)
{
  m_transform.m_qRotation = qRotation;
}

plQuat plMeshInstanceData::GetLocalRotation() const
{
  return m_transform.m_qRotation;
}

void plMeshInstanceData::SetLocalScaling(plVec3 vScaling)
{
  m_transform.m_vScale = vScaling;
}

plVec3 plMeshInstanceData::GetLocalScaling() const
{
  return m_transform.m_vScale;
}

static const plTypeVersion s_MeshInstanceDataVersion = 1;
plResult plMeshInstanceData::Serialize(plStreamWriter& ref_writer) const
{
  ref_writer.WriteVersion(s_MeshInstanceDataVersion);

  ref_writer << m_transform;
  ref_writer << m_color;

  return PLASMA_SUCCESS;
}

plResult plMeshInstanceData::Deserialize(plStreamReader& ref_reader)
{
  /*auto version = */ ref_reader.ReadVersion(s_MeshInstanceDataVersion);

  ref_reader >> m_transform;
  ref_reader >> m_color;

  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plInstancedMeshRenderData, 1, plRTTIDefaultAllocator<plInstancedMeshRenderData>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plInstancedMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_pExplicitInstanceData->m_hInstanceDataBuffer.GetInternalID().m_Data);
}

//////////////////////////////////////////////////////////////////////////////////////

plInstancedMeshComponentManager::plInstancedMeshComponentManager(plWorld* pWorld)
  : SUPER(pWorld)
{
}

void plInstancedMeshComponentManager::EnqueueUpdate(const plInstancedMeshComponent* pComponent) const
{
  plUInt64 uiCurrentFrame = plRenderWorld::GetFrameCounter();
  if (pComponent->m_uiEnqueuedFrame == uiCurrentFrame)
    return;

  PLASMA_LOCK(m_Mutex);
  if (pComponent->m_uiEnqueuedFrame == uiCurrentFrame)
    return;

  auto instanceData = pComponent->GetInstanceData();
  if (instanceData.IsEmpty())
    return;

  m_RequireUpdate.PushBack({pComponent->GetHandle(), instanceData});
  pComponent->m_uiEnqueuedFrame = uiCurrentFrame;
}

void plInstancedMeshComponentManager::OnRenderEvent(const plRenderWorldRenderEvent& e)
{
  if (e.m_Type != plRenderWorldRenderEvent::Type::BeginRender)
    return;

  PLASMA_LOCK(m_Mutex);

  if (m_RequireUpdate.IsEmpty())
    return;

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALPass* pGALPass = pDevice->BeginPass("Update Instanced Mesh Data");

  plRenderContext* pRenderContext = plRenderContext::GetDefaultInstance();
  pRenderContext->BeginCompute(pGALPass);

  for (const auto& componentToUpdate : m_RequireUpdate)
  {
    plInstancedMeshComponent* pComp = nullptr;
    if (!TryGetComponent(componentToUpdate.m_hComponent, pComp))
      continue;

    if (pComp->m_pExplicitInstanceData)
    {
      plUInt32 uiOffset = 0;
      auto instanceData = pComp->m_pExplicitInstanceData->GetInstanceData(componentToUpdate.m_InstanceData.GetCount(), uiOffset);
      instanceData.CopyFrom(componentToUpdate.m_InstanceData);

      pComp->m_pExplicitInstanceData->UpdateInstanceData(pRenderContext, instanceData.GetCount());
    }
  }

  pRenderContext->EndCompute();
  pDevice->EndPass(pGALPass);

  m_RequireUpdate.Clear();
}

void plInstancedMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  plRenderWorld::GetRenderEvent().AddEventHandler(plMakeDelegate(&plInstancedMeshComponentManager::OnRenderEvent, this));
}

void plInstancedMeshComponentManager::Deinitialize()
{
  plRenderWorld::GetRenderEvent().RemoveEventHandler(plMakeDelegate(&plInstancedMeshComponentManager::OnRenderEvent, this));

  SUPER::Deinitialize();
}

//////////////////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plInstancedMeshComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    PLASMA_ACCESSOR_PROPERTY("MainColor", GetColor, SetColor)->AddAttributes(new plExposeColorAlphaAttribute()),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),

    PLASMA_ARRAY_ACCESSOR_PROPERTY("InstanceData", Instances_GetCount, Instances_GetValue, Instances_SetValue, Instances_Insert, Instances_Remove),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractGeometry, OnMsgExtractGeometry),
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plInstancedMeshComponent::plInstancedMeshComponent() = default;
plInstancedMeshComponent::~plInstancedMeshComponent() = default;

void plInstancedMeshComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  inout_stream.GetStream().WriteArray(m_RawInstancedData).IgnoreResult();
}

void plInstancedMeshComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  inout_stream.GetStream().ReadArray(m_RawInstancedData).IgnoreResult();
}

void plInstancedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  PLASMA_ASSERT_DEV(m_pExplicitInstanceData == nullptr, "Instance data must not be initialized at this point");
  m_pExplicitInstanceData = PLASMA_DEFAULT_NEW(plInstanceData);
}

void plInstancedMeshComponent::OnDeactivated()
{
  PLASMA_DEFAULT_DELETE(m_pExplicitInstanceData);
  m_pExplicitInstanceData = nullptr;

  SUPER::OnDeactivated();
}

void plInstancedMeshComponent::OnMsgExtractGeometry(plMsgExtractGeometry& ref_msg) {}

plResult plInstancedMeshComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  plBoundingBoxSphere singleBounds;
  if (m_hMesh.IsValid())
  {
    plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::AllowLoadingFallback);
    singleBounds = pMesh->GetBounds();

    for (const auto& instance : m_RawInstancedData)
    {
      auto instanceBounds = singleBounds;
      instanceBounds.Transform(instance.m_transform.GetAsMat4());

      ref_bounds.ExpandToInclude(instanceBounds);
    }

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

void plInstancedMeshComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  SUPER::OnMsgExtractRenderData(msg);

  static_cast<const plInstancedMeshComponentManager*>(GetOwningManager())->EnqueueUpdate(this);
}

plMeshRenderData* plInstancedMeshComponent::CreateRenderData() const
{
  auto pRenderData = plCreateRenderDataForThisFrame<plInstancedMeshRenderData>(GetOwner());

  if (m_pExplicitInstanceData)
  {
    pRenderData->m_pExplicitInstanceData = m_pExplicitInstanceData;
    pRenderData->m_uiExplicitInstanceCount = m_RawInstancedData.GetCount();

    const plWindWorldModuleInterface* pWindInterface = GetWorld()->GetModuleReadOnly<plWindWorldModuleInterface>();
    if (pWindInterface)
    {
      const plVec3 position = GetOwner()->GetGlobalPosition();
      const plVec3 windSamplePosition = position + plVec3(0, 0, 2); // avoid issues where position is at the contact point with the floor
      const plVec3 windowForce = pWindInterface->GetWindAt(windSamplePosition);
      pRenderData->m_Wind = windowForce;
    }
  }

  return pRenderData;
}

plUInt32 plInstancedMeshComponent::Instances_GetCount() const
{
  return m_RawInstancedData.GetCount();
}

plMeshInstanceData plInstancedMeshComponent::Instances_GetValue(plUInt32 uiIndex) const
{
  return m_RawInstancedData[uiIndex];
}

void plInstancedMeshComponent::Instances_SetValue(plUInt32 uiIndex, plMeshInstanceData value)
{
  m_RawInstancedData[uiIndex] = value;

  TriggerLocalBoundsUpdate();
}

void plInstancedMeshComponent::Instances_Insert(plUInt32 uiIndex, plMeshInstanceData value)
{
  m_RawInstancedData.Insert(value, uiIndex);

  TriggerLocalBoundsUpdate();
}

void plInstancedMeshComponent::Instances_Remove(plUInt32 uiIndex)
{
  m_RawInstancedData.RemoveAtAndCopy(uiIndex);

  TriggerLocalBoundsUpdate();
}

plArrayPtr<plPerInstanceData> plInstancedMeshComponent::GetInstanceData() const
{
  if (!m_pExplicitInstanceData || m_RawInstancedData.IsEmpty())
    return plArrayPtr<plPerInstanceData>();

  auto instanceData = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plPerInstanceData, m_RawInstancedData.GetCount());

  const plTransform ownerTransform = GetOwner()->GetGlobalTransform();

  float fBoundingSphereRadius = 1.0f;

  if (m_hMesh.IsValid())
  {
    plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::AllowLoadingFallback);
    fBoundingSphereRadius = pMesh->GetBounds().GetSphere().m_fRadius;
  }

  for (plUInt32 i = 0; i < m_RawInstancedData.GetCount(); ++i)
  {
    const plTransform globalTransform = ownerTransform * m_RawInstancedData[i].m_transform;
    const plMat4 objectToWorld = globalTransform.GetAsMat4();

    instanceData[i].ObjectToWorld = objectToWorld;

    #if PLASMA_ENABLED(PLASMA_GAMEOBJECT_VELOCITY)
      const plMat4 lastObjectToWorld = (GetOwner()->GetLastGlobalTransform() * m_RawInstancedData[i].m_transform).GetAsMat4();
      instanceData[i].LastObjectToWorld = lastObjectToWorld;
    #endif

    if (m_RawInstancedData[i].m_transform.ContainsUniformScale())
    {
      instanceData[i].ObjectToWorldNormal = objectToWorld;

      #if PLASMA_ENABLED(PLASMA_GAMEOBJECT_VELOCITY)
        instanceData[i].LastObjectToWorldNormal = lastObjectToWorld;
      #endif
    }
    else
    {
      plMat3 mInverse = objectToWorld.GetRotationalPart();
      mInverse.Invert(0.0f).IgnoreResult();
      // we explicitly ignore the return value here (success / failure)
      // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

      plShaderTransform shaderT;
      shaderT = mInverse.GetTranspose();
      instanceData[i].ObjectToWorldNormal = shaderT;

      #if PLASMA_ENABLED(PLASMA_GAMEOBJECT_VELOCITY)
        mInverse = lastObjectToWorld.GetRotationalPart();
        mInverse.Invert(0.0f).IgnoreResult();
        // we explicitly ignore the return value here (success / failure)
        // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

        shaderT = mInverse.GetTranspose();
        instanceData[i].LastObjectToWorldNormal = shaderT;
      #endif
    }

    instanceData[i].GameObjectID = GetUniqueIdForRendering();
    instanceData[i].BoundingSphereRadius = fBoundingSphereRadius * m_RawInstancedData[i].m_transform.GetMaxScale();

    instanceData[i].Color = m_Color * m_RawInstancedData[i].m_color;
  }

  return instanceData;
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_InstancedMeshComponent);
