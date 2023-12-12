#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class plPhantomRTTI : public plRTTI
{
  friend class plPhantomRttiManager;

public:
  ~plPhantomRTTI();

private:
  plPhantomRTTI(const char* szName, const plRTTI* pParentType, plUInt32 uiTypeSize, plUInt32 uiTypeVersion, plUInt8 uiVariantType,
    plBitflags<plTypeFlags> flags, const char* szPluginName);

  void SetProperties(plDynamicArray<plReflectedPropertyDescriptor>& properties);
  void SetFunctions(plDynamicArray<plReflectedFunctionDescriptor>& functions);
  void SetAttributes(plDynamicArray<const plPropertyAttribute*>& attributes);
  bool IsEqualToDescriptor(const plReflectedTypeDescriptor& desc);

  void UpdateType(plReflectedTypeDescriptor& desc);

private:
  plString m_sTypeNameStorage;
  plString m_sPluginNameStorage;
  plDynamicArray<plAbstractProperty*> m_PropertiesStorage;
  plDynamicArray<plAbstractFunctionProperty*> m_FunctionsStorage;
  plDynamicArray<const plPropertyAttribute*> m_AttributesStorage;
};
