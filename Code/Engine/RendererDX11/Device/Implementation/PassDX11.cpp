#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/PassDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

plGALPassDX11::plGALPassDX11(plGALDevice& device)
  : plGALPass(device)
{
  m_pCommandEncoderState = PL_DEFAULT_NEW(plGALCommandEncoderRenderState);
  m_pCommandEncoderImpl = PL_DEFAULT_NEW(plGALCommandEncoderImplDX11, static_cast<plGALDeviceDX11&>(device));

  m_pRenderCommandEncoder = PL_DEFAULT_NEW(plGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = PL_DEFAULT_NEW(plGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pRenderCommandEncoder.Borrow();
}

plGALPassDX11::~plGALPassDX11() = default;

plGALRenderCommandEncoder* plGALPassDX11::BeginRenderingPlatform(const plGALRenderingSetup& renderingSetup, const char* szName)
{
  m_pCommandEncoderImpl->BeginRendering(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void plGALPassDX11::EndRenderingPlatform(plGALRenderCommandEncoder* pCommandEncoder)
{
  PL_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

plGALComputeCommandEncoder* plGALPassDX11::BeginComputePlatform(const char* szName)
{
  m_pCommandEncoderImpl->BeginCompute();
  return m_pComputeCommandEncoder.Borrow();
}

void plGALPassDX11::EndComputePlatform(plGALComputeCommandEncoder* pCommandEncoder)
{
  PL_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

void plGALPassDX11::BeginPass(const char* szName)
{
}

void plGALPassDX11::EndPass()
{
}
