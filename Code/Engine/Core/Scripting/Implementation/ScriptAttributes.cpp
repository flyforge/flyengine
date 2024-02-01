#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plScriptExtensionAttribute, 1, plRTTIDefaultAllocator<plScriptExtensionAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("TypeName", m_sTypeName),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plScriptExtensionAttribute::plScriptExtensionAttribute() = default;
plScriptExtensionAttribute::plScriptExtensionAttribute(plStringView sTypeName)
  : m_sTypeName(sTypeName)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plScriptBaseClassFunctionAttribute, 1, plRTTIDefaultAllocator<plScriptBaseClassFunctionAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Index", m_uiIndex),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plScriptBaseClassFunctionAttribute::plScriptBaseClassFunctionAttribute() = default;
plScriptBaseClassFunctionAttribute::plScriptBaseClassFunctionAttribute(plUInt16 uiIndex)
  : m_uiIndex(uiIndex)
{
}


PL_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptAttributes);

