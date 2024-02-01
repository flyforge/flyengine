#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/PassVulkan.h>

plGALPassVulkan::plGALPassVulkan(plGALDevice& device)
  : plGALPass(device)
{
  m_pCommandEncoderState = PL_DEFAULT_NEW(plGALCommandEncoderRenderState);
  m_pCommandEncoderImpl = PL_DEFAULT_NEW(plGALCommandEncoderImplVulkan, static_cast<plGALDeviceVulkan&>(device));

  m_pRenderCommandEncoder = PL_DEFAULT_NEW(plGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = PL_DEFAULT_NEW(plGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
}

plGALPassVulkan::~plGALPassVulkan() = default;

void plGALPassVulkan::Reset()
{
  m_pCommandEncoderImpl->Reset();
  m_pRenderCommandEncoder->InvalidateState();
  m_pComputeCommandEncoder->InvalidateState();
}

void plGALPassVulkan::MarkDirty()
{
  m_pCommandEncoderImpl->MarkDirty();
}

void plGALPassVulkan::SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, plPipelineBarrierVulkan* pipelineBarrier)
{
  m_pCommandEncoderImpl->SetCurrentCommandBuffer(commandBuffer, pipelineBarrier);
}

plGALRenderCommandEncoder* plGALPassVulkan::BeginRenderingPlatform(const plGALRenderingSetup& renderingSetup, const char* szName)
{
  auto& deviceVulkan = static_cast<plGALDeviceVulkan&>(m_Device);
  deviceVulkan.GetCurrentCommandBuffer();

  m_pCommandEncoderImpl->BeginRendering(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void plGALPassVulkan::EndRenderingPlatform(plGALRenderCommandEncoder* pCommandEncoder)
{
  PL_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndRendering();
}

plGALComputeCommandEncoder* plGALPassVulkan::BeginComputePlatform(const char* szName)
{
  auto& deviceVulkan = static_cast<plGALDeviceVulkan&>(m_Device);
  deviceVulkan.GetCurrentCommandBuffer();

  m_pCommandEncoderImpl->BeginCompute();

  return m_pComputeCommandEncoder.Borrow();
}

void plGALPassVulkan::EndComputePlatform(plGALComputeCommandEncoder* pCommandEncoder)
{
  PL_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndCompute();
}
