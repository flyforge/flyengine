#include <Core/CorePCH.h>

#include <Core/Messages/ApplyOnlyToMessage.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgOnlyApplyToObject);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgOnlyApplyToObject, 1, plRTTIDefaultAllocator<plMsgOnlyApplyToObject>)
{
  ///\todo enable this once we have object reference properties
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Object", m_hObject),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


PLASMA_STATICLINK_FILE(Core, Core_Messages_Implementation_ApplyOnlyToMessage);
