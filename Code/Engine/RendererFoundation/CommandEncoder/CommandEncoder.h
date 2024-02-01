
#pragma once

#include <Foundation/Threading/ThreadUtils.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

class PL_RENDERERFOUNDATION_DLL plGALCommandEncoder
{
  PL_DISALLOW_COPY_AND_ASSIGN(plGALCommandEncoder);

public:
  // State setting functions

  void SetShader(plGALShaderHandle hShader);

  void SetConstantBuffer(const plShaderResourceBinding& binding, plGALBufferHandle hBuffer);
  void SetSamplerState(const plShaderResourceBinding& binding, plGALSamplerStateHandle hSamplerState);
  void SetResourceView(const plShaderResourceBinding& binding, plGALResourceViewHandle hResourceView);
  void SetUnorderedAccessView(const plShaderResourceBinding& binding, plGALUnorderedAccessViewHandle hUnorderedAccessView);
  void SetPushConstants(plArrayPtr<const plUInt8> data);

  // Query functions

  void BeginQuery(plGALQueryHandle hQuery);
  void EndQuery(plGALQueryHandle hQuery);

  /// \return Success if retrieving the query succeeded.
  plResult GetQueryResult(plGALQueryHandle hQuery, plUInt64& ref_uiQueryResult);

  // Timestamp functions

  plGALTimestampHandle InsertTimestamp();

  // Resource functions

  /// Clears an unordered access view with a float value.
  void ClearUnorderedAccessView(plGALUnorderedAccessViewHandle hUnorderedAccessView, plVec4 vClearValues);

  /// Clears an unordered access view with an int value.
  void ClearUnorderedAccessView(plGALUnorderedAccessViewHandle hUnorderedAccessView, plVec4U32 vClearValues);

  void CopyBuffer(plGALBufferHandle hDest, plGALBufferHandle hSource);
  void CopyBufferRegion(plGALBufferHandle hDest, plUInt32 uiDestOffset, plGALBufferHandle hSource, plUInt32 uiSourceOffset, plUInt32 uiByteCount);
  void UpdateBuffer(plGALBufferHandle hDest, plUInt32 uiDestOffset, plArrayPtr<const plUInt8> sourceData, plGALUpdateMode::Enum updateMode = plGALUpdateMode::Discard);

  void CopyTexture(plGALTextureHandle hDest, plGALTextureHandle hSource);
  void CopyTextureRegion(plGALTextureHandle hDest, const plGALTextureSubresource& destinationSubResource, const plVec3U32& vDestinationPoint, plGALTextureHandle hSource, const plGALTextureSubresource& sourceSubResource, const plBoundingBoxu32& box);

  void UpdateTexture(plGALTextureHandle hDest, const plGALTextureSubresource& destinationSubResource, const plBoundingBoxu32& destinationBox, const plGALSystemMemoryDescription& sourceData);

  void ResolveTexture(plGALTextureHandle hDest, const plGALTextureSubresource& destinationSubResource, plGALTextureHandle hSource, const plGALTextureSubresource& sourceSubResource);

  void ReadbackTexture(plGALTextureHandle hTexture);
  void CopyTextureReadbackResult(plGALTextureHandle hTexture, plArrayPtr<plGALTextureSubresource> sourceSubResource, plArrayPtr<plGALSystemMemoryDescription> targetData);

  void GenerateMipMaps(plGALResourceViewHandle hResourceView);

  // Misc

  void Flush();

  // Debug helper functions

  void PushMarker(const char* szMarker);
  void PopMarker();
  void InsertEventMarker(const char* szMarker);

  virtual void ClearStatisticsCounters();

  PL_ALWAYS_INLINE plGALDevice& GetDevice() { return m_Device; }
  // Don't use light hearted ;)
  void InvalidateState();

protected:
  friend class plGALDevice;

  plGALCommandEncoder(plGALDevice& device, plGALCommandEncoderState& state, plGALCommandEncoderCommonPlatformInterface& commonImpl);
  virtual ~plGALCommandEncoder();


  void AssertRenderingThread()
  {
    PL_ASSERT_DEV(plThreadUtils::IsMainThread(), "This function can only be executed on the main thread.");
  }

private:
  friend class plMemoryUtils;

  // Parent Device
  plGALDevice& m_Device;
  plGALCommandEncoderState& m_State;
  plGALCommandEncoderCommonPlatformInterface& m_CommonImpl;
};
