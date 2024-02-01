#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class PL_RENDERERCORE_DLL plConstantBufferStorageBase
{
protected:
  friend class plRenderContext;
  friend class plMemoryUtils;

  plConstantBufferStorageBase(plUInt32 uiSizeInBytes);
  ~plConstantBufferStorageBase();

public:
  plArrayPtr<plUInt8> GetRawDataForWriting();
  plArrayPtr<const plUInt8> GetRawDataForReading() const;

  void UploadData(plGALCommandEncoder* pCommandEncoder);

  PL_ALWAYS_INLINE plGALBufferHandle GetGALBufferHandle() const { return m_hGALConstantBuffer; }

protected:
  bool m_bHasBeenModified = false;
  plUInt32 m_uiLastHash = 0;
  plGALBufferHandle m_hGALConstantBuffer;

  plArrayPtr<plUInt8> m_Data;
};

template <typename T>
class plConstantBufferStorage : public plConstantBufferStorageBase
{
public:
  PL_FORCE_INLINE T& GetDataForWriting()
  {
    plArrayPtr<plUInt8> rawData = GetRawDataForWriting();
    PL_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<T*>(rawData.GetPtr());
  }

  PL_FORCE_INLINE const T& GetDataForReading() const
  {
    plArrayPtr<const plUInt8> rawData = GetRawDataForReading();
    PL_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<const T*>(rawData.GetPtr());
  }
};

using plConstantBufferStorageId = plGenericId<24, 8>;

class plConstantBufferStorageHandle
{
  PL_DECLARE_HANDLE_TYPE(plConstantBufferStorageHandle, plConstantBufferStorageId);

  friend class plRenderContext;
};
