#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Reflection/PhantomProperty.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

plPhantomConstantProperty::plPhantomConstantProperty(const plReflectedPropertyDescriptor* pDesc)
  : plAbstractConstantProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_Value = pDesc->m_ConstantValue;
  m_pPropertyType = plRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(plPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

plPhantomConstantProperty::~plPhantomConstantProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<plPropertyAttribute*>(pAttr));
  }
}

const plRTTI* plPhantomConstantProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

void* plPhantomConstantProperty::GetPropertyPointer() const
{
  return nullptr;
}



plPhantomMemberProperty::plPhantomMemberProperty(const plReflectedPropertyDescriptor* pDesc)
  : plAbstractMemberProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = plRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(plPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

plPhantomMemberProperty::~plPhantomMemberProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<plPropertyAttribute*>(pAttr));
  }
}

const plRTTI* plPhantomMemberProperty::GetSpecificType() const
{
  return m_pPropertyType;
}



plPhantomFunctionProperty::plPhantomFunctionProperty(plReflectedFunctionDescriptor* pDesc)
  : plAbstractFunctionProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_FunctionType = pDesc->m_Type;
  m_Flags = pDesc->m_Flags;
  m_Flags.Add(plPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();

  m_ReturnValue = pDesc->m_ReturnValue;
  m_Arguments.Swap(pDesc->m_Arguments);
}



plPhantomFunctionProperty::~plPhantomFunctionProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<plPropertyAttribute*>(pAttr));
  }
}

plFunctionType::Enum plPhantomFunctionProperty::GetFunctionType() const
{
  return m_FunctionType;
}

const plRTTI* plPhantomFunctionProperty::GetReturnType() const
{
  return plRTTI::FindTypeByName(m_ReturnValue.m_sType);
}

plBitflags<plPropertyFlags> plPhantomFunctionProperty::GetReturnFlags() const
{
  return m_ReturnValue.m_Flags;
}

plUInt32 plPhantomFunctionProperty::GetArgumentCount() const
{
  return m_Arguments.GetCount();
}

const plRTTI* plPhantomFunctionProperty::GetArgumentType(plUInt32 uiParamIndex) const
{
  return plRTTI::FindTypeByName(m_Arguments[uiParamIndex].m_sType);
}

plBitflags<plPropertyFlags> plPhantomFunctionProperty::GetArgumentFlags(plUInt32 uiParamIndex) const
{
  return m_Arguments[uiParamIndex].m_Flags;
}

void plPhantomFunctionProperty::Execute(void* pInstance, plArrayPtr<plVariant> values, plVariant& ref_returnValue) const
{
  PL_ASSERT_NOT_IMPLEMENTED;
}

plPhantomArrayProperty::plPhantomArrayProperty(const plReflectedPropertyDescriptor* pDesc)
  : plAbstractArrayProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = plRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(plPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

plPhantomArrayProperty::~plPhantomArrayProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<plPropertyAttribute*>(pAttr));
  }
}

const plRTTI* plPhantomArrayProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

plPhantomSetProperty::plPhantomSetProperty(const plReflectedPropertyDescriptor* pDesc)
  : plAbstractSetProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = plRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(plPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

plPhantomSetProperty::~plPhantomSetProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<plPropertyAttribute*>(pAttr));
  }
}

const plRTTI* plPhantomSetProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

plPhantomMapProperty::plPhantomMapProperty(const plReflectedPropertyDescriptor* pDesc)
  : plAbstractMapProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = plRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(plPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

plPhantomMapProperty::~plPhantomMapProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<plPropertyAttribute*>(pAttr));
  }
}

const plRTTI* plPhantomMapProperty::GetSpecificType() const
{
  return m_pPropertyType;
}
