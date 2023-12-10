
#pragma once

#include <RendererFoundation/Resources/RenderTargetSetup.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALPass
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plGALPass);

public:
  plGALRenderCommandEncoder* BeginRendering(const plGALRenderingSetup& renderingSetup, const char* szName = "");
  void EndRendering(plGALRenderCommandEncoder* pCommandEncoder);

  plGALComputeCommandEncoder* BeginCompute(const char* szName = "");
  void EndCompute(plGALComputeCommandEncoder* pCommandEncoder);

  // BeginRaytracing() could be here as well (would match Vulkan)

protected:
  virtual plGALRenderCommandEncoder* BeginRenderingPlatform(const plGALRenderingSetup& renderingSetup, const char* szName) = 0;
  virtual void EndRenderingPlatform(plGALRenderCommandEncoder* pCommandEncoder) = 0;

  virtual plGALComputeCommandEncoder* BeginComputePlatform(const char* szName) = 0;
  virtual void EndComputePlatform(plGALComputeCommandEncoder* pCommandEncoder) = 0;

  plGALPass(plGALDevice& device);
  virtual ~plGALPass();

  plGALDevice& m_Device;

  enum class CommandEncoderType
  {
    Invalid,
    Render,
    Compute
  };

  CommandEncoderType m_CurrentCommandEncoderType = CommandEncoderType::Invalid;
  bool m_bMarker = false;
};
