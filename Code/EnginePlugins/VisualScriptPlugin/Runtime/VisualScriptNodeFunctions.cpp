#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

using ExecResult = plVisualScriptGraphDescription::ExecResult;
using ExecuteFunctionGetter = plVisualScriptGraphDescription::ExecuteFunction (*)(plVisualScriptDataType::Enum dataType);

#define MAKE_EXEC_FUNC_GETTER(funcName)                                                                               \
  plVisualScriptGraphDescription::ExecuteFunction PLASMA_CONCAT(funcName, _Getter)(plVisualScriptDataType::Enum dataType) \
  {                                                                                                                   \
    static plVisualScriptGraphDescription::ExecuteFunction functionTable[] = {                                        \
      nullptr, /* Invalid*/                                                                                           \
      &funcName<bool>,                                                                                                \
      &funcName<plUInt8>,                                                                                             \
      &funcName<plInt32>,                                                                                             \
      &funcName<plInt64>,                                                                                             \
      &funcName<float>,                                                                                               \
      &funcName<double>,                                                                                              \
      &funcName<plColor>,                                                                                             \
      &funcName<plVec3>,                                                                                              \
      &funcName<plQuat>,                                                                                              \
      &funcName<plTransform>,                                                                                         \
      &funcName<plTime>,                                                                                              \
      &funcName<plAngle>,                                                                                             \
      &funcName<plString>,                                                                                            \
      &funcName<plHashedString>,                                                                                      \
      &funcName<plGameObjectHandle>,                                                                                  \
      &funcName<plComponentHandle>,                                                                                   \
      &funcName<plTypedPointer>,                                                                                      \
      &funcName<plVariant>,                                                                                           \
      &funcName<plVariantArray>,                                                                                      \
      &funcName<plVariantDictionary>,                                                                                 \
      &funcName<plScriptCoroutineHandle>,                                                                             \
    };                                                                                                                \
                                                                                                                      \
    static_assert(PLASMA_ARRAY_SIZE(functionTable) == plVisualScriptDataType::Count);                                     \
    if (dataType >= 0 && dataType < PLASMA_ARRAY_SIZE(functionTable))                                                     \
      return functionTable[dataType];                                                                                 \
                                                                                                                      \
    plLog::Error("Invalid data type for deducted type {}. Script needs re-transform.", dataType);                     \
    return nullptr;                                                                                                   \
  }

template <typename T>
plStringView GetTypeName()
{
  if constexpr (std::is_same_v<T, plTypedPointer>)
  {
    return "plTypePointer";
  }
  else
  {
    return plGetStaticRTTI<T>()->GetTypeName();
  }
}

namespace
{
  static PLASMA_FORCE_INLINE plResult FillFunctionArgs(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node, const plAbstractFunctionProperty* pFunction, plUInt32 uiStartSlot, plDynamicArray<plVariant>& out_args)
  {
    const plUInt32 uiArgCount = pFunction->GetArgumentCount();
    if (uiArgCount != node.m_NumInputDataOffsets - uiStartSlot)
    {
      plLog::Error("Visual script {} '{}': Argument count mismatch. Script needs re-transform.", plVisualScriptNodeDescription::Type::GetName(node.m_Type), pFunction->GetPropertyName());
      return PLASMA_FAILURE;
    }

    for (plUInt32 i = 0; i < uiArgCount; ++i)
    {
      const plRTTI* pArgType = pFunction->GetArgumentType(i);
      out_args.PushBack(inout_context.GetDataAsVariant(node.GetInputDataOffset(uiStartSlot + i), pArgType));
    }

    return PLASMA_SUCCESS;
  }

  static PLASMA_FORCE_INLINE plScriptWorldModule* GetScriptModule(plVisualScriptExecutionContext& inout_context)
  {
    plWorld* pWorld = inout_context.GetInstance().GetWorld();
    if (pWorld == nullptr)
    {
      plLog::Error("Visual script coroutines need a script instance with a valid plWorld");
      return nullptr;
    }

    return pWorld->GetOrCreateModule<plScriptWorldModule>();
  }

  static ExecResult NodeFunction_ReflectedFunction(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    PLASMA_ASSERT_DEBUG(userData.m_pProperty->GetCategory() == plPropertyCategory::Function, "Property '{}' is not a function", userData.m_pProperty->GetPropertyName());
    auto pFunction = static_cast<const plAbstractFunctionProperty*>(userData.m_pProperty);

    plTypedPointer pInstance;
    plUInt32 uiSlot = 0;

    if (pFunction->GetFunctionType() == plFunctionType::Member)
    {
      pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));
      if (pInstance.m_pObject == nullptr)
      {
        plLog::Error("Visual script function call '{}': Target object is invalid (nullptr)", pFunction->GetPropertyName());
        return ExecResult::Error();
      }

      if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
      {
        plLog::Error("Visual script function call '{}': Target object is not of expected type '{}'", pFunction->GetPropertyName(), userData.m_pType->GetTypeName());
        return ExecResult::Error();
      }

