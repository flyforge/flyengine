#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

plGALComputeCommandEncoder::plGALComputeCommandEncoder(plGALDevice& ref_device, plGALCommandEncoderState& ref_state, plGALCommandEncoderCommonPlatformInterface& ref_commonImpl, plGALCommandEncoderComputePlatformInterface& ref_computeImpl)
  : plGALCommandEncoder(ref_device, ref_state, ref_commonImpl)
  , m_ComputeImpl(ref_computeImpl)
{
}

plGALComputeCommandEncoder::~plGALComputeCommandEncoder() = default;

plResult plGALComputeCommandEncoder::Dispatch(plUInt32 uiThreadGroupCountX, plUInt32 uiThreadGroupCountY, plUInt32 uiThreadGroupCountZ)
{
  AssertRenderingThread();

  PL_ASSERT_DEBUG(uiThreadGroupCountX > 0 && uiThreadGroupCountY > 0 && uiThreadGroupCountZ > 0, "Thread group counts of zero are not meaningful. Did you mean 1?");

  CountDispatchCall();
  return m_ComputeImpl.DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

plResult plGALComputeCommandEncoder::DispatchIndirect(plGALBufferHandle hIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();

  const plGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  PL_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  CountDispatchCall();
  return m_ComputeImpl.DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void plGALComputeCommandEncoder::ClearStatisticsCounters()
{
  plGALCommandEncoder::ClearStatisticsCounters();

  m_uiDispatchCalls = 0;
}


