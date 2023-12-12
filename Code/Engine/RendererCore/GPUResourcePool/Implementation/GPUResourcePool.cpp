#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
#  include <Foundation/Utilities/Stats.h>
#endif

plGPUResourcePool* plGPUResourcePool::s_pDefaultInstance = nullptr;

plGPUResourcePool::plGPUResourcePool()
{
  m_pDevice = plGALDevice::GetDefaultDevice();

  m_GALDeviceEventSubscriptionID = m_pDevice->m_Events.AddEventHandler(plMakeDelegate(&plGPUResourcePool::GALDeviceEventHandler, this));
}

plGPUResourcePool::~plGPUResourcePool()
{
  m_pDevice->m_Events.RemoveEventHandler(m_GALDeviceEventSubscriptionID);
  if (!m_TexturesInUse.IsEmpty())
  {
    plLog::SeriousWarning("Destructing a GPU resource pool of which textures are still in use!");
  }

  // Free remaining resources
  RunGC(0);
}

plGALTextureHandle plGPUResourcePool::GetRenderTarget(const plGALTextureCreationDescription& textureDesc)
{
  PLASMA_LOCK(m_Lock);

  if (!textureDesc.m_bCreateRenderTarget)
  {
    plLog::Error("Texture description for render target usage has not set bCreateRenderTarget!");
    return plGALTextureHandle();
  }

  const plUInt32 uiTextureDescHash = textureDesc.CalculateHash();

  // Check if there is a fitting texture available
  auto it = m_AvailableTextures.Find(uiTextureDescHash);
  if (it.IsValid())
  {
    plDynamicArray<TextureHandleWithAge>& textures = it.Value();
    if (!textures.IsEmpty())
    {
      plGALTextureHandle hTexture = textures.PeekBack().m_hTexture;
      textures.PopBack();

      PLASMA_ASSERT_DEV(m_pDevice->GetTexture(hTexture) != nullptr, "Invalid texture in resource pool");

      m_TexturesInUse.Insert(hTexture);

      return hTexture;
    }
  }

  // Since we found no matching texture we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  plGALTextureHandle hNewTexture = m_pDevice->CreateTexture(textureDesc);

  if (hNewTexture.IsInvalidated())
  {
    plLog::Error("GPU resource pool couldn't create new texture for given desc (size: {0} x {1}, format: {2})", textureDesc.m_uiWidth,
      textureDesc.m_uiHeight, textureDesc.m_Format);
    return plGALTextureHandle();
  }

  // Also track the new created texture
  m_TexturesInUse.Insert(hNewTexture);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForTexture(textureDesc);

  UpdateMemoryStats();

  return hNewTexture;
}

plGALTextureHandle plGPUResourcePool::GetRenderTarget(
  plUInt32 uiWidth, plUInt32 uiHeight, plGALResourceFormat::Enum format, plGALMSAASampleCount::Enum sampleCount, plUInt32 uiSliceColunt)
{
  plGALTextureCreationDescription TextureDesc;
  TextureDesc.m_bCreateRenderTarget = true;
  TextureDesc.m_bAllowShaderResourceView = true;
  TextureDesc.m_Format = format;
  TextureDesc.m_Type = plGALTextureType::Texture2D;
  TextureDesc.m_uiWidth = uiWidth;
  TextureDesc.m_uiHeight = uiHeight;
  TextureDesc.m_SampleCount = sampleCount;
  TextureDesc.m_uiArraySize = uiSliceColunt;

  return GetRenderTarget(TextureDesc);
}

