
#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

struct plGALCommandEncoderRenderState;
class plGALRenderCommandEncoder;
class plGALComputeCommandEncoder;

class plGALCommandEncoderImplDX11;

class plGALPassDX11 : public plGALPass
{
protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALPassDX11(plGALDevice& device);
  virtual ~plGALPassDX11();

  virtual plGALRenderCommandEncoder* BeginRenderingPlatform(const plGALRenderingSetup& renderingSetup, const char* szName) override;
  virtual void EndRenderingPlatform(plGALRenderCommandEncoder* pCommandEncoder) override;

  virtual plGALComputeCommandEncoder* BeginComputePlatform(const char* szName) override;
  virtual void EndComputePlatform(plGALComputeCommandEncoder* pCommandEncoder) override;

  void BeginPass(const char* szName);
  void EndPass();

private:
  plUniquePtr<plGALCommandEncoderRenderState> m_pCommandEncoderState;
  plUniquePtr<plGALCommandEncoderImplDX11> m_pCommandEncoderImpl;

  plUniquePtr<plGALRenderCommandEncoder> m_pRenderCommandEncoder;
  plUniquePtr<plGALComputeCommandEncoder> m_pComputeCommandEncoder;
};
