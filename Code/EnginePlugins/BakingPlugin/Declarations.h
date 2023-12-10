#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>

namespace plBakingInternal
{
  struct Volume
  {
    plSimdMat4f m_GlobalToLocalTransform;
  };
} // namespace plBakingInternal
