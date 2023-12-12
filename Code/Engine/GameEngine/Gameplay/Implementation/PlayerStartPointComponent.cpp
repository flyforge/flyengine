#include <GameEngine/GameEnginePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plPlayerStartPointComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("PlayerPrefab", GetPlayerPrefabFile, SetPlayerPrefabFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
    PLASMA_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("PlayerPrefab")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
    new plColorAttribute(plColorScheme::Gameplay),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 0.5f, plColor::DarkSlateBlue),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plPlayerStartPointComponent::plPlayerStartPointComponent() = default;
plPlayerStartPointComponent::~plPlayerStartPointComponent() = default;

void plPlayerStartPointComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hPlayerPrefab;

  plPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), stream, m_Parameters);
}

void plPlayerStartPointComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hPlayerPrefab;

  if (uiVersion >= 2)
  {
    plPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, stream);
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
  return plRangeView<const char*, plUInt32>([]() -> plUInt32 { return 0; }, [this]() -> plUInt32 { return m_Parameters.GetCount(); }, [](plUInt32& it) { ++it; }, [this](const plUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
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


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_PlayerStartPointComponent);
