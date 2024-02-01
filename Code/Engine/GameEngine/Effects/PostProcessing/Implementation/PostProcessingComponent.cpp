#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Effects/PostProcessing/PostProcessingComponent.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plPostProcessingComponentManager::plPostProcessingComponentManager(plWorld* pWorld)
  : plComponentManager(pWorld)
{
}

void plPostProcessingComponentManager::Initialize()
{
  auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plPostProcessingComponentManager::UpdateComponents, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostTransform;

  RegisterUpdateFunction(desc);
}

void plPostProcessingComponentManager::UpdateComponents(const UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->SampleAndSetViewProperties();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plPostProcessingValueMapping, plNoBase, 1, plRTTIDefaultAllocator<plPostProcessingValueMapping>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("RenderPass", m_sRenderPassName),
    PL_MEMBER_PROPERTY("Property", m_sPropertyName),
    PL_MEMBER_PROPERTY("VolumeValue", m_sVolumeValueName),
    PL_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("InterpolationDuration", m_InterpolationDuration),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

plResult plPostProcessingValueMapping::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_sRenderPassName;
  inout_stream << m_sPropertyName;
  inout_stream << m_sVolumeValueName;
  inout_stream << m_DefaultValue;
  inout_stream << m_InterpolationDuration;

  return PL_SUCCESS;
}

plResult plPostProcessingValueMapping::Deserialize(plStreamReader& inout_stream)
{
  inout_stream >> m_sRenderPassName;
  inout_stream >> m_sPropertyName;
  inout_stream >> m_sVolumeValueName;
  inout_stream >> m_DefaultValue;
  inout_stream >> m_InterpolationDuration;

  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plPostProcessingComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("VolumeType", GetVolumeType, SetVolumeType)->AddAttributes(new plDynamicStringEnumAttribute("SpatialDataCategoryEnum"), new plDefaultValueAttribute("GenericVolume")),
    PL_ARRAY_ACCESSOR_PROPERTY("Mappings", Mappings_GetCount, Mappings_GetMapping, Mappings_SetMapping, Mappings_Insert, Mappings_Remove),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plPostProcessingComponent::plPostProcessingComponent() = default;
plPostProcessingComponent::plPostProcessingComponent(plPostProcessingComponent&& other) = default;
plPostProcessingComponent::~plPostProcessingComponent() = default;
plPostProcessingComponent& plPostProcessingComponent::operator=(plPostProcessingComponent&& other) = default;

void plPostProcessingComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  auto& sCategory = plSpatialData::GetCategoryName(m_SpatialCategory);
  s << sCategory;

  s.WriteArray(m_Mappings).IgnoreResult();
}

void plPostProcessingComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  plHashedString sCategory;
  s >> sCategory;
  m_SpatialCategory = plSpatialData::RegisterCategory(sCategory, plSpatialData::Flags::None);

  s.ReadArray(m_Mappings).IgnoreResult();
}

void plPostProcessingComponent::SetVolumeType(const char* szType)
{
  m_SpatialCategory = plSpatialData::RegisterCategory(szType, plSpatialData::Flags::None);
}

const char* plPostProcessingComponent::GetVolumeType() const
{
  return plSpatialData::GetCategoryName(m_SpatialCategory);
}

void plPostProcessingComponent::Initialize()
{
  m_pSampler = PL_DEFAULT_NEW(plVolumeSampler);

  RegisterSamplerValues();
}

void plPostProcessingComponent::Deinitialize()
{
  m_pSampler = nullptr;
}

void plPostProcessingComponent::OnActivated()
{
  SUPER::OnActivated();

  plCameraComponent* pCameraComponent = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pCameraComponent))
  {
    m_hCameraComponent = pCameraComponent->GetHandle();
  }
}

void plPostProcessingComponent::OnDeactivated()
{
  ResetViewProperties();

  m_hCameraComponent.Invalidate();

  SUPER::OnDeactivated();
}

void plPostProcessingComponent::Mappings_SetMapping(plUInt32 i, const plPostProcessingValueMapping& mapping)
{
  m_Mappings.EnsureCount(i + 1);
  m_Mappings[i] = mapping;

  RegisterSamplerValues();
}

void plPostProcessingComponent::Mappings_Insert(plUInt32 uiIndex, const plPostProcessingValueMapping& mapping)
{
  m_Mappings.Insert(mapping, uiIndex);

  RegisterSamplerValues();
}

void plPostProcessingComponent::Mappings_Remove(plUInt32 uiIndex)
{
  m_Mappings.RemoveAtAndCopy(uiIndex);

  ResetViewProperties();
  RegisterSamplerValues();
}

plView* plPostProcessingComponent::FindView() const
{
  const plWorld* pWorld = GetWorld();
  plView* pView = nullptr;

  const plCameraComponent* pCameraComponent = nullptr;
  if (pWorld->TryGetComponent(m_hCameraComponent, pCameraComponent) && pCameraComponent->GetUsageHint() == plCameraUsageHint::RenderTarget)
  {
    plRenderWorld::TryGetView(pCameraComponent->GetRenderTargetView(), pView);
  }

  if (pView == nullptr)
  {
    pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView, pWorld);
  }

  return pView;
}

void plPostProcessingComponent::RegisterSamplerValues()
{
  if (m_pSampler == nullptr)
    return;

  m_pSampler->DeregisterAllValues();

  for (auto& mapping : m_Mappings)
  {
    if (mapping.m_sVolumeValueName.IsEmpty())
      continue;

    m_pSampler->RegisterValue(mapping.m_sVolumeValueName, mapping.m_DefaultValue, mapping.m_InterpolationDuration);
  }
}

void plPostProcessingComponent::ResetViewProperties()
{
  if (plView* pView = FindView())
  {
    pView->ResetRenderPassProperties();
  }
}

void plPostProcessingComponent::SampleAndSetViewProperties()
{
  plView* pView = FindView();
  if (pView == nullptr)
    return;

  const plVec3 vSamplePos = pView->GetCullingCamera()->GetCenterPosition();

  plWorld* pWorld = GetWorld();
  plTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = pWorld->GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = plClock::GetGlobalClock()->GetTimeDiff();
  }

  m_pSampler->SampleAtPosition(*pWorld, m_SpatialCategory, vSamplePos, deltaTime);

  for (auto& mapping : m_Mappings)
  {
    if (mapping.m_sRenderPassName.IsEmpty() || mapping.m_sPropertyName.IsEmpty())
      continue;

    plVariant value;
    if (mapping.m_sVolumeValueName.IsEmpty())
    {
      value = mapping.m_DefaultValue;
    }
    else
    {
      value = m_pSampler->GetValue(mapping.m_sVolumeValueName);
    }

    pView->SetRenderPassProperty(mapping.m_sRenderPassName, mapping.m_sPropertyName, value);
  }
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Effects_PostProcessing_Implementation_PostProcessingComponent);

