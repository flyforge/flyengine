#pragma once

#include <Foundation/Reflection/ReflectionUtils.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

using SerializeFunction = plResult (*)(const plVisualScriptNodeDescription& nodeDesc, plStreamWriter& inout_stream, plUInt32& out_Size, plUInt32& out_alignment);
using DeserializeFunction = plResult (*)(plVisualScriptGraphDescription::Node& node, plStreamReader& inout_stream, plUInt8*& inout_pAdditionalData);
using ToStringFunction = void (*)(const plVisualScriptNodeDescription& nodeDesc, plStringBuilder& out_sResult);

namespace
{
  template <typename T, typename U>
  static plUInt32 GetDynamicSize(plUInt32 uiCount)
  {
    plUInt32 uiSize = sizeof(T);
    if (uiCount > 1)
    {
      uiSize += sizeof(U) * (uiCount - 1);
    }
    return uiSize;
  }

  struct NodeUserData_Type
  {
    const plRTTI* m_pType = nullptr;

#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
    plUInt32 m_uiPadding;
#endif

    static plResult Serialize(const plVisualScriptNodeDescription& nodeDesc, plStreamWriter& inout_stream, plUInt32& out_uiSize, plUInt32& out_uiAlignment)
    {
      inout_stream << nodeDesc.m_sTargetTypeName;

      out_uiSize = sizeof(NodeUserData_Type);
      out_uiAlignment = PLASMA_ALIGNMENT_OF(NodeUserData_Type);
      return PLASMA_SUCCESS;
    }

    static plResult ReadType(plStreamReader& inout_stream, const plRTTI*& out_pType)
    {
      plStringBuilder sTypeName;
      inout_stream >> sTypeName;

      out_pType = plRTTI::FindTypeByName(sTypeName);
      if (out_pType == nullptr)
      {
        plLog::Error("Unknown type '{}'", sTypeName);
        return PLASMA_FAILURE;
      }

      return PLASMA_SUCCESS;
    }

