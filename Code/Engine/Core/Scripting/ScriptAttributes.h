#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class PLASMA_CORE_DLL plScriptExtensionAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plScriptExtensionAttribute, plPropertyAttribute);

public:
  plScriptExtensionAttribute();
  plScriptExtensionAttribute(plStringView sTypeName);

  plStringView GetTypeName() const { return m_sTypeName; }

private:
  plUntrackedString m_sTypeName;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_CORE_DLL plScriptBaseClassFunctionAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plScriptBaseClassFunctionAttribute, plPropertyAttribute);

public:
  plScriptBaseClassFunctionAttribute();
  plScriptBaseClassFunctionAttribute(plUInt16 uiIndex);

  plUInt16 GetIndex() const { return m_uiIndex; }

private:
  plUInt16 m_uiIndex;
};