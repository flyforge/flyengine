#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetProcessorMessages.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcessAssetMsg, 1, plRTTIDefaultAllocator<plProcessAssetMsg>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("AssetGuid", m_AssetGuid),
    PL_MEMBER_PROPERTY("AssetHash", m_AssetHash),
    PL_MEMBER_PROPERTY("ThumbHash", m_ThumbHash),
    PL_MEMBER_PROPERTY("AssetPath", m_sAssetPath),
    PL_MEMBER_PROPERTY("Platform", m_sPlatform),
    PL_ARRAY_MEMBER_PROPERTY("DepRefHull", m_DepRefHull),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcessAssetResponseMsg, 1, plRTTIDefaultAllocator<plProcessAssetResponseMsg>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Status", m_Status),
    PL_ARRAY_MEMBER_PROPERTY("LogEntries", m_LogEntries),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
