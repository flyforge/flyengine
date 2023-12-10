#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <Core/Messages/EventMessage.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

namespace
{
  constexpr const char* szPluginName = "EditorPluginVisualScript";
  constexpr const char* szEventHandlerCategory = "Add Event Handler/";
  constexpr const char* szCoroutinesCategory = "Coroutines";
  constexpr const char* szEnumsCategory = "Enums";

  const plRTTI* FindTopMostBaseClass(const plRTTI* pRtti)
  {
    const plRTTI* pReflectedClass = plGetStaticRTTI<plReflectedClass>();
    while (pRtti->GetParentType() != nullptr && pRtti->GetParentType() != pReflectedClass)
    {
      pRtti = pRtti->GetParentType();
    }
    return pRtti;
  }

  void CollectFunctionArgumentAttributes(const plAbstractFunctionProperty* pFuncProp, plDynamicArray<const plFunctionArgumentAttributes*>& out_attributes)
  {
    for (auto pAttr : pFuncProp->GetAttributes())
    {
      if (auto pFuncArgAttr = plDynamicCast<const plFunctionArgumentAttributes*>(pAttr))
      {
        plUInt32 uiArgIndex = pFuncArgAttr->GetArgumentIndex();
        out_attributes.EnsureCount(uiArgIndex + 1);
        out_attributes[uiArgIndex] = pFuncArgAttr;
      }
    }
  }

  void AddInputProperty(plReflectedTypeDescriptor& ref_typeDesc, plStringView sName, const plRTTI* pRtti, plVisualScriptDataType::Enum scriptDataType, plArrayPtr<const plPropertyAttribute* const> attributes = {})
  {
    auto& propDesc = ref_typeDesc.m_Properties.ExpandAndGetRef();
    propDesc.m_sName = sName;
    propDesc.m_Flags = plPropertyFlags::StandardType;

    for (auto pAttr : attributes)
    {
      propDesc.m_Attributes.PushBack(pAttr->GetDynamicRTTI()->GetAllocator()->Clone<plPropertyAttribute>(pAttr));
    }

    if (pRtti->GetTypeFlags().IsSet(plTypeFlags::IsEnum))
    {
      propDesc.m_Category = plPropertyCategory::Member;
      propDesc.m_sType = pRtti->GetTypeName();
      propDesc.m_Flags = plPropertyFlags::IsEnum;
    }
    else
    {
      if (scriptDataType == plVisualScriptDataType::Variant)
      {
        propDesc.m_Category = plPropertyCategory::Member;
        propDesc.m_sType = plGetStaticRTTI<plVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(PLASMA_DEFAULT_NEW(plVisualScriptVariableAttribute));
      }
      else if (scriptDataType == plVisualScriptDataType::Array)
      {
        propDesc.m_Category = plPropertyCategory::Array;
        propDesc.m_sType = plGetStaticRTTI<plVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(PLASMA_DEFAULT_NEW(plVisualScriptVariableAttribute));
      }
      else if (scriptDataType == plVisualScriptDataType::Map)
      {
        propDesc.m_Category = plPropertyCategory::Map;
        propDesc.m_sType = plGetStaticRTTI<plVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(PLASMA_DEFAULT_NEW(plVisualScriptVariableAttribute));
      }
      else
      {
        propDesc.m_Category = plPropertyCategory::Member;
        propDesc.m_sType = plVisualScriptDataType::GetRtti(scriptDataType)->GetTypeName();
      }
    }
  }

  plStringView StripTypeName(plStringView sTypeName)
  {
    sTypeName.TrimWordStart("pl");
    return sTypeName;
  }

  plStringView GetTypeName(const plRTTI* pRtti)
  {
    plStringView sTypeName = pRtti->GetTypeName();
    if (auto pScriptExtension = pRtti->GetAttributeByType<plScriptExtensionAttribute>())
    {
      sTypeName = pScriptExtension->GetTypeName();
    }
    return StripTypeName(sTypeName);
  }

