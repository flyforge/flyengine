#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgTypeScriptMsgProxy);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgTypeScriptMsgProxy, 1, plRTTIDefaultAllocator<plMsgTypeScriptMsgProxy>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plTypeScriptComponent, 4, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Script", GetTypeScriptComponentFile, SetTypeScriptComponentFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Code_TypeScript")),
    PLASMA_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("Script")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Scripting"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgTypeScriptMsgProxy, OnMsgTypeScriptMsgProxy)
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plTypeScriptComponent::plTypeScriptComponent() = default;
plTypeScriptComponent::~plTypeScriptComponent() = default;

void plTypeScriptComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_TypeScriptComponentGuid;

  // version 3
  plUInt16 uiNumParams = static_cast<plUInt16>(m_Parameters.GetCount());
  s << uiNumParams;

  for (plUInt32 p = 0; p < uiNumParams; ++p)
  {
    s << m_Parameters.GetKey(p);
    s << m_Parameters.GetValue(p);
  }
}

void plTypeScriptComponent::DeserializeComponent(plWorldReader& stream)
{
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion >= 4)
  {
    SUPER::DeserializeComponent(stream);
  }

  auto& s = stream.GetStream();

  s >> m_TypeScriptComponentGuid;

  if (uiVersion >= 3)
  {
    plUInt16 uiNumParams = 0;
    s >> uiNumParams;
    m_Parameters.Reserve(uiNumParams);

    plHashedString key;
    plVariant value;

    for (plUInt32 p = 0; p < uiNumParams; ++p)
    {
      s >> key;
      s >> value;

      m_Parameters.Insert(key, value);
    }
  }

  // reset all user flags
  for (plUInt32 i = 0; i < 8; ++i)
  {
    SetUserFlag(i, false);
  }
}

bool plTypeScriptComponent::HandlesMessage(const plMessage& msg) const
{
  plTypeScriptBinding& binding = static_cast<const plTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  return binding.HasMessageHandler(m_ComponentTypeInfo, msg.GetDynamicRTTI());
}

bool plTypeScriptComponent::OnUnhandledMessage(plMessage& msg, bool bWasPostedMsg)
{
  return HandleUnhandledMessage(msg, bWasPostedMsg);
}

bool plTypeScriptComponent::OnUnhandledMessage(plMessage& msg, bool bWasPostedMsg) const
{
  return const_cast<plTypeScriptComponent*>(this)->HandleUnhandledMessage(msg, bWasPostedMsg);
}

bool plTypeScriptComponent::HandleUnhandledMessage(plMessage& msg, bool bWasPostedMsg)
{
  if (GetUserFlag(UserFlag::ScriptFailure))
    return false;

  plTypeScriptBinding& binding = static_cast<plTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  return binding.DeliverMessage(m_ComponentTypeInfo, this, msg, bWasPostedMsg == false);
}

void plTypeScriptComponent::BroadcastEventMsg(plEventMessage& msg)
{
  const plRTTI* pType = msg.GetDynamicRTTI();

  for (auto& sender : m_EventSenders)
  {
    if (sender.m_pMsgType == pType)
    {
      sender.m_Sender.SendEventMessage(msg, this, GetOwner()->GetParent());
      return;
    }
  }

  auto& sender = m_EventSenders.ExpandAndGetRef();
  sender.m_pMsgType = pType;
  sender.m_Sender.SendEventMessage(msg, this, GetOwner()->GetParent());
}

bool plTypeScriptComponent::CallTsFunc(const char* szFuncName)
{
  if (GetUserFlag(UserFlag::ScriptFailure))
    return false;

  plTypeScriptBinding& binding = static_cast<plTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  plDuktapeHelper duk(binding.GetDukTapeContext());

  binding.DukPutComponentObject(this); // [ comp ]

  if (duk.PrepareMethodCall(szFuncName).Succeeded()) // [ comp func comp ]
  {
    duk.CallPreparedMethod().IgnoreResult(); // [ comp result ]
    duk.PopStack(2);                         // [ ]

    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
  }
  else
  {
    // remove 'this'   [ comp ]
    duk.PopStack(); // [ ]

    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, false, 0);
  }
}

