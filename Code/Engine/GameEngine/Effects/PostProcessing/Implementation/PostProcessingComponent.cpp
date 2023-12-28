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
  auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plPostProcessingComponentManager::UpdateComponents, this);
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
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plPostProcessingValueMapping, plNoBase, 1, plRTTIDefaultAllocator<plPostProcessingValueMapping>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("RenderPass", m_sRenderPassName),
    PLASMA_MEMBER_PROPERTY("Property", m_sPropertyName),
    PLASMA_MEMBER_PROPERTY("VolumeValue", m_sVolumeValueName),
    PLASMA_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new plDefaultValueAttribute(1.0)),
    PLASMA_MEMBER_PROPERTY("InterpolationDuration", m_InterpolationDuration),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plResult plPostProcessingValueMapping::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_sRenderPassName;
  inout_stream << m_sPropertyName;
  inout_stream << m_sVolumeValueName;
  inout_stream << m_DefaultValue;
  inout_stream << m_InterpolationDuration;

  return PLASMA_SUCCESS;
}

plResult plPostProcessingValueMapping::Deserialize(plStreamReader& inout_stream)
{
  inout_stream >> m_sRenderPassName;
  inout_stream >> m_sPropertyName;
  inout_stream >> m_sVolumeValueName;
  inout_stream >> m_DefaultValue;
  inout_stream >> m_InterpolationDuration;

  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plPostProcessingComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_ACCESSOR_PROPERTY("Mappings", Mappings_GetCount, Mappings_GetMapping, Mappings_SetMapping, Mappings_Insert, Mappings_Remove),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plPostProcessingComponent::plPostProcessingComponent() = default;
plPostProcessingComponent::plPostProcessingComponent(plPostProcessingComponent&& other) = default;
plPostProcessingComponent::~plPostProcessingComponent() = default;

void plPostProcessingComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s.WriteArray(m_Mappings).IgnoreResult();
}

void plPostProcessingComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s.ReadArray(m_Mappings).IgnoreResult();
}

plPostProcessingComponent& plPostProcessingComponent::operator=(plPostProcessingComponent&& other) = default;

void plPostProcessingComponent::Initialize()
{
  m_pSampler = PLASMA_DEFAULT_NEW(plVolumeSampler);

  RegisterSamplerValues();
}

void plPostProcessingComponent::Deinitialize()
{
  m_pSampler = nullptr;
}

void plPostProcessingComponent::OnActivated()
{
  plCameraComponent* pCameraComponent = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pCameraComponent))
  {
    m_hCameraComponent = pCameraComponent->GetHandle();
  }
}

void plPostProcessingComponent::OnDeactivated()
{
  m_hCameraComponent.Invalidate();
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

  RegisterSamplerValues();
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

void plPostProcessingComponent::SampleAndSetViewProperties()
{
  plWorld* pWorld = GetWorld();

  plView* pView = nullptr;
  {
    plCameraComponent* pCameraComponent = nullptr;
    if (pWorld->TryGetComponent(m_hCameraComponent, pCameraComponent) && pCameraComponent->GetUsageHint() == plCameraUsageHint::RenderTarget)
    {
      plRenderWorld::TryGetView(pCameraComponent->GetRenderTargetView(), pView);
    }

    if (pView == nullptr)
    {
      pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView, pWorld);
    }
  }
  if (pView == nullptr)
    return;

  const plVec3 vSamplePos =  pView->GetCullingCamera()->GetCenterPosition();

  plTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = pWorld->GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = plClock::GetGlobalClock()->GetTimeDiff();
  }

  m_pSampler->SampleAtPosition(*pWorld, vSamplePos, deltaTime);

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
