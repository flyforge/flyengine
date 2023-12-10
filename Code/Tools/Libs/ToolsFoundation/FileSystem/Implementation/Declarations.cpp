#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <ToolsFoundation/FileSystem/Declarations.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plFileStatus, plNoBase, 3, plRTTIDefaultAllocator<plFileStatus>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("LastModified", m_LastModified),
    PLASMA_MEMBER_PROPERTY("Hash", m_uiHash),
    PLASMA_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on
