#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <GameEngine/Volumes/VolumeComponent.h>

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plVolumeComponent, 1)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Type", GetVolumeType, SetVolumeType)->AddAttributes(new plDynamicStringEnumAttribute("SpatialDataCategoryEnum"), new plDefaultValueAttribute("GenericVolume")),
    PL_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new plClampValueAttribute(-64.0f, 64.0f)),
    PL_ACCESSOR_PROPERTY("Template", GetTemplateFile, SetTemplateFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    PL_MAP_ACCESSOR_PROPERTY("Values", Reflection_GetKeys, Reflection_GetValue, Reflection_InsertValue, Reflection_RemoveValue),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Name", In, "Value"),
    PL_SCRIPT_FUNCTION_PROPERTY(GetValue, In, "Name"),
  }
  PL_END_FUNCTIONS;

  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plVolumeComponent::plVolumeComponent() = default;
plVolumeComponent::~plVolumeComponent() = default;

void plVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeFromTemplate();

  GetOwner()->UpdateLocalBounds();
}

void plVolumeComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  RemoveReloadFunction();

  GetOwner()->UpdateLocalBounds();
}

void plVolumeComponent::SetTemplateFile(const char* szFile)
{
  plBlackboardTemplateResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plBlackboardTemplateResource>(szFile);
  }

  SetTemplate(hResource);
}

const char* plVolumeComponent::GetTemplateFile() const
{
  if (!m_hTemplateResource.IsValid())
    return "";

  return m_hTemplateResource.GetResourceID();
}

void plVolumeComponent::SetTemplate(const plBlackboardTemplateResourceHandle& hResource)
{
  RemoveReloadFunction();

  m_hTemplateResource = hResource;

  if (IsActiveAndInitialized())
  {
    ReloadTemplate();
  }
}

void plVolumeComponent::SetSortOrder(float fOrder)
{
  fOrder = plMath::Clamp(fOrder, -64.0f, 64.0f);
  m_fSortOrder = fOrder;
}

void plVolumeComponent::SetVolumeType(const char* szType)
{
  m_SpatialCategory = plSpatialData::RegisterCategory(szType, plSpatialData::Flags::None);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

const char* plVolumeComponent::GetVolumeType() const
{
  return plSpatialData::GetCategoryName(m_SpatialCategory);
}

void plVolumeComponent::SetValue(const plHashedString& sName, const plVariant& value)
{
  m_Values.Insert(sName, value);
}

void plVolumeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_fSortOrder;

  auto& sCategory = plSpatialData::GetCategoryName(m_SpatialCategory);
  s << sCategory;

  s << m_hTemplateResource;

  // Only serialize overwritten values so a template change doesn't require a re-save of all volumes
  plUInt32 numValues = m_OverwrittenValues.GetCount();
  s << numValues;
  for (auto& sName : m_OverwrittenValues)
  {
    plVariant value;
    m_Values.TryGetValue(sName, value);

    s << sName;
    s << value;
  }
}

void plVolumeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_fSortOrder;

  plHashedString sCategory;
  s >> sCategory;
  m_SpatialCategory = plSpatialData::RegisterCategory(sCategory, plSpatialData::Flags::None);

  s >> m_hTemplateResource;

  // m_OverwrittenValues is only used in editor so we don't write to it here
  plUInt32 numValues = 0;
  s >> numValues;
  for (plUInt32 i = 0; i < numValues; ++i)
  {
    plHashedString sName;
    plVariant value;
    s >> sName;
    s >> value;

    m_Values.Insert(sName, value);
  }
}

const plRangeView<const plString&, plUInt32> plVolumeComponent::Reflection_GetKeys() const
{
  return plRangeView<const plString&, plUInt32>([]() -> plUInt32
    { return 0; },
    [this]() -> plUInt32
    { return m_OverwrittenValues.GetCount(); },
    [](plUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const plUInt32& uiIt) -> const plString&
    { return m_OverwrittenValues[uiIt].GetString(); });
}

