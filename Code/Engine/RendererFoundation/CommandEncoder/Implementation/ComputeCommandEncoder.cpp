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

void plGALComputeCommandEncoder::Dispatch(plUInt32 uiThreadGroupCountX, plUInt32 uiThreadGroupCountY, plUInt32 uiThreadGroupCountZ)
{
  AssertRenderingThread();

  PLASMA_ASSERT_DEBUG(uiThreadGroupCountX > 0 && uiThreadGroupCountY > 0 && uiThreadGroupCountZ > 0, "Thread group counts of zero are not meaningful. Did you mean 1?");

  /// \todo Assert for compute

  m_ComputeImpl.DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  CountDispatchCall();
}

void plGALComputeCommandEncoder::DispatchIndirect(plGALBufferHandle hIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for compute
  /// \todo Assert for indirect dispatch
  /// \todo Assert offset < buffer size

  const plGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  PLASMA_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  m_ComputeImpl.DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDispatchCall();
}

void plGALComputeCommandEncoder::ClearStatisticsCounters()
{
  plGALCommandEncoder::ClearStatisticsCounters();

  m_uiDispatchCalls = 0;
}


PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_CommandEncoder_Implementation_ComputeCommandEncoder);
