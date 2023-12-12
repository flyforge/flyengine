#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>

plConstantBufferStorageBase::plConstantBufferStorageBase(plUInt32 uiSizeInBytes)
  : m_bHasBeenModified(false)
  , m_uiLastHash(0)
{
  m_Data = plMakeArrayPtr(static_cast<plUInt8*>(plFoundation::GetAlignedAllocator()->Allocate(uiSizeInBytes, 16)), uiSizeInBytes);
  plMemoryUtils::ZeroFill(m_Data.GetPtr(), m_Data.GetCount());

  m_hGALConstantBuffer = plGALDevice::GetDefaultDevice()->CreateConstantBuffer(uiSizeInBytes);
}

plConstantBufferStorageBase::~plConstantBufferStorageBase()
{
  plGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGALConstantBuffer);

  plFoundation::GetAlignedAllocator()->Deallocate(m_Data.GetPtr());
  m_Data.Clear();
}

plArrayPtr<plUInt8> plConstantBufferStorageBase::GetRawDataForWriting()
{
  m_bHasBeenModified = true;
  return m_Data;
}

plArrayPtr<const plUInt8> plConstantBufferStorageBase::GetRawDataForReading() const
{
  return m_Data;
}

void plConstantBufferStorageBase::UploadData(plGALCommandEncoder* pCommandEncoder)
{
  if (!m_bHasBeenModified)
    return;

  m_bHasBeenModified = false;

  plUInt32 uiNewHash = plHashingUtils::xxHash32(m_Data.GetPtr(), m_Data.GetCount());
  if (m_uiLastHash != uiNewHash)
  {
    pCommandEncoder->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data);
    m_uiLastHash = uiNewHash;
  }
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ConstantBufferStorage);
