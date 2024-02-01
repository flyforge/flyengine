
#pragma once

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

class PL_RENDERERFOUNDATION_DLL plGALComputeCommandEncoder : public plGALCommandEncoder
{
public:
  plGALComputeCommandEncoder(plGALDevice& ref_device, plGALCommandEncoderState& ref_state, plGALCommandEncoderCommonPlatformInterface& ref_commonImpl, plGALCommandEncoderComputePlatformInterface& ref_computeImpl);
  virtual ~plGALComputeCommandEncoder();

  // Dispatch

  plResult Dispatch(plUInt32 uiThreadGroupCountX, plUInt32 uiThreadGroupCountY, plUInt32 uiThreadGroupCountZ);
  plResult DispatchIndirect(plGALBufferHandle hIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes);

  virtual void ClearStatisticsCounters() override;

private:
  void CountDispatchCall() { m_uiDispatchCalls++; }

  // Statistic variables
  plUInt32 m_uiDispatchCalls = 0;

  plGALCommandEncoderComputePlatformInterface& m_ComputeImpl;
};
