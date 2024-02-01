#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plReflectionProbeMode, 1)
  PL_BITFLAGS_CONSTANTS(plReflectionProbeMode::Static, plReflectionProbeMode::Dynamic)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plProbeFlags, 1)
  PL_BITFLAGS_CONSTANTS(plProbeFlags::SkyLight, plProbeFlags::HasCustomCubeMap, plProbeFlags::Sphere, plProbeFlags::Box, plProbeFlags::Dynamic)
PL_END_STATIC_REFLECTED_BITFLAGS;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plReflectionProbeRenderData, 1, plRTTIDefaultAllocator<plReflectionProbeRenderData>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PL_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeData);
