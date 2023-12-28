#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plScriptComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("UpdateInterval", GetUpdateInterval, SetUpdateInterval)->AddAttributes(new plClampValueAttribute(plTime::Zero(), plVariant())),
    PLASMA_ACCESSOR_PROPERTY("ScriptClass", GetScriptClassFile, SetScriptClassFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_ScriptClass")),
    PLASMA_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("ScriptClass")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Scripting"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plScriptComponent::plScriptComponent() = default;
plScriptComponent::~plScriptComponent() = default;

void plScriptComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hScriptClass;
  s << m_UpdateInterval;

  plUInt16 uiNumParams = static_cast<plUInt16>(m_Parameters.GetCount());
  s << uiNumParams;

  for (plUInt32 p = 0; p < uiNumParams; ++p)
  {
    s << m_Parameters.GetKey(p);
    s << m_Parameters.GetValue(p);
  }
}

void plScriptComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hScriptClass;
  s >> m_UpdateInterval;

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

void plScriptComponent::Initialize()
{
  SUPER::Initialize();

  if (m_hScriptClass.IsValid())
  {
    InstantiateScript(false);
  }
}

void plScriptComponent::Deinitialize()
{
  SUPER::Deinitialize();

  ClearInstance(false);
}

void plScriptComponent::OnActivated()
{
  SUPER::OnActivated();

  CallScriptFunction(plComponent_ScriptBaseClassFunctions::OnActivated);
}

void plScriptComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  CallScriptFunction(plComponent_ScriptBaseClassFunctions::OnDeactivated);
}

void plScriptComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  CallScriptFunction(plComponent_ScriptBaseClassFunctions::OnSimulationStarted);
}

bool plScriptComponent::SendEventMessage(plMessage& inout_msg)
{
  auto& sender = FindSender(inout_msg);
  return sender.SendEventMessage(inout_msg, this, GetOwner());
}

void plScriptComponent::PostEventMessage(plMessage& inout_msg, plTime delay)
{
  auto& sender = FindSender(inout_msg);
  sender.PostEventMessage(inout_msg, this, GetOwner(), delay);
}

void plScriptComponent::SetScriptClass(const plScriptClassResourceHandle& hScript)
{
  if (m_hScriptClass == hScript)
    return;

  if (IsInitialized())
  {
    ClearInstance(IsActiveAndInitialized());
  }

  m_hScriptClass = hScript;

  if (IsInitialized() && m_hScriptClass.IsValid())
  {
    InstantiateScript(IsActiveAndInitialized());
  }
}

void plScriptComponent::SetScriptClassFile(const char* szFile)
{
  plScriptClassResourceHandle hScript;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hScript = plResourceManager::LoadResource<plScriptClassResource>(szFile);
  }

  SetScriptClass(hScript);
}

const char* plScriptComponent::GetScriptClassFile() const
{
  return m_hScriptClass.IsValid() ? m_hScriptClass.GetResourceID().GetData() : "";
}

void plScriptComponent::SetUpdateInterval(plTime interval)
{
  m_UpdateInterval = interval;

  if (IsActiveAndInitialized())
  {
    UpdateScheduling();
  }
}

plTime plScriptComponent::GetUpdateInterval() const
{
  return m_UpdateInterval;
}

