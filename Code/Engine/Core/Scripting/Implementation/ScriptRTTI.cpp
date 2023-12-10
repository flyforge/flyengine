#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptRTTI.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Reflection/ReflectionUtils.h>

plScriptRTTI::plScriptRTTI(plStringView sName, const plRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers)
  : plRTTI(nullptr, pParentType, 0, 1, plVariantType::Invalid, plTypeFlags::Class, nullptr, plArrayPtr<const plAbstractProperty*>(), plArrayPtr<const plAbstractFunctionProperty*>(), plArrayPtr<const plPropertyAttribute*>(), plArrayPtr<plAbstractMessageHandler*>(), plArrayPtr<plMessageSenderInfo>(), nullptr)
  , m_sTypeNameStorage(sName)
  , m_FunctionStorage(std::move(functions))
  , m_MessageHandlerStorage(std::move(messageHandlers))
{
  m_sTypeName = m_sTypeNameStorage.GetData();

  for (auto& pFunction : m_FunctionStorage)
  {
    if (pFunction != nullptr)
    {
      m_FunctionRawPtrs.PushBack(pFunction.Borrow());
    }
  }

  for (auto& pMessageHandler : m_MessageHandlerStorage)
  {
    if (pMessageHandler != nullptr)
    {
      m_MessageHandlerRawPtrs.PushBack(pMessageHandler.Borrow());
    }
  }

  m_Functions = m_FunctionRawPtrs;
  m_MessageHandlers = m_MessageHandlerRawPtrs;

  RegisterType();

  SetupParentHierarchy();
  GatherDynamicMessageHandlers();
}

plScriptRTTI::~plScriptRTTI()
{
  UnregisterType();
  m_sTypeName = nullptr;
}

const plAbstractFunctionProperty* plScriptRTTI::GetFunctionByIndex(plUInt32 uiIndex) const
{
  if (uiIndex < m_FunctionStorage.GetCount())
  {
    return m_FunctionStorage.GetData()[uiIndex].Borrow();
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

plScriptFunctionProperty::plScriptFunctionProperty(plStringView sName)
  : plAbstractFunctionProperty(nullptr)
{
  m_sPropertyNameStorage.Assign(sName);
  m_szPropertyName = m_sPropertyNameStorage.GetData();
}

plScriptFunctionProperty::~plScriptFunctionProperty() = default;

//////////////////////////////////////////////////////////////////////////

plScriptMessageHandler::plScriptMessageHandler(const plScriptMessageDesc& desc)
  : m_Properties(desc.m_Properties)
{
  plUniquePtr<plMessage> pMessage = desc.m_pType->GetAllocator()->Allocate<plMessage>();

  m_Id = pMessage->GetId();
  m_bIsConst = true;
}

plScriptMessageHandler::~plScriptMessageHandler() = default;

void plScriptMessageHandler::FillMessagePropertyValues(const plMessage& msg, plDynamicArray<plVariant>& out_propertyValues)
{
  out_propertyValues.Clear();

  for (auto pProp : m_Properties)
  {
    if (pProp->GetCategory() == plPropertyCategory::Member)
    {
      out_propertyValues.PushBack(plReflectionUtils::GetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pProp), &msg));
    }
    else
    {
      PLASMA_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

plScriptInstance::plScriptInstance(plReflectedClass& inout_owner, plWorld* pWorld)
  : m_Owner(inout_owner)
  , m_pWorld(pWorld)
{
}
