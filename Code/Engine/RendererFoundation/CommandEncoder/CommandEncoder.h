
#pragma once

#include <Foundation/Threading/ThreadUtils.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALCommandEncoder
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plGALCommandEncoder);

public:
  // State setting functions

  void SetShader(plGALShaderHandle hShader);

  void SetConstantBuffer(plUInt32 uiSlot, plGALBufferHandle hBuffer);
  void SetSamplerState(plGALShaderStage::Enum stage, plUInt32 uiSlot, plGALSamplerStateHandle hSamplerState);
  void SetResourceView(plGALShaderStage::Enum stage, plUInt32 uiSlot, plGALResourceViewHandle hResourceView);
  void SetUnorderedAccessView(plUInt32 uiSlot, plGALUnorderedAccessViewHandle hUnorderedAccessView);

  // Returns whether a resource view has been unset for the given resource
  bool UnsetResourceViews(const plGALResourceBase* pResource);
  bool UnsetResourceViews(plGALResourceViewHandle hResourceView);
  // Returns whether a unordered access view has been unset for the given resource
  bool UnsetUnorderedAccessViews(const plGALResourceBase* pResource);
  bool UnsetUnorderedAccessViews(plGALUnorderedAccessViewHandle hUnorderedAccessView);

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

  PLASMA_ALWAYS_INLINE plGALDevice& GetDevice() { return m_Device; }
  // Don't use light hearted ;)
  void InvalidateState();

protected:
  friend class plGALDevice;

  plGALCommandEncoder(plGALDevice& device, plGALCommandEncoderState& state, plGALCommandEncoderCommonPlatformInterface& commonImpl);
  virtual ~plGALCommandEncoder();


  void AssertRenderingThread()
  {
    PLASMA_ASSERT_DEV(plThreadUtils::IsMainThread(), "This function can only be executed on the main thread.");
  }

  void CountStateChange() { m_uiStateChanges++; }
  void CountRedundantStateChange() { m_uiRedundantStateChanges++; }

private:
  friend class plMemoryUtils;

  // Parent Device
  plGALDevice& m_Device;

  // Statistic variables
  plUInt32 m_uiStateChanges = 0;
  plUInt32 m_uiRedundantStateChanges = 0;

  plGALCommandEncoderState& m_State;

  plGALCommandEncoderCommonPlatformInterface& m_CommonImpl;
};