bool plVolumeComponent::Reflection_GetValue(const char* szName, plVariant& value) const
{
  return m_Values.TryGetValue(plTempHashedString(szName), value);
}

void plVolumeComponent::Reflection_InsertValue(const char* szName, const plVariant& value)
{
  plHashedString sName;
  sName.Assign(szName);

  // Only needed in editor
  if (GetUniqueID() != plInvalidIndex && m_OverwrittenValues.Contains(sName) == false)
  {
    m_OverwrittenValues.PushBack(sName);
  }

  m_Values.Insert(sName, value);
}

void plVolumeComponent::Reflection_RemoveValue(const char* szName)
{
  plHashedString sName;
  sName.Assign(szName);

  m_OverwrittenValues.RemoveAndCopy(sName);

  m_Values.Remove(sName);
}

void plVolumeComponent::InitializeFromTemplate()
{
  if (!m_hTemplateResource.IsValid())
    return;

  plResourceLock<plBlackboardTemplateResource> pTemplate(m_hTemplateResource, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pTemplate.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    if (m_Values.Contains(entry.m_sName) == false)
    {
      m_Values.Insert(entry.m_sName, entry.m_InitialValue);
    }
  }

  if (m_bReloadFunctionAdded == false)
  {
    GetWorld()->AddResourceReloadFunction(m_hTemplateResource, GetHandle(), nullptr,
      [](const plWorld::ResourceReloadContext& context)
      {
        plStaticCast<plVolumeComponent*>(context.m_pComponent)->ReloadTemplate();
      });

    m_bReloadFunctionAdded = true;
  }
}

void plVolumeComponent::ReloadTemplate()
{
  // Remove all values that are not overwritten
  plHashTable<plHashedString, plVariant> overwrittenValues;
  for (auto& sName : m_OverwrittenValues)
  {
    overwrittenValues.Insert(sName, m_Values[sName]);
  }
  m_Values.Swap(overwrittenValues);

  InitializeFromTemplate();
}

void plVolumeComponent::RemoveReloadFunction()
{
  if (m_bReloadFunctionAdded)
  {
    GetWorld()->RemoveResourceReloadFunction(m_hTemplateResource, GetHandle(), nullptr);

    m_bReloadFunctionAdded = false;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plVolumeSphereComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plSphereManipulatorAttribute("Radius"),
    new plSphereVisualizerAttribute("Radius", plColorScheme::LightUI(plColorScheme::Cyan)),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plVolumeSphereComponent::plVolumeSphereComponent() = default;
plVolumeSphereComponent::~plVolumeSphereComponent() = default;

void plVolumeSphereComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void plVolumeSphereComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = plMath::Max(fFalloff, 0.0001f);
}

void plVolumeSphereComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
}

void plVolumeSphereComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
}

void plVolumeSphereComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), m_fRadius), m_SpatialCategory);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plVolumeBoxComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new plDefaultValueAttribute(plVec3(10.0f)), new plClampValueAttribute(plVec3(0), plVariant())),
    PL_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new plDefaultValueAttribute(plVec3(0.5f)), new plClampValueAttribute(plVec3(0.0f), plVec3(1.0f))),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plBoxManipulatorAttribute("Extents", 1.0f, true),
    new plBoxVisualizerAttribute("Extents", 1.0f, plColorScheme::LightUI(plColorScheme::Cyan)),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plVolumeBoxComponent::plVolumeBoxComponent() = default;
plVolumeBoxComponent::~plVolumeBoxComponent() = default;

void plVolumeBoxComponent::SetExtents(const plVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void plVolumeBoxComponent::SetFalloff(const plVec3& vFalloff)
{
  m_vFalloff = vFalloff.CompMax(plVec3(0.0001f));
}

void plVolumeBoxComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vFalloff;
}

void plVolumeBoxComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFalloff;
}

void plVolumeBoxComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(plBoundingBoxSphere::MakeFromBox(plBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), m_SpatialCategory);
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Volumes_Implementation_VolumeComponent);
