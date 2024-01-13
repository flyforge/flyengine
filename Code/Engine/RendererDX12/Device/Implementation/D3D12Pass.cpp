#include <RendererDX12/Device/D3D12Pass.h>
#include <RendererDX12/CommandEncoder/CommandEncoderImplD3D12.h>
#include <RendererDX12/Device/D3D12Device.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

plGALPassDX12::plGALPassDX12(plGALDevice& device)
  : plGALPass(device)
{
  m_pCommandEncoderState = PLASMA_DEFAULT_NEW(plGALCommandEncoderRenderState);
  m_pCommandEncoderImpl = PLASMA_DEFAULT_NEW(plGALCommandEncoderImplDX12, static_cast<plGALDeviceDX12&>(device));

  m_pRenderCommandEncoder = PLASMA_DEFAULT_NEW(plGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = PLASMA_DEFAULT_NEW(plGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pRenderCommandEncoder.Borrow();
}

plGALPassDX12::~plGALPassDX12()
{
}

plGALRenderCommandEncoder* plGALPassDX12::BeginRenderingPlatform(const plGALRenderingSetup& renderingSetup, const char* szName)
{
  m_pCommandEncoderImpl->BeginRendering(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void plGALPassDX12::EndRenderingPlatform(plGALRenderCommandEncoder* pCommandEncoder)
{
  PLASMA_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

plGALComputeCommandEncoder* plGALPassDX12::BeginComputePlatform(const char* szName)
{
  m_pCommandEncoderImpl->BeginCompute();
  return m_pComputeCommandEncoder.Borrow();
}

void plGALPassDX12::EndComputePlatform(plGALComputeCommandEncoder* pCommandEncoder)
{
  PLASMA_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

void plGALPassDX12::BeginPass(const char* szName)
{

}

void plGALPassDX12::EndPass()
{
}