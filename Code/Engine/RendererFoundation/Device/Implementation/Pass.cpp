#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Pass.h>

plGALRenderCommandEncoder* plGALPass::BeginRendering(const plGALRenderingSetup& renderingSetup, const char* szName /*= ""*/)
{
  PLASMA_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Render;

  plGALRenderCommandEncoder* pCommandEncoder = BeginRenderingPlatform(renderingSetup, szName);

  m_bMarker = !plStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    pCommandEncoder->PushMarker(szName);
  }

  return pCommandEncoder;
}

void plGALPass::EndRendering(plGALRenderCommandEncoder* pCommandEncoder)
{
  PLASMA_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Render, "BeginRendering has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    pCommandEncoder->PopMarker();
    m_bMarker = false;
  }

  EndRenderingPlatform(pCommandEncoder);
}

plGALComputeCommandEncoder* plGALPass::BeginCompute(const char* szName /*= ""*/)
{
  PLASMA_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Compute;

  plGALComputeCommandEncoder* pCommandEncoder = BeginComputePlatform(szName);

  m_bMarker = !plStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    pCommandEncoder->PushMarker(szName);
  }

  return pCommandEncoder;
}

void plGALPass::EndCompute(plGALComputeCommandEncoder* pCommandEncoder)
{
  PLASMA_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "BeginCompute has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    pCommandEncoder->PopMarker();
    m_bMarker = false;
  }

  EndComputePlatform(pCommandEncoder);
}

plGALPass::plGALPass(plGALDevice& device)
  : m_Device(device)
{
}

plGALPass::~plGALPass() = default;


PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_Pass);
