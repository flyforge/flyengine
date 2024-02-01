#pragma once

#include <Core/CoreDLL.h>

class plImage;

/// \brief Base class for window output targets
///
/// A window output target is usually tied tightly to a window (\sa plWindowBase) and represents the
/// graphics APIs side of the render output.
/// E.g. in a DirectX implementation this would be a swapchain.
///
/// This interface provides the high level functionality that is needed by plGameApplication to work with
/// the render output.
class PL_CORE_DLL plWindowOutputTargetBase
{
public:
  virtual ~plWindowOutputTargetBase() = default;
  virtual void Present(bool bEnableVSync) = 0;
  virtual plResult CaptureImage(plImage& out_image) = 0;
};
