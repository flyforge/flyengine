#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class plTypeScriptParameter : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plTypeScriptParameter, plReflectedClass);

public:
  plString m_sName;
};

class plTypeScriptParameterNumber : public plTypeScriptParameter
{
  PL_ADD_DYNAMIC_REFLECTION(plTypeScriptParameterNumber, plTypeScriptParameter);

public:
  double m_DefaultValue = 0;
};

class plTypeScriptParameterBool : public plTypeScriptParameter
{
  PL_ADD_DYNAMIC_REFLECTION(plTypeScriptParameterBool, plTypeScriptParameter);

public:
  bool m_DefaultValue = false;
};

class plTypeScriptParameterString : public plTypeScriptParameter
{
  PL_ADD_DYNAMIC_REFLECTION(plTypeScriptParameterString, plTypeScriptParameter);

public:
  plString m_DefaultValue;
};

class plTypeScriptParameterVec3 : public plTypeScriptParameter
{
  PL_ADD_DYNAMIC_REFLECTION(plTypeScriptParameterVec3, plTypeScriptParameter);

public:
  plVec3 m_DefaultValue = plVec3(0.0f);
};

class plTypeScriptParameterColor : public plTypeScriptParameter
{
  PL_ADD_DYNAMIC_REFLECTION(plTypeScriptParameterColor, plTypeScriptParameter);

public:
  plColor m_DefaultValue = plColor::White;
};


class plTypeScriptAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plTypeScriptAssetProperties, plReflectedClass);

public:
  plTypeScriptAssetProperties();
  ~plTypeScriptAssetProperties();

  plString m_sScriptFile;

  plDynamicArray<plTypeScriptParameterNumber> m_NumberParameters;
  plDynamicArray<plTypeScriptParameterBool> m_BoolParameters;
  plDynamicArray<plTypeScriptParameterString> m_StringParameters;
  plDynamicArray<plTypeScriptParameterVec3> m_Vec3Parameters;
  plDynamicArray<plTypeScriptParameterColor> m_ColorParameters;
};
