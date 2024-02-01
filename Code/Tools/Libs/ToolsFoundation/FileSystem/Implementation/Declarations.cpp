#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <ToolsFoundation/FileSystem/Declarations.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plFileStatus, plNoBase, 3, plRTTIDefaultAllocator<plFileStatus>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("LastModified", m_LastModified),
    PL_MEMBER_PROPERTY("Hash", m_uiHash),
    PL_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on
