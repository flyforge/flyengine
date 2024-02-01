#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameter, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameterNumber, 1, plRTTIDefaultAllocator<plTypeScriptParameterNumber>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameterBool, 1, plRTTIDefaultAllocator<plTypeScriptParameterBool>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameterString, 1, plRTTIDefaultAllocator<plTypeScriptParameterString>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameterVec3, 1, plRTTIDefaultAllocator<plTypeScriptParameterVec3>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameterColor, 1, plRTTIDefaultAllocator<plTypeScriptParameterColor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptAssetProperties, 1, plRTTIDefaultAllocator<plTypeScriptAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ScriptFile", m_sScriptFile)->AddAttributes(new plFileBrowserAttribute("Select Script", "*.ts")),
    PL_ARRAY_MEMBER_PROPERTY("NumberParameters", m_NumberParameters),
    PL_ARRAY_MEMBER_PROPERTY("BoolParameters", m_BoolParameters),
    PL_ARRAY_MEMBER_PROPERTY("StringParameters", m_StringParameters),
    PL_ARRAY_MEMBER_PROPERTY("Vec3Parameters", m_Vec3Parameters),
    PL_ARRAY_MEMBER_PROPERTY("ColorParameters", m_ColorParameters),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTypeScriptAssetProperties::plTypeScriptAssetProperties() = default;
plTypeScriptAssetProperties::~plTypeScriptAssetProperties() = default;