    static plResult Deserialize(plVisualScriptGraphDescription::Node& ref_node, plStreamReader& inout_stream, plUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_Type>(inout_pAdditionalData);
      PLASMA_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));
      return PLASMA_SUCCESS;
    }

    static void ToString(const plVisualScriptNodeDescription& nodeDesc, plStringBuilder& out_sResult)
    {
      if (nodeDesc.m_sTargetTypeName.IsEmpty() == false)
      {
        out_sResult.Append(nodeDesc.m_sTargetTypeName);
      }
    }
  };

  static_assert(sizeof(NodeUserData_Type) == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_TypeAndProperty : public NodeUserData_Type
  {
    const plAbstractProperty* m_pProperty = nullptr;

#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
    plUInt32 m_uiPadding;
#endif

    static plResult Serialize(const plVisualScriptNodeDescription& nodeDesc, plStreamWriter& inout_stream, plUInt32& out_uiSize, plUInt32& out_uiAlignment)
    {
      PLASMA_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      const plVariantArray& propertiesVar = nodeDesc.m_Value.Get<plVariantArray>();
      PLASMA_ASSERT_DEBUG(propertiesVar.GetCount() == 1, "Invalid number of properties");

      inout_stream << propertiesVar[0].Get<plHashedString>();

      out_uiSize = sizeof(NodeUserData_TypeAndProperty);
      out_uiAlignment = PLASMA_ALIGNMENT_OF(NodeUserData_TypeAndProperty);
      return PLASMA_SUCCESS;
    }

    template <typename T>
    static plResult ReadProperty(plStreamReader& inout_stream, const plRTTI* pType, plArrayPtr<T> properties, const plAbstractProperty*& out_pProp)
    {
      plStringBuilder sPropName;
      inout_stream >> sPropName;

      out_pProp = nullptr;
      for (auto& pProp : properties)
      {
        if (sPropName == pProp->GetPropertyName())
        {
          out_pProp = pProp;
          break;
        }
      }

      if (out_pProp == nullptr)
      {
        constexpr bool isFunction = std::is_same_v<T, const plAbstractFunctionProperty* const>;
        plLog::Error("{} '{}' not found on type '{}'", isFunction ? "Function" : "Property", sPropName, pType->GetTypeName());
        return PLASMA_FAILURE;
      }

      return PLASMA_SUCCESS;
    }

    template <bool PropIsFunction>
    static plResult Deserialize(plVisualScriptGraphDescription::Node& ref_node, plStreamReader& inout_stream, plUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_TypeAndProperty>(inout_pAdditionalData);
      PLASMA_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));

      if constexpr (PropIsFunction)
      {
        PLASMA_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetFunctions(), userData.m_pProperty));
      }
      else
      {
        PLASMA_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetProperties(), userData.m_pProperty));
      }

      return PLASMA_SUCCESS;
    }

    static void ToString(const plVisualScriptNodeDescription& nodeDesc, plStringBuilder& out_sResult)
    {
      NodeUserData_Type::ToString(nodeDesc, out_sResult);

      if (nodeDesc.m_Value.IsA<plVariantArray>())
      {
        const plVariantArray& propertiesVar = nodeDesc.m_Value.Get<plVariantArray>();
        if (propertiesVar.IsEmpty() == false)
        {
          out_sResult.Append(".", propertiesVar[0].Get<plHashedString>());
        }
      }
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndProperty) == 16);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_TypeAndProperties : public NodeUserData_Type
  {
    plUInt32 m_uiNumProperties = 0;

#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
    plUInt32 m_uiPadding0;
#endif

    // This struct is allocated with enough space behind it to hold an array with m_uiNumProperties size.
    const plAbstractProperty* m_Properties[1];

#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
    plUInt32 m_uiPadding1;
#endif

    static plResult Serialize(const plVisualScriptNodeDescription& nodeDesc, plStreamWriter& inout_stream, plUInt32& out_uiSize, plUInt32& out_uiAlignment)
    {
      PLASMA_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      const plVariantArray& propertiesVar = nodeDesc.m_Value.Get<plVariantArray>();

      plUInt32 uiCount = propertiesVar.GetCount();
      inout_stream << uiCount;

      for (auto& var : propertiesVar)
      {
        plHashedString sPropName = var.Get<plHashedString>();
        inout_stream << sPropName;
      }

      static_assert(sizeof(void*) <= sizeof(plUInt64));
      out_uiSize = GetDynamicSize<NodeUserData_TypeAndProperties, plUInt64>(uiCount);
      out_uiAlignment = PLASMA_ALIGNMENT_OF(NodeUserData_TypeAndProperties);
      return PLASMA_SUCCESS;
    }

    static plResult Deserialize(plVisualScriptGraphDescription::Node& ref_node, plStreamReader& inout_stream, plUInt8*& inout_pAdditionalData)
    {
      const plRTTI* pType = nullptr;
      PLASMA_SUCCEED_OR_RETURN(ReadType(inout_stream, pType));

      plUInt32 uiCount = 0;
      inout_stream >> uiCount;

      const plUInt32 uiByteSize = GetDynamicSize<NodeUserData_TypeAndProperties, plUInt64>(uiCount);
      auto& userData = ref_node.InitUserData<NodeUserData_TypeAndProperties>(inout_pAdditionalData, uiByteSize);
      userData.m_pType = pType;
      userData.m_uiNumProperties = uiCount;

      plHybridArray<const plAbstractProperty*, 32> properties;
      userData.m_pType->GetAllProperties(properties);

      for (plUInt32 i = 0; i < uiCount; ++i)
      {
        const plAbstractProperty* pProperty = nullptr;
        PLASMA_SUCCEED_OR_RETURN(NodeUserData_TypeAndProperty::ReadProperty(inout_stream, userData.m_pType, properties.GetArrayPtr(), pProperty));
        userData.m_Properties[i] = pProperty;
      }

      return PLASMA_SUCCESS;
    }

    static void ToString(const plVisualScriptNodeDescription& nodeDesc, plStringBuilder& out_sResult)
    {
      NodeUserData_TypeAndProperty::ToString(nodeDesc, out_sResult);
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndProperties) == 24);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Switch
  {
    plUInt32 m_uiNumCases = 0;

    // This struct is allocated with enough space behind it to hold an array with m_uiNumCases size.
    plInt64 m_Cases[1];

    static plResult Serialize(const plVisualScriptNodeDescription& nodeDesc, plStreamWriter& inout_stream, plUInt32& out_uiSize, plUInt32& out_uiAlignment)
    {
      const plVariantArray& casesVar = nodeDesc.m_Value.Get<plVariantArray>();

      plUInt32 uiCount = casesVar.GetCount();
      inout_stream << uiCount;

      for (auto& var : casesVar)
      {
        plInt64 iCaseValue = var.ConvertTo<plInt64>();
        inout_stream << iCaseValue;
      }

      out_uiSize = GetDynamicSize<NodeUserData_Switch, plInt64>(uiCount);
      out_uiAlignment = PLASMA_ALIGNMENT_OF(NodeUserData_Switch);
      return PLASMA_SUCCESS;
    }

    static plResult Deserialize(plVisualScriptGraphDescription::Node& ref_node, plStreamReader& inout_stream, plUInt8*& inout_pAdditionalData)
    {
      plUInt32 uiCount = 0;
      inout_stream >> uiCount;

      const plUInt32 uiByteSize = GetDynamicSize<NodeUserData_Switch, plInt64>(uiCount);
      auto& userData = ref_node.InitUserData<NodeUserData_Switch>(inout_pAdditionalData, uiByteSize);
      userData.m_uiNumCases = uiCount;

      for (plUInt32 i = 0; i < uiCount; ++i)
      {
        inout_stream >> userData.m_Cases[i];
      }

      return PLASMA_SUCCESS;
    }

    static void ToString(const plVisualScriptNodeDescription& nodeDesc, plStringBuilder& out_sResult)
    {
      // Nothing to add here
    }
  };

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Comparison
  {
    plEnum<plComparisonOperator> m_ComparisonOperator;

    static plResult Serialize(const plVisualScriptNodeDescription& nodeDesc, plStreamWriter& inout_stream, plUInt32& out_uiSize, plUInt32& out_uiAlignment)
    {
      plEnum<plComparisonOperator> compOp = static_cast<plComparisonOperator::Enum>(nodeDesc.m_Value.Get<plInt64>());
      inout_stream << compOp;

      out_uiSize = sizeof(NodeUserData_Comparison);
      out_uiAlignment = PLASMA_ALIGNMENT_OF(NodeUserData_Comparison);
      return PLASMA_SUCCESS;
    }

    static plResult Deserialize(plVisualScriptGraphDescription::Node& ref_node, plStreamReader& inout_stream, plUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_Comparison>(inout_pAdditionalData);
      inout_stream >> userData.m_ComparisonOperator;

      return PLASMA_SUCCESS;
    }

    static void ToString(const plVisualScriptNodeDescription& nodeDesc, plStringBuilder& out_sResult)
    {
      plStringBuilder sCompOp;
      plReflectionUtils::EnumerationToString(plGetStaticRTTI<plComparisonOperator>(), nodeDesc.m_Value.Get<plInt64>(), sCompOp, plReflectionUtils::EnumConversionMode::ValueNameOnly);

      out_sResult.Append(" ", sCompOp);
    }
  };

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_StartCoroutine : public NodeUserData_Type
  {
    plEnum<plScriptCoroutineCreationMode> m_CreationMode;

    static plResult Serialize(const plVisualScriptNodeDescription& nodeDesc, plStreamWriter& inout_stream, plUInt32& out_uiSize, plUInt32& out_uiAlignment)
    {
      PLASMA_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      plEnum<plScriptCoroutineCreationMode> creationMode = static_cast<plScriptCoroutineCreationMode::Enum>(nodeDesc.m_Value.Get<plInt64>());
      inout_stream << creationMode;

      out_uiSize = sizeof(NodeUserData_StartCoroutine);
      out_uiAlignment = PLASMA_ALIGNMENT_OF(NodeUserData_StartCoroutine);
      return PLASMA_SUCCESS;
    }

    static plResult Deserialize(plVisualScriptGraphDescription::Node& ref_node, plStreamReader& inout_stream, plUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_StartCoroutine>(inout_pAdditionalData);
      PLASMA_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));

      inout_stream >> userData.m_CreationMode;

      return PLASMA_SUCCESS;
    }

    static void ToString(const plVisualScriptNodeDescription& nodeDesc, plStringBuilder& out_sResult)
    {
      NodeUserData_Type::ToString(nodeDesc, out_sResult);

      plStringBuilder sCreationMode;
      plReflectionUtils::EnumerationToString(plGetStaticRTTI<plScriptCoroutineCreationMode>(), nodeDesc.m_Value.Get<plInt64>(), sCreationMode, plReflectionUtils::EnumConversionMode::ValueNameOnly);

      out_sResult.Append(" ", sCreationMode);
    }
  };

  static_assert(sizeof(NodeUserData_StartCoroutine) == 16);

  //////////////////////////////////////////////////////////////////////////

  struct UserDataContext
  {
    SerializeFunction m_SerializeFunc = nullptr;
    DeserializeFunction m_DeserializeFunc = nullptr;
    ToStringFunction m_ToStringFunc = nullptr;
  };

   inline UserDataContext s_TypeToUserDataContexts[] =
   {
    {}, // Invalid,
    {}, // EntryCall,
    {}, // EntryCall_Coroutine,
    {&NodeUserData_TypeAndProperties::Serialize,
      &NodeUserData_TypeAndProperties::Deserialize,
      &NodeUserData_TypeAndProperties::ToString}, // MessageHandler,
    {&NodeUserData_TypeAndProperties::Serialize,
      &NodeUserData_TypeAndProperties::Deserialize,
      &NodeUserData_TypeAndProperties::ToString}, // MessageHandler_Coroutine,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize<true>,
      &NodeUserData_TypeAndProperty::ToString}, // ReflectedFunction,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize<false>,
      &NodeUserData_TypeAndProperty::ToString}, // GetReflectedProperty,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize<false>,
      &NodeUserData_TypeAndProperty::ToString}, // SetReflectedProperty,
    {&NodeUserData_TypeAndProperty::Serialize,
      &NodeUserData_TypeAndProperty::Deserialize<true>,
      &NodeUserData_TypeAndProperty::ToString}, // InplaceCoroutine,
    {},                                         // GetScriptOwner,
    {&NodeUserData_TypeAndProperties::Serialize,
      &NodeUserData_TypeAndProperties::Deserialize,
      &NodeUserData_TypeAndProperties::ToString}, // SendMessage,

    {}, // FirstBuiltin,

    {}, // Builtin_Constant,
    {}, // Builtin_GetVariable,
    {}, // Builtin_SetVariable,
    {}, // Builtin_IncVariable,
    {}, // Builtin_DecVariable,

    {}, // Builtin_Branch,
    {&NodeUserData_Switch::Serialize,
      &NodeUserData_Switch::Deserialize,
      &NodeUserData_Switch::ToString}, // Builtin_Switch,
    {},                                // Builtin_WhileLoop,
    {},                                // Builtin_ForLoop,
    {},                                // Builtin_ForEachLoop,
    {},                                // Builtin_ReverseForEachLoop,
    {},                                // Builtin_Break,
    {},                                // Builtin_Jump,

    {}, // Builtin_And,
    {}, // Builtin_Or,
    {}, // Builtin_Not,
    {&NodeUserData_Comparison::Serialize,
      &NodeUserData_Comparison::Deserialize,
      &NodeUserData_Comparison::ToString}, // Builtin_Compare,
    {},                                    // Builtin_CompareExec,
    {},                                    // Builtin_IsValid,
    {},                                    // Builtin_Select,

    {}, // Builtin_Add,
    {}, // Builtin_Subtract,
    {}, // Builtin_Multiply,
    {}, // Builtin_Divide,
    {}, // Builtin_Expression,

    {}, // Builtin_ToBool,
    {}, // Builtin_ToByte,
    {}, // Builtin_ToInt,
    {}, // Builtin_ToInt64,
    {}, // Builtin_ToFloat,
    {}, // Builtin_ToDouble,
    {}, // Builtin_ToString,
    {}, // Builtin_String_Format,
    {}, // Builtin_ToHashedString,
    {}, // Builtin_ToVariant,
    {}, // Builtin_Variant_ConvertTo,

    {}, // Builtin_MakeArray
    {}, // Builtin_Array_GetElement,
    {}, // Builtin_Array_SetElement,
    {}, // Builtin_Array_GetCount,
    {}, // Builtin_Array_IsEmpty,
    {}, // Builtin_Array_Clear,
    {}, // Builtin_Array_Contains,
    {}, // Builtin_Array_IndexOf,
    {}, // Builtin_Array_Insert,
    {}, // Builtin_Array_PushBack,
    {}, // Builtin_Array_Remove,
    {}, // Builtin_Array_RemoveAt,

    {&NodeUserData_Type::Serialize,
      &NodeUserData_Type::Deserialize,
      &NodeUserData_Type::ToString}, // Builtin_TryGetComponentOfBaseType

    {&NodeUserData_StartCoroutine::Serialize,
      &NodeUserData_StartCoroutine::Deserialize,
      &NodeUserData_StartCoroutine::ToString}, // Builtin_StartCoroutine,
    {},                                        // Builtin_StopCoroutine,
    {},                                        // Builtin_StopAllCoroutines,
    {},                                        // Builtin_WaitForAll,
    {},                                        // Builtin_WaitForAny,
    {},                                        // Builtin_Yield,

    {}, // LastBuiltin,
  };

  static_assert(PLASMA_ARRAY_SIZE(s_TypeToUserDataContexts) == plVisualScriptNodeDescription::Type::Count);
} // namespace

const UserDataContext& GetUserDataContext(plVisualScriptNodeDescription::Type::Enum nodeType)
{
  PLASMA_ASSERT_DEBUG(nodeType >= 0 && nodeType < PLASMA_ARRAY_SIZE(s_TypeToUserDataContexts), "Out of bounds access");
  return s_TypeToUserDataContexts[nodeType];
}
