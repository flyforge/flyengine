#include <GameEngine/GameEnginePCH.h>

#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachineState_Script.h>

namespace
{
  struct ScriptInstanceData
  {
    plReflectedClass* m_pOwner = nullptr;

    plStateMachineInstance* m_pStateMachineInstance = nullptr;
    const plArrayMap<plHashedString, plVariant>* m_pParameters = nullptr;
    plScriptClassResourceHandle m_hScriptClass;

    plSharedPtr<plScriptRTTI> m_pScriptType;
    plUniquePtr<plScriptInstance> m_pInstance;

    ~ScriptInstanceData()
    {
      ClearInstance();
    }

    void InstantiateScript(const plStateMachineState* pFromState = nullptr)
    {
      ClearInstance();

      plResourceLock<plScriptClassResource> pScript(m_hScriptClass, plResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pScript.GetAcquireResult() != plResourceAcquireResult::Final)
      {
        plLog::Error("Failed to load script '{}'", (m_hScriptClass.IsValid() ? m_hScriptClass.GetResourceID().GetData() : ""));
        return;
      }

      auto pScriptType = pScript->GetType();
      if (pScriptType == nullptr || pScriptType->IsDerivedFrom(plGetStaticRTTI<plStateMachineState>()) == false)
      {
        plLog::Error("Script type '{}' is not a state machine state", pScriptType != nullptr ? pScriptType->GetTypeName() : "NULL");
        return;
      }

      m_pScriptType = pScriptType;

      m_pInstance = pScript->Instantiate(*m_pOwner, m_pStateMachineInstance->GetOwnerWorld());
      if (m_pInstance != nullptr)
      {
        m_pInstance->SetInstanceVariables(*m_pParameters);
      }

      if (plWorld* pWorld = m_pStateMachineInstance->GetOwnerWorld())
      {
        pWorld->AddResourceReloadFunction(m_hScriptClass, plComponentHandle(), this,
          [](plWorld::ResourceReloadContext& context) {
            static_cast<ScriptInstanceData*>(context.m_pUserData)->ReloadScript();
          });
      }

      CallOnEnter(pFromState);
    }

    void ClearInstance()
    {
      CallOnExit(nullptr);

      if (m_pStateMachineInstance != nullptr)
      {
        if (plWorld* pWorld = m_pStateMachineInstance->GetOwnerWorld())
        {
          auto pModule = pWorld->GetOrCreateModule<plScriptWorldModule>();
          pModule->StopAndDeleteAllCoroutines(m_pInstance.Borrow());

          pWorld->RemoveResourceReloadFunction(m_hScriptClass, plComponentHandle(), this);
        }
      }

      m_pInstance = nullptr;
      m_pScriptType = nullptr;
    }

    void ReloadScript()
    {
      InstantiateScript();
    }

    const plAbstractFunctionProperty* GetScriptFunction(plUInt32 uiFunctionIndex)
    {
      if (m_pScriptType != nullptr && m_pInstance != nullptr)
      {
        return m_pScriptType->GetFunctionByIndex(uiFunctionIndex);
      }

      return nullptr;
    }

    void CallOnEnter(const plStateMachineState* pFromState)
    {
      if (auto pFunction = GetScriptFunction(plStateMachineState_ScriptBaseClassFunctions::OnEnter))
      {
        plVariant args[] = {m_pStateMachineInstance, pFromState};
        plVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), plMakeArrayPtr(args), returnValue);
      }
    }

    void CallOnExit(const plStateMachineState* pToState)
    {
      if (auto pFunction = GetScriptFunction(plStateMachineState_ScriptBaseClassFunctions::OnExit))
      {
        plVariant args[] = {m_pStateMachineInstance, pToState};
        plVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), plMakeArrayPtr(args), returnValue);
      }
    }

    void CallUpdate(plTime deltaTime)
    {
      if (auto pFunction = GetScriptFunction(plStateMachineState_ScriptBaseClassFunctions::Update))
      {
        plVariant args[] = {m_pStateMachineInstance, deltaTime};
        plVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), plMakeArrayPtr(args), returnValue);
      }
    }
  };
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineState_Script, 1, plRTTIDefaultAllocator<plStateMachineState_Script>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("ScriptClass", GetScriptClassFile, SetScriptClassFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_ScriptClass")),
    PL_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("ScriptClass")),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineState_Script::plStateMachineState_Script(plStringView sName)
  : plStateMachineState(sName)
{
}

plStateMachineState_Script::~plStateMachineState_Script() = default;

void plStateMachineState_Script::OnEnter(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pFromState) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);

  if (instanceData.m_pInstance == nullptr)
  {
    instanceData.m_pOwner = const_cast<plStateMachineState_Script*>(this);
    instanceData.m_pStateMachineInstance = &ref_instance;
    instanceData.m_pParameters = &m_Parameters;
    instanceData.m_hScriptClass = plResourceManager::LoadResource<plScriptClassResource>(m_sScriptClassFile);

    instanceData.InstantiateScript(pFromState);
  }
  else
  {
    instanceData.CallOnEnter(pFromState);
  }
}

void plStateMachineState_Script::OnExit(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pToState) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);
  instanceData.CallOnExit(pToState);
}

void plStateMachineState_Script::Update(plStateMachineInstance& ref_instance, void* pInstanceData, plTime deltaTime) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);
  instanceData.CallUpdate(deltaTime);
}

plResult plStateMachineState_Script::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sScriptClassFile;

  plUInt16 uiNumParams = static_cast<plUInt16>(m_Parameters.GetCount());
  inout_stream << uiNumParams;

  for (plUInt32 p = 0; p < uiNumParams; ++p)
  {
    inout_stream << m_Parameters.GetKey(p);
    inout_stream << m_Parameters.GetValue(p);
  }

  return PL_SUCCESS;
}

plResult plStateMachineState_Script::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sScriptClassFile;

  plUInt16 uiNumParams = 0;
  inout_stream >> uiNumParams;
  m_Parameters.Reserve(uiNumParams);

  plHashedString key;
  plVariant value;
  for (plUInt32 p = 0; p < uiNumParams; ++p)
  {
    inout_stream >> key;
    inout_stream >> value;

    m_Parameters.Insert(key, value);
  }

  return PL_SUCCESS;
}

bool plStateMachineState_Script::GetInstanceDataDesc(plInstanceDataDesc& out_desc)
{
  out_desc.FillFromType<ScriptInstanceData>();
  return true;
}

void plStateMachineState_Script::SetScriptClassFile(const char* szFile)
{
  m_sScriptClassFile = szFile;

  // Note that we can't load the resource here directly. State machine states are instantiated during
  // state machine asset transform but the script class resource overwrites are not known there so the resource load would fail.
}

const char* plStateMachineState_Script::GetScriptClassFile() const
{
  return m_sScriptClassFile;
}

const plRangeView<const char*, plUInt32> plStateMachineState_Script::GetParameters() const
{
  return plRangeView<const char*, plUInt32>([]() -> plUInt32 { return 0; },
    [this]() -> plUInt32 { return m_Parameters.GetCount(); },
    [](plUInt32& ref_uiIt) { ++ref_uiIt; },
    [this](const plUInt32& uiIt) -> const char* { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void plStateMachineState_Script::SetParameter(const char* szKey, const plVariant& value)
{
  plHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != plInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void plStateMachineState_Script::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(plTempHashedString(szKey)))
  {
  }
}

bool plStateMachineState_Script::GetParameter(const char* szKey, plVariant& out_value) const
{
  plUInt32 it = m_Parameters.Find(szKey);

  if (it == plInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}


PL_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineState_Script);

