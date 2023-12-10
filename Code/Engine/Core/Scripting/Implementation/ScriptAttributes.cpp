#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plScriptExtensionAttribute, 1, plRTTIDefaultAllocator<plScriptExtensionAttribute>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("TypeName", m_sTypeName),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plScriptExtensionAttribute::plScriptExtensionAttribute() = default;
plScriptExtensionAttribute::plScriptExtensionAttribute(plStringView sTypeName)
  : m_sTypeName(sTypeName)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plScriptBaseClassFunctionAttribute, 1, plRTTIDefaultAllocator<plScriptBaseClassFunctionAttribute>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Index", m_uiIndex),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plScriptBaseClassFunctionAttribute::plScriptBaseClassFunctionAttribute() = default;
plScriptBaseClassFunctionAttribute::plScriptBaseClassFunctionAttribute(plUInt16 uiIndex)
  : m_uiIndex(uiIndex)
{
}