const plRangeView<const char*, plUInt32> plScriptComponent::GetParameters() const
{
  return plRangeView<const char*, plUInt32>([]() -> plUInt32 { return 0; },
    [this]() -> plUInt32 { return m_Parameters.GetCount(); },
    [](plUInt32& ref_uiIt) { ++ref_uiIt; },
    [this](const plUInt32& uiIt) -> const char* { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void plScriptComponent::SetParameter(const char* szKey, const plVariant& value)
{
  plHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != plInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  if (IsInitialized() && m_hScriptClass.IsValid())
  {
    InstantiateScript(IsActiveAndInitialized());
  }
}

void plScriptComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(plTempHashedString(szKey)))
  {
    if (IsInitialized() && m_hScriptClass.IsValid())
    {
      InstantiateScript(IsActiveAndInitialized());
    }
  }
}

bool plScriptComponent::GetParameter(const char* szKey, plVariant& out_value) const
{
  plUInt32 it = m_Parameters.Find(szKey);

  if (it == plInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void plScriptComponent::InstantiateScript(bool bActivate)
{
  ClearInstance(IsActiveAndInitialized());

  plResourceLock<plScriptClassResource> pScript(m_hScriptClass, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pScript.GetAcquireResult() != plResourceAcquireResult::Final)
  {
    plLog::Error("Failed to load script '{}'", GetScriptClassFile());
    return;
  }

  auto pScriptType = pScript->GetType();
  if (pScriptType == nullptr || pScriptType->IsDerivedFrom(plGetStaticRTTI<plComponent>()) == false)
  {
    plLog::Error("Script type '{}' is not a component", pScriptType != nullptr ? pScriptType->GetTypeName() : "NULL");
    return;
  }

  m_pScriptType = pScriptType;
  m_pMessageDispatchType = pScriptType;

  m_pInstance = pScript->Instantiate(*this, GetWorld());
  if (m_pInstance != nullptr)
  {
    m_pInstance->ApplyParameters(m_Parameters);
  }

  UpdateScheduling();

  CallScriptFunction(plComponent_ScriptBaseClassFunctions::Initialize);
  if (bActivate)
  {
    CallScriptFunction(plComponent_ScriptBaseClassFunctions::OnActivated);
  }
}

void plScriptComponent::ClearInstance(bool bDeactivate)
{
  if (bDeactivate)
  {
    CallScriptFunction(plComponent_ScriptBaseClassFunctions::OnDeactivated);
  }
  CallScriptFunction(plComponent_ScriptBaseClassFunctions::Deinitialize);

  auto pModule = GetWorld()->GetOrCreateModule<plScriptWorldModule>();
  if (auto pUpdateFunction = GetScriptFunction(plComponent_ScriptBaseClassFunctions::Update))
  {
    pModule->RemoveUpdateFunctionToSchedule(pUpdateFunction, m_pInstance.Borrow());
  }

  pModule->StopAndDeleteAllCoroutines(m_pInstance.Borrow());
  pModule->RemoveScriptReloadFunction(m_hScriptClass, plMakeDelegate(&plScriptComponent::ReloadScript, this));

  m_pInstance = nullptr;
  m_pScriptType = nullptr;

  m_pMessageDispatchType = GetDynamicRTTI();
}

void plScriptComponent::UpdateScheduling()
{
  auto pModule = GetWorld()->GetOrCreateModule<plScriptWorldModule>();
  if (auto pUpdateFunction = GetScriptFunction(plComponent_ScriptBaseClassFunctions::Update))
  {
    const bool bOnlyWhenSimulating = true;
    pModule->AddUpdateFunctionToSchedule(pUpdateFunction, m_pInstance.Borrow(), m_UpdateInterval, bOnlyWhenSimulating);
  }

  pModule->AddScriptReloadFunction(m_hScriptClass, plMakeDelegate(&plScriptComponent::ReloadScript, this));
}

const plAbstractFunctionProperty* plScriptComponent::GetScriptFunction(plUInt32 uiFunctionIndex)
{
  if (m_pScriptType != nullptr && m_pInstance != nullptr)
  {
    return m_pScriptType->GetFunctionByIndex(uiFunctionIndex);
  }

  return nullptr;
}

void plScriptComponent::CallScriptFunction(plUInt32 uiFunctionIndex)
{
  if (auto pFunction = GetScriptFunction(uiFunctionIndex))
  {
    plVariant returnValue;
    pFunction->Execute(m_pInstance.Borrow(), plArrayPtr<plVariant>(), returnValue);
  }
}

void plScriptComponent::ReloadScript()
{
  InstantiateScript(IsActiveAndInitialized());
}

plEventMessageSender<plMessage>& plScriptComponent::FindSender(plMessage& inout_msg)
{
  const plRTTI* pType = inout_msg.GetDynamicRTTI();
  if (pType->IsDerivedFrom<plEventMessage>())
  {
    static_cast<plEventMessage&>(inout_msg).FillFromSenderComponent(this);
  }

  for (auto& sender : m_EventSenders)
  {
    if (sender.m_pMsgType == pType)
    {
      return sender.m_Sender;
    }
  }

  auto& sender = m_EventSenders.ExpandAndGetRef();
  sender.m_pMsgType = pType;
  return sender.m_Sender;
}
