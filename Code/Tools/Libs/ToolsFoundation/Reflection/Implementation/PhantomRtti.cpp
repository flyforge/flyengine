#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/PhantomProperty.h>
#include <ToolsFoundation/Reflection/PhantomRtti.h>

plPhantomRTTI::plPhantomRTTI(const char* szName, const plRTTI* pParentType, plUInt32 uiTypeSize, plUInt32 uiTypeVersion, plUInt8 uiVariantType,
  plBitflags<plTypeFlags> flags, const char* szPluginName)
  : plRTTI(nullptr, pParentType, uiTypeSize, uiTypeVersion, uiVariantType, flags | plTypeFlags::Phantom, nullptr, plArrayPtr<const plAbstractProperty*>(),
      plArrayPtr<const plAbstractFunctionProperty*>(), plArrayPtr<const plPropertyAttribute*>(), plArrayPtr<plAbstractMessageHandler*>(),
      plArrayPtr<plMessageSenderInfo>(), nullptr)
{
  m_sTypeNameStorage = szName;
  m_sPluginNameStorage = szPluginName;

  m_sTypeName = m_sTypeNameStorage.GetData();
  m_sPluginName = m_sPluginNameStorage.GetData();

  RegisterType();
}

plPhantomRTTI::~plPhantomRTTI()
{
  UnregisterType();
  m_sTypeName = nullptr;

  for (auto pProp : m_PropertiesStorage)
  {
    PLASMA_DEFAULT_DELETE(pProp);
  }
  for (auto pFunc : m_FunctionsStorage)
  {
    PLASMA_DEFAULT_DELETE(pFunc);
  }
  for (auto pAttrib : m_AttributesStorage)
  {
    auto pAttribNonConst = const_cast<plPropertyAttribute*>(pAttrib);
    PLASMA_DEFAULT_DELETE(pAttribNonConst);
  }
}

void plPhantomRTTI::SetProperties(plDynamicArray<plReflectedPropertyDescriptor>& properties)
{
  for (auto pProp : m_PropertiesStorage)
  {
    PLASMA_DEFAULT_DELETE(pProp);
  }
  m_PropertiesStorage.Clear();

  const plUInt32 iCount = properties.GetCount();
  m_PropertiesStorage.Reserve(iCount);

  for (plUInt32 i = 0; i < iCount; i++)
  {
    switch (properties[i].m_Category)
    {
      case plPropertyCategory::Constant:
      {
        m_PropertiesStorage.PushBack(PLASMA_DEFAULT_NEW(plPhantomConstantProperty, &properties[i]));
      }
      break;
      case plPropertyCategory::Member:
      {
        m_PropertiesStorage.PushBack(PLASMA_DEFAULT_NEW(plPhantomMemberProperty, &properties[i]));
      }
      break;
      case plPropertyCategory::Array:
      {
        m_PropertiesStorage.PushBack(PLASMA_DEFAULT_NEW(plPhantomArrayProperty, &properties[i]));
      }
      break;
      case plPropertyCategory::Set:
      {
        m_PropertiesStorage.PushBack(PLASMA_DEFAULT_NEW(plPhantomSetProperty, &properties[i]));
      }
      break;
      case plPropertyCategory::Map:
      {
        m_PropertiesStorage.PushBack(PLASMA_DEFAULT_NEW(plPhantomMapProperty, &properties[i]));
      }
      break;
      case plPropertyCategory::Function:
        break; // Handled in SetFunctions
    }
  }

  m_Properties = m_PropertiesStorage.GetArrayPtr();
}


void plPhantomRTTI::SetFunctions(plDynamicArray<plReflectedFunctionDescriptor>& functions)
{
  for (auto pProp : m_FunctionsStorage)
  {
    PLASMA_DEFAULT_DELETE(pProp);
  }
  m_FunctionsStorage.Clear();

  const plUInt32 iCount = functions.GetCount();
  m_FunctionsStorage.Reserve(iCount);

  for (plUInt32 i = 0; i < iCount; i++)
  {
    m_FunctionsStorage.PushBack(PLASMA_DEFAULT_NEW(plPhantomFunctionProperty, &functions[i]));
  }

  m_Functions = m_FunctionsStorage.GetArrayPtr();
}

void plPhantomRTTI::SetAttributes(plDynamicArray<const plPropertyAttribute*>& attributes)
{
  for (auto pAttrib : m_AttributesStorage)
  {
    auto pAttribNonConst = const_cast<plPropertyAttribute*>(pAttrib);
    PLASMA_DEFAULT_DELETE(pAttribNonConst);
  }
  m_AttributesStorage.Clear();
  m_AttributesStorage = attributes;
  m_Attributes = m_AttributesStorage;
  attributes.Clear();
}