      ++uiSlot;
    }

    plHybridArray<plVariant, 8> args;
    if (FillFunctionArgs(inout_context, node, pFunction, uiSlot, args).Failed())
    {
      return ExecResult::Error();
    }

    plVariant returnValue;
    pFunction->Execute(pInstance.m_pObject, args, returnValue);

    auto dataOffsetR = node.GetOutputDataOffset(0);
    if (dataOffsetR.IsValid())
    {
      inout_context.SetDataFromVariant(dataOffsetR, returnValue);
    }

    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_GetReflectedProperty(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    auto pProperty = userData.m_pProperty;

    plTypedPointer pInstance;
    pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));

    if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
    {
      plLog::Error("Visual script get property '{}': Target object is not of expected type '{}'", pProperty->GetPropertyName(), userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    if (pProperty->GetCategory() == plPropertyCategory::Member)
    {
      auto pMemberProperty = static_cast<const plAbstractMemberProperty*>(pProperty);

      if constexpr (std::is_same_v<T, plGameObjectHandle> ||
                    std::is_same_v<T, plComponentHandle> ||
                    std::is_same_v<T, plTypedPointer>)
      {
        PLASMA_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        PLASMA_ASSERT_DEBUG(pProperty->GetSpecificType() == plGetStaticRTTI<T>(), "");

        T value;
        pMemberProperty->GetValuePtr(pInstance.m_pObject, &value);
        inout_context.SetData(node.GetOutputDataOffset(0), value);
      }
    }
    else
    {
      PLASMA_ASSERT_NOT_IMPLEMENTED;
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_GetReflectedProperty);

  template <typename T>
  static ExecResult NodeFunction_SetReflectedProperty(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    auto pProperty = userData.m_pProperty;

    plTypedPointer pInstance;
    pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));

    if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
    {
      plLog::Error("Visual script get property '{}': Target object is not of expected type '{}'", pProperty->GetPropertyName(), userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    if (pProperty->GetCategory() == plPropertyCategory::Member)
    {
      auto pMemberProperty = static_cast<const plAbstractMemberProperty*>(pProperty);

      if constexpr (std::is_same_v<T, plGameObjectHandle> ||
                    std::is_same_v<T, plComponentHandle> ||
                    std::is_same_v<T, plTypedPointer>)
      {
        PLASMA_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        PLASMA_ASSERT_DEBUG(pProperty->GetSpecificType() == plGetStaticRTTI<T>(), "");

        const T& value = inout_context.GetData<T>(node.GetInputDataOffset(1));
        pMemberProperty->SetValuePtr(pInstance.m_pObject, &value);
      }
    }
    else
    {
      PLASMA_ASSERT_NOT_IMPLEMENTED;
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_SetReflectedProperty);

  static ExecResult NodeFunction_InplaceCoroutine(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    plScriptCoroutine* pCoroutine = inout_context.GetCurrentCoroutine();
    if (pCoroutine == nullptr)
    {
      auto pModule = GetScriptModule(inout_context);
      if (pModule == nullptr)
        return ExecResult::Error();

      auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
      pModule->CreateCoroutine(userData.m_pType, userData.m_pType->GetTypeName(), inout_context.GetInstance(), plScriptCoroutineCreationMode::AllowOverlap, pCoroutine);

      PLASMA_ASSERT_DEBUG(userData.m_pProperty->GetCategory() == plPropertyCategory::Function, "Property '{}' is not a function", userData.m_pProperty->GetPropertyName());
      auto pFunction = static_cast<const plAbstractFunctionProperty*>(userData.m_pProperty);

      plHybridArray<plVariant, 8> args;
      if (FillFunctionArgs(inout_context, node, pFunction, 0, args).Failed())
      {
        return ExecResult::Error();
      }

      pCoroutine->Start(args);

      inout_context.SetCurrentCoroutine(pCoroutine);
    }

    auto result = pCoroutine->Update(inout_context.GetDeltaTimeSinceLastExecution());
    if (result.m_State == plScriptCoroutine::Result::State::Running)
    {
      return ExecResult::ContinueLater(result.m_MaxDelay);
    }

    plWorld* pWorld = inout_context.GetInstance().GetWorld();
    auto pModule = pWorld->GetOrCreateModule<plScriptWorldModule>();
    pModule->StopAndDeleteCoroutine(pCoroutine->GetHandle());
    inout_context.SetCurrentCoroutine(nullptr);

    return ExecResult::RunNext(result.m_State == plScriptCoroutine::Result::State::Completed ? 0 : 1);
  }

  static ExecResult NodeFunction_GetScriptOwner(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    plWorld* pWorld = inout_context.GetInstance().GetWorld();
    inout_context.SetPointerData(node.GetOutputDataOffset(0), pWorld, plGetStaticRTTI<plWorld>());

    plReflectedClass& owner = inout_context.GetInstance().GetOwner();
    if (auto pComponent = plDynamicCast<plComponent*>(&owner))
    {
      inout_context.SetPointerData(node.GetOutputDataOffset(1), pComponent->GetOwner());
      inout_context.SetPointerData(node.GetOutputDataOffset(2), pComponent);
    }
    else
    {
      inout_context.SetPointerData(node.GetOutputDataOffset(1), &owner, owner.GetDynamicRTTI());
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_SendMessage(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperties>();
    auto targetObjectDataOffset = node.GetInputDataOffset(0);
    auto targetComponentDataOffset = node.GetInputDataOffset(1);

    auto pTargetObject = targetObjectDataOffset.IsValid() ? static_cast<plGameObject*>(inout_context.GetPointerData(targetObjectDataOffset).m_pObject) : nullptr;
    auto pTargetComponent = targetComponentDataOffset.IsValid() ? static_cast<plComponent*>(inout_context.GetPointerData(targetComponentDataOffset).m_pObject) : nullptr;
    if (pTargetObject == nullptr && pTargetComponent == nullptr)
    {
      plLog::Error("Visual script send '{}': Invalid target game object and component.", userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    auto mode = static_cast<plVisualScriptSendMessageMode::Enum>(inout_context.GetData<plInt64>(node.GetInputDataOffset(2)));
    plTime delay = inout_context.GetData<plTime>(node.GetInputDataOffset(3));

    plScriptComponent* pSenderComponent = nullptr;
    if (mode == plVisualScriptSendMessageMode::Event)
    {
      pSenderComponent = plDynamicCast<plScriptComponent*>(&inout_context.GetInstance().GetOwner());
    }

    const plUInt32 uiStartSlot = 4;

    plUniquePtr<plMessage> pMessage = userData.m_pType->GetAllocator()->Allocate<plMessage>();
    for (plUInt32 i = 0; i < userData.m_uiNumProperties; ++i)
    {
      auto pProp = userData.m_Properties[i];
      const plRTTI* pPropType = pProp->GetSpecificType();
      plVariant value = inout_context.GetDataAsVariant(node.GetInputDataOffset(uiStartSlot + i), pPropType);

      if (pProp->GetCategory() == plPropertyCategory::Member)
      {
        plReflectionUtils::SetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pProp), pMessage.Borrow(), value);
      }
      else
      {
        PLASMA_ASSERT_NOT_IMPLEMENTED;
      }
    }

    bool bWriteOutputs = false;
    if (pTargetComponent != nullptr)
    {
      if (delay.IsPositive())
      {
        pTargetComponent->PostMessage(*pMessage, delay);
      }
      else
      {
        bWriteOutputs = pTargetComponent->SendMessage(*pMessage);
      }
    }
    else if (pTargetObject != nullptr)
    {
      if (delay.IsPositive())
      {
        if (mode == plVisualScriptSendMessageMode::Direct)
          pTargetObject->PostMessage(*pMessage, delay);
        else if (mode == plVisualScriptSendMessageMode::Recursive)
          pTargetObject->PostMessageRecursive(*pMessage, delay);
        else
          pTargetObject->PostEventMessage(*pMessage, pSenderComponent, delay);
      }
      else
      {
        if (mode == plVisualScriptSendMessageMode::Direct)
          bWriteOutputs = pTargetObject->SendMessage(*pMessage);
        else if (mode == plVisualScriptSendMessageMode::Recursive)
          bWriteOutputs = pTargetObject->SendMessageRecursive(*pMessage);
        else
          bWriteOutputs = pTargetObject->SendEventMessage(*pMessage, pSenderComponent);
      }
    }

    if (bWriteOutputs)
    {
      for (plUInt32 i = 0; i < userData.m_uiNumProperties; ++i)
      {
        auto dataOffset = node.GetOutputDataOffset(i);
        if (dataOffset.IsValid() == false)
          continue;

        auto pProp = userData.m_Properties[i];
        plVariant value;

        if (pProp->GetCategory() == plPropertyCategory::Member)
        {
          value = plReflectionUtils::GetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pProp), pMessage.Borrow());
        }
        else
        {
          PLASMA_ASSERT_NOT_IMPLEMENTED;
        }

        inout_context.SetDataFromVariant(dataOffset, value);
      }
    }

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_SetVariable(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, plGameObjectHandle> ||
                  std::is_same_v<T, plComponentHandle> ||
                  std::is_same_v<T, plTypedPointer>)
    {
      plTypedPointer ptr = inout_context.GetPointerData(node.GetInputDataOffset(0));
      inout_context.SetPointerData(node.GetOutputDataOffset(0), ptr.m_pObject, ptr.m_pType);
    }
    else
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<T>(node.GetInputDataOffset(0)));
    }
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_SetVariable);

  template <typename T>
  static ExecResult NodeFunction_Builtin_IncVariable(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt32> ||
                  std::is_same_v<T, plInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double>)
    {
      T a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      inout_context.SetData(node.GetOutputDataOffset(0), ++a);
    }
    else
    {
      plLog::Error("Increment is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IncVariable);

  template <typename T>
  static ExecResult NodeFunction_Builtin_DecVariable(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt32> ||
                  std::is_same_v<T, plInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double>)
    {
      T a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      inout_context.SetData(node.GetOutputDataOffset(0), --a);
    }
    else
    {
      plLog::Error("Decrement is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_DecVariable);

  static ExecResult NodeFunction_Builtin_Branch(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    bool bCondition = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    return ExecResult::RunNext(bCondition ? 0 : 1);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_Switch(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    plInt64 iValue = 0;
    if constexpr (std::is_same_v<T, plInt64>)
    {
      iValue = inout_context.GetData<plInt64>(node.GetInputDataOffset(0));
    }
    else if constexpr (std::is_same_v<T, plHashedString>)
    {
      iValue = inout_context.GetData<plHashedString>(node.GetInputDataOffset(0)).GetHash();
    }
    else
    {
      PLASMA_ASSERT_NOT_IMPLEMENTED;
    }

    auto& userData = node.GetUserData<NodeUserData_Switch>();
    for (plUInt32 i = 0; i < userData.m_uiNumCases; ++i)
    {
      if (iValue == userData.m_Cases[i])
      {
        return ExecResult::RunNext(i);
      }
    }

    return ExecResult::RunNext(userData.m_uiNumCases);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Switch);

  static ExecResult NodeFunction_Builtin_And(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    bool b = inout_context.GetData<bool>(node.GetInputDataOffset(1));
    inout_context.SetData(node.GetOutputDataOffset(0), a && b);
    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Or(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    bool b = inout_context.GetData<bool>(node.GetInputDataOffset(1));
    inout_context.SetData(node.GetOutputDataOffset(0), a || b);
    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Not(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    inout_context.SetData(node.GetOutputDataOffset(0), !a);
    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_Compare(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Comparison>();
    bool bRes = false;

    if constexpr (std::is_same_v<T, bool> ||
                  std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt32> ||
                  std::is_same_v<T, plInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, plColor> ||
                  std::is_same_v<T, plVec3> ||
                  std::is_same_v<T, plTime> ||
                  std::is_same_v<T, plAngle> ||
                  std::is_same_v<T, plString> ||
                  std::is_same_v<T, plHashedString>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      bRes = plComparisonOperator::Compare(userData.m_ComparisonOperator, a, b);
    }
    else if constexpr (std::is_same_v<T, plGameObjectHandle> ||
                       std::is_same_v<T, plComponentHandle> ||
                       std::is_same_v<T, plTypedPointer>)
    {
      plTypedPointer a = inout_context.GetPointerData(node.GetInputDataOffset(0));
      plTypedPointer b = inout_context.GetPointerData(node.GetInputDataOffset(1));
      bRes = plComparisonOperator::Compare(userData.m_ComparisonOperator, a.m_pObject, b.m_pObject);
    }
    else if constexpr (std::is_same_v<T, plVariant>)
    {
      plVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      plVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);

      if (userData.m_ComparisonOperator == plComparisonOperator::Equal)
      {
        bRes = a == b;
      }
      else if (userData.m_ComparisonOperator == plComparisonOperator::NotEqual)
      {
        bRes = a != b;
      }
      else
      {
        plStringBuilder sCompOp;
        plReflectionUtils::EnumerationToString(userData.m_ComparisonOperator, sCompOp, plReflectionUtils::EnumConversionMode::ValueNameOnly);

        plLog::Error("Comparison '{}' is not defined for type '{}'", sCompOp, GetTypeName<T>());
      }
    }
    else if constexpr (std::is_same_v<T, plQuat> ||
                       std::is_same_v<T, plTransform> ||
                       std::is_same_v<T, plVariantArray> ||
                       std::is_same_v<T, plVariantDictionary>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));

      if (userData.m_ComparisonOperator == plComparisonOperator::Equal)
      {
        bRes = a == b;
      }
      else if (userData.m_ComparisonOperator == plComparisonOperator::NotEqual)
      {
        bRes = a != b;
      }
      else
      {
        plStringBuilder sCompOp;
        plReflectionUtils::EnumerationToString(userData.m_ComparisonOperator, sCompOp, plReflectionUtils::EnumConversionMode::ValueNameOnly);

        plLog::Error("Comparison '{}' is not defined for type '{}'", sCompOp, GetTypeName<T>());
      }
    }
    else
    {
      plLog::Error("Comparison is not defined for type '{}'", GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bRes);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Compare);

  template <typename T>
  static ExecResult NodeFunction_Builtin_IsValid(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bIsValid = true;
    if constexpr (std::is_same_v<T, float>)
    {
      bIsValid = plMath::IsFinite(inout_context.GetData<float>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, double>)
    {
      bIsValid = plMath::IsFinite(inout_context.GetData<double>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, plColor>)
    {
      bIsValid = inout_context.GetData<plColor>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, plVec3>)
    {
      bIsValid = inout_context.GetData<plVec3>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, plQuat>)
    {
      bIsValid = inout_context.GetData<plQuat>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, plString>)
    {
      bIsValid = inout_context.GetData<plString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same_v<T, plHashedString>)
    {
      bIsValid = inout_context.GetData<plHashedString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same_v<T, plGameObjectHandle> ||
                       std::is_same_v<T, plComponentHandle> ||
                       std::is_same_v<T, plTypedPointer>)
    {
      bIsValid = inout_context.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same_v<T, plVariant>)
    {
      bIsValid = inout_context.GetData<plVariant>(dataOffset).IsValid();
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bIsValid);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IsValid);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_Add(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt32> ||
                  std::is_same_v<T, plInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, plColor> ||
                  std::is_same_v<T, plVec3> ||
                  std::is_same_v<T, plTime> ||
                  std::is_same_v<T, plAngle>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a + b));
    }
    else if constexpr (std::is_same_v<T, plString>)
    {
      auto& a = inout_context.GetData<plString>(node.GetInputDataOffset(0));
      auto& b = inout_context.GetData<plString>(node.GetInputDataOffset(1));

      plStringBuilder s;
      s.Set(a, b);

      inout_context.SetData(node.GetOutputDataOffset(0), plString(s.GetView()));
    }
    else if constexpr (std::is_same_v<T, plHashedString>)
    {
      auto& a = inout_context.GetData<plHashedString>(node.GetInputDataOffset(0));
      auto& b = inout_context.GetData<plHashedString>(node.GetInputDataOffset(1));

      plStringBuilder s;
      s.Set(a, b);
      plHashedString sHashed;
      sHashed.Assign(s);

      inout_context.SetData(node.GetOutputDataOffset(0), sHashed);
    }
    else if constexpr (std::is_same_v<T, plVariant>)
    {
      plVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      plVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a + b);
    }
    else
    {
      plLog::Error("Add is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Add);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Sub(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt32> ||
                  std::is_same_v<T, plInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, plColor> ||
                  std::is_same_v<T, plVec3> ||
                  std::is_same_v<T, plTime> ||
                  std::is_same_v<T, plAngle>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a - b));
    }
    else if constexpr (std::is_same_v<T, plVariant>)
    {
      plVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      plVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a - b);
    }
    else
    {
      plLog::Error("Subtract is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Sub);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Mul(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt32> ||
                  std::is_same_v<T, plInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, plColor> ||
                  std::is_same_v<T, plTime>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a * b));
    }
    else if constexpr (std::is_same_v<T, plVec3>)
    {
      const plVec3& a = inout_context.GetData<plVec3>(node.GetInputDataOffset(0));
      const plVec3& b = inout_context.GetData<plVec3>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompMul(b));
    }
    else if constexpr (std::is_same_v<T, plAngle>)
    {
      const plAngle& a = inout_context.GetData<plAngle>(node.GetInputDataOffset(0));
      const plAngle& b = inout_context.GetData<plAngle>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), plAngle(a * b.GetRadian()));
    }
    else if constexpr (std::is_same_v<T, plVariant>)
    {
      plVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      plVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a * b);
    }
    else
    {
      plLog::Error("Multiply is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Mul);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Div(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt32> ||
                  std::is_same_v<T, plInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, plTime>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a / b));
    }
    else if constexpr (std::is_same_v<T, plVec3>)
    {
      const plVec3& a = inout_context.GetData<plVec3>(node.GetInputDataOffset(0));
      const plVec3& b = inout_context.GetData<plVec3>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompDiv(b));
    }
    else if constexpr (std::is_same_v<T, plAngle>)
    {
      const plAngle& a = inout_context.GetData<plAngle>(node.GetInputDataOffset(0));
      const plAngle& b = inout_context.GetData<plAngle>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), plAngle(a / b.GetRadian()));
    }
    else if constexpr (std::is_same_v<T, plVariant>)
    {
      plVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      plVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a / b);
    }
    else
    {
      plLog::Error("Divide is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Div);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToBool(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bRes = false;
    if constexpr (std::is_same_v<T, bool>)
    {
      bRes = inout_context.GetData<T>(dataOffset);
    }
    else if constexpr (std::is_same_v<T, plUInt8> ||
                       std::is_same_v<T, plInt32> ||
                       std::is_same_v<T, plInt64> ||
                       std::is_same_v<T, float> ||
                       std::is_same_v<T, double>)
    {
      bRes = inout_context.GetData<T>(dataOffset) != 0;
    }
    else if constexpr (std::is_same_v<T, plGameObjectHandle> ||
                       std::is_same_v<T, plComponentHandle> ||
                       std::is_same_v<T, plTypedPointer>)
    {
      bRes = inout_context.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same_v<T, plVariant>)
    {
      bRes = inout_context.GetData<plVariant>(dataOffset).ConvertTo<bool>();
    }
    else
    {
      plLog::Error("ToBool is not defined for type '{}'", GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bRes);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToBool);

  template <typename NumberType, typename T>
  PLASMA_FORCE_INLINE static ExecResult NodeFunction_Builtin_ToNumber(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node, const char* szName)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    NumberType res = 0;
    if constexpr (std::is_same_v<T, bool>)
    {
      res = inout_context.GetData<T>(dataOffset) ? 1 : 0;
    }
    else if constexpr (std::is_same_v<T, plUInt8> ||
                       std::is_same_v<T, plInt32> ||
                       std::is_same_v<T, plInt64> ||
                       std::is_same_v<T, float> ||
                       std::is_same_v<T, double>)
    {
      res = static_cast<NumberType>(inout_context.GetData<T>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, plVariant>)
    {
      res = inout_context.GetData<plVariant>(dataOffset).ConvertTo<NumberType>();
    }
    else
    {
      plLog::Error("To{} is not defined for type '{}'", szName, GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), res);
    return ExecResult::RunNext(0);
  }

#define MAKE_TONUMBER_EXEC_FUNC(NumberType, Name)                                                                                                              \
  template <typename T>                                                                                                                                        \
  static ExecResult PLASMA_CONCAT(NodeFunction_Builtin_To, Name)(plVisualScriptExecutionContext & inout_context, const plVisualScriptGraphDescription::Node& node) \
  {                                                                                                                                                            \
    return NodeFunction_Builtin_ToNumber<NumberType, T>(inout_context, node, #Name);                                                                           \
  }

  MAKE_TONUMBER_EXEC_FUNC(plUInt8, Byte);
  MAKE_TONUMBER_EXEC_FUNC(plInt32, Int);
  MAKE_TONUMBER_EXEC_FUNC(plInt64, Int64);
  MAKE_TONUMBER_EXEC_FUNC(float, Float);
  MAKE_TONUMBER_EXEC_FUNC(double, Double);

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToByte);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToInt);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToInt64);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToFloat);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToDouble);

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToString(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    plStringBuilder sb;
    plStringView s;
    if constexpr (std::is_same_v<T, plGameObjectHandle> ||
                  std::is_same_v<T, plComponentHandle> ||
                  std::is_same_v<T, plTypedPointer>)
    {
      plTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
      sb.Format("{} {}", p.m_pType->GetTypeName(), plArgP(p.m_pObject));
      s = sb;
    }
    else if constexpr (std::is_same_v<T, plString>)
    {
      s = inout_context.GetData<plString>(node.GetInputDataOffset(0));
    }
    else
    {
      s = plConversionUtils::ToString(inout_context.GetData<T>(node.GetInputDataOffset(0)), sb);
    }

    inout_context.SetData(node.GetOutputDataOffset(0), s);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToString);

  static ExecResult NodeFunction_Builtin_String_Format(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto& sText = inout_context.GetData<plString>(node.GetInputDataOffset(0));
    auto& params = inout_context.GetData<plVariantArray>(node.GetInputDataOffset(1));

    plHybridArray<plString, 12> stringStorage;
    stringStorage.Reserve(params.GetCount());
    for (auto& param : params)
    {
      stringStorage.PushBack(param.ConvertTo<plString>());
    }

    plHybridArray<plStringView, 12> stringViews;
    stringViews.Reserve(stringStorage.GetCount());
    for (auto& s : stringStorage)
    {
      stringViews.PushBack(s);
    }

    plFormatString fs(sText.GetView());
    plStringBuilder sStorage;
    plStringView sFormatted = fs.BuildFormattedText(sStorage, stringViews.GetData(), stringViews.GetCount());

    inout_context.SetData(node.GetOutputDataOffset(0), sFormatted);
    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToHashedString(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    plStringBuilder sb;
    plStringView s;
    if constexpr (std::is_same_v<T, plGameObjectHandle> ||
                  std::is_same_v<T, plComponentHandle> ||
                  std::is_same_v<T, plTypedPointer>)
    {
      plTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
      sb.Format("{} {}", p.m_pType->GetTypeName(), plArgP(p.m_pObject));
      s = sb;
    }
    else if constexpr (std::is_same_v<T, plString>)
    {
      s = inout_context.GetData<plString>(node.GetInputDataOffset(0));
    }
    else if constexpr (std::is_same_v<T, plHashedString>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<plHashedString>(node.GetInputDataOffset(0)));
      return ExecResult::RunNext(0);
    }
    else
    {
      s = plConversionUtils::ToString(inout_context.GetData<T>(node.GetInputDataOffset(0)), sb);
    }

    plHashedString sHashed;
    sHashed.Assign(s);
    inout_context.SetData(node.GetOutputDataOffset(0), sHashed);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToHashedString);

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToVariant(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    plVariant v;
    if constexpr (std::is_same_v<T, plTypedPointer>)
    {
      plTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
      v = plVariant(p.m_pObject, p.m_pType);
    }
    else
    {
      v = inout_context.GetData<T>(node.GetInputDataOffset(0));
    }
    inout_context.SetData(node.GetOutputDataOffset(0), v);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToVariant);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Variant_ConvertTo(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    const plVariant& v = inout_context.GetData<plVariant>(node.GetInputDataOffset(0));
    if constexpr (std::is_same_v<T, plTypedPointer>)
    {
      if (v.IsA<plTypedPointer>())
      {
        plTypedPointer typedPtr = v.Get<plTypedPointer>();
        inout_context.SetPointerData(node.GetOutputDataOffset(0), typedPtr.m_pObject, typedPtr.m_pType);
        return ExecResult::RunNext(0);
      }

      inout_context.SetPointerData<void*>(node.GetOutputDataOffset(0), nullptr, nullptr);
      return ExecResult::RunNext(1);
    }
    else if constexpr (std::is_same_v<T, plVariant>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), v);
      return ExecResult::RunNext(0);
    }
    else
    {
      plResult conversionResult = PLASMA_SUCCESS;
      inout_context.SetData(node.GetOutputDataOffset(0), v.ConvertTo<T>(&conversionResult));
      return ExecResult::RunNext(conversionResult.Succeeded() ? 0 : 1);
    }
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Variant_ConvertTo);

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_MakeArray(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    plVariantArray& a = inout_context.GetWritableData<plVariantArray>(node.GetOutputDataOffset(0));
    a.Clear();
    a.Reserve(node.m_NumInputDataOffsets);

    for (plUInt32 i = 0; i < node.m_NumInputDataOffsets; ++i)
    {
      auto dataOffset = node.GetInputDataOffset(i);

      if (dataOffset.IsConstant())
      {
        a.PushBack(inout_context.GetDataAsVariant(dataOffset, nullptr));
      }
      else
      {
        a.PushBack(inout_context.GetData<plVariant>(dataOffset));
      }
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_GetElement(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    const plVariantArray& a = inout_context.GetData<plVariantArray>(node.GetInputDataOffset(0));
    plUInt32 uiIndex = inout_context.GetData<int>(node.GetInputDataOffset(1));
    inout_context.SetData(node.GetOutputDataOffset(0), a[uiIndex]);

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_GetCount(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    const plVariantArray& a = inout_context.GetData<plVariantArray>(node.GetInputDataOffset(0));
    inout_context.SetData<int>(node.GetOutputDataOffset(0), a.GetCount());

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_TryGetComponentOfBaseType(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Type>();

    plTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
    if (p.m_pType != plGetStaticRTTI<plGameObject>())
    {
      plLog::Error("Visual script call TryGetComponentOfBaseType: Game object is not of type 'plGameObject'");
      return ExecResult::RunNext(0);
    }

    plComponent* pComponent = nullptr;
    static_cast<plGameObject*>(p.m_pObject)->TryGetComponentOfBaseType(userData.m_pType, pComponent);
    inout_context.SetPointerData(node.GetOutputDataOffset(0), pComponent);

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_StartCoroutine(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    auto& userData = node.GetUserData<NodeUserData_StartCoroutine>();
    plString sName = inout_context.GetData<plString>(node.GetInputDataOffset(0));

    plScriptCoroutine* pCoroutine = nullptr;
    auto hCoroutine = pModule->CreateCoroutine(userData.m_pType, sName, inout_context.GetInstance(), userData.m_CreationMode, pCoroutine);
    pModule->StartCoroutine(hCoroutine, plArrayPtr<plVariant>());

    inout_context.SetData(node.GetOutputDataOffset(0), hCoroutine);


    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_StopCoroutine(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    auto hCoroutine = inout_context.GetData<plScriptCoroutineHandle>(node.GetInputDataOffset(0));
    if (pModule->IsCoroutineFinished(hCoroutine) == false)
    {
      pModule->StopAndDeleteCoroutine(hCoroutine);
    }
    else
    {
      auto sName = inout_context.GetData<plString>(node.GetInputDataOffset(1));
      pModule->StopAndDeleteCoroutine(sName, &inout_context.GetInstance());
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_StopAllCoroutines(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    pModule->StopAndDeleteAllCoroutines(&inout_context.GetInstance());

    return ExecResult::RunNext(0);
  }

  template <bool bWaitForAll>
  static ExecResult NodeFunction_Builtin_WaitForX(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    const plUInt32 uiNumCoroutines = node.m_NumInputDataOffsets;
    plUInt32 uiNumFinishedCoroutines = 0;

    for (plUInt32 i = 0; i < uiNumCoroutines; ++i)
    {
      auto hCoroutine = inout_context.GetData<plScriptCoroutineHandle>(node.GetInputDataOffset(i));
      if (pModule->IsCoroutineFinished(hCoroutine))
      {
        if constexpr (bWaitForAll == false)
        {
          return ExecResult::RunNext(0);
        }
        else
        {
          ++uiNumFinishedCoroutines;
        }
      }
    }

    if constexpr (bWaitForAll)
    {
      if (uiNumFinishedCoroutines == uiNumCoroutines)
      {
        return ExecResult::RunNext(0);
      }
    }

    return ExecResult::ContinueLater(plTime::MakeZero());
  }

  static ExecResult NodeFunction_Builtin_Yield(plVisualScriptExecutionContext& inout_context, const plVisualScriptGraphDescription::Node& node)
  {
    plScriptCoroutine* pCoroutine = inout_context.GetCurrentCoroutine();
    if (pCoroutine == nullptr)
    {
      // set marker value of 0x1 to indicate we are in a yield
      inout_context.SetCurrentCoroutine(reinterpret_cast<plScriptCoroutine*>(0x1));

      return ExecResult::ContinueLater(plTime::MakeZero());
    }

    inout_context.SetCurrentCoroutine(nullptr);

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  struct ExecuteFunctionContext
  {
    plVisualScriptGraphDescription::ExecuteFunction m_Func = nullptr;
    ExecuteFunctionGetter m_FuncGetter = nullptr;
  };

  static ExecuteFunctionContext s_TypeToExecuteFunctions[] = {
    {},                                                   // Invalid,
    {},                                                   // EntryCall,
    {},                                                   // EntryCall_Coroutine,
    {},                                                   // MessageHandler,
    {},                                                   // MessageHandler_Coroutine,
    {&NodeFunction_ReflectedFunction},                    // ReflectedFunction,
    {nullptr, &NodeFunction_GetReflectedProperty_Getter}, // GetReflectedProperty,
    {nullptr, &NodeFunction_SetReflectedProperty_Getter}, // SetReflectedProperty,
    {&NodeFunction_InplaceCoroutine},                     // InplaceCoroutine,
    {&NodeFunction_GetScriptOwner},                       // GetScriptOwner,
    {&NodeFunction_SendMessage},                          // SendMessage,

    {}, // FirstBuiltin,

    {},                                                  // Builtin_Constant,
    {},                                                  // Builtin_GetVariable,
    {nullptr, &NodeFunction_Builtin_SetVariable_Getter}, // Builtin_SetVariable,
    {nullptr, &NodeFunction_Builtin_IncVariable_Getter}, // Builtin_IncVariable,
    {nullptr, &NodeFunction_Builtin_DecVariable_Getter}, // Builtin_DecVariable,

    {&NodeFunction_Builtin_Branch},                 // Builtin_Branch,
    {nullptr, &NodeFunction_Builtin_Switch_Getter}, // Builtin_Switch,
    {},                                             // Builtin_WhileLoop,
    {},                                             // Builtin_ForLoop,
    {},                                             // Builtin_ForEachLoop,
    {},                                             // Builtin_ReverseForEachLoop,
    {},                                             // Builtin_Break,
    {},                                             // Builtin_Jump,

    {&NodeFunction_Builtin_And},                     // Builtin_And,
    {&NodeFunction_Builtin_Or},                      // Builtin_Or,
    {&NodeFunction_Builtin_Not},                     // Builtin_Not,
    {nullptr, &NodeFunction_Builtin_Compare_Getter}, // Builtin_Compare,
    {}, // Builtin_CompareExec,
    {nullptr, &NodeFunction_Builtin_IsValid_Getter}, // Builtin_IsValid,
    {},                                              // Builtin_Select,

    {nullptr, &NodeFunction_Builtin_Add_Getter}, // Builtin_Add,
    {nullptr, &NodeFunction_Builtin_Sub_Getter}, // Builtin_Subtract,
    {nullptr, &NodeFunction_Builtin_Mul_Getter}, // Builtin_Multiply,
    {nullptr, &NodeFunction_Builtin_Div_Getter}, // Builtin_Divide,
    {}, // Builtin_Expression,

    {nullptr, &NodeFunction_Builtin_ToBool_Getter},            // Builtin_ToBool,
    {nullptr, &NodeFunction_Builtin_ToByte_Getter},            // Builtin_ToByte,
    {nullptr, &NodeFunction_Builtin_ToInt_Getter},             // Builtin_ToInt,
    {nullptr, &NodeFunction_Builtin_ToInt64_Getter},           // Builtin_ToInt64,
    {nullptr, &NodeFunction_Builtin_ToFloat_Getter},           // Builtin_ToFloat,
    {nullptr, &NodeFunction_Builtin_ToDouble_Getter},          // Builtin_ToDouble,
    {nullptr, &NodeFunction_Builtin_ToString_Getter},          // Builtin_ToString,
    {&NodeFunction_Builtin_String_Format},                     // Builtin_String_Format,
    {nullptr, &NodeFunction_Builtin_ToHashedString_Getter},    // Builtin_ToHashedString,
    {nullptr, &NodeFunction_Builtin_ToVariant_Getter},         // Builtin_ToVariant,
    {nullptr, &NodeFunction_Builtin_Variant_ConvertTo_Getter}, // Builtin_Variant_ConvertTo,

    {&NodeFunction_Builtin_MakeArray},        // Builtin_MakeArray
    {&NodeFunction_Builtin_Array_GetElement}, // Builtin_Array_GetElement,
    {},                                       // Builtin_Array_SetElement,
    {&NodeFunction_Builtin_Array_GetCount},   // Builtin_Array_GetCount,
    {},                                       // Builtin_Array_IsEmpty,
    {},                                       // Builtin_Array_Clear,
    {},                                       // Builtin_Array_Contains,
    {},                                       // Builtin_Array_IndexOf,
    {},                                       // Builtin_Array_Insert,
    {},                                       // Builtin_Array_PushBack,
    {},                                       // Builtin_Array_Remove,
    {},                                       // Builtin_Array_RemoveAt,

    {&NodeFunction_Builtin_TryGetComponentOfBaseType}, // Builtin_TryGetComponentOfBaseType

    {&NodeFunction_Builtin_StartCoroutine},    // Builtin_StartCoroutine,
    {&NodeFunction_Builtin_StopCoroutine},     // Builtin_StopCoroutine,
    {&NodeFunction_Builtin_StopAllCoroutines}, // Builtin_StopAllCoroutines,
    {&NodeFunction_Builtin_WaitForX<true>},    // Builtin_WaitForAll,
    {&NodeFunction_Builtin_WaitForX<false>},   // Builtin_WaitForAny,
    {&NodeFunction_Builtin_Yield},             // Builtin_Yield,

    {}, // LastBuiltin,
  };

  static_assert(PLASMA_ARRAY_SIZE(s_TypeToExecuteFunctions) == plVisualScriptNodeDescription::Type::Count);
} // namespace

plVisualScriptGraphDescription::ExecuteFunction GetExecuteFunction(plVisualScriptNodeDescription::Type::Enum nodeType, plVisualScriptDataType::Enum dataType)
{
  PLASMA_ASSERT_DEBUG(nodeType >= 0 && nodeType < PLASMA_ARRAY_SIZE(s_TypeToExecuteFunctions), "Out of bounds access");
  auto& context = s_TypeToExecuteFunctions[nodeType];
  if (context.m_Func != nullptr)
  {
    return context.m_Func;
  }

  if (context.m_FuncGetter != nullptr)
  {
    return context.m_FuncGetter(dataType);
  }

  return nullptr;
}

#undef MAKE_EXEC_FUNC_GETTER
#undef MAKE_TONUMBER_EXEC_FUNC
