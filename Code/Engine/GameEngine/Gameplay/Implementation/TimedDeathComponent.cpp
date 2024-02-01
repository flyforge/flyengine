#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/TimedDeathComponent.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plTimedDeathComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("MinDelay", m_MinDelay)->AddAttributes(new plClampValueAttribute(plTime(), plVariant()), new plDefaultValueAttribute(plTime::MakeFromSeconds(1.0))),
    PL_MEMBER_PROPERTY("DelayRange", m_DelayRange)->AddAttributes(new plClampValueAttribute(plTime(), plVariant())),
    PL_ACCESSOR_PROPERTY("TimeoutPrefab", GetTimeoutPrefab, SetTimeoutPrefab)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab", plDependencyFlags::Package)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnTriggered),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTimedDeathComponent::plTimedDeathComponent() = default;
plTimedDeathComponent::~plTimedDeathComponent() = default;

void plTimedDeathComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_MinDelay;
  s << m_DelayRange;
  s << m_hTimeoutPrefab;
}

void plTimedDeathComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_hTimeoutPrefab;
}

void plTimedDeathComponent::OnSimulationStarted()
{
  plMsgComponentInternalTrigger msg;
  msg.m_sMessage.Assign("Suicide");

  plWorld* pWorld = GetWorld();

  const plTime tKill = plTime::MakeFromSeconds(pWorld->GetRandomNumberGenerator().DoubleInRange(m_MinDelay.GetSeconds(), m_DelayRange.GetSeconds()));

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

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Min Delay", "MinDelay");
    pNode->RenameProperty("Delay Range", "DelayRange");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
  }
};

plTimedDeathComponentPatch_1_2 g_plTimedDeathComponentPatch_1_2;



PL_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_TimedDeathComponent);