void plPhantomRTTI::UpdateType(plReflectedTypeDescriptor& desc)
{
  plRTTI::UpdateType(plRTTI::FindTypeByName(desc.m_sParentTypeName), 0, desc.m_uiTypeVersion, plVariantType::Invalid, desc.m_Flags);

  m_sPluginNameStorage = desc.m_sPluginName;
  m_sPluginName = m_sPluginNameStorage.GetData();

  SetProperties(desc.m_Properties);
  SetFunctions(desc.m_Functions);
  SetAttributes(desc.m_Attributes);
  SetupParentHierarchy();
}

bool plPhantomRTTI::IsEqualToDescriptor(const plReflectedTypeDescriptor& desc)
{
  if ((desc.m_Flags.GetValue() & ~plTypeFlags::Phantom) != (GetTypeFlags().GetValue() & ~plTypeFlags::Phantom))
    return false;

  if (desc.m_sParentTypeName.IsEmpty() && GetParentType() != nullptr)
    return false;

  if (GetParentType() != nullptr && desc.m_sParentTypeName != GetParentType()->GetTypeName())
    return false;

  if (desc.m_sPluginName != GetPluginName())
    return false;

  if (desc.m_sTypeName != GetTypeName())
    return false;

  if (desc.m_Properties.GetCount() != GetProperties().GetCount())
    return false;

  for (plUInt32 i = 0; i < GetProperties().GetCount(); i++)
  {
    if (desc.m_Properties[i].m_Category != GetProperties()[i]->GetCategory())
      return false;

    if (desc.m_Properties[i].m_sName != GetProperties()[i]->GetPropertyName())
      return false;

    if ((desc.m_Properties[i].m_Flags.GetValue() & ~plPropertyFlags::Phantom) !=
        (GetProperties()[i]->GetFlags().GetValue() & ~plPropertyFlags::Phantom))
      return false;

    switch (desc.m_Properties[i].m_Category)
    {
      case plPropertyCategory::Constant:
      {
        auto pProp = (plPhantomConstantProperty*)GetProperties()[i];

        if (pProp->GetSpecificType() != plRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;

        if (pProp->GetConstant() != desc.m_Properties[i].m_ConstantValue)
          return false;
      }
      break;
      case plPropertyCategory::Member:
      {
        if (GetProperties()[i]->GetSpecificType() != plRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case plPropertyCategory::Array:
      {
        if (GetProperties()[i]->GetSpecificType() != plRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case plPropertyCategory::Set:
      {
        if (GetProperties()[i]->GetSpecificType() != plRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case plPropertyCategory::Map:
      {
        if (GetProperties()[i]->GetSpecificType() != plRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case plPropertyCategory::Function:
        break; // Functions handled below
    }

    if (desc.m_Functions.GetCount() != GetFunctions().GetCount())
      return false;

    for (plUInt32 j = 0; j < GetFunctions().GetCount(); j++)
    {
      const plAbstractFunctionProperty* pProp = GetFunctions()[j];
      if (desc.m_Functions[j].m_sName != pProp->GetPropertyName())
        return false;
      if ((desc.m_Functions[j].m_Flags.GetValue() & ~plPropertyFlags::Phantom) != (pProp->GetFlags().GetValue() & ~plPropertyFlags::Phantom))
        return false;
      if (desc.m_Functions[j].m_Type != pProp->GetFunctionType())
        return false;

      if (pProp->GetReturnType() != plRTTI::FindTypeByName(desc.m_Functions[j].m_ReturnValue.m_sType))
        return false;
      if (pProp->GetReturnFlags() != desc.m_Functions[j].m_ReturnValue.m_Flags)
        return false;
      if (desc.m_Functions[j].m_Arguments.GetCount() != pProp->GetArgumentCount())
        return false;
      for (plUInt32 a = 0; a < pProp->GetArgumentCount(); a++)
      {
        if (pProp->GetArgumentType(a) != plRTTI::FindTypeByName(desc.m_Functions[j].m_Arguments[a].m_sType))
          return false;
        if (pProp->GetArgumentFlags(a) != desc.m_Functions[j].m_Arguments[a].m_Flags)
          return false;
      }
    }

    if (desc.m_Properties[i].m_Attributes.GetCount() != GetProperties()[i]->GetAttributes().GetCount())
      return false;

    for (plUInt32 i2 = 0; i2 < desc.m_Properties[i].m_Attributes.GetCount(); i2++)
    {
      if (!plReflectionUtils::IsEqual(desc.m_Properties[i].m_Attributes[i2], GetProperties()[i]->GetAttributes()[i2]))
        return false;
    }
  }

  if (desc.m_Attributes.GetCount() != GetAttributes().GetCount())
    return false;

  // TODO: compare attribute values?
  for (plUInt32 i = 0; i < GetAttributes().GetCount(); i++)
  {
    if (desc.m_Attributes[i]->GetDynamicRTTI() != GetAttributes()[i]->GetDynamicRTTI())
      return false;
  }
  return true;
}