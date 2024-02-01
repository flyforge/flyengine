#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Add this attribute to a class to add script functions to the szTypeName class.
/// This might be necessary if the specified class is not reflected or to separate script functions from the specified class.
class PL_CORE_DLL plScriptExtensionAttribute : public plPropertyAttribute
{
  PL_ADD_DYNAMIC_REFLECTION(plScriptExtensionAttribute, plPropertyAttribute);

public:
  plScriptExtensionAttribute();
  plScriptExtensionAttribute(plStringView sTypeName);

  plStringView GetTypeName() const { return m_sTypeName; }

private:
  plUntrackedString m_sTypeName;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Add this attribute to a script function to mark it as a base class function.
/// These are functions that can be entry points to visual scripts or over-writable functions in script languages like e.g. typescript.
class PL_CORE_DLL plScriptBaseClassFunctionAttribute : public plPropertyAttribute
{
  PL_ADD_DYNAMIC_REFLECTION(plScriptBaseClassFunctionAttribute, plPropertyAttribute);

public:
  plScriptBaseClassFunctionAttribute();
  plScriptBaseClassFunctionAttribute(plUInt16 uiIndex);

  plUInt16 GetIndex() const { return m_uiIndex; }

private:
  plUInt16 m_uiIndex;
};
