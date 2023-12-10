
#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

struct plGALCommandEncoderRenderState;
class plGALRenderCommandEncoder;
class plGALComputeCommandEncoder;
class plGALCommandEncoderImplVulkan;

class plGALPassVulkan : public plGALPass
{
protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;
  void Reset();
  void MarkDirty();
  void SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, plPipelineBarrierVulkan* pipelineBarrier);

  virtual plGALRenderCommandEncoder* BeginRenderingPlatform(const plGALRenderingSetup& renderingSetup, const char* szName) override;
  virtual void EndRenderingPlatform(plGALRenderCommandEncoder* pCommandEncoder) override;

  virtual plGALComputeCommandEncoder* BeginComputePlatform(const char* szName) override;
  virtual void EndComputePlatform(plGALComputeCommandEncoder* pCommandEncoder) override;

  plGALPassVulkan(plGALDevice& device);
  virtual ~plGALPassVulkan();

  void BeginPass(const char* szName);
  void EndPass();

private:
  plUniquePtr<plGALCommandEncoderRenderState> m_pCommandEncoderState;
  plUniquePtr<plGALCommandEncoderImplVulkan> m_pCommandEncoderImpl;

  plUniquePtr<plGALRenderCommandEncoder> m_pRenderCommandEncoder;
  plUniquePtr<plGALComputeCommandEncoder> m_pComputeCommandEncoder;
};
