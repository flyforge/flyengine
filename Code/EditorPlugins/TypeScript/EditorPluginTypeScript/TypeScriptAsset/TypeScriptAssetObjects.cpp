#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameter, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sName),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameterNumber, 1, plRTTIDefaultAllocator<plTypeScriptParameterNumber>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameterBool, 1, plRTTIDefaultAllocator<plTypeScriptParameterBool>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameterString, 1, plRTTIDefaultAllocator<plTypeScriptParameterString>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameterVec3, 1, plRTTIDefaultAllocator<plTypeScriptParameterVec3>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptParameterColor, 1, plRTTIDefaultAllocator<plTypeScriptParameterColor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptAssetProperties, 1, plRTTIDefaultAllocator<plTypeScriptAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ScriptFile", m_sScriptFile)->AddAttributes(new plFileBrowserAttribute("Select Script", "*.ts")),
    PLASMA_ARRAY_MEMBER_PROPERTY("NumberParameters", m_NumberParameters),
    PLASMA_ARRAY_MEMBER_PROPERTY("BoolParameters", m_BoolParameters),
    PLASMA_ARRAY_MEMBER_PROPERTY("StringParameters", m_StringParameters),
    PLASMA_ARRAY_MEMBER_PROPERTY("Vec3Parameters", m_Vec3Parameters),
    PLASMA_ARRAY_MEMBER_PROPERTY("ColorParameters", m_ColorParameters),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTypeScriptAssetProperties::plTypeScriptAssetProperties() = default;
plTypeScriptAssetProperties::~plTypeScriptAssetProperties() = default;
