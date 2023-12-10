#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct PLASMA_EDITORFRAMEWORK_DLL plExposedParameter
{
  plExposedParameter();
  virtual ~plExposedParameter();

  plString m_sName;
  plString m_sType;
  plVariant m_DefaultValue;
  plHybridArray<plPropertyAttribute*, 2> m_Attributes;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_EDITORFRAMEWORK_DLL, plExposedParameter)

class PLASMA_EDITORFRAMEWORK_DLL plExposedParameters : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExposedParameters, plReflectedClass);

public:
  plExposedParameters();
  virtual ~plExposedParameters();

  const plExposedParameter* Find(const char* szParamName) const;

  plDynamicArray<plExposedParameter*> m_Parameters;
};
