#include <Core/System/Implementation/null/InputDevice_null.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStandardInputDevice, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStandardInputDevice::plStandardInputDevice(plUInt32 uiWindowNumber) {}
plStandardInputDevice::~plStandardInputDevice() = default;

void plStandardInputDevice::SetShowMouseCursor(bool bShow) {}

void plStandardInputDevice::SetClipMouseCursor(plMouseCursorClipMode::Enum mode) {}

plMouseCursorClipMode::Enum plStandardInputDevice::GetClipMouseCursor() const
{
  return plMouseCursorClipMode::Default;
}

void plStandardInputDevice::InitializeDevice() {}

void plStandardInputDevice::RegisterInputSlots() {}
