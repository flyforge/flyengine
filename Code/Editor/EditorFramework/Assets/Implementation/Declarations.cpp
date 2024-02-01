#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/Declarations.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plTransformResult, 1)
  PL_ENUM_CONSTANT(plTransformResult::Success),
  PL_ENUM_CONSTANT(plTransformResult::Failure),
  PL_ENUM_CONSTANT(plTransformResult::NeedsImport),
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_TYPE(plTransformStatus, plNoBase, 1, plRTTIDefaultAllocator<plTransformStatus>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("Result", plTransformResult, m_Result),
    PL_MEMBER_PROPERTY("Message", m_sMessage),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on
