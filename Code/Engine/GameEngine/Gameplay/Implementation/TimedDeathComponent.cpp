#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/TimedDeathComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plTimedDeathComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("MinDelay", m_MinDelay)->AddAttributes(new plClampValueAttribute(plTime(), plVariant()), new plDefaultValueAttribute(plTime::Seconds(1.0))),
    PLASMA_MEMBER_PROPERTY("DelayRange", m_DelayRange)->AddAttributes(new plClampValueAttribute(plTime(), plVariant())),
    PLASMA_ACCESSOR_PROPERTY("TimeoutPrefab", GetTimeoutPrefab, SetTimeoutPrefab)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnTriggered),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTimedDeathComponent::plTimedDeathComponent() = default;
plTimedDeathComponent::~plTimedDeathComponent() = default;

void plTimedDeathComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_MinDelay;
  s << m_DelayRange;
  s << m_hTimeoutPrefab;
}

void plTimedDeathComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_hTimeoutPrefab;
}

void plTimedDeathComponent::OnSimulationStarted()
{
  plMsgComponentInternalTrigger msg;
  msg.m_sMessage.Assign("Suicide");

  plWorld* pWorld = GetWorld();

  const plTime tKill = plTime::Seconds(pWorld->GetRandomNumberGenerator().DoubleInRange(m_MinDelay.GetSeconds(), m_DelayRange.GetSeconds()));

  PostMessage(msg, tKill);

  // make sure the prefab is available when the component dies
  if (m_hTimeoutPrefab.IsValid())
  {
    plResourceManager::PreloadResource(m_hTimeoutPrefab);
  }
}

void plTimedDeathComponent::OnTriggered(plMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != plTempHashedString("Suicide"))
    return;

  if (m_hTimeoutPrefab.IsValid())
  {
    plResourceLock<plPrefabResource> pPrefab(m_hTimeoutPrefab, plResourceAcquireMode::AllowLoadingFallback);

    plPrefabInstantiationOptions options;
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), options);
  }

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}

void plTimedDeathComponent::SetTimeoutPrefab(const char* szPrefab)
{
  plPrefabResourceHandle hPrefab;

  if (!plStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = plResourceManager::LoadResource<plPrefabResource>(szPrefab);
  }

  m_hTimeoutPrefab = hPrefab;
}


const char* plTimedDeathComponent::GetTimeoutPrefab() const
{
  if (!m_hTimeoutPrefab.IsValid())
    return "";

  return m_hTimeoutPrefab.GetResourceID();
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plTimedDeathComponentPatch_1_2 : public plGraphPatch
{
public:
  plTimedDeathComponentPatch_1_2()
    : plGraphPatch("plTimedDeathComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Min Delay", "MinDelay");
    pNode->RenameProperty("Delay Range", "DelayRange");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
  }
};

plTimedDeathComponentPatch_1_2 g_plTimedDeathComponentPatch_1_2;



PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_TimedDeathComponent);
