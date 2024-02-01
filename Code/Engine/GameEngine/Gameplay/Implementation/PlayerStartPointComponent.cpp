#include <GameEngine/GameEnginePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plPlayerStartPointComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("PlayerPrefab", GetPlayerPrefabFile, SetPlayerPrefabFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab", plDependencyFlags::Package)),
    PL_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("PlayerPrefab")),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 0.5f, plColor::DarkSlateBlue),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plPlayerStartPointComponent::plPlayerStartPointComponent() = default;
plPlayerStartPointComponent::~plPlayerStartPointComponent() = default;

void plPlayerStartPointComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hPlayerPrefab;

  plPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), inout_stream, m_Parameters);
}

void plPlayerStartPointComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hPlayerPrefab;

  if (uiVersion >= 2)
  {
    plPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, inout_stream);
  }
}

void plPlayerStartPointComponent::SetPlayerPrefabFile(const char* szFile)
{
  plPrefabResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plPrefabResource>(szFile);
  }

  SetPlayerPrefab(hResource);
}

const char* plPlayerStartPointComponent::GetPlayerPrefabFile() const
{
  if (!m_hPlayerPrefab.IsValid())
    return "";

  return m_hPlayerPrefab.GetResourceID();
}

void plPlayerStartPointComponent::SetPlayerPrefab(const plPrefabResourceHandle& hPrefab)
{
  m_hPlayerPrefab = hPrefab;
}

const plPrefabResourceHandle& plPlayerStartPointComponent::GetPlayerPrefab() const
{
  return m_hPlayerPrefab;
}

const plRangeView<const char*, plUInt32> plPlayerStartPointComponent::GetParameters() const
{
  return plRangeView<const char*, plUInt32>([]() -> plUInt32 { return 0; }, [this]() -> plUInt32 { return m_Parameters.GetCount(); }, [](plUInt32& ref_uiIt) { ++ref_uiIt; }, [this](const plUInt32& uiIt) -> const char* { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void plPlayerStartPointComponent::SetParameter(const char* szKey, const plVariant& value)
{
  plHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != plInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void plPlayerStartPointComponent::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(plTempHashedString(szKey));
}

bool plPlayerStartPointComponent::GetParameter(const char* szKey, plVariant& out_value) const
{
  plUInt32 it = m_Parameters.Find(szKey);

  if (it == plInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_PlayerStartPointComponent);
