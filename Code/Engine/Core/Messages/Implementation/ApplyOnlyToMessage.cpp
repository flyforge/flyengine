#include <Core/CorePCH.h>

#include <Core/Messages/ApplyOnlyToMessage.h>

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgOnlyApplyToObject);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgOnlyApplyToObject, 1, plRTTIDefaultAllocator<plMsgOnlyApplyToObject>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Object", m_hObject),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


PL_STATICLINK_FILE(Core, Core_Messages_Implementation_ApplyOnlyToMessage);
