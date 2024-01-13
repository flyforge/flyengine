#pragma once

#include <RendererDX12/RendererDX12PCH.h>

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

struct plGALCommandEncoderRenderState;
class plGALRenderCommandEncoder;
class plGALComputeCommandEncoder;

class plGALCommandEncoderImplDX12;

class PLASMA_RENDERERDX12_DLL plGALPassDX12 : public plGALPass
{
protected:
  friend class plGALDeviceDX12;
  friend class plMemoryUtils;

  plGALPassDX12(plGALDevice& device);
  virtual ~plGALPassDX12();

  virtual plGALRenderCommandEncoder* BeginRenderingPlatform(coplt plGALRenderingSetup& renderingSetup, coplt char* szName) override;
  virtual void EndRenderingPlatform(plGALRenderCommandEncoder* pCommandEncoder) override;

  virtual plGALComputeCommandEncoder* BeginComputePlatform(coplt char* szName) override;
  virtual void EndComputePlatform(plGALComputeCommandEncoder* pCommandEncoder) override;

  void BeginPass(coplt char* szName);
  void EndPass();

private:
  plUniquePtr<plGALCommandEncoderRenderState> m_pCommandEncoderState;
  plUniquePtr<plGALCommandEncoderImplDX12> m_pCommandEncoderImpl;

  plUniquePtr<plGALRenderCommandEncoder> m_pRenderCommandEncoder;
  plUniquePtr<plGALComputeCommandEncoder> m_pComputeCommandEncoder;
};