void plGPUResourcePool::ReturnRenderTarget(plGALTextureHandle hRenderTarget)
{
  PLASMA_LOCK(m_Lock);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_TexturesInUse.Contains(hRenderTarget))
  {
    plLog::Error("Returning a texture to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

#endif

  m_TexturesInUse.Remove(hRenderTarget);

  if (const plGALTexture* pTexture = m_pDevice->GetTexture(hRenderTarget))
  {
    const plUInt32 uiTextureDescHash = pTexture->GetDescription().CalculateHash();

    auto it = m_AvailableTextures.Find(uiTextureDescHash);
    if (!it.IsValid())
    {
      it = m_AvailableTextures.Insert(uiTextureDescHash, plDynamicArray<TextureHandleWithAge>());
    }

    it.Value().PushBack({hRenderTarget, plRenderWorld::GetFrameCounter()});
  }
}

plGALBufferHandle plGPUResourcePool::GetBuffer(const plGALBufferCreationDescription& bufferDesc)
{
  PLASMA_LOCK(m_Lock);

  const plUInt32 uiBufferDescHash = bufferDesc.CalculateHash();

  // Check if there is a fitting buffer available
  auto it = m_AvailableBuffers.Find(uiBufferDescHash);
  if (it.IsValid())
  {
    plDynamicArray<BufferHandleWithAge>& buffers = it.Value();
    if (!buffers.IsEmpty())
    {
      plGALBufferHandle hBuffer = buffers.PeekBack().m_hBuffer;
      buffers.PopBack();

      PLASMA_ASSERT_DEV(m_pDevice->GetBuffer(hBuffer) != nullptr, "Invalid buffer in resource pool");

      m_BuffersInUse.Insert(hBuffer);

      return hBuffer;
    }
  }

  // Since we found no matching buffer we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  plGALBufferHandle hNewBuffer = m_pDevice->CreateBuffer(bufferDesc);

  if (hNewBuffer.IsInvalidated())
  {
    plLog::Error("GPU resource pool couldn't create new buffer for given desc (size: {0})", bufferDesc.m_uiTotalSize);
    return plGALBufferHandle();
  }

  // Also track the new created buffer
  m_BuffersInUse.Insert(hNewBuffer);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForBuffer(bufferDesc);

  UpdateMemoryStats();

  return hNewBuffer;
}

void plGPUResourcePool::ReturnBuffer(plGALBufferHandle hBuffer)
{
  PLASMA_LOCK(m_Lock);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_BuffersInUse.Contains(hBuffer))
  {
    plLog::Error("Returning a buffer to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

#endif

  m_BuffersInUse.Remove(hBuffer);

  if (const plGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer))
  {
    const plUInt32 uiBufferDescHash = pBuffer->GetDescription().CalculateHash();

    auto it = m_AvailableBuffers.Find(uiBufferDescHash);
    if (!it.IsValid())
    {
      it = m_AvailableBuffers.Insert(uiBufferDescHash, plDynamicArray<BufferHandleWithAge>());
    }

    it.Value().PushBack({hBuffer, plRenderWorld::GetFrameCounter()});
  }
}

void plGPUResourcePool::RunGC(plUInt32 uiMinimumAge)
{
  PLASMA_LOCK(m_Lock);

  PLASMA_PROFILE_SCOPE("RunGC");
  plUInt64 uiCurrentFrame = plRenderWorld::GetFrameCounter();
  // Destroy all available textures older than uiMinimumAge frames
  {
    for (auto it = m_AvailableTextures.GetIterator(); it.IsValid();)
    {
      auto& textures = it.Value();
      for (plInt32 i = (plInt32)textures.GetCount() - 1; i >= 0; i--)
      {
        TextureHandleWithAge& texture = textures[i];
        if (texture.m_uiLastUsed + uiMinimumAge <= uiCurrentFrame)
        {
          if (const plGALTexture* pTexture = m_pDevice->GetTexture(texture.m_hTexture))
          {
            m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForTexture(pTexture->GetDescription());
          }

          m_pDevice->DestroyTexture(texture.m_hTexture);
          textures.RemoveAtAndCopy(i);
        }
        else
        {
          // The available textures are used as a stack. Thus they are ordered by last used.
          break;
        }
      }
      if (textures.IsEmpty())
      {
        auto itCopy = it;
        ++it;
        m_AvailableTextures.Remove(itCopy);
      }
      else
      {
        ++it;
      }
    }
  }

  // Destroy all available buffers older than uiMinimumAge frames
  {
    for (auto it = m_AvailableBuffers.GetIterator(); it.IsValid();)
    {
      auto& buffers = it.Value();
      for (plInt32 i = (plInt32)buffers.GetCount() - 1; i >= 0; i--)
      {
        BufferHandleWithAge& buffer = buffers[i];
        if (buffer.m_uiLastUsed + uiMinimumAge <= uiCurrentFrame)
        {
          if (const plGALBuffer* pBuffer = m_pDevice->GetBuffer(buffer.m_hBuffer))
          {
            m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForBuffer(pBuffer->GetDescription());
          }

          m_pDevice->DestroyBuffer(buffer.m_hBuffer);
          buffers.RemoveAtAndCopy(i);
        }
        else
        {
          // The available buffers are used as a stack. Thus they are ordered by last used.
          break;
        }
      }
      if (buffers.IsEmpty())
      {
        auto itCopy = it;
        ++it;
        m_AvailableBuffers.Remove(itCopy);
      }
      else
      {
        ++it;
      }
    }
  }

  m_uiNumAllocationsSinceLastGC = 0;

  UpdateMemoryStats();
}



plGPUResourcePool* plGPUResourcePool::GetDefaultInstance()
{
  return s_pDefaultInstance;
}

void plGPUResourcePool::SetDefaultInstance(plGPUResourcePool* pDefaultInstance)
{
  PLASMA_DEFAULT_DELETE(s_pDefaultInstance);
  s_pDefaultInstance = pDefaultInstance;
}


void plGPUResourcePool::CheckAndPotentiallyRunGC()
{
  if ((m_uiNumAllocationsSinceLastGC >= m_uiNumAllocationsThresholdForGC) || (m_uiCurrentlyAllocatedMemory >= m_uiMemoryThresholdForGC))
  {
    // Only try to collect resources unused for 3 or more frames. Using a smaller number will result in constant memory thrashing.
    RunGC(3);
  }
}

void plGPUResourcePool::UpdateMemoryStats() const
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)

  float fMegaBytes = float(m_uiCurrentlyAllocatedMemory) / (1024.0f * 1024.0f);

  plStringBuilder sOut;
  sOut.Format("{0} (Mb)", plArgF(fMegaBytes, 4));
  plStats::SetStat("GPU Resource Pool/Memory Consumption", sOut.GetData());

#endif
}

void plGPUResourcePool::GALDeviceEventHandler(const plGALDeviceEvent& e)
{
  if (e.m_Type == plGALDeviceEvent::AfterEndFrame)
  {
    ++m_uiFramesSinceLastGC;
    if (m_uiFramesSinceLastGC >= m_uiFramesThresholdSinceLastGC)
    {
      m_uiFramesSinceLastGC = 0;
      RunGC(10);
    }
  }
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_GPUResourcePool_Implementation_GPUResourcePool);
