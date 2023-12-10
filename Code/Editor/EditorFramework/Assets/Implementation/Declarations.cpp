#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/Declarations.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plTransformResult, 1)
  PLASMA_ENUM_CONSTANT(plTransformResult::Success),
  PLASMA_ENUM_CONSTANT(plTransformResult::Failure),
  PLASMA_ENUM_CONSTANT(plTransformResult::NeedsImport),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plTransformStatus, plNoBase, 1, plRTTIDefaultAllocator<plTransformStatus>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Result", plTransformResult, m_Result),
    PLASMA_MEMBER_PROPERTY("Message", m_sMessage),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on
