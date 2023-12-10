#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetProcessorMessages.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcessAssetMsg, 1, plRTTIDefaultAllocator<plProcessAssetMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("AssetGuid", m_AssetGuid),
    PLASMA_MEMBER_PROPERTY("AssetHash", m_AssetHash),
    PLASMA_MEMBER_PROPERTY("ThumbHash", m_ThumbHash),
    PLASMA_MEMBER_PROPERTY("AssetPath", m_sAssetPath),
    PLASMA_MEMBER_PROPERTY("Platform", m_sPlatform),
    PLASMA_ARRAY_MEMBER_PROPERTY("DepRefHull", m_DepRefHull),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcessAssetResponseMsg, 1, plRTTIDefaultAllocator<plProcessAssetResponseMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Status", m_Status),
    PLASMA_ARRAY_MEMBER_PROPERTY("LogEntries", m_LogEntries),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