  plColorGammaUB NiceColorFromName(plStringView sTypeName, plStringView sCategory = plStringView())
  {
    float typeX = plSimdRandom::FloatZeroToOne(plSimdVec4i(plHashingUtils::StringHash(sTypeName))).x();

    float x = typeX;
    if (sCategory.IsEmpty() == false)
    {
      x = plSimdRandom::FloatZeroToOne(plSimdVec4i(plHashingUtils::StringHash(sCategory))).x();
      x += typeX * plColorScheme::s_fIndexNormalizer;
    }

    return plColorScheme::DarkUI(x);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

static plColorScheme::Enum s_scriptDataTypeToPinColor[] = {
  plColorScheme::Gray,   // Invalid
  plColorScheme::Red,    // Bool,
  plColorScheme::Cyan,   // Byte,
  plColorScheme::Teal,   // Int,
  plColorScheme::Teal,   // Int64,
  plColorScheme::Green,  // Float,
  plColorScheme::Green,  // Double,
  plColorScheme::Lime,   // Color,
  plColorScheme::Orange, // Vector3,
  plColorScheme::Orange, // Quaternion,
  plColorScheme::Orange, // Transform,
  plColorScheme::Violet, // Time,
  plColorScheme::Green,  // Angle,
  plColorScheme::Grape,  // String,
  plColorScheme::Grape,  // HashedString,
  plColorScheme::Blue,   // GameObject,
  plColorScheme::Blue,   // Component,
  plColorScheme::Blue,   // TypedPointer,
  plColorScheme::Pink,   // Variant,
  plColorScheme::Pink,   // VariantArray,
  plColorScheme::Pink,   // VariantDictionary,
  plColorScheme::Cyan,   // Coroutine,
};

static_assert(PLASMA_ARRAY_SIZE(s_scriptDataTypeToPinColor) == plVisualScriptDataType::Count);

// static
plColor plVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(plVisualScriptDataType::Enum dataType)
{
  if (dataType == plVisualScriptDataType::EnumValue)
  {
    return plColorScheme::DarkUI(plColorScheme::Teal);
  }

  PLASMA_ASSERT_DEBUG(dataType >= 0 && dataType < PLASMA_ARRAY_SIZE(s_scriptDataTypeToPinColor), "Out of bounds access");
  return plColorScheme::DarkUI(s_scriptDataTypeToPinColor[dataType]);
}

plColor plVisualScriptNodeRegistry::PinDesc::GetColor() const
{
  if (IsExecutionPin())
  {
    return plColorScheme::DarkUI(plColorScheme::Gray);
  }

  if (m_ScriptDataType > plVisualScriptDataType::Invalid && m_ScriptDataType < plVisualScriptDataType::Count)
  {
    return GetColorForScriptDataType(m_ScriptDataType);
  }

  if (m_ScriptDataType == plVisualScriptDataType::EnumValue)
  {
    return plColorScheme::DarkUI(plColorScheme::Teal);
  }

  if (m_ScriptDataType == plVisualScriptDataType::Any)
  {
    return plColorScheme::DarkUI(plColorScheme::Gray);
  }

  return plColorScheme::DarkUI(plColorScheme::Blue);
}

//////////////////////////////////////////////////////////////////////////

void AddExecutionPin(plVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, plStringView sName, plHashedString sDynamicPinProperty, bool bSplitExecution, plSmallArray<plVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
{
  auto& pin = inout_pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_pDataType = nullptr;
  pin.m_ScriptDataType = plVisualScriptDataType::Invalid;
  pin.m_bSplitExecution = bSplitExecution;

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void plVisualScriptNodeRegistry::NodeDesc::AddInputExecutionPin(plStringView sName, const plHashedString& sDynamicPinProperty /*= plHashedString()*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, false, m_InputPins);

  m_bImplicitExecution = false;
}

void plVisualScriptNodeRegistry::NodeDesc::AddOutputExecutionPin(plStringView sName, const plHashedString& sDynamicPinProperty /*= plHashedString()*/, bool bSplitExecution /*= false*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, bSplitExecution, m_OutputPins);

  m_bImplicitExecution = false;
}

void AddDataPin(plVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, plStringView sName, const plRTTI* pDataType, plVisualScriptDataType::Enum scriptDataType, bool bRequired, plHashedString sDynamicPinProperty, plVisualScriptNodeRegistry::PinDesc::DeductTypeFunc deductTypeFunc, plSmallArray<plVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
{
  if ((scriptDataType == plVisualScriptDataType::AnyPointer || scriptDataType == plVisualScriptDataType::Any) && deductTypeFunc == nullptr)
  {
    deductTypeFunc = &plVisualScriptTypeDeduction::DeductFromNodeDataType;
  }

  auto& pin = inout_pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_DeductTypeFunc = deductTypeFunc;
  pin.m_pDataType = pDataType;
  pin.m_ScriptDataType = scriptDataType;
  pin.m_bRequired = bRequired;

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void plVisualScriptNodeRegistry::NodeDesc::AddInputDataPin(plStringView sName, const plRTTI* pDataType, plVisualScriptDataType::Enum scriptDataType, bool bRequired, const plHashedString& sDynamicPinProperty /*= plHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, bRequired, sDynamicPinProperty, deductTypeFunc, m_InputPins);
}

void plVisualScriptNodeRegistry::NodeDesc::AddOutputDataPin(plStringView sName, const plRTTI* pDataType, plVisualScriptDataType::Enum scriptDataType, const plHashedString& sDynamicPinProperty /*= plHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, false, sDynamicPinProperty, deductTypeFunc, m_OutputPins);
}

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_SINGLETON(plVisualScriptNodeRegistry);

plVisualScriptNodeRegistry::plVisualScriptNodeRegistry()
  : m_SingletonRegistrar(this)
{
  plPhantomRttiManager::s_Events.AddEventHandler(plMakeDelegate(&plVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler, this));

  UpdateNodeTypes();
}

plVisualScriptNodeRegistry::~plVisualScriptNodeRegistry()
{
  plPhantomRttiManager::s_Events.RemoveEventHandler(plMakeDelegate(&plVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler, this));
}

void plVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler(const plPhantomRttiManagerEvent& e)
{
  if (e.m_pChangedType->GetPluginName() == "EditorPluginVisualScript")
    return;

  if ((e.m_Type == plPhantomRttiManagerEvent::Type::TypeAdded && m_TypeToNodeDescs.Contains(e.m_pChangedType) == false) ||
      e.m_Type == plPhantomRttiManagerEvent::Type::TypeChanged)
  {
    UpdateNodeType(e.m_pChangedType);
  }
}

void plVisualScriptNodeRegistry::UpdateNodeTypes()
{
  PLASMA_PROFILE_SCOPE("Update VS Node Types");

  // Base Node Type
  if (m_pBaseType == nullptr)
  {
    plReflectedTypeDescriptor desc;
    desc.m_sTypeName = "plVisualScriptNodeBase";
    desc.m_sPluginName = szPluginName;
    desc.m_sParentTypeName = plGetStaticRTTI<plReflectedClass>()->GetTypeName();
    desc.m_Flags = plTypeFlags::Phantom | plTypeFlags::Abstract | plTypeFlags::Class;

    m_pBaseType = plPhantomRttiManager::RegisterType(desc);
  }

  if (m_bBuiltinTypesCreated == false)
  {
    CreateBuiltinTypes();
    m_bBuiltinTypesCreated = true;
  }

  auto& componentTypesDynEnum = plDynamicStringEnum::CreateDynamicEnum("ComponentTypes");
  auto& scriptBaseClassesDynEnum = plDynamicStringEnum::CreateDynamicEnum("ScriptBaseClasses");

  plRTTI::ForEachType([this](const plRTTI* pRtti) { UpdateNodeType(pRtti); });
}

void plVisualScriptNodeRegistry::UpdateNodeType(const plRTTI* pRtti)
{
  if (pRtti->GetAttributeByType<plHiddenAttribute>() != nullptr || pRtti->GetAttributeByType<plExcludeFromScript>() != nullptr)
    return;

  if (pRtti->IsDerivedFrom<plComponent>())
  {
    auto& componentTypesDynEnum = plDynamicStringEnum::GetDynamicEnum("ComponentTypes");
    componentTypesDynEnum.AddValidValue(pRtti->GetTypeName(), true);
  }

  if (pRtti->IsDerivedFrom<plScriptCoroutine>())
  {
    CreateCoroutineNodeType(pRtti);
  }
  else if (pRtti->IsDerivedFrom<plMessage>())
  {
    CreateMessageNodeTypes(pRtti);
  }
  else
  {
    // expose reflected functions and properties to visual scripts
    {
      bool bExposeToVisualScript = false;
      bool bHasBaseClassFunctions = false;

      for (const plAbstractFunctionProperty* pFuncProp : pRtti->GetFunctions())
      {
        auto pScriptableFunctionAttribute = pFuncProp->GetAttributeByType<plScriptableFunctionAttribute>();
        if (pScriptableFunctionAttribute == nullptr)
          continue;

        bExposeToVisualScript = true;

        bool bIsBaseClassFunction = pFuncProp->GetAttributeByType<plScriptBaseClassFunctionAttribute>() != nullptr;
        if (bIsBaseClassFunction)
        {
          bHasBaseClassFunctions = true;
        }

        CreateFunctionCallNodeType(pRtti, pFuncProp, pScriptableFunctionAttribute, bIsBaseClassFunction);
      }

      if (bExposeToVisualScript)
      {
        for (const plAbstractProperty* pProp : pRtti->GetProperties())
        {
        }
      }

      if (bHasBaseClassFunctions)
      {
        auto& scriptBaseClassesDynEnum = plDynamicStringEnum::GetDynamicEnum("ScriptBaseClasses");
        scriptBaseClassesDynEnum.AddValidValue(StripTypeName(pRtti->GetTypeName()));

        CreateGetOwnerNodeType(pRtti);
      }
    }
  }
}

plResult plVisualScriptNodeRegistry::GetScriptDataType(const plRTTI* pRtti, plVisualScriptDataType::Enum& out_scriptDataType, plStringView sFunctionName /*= plStringView()*/, plStringView sArgName /*= plStringView()*/)
{
  if (pRtti->GetTypeFlags().IsSet(plTypeFlags::IsEnum))
  {
    CreateEnumNodeTypes(pRtti);
  }

  plVisualScriptDataType::Enum scriptDataType = plVisualScriptDataType::FromRtti(pRtti);
  if (scriptDataType == plVisualScriptDataType::Invalid)
  {
    plLog::Warning("The script function '{}' uses an argument '{}' of type '{}' which is not a valid script data type, therefore this function will not be available in visual scripts", sFunctionName, sArgName, pRtti->GetTypeName());
    return PLASMA_FAILURE;
  }

  out_scriptDataType = scriptDataType;
  return PLASMA_SUCCESS;
}

plVisualScriptDataType::Enum plVisualScriptNodeRegistry::GetScriptDataType(const plAbstractProperty* pProp)
{
  if (pProp->GetCategory() == plPropertyCategory::Member)
  {
    plVisualScriptDataType::Enum result = plVisualScriptDataType::Invalid;
    GetScriptDataType(pProp->GetSpecificType(), result).IgnoreResult();
    return result;
  }
  else if (pProp->GetCategory() == plPropertyCategory::Array)
  {
    return plVisualScriptDataType::Array;
  }
  else if (pProp->GetCategory() == plPropertyCategory::Map)
  {
    return plVisualScriptDataType::Map;
  }

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return plVisualScriptDataType::Invalid;
}

template <typename T>
void plVisualScriptNodeRegistry::AddInputDataPin(plReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, plStringView sName)
{
  const plRTTI* pDataType = plGetStaticRTTI<T>();

  plVisualScriptDataType::Enum scriptDataType;
  PLASMA_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", sName).Succeeded(), "Invalid script data type");

  AddInputProperty(ref_typeDesc, sName, pDataType, scriptDataType);

  ref_nodeDesc.AddInputDataPin(sName, pDataType, scriptDataType, false);
};

void plVisualScriptNodeRegistry::AddInputDataPin_Any(plReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, plStringView sName, bool bRequired, bool bAddVariantProperty /*= false*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  if (bAddVariantProperty)
  {
    AddInputProperty(ref_typeDesc, sName, plGetStaticRTTI<plVariant>(), plVisualScriptDataType::Variant);
  }

  ref_nodeDesc.AddInputDataPin(sName, nullptr, plVisualScriptDataType::Any, bRequired, plHashedString(), deductTypeFunc);
}

template <typename T>
void plVisualScriptNodeRegistry::AddOutputDataPin(NodeDesc& ref_nodeDesc, plStringView sName)
{
  const plRTTI* pDataType = plGetStaticRTTI<T>();

  plVisualScriptDataType::Enum scriptDataType;
  PLASMA_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", sName).Succeeded(), "Invalid script data type");

  ref_nodeDesc.AddOutputDataPin(sName, pDataType, scriptDataType);
};

void plVisualScriptNodeRegistry::CreateBuiltinTypes()
{
  const plColorGammaUB logicColor = PinDesc::GetColorForScriptDataType(plVisualScriptDataType::Invalid);
  const plColorGammaUB mathColor = PinDesc::GetColorForScriptDataType(plVisualScriptDataType::Int);
  const plColorGammaUB stringColor = PinDesc::GetColorForScriptDataType(plVisualScriptDataType::String);
  const plColorGammaUB gameObjectColor = PinDesc::GetColorForScriptDataType(plVisualScriptDataType::GameObject);
  const plColorGammaUB variantColor = PinDesc::GetColorForScriptDataType(plVisualScriptDataType::Variant);
  const plColorGammaUB coroutineColor = PinDesc::GetColorForScriptDataType(plVisualScriptDataType::Coroutine);

  // GetReflectedProperty
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "GetProperty", "Properties", logicColor);

    AddInputProperty(typeDesc, "Type", plGetStaticRTTI<plString>(), plVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", plGetStaticRTTI<plString>(), plVisualScriptDataType::String);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "{Type}::Get {Property}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::GetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &plVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputDataPin("Object", nullptr, plVisualScriptDataType::AnyPointer, true, plHashedString(), &plVisualScriptTypeDeduction::DeductFromTypeProperty);
    nodeDesc.AddOutputDataPin("Value", nullptr, plVisualScriptDataType::Any);

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // SetReflectedProperty
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "SetProperty", "Properties", logicColor);

    AddInputProperty(typeDesc, "Type", plGetStaticRTTI<plString>(), plVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", plGetStaticRTTI<plString>(), plVisualScriptDataType::String);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "{Type}::Set {Property} = {Value}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::SetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &plVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Object", nullptr, plVisualScriptDataType::AnyPointer, true, plHashedString(), &plVisualScriptTypeDeduction::DeductFromTypeProperty);
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_GetVariable
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_GetVariable", "Variables", logicColor);

    AddInputProperty(typeDesc, "Name", plGetStaticRTTI<plString>(), plVisualScriptDataType::String);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "Get {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_GetVariable;
    nodeDesc.m_DeductTypeFunc = &plVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddOutputDataPin("Value", nullptr, plVisualScriptDataType::Any);

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_SetVariable
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_SetVariable", "Variables", logicColor);

    AddInputProperty(typeDesc, "Name", plGetStaticRTTI<plString>(), plVisualScriptDataType::String);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "Set {Name} = {Value}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_SetVariable;
    nodeDesc.m_DeductTypeFunc = &plVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_IncVariable, Builtin_DecVariable
  {
    plVisualScriptNodeDescription::Type::Enum nodeTypes[] = {
      plVisualScriptNodeDescription::Type::Builtin_IncVariable,
      plVisualScriptNodeDescription::Type::Builtin_DecVariable,
    };

    const char* szNodeTitles[] = {
      "++ {Name}",
      "-- {Name}",
    };

    static_assert(PLASMA_ARRAY_SIZE(nodeTypes) == PLASMA_ARRAY_SIZE(szNodeTitles));

    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(nodeTypes); ++i)
    {
      plReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, plVisualScriptNodeDescription::Type::GetName(nodeTypes[i]), "Variables", logicColor);

      AddInputProperty(typeDesc, "Name", plGetStaticRTTI<plString>(), plVisualScriptDataType::String);

      auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, szNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = nodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &plVisualScriptTypeDeduction::DeductFromVariableNameProperty;
      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("");
      nodeDesc.AddOutputDataPin("Value", nullptr, plVisualScriptDataType::Any);

      m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_Branch
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Branch", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_Branch;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("True");
    nodeDesc.AddOutputExecutionPin("False");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Switch
  {
    plVisualScriptDataType::Enum switchDataTypes[] = {
      plVisualScriptDataType::Int64,
      plVisualScriptDataType::HashedString,
    };

    const char* szSwitchTypeNames[] = {
      "Builtin_SwitchInt64",
      "Builtin_SwitchString",
    };

    const char* szSwitchTitles[] = {
      "Int64::Switch",
      "HashedString::Switch",
    };

    static_assert(PLASMA_ARRAY_SIZE(switchDataTypes) == PLASMA_ARRAY_SIZE(szSwitchTypeNames));
    static_assert(PLASMA_ARRAY_SIZE(switchDataTypes) == PLASMA_ARRAY_SIZE(szSwitchTitles));

    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(switchDataTypes); ++i)
    {
      const plRTTI* pValueType = plVisualScriptDataType::GetRtti(switchDataTypes[i]);

      plReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, szSwitchTypeNames[i], "Logic", logicColor);

      {
        auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
        propDesc.m_Category = plPropertyCategory::Array;
        propDesc.m_sName = "Cases";
        propDesc.m_sType = pValueType->GetTypeName();
        propDesc.m_Flags = plPropertyFlags::StandardType;

        auto pMaxSizeAttr = PLASMA_DEFAULT_NEW(plMaxArraySizeAttribute, 16);
        propDesc.m_Attributes.PushBack(pMaxSizeAttr);

        auto pNoTempAttr = PLASMA_DEFAULT_NEW(plNoTemporaryTransactionsAttribute);
        propDesc.m_Attributes.PushBack(pNoTempAttr);
      }

      auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, szSwitchTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_Switch;
      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("Case", plMakeHashedString("Cases"));
      nodeDesc.AddOutputExecutionPin("Default");

      nodeDesc.AddInputDataPin("Value", pValueType, switchDataTypes[i], true);

      m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

    // Builtin_WhileLoop
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_WhileLoop", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_WhileLoop;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("LoopBody");
    nodeDesc.AddOutputExecutionPin("Completed");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_ForLoop
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_ForLoop", "Logic", logicColor);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "ForLoop [{FirstIndex}..{LastIndex}]");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_ForLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<int>(typeDesc, nodeDesc, "FirstIndex");
    AddInputDataPin<int>(typeDesc, nodeDesc, "LastIndex");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_ForEachLoop
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_ForEachLoop", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_ForEachLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<plVariantArray>(typeDesc, nodeDesc, "Array");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<plVariant>(nodeDesc, "Element");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_ReverseForEachLoop
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_ReverseForEachLoop", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<plVariantArray>(typeDesc, nodeDesc, "Array");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<plVariant>(nodeDesc, "Element");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Break
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Break", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_Break;
    nodeDesc.AddInputExecutionPin("");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_And
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_And", "Logic", logicColor);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "{A} AND {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_And;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Or
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Or", "Logic", logicColor);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "{A} OR {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_Or;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Not
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Not", "Logic", logicColor);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "NOT {A}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_Not;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Compare
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Compare", "Logic", logicColor);

    AddInputProperty(typeDesc, "Operator", plGetStaticRTTI<plComparisonOperator>(), plVisualScriptDataType::Int64);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_Compare;
    nodeDesc.m_DeductTypeFunc = &plVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_IsValid
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_IsValid", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_IsValid;
    nodeDesc.m_DeductTypeFunc = &plVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Add, Builtin_Sub, Builtin_Mul, Builtin_Div
  {
    plVisualScriptNodeDescription::Type::Enum mathNodeTypes[] = {
      plVisualScriptNodeDescription::Type::Builtin_Add,
      plVisualScriptNodeDescription::Type::Builtin_Subtract,
      plVisualScriptNodeDescription::Type::Builtin_Multiply,
      plVisualScriptNodeDescription::Type::Builtin_Divide,
    };

    const char* szMathNodeTitles[] = {
      "{A} + {B}",
      "{A} - {B}",
      "{A} * {B}",
      "{A} / {B}",
    };

    static_assert(PLASMA_ARRAY_SIZE(mathNodeTypes) == PLASMA_ARRAY_SIZE(szMathNodeTitles));

    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(mathNodeTypes); ++i)
    {
      plReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, plVisualScriptNodeDescription::Type::GetName(mathNodeTypes[i]), "Math", mathColor);

      auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, szMathNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = mathNodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &plVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
      AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
      nodeDesc.AddOutputDataPin("", nullptr, plVisualScriptDataType::Any);

      m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_ToBool, Builtin_ToByte, Builtin_ToInt, Builtin_ToInt64, Builtin_ToFloat, Builtin_ToDouble, Builtin_ToString, Builtin_ToVariant,
  {
    struct ConversionNodeDesc
    {
      const char* m_szCategory;
      plColorGammaUB m_Color;
      plVisualScriptDataType::Enum m_DataType;
    };

    ConversionNodeDesc conversionNodeDescs[] = {
      {"Logic", logicColor, plVisualScriptDataType::Bool},
      {"Math", mathColor, plVisualScriptDataType::Byte},
      {"Math", mathColor, plVisualScriptDataType::Int},
      {"Math", mathColor, plVisualScriptDataType::Int64},
      {"Math", mathColor, plVisualScriptDataType::Float},
      {"Math", mathColor, plVisualScriptDataType::Double},
      {"String", stringColor, plVisualScriptDataType::String},
      {"Variant", variantColor, plVisualScriptDataType::Variant},
    };

    for (auto& conversionNodeDesc : conversionNodeDescs)
    {
      auto nodeType = plVisualScriptNodeDescription::Type::GetConversionType(conversionNodeDesc.m_DataType);

      plReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, plVisualScriptNodeDescription::Type::GetName(nodeType), conversionNodeDesc.m_szCategory, conversionNodeDesc.m_Color);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = nodeType;
      nodeDesc.m_DeductTypeFunc = &plVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
      nodeDesc.AddOutputDataPin("", plVisualScriptDataType::GetRtti(conversionNodeDesc.m_DataType), conversionNodeDesc.m_DataType);

      m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_String_Format,
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_String_Format", "String", stringColor);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "String::Format {Text}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_String_Format;

    AddInputDataPin<plString>(typeDesc, nodeDesc, "Text");
    AddInputProperty(typeDesc, "Params", plGetStaticRTTI<plVariantArray>(), plVisualScriptDataType::Array);
    nodeDesc.AddInputDataPin("Params", plGetStaticRTTI<plVariant>(), plVisualScriptDataType::Variant, false, plMakeHashedString("Params"));
    AddOutputDataPin<plString>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Variant_ConvertTo
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Variant_ConvertTo", "Variant", variantColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = plPropertyCategory::Member;
      propDesc.m_sName = "Type";
      propDesc.m_sType = plGetStaticRTTI<plVisualScriptDataType>()->GetTypeName();
      propDesc.m_Flags = plPropertyFlags::IsEnum;

      auto pAttr = PLASMA_DEFAULT_NEW(plDefaultValueAttribute, plVisualScriptDataType::Bool);
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "Variant::ConvertTo {Type}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_Variant_ConvertTo;
    nodeDesc.m_DeductTypeFunc = &plVisualScriptTypeDeduction::DeductFromScriptDataTypeProperty;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("Succeeded");
    nodeDesc.AddOutputExecutionPin("Failed");
    nodeDesc.AddInputDataPin("Variant", plGetStaticRTTI<plVariant>(), plVisualScriptDataType::Variant, true);
    nodeDesc.AddOutputDataPin("Result", nullptr, plVisualScriptDataType::Any);

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_MakeArray
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_MakeArray", "Array", variantColor);

    plHashedString sCount = plMakeHashedString("Count");
    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = plPropertyCategory::Member;
      propDesc.m_sName = sCount.GetView();
      propDesc.m_sType = plGetStaticRTTI<plUInt32>()->GetTypeName();
      propDesc.m_Flags = plPropertyFlags::StandardType;

      auto pNoTempAttr = PLASMA_DEFAULT_NEW(plNoTemporaryTransactionsAttribute);
      propDesc.m_Attributes.PushBack(pNoTempAttr);

      auto pClampAttr = PLASMA_DEFAULT_NEW(plClampValueAttribute, 0, 16);
      propDesc.m_Attributes.PushBack(pClampAttr);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_MakeArray;

    nodeDesc.AddInputDataPin("", plGetStaticRTTI<plVariant>(), plVisualScriptDataType::Variant, false, sCount);
    nodeDesc.AddOutputDataPin("", plGetStaticRTTI<plVariantArray>(), plVisualScriptDataType::Array);

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_TryGetComponentOfBaseType
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_TryGetComponentOfBaseType", "GameObject", gameObjectColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = plPropertyCategory::Member;
      propDesc.m_sName = "TypeName";
      propDesc.m_sType = plGetStaticRTTI<plString>()->GetTypeName();
      propDesc.m_Flags = plPropertyFlags::StandardType;

      auto pAttr = PLASMA_DEFAULT_NEW(plDynamicStringEnumAttribute, "ComponentTypes");
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "GameObject::TryGetComponentOfBaseType {TypeName}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_TryGetComponentOfBaseType;

    nodeDesc.AddInputDataPin("GameObject", plGetStaticRTTI<plGameObject>(), plVisualScriptDataType::GameObject, false);
    AddOutputDataPin<plComponent>(nodeDesc, "Component");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_StartCoroutine
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_StartCoroutine", szCoroutinesCategory, coroutineColor);

    AddInputProperty(typeDesc, "CoroutineMode", plGetStaticRTTI<plScriptCoroutineCreationMode>(), plVisualScriptDataType::Int64);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "StartCoroutine {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_StartCoroutine;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("CoroutineBody", plHashedString(), true);
    AddInputDataPin<plString>(typeDesc, nodeDesc, "Name");
    nodeDesc.AddOutputDataPin("CoroutineID", plGetStaticRTTI<plScriptCoroutineHandle>(), plVisualScriptDataType::Coroutine);

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_StopCoroutine
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_StopCoroutine", szCoroutinesCategory, coroutineColor);

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, "StopCoroutine {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_StopCoroutine;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("CoroutineID", plGetStaticRTTI<plScriptCoroutineHandle>(), plVisualScriptDataType::Coroutine, false);
    AddInputDataPin<plString>(typeDesc, nodeDesc, "Name");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_StopAllCoroutines
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_StopAllCoroutines", szCoroutinesCategory, coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_StopAllCoroutines;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_WaitForAll
  {
    plVisualScriptNodeDescription::Type::Enum waitTypes[] = {
      plVisualScriptNodeDescription::Type::Builtin_WaitForAll,
      plVisualScriptNodeDescription::Type::Builtin_WaitForAny,
    };

    for (auto waitType : waitTypes)
    {
      plReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, plVisualScriptNodeDescription::Type::GetName(waitType), szCoroutinesCategory, coroutineColor);

      plHashedString sCount = plMakeHashedString("Count");
      {
        auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
        propDesc.m_Category = plPropertyCategory::Member;
        propDesc.m_sName = sCount.GetView();
        propDesc.m_sType = plGetStaticRTTI<plUInt32>()->GetTypeName();
        propDesc.m_Flags = plPropertyFlags::StandardType;

        auto pNoTempAttr = PLASMA_DEFAULT_NEW(plNoTemporaryTransactionsAttribute);
        propDesc.m_Attributes.PushBack(pNoTempAttr);

        auto pDefaultAttr = PLASMA_DEFAULT_NEW(plDefaultValueAttribute, 1);
        propDesc.m_Attributes.PushBack(pDefaultAttr);

        auto pClampAttr = PLASMA_DEFAULT_NEW(plClampValueAttribute, 1, 16);
        propDesc.m_Attributes.PushBack(pClampAttr);
      }

      NodeDesc nodeDesc;
      nodeDesc.m_Type = waitType;

      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("");
      nodeDesc.AddInputDataPin("", plGetStaticRTTI<plScriptCoroutineHandle>(), plVisualScriptDataType::Coroutine, false, sCount);

      m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_Yield
  {
    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Yield", szCoroutinesCategory, coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_Yield;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }
}

void plVisualScriptNodeRegistry::CreateGetOwnerNodeType(const plRTTI* pRtti)
{
  plStringView sBaseClass = StripTypeName(pRtti->GetTypeName());

  plReflectedTypeDescriptor typeDesc;
  {
    plStringBuilder sTypeName;
    sTypeName.Set(sBaseClass, "::GetScriptOwner");

    plStringBuilder sCategory;
    sCategory.Set(sBaseClass);

    plColorGammaUB color = NiceColorFromName(sBaseClass);

    FillDesc(typeDesc, sTypeName, sCategory, color);
  }

  NodeDesc nodeDesc;
  nodeDesc.m_sFilterByBaseClass.Assign(sBaseClass);
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_Type = plVisualScriptNodeDescription::Type::GetScriptOwner;

  plVisualScriptDataType::Enum scriptDataType;
  if (GetScriptDataType(pRtti, scriptDataType, "GetScriptOwner", "").Failed())
    return;

  if (pRtti->IsDerivedFrom<plComponent>())
  {
    nodeDesc.AddOutputDataPin("World", plGetStaticRTTI<plWorld>(), plVisualScriptDataType::TypedPointer);
    nodeDesc.AddOutputDataPin("GameObject", plGetStaticRTTI<plGameObject>(), plVisualScriptDataType::GameObject);
    nodeDesc.AddOutputDataPin("Component", plGetStaticRTTI<plComponent>(), plVisualScriptDataType::Component);
  }
  else
  {
    nodeDesc.AddOutputDataPin("World", plGetStaticRTTI<plWorld>(), plVisualScriptDataType::TypedPointer);
    nodeDesc.AddOutputDataPin("Owner", pRtti, scriptDataType);
  }

  m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
}

void plVisualScriptNodeRegistry::CreateFunctionCallNodeType(const plRTTI* pRtti, const plAbstractFunctionProperty* pFunction, const plScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction)
{
  plHashSet<plStringView> dynamicPins;
  for (auto pAttribute : pFunction->GetAttributes())
  {
    if (auto pDynamicPinAttribute = plDynamicCast<const plDynamicPinAttribute*>(pAttribute))
    {
      dynamicPins.Insert(pDynamicPinAttribute->GetProperty());
    }
  }

  plHybridArray<const plFunctionArgumentAttributes*, 8> argumentAttributes;
  CollectFunctionArgumentAttributes(pFunction, argumentAttributes);

  plStringView sTypeName = StripTypeName(pRtti->GetTypeName());

  plStringView sFunctionName = pFunction->GetPropertyName();
  sFunctionName.TrimWordStart("Reflection_");

  plReflectedTypeDescriptor typeDesc;
  bool bHasTitle = false;
  {
    if (bIsEntryFunction)
    {
      plStringBuilder sCategory;
      sCategory.Set(szEventHandlerCategory, sTypeName);

      plColorGammaUB color = NiceColorFromName(sTypeName);

      FillDesc(typeDesc, pRtti, sCategory, &color);
    }
    else
    {
      FillDesc(typeDesc, pRtti);
    }

    plStringBuilder temp;
    temp.Set(typeDesc.m_sTypeName, "::", sFunctionName);
    typeDesc.m_sTypeName = temp;

    if (bIsEntryFunction)
    {
      AddInputProperty(typeDesc, "CoroutineMode", plGetStaticRTTI<plScriptCoroutineCreationMode>(), plVisualScriptDataType::Int64);
    }

    if (auto pTitleAttribute = pFunction->GetAttributeByType<plTitleAttribute>())
    {
      auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, pTitleAttribute->GetTitle());
      typeDesc.m_Attributes.PushBack(pAttr);

      bHasTitle = true;
    }
  }

  NodeDesc nodeDesc;
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_TargetProperties.PushBack(pFunction);
  if (bIsEntryFunction)
  {
    nodeDesc.m_sFilterByBaseClass.Assign(sTypeName);
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::EntryCall;
  }
  else
  {
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::ReflectedFunction;
  }

  {
    if (pFunction->GetFlags().IsSet(plPropertyFlags::Const) == false)
    {
      if (bIsEntryFunction == false)
      {
        nodeDesc.AddInputExecutionPin("");
      }
      nodeDesc.AddOutputExecutionPin("");
    }

    if (bIsEntryFunction == false)
    {
      if (pFunction->GetFunctionType() == plFunctionType::Member)
      {
        // GameObject and World pins will default to the script owner's game object/world thus they are not required
        const bool bRequired = pRtti->IsDerivedFrom<plGameObject>() == false && pRtti->IsDerivedFrom<plWorld>() == false;
        nodeDesc.AddInputDataPin(sTypeName, pRtti, plVisualScriptDataType::FromRtti(pRtti), bRequired);
      }

      if (const plRTTI* pReturnRtti = pFunction->GetReturnType())
      {
        plVisualScriptDataType::Enum scriptDataType;
        if (GetScriptDataType(pReturnRtti, scriptDataType, pFunction->GetPropertyName(), "return value").Failed())
        {
          return;
        }

        nodeDesc.AddOutputDataPin("Result", pReturnRtti, scriptDataType);
      }
    }

    plUInt32 titleArgIdx = plInvalidIndex;

    plStringBuilder sArgName;
    for (plUInt32 argIdx = 0; argIdx < pFunction->GetArgumentCount(); ++argIdx)
    {
      sArgName = pScriptableFunctionAttribute->GetArgumentName(argIdx);
      if (sArgName.IsEmpty())
        sArgName.Format("Arg{}", argIdx);

      auto pArgRtti = pFunction->GetArgumentType(argIdx);
      auto argType = pScriptableFunctionAttribute->GetArgumentType(argIdx);
      const bool bIsDynamicPinProperty = dynamicPins.Contains(sArgName);

      plHashedString sDynamicPinProperty;
      if (bIsDynamicPinProperty)
      {
        sDynamicPinProperty.Assign(sArgName);
      }

      plVisualScriptDataType::Enum scriptDataType;
      if (GetScriptDataType(pArgRtti, scriptDataType, pFunction->GetPropertyName(), sArgName).Failed())
      {
        return;
      }

      plVisualScriptDataType::Enum pinScriptDataType = scriptDataType;
      if (bIsDynamicPinProperty && scriptDataType == plVisualScriptDataType::Array)
      {
        pArgRtti = plGetStaticRTTI<plVariant>();
        pinScriptDataType = plVisualScriptDataType::Variant;
      }

      if (bIsEntryFunction)
      {
        nodeDesc.AddOutputDataPin(sArgName, pArgRtti, scriptDataType);
      }
      else
      {
        if (argType == plScriptableFunctionAttribute::In || argType == plScriptableFunctionAttribute::Inout)
        {
          if (plVisualScriptDataType::IsPointer(scriptDataType) == false)
          {
            plArrayPtr<const plPropertyAttribute* const> attributes;
            if (argIdx < argumentAttributes.GetCount() && argumentAttributes[argIdx] != nullptr)
            {
              attributes = argumentAttributes[argIdx]->GetArgumentAttributes();
            }

            AddInputProperty(typeDesc, sArgName, pArgRtti, scriptDataType, attributes);
          }

          nodeDesc.AddInputDataPin(sArgName, pArgRtti, pinScriptDataType, false, sDynamicPinProperty);

          if (titleArgIdx == plInvalidIndex &&
              (pinScriptDataType == plVisualScriptDataType::String || pinScriptDataType == plVisualScriptDataType::HashedString))
          {
            titleArgIdx = argIdx;
          }
        }
        else if (argType == plScriptableFunctionAttribute::Out || argType == plScriptableFunctionAttribute::Inout)
        {
          plLog::Error("Script function out parameter are not yet supported");
          return;

#if 0
          if (!pFunction->GetArgumentFlags(argIdx).IsSet(plPropertyFlags::Reference))
          {
            // TODO: plPropertyFlags::Reference is also set for const-ref parameters, should we change that ?

            plLog::Error("Script function '{}' argument {} is marked 'out' but is not a non-const reference value", pRtti->GetTypeName(), argIdx);
            return;
          }

          nodeDesc.AddOutputDataPin(sArgName, pArgRtti, scriptDataType, sDynamicPinProperty);
#endif
        }
      }
    }

    if (bIsEntryFunction)
    {
      nodeDesc.AddOutputDataPin("CoroutineID", plGetStaticRTTI<plScriptCoroutineHandle>(), plVisualScriptDataType::Coroutine);
    }

    if (bHasTitle == false && titleArgIdx != plInvalidIndex)
    {
      plStringBuilder sTitle;
      sTitle.Set(GetTypeName(pRtti), "::", sFunctionName, " {", pScriptableFunctionAttribute->GetArgumentName(titleArgIdx), "}");

      auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, sTitle);
      typeDesc.m_Attributes.PushBack(pAttr);
    }
  }

  m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
}

void plVisualScriptNodeRegistry::CreateCoroutineNodeType(const plRTTI* pRtti)
{
  if (pRtti->GetTypeFlags().IsSet(plTypeFlags::Abstract))
    return;

  const plAbstractFunctionProperty* pStartFunc = nullptr;
  const plScriptableFunctionAttribute* pScriptableFuncAttribute = nullptr;
  for (auto pFunc : pRtti->GetFunctions())
  {
    if (plStringUtils::IsEqual(pFunc->GetPropertyName(), "Start"))
    {
      if (auto pAttr = pFunc->GetAttributeByType<plScriptableFunctionAttribute>())
      {
        pStartFunc = pFunc;
        pScriptableFuncAttribute = pAttr;
        break;
      }
    }
  }

  if (pStartFunc == nullptr || pScriptableFuncAttribute == nullptr)
  {
    plLog::Warning("The script coroutine '{}' has no reflected script function called 'Start'.", pRtti->GetTypeName());
    return;
  }

  plReflectedTypeDescriptor typeDesc;
  {
    const plColorGammaUB coroutineColor = PinDesc::GetColorForScriptDataType(plVisualScriptDataType::Coroutine);
    FillDesc(typeDesc, pRtti, szCoroutinesCategory, &coroutineColor);

    plStringBuilder temp;
    temp.Set("Coroutine::", typeDesc.m_sTypeName);
    typeDesc.m_sTypeName = temp;

    if (auto pTitleAttribute = pRtti->GetAttributeByType<plTitleAttribute>())
    {
      auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, pTitleAttribute->GetTitle());
      typeDesc.m_Attributes.PushBack(pAttr);
    }
  }

  NodeDesc nodeDesc;
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_TargetProperties.PushBack(pStartFunc);
  nodeDesc.m_Type = plVisualScriptNodeDescription::Type::InplaceCoroutine;

  nodeDesc.AddInputExecutionPin("");
  nodeDesc.AddOutputExecutionPin("Succeeded");
  nodeDesc.AddOutputExecutionPin("Failed");

  plStringBuilder sArgName;
  for (plUInt32 argIdx = 0; argIdx < pStartFunc->GetArgumentCount(); ++argIdx)
  {
    sArgName = pScriptableFuncAttribute->GetArgumentName(argIdx);
    if (sArgName.IsEmpty())
      sArgName.Format("Arg{}", argIdx);

    auto pArgRtti = pStartFunc->GetArgumentType(argIdx);
    auto argType = pScriptableFuncAttribute->GetArgumentType(argIdx);
    if (argType != plScriptableFunctionAttribute::In)
    {
      plLog::Error("Script function out parameter are not yet supported");
      return;
    }

    plVisualScriptDataType::Enum scriptDataType = plVisualScriptDataType::Invalid;
    if (GetScriptDataType(pArgRtti, scriptDataType, pStartFunc->GetPropertyName(), sArgName).Failed())
    {
      return;
    }

    if (plVisualScriptDataType::IsPointer(scriptDataType) == false)
    {
      AddInputProperty(typeDesc, sArgName, pArgRtti, scriptDataType);
    }

    nodeDesc.AddInputDataPin(sArgName, pArgRtti, scriptDataType, false);
  }

  m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
}

void plVisualScriptNodeRegistry::CreateMessageNodeTypes(const plRTTI* pRtti)
{
  if (pRtti == plGetStaticRTTI<plMessage>() ||
      pRtti == plGetStaticRTTI<plEventMessage>() ||
      pRtti->GetTypeFlags().IsSet(plTypeFlags::Abstract))
    return;

  // Message Handler
  {
    plReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti, szEventHandlerCategory);

      plStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "On", GetTypeName(pRtti));
      typeDesc.m_sTypeName = temp;

      AddInputProperty(typeDesc, "CoroutineMode", plGetStaticRTTI<plScriptCoroutineCreationMode>(), plVisualScriptDataType::Int64);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::MessageHandler;

    nodeDesc.AddOutputExecutionPin("");

    plHybridArray<const plAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      auto pPropRtti = pProp->GetSpecificType();
      plVisualScriptDataType::Enum scriptDataType = GetScriptDataType(pProp);
      if (scriptDataType == plVisualScriptDataType::Invalid)
        continue;

      nodeDesc.AddOutputDataPin(pProp->GetPropertyName(), pPropRtti, scriptDataType);

      nodeDesc.m_TargetProperties.PushBack(pProp);
    }

    nodeDesc.AddOutputDataPin("CoroutineID", plGetStaticRTTI<plScriptCoroutineHandle>(), plVisualScriptDataType::Coroutine);

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Message Sender
  {
    plReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti, "Messages");

      plStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "Send", GetTypeName(pRtti));
      typeDesc.m_sTypeName = temp;

      temp.Set("Send{?SendMode}", GetTypeName(pRtti), " {Delay}");
      auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, temp);
      typeDesc.m_Attributes.PushBack(pAttr);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::SendMessage;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("GameObject", plGetStaticRTTI<plGameObject>(), plVisualScriptDataType::GameObject, false);
    nodeDesc.AddInputDataPin("Component", plGetStaticRTTI<plComponent>(), plVisualScriptDataType::Component, false);
    AddInputDataPin<plVisualScriptSendMessageMode>(typeDesc, nodeDesc, "SendMode");
    AddInputDataPin<plTime>(typeDesc, nodeDesc, "Delay");

    plHybridArray<const plAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
        continue;

      auto szPropName = pProp->GetPropertyName();
      auto pPropRtti = pProp->GetSpecificType();
      plVisualScriptDataType::Enum scriptDataType = GetScriptDataType(pProp);
      if (scriptDataType == plVisualScriptDataType::Invalid)
        continue;

      if (plVisualScriptDataType::IsPointer(scriptDataType) == false)
      {
        AddInputProperty(typeDesc, szPropName, pPropRtti, scriptDataType);
      }

      nodeDesc.AddInputDataPin(szPropName, pPropRtti, scriptDataType, false);
      nodeDesc.AddOutputDataPin(szPropName, pPropRtti, scriptDataType);

      nodeDesc.m_TargetProperties.PushBack(pProp);
    }

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }
}

void plVisualScriptNodeRegistry::CreateEnumNodeTypes(const plRTTI* pRtti)
{
  if (m_EnumTypes.Insert(pRtti))
    return;

  plStringView sTypeName = GetTypeName(pRtti);
  plColorGammaUB enumColor = PinDesc::GetColorForScriptDataType(plVisualScriptDataType::EnumValue);

  // Value
  {
    plStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Value");

    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, szEnumsCategory, enumColor);
    AddInputProperty(typeDesc, "Value", pRtti, plVisualScriptDataType::EnumValue);

    plStringBuilder sTitle;
    sTitle.Set(GetTypeName(pRtti), "::{Value}");

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, sTitle);
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_Constant;
    nodeDesc.AddOutputDataPin("Value", pRtti, plVisualScriptDataType::EnumValue);

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Switch
  {
    plStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Switch");

    plReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, szEnumsCategory, enumColor);

    plStringBuilder sTitle;
    sTitle.Set(GetTypeName(pRtti), "::Switch");

    auto pAttr = PLASMA_DEFAULT_NEW(plTitleAttribute, sTitle);
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type = plVisualScriptNodeDescription::Type::Builtin_Switch;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddInputDataPin("Value", pRtti, plVisualScriptDataType::EnumValue, false);

    plHybridArray<plReflectionUtils::EnumKeyValuePair, 16> enumKeysAndValues;
    plReflectionUtils::GetEnumKeysAndValues(pRtti, enumKeysAndValues, plReflectionUtils::EnumConversionMode::ValueNameOnly);
    for (auto& keyAndValue : enumKeysAndValues)
    {
      nodeDesc.AddOutputExecutionPin(keyAndValue.m_sKey);
    }

    m_TypeToNodeDescs.Insert(plPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }
}

void plVisualScriptNodeRegistry::FillDesc(plReflectedTypeDescriptor& desc, const plRTTI* pRtti, plStringView sCategoryOverride /*= plStringView()*/, const plColorGammaUB* pColorOverride /*= nullptr */)
{
  plStringBuilder sTypeName = GetTypeName(pRtti);
  const plRTTI* pBaseClass = FindTopMostBaseClass(pRtti);

  plStringBuilder sCategory;
  if (sCategoryOverride.IsEmpty())
  {
    if (pBaseClass != pRtti)
    {
      sCategory.Set(StripTypeName(pBaseClass->GetTypeName()), "/", sTypeName);
    }
    else
    {
      sCategory = sTypeName;
    }
  }
  else
  {
    sCategory = sCategoryOverride;
  }

  plColorGammaUB color;
  if (pColorOverride == nullptr)
  {
    if (pBaseClass != pRtti)
    {
      color = NiceColorFromName(sTypeName, StripTypeName(pBaseClass->GetTypeName()));
    }
    else
    {
      auto scriptDataType = plVisualScriptDataType::FromRtti(pRtti);
      if (scriptDataType != plVisualScriptDataType::Invalid &&
          scriptDataType != plVisualScriptDataType::Component &&
          scriptDataType != plVisualScriptDataType::TypedPointer)
      {
        color = PinDesc::GetColorForScriptDataType(scriptDataType);
      }
      else
      {
        color = NiceColorFromName(sTypeName);
      }
    }
  }
  else
  {
    color = *pColorOverride;
  }

  FillDesc(desc, sTypeName, sCategory, color);
}

void plVisualScriptNodeRegistry::FillDesc(plReflectedTypeDescriptor& desc, plStringView sTypeName, plStringView sCategory, const plColorGammaUB& color)
{
  plStringBuilder sTypeNameFull;
  sTypeNameFull.Set(s_szTypeNamePrefix, sTypeName);

  desc.m_sTypeName = sTypeNameFull;
  desc.m_sPluginName = szPluginName;
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = plTypeFlags::Phantom | plTypeFlags::Class;

  // Category
  {
    plStringBuilder tmp;
    auto pAttr = PLASMA_DEFAULT_NEW(plCategoryAttribute, sCategory.GetData(tmp));
    desc.m_Attributes.PushBack(pAttr);
  }

  // Color
  {
    auto pAttr = PLASMA_DEFAULT_NEW(plColorAttribute, color);
    desc.m_Attributes.PushBack(pAttr);
  }
}
