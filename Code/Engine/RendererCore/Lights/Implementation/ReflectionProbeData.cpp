#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plReflectionProbeMode, 1)
  PLASMA_BITFLAGS_CONSTANTS(plReflectionProbeMode::Static, plReflectionProbeMode::Dynamic)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_BITFLAGS(plProbeFlags, 1)
  PLASMA_BITFLAGS_CONSTANTS(plProbeFlags::SkyLight, plProbeFlags::HasCustomCubeMap, plProbeFlags::Sphere, plProbeFlags::Box, plProbeFlags::Dynamic)
PLASMA_END_STATIC_REFLECTED_BITFLAGS;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plReflectionProbeRenderData, 1, plRTTIDefaultAllocator<plReflectionProbeRenderData>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeData);
