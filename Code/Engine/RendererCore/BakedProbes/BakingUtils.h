#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <RendererCore/Declarations.h>

using plCompressedSkyVisibility = plUInt32;

namespace plBakingUtils
{
  PL_RENDERERCORE_DLL plVec3 FibonacciSphere(plUInt32 uiSampleIndex, plUInt32 uiNumSamples);

  PL_RENDERERCORE_DLL plCompressedSkyVisibility CompressSkyVisibility(const plAmbientCube<float>& skyVisibility);
  PL_RENDERERCORE_DLL void DecompressSkyVisibility(plCompressedSkyVisibility compressedSkyVisibility, plAmbientCube<float>& out_skyVisibility);
} // namespace plBakingUtils
