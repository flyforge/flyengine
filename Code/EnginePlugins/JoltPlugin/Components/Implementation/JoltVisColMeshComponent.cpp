#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Components/JoltVisColMeshComponent.h>
#include <JoltPlugin/Shapes/JoltShapeConvexHullComponent.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltVisColMeshComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Triangle;CompatibleAsset_Jolt_Colmesh_Convex", plDependencyFlags::Package)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics/Jolt/Misc"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltVisColMeshComponent::plJoltVisColMeshComponent() = default;
plJoltVisColMeshComponent::~plJoltVisColMeshComponent() = default;

void plJoltVisColMeshComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hCollisionMesh;
}


void plJoltVisColMeshComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hCollisionMesh;

  GetWorld()->GetOrCreateComponentManager<plJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
}

plResult plJoltVisColMeshComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  // have to assume this isn't thread safe
  // CreateCollisionRenderMesh();

  if (m_hMesh.IsValid())
  {
    plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::BlockTillLoaded);
    ref_bounds = pMesh->GetBounds();
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

void plJoltVisColMeshComponent::SetMeshFile(const char* szFile)
{
  plJoltMeshResourceHandle hMesh;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = plResourceManager::LoadResource<plJoltMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* plJoltVisColMeshComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void plJoltVisColMeshComponent::SetMesh(const plJoltMeshResourceHandle& hMesh)
{
  if (m_hCollisionMesh != hMesh)
  {
    m_hCollisionMesh = hMesh;
    m_hMesh.Invalidate();

    GetWorld()->GetOrCreateComponentManager<plJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void plJoltVisColMeshComponent::CreateCollisionRenderMesh()
{
  if (!m_hCollisionMesh.IsValid())
  {
    plJoltStaticActorComponent* pSibling = nullptr;
    if (GetOwner()->TryGetComponentOfBaseType(pSibling))
    {
      m_hCollisionMesh = pSibling->GetMesh();
    }
  }

  if (!m_hCollisionMesh.IsValid())
  {
    plJoltShapeConvexHullComponent* pSibling = nullptr;
    if (GetOwner()->TryGetComponentOfBaseType(pSibling))
    {
      m_hCollisionMesh = pSibling->GetMesh();
    }
  }

  if (!m_hCollisionMesh.IsValid())
    return;

  plResourceLock<plJoltMeshResource> pMesh(m_hCollisionMesh, plResourceAcquireMode::BlockTillLoaded);

  if (pMesh.GetAcquireResult() == plResourceAcquireResult::MissingFallback)
    return;

  plStringBuilder sColMeshName = pMesh->GetResourceID();
  sColMeshName.AppendFormat("_{0}_JoltVisColMesh",
    pMesh->GetCurrentResourceChangeCounter()); // the change counter allows to react to resource updates

  m_hMesh = plResourceManager::GetExistingResource<plMeshResource>(sColMeshName);

  if (m_hMesh.IsValid())
  {
    TriggerLocalBoundsUpdate();
    return;
  }

  plCpuMeshResourceHandle hCpuMesh = pMesh->ConvertToCpuMesh();

  if (!hCpuMesh.IsValid())
    return;

  plResourceLock<plCpuMeshResource> pCpuMesh(hCpuMesh, plResourceAcquireMode::BlockTillLoaded);

  plMeshResourceDescriptor md = pCpuMesh->GetDescriptor();

  md.SetMaterial(0, "Materials/Common/ColMesh.plMaterial");

  m_hMesh = plResourceManager::GetOrCreateResource<plMeshResource>(sColMeshName, std::move(md), "Collision Mesh Visualization");

  TriggerLocalBoundsUpdate();
}

void plJoltVisColMeshComponent::Initialize()
{
  SUPER::Initialize();

  GetWorld()->GetOrCreateComponentManager<plJoltVisColMeshComponentManager>()->EnqueueUpdate(GetHandle());
}

void plJoltVisColMeshComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::AllowLoadingFallback);

  plRenderData::Caching::Enum caching = plRenderData::Caching::IfStatic;

  if (pMesh.GetAcquireResult() != plResourceAcquireResult::Final)
  {
    caching = plRenderData::Caching::Never;
  }

  plArrayPtr<const plMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (plUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const plUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    plMaterialResourceHandle hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    plMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plMeshRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::LitOpaque, caching);
  }
}

//////////////////////////////////////////////////////////////////////////

void plJoltVisColMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  plWorldModule::UpdateFunctionDesc desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plJoltVisColMeshComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plJoltVisColMeshComponentManager::ResourceEventHandler, this));
}

void plJoltVisColMeshComponentManager::Deinitialize()
{
  PLASMA_LOCK(m_Mutex);

  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plJoltVisColMeshComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void plJoltVisColMeshComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  plDeque<plComponentHandle> requireUpdate;
  m_RequireUpdate.Swap(requireUpdate);

  for (const auto& hComp : requireUpdate)
  {
    plJoltVisColMeshComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp))
      continue;

    pComp->CreateCollisionRenderMesh();
  }
}

void plJoltVisColMeshComponentManager::EnqueueUpdate(plComponentHandle hComponent)
{
  m_RequireUpdate.PushBack(hComponent);
}

void plJoltVisColMeshComponentManager::ResourceEventHandler(const plResourceEvent& e)
{
  if ((e.m_Type == plResourceEvent::Type::ResourceContentUnloading || e.m_Type == plResourceEvent::Type::ResourceContentUpdated) && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<plJoltMeshResource>())
  {
    PLASMA_LOCK(m_Mutex);

    plJoltMeshResourceHandle hResource((plJoltMeshResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const plJoltVisColMeshComponent* pComponent = static_cast<plJoltVisColMeshComponent*>(it.Value());

      if (pComponent->GetMesh() == hResource)
      {
        m_RequireUpdate.PushBack(pComponent->GetHandle());
      }
    }
  }
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltVisColMeshComponent);