void plTypeScriptComponent::SetExposedVariables()
{
  plTypeScriptBinding& binding = static_cast<plTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  plDuktapeHelper duk(binding.GetDukTapeContext());

  binding.DukPutComponentObject(this); // [ comp ]

  for (plUInt32 p = 0; p < m_Parameters.GetCount(); ++p)
  {
    const auto& pair = m_Parameters.GetPair(p);

    plTypeScriptBinding::SetVariantProperty(duk, pair.key.GetString(), -1, pair.value); // [ comp ]
  }

  duk.PopStack(); // [ ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void plTypeScriptComponent::Initialize()
{
  // SUPER::Initialize() does nothing

  if (!GetUserFlag(UserFlag::SimStartedTS))
    return;

  if (!GetUserFlag(UserFlag::InitializedTS))
  {
    SetUserFlag(UserFlag::InitializedTS, true);

    CallTsFunc("Initialize");
  }
}

void plTypeScriptComponent::Deinitialize()
{
  // mirror what plComponent::Deinitialize does, but make sure to CallTsFunc at the right time

  PLASMA_ASSERT_DEV(GetOwner() != nullptr, "Owner must still be valid");

  if (IsActive())
  {
    SetActiveFlag(false);
  }

  if (GetUserFlag(UserFlag::InitializedTS))
  {
    CallTsFunc("Deinitialize");
  }

  SetUserFlag(UserFlag::InitializedTS, false);
}

void plTypeScriptComponent::OnActivated()
{
  // SUPER::OnActivated() does nothing

  if (!GetUserFlag(UserFlag::SimStartedTS))
    return;

  plTypeScriptComponent::Initialize();

  SetUserFlag(UserFlag::OnActivatedTS, true);

  CallTsFunc("OnActivated");
}

void plTypeScriptComponent::OnDeactivated()
{
  if (GetUserFlag(UserFlag::OnActivatedTS))
  {
    CallTsFunc("OnDeactivated");
  }

  SetUserFlag(UserFlag::OnActivatedTS, false);

  // SUPER::OnDeactivated() does nothing
}

void plTypeScriptComponent::OnSimulationStarted()
{
  plTypeScriptBinding& binding = static_cast<plTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  SetUserFlag(UserFlag::SimStartedTS, true);

  if (binding.LoadComponent(m_TypeScriptComponentGuid, m_ComponentTypeInfo).Failed())
  {
    SetUserFlag(UserFlag::ScriptFailure, true);
    plLog::Error("Failed to load TS component type.");
    return;
  }

  plUInt32 uiStashIdx = 0;
  if (binding.RegisterComponent(m_ComponentTypeInfo.Value().m_sComponentTypeName, GetHandle(), uiStashIdx, false).Failed())
  {
    SetUserFlag(UserFlag::ScriptFailure, true);
    plLog::Error("Failed to register TS component type '{}'. Class may not exist under that name.", m_ComponentTypeInfo.Value().m_sComponentTypeName);
    return;
  }

  // if the TS component has any message handlers, we need to capture all messages and redirect them to the script
  EnableUnhandledMessageHandler(!m_ComponentTypeInfo.Value().m_MessageHandlers.IsEmpty());

  SetExposedVariables();

  plTypeScriptComponent::OnActivated();

  CallTsFunc("OnSimulationStarted");
}

void plTypeScriptComponent::Update(plTypeScriptBinding& binding)
{
  if (GetUserFlag(UserFlag::ScriptFailure) || GetUserFlag(UserFlag::NoTsTick))
    return;

  if (m_UpdateInterval.IsNegative())
    return;

  const plTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (m_LastUpdate + m_UpdateInterval > tNow)
    return;

  PLASMA_PROFILE_SCOPE(GetOwner()->GetName());

  m_LastUpdate = tNow;

  plDuktapeHelper duk(binding.GetDukTapeContext());

  binding.DukPutComponentObject(this); // [ comp ]

  if (duk.PrepareMethodCall("Tick").Succeeded()) // [ comp func comp ]
  {
    duk.CallPreparedMethod().IgnoreResult(); // [ comp result ]
    duk.PopStack(2);                         // [ ]
  }
  else
  {
    // remove 'this'   [ comp ]
    duk.PopStack(); // [ ]

    SetUserFlag(UserFlag::NoTsTick, true);
  }

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void plTypeScriptComponent::SetTypeScriptComponentFile(const char* szFile)
{
  if (plConversionUtils::IsStringUuid(szFile))
  {
    SetTypeScriptComponentGuid(plConversionUtils::ConvertStringToUuid(szFile));
  }
  else
  {
    SetTypeScriptComponentGuid(plUuid());
  }
}

const char* plTypeScriptComponent::GetTypeScriptComponentFile() const
{
  if (m_TypeScriptComponentGuid.IsValid())
  {
    static plStringBuilder sGuid; // need dummy storage
    return plConversionUtils::ToString(m_TypeScriptComponentGuid, sGuid);
  }

  return "";
}

void plTypeScriptComponent::SetTypeScriptComponentGuid(const plUuid& hResource)
{
  m_TypeScriptComponentGuid = hResource;
}

const plUuid& plTypeScriptComponent::GetTypeScriptComponentGuid() const
{
  return m_TypeScriptComponentGuid;
}

void plTypeScriptComponent::OnMsgTypeScriptMsgProxy(plMsgTypeScriptMsgProxy& msg)
{
  if (GetUserFlag(UserFlag::ScriptFailure))
    return;

  plTypeScriptBinding& binding = static_cast<plTypeScriptComponentManager*>(GetOwningManager())->GetTsBinding();

  binding.DeliverTsMessage(m_ComponentTypeInfo, this, msg);
}

const plRangeView<const char*, plUInt32> plTypeScriptComponent::GetParameters() const
{
  return plRangeView<const char*, plUInt32>([]() -> plUInt32
    { return 0; },
    [this]() -> plUInt32
    { return m_Parameters.GetCount(); },
    [](plUInt32& it)
    { ++it; },
    [this](const plUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
}

void plTypeScriptComponent::SetParameter(const char* szKey, const plVariant& value)
{
  plHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != plInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  // GetWorld()->GetComponentManager<plTypeScriptComponentManager>()->AddToUpdateList(this);
}

void plTypeScriptComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(plTempHashedString(szKey)))
  {
    // GetWorld()->GetComponentManager<plTypeScriptComponentManager>()->AddToUpdateList(this);
  }
}

bool plTypeScriptComponent::GetParameter(const char* szKey, plVariant& out_value) const
{
  plUInt32 it = m_Parameters.Find(szKey);

  if (it == plInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}
