#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct PL_EDITORFRAMEWORK_DLL plExposedParameter
{
  plExposedParameter();
  virtual ~plExposedParameter();

  plString m_sName;
  plString m_sType;
  plVariant m_DefaultValue;
  plHybridArray<plPropertyAttribute*, 2> m_Attributes;
};
PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORFRAMEWORK_DLL, plExposedParameter)

class PL_EDITORFRAMEWORK_DLL plExposedParameters : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plExposedParameters, plReflectedClass);

public:
  plExposedParameters();
  virtual ~plExposedParameters();

  const plExposedParameter* Find(const char* szParamName) const;

  plDynamicArray<plExposedParameter*> m_Parameters;
};
