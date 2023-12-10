#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plScriptCoroutineHandle, plNoBase, 1, plRTTIDefaultAllocator<plScriptCoroutineHandle>)
PLASMA_END_STATIC_REFLECTED_TYPE;
PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plScriptCoroutineHandle);

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plScriptCoroutine, plNoBase, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY_READ_ONLY("Name", GetName),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_FUNCTION_PROPERTY(UpdateAndSchedule),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plScriptCoroutine::plScriptCoroutine() = default;

plScriptCoroutine::~plScriptCoroutine()
{
  PLASMA_ASSERT_DEV(m_pOwnerModule == nullptr, "Deinitialize was not called");
}

void plScriptCoroutine::UpdateAndSchedule(plTime deltaTimeSinceLastUpdate)
{
  auto result = Update(deltaTimeSinceLastUpdate);

  // Has been deleted during update
  if (m_pOwnerModule == nullptr)
    return;

  if (result.m_State == Result::State::Running)
  {
    // We can safely pass false here since we would not end up here if the coroutine is used in a simulation only function
    // but the simulation is not running because then the outer function should not have been called.
    const bool bOnlyWhenSimulating = false;
    m_pOwnerModule->AddUpdateFunctionToSchedule(GetUpdateFunctionProperty(), this, result.m_MaxDelay, bOnlyWhenSimulating);
  }
  else
  {
    m_pOwnerModule->StopAndDeleteCoroutine(GetHandle());
  }
}

void plScriptCoroutine::Initialize(plScriptCoroutineId id, plStringView sName, plScriptInstance& inout_instance, plScriptWorldModule& inout_ownerModule)
{
  m_Id = id;
  m_sName.Assign(sName);
  m_pInstance = &inout_instance;
  m_pOwnerModule = &inout_ownerModule;
}

void plScriptCoroutine::Deinitialize()
{
  m_pOwnerModule->RemoveUpdateFunctionToSchedule(GetUpdateFunctionProperty(), this);
  m_pOwnerModule = nullptr;
}

// static
const plAbstractFunctionProperty* plScriptCoroutine::GetUpdateFunctionProperty()
{
  static const plAbstractFunctionProperty* pUpdateFunctionProperty = []() -> const plAbstractFunctionProperty* {
    const plRTTI* pType = plGetStaticRTTI<plScriptCoroutine>();
    auto functions = pType->GetFunctions();
    for (auto pFunc : functions)
    {
      if (plStringUtils::IsEqual(pFunc->GetPropertyName(), "UpdateAndSchedule"))
      {
        return pFunc;
      }
    }
    return nullptr;
  }();

  return pUpdateFunctionProperty;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plScriptCoroutineCreationMode, 1)
  PLASMA_ENUM_CONSTANTS(plScriptCoroutineCreationMode::StopOther, plScriptCoroutineCreationMode::DontCreateNew, plScriptCoroutineCreationMode::AllowOverlap)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

plScriptCoroutineRTTI::plScriptCoroutineRTTI(plStringView sName, plUniquePtr<plRTTIAllocator>&& pAllocator)
  : plRTTI(nullptr, plGetStaticRTTI<plScriptCoroutine>(), 0, 1, plVariantType::Invalid, plTypeFlags::Class, nullptr, plArrayPtr<const plAbstractProperty*>(), plArrayPtr<const plAbstractFunctionProperty*>(), plArrayPtr<const plPropertyAttribute*>(), plArrayPtr<plAbstractMessageHandler*>(), plArrayPtr<plMessageSenderInfo>(), nullptr)
  , m_sTypeNameStorage(sName)
  , m_pAllocatorStorage(std::move(pAllocator))
{
  m_sTypeName = m_sTypeNameStorage;
  m_pAllocator = m_pAllocatorStorage.Borrow();

  RegisterType();

  SetupParentHierarchy();
}

plScriptCoroutineRTTI::~plScriptCoroutineRTTI()
{
  UnregisterType();
  m_sTypeName = nullptr;
}

//////////////////////////////////////////////////////////////////////////

plScriptCoroutineFunctionProperty::plScriptCoroutineFunctionProperty(plStringView sName, const plSharedPtr<plScriptCoroutineRTTI>& pType, plScriptCoroutineCreationMode::Enum creationMode)
  : plScriptFunctionProperty(sName)
  , m_pType(pType)
  , m_CreationMode(creationMode)
{
}

plScriptCoroutineFunctionProperty::~plScriptCoroutineFunctionProperty() = default;

void plScriptCoroutineFunctionProperty::Execute(void* pInstance, plArrayPtr<plVariant> arguments, plVariant& out_returnValue) const
{
  PLASMA_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pScriptInstance = static_cast<plScriptInstance*>(pInstance);

  plWorld* pWorld = pScriptInstance->GetWorld();
  if (pWorld == nullptr)
  {
    plLog::Error("Script coroutines need a script instance with a valid plWorld");
    return;
  }

  auto pModule = pWorld->GetOrCreateModule<plScriptWorldModule>();

  plScriptCoroutine* pCoroutine = nullptr;
  auto hCoroutine = pModule->CreateCoroutine(m_pType.Borrow(), m_szPropertyName, *pScriptInstance, m_CreationMode, pCoroutine);

  if (pCoroutine != nullptr)
  {
    plHybridArray<plVariant, 8> finalArgs;
    finalArgs = arguments;
    finalArgs.PushBack(hCoroutine);

    pModule->StartCoroutine(hCoroutine, finalArgs);
  }
}

//////////////////////////////////////////////////////////////////////////

plScriptCoroutineMessageHandler::plScriptCoroutineMessageHandler(plStringView sName, const plScriptMessageDesc& desc, const plSharedPtr<plScriptCoroutineRTTI>& pType, plScriptCoroutineCreationMode::Enum creationMode)
  : plScriptMessageHandler(desc)
  , m_pType(pType)
  , m_CreationMode(creationMode)
{
  m_sName.Assign(sName);
  m_DispatchFunc = &Dispatch;
}

plScriptCoroutineMessageHandler::~plScriptCoroutineMessageHandler() = default;

// static
void plScriptCoroutineMessageHandler::Dispatch(plAbstractMessageHandler* pSelf, void* pInstance, plMessage& ref_msg)
{
  PLASMA_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pHandler = static_cast<plScriptCoroutineMessageHandler*>(pSelf);
  auto pComponent = static_cast<plScriptComponent*>(pInstance);
  auto pScriptInstance = pComponent->GetScriptInstance();

  plWorld* pWorld = pScriptInstance->GetWorld();
  if (pWorld == nullptr)
  {
    plLog::Error("Script coroutines need a script instance with a valid plWorld");
    return;
  }

  auto pModule = pWorld->GetOrCreateModule<plScriptWorldModule>();

  plScriptCoroutine* pCoroutine = nullptr;
  auto hCoroutine = pModule->CreateCoroutine(pHandler->m_pType.Borrow(), pHandler->m_sName, *pScriptInstance, pHandler->m_CreationMode, pCoroutine);

  if (pCoroutine != nullptr)
  {
    plHybridArray<plVariant, 8> arguments;
    pHandler->FillMessagePropertyValues(ref_msg, arguments);
    arguments.PushBack(hCoroutine);

    pModule->StartCoroutine(hCoroutine, arguments);
  }
}